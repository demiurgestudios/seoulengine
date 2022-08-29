/**
 * \file DepthStencilSurface.h
 * \brief DepthStencilSurface represents a depth-stencil buffer on the GPU.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef DEPTH_STENCIL_SURFACE_H
#define DEPTH_STENCIL_SURFACE_H

#include "BaseGraphicsObject.h"
#include "DepthStencilFormat.h"
#include "Texture.h"
namespace Seoul { class DataStoreTableUtil; }

namespace Seoul
{

/**
 * DepthStencilSurface is a depth-stencil buffer on the GPU.
 * All rendering must typically have a DS surface attached, even if
 * it is not being used. DepthStencilSurface must be specialized per
 * platform.
 */
class DepthStencilSurface SEOUL_ABSTRACT : public BaseTexture
{
public:
	static DepthStencilSurface* GetActiveDepthStencilSurface()
	{
		return s_pCurrentSurface;
	}

	DepthStencilSurface(const DataStoreTableUtil& configSettings);
	virtual ~DepthStencilSurface();

	enum Flags
	{
		kNone = 0,
		kTakeWidthFromBackBuffer = (1 << 0),
		kTakeHeightFromBackBuffer = (1 << 1)
	};

	virtual void Select() = 0;
	virtual void Unselect() = 0;
	virtual void Resolve() = 0;

	DepthStencilFormat GetFormat() const
	{
		return m_eFormat;
	}

	/**
	 * @return True if the width of this DepthStencilSurface is defined to be
	 * preportional to the back buffer, false otherwise.
	 */
	Bool IsWidthProportionalToBackBuffer() const
	{
		return ((m_Flags & kTakeWidthFromBackBuffer) != 0);
	}

	/**
	 * @return True if the height of this DepthStencilSurface is defined to be
	 * preportional to the back buffer, false otherwise.
	 */
	Bool IsHeightProportionalToBackBuffer() const
	{
		return ((m_Flags & kTakeHeightFromBackBuffer) != 0);
	}

	Bool IsProportional() const
	{
		return IsWidthProportionalToBackBuffer() || IsHeightProportionalToBackBuffer();
	}

	/**
	 * Returns true if this DepthStencilSurface has a stencil
	 * buffer, false otherwise.
	 */
	Bool HasStencilBuffer() const
	{
		return DepthStencilFormatHasStencilBuffer(m_eFormat);
	}

protected:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(DepthStencilSurface);

	void InternalRefreshWidthAndHeight();

	static DepthStencilSurface* s_pCurrentSurface;

	UInt32 m_Flags;
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
	DepthStencilFormat m_eFormat;

private:
	SEOUL_DISABLE_COPY(DepthStencilSurface);
}; // class DepthStencilSurface

} // namespace Seoul

#endif // include guard
