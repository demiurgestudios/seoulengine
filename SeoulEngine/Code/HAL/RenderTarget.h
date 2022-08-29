/**
 * \file RenderTarget.h
 * \brief Represents a two dimensional color target on the GPU.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef RENDER_TARGET_H
#define RENDER_TARGET_H

#include "BaseGraphicsObject.h"
#include "Texture.h"
namespace Seoul { class DataStoreTableUtil; }

namespace Seoul
{

/**
 * RenderTarget is a buffer on the GPU used for render output
 * All rendering must typically have a target attached, even if
 * it is not being used (i.e. for depth-only rendering). The render target
 * which all rendering must eventually be directed to to be displayed
 * on the video hardware is the back buffer. The back buffer is selected
 * by setting a nullptr RenderTarget object in appropriate contexts.
 */
class RenderTarget SEOUL_ABSTRACT : public BaseTexture
{
public:
	static RenderTarget* GetActiveRenderTarget()
	{
		return s_pActiveRenderTarget;
	}

	RenderTarget(const DataStoreTableUtil& configSettings);
	virtual ~RenderTarget();

	enum Flags
	{
		kNone = 0,
		kTakeWidthFromBackBuffer = (1 << 0),
		kTakeHeightFromBackBuffer = (1 << 1),
		kSimultaneousInputOutput = (1 << 2)
	};

	// BaseGraphicsObject overrides
	virtual void OnReset() SEOUL_OVERRIDE;
	// /BaseGraphicsObject overrides

	virtual void Select() = 0;
	virtual void Unselect() = 0;
	virtual void Resolve() = 0;

	/**
	 * Returns the proportion that this RenderTarget's
	 * width is relative to the backbuffer width.
	 *
	 * You must check IsWidthProportionalToBackBuffer().
	 * This value will only be valid if that method returns true.
	 */
	Float32 GetWidthProportion() const
	{
		return m_fWidthFactor;
	}

	/**
	 * Returns the proportion that this RenderTarget's
	 * height is relative to the backbuffer height.
	 *
	 * You must check IsHeightProportionalToBackBuffer().
	 * This value will only be valid if that method returns true.
	 */
	Float32 GetHeightProportion() const
	{
		return m_fHeightFactor;
	}

	Bool IsWidthProportionalToBackBuffer() const
	{
		return ((m_Flags & kTakeWidthFromBackBuffer) != 0);
	}

	Bool IsHeightProportionalToBackBuffer() const
	{
		return ((m_Flags & kTakeHeightFromBackBuffer) != 0);
	}

	Bool IsProportional() const
	{
		return IsWidthProportionalToBackBuffer() || IsHeightProportionalToBackBuffer();
	}

	Bool SupportsSimultaneousInputOutput() const
	{
		return ((m_Flags & kSimultaneousInputOutput) != 0);
	}

	PixelFormat GetFormat() const
	{
		return m_eFormat;
	}

	Atomic32Type GetResetCount() const
	{
		return m_ResetCount;
	}

protected:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(RenderTarget);

	void InternalRefreshWidthAndHeight();

	UInt16 m_Flags;
	UInt16 m_Levels;
	union
	{
		UInt32 m_Width;
		Float32 m_fWidthFactor;
	};
	union
	{
		UInt32 m_Height;
		Float32 m_fHeightFactor;
	};
	UInt32 m_WidthTimesHeightThreshold;
	Float32 m_fThresholdWidthFactor;
	Float32 m_fThresholdHeightFactor;
	PixelFormat m_eFormat;
	Atomic32 m_ResetCount;

	static RenderTarget* s_pActiveRenderTarget;

private:
	SEOUL_DISABLE_COPY(RenderTarget);
}; // class RenderTarget

} // namespace Seoul

#endif // include guard
