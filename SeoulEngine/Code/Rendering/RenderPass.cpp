/**
 * \file RenderPass.cpp
 * \brief One pass in the render sequence.
 *
 * Rendering occurs in multiple passes. Each pass populates one or more
 * 2D graphics buffers on the GPU, which is then combined or otherwise
 * used in later passes. The final pass, in general, always outputs to the
 * back buffer, which is the graphics buffer on the GPU that is flipped to
 * the video device for final display.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EffectManager.h"
#include "Logger.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDataStoreTableUtil.h"
#include "RenderDevice.h"
#include "Renderer.h"
#include "RenderPass.h"

namespace Seoul
{

/** Number of command streams buffered. */
static const UInt32 kuBuffers = 3u;

/** Constants used to extract configuration values from a render pass configuration. */
static const HString ksClearColor("ClearColor");
static const HString ksClearDepth("ClearDepth");
static const HString ksClearFlags("ClearFlags");
static const HString ksClearStencil("ClearStencil");
static const HString ksEffectTechniques("EffectTechniques");
static const HString ksPassEffect("PassEffect");
static const HString ksPassEffectTechnique("PassEffectTechnique");
static const HString ksPassRootType("PassRootType");
static const HString ksRenderIterationCount("RenderIterationCount");
static const HString ksResolveDepthStencil("ResolveDepthStencil");
static const HString ksResolveRenderTarget("ResolveRenderTarget");
static const HString ksSurface("Surface");
static const HString ksTrackRenderStats("TrackRenderStats");

RenderPass::Poseables RenderPass::s_Poseables;

/** Helper function, reads settings for specifying clear
 *  behavior for this render pass.
 */
void RenderPass::InternalReadClearSettings(
	const DataStoreTableUtil& configSettings,
	Bool& rbErrors,
	String& rsErrorString)
{
	Vector<String> vScratchBuffer;

	// Clear flags
	m_Settings.m_uFlags = 0u;
	if (configSettings.GetValue(ksClearFlags, vScratchBuffer))
	{
		const UInt32 count = vScratchBuffer.GetSize();
		for (UInt32 i = 0u; i < count; ++i)
		{
			if (vScratchBuffer[i].CompareASCIICaseInsensitive("Color") == 0)
			{
				m_Settings.m_uFlags |= (UInt32)ClearFlags::kColorTarget;
			}
			else if (vScratchBuffer[i].CompareASCIICaseInsensitive("Depth") == 0)
			{
				m_Settings.m_uFlags |= (UInt32)ClearFlags::kDepthTarget;
			}
			else if (vScratchBuffer[i].CompareASCIICaseInsensitive("Stencil") == 0)
			{
				m_Settings.m_uFlags |= (UInt32)ClearFlags::kStencilTarget;
			}
			else
			{
				rbErrors = true;
#if !SEOUL_SHIP
				rsErrorString += "Invalid ClearFlag (" + vScratchBuffer[i] + ").\n";
#endif
			}
		}
	}
	// /Clear flags

	// Clear color
	if (!configSettings.GetValue(ksClearColor, m_Settings.m_ClearColor))
	{
		m_Settings.m_ClearColor = Color4(0.0f, 0.0f, 0.0f, 1.0f);
	}
	// /Clear color

	// Clear depth
	if (!configSettings.GetValue(ksClearDepth, m_Settings.m_fClearDepth))
	{
		m_Settings.m_fClearDepth = 1.0f;
	}
	// /Clear depth

	// Clear stencil
	if (!configSettings.GetValue(ksClearStencil, m_Settings.m_uClearStencil))
	{
		m_Settings.m_uClearStencil = 0u;
	}
	// /Clear stencil
}

/** Instantiate a render pass from a name and JSON file section.
 *  Sets up all the various properties needed to define a render pass.
 *  If any required params are not present, issues a warning, and marks
 *  the pass as invalid.
 */
