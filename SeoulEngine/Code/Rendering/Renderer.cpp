/**
 * \file Renderer.cpp
 * \brief Renderer handles high-level rendering flow.
 *
 * If the rendering system is split into a low-level and a high-level
 * component, Renderer can be considered the root of the high-level component
 * while RenderDevice would be the root of the low-level component.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ColorBlindViz.h"
#include "Effect.h"
#include "EffectPass.h"
#include "EffectManager.h"
#include "EffectTechnique.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "JobsFunction.h"
#include "JobsManager.h"
#include "Logger.h"
#include "Path.h"
#include "ReflectionDataStoreTableUtil.h"
#include "ReflectionDefine.h"
#include "RenderDevice.h"
#include "Renderer.h"
#include "RenderPass.h"
#include "RenderSurface.h"
#include "ScopedAction.h"
#include "SeoulProfiler.h"
#include "SettingsManager.h"
#include "TextureConfig.h"

namespace Seoul
{

static const HString kColorBlindVizSpawnType("ColorBlindViz");

/** Constants used to extract values from the renderer configuration file. */
static const HString ksDepthStencilSurfaces("DepthStencilSurfaces");
static const HString ksName("Name");
static const HString ksRenderPasses("RenderPasses");
static const HString ksRenderSurfaces("RenderSurfaces");
static const HString ksRenderTargets("RenderTargets");

/** Append the last frame's tick time to the running history. */
void FrameRateTracking::AddFrameTicks(Int64 iFrameTicks, Int64 iFrameTicksWithSynchronize)
{
	for (UInt32 i = 0u; i + 1u < m_vFrameTicksHistory.GetSize(); ++i)
	{
		m_vFrameTicksHistory[i] = m_vFrameTicksHistory[i + 1];
		m_vFrameTicksWithSynchronizeHistory[i] = m_vFrameTicksWithSynchronizeHistory[i + 1];
	}

	if (!m_vFrameTicksHistory.IsEmpty())
	{
		// Finally set the last entry to the last measure frame tick count.
		m_vFrameTicksHistory.Back() = iFrameTicks;
		m_vFrameTicksWithSynchronizeHistory.Back() = iFrameTicksWithSynchronize;
	}
}

/**
 * Creates an umanaged 1 pixel texture that contains
 * all 0 values. Can be used in cases where a "null" texture
 * is needed (black, with an alpha of 0)
 */
void Renderer::InternalCreateNullTexture()
{
	SEOUL_ASSERT(IsMainThread());

	/** Name to identify the null texture. */
	static const HString kNullTextureHumanReadableName("NullTexture");

	// Create the error texture. This must succeed.
	auto const uSize = sizeof(ColorARGBu8);
	ColorARGBu8* pData = (ColorARGBu8*)MemoryManager::AllocateAligned(
		uSize,
		MemoryBudgets::Rendering,
		SEOUL_ALIGN_OF(ColorARGBu8));

	*pData = ColorARGBu8::Black();

	auto eFormat = PixelFormat::kA8R8G8B8;
	TextureData data = TextureData::CreateFromInMemoryBuffer(pData, uSize, eFormat);
	TextureConfig config;
	m_pNullTexture = RenderDevice::Get()->CreateTexture(
		config,
		data,
		1u,
		1u,
		eFormat);

	// This marks the texture as fully opaque, which means
	// it is a "perfect" occluder (no alpha bits).
	m_pNullTexture->SetIsFullOccluder();
}

/**
 * @return A root poseable that can be used to pose and render UI screens - in this
 * case, this always returns the global ColorBlindViz singleton.
 */
static IPoseable* SpawnColorBlindViz(
	const DataStoreTableUtil& configSettings,
	Bool& rbRenderPassOwnsPoseableObject)
{
	rbRenderPassOwnsPoseableObject = true;

	return SEOUL_NEW(MemoryBudgets::Rendering) ColorBlindViz(configSettings);
}

