/**
 * \file RenderPass.h
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

#pragma once
#ifndef RENDER_PASS_H
#define RENDER_PASS_H

#include "AtomicRingBuffer.h"
#include "ClearFlags.h"
#include "Effect.h"
#include "IPoseable.h"
#include "QueryStats.h"
#include "RenderSurface.h"
#include "SeoulProfiler.h"
namespace Seoul { class DataStoreTableUtil; }

namespace Seoul
{

// Forward declarations
struct RenderStats;

typedef IPoseable* (*PoseableSpawnDelegate)(
	const DataStoreTableUtil& configSettings,
	Bool& rbRenderPassOwnsPoseableObject);

// Check so that we can store both RenderPass flags
// and clear flags in the same UInt32.
SEOUL_STATIC_ASSERT((UInt32)ClearFlags::kClearAll == ((1 << 0) | (1 << 1) | (1 << 2)));

/**
 * Contains the settings for generating one pass in a multi-pass
 * render sequence.
 */
class RenderPass SEOUL_SEALED
{
public:
	struct Settings
	{
		enum Flags
		{
			kResolveDepthStencil = (1 << 18),
			kResolveRenderTarget = (1 << 19)
		};

		static Settings Create()
		{
			Settings ret;
			ret.m_uFlags = 0u;
			ret.m_ClearColor = Color4(0.0f, 0.0f, 0.0f, 1.0f);
			ret.m_fClearDepth = 1.0f;
			ret.m_uClearStencil = 0u;
			return ret;
		}

		static Settings Create(
			UInt32 uFlags,
			const Color4& clearColor,
			Float fClearDepth,
			UInt8 uClearStencil)
		{
			Settings ret;
			ret.m_uFlags = uFlags;
			ret.m_ClearColor = clearColor;
			ret.m_fClearDepth = fClearDepth;
			ret.m_uClearStencil = uClearStencil;
			return ret;
		}

		UInt32 m_uFlags;
		Color4 m_ClearColor;
		Float m_fClearDepth;
		UInt8 m_uClearStencil;
	};

	RenderPass(
		HString passName,
		const DataStoreTableUtil& configSettings);
	~RenderPass();

	static PoseableSpawnDelegate GetPoseableSpawnDelegate(HString typeName)
	{
		PoseableSpawnDelegate ret = nullptr;
		if (!s_Poseables.GetValue(typeName, ret))
		{
			ret = nullptr;
		}

		return ret;
	}

	/**
	 * Registers a function pointer with an HString identifier that will be
	 * called when a poseable of the type identified with the HString is needed.
	 *
	 * This function is called to create the PassRoot for a pass, which
	 * is the root object that kicks off posing for a given pass.
	 */
	static void RegisterPoseableSpawnDelegate(HString typeName, PoseableSpawnDelegate delegate)
	{
		s_Poseables.Insert(typeName, delegate);
	}

	/**
	 * Unregister a previously registered spawn delegate.
	 */
	static void UnregisterPoseableSpawnDelegate(HString typeName)
	{
		s_Poseables.Erase(typeName);
	}

	/**
	 * Settings for this render pass
	 */
	const Settings& GetSettings() const
	{
		return m_Settings;
	}

	/**
	 * The global effect for this pass.
	 *
	 * Can be nullptr. The global effect is often used to set per-pass
	 * render states. It is also the effect which contains post processing
	 * shader code during post processing passes.
	 */
	const EffectContentHandle& GetPassEffect() const
	{
		return m_hPassEffect;
	}

	/**
	 * Identifying name for the pass, used for debug, has
	 * no other special meaning, although all names must be unique
	 * in the JSON file that configures a pass.
	 */
	HString GetName() const
	{
		return m_PassName;
	}

	/**
	 * The # of times this pass should be rendered in
	 * a single frame, one after another.
	 *
	 * Typically a RenderPass with an iteration count
	 * > 1 is doing some form of recursive filtering, where
	 * a simple shader is run over and over on a buffer to
	 * produce the final amount of blur.
	 */
	UInt32 GetRenderIterationCount() const
	{
		return m_uRenderIterationCount;
	}

	/**
	 * @return The command stream builder for this RenderPass - this
	 * must be executed and then reset on the render thread each frame.
	 */
	CheckedPtr<RenderCommandStreamBuilder> GetRenderCommandStreamBuilder()
	{
		return m_pRenderCommandStreamBuilderToPopulate.Get();
	}

	// If available, execute the last command stream issued to the
	// graphics hardware. Expected to be used to implement redraw
	// events on platforms that need this.
	Bool ExecuteLastCommandStream();

	// Submit commands in the pending command stream to the graphics hardware.
	void ExecuteAndResetCommandStream(RenderStats& rStats);

	/**
	 * The target GPU buffer that output from this pass will
	 * be rendered into.
	 */
	RenderSurface2D* GetSurface() const
	{
		return m_pSurface;
	}