RenderPass::RenderPass(
	HString passName,
	const DataStoreTableUtil& configSettings)
	: m_AvailableCommandStreamBuilders()
	, m_pLastBuilder()
	, m_PopulatedCommandStreamBuilders()
	, m_pRenderCommandStreamBuilderToPopulate()
	, m_PassName(passName)
	, m_Stats(QueryStats::Create())
	, m_iCurrentTechniqueIndex(0)
	, m_bEnabled(true)
{
	SEOUL_PROF_INIT_VAR(m_ProfPrePose, String(m_PassName) + ".PrePose");
	SEOUL_PROF_INIT_VAR(m_ProfPose, String(m_PassName) + ".Pose");

	Bool bErrors = false;
#if !SEOUL_SHIP
	String sErrorString = "The following errors occured loading settings for "
		"pass (" + String(m_PassName.CStr()) + ").\n";
#else
	String sErrorString;
#endif

	// Clear settings.
	InternalReadClearSettings(configSettings, bErrors, sErrorString);

	// Iteration count
	if (!configSettings.GetValue<UInt32>(ksRenderIterationCount, m_uRenderIterationCount))
	{
		m_uRenderIterationCount = 1u;
	}

	// Resolve the depth-stencil surface.
	Bool bResolveDepthStencil = false;
	configSettings.GetValue<Bool>(ksResolveDepthStencil, bResolveDepthStencil);
	if (bResolveDepthStencil)
	{
		m_Settings.m_uFlags |= Settings::kResolveDepthStencil;
	}
	else
	{
		m_Settings.m_uFlags &= ~Settings::kResolveDepthStencil;
	}

	// Resolve the render target.
	Bool bResolveRenderTarget = false;
	configSettings.GetValue<Bool>(ksResolveRenderTarget, bResolveRenderTarget);
	if (bResolveRenderTarget)
	{
		m_Settings.m_uFlags |= Settings::kResolveRenderTarget;
	}
	else
	{
		m_Settings.m_uFlags &= ~Settings::kResolveRenderTarget;
	}

	HString passRootType;
	if (configSettings.GetValue<HString>(ksPassRootType, passRootType))
	{
		PoseableSpawnDelegate delegate = nullptr;
		if (!s_Poseables.GetValue(passRootType, delegate))
		{
			m_pPassRoot = nullptr;
			m_bOwnsPassRoot = false;
			bErrors = true;
#if !SEOUL_SHIP
			sErrorString += "\t- Invalid pass root (" + String(passRootType.CStr()) + ").\n";
#endif
		}
		else
		{
			SEOUL_ASSERT(delegate);
			m_pPassRoot = delegate(configSettings, m_bOwnsPassRoot);
		}
	}
	else
	{
		m_pPassRoot = nullptr;
		m_bOwnsPassRoot = false;
		bErrors = true;
#if !SEOUL_SHIP
		sErrorString += "\t- Pass root not found, must have a property \"PassRootType\".\n";
#endif
	}

	// Pass effect
	String sPassEffect;
	if (configSettings.GetValue<String>(ksPassEffect, sPassEffect) &&
		!sPassEffect.IsEmpty())
	{
		FilePath filePath = FilePath::CreateContentFilePath(sPassEffect);
		m_hPassEffect = EffectManager::Get()->GetEffect(filePath);
	}
	else
	{
		m_hPassEffect.Reset();
	}
	// /Pass effect

	// Render surface
	HString sSurface;
	if (configSettings.GetValue<HString>(ksSurface, sSurface) &&
		!sSurface.IsEmpty())
	{
		m_pSurface = Renderer::Get()->GetSurface(sSurface);
	}
	else
	{
		// Surface can be nullptr. nullptr is a special value
		// for the backbuffer.
		m_pSurface = nullptr;
	}

	// Unset the stencil bit if the surface has no stencil.
	if (m_pSurface)
	{
		if ((m_Settings.m_uFlags & (UInt32)ClearFlags::kStencilTarget) != 0 &&
			!m_pSurface->HasStencilBuffer())
		{
			m_Settings.m_uFlags &= ~((UInt32)ClearFlags::kStencilTarget);
		}
	}
	else
	{
		if ((m_Settings.m_uFlags & (UInt32)ClearFlags::kStencilTarget) != 0 &&
			!DepthStencilFormatHasStencilBuffer(RenderDevice::Get()->GetBackBufferDepthStencilFormat()))
		{
			m_Settings.m_uFlags &= ~((UInt32)ClearFlags::kStencilTarget);
		}
	}
	// /Render surface

	// Technique
	if (!configSettings.GetValue(ksEffectTechniques, m_vEffectTechniqueNames))
	{
		m_vEffectTechniqueNames.Clear();
	}
	// /Technique

	// Pass Technique
	if (!configSettings.GetValue<HString>(ksPassEffectTechnique, m_PassEffectTechniqueName))
	{
		m_PassEffectTechniqueName = HString();
	}
	// /Pass Technique

	if (!configSettings.GetValue<Bool>(ksTrackRenderStats, m_bTrackRenderStats))
	{
		m_bTrackRenderStats = false;
	}

	if (bErrors)
	{
		SEOUL_WARN("%s", sErrorString.CStr());
		InternalClear();
	}
	else
	{
		// Populate available buffers - double buffering + 1
		// for keeping a buffer to handle redraw requests.
		for (UInt32 i = 0u; i < kuBuffers; ++i)
		{
			m_AvailableCommandStreamBuilders.Push(RenderDevice::Get()->CreateRenderCommandStreamBuilder(1024u));
		}

		m_bValid = true;
	}
}