Renderer::Renderer()
	: m_FrameRateTracking()
	, m_PendingQueryStats(QueryStats::Create())
	, m_PendingRenderStats(RenderStats::Create())
	, m_QueryStats(QueryStats::Create())
	, m_RenderStats(RenderStats::Create())
	, m_PendingConfigFilePath()
	, m_PendingConfig()
	, m_RendererConfigurationFilePath()
	, m_sRendererConfigurationName()
	, m_pNullTexture()
	, m_iPauseTimeInTicks(0)
	, m_iFrameStartTicks(0)
	, m_bWaitingForCompletion(false)
{
	InternalCreateNullTexture();

	// Register the root poseable hook for rendering color blind viz mode.
	RenderPass::RegisterPoseableSpawnDelegate(
		kColorBlindVizSpawnType,
		SpawnColorBlindViz);
}

Renderer::~Renderer()
{
	ClearConfiguration();

	// Unregister the root poseable hook for rendering color blind viz mode.
	RenderPass::UnregisterPoseableSpawnDelegate(
		kColorBlindVizSpawnType);

	m_pNullTexture.Reset();
}

/**
 * Pose all of this Renderer's passes.
 *
 * "Posing" is the process of building a render pass's
 * render tree. Each pass has a render tree which is traversed
 * depth-first to actually issue commands to the GPU.
 */
void Renderer::Pose(Float fDeltaTime)
{
	SEOUL_ASSERT(IsMainThread());

	UInt32 const uPassCount = m_vRenderPasses.GetSize();

	// Before doing any PrePose or Pose processing, make
	// sure an active render thread has already finished
	// the scene start.
	if (m_pRenderJob.IsValid() && m_pRenderJob->GetJobState() == Jobs::State::kScheduledForOrRunning)
	{
		SEOUL_PROF("Render.BeginSceneWait");
		while (m_bWaitingOnBeginScene)
		{
			m_WaitingOnBeginSceneSignal.Wait();
		}
	}

	// First execute pre-pose passes sequentially - these must
	// be executed one after another on the same thread as
	// the one that called Pose(). This is also the
	// absolute cutoff for any running pose jobs from the
	// previous frame to complete.
	{
		if (Content::LoadManager::Get())
		{
			Content::LoadManager::Get()->PrePose();
		}

		for (UInt32 i = 0u; i < uPassCount; ++i)
		{
			CheckedPtr<RenderPass> pPass = m_vRenderPasses[i];
			pPass->PrePose(fDeltaTime);
		}
	}

	// Now do posing.
	{
		for (UInt32 i = 0u; i < uPassCount; ++i)
		{
			CheckedPtr<RenderPass> pPass = m_vRenderPasses[i];
			pPass->Pose(fDeltaTime);
		}
	}
}

/**
 * Execute this Renderer's passes, previously built
 * by a call to Pose().
 */
