/**
 * \file FxManager.h
 * \brief Global singleton that owns value-over-time effects.
 * Fx are curve base effects, typically particle systems, but
 * also Fx such as camera shake, or even sound effect trigger
 * and playback.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FX_MANAGER_H
#define FX_MANAGER_H

#include "FilePath.h"
#include "Matrix4D.h"
#include "Prereqs.h"
#include "Singleton.h"
#include "Vector2D.h"
#include "Vector3D.h"
namespace Seoul { class Camera; }
namespace Seoul { class IFxRenderer; }

namespace Seoul
{

// Forward declarations
class Fx;
struct FxProperties;

struct FxPreviewModeState SEOUL_SEALED
{
	FxPreviewModeState()
		: m_vPosition()
		, m_bActive(false)
	{
	}

	Vector3D m_vPosition;
	Bool m_bActive;
}; // struct FxPreviewModeState

class FxManager SEOUL_ABSTRACT : public Singleton<FxManager>
{
public:
	FxManager()
		: m_vScreenShakeOffset(0, 0)
	{
	}

	virtual ~FxManager()
	{
	}

	// Assign rpInstance a valid Fx object corresponding to filePath and
	// return true, or leave rpInstance unchanged and return false
	// in an error case. If rpInstance is non-null on input,
	// in the success case, it will be swapped for the new value
	// and the old value will be deleted.
	virtual Bool GetFx(FilePath filePath, Fx*& rpInstance) = 0;

	// Retrieve info about Fx preview mode - active/inactive, and various state.
	virtual Bool GetFxPreviewModeState(FxPreviewModeState& rState) const = 0;

	// True if an fx preview is currently active, false otherwise.
	virtual Bool IsPreviewFxValid() const = 0;

	// Call once per frame to render/update the preview Fx, if it
	// is active.
	virtual void RenderPreviewFx(IFxRenderer& rRenderer) = 0;

	// Update the camera to be used for rendering preview FX.
	virtual void SetPreviewFxCamera(const SharedPtr<Camera>& pCamera) = 0;

	// Update the flags used for preview FX.
	virtual void SetPreviewFxFlags(UInt32 uFlags) = 0;

	// Update the spawn position of the preview FX.
	virtual void SetPreviewFxPosition(const Vector3D& vPosition) = 0;

	// Parenting transform of the preview FX. In addition
	// to the position.
	virtual void SetPreviewFxTransform(const Matrix4D& mTransform) = 0;

	// Call once per frame to update the preview Fx, if it
	// is active.
	virtual void UpdatePreviewFx(Float fDeltaTimeInSeconds) = 0;

	// Equivalent to GetFx() but only prefetches the content.
	virtual void Prefetch(FilePath filePath) = 0;

	// Call per-frame to advance the FX system.
	virtual void Tick(Float fDeltaTimeInSeconds) = 0;

	const Vector2D& GetScreenShakeOffset() const
	{
		return m_vScreenShakeOffset;
	}
	void SetScreenShakeOffset(const Vector2D& vScreenShakeOffset)
	{
		m_vScreenShakeOffset = vScreenShakeOffset;
	}

private:
	SEOUL_DISABLE_COPY(FxManager);

	Vector2D m_vScreenShakeOffset;
}; // class FxManager

class NullFxManager SEOUL_SEALED : public FxManager
{
public:
	NullFxManager()
	{
	}

	~NullFxManager()
	{
	}

	/** @return Always false. */
	virtual Bool GetFx(FilePath filePath, Fx*& rpInstance) SEOUL_OVERRIDE
	{
		return false;
	}

	/** @return Always false. */
	virtual Bool GetFxPreviewModeState(FxPreviewModeState& rState) const SEOUL_OVERRIDE
	{
		return false;
	}

	/** @return Always false. */
	virtual Bool IsPreviewFxValid() const SEOUL_OVERRIDE
	{
		return false;
	}

	virtual void RenderPreviewFx(IFxRenderer& rRenderer) SEOUL_OVERRIDE
	{
		// Nop
	}

	virtual void SetPreviewFxCamera(const SharedPtr<Camera>& pCamera) SEOUL_OVERRIDE
	{
		// Nop
	}

	virtual void SetPreviewFxFlags(UInt32 uFlags) SEOUL_OVERRIDE
	{
		// Nop
	}

	virtual void SetPreviewFxPosition(const Vector3D& vPosition) SEOUL_OVERRIDE
	{
		// Nop
	}

	virtual void SetPreviewFxTransform(const Matrix4D& mTransform) SEOUL_OVERRIDE
	{
		// Nop
	}

	virtual void UpdatePreviewFx(Float fDeltaTimeInSeconds) SEOUL_OVERRIDE
	{
		// Nop
	}

	virtual void Prefetch(FilePath filePath) SEOUL_OVERRIDE
	{
		// Nop
	}

	virtual void Tick(Float fDeltaTimeInSeconds) SEOUL_OVERRIDE
	{
		// Nop
	}

private:
	SEOUL_DISABLE_COPY(NullFxManager);
};

} // namespace Seoul

#endif // include guard