RenderPass::~RenderPass()
{
	if (m_bOwnsPassRoot)
	{
		SafeDelete(m_pPassRoot);
	}

	// Sanity checks.
	SEOUL_ASSERT(!m_pRenderCommandStreamBuilderToPopulate.IsValid());

	{
		// Destroy all populated command streams.
		RenderCommandStreamBuilder* pBuilder = m_PopulatedCommandStreamBuilders.Pop();
		while (nullptr != pBuilder)
		{
			SafeDelete(pBuilder);
			pBuilder = m_PopulatedCommandStreamBuilders.Pop();
		}
	}

	// Destroy the "last" command stream.
	SafeDelete(m_pLastBuilder);

	{
		// Destroy all available command streams.
		RenderCommandStreamBuilder* pBuilder = m_AvailableCommandStreamBuilders.Pop();
		while (nullptr != pBuilder)
		{
			SafeDelete(pBuilder);
			pBuilder = m_AvailableCommandStreamBuilders.Pop();
		}
	}
}

/**
 * If available, execute the last command stream issued to the
 * graphics hardware. Expected to be used to implement redraw
 * events on platforms that need this.
 */
Bool RenderPass::ExecuteLastCommandStream()
{
	SEOUL_ASSERT(IsRenderThread());

	// No last to submit, nop.
	if (nullptr == m_pLastBuilder)
	{
		return false;
	}

	// Execute the render tree, built by the root poseable, and reset it
	// after executing it uRenderIterationCount times successively.
	const UInt32 uRenderIterationCount = GetRenderIterationCount();
	for (UInt32 j = 0u; j < uRenderIterationCount; ++j)
	{
		RenderStats unusedStats = RenderStats::Create();
		m_pLastBuilder->ExecuteCommandStream(unusedStats);
	}

	return true;
}

/**
 * Execute the last populated command stream.
 */
void RenderPass::ExecuteAndResetCommandStream(RenderStats& rStats)
{
	SEOUL_ASSERT(IsRenderThread());

	RenderCommandStreamBuilder* pBuilder = m_PopulatedCommandStreamBuilders.Pop();
	if (nullptr != pBuilder)
	{
		// Execute the render tree, built by the root poseable, and reset it
		// after executing it uRenderIterationCount times successively.
		const UInt32 uRenderIterationCount = GetRenderIterationCount();
		for (UInt32 j = 0u; j < uRenderIterationCount; ++j)
		{
			RenderStats stats = RenderStats::Create();
			pBuilder->ExecuteCommandStream(stats);
			rStats += stats;
		}

		// Reset and push through the last builder we used.
		if (m_pLastBuilder)
		{
			m_pLastBuilder->ResetCommandStream();
			m_AvailableCommandStreamBuilders.Push(m_pLastBuilder);
		}

		// Cache builder (without reset) in last, in case we
		// need to redraw this frame.
		m_pLastBuilder = pBuilder;
	}
}

void RenderPass::PrePose(Float fDeltaTime)
{
	SEOUL_PROF_VAR(m_ProfPrePose);

	SEOUL_ASSERT(IsMainThread());

	m_Stats = QueryStats::Create();

	if (IsValid() && IsEnabled() && GetRenderIterationCount() > 0u)
	{
		IPoseable* pPoseable = GetPassRoot();
		SEOUL_ASSERT(pPoseable);

		m_iCurrentTechniqueIndex = 0;
		pPoseable->PrePose(
			fDeltaTime,
			*this,
			nullptr);
	}
}

void RenderPass::Pose(Float fDeltaTime)
{
	SEOUL_PROF_VAR(m_ProfPose);

	SEOUL_ASSERT(IsMainThread());

	if (IsValid() && IsEnabled() && GetRenderIterationCount() > 0u)
	{
		InternalPose(fDeltaTime);
	}
}

/**
 * Resets the RenderPass to its default state.
 */
void RenderPass::InternalClear()
{
	m_bValid = false;

	m_Stats = QueryStats::Create();
	m_pSurface = nullptr;
	if (m_bOwnsPassRoot)
	{
		SafeDelete(m_pPassRoot);
		m_bOwnsPassRoot = false;
	}
	m_PassName = HString();
	m_hPassEffect.Reset();
	m_PassEffectTechniqueName = HString();
	m_vEffectTechniqueNames.Clear();
}