void Renderer::Render(Float fDeltaTime)
{
	SEOUL_ASSERT(IsMainThread());

	// Minimum time we give to the job system from the main thread each frame, in milliseconds.
	static const Double kfMinJobTimeMs = 4.0;

	Int64 const iStartSynchronize = SeoulTime::GetGameTimeInTicks();
	{
		// Prior to submitting a new frame, possibly synchronize the render thread,
		// if we're using threaded render submission.
		if (m_pRenderJob.IsValid())
		{
			SEOUL_PROF("Render.Synchronize");
			RenderDeviceScopedWait scopedWait; // Signal device we're waiting, see comment on RenderDeviceScopedWait.
			m_pRenderJob->WaitUntilJobIsNotRunning();
			m_pRenderJob.Reset();
		}

		// If we didn't give the job system enough time to do work for this frame, if it has
		// work, do so now.
		Int64 const iMinJobTime = SeoulTime::ConvertMillisecondsToTicks(kfMinJobTimeMs);
		while ((SeoulTime::GetGameTimeInTicks() - iStartSynchronize) < iMinJobTime)
		{
			// If the Job Manager has nothing to do right now, break out of this extra time loop.
			if (!Jobs::Manager::Get()->YieldThreadTime())
			{
				break;
			}
		}
	}
	Int64 const iEndSynchronize = SeoulTime::GetGameTimeInTicks();

	// Apply any configuration immediately after synchronization.
	InternalApplyConfiguration();

	// Update query and render stats from pending.
	m_QueryStats = m_PendingQueryStats;
	m_RenderStats = m_PendingRenderStats;

	// When enabled, use this to step up or down vsync interval.
	{
		auto const iVsyncInterval = RenderDevice::Get()->GetVsyncInterval();
		if (iVsyncInterval > 0)
		{
			// Compute the current target based on active vsync interval
			// and the measured mean of our sampling window.
			auto const fTarget = 1000.0 / (RenderDevice::Get()->GetDisplayRefreshRate().ToHz() / (Double)iVsyncInterval);
			auto const fMean = SeoulTime::ConvertTicksToMilliseconds(GetFrameRateTracking().GetMeanFrameTicks().First);

			// Step down if mean frame time is less than 75% of our current
			// interval. We never step from vsync to not vsync.
			if (iVsyncInterval > 1 && fMean < 0.75 * fTarget)
			{
				RenderDevice::Get()->SetDesiredVsyncInterval(iVsyncInterval - 1);
			}
			// Step up if mean frame time is greater than 75% of our
			// current interval. We never step up beyond an interval of 3.
			else if (iVsyncInterval < 3 && fMean > (fTarget + fTarget * 0.75))
			{
				RenderDevice::Get()->SetDesiredVsyncInterval(iVsyncInterval + 1);
			}
		}
	}

	// If we are the render thread, execute render tasks immediately.
	if (IsRenderThread())
	{
		m_bWaitingOnBeginScene = true; // Make before kicking.
		DoRender(fDeltaTime);
	}
	// Otherwise, kick them off on the render thread.
	else
	{
		m_pRenderJob.Reset();
		m_bWaitingOnBeginScene = true; // Make before kicking.
		m_pRenderJob = Jobs::MakeFunction(
			GetRenderThreadId(),
			SEOUL_BIND_DELEGATE(&Renderer::DoRender, this),
			fDeltaTime);
		m_pRenderJob->SetJobQuantum(Jobs::Quantum::kDisplayRefreshPeriodic);
		m_pRenderJob->StartJob(true);
	}

	// Update framerate stats.
	Int64 const iEndFrameTicks = SeoulTime::GetGameTimeInTicks();

	// Mark.
	Int64 const iFrameStartTicks = m_iFrameStartTicks;

	// Compute the frame time without time spent synchronizing, and the total frame time.
	// Factor out any ticks during which the timer was paused.
	Int64 const iFrameTicksWithSynchronize = Max((iEndFrameTicks - m_iFrameStartTicks) - m_iPauseTimeInTicks, (Int64)0);
	Int64 const iFrameTicksWithoutSynchronize = Max(iFrameTicksWithSynchronize - (iEndSynchronize - iStartSynchronize), (Int64)0);
	m_iFrameStartTicks = iEndFrameTicks;
	m_iPauseTimeInTicks = 0;

	// Update the frame time history
	if (0 != iFrameStartTicks)
	{
		m_FrameRateTracking.AddFrameTicks(iFrameTicksWithoutSynchronize, iFrameTicksWithSynchronize);
	}
}

