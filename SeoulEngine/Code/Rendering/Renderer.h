/**
 * \file Renderer.h
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

#pragma once
#ifndef RENDERER_H
#define RENDERER_H

#include "Atomic32.h"
#include "HashTable.h"
#include "QueryStats.h"
#include "RenderCommandStreamBuilder.h"
#include "ScopedPtr.h"
#include "SeoulSignal.h"
#include "SeoulString.h"
#include "Singleton.h"
#include "Vector.h"
namespace Seoul { class DataNode; }
namespace Seoul { class DataStore; }
namespace Seoul { class DataStoreTableUtil; }
namespace Seoul { class DepthStencilSurface; }
namespace Seoul { class RenderDevice; }
namespace Seoul { class RenderSurface2D; }
namespace Seoul { class RenderTarget; }
namespace Seoul { namespace Jobs { class Job; } }

namespace Seoul
{

/**
 * State used to track frame rate history from the renderer's POV.
 */
struct FrameRateTracking SEOUL_SEALED
{
	FrameRateTracking()
		: m_vFrameTicksHistory(60, (Int64)0)
		, m_vFrameTicksWithSynchronizeHistory(60, (Int64)0)
	{
	}

	void AddFrameTicks(Int64 iFrameTicks, Int64 iFrameTicksWithSynchronize);

	/** @return Max frame ticks from the frame tick history. */
	Pair<Int64, Int64> GetMaxFrameTicks() const
	{
		UInt32 const uFrameTicksHistorySize = m_vFrameTicksHistory.GetSize();
		auto ret(MakePair((Int64)0, (Int64)0));
		for (UInt32 i = 0u; i < uFrameTicksHistorySize; ++i)
		{
			ret.First = Max(ret.First, m_vFrameTicksHistory[i]);
			ret.Second = Max(ret.Second, m_vFrameTicksWithSynchronizeHistory[i]);
		}

		return ret;
	}

	/** @return Averaged frame ticks from the frame tick history. */
	Pair<Int64, Int64> GetMeanFrameTicks() const
	{
		UInt32 const uFrameTicksHistorySize = m_vFrameTicksHistory.GetSize();
		auto ret(MakePair((Int64)0, (Int64)0));
		for (UInt32 i = 0u; i < uFrameTicksHistorySize; ++i)
		{
			ret.First += m_vFrameTicksHistory[i];
			ret.Second += m_vFrameTicksWithSynchronizeHistory[i];
		}

		ret.First = (Int64)((Double)ret.First / (Double)uFrameTicksHistorySize);
		ret.Second = (Int64)((Double)ret.Second / (Double)uFrameTicksHistorySize);
		return ret;
	}

	typedef Vector<Int64, MemoryBudgets::Rendering> FrameTicksHistory;
	FrameTicksHistory m_vFrameTicksHistory;
	FrameTicksHistory m_vFrameTicksWithSynchronizeHistory;
}; // struct FrameRateTracking

/**
 * Renderer is the one and only Renderer - it is the root
 * of the platform-independent "high-level" of the rendering system.
 */
class Renderer SEOUL_SEALED : public Singleton<Renderer>
{
public:
	SEOUL_DELEGATE_TARGET(Renderer);

	Renderer();
	~Renderer();

	void Pose(Float fDeltaTime);
	void Render(Float fDeltaTime);

	DepthStencilSurface* GetDepthStencilSurface(HString surfaceName) const
	{
		SharedPtr<DepthStencilSurface> pReturn;
		m_tDepthStencilSurfaces.GetValue(surfaceName, pReturn);
		return pReturn.GetPtr();
	}

	RenderTarget* GetRenderTarget(HString targetName) const
	{
		SharedPtr<RenderTarget> pReturn;
		m_tTargets.GetValue(targetName, pReturn);
		return pReturn.GetPtr();
	}

	RenderSurface2D* GetSurface(HString surfaceName) const
	{
		RenderSurface2D* pReturn = nullptr;
		m_tSurfaces.GetValue(surfaceName, pReturn);
		return pReturn;
	}