void RenderPass::InternalPose(Float fDeltaTime)
{
	SEOUL_ASSERT(IsMainThread());

	m_Stats = QueryStats::Create();

	IPoseable* pPoseable = GetPassRoot();
	SEOUL_ASSERT(pPoseable);

	m_pRenderCommandStreamBuilderToPopulate = m_AvailableCommandStreamBuilders.Pop();

	// Pose if we have an available buffer.
	if (m_pRenderCommandStreamBuilderToPopulate.IsValid())
	{
		m_iCurrentTechniqueIndex = 0;
		pPoseable->Pose(
			fDeltaTime,
			*this,
			nullptr);

		RenderCommandStreamBuilder* pBuilder = m_pRenderCommandStreamBuilderToPopulate;
		m_pRenderCommandStreamBuilderToPopulate.Reset();

		m_PopulatedCommandStreamBuilders.Push(pBuilder);
	}
	// If we're not posing due to an already populated buffer, call SkipPose()
	// to give objects a chance to perform maintenance.
	else
	{
		pPoseable->SkipPose(fDeltaTime);
	}
}

static void InternalStaticDoResolve(RenderCommandStreamBuilder& rBuilder, RenderPass& rPass)
{
	const RenderPass::Settings& settings = rPass.GetSettings();
	RenderSurface2D* pSurface = rPass.GetSurface();

	if (pSurface)
	{
		if ((settings.m_uFlags & RenderPass::Settings::kResolveRenderTarget) != 0)
		{
			if (pSurface->GetRenderTarget())
			{
				rBuilder.ResolveRenderTarget(pSurface->GetRenderTarget());
			}
		}

		if ((settings.m_uFlags & RenderPass::Settings::kResolveDepthStencil) != 0)
		{
			if (pSurface->GetDepthStencilSurface())
			{
				rBuilder.ResolveDepthStencilSurface(pSurface->GetDepthStencilSurface());
			}
		}
	}
}

void BeginPass(RenderCommandStreamBuilder& rBuilder, RenderPass& rPass, Bool bUseFullTarget /*= false*/)
{
	SEOUL_BEGIN_GFX_EVENT(rBuilder, "%s", rPass.GetName().CStr());

	RenderDevice& rDevice = *RenderDevice::Get();

	if (rPass.GetSurface())
	{
		rPass.GetSurface()->Select(rBuilder);
	}
	else
	{
		RenderSurface2D::Reset(rBuilder);
	}

	// If we're rendering to the background, make sure we
	// do a full target clear instead of only clearing the
	// viewport area, if needed.
	Bool bResetAllFullClear = false;
	if (!rPass.GetSurface() || !rPass.GetSurface()->GetRenderTarget())
	{
		Viewport fullTargetViewport = Viewport::Create(
			rDevice.GetBackBufferViewport().m_iTargetWidth,
			rDevice.GetBackBufferViewport().m_iTargetHeight,
			0u,
			0u,
			rDevice.GetBackBufferViewport().m_iTargetWidth,
			rDevice.GetBackBufferViewport().m_iTargetHeight);

		// If the full target viewport is different from the
		// standard backbuffer viewport, set the full viewport
		// and mark that the reset needs to happen after the
		// clear.
		if (fullTargetViewport != rDevice.GetBackBufferViewport())
		{
			bResetAllFullClear = !bUseFullTarget;
			rBuilder.SetCurrentViewport(fullTargetViewport);
			rBuilder.SetScissor(true, ToClearSafeScissor(fullTargetViewport));
		}
	}

	// Handle clearing the render surface for this pass, if present.
	const RenderPass::Settings& clear = rPass.GetSettings();
	if ((clear.m_uFlags & (UInt32)ClearFlags::kClearAll) != 0u)
	{
		rBuilder.Clear(
			(clear.m_uFlags & (UInt32)ClearFlags::kClearAll),
			clear.m_ClearColor,
			clear.m_fClearDepth,
			clear.m_uClearStencil);
	}

	// Restore the viewport if we modified it to do a full target clear.
	if (bResetAllFullClear)
	{
		Viewport const backBufferViewport = rDevice.GetBackBufferViewport();
		rBuilder.SetCurrentViewport(backBufferViewport);
		rBuilder.SetScissor(true, ToClearSafeScissor(backBufferViewport));
	}
}

void EndPass(RenderCommandStreamBuilder& rBuilder, RenderPass& rPass)
{
	// Give the device a chance to perform ops post the render pass.
	const RenderPass::Settings& clear = rPass.GetSettings();
	rBuilder.PostPass((clear.m_uFlags & (UInt32)ClearFlags::kClearAll));

	// Resolve the target if needed by the pass.
	InternalStaticDoResolve(rBuilder, rPass);

	// Done marking GUI debugging.
	SEOUL_END_GFX_EVENT(rBuilder);
}

} // namespace Seoul