void Renderer::DoRender(Float fDeltaTime)
{
	SEOUL_PROF("Renderer.DoRender");

	// Synchronization - must always reset "waiting for begin scene"
	// and activate on exit.
	auto const scoped(MakeScopedAction(
	[&]()
	{
		m_bWaitingOnBeginScene = true;
	},
	[&]()
	{
		if (m_bWaitingOnBeginScene)
		{
			m_bWaitingOnBeginScene = false;
			m_WaitingOnBeginSceneSignal.Activate();
		}
	}));

	RenderDevice& rDevice = *RenderDevice::Get();

	const UInt32 uPassCount = m_vRenderPasses.GetSize();

	// If the main thread is waiting for render thread completion, return immediately without
	// attempting to process buffers.
	if (m_bWaitingForCompletion)
	{
		return;
	}

	// Begin the scene - if this fails, nothing more to do - leave the
	// command stream pending and finish the job for the current frame.
	{
		SEOUL_PROF("RenderDevice.BeginScene");
		if (!rDevice.BeginScene())
		{
			return;
		}

		// Manual.
		m_bWaitingOnBeginScene = false;
		m_WaitingOnBeginSceneSignal.Activate();
	}

	// Reset per-frame render stats
	m_PendingRenderStats.BeginFrame();

	// Render all passes.
	for (UInt32 i = 0u; i < uPassCount; ++i)
	{
		CheckedPtr<RenderPass> pPass = m_vRenderPasses[i];

		// Execute the pass's command stream.
		RenderStats stats = RenderStats::Create();
		pPass->ExecuteAndResetCommandStream(stats);
		if (pPass->TrackRenderStats())
		{
			m_PendingRenderStats += stats;
		}
	}

	// Make sure Effects are not maintaining lingering references
	// to textures.
	EffectManager::Get()->UnsetAllTextures();

	// End the frame.
	{
		SEOUL_PROF("RenderDevice.EndScene");
		rDevice.EndScene();
	}

	// Now accumulate stats for all passes.
	m_PendingQueryStats = QueryStats::Create();
	for (UInt32 i = 0u; i < uPassCount; ++i)
	{
		CheckedPtr<RenderPass> pPass = m_vRenderPasses[i];

		m_PendingQueryStats += pPass->GetQueryStats();
	}
}

/**
 * Initializes the renderer by using the JSON file section
 * specified by configSection in the JSON file sConfigFilename.
 *
 * \warning This method calls Content::LoadManager::SetActiveContentGroups()
 * before loading, which will block the thread if Content::LoadManager has
 * loads to complete. It also does not restore the active content group
 * afterwards, so the active content group will be left as kEngine
 * when this method returns.
 *
 * Any existing Renderer state will be reset to its default
 * configuration.
 */
void Renderer::ReadConfiguration(FilePath configFilePath, HString configSection)
{
	SEOUL_ASSERT(IsMainThread());

	m_PendingConfigFilePath = configFilePath;
	m_PendingConfig = configSection;
}

void Renderer::InternalApplyConfiguration()
{
	if (m_PendingConfigFilePath == m_RendererConfigurationFilePath &&
		m_PendingConfig == m_sRendererConfigurationName)
	{
		return;
	}

	auto const configFilePath = m_PendingConfigFilePath;
	auto const configSection = m_PendingConfig;

	// Don't reload the configuration if we're already set to the same configuration filename
	// and section.
	if (configFilePath != m_RendererConfigurationFilePath ||
		configSection != m_sRendererConfigurationName)
	{
		ClearConfiguration();

		SharedPtr<DataStore> pDataStore(SettingsManager::Get()->WaitForSettings(configFilePath));
		DataNode dataNode;
		if (pDataStore.IsValid() &&
			pDataStore->GetValueFromTable(pDataStore->GetRootNode(), configSection, dataNode))
		{
			if (InternalReadDepthStencil(*pDataStore, dataNode) &&
				InternalReadTargets(*pDataStore, dataNode) &&
				InternalReadSurfaces(*pDataStore, dataNode) &&
				InternalReadPasses(*pDataStore, dataNode))
			{
				m_PendingConfigFilePath = configFilePath;
				m_PendingConfig = configSection;
				m_RendererConfigurationFilePath = configFilePath;
				m_sRendererConfigurationName = configSection;
				return;
			}
		}

		SEOUL_WARN("Failed reading renderer section (%s) from config file (%s).",
			configSection.CStr(),
			configFilePath.CStr());

		ClearConfiguration();
	}
}

/**
 * @return True if a pass with passName is enabled, false otherwise. If no
 * pass with name passName exists, this function always returns false.
 */
Bool Renderer::IsPassEnabled(HString passName) const
{
	UInt32 const nPasses = m_vRenderPasses.GetSize();
	for (UInt32 i = 0u; i < nPasses; ++i)
	{
		if (m_vRenderPasses[i].IsValid() && m_vRenderPasses[i]->GetName() == passName)
		{
			return m_vRenderPasses[i]->IsEnabled();
		}
	}

	return false;
}