	// Configure the Renderer based on an json file.
	void ReadConfiguration(FilePath configFilePath, HString configSection);
	void ClearConfiguration();

	FilePath GetRendererConfigurationFilePath() const
	{
		return m_RendererConfigurationFilePath;
	}

	HString GetRendererConfigurationName() const
	{
		return m_sRendererConfigurationName;
	}

	/**
	 * Get per-frame query stats from the last rendered frame.
	 */
	const QueryStats& GetQueryStats() const
	{
		return m_QueryStats;
	}

	/**
	 * @return Get the per-frame render submission stats from the last rendered frame.
	 */
	const RenderStats& GetRenderStats() const
	{
		return m_RenderStats;
	}

	/**
	 * Null texture - texture contains a single pixel
	 * with all 0s and can be used to set a predictable invalid
	 * texture.
	 */
	TextureContentHandle GetNullTexture() const
	{
		return TextureContentHandle(m_pNullTexture.GetPtr());
	}

	// Return true if a passName is enabled, false otherwise.
	Bool IsPassEnabled(HString passName) const;

	// Update the enabled state of passName.
	void SetPassEnabled(HString passName, Bool bEnabled);

	/** @return A read-only reference to the current frame rat state. */
	const FrameRateTracking& GetFrameRateTracking() const
	{
		return m_FrameRateTracking;
	}

	/**
	 * Typically called by Engine, used to
	 * ignore windows of time to aovid spikes in the frame
	 * tick history.
	 */
	void AddPauseTicks(Int64 iPauseTicks)
	{
		m_iPauseTimeInTicks += iPauseTicks;
	}

	// Call to busy wait until the Renderer's render job is not running. Should
	// be called if code needs to execute that is mutually exclusive from code that
	// will be executed in an IPoseable::Pose() implementation.
	//
	// Must be called from the main thread.
	void WaitForRenderJob();

	// Must only be called from the render thread. When available,
	// repeats the last command stream that was submitted to the GPU.
	//
	// Intended to be used on platforms that need to respond to
	// repaint events.
	Bool RenderThread_ResubmitLast();

private:
	FrameRateTracking m_FrameRateTracking;

	QueryStats m_PendingQueryStats;
	RenderStats m_PendingRenderStats;
	QueryStats m_QueryStats;
	RenderStats m_RenderStats;

	Vector< CheckedPtr<RenderPass> > m_vRenderPasses;

	typedef HashTable< HString, SharedPtr<DepthStencilSurface>, MemoryBudgets::Rendering > DepthStencilSurfaces;
	DepthStencilSurfaces m_tDepthStencilSurfaces;

	typedef HashTable< HString, SharedPtr<RenderTarget>, MemoryBudgets::Rendering > Targets;
	Targets m_tTargets;

	typedef HashTable< HString, RenderSurface2D*, MemoryBudgets::Rendering > Surfaces;
	Surfaces m_tSurfaces;

	FilePath m_PendingConfigFilePath;
	HString m_PendingConfig;
	FilePath m_RendererConfigurationFilePath;
	HString m_sRendererConfigurationName;

	void InternalApplyConfiguration();
	Bool InternalReadDepthStencil(
		const DataStore& dataStore,
		const DataNode& dataNode);
	Bool InternalReadTargets(
		const DataStore& dataStore,
		const DataNode& dataNode);
	Bool InternalReadSurfaces(
		const DataStore& dataStore,
		const DataNode& dataNode);
	Bool InternalReadPasses(
		const DataStore& dataStore,
		const DataNode& dataNode);

	void InternalCreateNullTexture();

	SharedPtr<BaseTexture> m_pNullTexture;
	SharedPtr<Jobs::Job> m_pRenderJob;
	Int64 m_iPauseTimeInTicks;
	Int64 m_iFrameStartTicks;
	Atomic32Value<Bool> m_bWaitingForCompletion;

	void DoRender(Float fDeltaTime);

	Atomic32Value<Bool> m_bWaitingOnBeginScene;
	Signal m_WaitingOnBeginSceneSignal;

	SEOUL_DISABLE_COPY(Renderer);
};

} // namespace Seoul

#endif // include guard