	/**
	 * The name of the technique that poseables should use
	 * when drawing themselves during this pass.
	 */
	HString GetEffectTechniqueName(Int32 iIndex = 0) const
	{
		if (iIndex >= 0 && (Int32)m_vEffectTechniqueNames.GetSize() > iIndex)
		{
			return m_vEffectTechniqueNames[iIndex];
		}
		else
		{
			return HString();
		}
	}

	/**
	 * Returns the next technique index of the running
	 * technique index. This can be used to select and use one of multiple
	 * techniques defined for this pass.
	 *
	 * @return A valid technique index, or -1 if all techniques have been
	 * selected.
	 */
	Int32 GetNextEffectTechniqueIndex()
	{
		if (m_iCurrentTechniqueIndex < (Int32)m_vEffectTechniqueNames.GetSize())
		{
			return m_iCurrentTechniqueIndex++;
		}
		else
		{
			return -1;
		}
	}

	/**
	 * If defined, PassEffectTechniqueName is the name of the
	 * the technique used for drawing the EffectPass. Otherwise, it
	 * will be equal to the EffectTechniqueName, which must be defined
	 * for a pass.
	 */
	HString GetPassEffectTechniqueName() const
	{
		if (m_PassEffectTechniqueName.IsEmpty())
		{
			if (m_vEffectTechniqueNames.IsEmpty())
			{
				return HString();
			}
			else
			{
				return m_vEffectTechniqueNames.Front();
			}
		}
		else
		{
			return m_PassEffectTechniqueName;
		}
	}

	/**
	 * Root poseable which starts posing for this pass.
	 */
	IPoseable* GetPassRoot() const
	{
		return m_pPassRoot;
	}

	/**
	 * Whether all the required parameters for this pass are defined
	 * and if-so, did they initialize correctly.
	 */
	Bool IsValid() const
	{
		return m_bValid;
	}

	void Pose(Float fDeltaTime);
	void PrePose(Float fDeltaTime);

	/**
	 * Gets the current state of per-pass query stats.
	 */
	const QueryStats& GetQueryStats() const
	{
		return m_Stats;
	}

	/**
	 * Accumulate the stats into this object's total
	 * per-pass query stats.
	 *
	 * RenderPass automatically resets its per-pass query stats
	 * when Pose() is called.
	 */
	void AccumulateQueryStats(const QueryStats& stats)
	{
		m_Stats += stats;
	}

	/** Override the configured clear color of this Pass. */
	void SetClearColor(const Color4& c)
	{
		m_Settings.m_ClearColor = c;
	}

	/**
	 * @return True if render stats should be tracked when rendering this
	 * pass, false otherwise.
	 */
	Bool TrackRenderStats() const
	{
		return m_bTrackRenderStats;
	}

	/**
	 * @return True if this pass is enabled for rendering, false otherwise.
	 * Used at runtime only to enable/disable individual passes.
	 */
	Bool IsEnabled() const
	{
		return m_bEnabled;
	}

	void SetEnabled(Bool bEnabled)
	{
		m_bEnabled = bEnabled;
	}

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(RenderPass);

	void InternalPose(Float fDeltaTime);

	typedef HashTable<HString, PoseableSpawnDelegate> Poseables;
	static Poseables s_Poseables;

	AtomicRingBuffer<RenderCommandStreamBuilder*> m_AvailableCommandStreamBuilders;
	CheckedPtr<RenderCommandStreamBuilder> m_pLastBuilder;
	AtomicRingBuffer<RenderCommandStreamBuilder*> m_PopulatedCommandStreamBuilders;
	CheckedPtr<RenderCommandStreamBuilder> m_pRenderCommandStreamBuilderToPopulate;
	Settings m_Settings;

	Vector<HString> m_vEffectTechniqueNames;

	UInt32 m_uRenderIterationCount;
	HString m_PassEffectTechniqueName;
	EffectContentHandle m_hPassEffect;
	HString m_PassName;
	IPoseable* m_pPassRoot;
	RenderSurface2D* m_pSurface;
	QueryStats m_Stats;

	SEOUL_PROF_DEF_VAR(m_ProfPrePose)
	SEOUL_PROF_DEF_VAR(m_ProfPose)

	Int32 m_iCurrentTechniqueIndex;

	Bool m_bOwnsPassRoot;
	Bool m_bValid;
	Bool m_bTrackRenderStats;

	Bool m_bEnabled;

	void InternalClear();

	void InternalReadClearSettings(
		const DataStoreTableUtil& configSettings,
		Bool& rbErrors,
		String& rsErrorString);
};

void BeginPass(RenderCommandStreamBuilder& rBuilder, RenderPass& rPass, Bool bUseFullTarget = false);
void EndPass(RenderCommandStreamBuilder& rBuilder, RenderPass& rPass);

} // namespace Seoul

#endif // include guard