/**
 * Update the enabled state of a pass with name passName. This
 * function is a nop if a pass with name passName is not currently defined.
 */
void Renderer::SetPassEnabled(HString passName, Bool bEnabled)
{
	UInt32 const nPasses = m_vRenderPasses.GetSize();
	for (UInt32 i = 0u; i < nPasses; ++i)
	{
		if (m_vRenderPasses[i].IsValid() && m_vRenderPasses[i]->GetName() == passName)
		{
			m_vRenderPasses[i]->SetEnabled(bEnabled);
			return;
		}
	}
}

/**
 * Call to busy wait until the Renderer's render job is not running. Should
 * be called if code needs to execute that is mutually exclusive from code that
 * will be executed in an IPoseable::Pose() implementation.
 *
 * \pre Must be called from the main thread.
 *
 * \pre Must NOT be called from within an IPoseable::PrePose() implementation.
 */
void Renderer::WaitForRenderJob()
{
	SEOUL_ASSERT(IsMainThread());

	if (m_pRenderJob.IsValid())
	{
		m_bWaitingForCompletion = true;
		m_pRenderJob->WaitUntilJobIsNotRunning();
		m_bWaitingForCompletion = false;
	}
}

/**
 * Must only be called from the render thread. When available,
 * repeats the last command stream that was submitted to the GPU.
 *
 * Intended to be used on platforms that need to respond to
 * repaint events.
 */
Bool Renderer::RenderThread_ResubmitLast()
{
	SEOUL_ASSERT(IsRenderThread());

	RenderDevice& rDevice = *RenderDevice::Get();

	const UInt32 uPassCount = m_vRenderPasses.GetSize();

	// Begin the scene - if this fails, nothing more to do.
	if (!rDevice.BeginScene())
	{
		return false;
	}

	// Render all passes.
	Bool bReturn = false;
	for (UInt32 i = 0u; i < uPassCount; ++i)
	{
		CheckedPtr<RenderPass> pPass = m_vRenderPasses[i];

		// Run the last stream run by the pass.
		bReturn = pPass->ExecuteLastCommandStream() || bReturn;
	}

	// Make sure Effects are not maintaining lingering references
	// to textures.
	EffectManager::Get()->UnsetAllTextures();

	// End the frame.
	rDevice.EndScene();

	return bReturn;
}

/**
 * Resets the Renderer to its default configuration.
 */
void Renderer::ClearConfiguration()
{
	SEOUL_ASSERT(IsMainThread());

	// Reset the render job first, so the render thread is not doing any
	// work when we change the configuration.
	m_bWaitingForCompletion = true;

	// Make sure the per-frame render job on the render thread has completed.
	if (m_pRenderJob.IsValid())
	{
		m_pRenderJob->WaitUntilJobIsNotRunning();
		m_pRenderJob.Reset();
	}

	m_PendingConfig = HString();
	m_PendingConfigFilePath.Reset();
	m_sRendererConfigurationName = HString();
	m_RendererConfigurationFilePath.Reset();

	// Order of deletion here is important,
	// passes must go before surfaces. Depth and Targets
	// must go last.
	SafeDeleteVector(m_vRenderPasses);
	SafeDeleteTable(m_tSurfaces);
	m_tDepthStencilSurfaces.Clear();
	m_tTargets.Clear();

	m_bWaitingForCompletion = false;
}

/**
 * Read a collection of DepthStencilSurfaces from the file
 * specified by the DepthStencilSurfaces parameters in the JSON file
 * sBaseFilename in the section specified by section.
 */
Bool Renderer::InternalReadDepthStencil(
	const DataStore& dataStore,
	const DataNode& dataNode)
{
	DataNode depthStencilSurfaces;
	if (!dataStore.GetValueFromTable(dataNode, ksDepthStencilSurfaces, depthStencilSurfaces))
	{
		return false;
	}

	auto const iBegin = dataStore.TableBegin(depthStencilSurfaces);
	auto const iEnd = dataStore.TableEnd(depthStencilSurfaces);
	for (auto i = iBegin; iEnd != i; ++i)
	{
		DataStoreTableUtil table(dataStore, i->Second, i->First);
		SharedPtr<DepthStencilSurface> pDepthStencilSurface(
			RenderDevice::Get()->CreateDepthStencilSurface(table));

		SEOUL_VERIFY(m_tDepthStencilSurfaces.Insert(
			i->First,
			pDepthStencilSurface).Second);
	}

	return true;
}

/**
 * Read a collection of RenderTargets from the file
 * specified by the RenderTargets parameters in the JSON file
 * sBaseFilename in the section specified by section.
 */
Bool Renderer::InternalReadTargets(
	const DataStore& dataStore,
	const DataNode& dataNode)
{
	DataNode renderTargets;
	if (!dataStore.GetValueFromTable(dataNode, ksRenderTargets, renderTargets))
	{
		return false;
	}

	auto const iBegin = dataStore.TableBegin(renderTargets);
	auto const iEnd = dataStore.TableEnd(renderTargets);
	for (auto i = iBegin; iEnd != i; ++i)
	{
		DataStoreTableUtil table(dataStore, i->Second, i->First);
		SharedPtr<RenderTarget> pTarget2D(
			RenderDevice::Get()->CreateRenderTarget(table));
		SEOUL_VERIFY(m_tTargets.Insert(
			i->First,
			pTarget2D).Second);
	}

	return true;
}

/**
 * Read a collection of RenderSurface2Ds from the file
 * specified by the RenderSurfaces parameters in the JSON file
 * sBaseFilename in the section specified by section.
 */
Bool Renderer::InternalReadSurfaces(
	const DataStore& dataStore,
	const DataNode& dataNode)
{
	DataNode renderSurfaces;
	if (!dataStore.GetValueFromTable(dataNode, ksRenderSurfaces, renderSurfaces))
	{
		return false;
	}

	auto const iBegin = dataStore.TableBegin(renderSurfaces);
	auto const iEnd = dataStore.TableEnd(renderSurfaces);
	for (auto i = iBegin; iEnd != i; ++i)
	{
		DataStoreTableUtil table(dataStore, i->Second, i->First);
		RenderSurface2D* pRenderSurface2D = SEOUL_NEW(MemoryBudgets::Rendering) RenderSurface2D(table);
		SEOUL_VERIFY(m_tSurfaces.Insert(i->First, pRenderSurface2D).Second);
	}

	return true;
}

/**
 * Read a collection of RenderPasses from the file
 * specified by the RenderPasses parameters in the JSON file
 * sBaseFilename in the section specified by section.
 */
Bool Renderer::InternalReadPasses(
	const DataStore& dataStore,
	const DataNode& dataNode)
{
	DataNode renderPasses;
	if (!dataStore.GetValueFromTable(dataNode, ksRenderPasses, renderPasses))
	{
		return false;
	}

	UInt32 uArrayCount = 0u;
	if (!dataStore.GetArrayCount(renderPasses, uArrayCount))
	{
		return false;
	}

	for (UInt32 i = 0u; i < uArrayCount; ++i)
	{
		DataNode renderPass;
		if (!dataStore.GetValueFromArray(renderPasses, i, renderPass))
		{
			return false;
		}

		DataNode nameNode;
		String sName;
		if (!dataStore.GetValueFromTable(renderPass, ksName, nameNode) ||
			!dataStore.AsString(nameNode, sName))
		{
			return false;
		}

		HString name(sName);
		DataStoreTableUtil table(dataStore, renderPass, name);
		RenderPass* pPass = SEOUL_NEW(MemoryBudgets::Rendering) RenderPass(name, table);
		if (nullptr == pPass || !pPass->IsValid())
		{
			SafeDelete(pPass);
			return false;
		}

		m_vRenderPasses.PushBack(pPass);
	}

	return true;
}

} // namespace Seoul
