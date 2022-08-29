/**
 * \file Fx.h
 * \brief Defines a generic value-over-time effect, such as a particle system. The
 * semantics of this class are nearly identical to a Sound::Event in the sound system.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FX_H
#define FX_H

#include "Color.h"
#include "FilePath.h"
#include "Matrix3x4.h"
#include "Prereqs.h"
#include "Vector3D.h"
#include "Vector4D.h"
namespace Seoul { class Camera; }

namespace Seoul
{

// Used by FxFactory to query Fx for properties about this Fx and return
// values from it.
struct FxProperties SEOUL_SEALED
{
	FxProperties()
		: m_fDuration(0.f)
		, m_bHasLoops(false)
	{
	}
	Float m_fDuration;
	Bool m_bHasLoops;
};
template <> struct CanMemCpy<FxProperties> { static const Bool Value = true; };
template <> struct CanZeroInit<FxProperties> { static const Bool Value = true; };

struct FxParticle SEOUL_SEALED
{
	FxParticle()
		: m_mTransform()
		, m_vTexcoordScaleAndShift()
		, m_Color()
		, m_uAlphaClampMin(0)
		, m_uAlphaClampMax(0)
	{
	}

	Matrix3x4 m_mTransform;
	Vector4D m_vTexcoordScaleAndShift;
	RGBA m_Color;
	UInt8 m_uAlphaClampMin;
	UInt8 m_uAlphaClampMax;
}; // struct FxParticle
template <> struct CanMemCpy<FxParticle> { static const Bool Value = true; };
template <> struct CanZeroInit<FxParticle> { static const Bool Value = true; };

enum class FxRendererMode
{
	// Normal and additive are standing blending modes.
	//
	// These are efficiently implemented in Falcon
	// and preferred. Generally, use these and not
	// the extended modes.

	/** Standard alpha blending. */
	kNormal,

	/** Additive blending (one + one). */
	kAdditive,

	// Extended modes - these exist to support existing
	// content on existing projects. They should be
	// considered deprecated. They break batches
	// when used and are generally more expensive
	// then normal or additive (with the exception
	// of alpha clamp at the end, which is at the end
	// to be friendly to our Fx editor).

	/** SrcBlend = InvSrcAlpha, DestBlend = One. */
	kExtended_InvSrcAlpha_One,
	/** SrcBlend = InvSrcColor, DestBlend = One. */
	kExtended_InvSrcColor_One,

	/** SrcBlend = One, DestBlend = InvSrcColor. */
	kExtended_One_InvSrcColor,
	/** SrcBlend = One, DestBlend = SrcAlpha. */
	kExtended_One_SrcAlpha,
	/** SrcBlend = One, DestBlend = SrcColor. */
	kExtended_One_SrcColor,

	/** SrcBlend = SrcAlpha, DestBlend = InvSrcAlpha. */
	kExtended_SrcAlpha_InvSrcAlpha,
	/** SrcBlend = SrcAlpha, DestBlend = InvSrcColor. */
	kExtended_SrcAlpha_InvSrcColor,
	/** SrcBlend = SrcAlpha, DestBlend = One. */
	kExtended_SrcAlpha_One,
	/** SrcBlend = SrcAlpha, DestBlend = SrcAlpha. */
	kExtended_SrcAlpha_SrcAlpha,

	/** SrcBlend = SrcColor, DestBlend = InvSrcAlpha. */
	kExtended_SrcColor_InvSrcAlpha,
	/** SrcBlend = SrcColor, DestBlend = InvSrcColor. */
	kExtended_SrcColor_InvSrcColor,
	/** SrcBlend = SrcColor, DestBlend = One. */
	kExtended_SrcColor_One,

	/** SrcBlend = Zero, DestBlend = InvSrcColor. */
	kExtended_Zero_InvSrcColor,
	/** SrcBlend = Zero, DestBlend = SrcColor. */
	kExtended_Zero_SrcColor,

	// Though it is last, AlphaClamp is a standard/stock blend mode.
	// It is last as noted below for convenience in specifying in
	// the Fx editor.

	// NOTE: Must be last - this mode is not explicit in the Fx editor, it is inferred
	// by the definition of an alpha clamp curve.

	/** Alpha clamp - alpha is rescaled from [min, max] to [0, 1] and clamped. */
	kAlphaClamp,
	/** Alpha clamp with non-white in the color channels - requires a more expensive shader to apply. */
	kColorAlphaClamp,

	// Convenience for querying, not modes.
	FIRST_EXTENDED = kExtended_InvSrcAlpha_One,
	LAST_EXTENDED = kExtended_Zero_SrcColor,
};

/** @return true if eMode is an extended blend mode (always requires a batch break unless exactly the same mode), false otherwise. */
static inline Bool FxRendererModeIsExtended(FxRendererMode eMode)
{
	return (FxRendererMode::FIRST_EXTENDED <= eMode && eMode <= FxRendererMode::LAST_EXTENDED);
}

class IFxRenderer SEOUL_ABSTRACT
{
public:
	typedef Vector<FxParticle, MemoryBudgets::Fx> Buffer;

	virtual ~IFxRenderer()
	{
	}

	virtual Camera& GetCamera() const = 0;

	virtual Buffer& LockFxBuffer() = 0;
	virtual void UnlockFxBuffer(
		UInt32 uParticles,
		FilePath textureFilePath,
		FxRendererMode eMode,
		Bool bNeedsScreenAlign) = 0;

protected:
	IFxRenderer()
	{
	}

private:
	SEOUL_DISABLE_COPY(IFxRenderer);
}; // class IFxRenderer

typedef Vector<FilePath> FxAssetsVector;
class Fx SEOUL_ABSTRACT
{
public:
	virtual ~Fx()
	{
	}

	// Instantiate a new instance of an Fx that is an exact copy of this instance.
	virtual Fx* Clone() const = 0;

	// Return true if the data associated with this Fx is still being
	// loaded from disk, false otherwise.
	virtual Bool IsLoading() const = 0;

	// Return true if the Fx is currently playing, false otherwise.
	virtual Bool IsPlaying() const = 0;

	// Return the Assets in use by this Fx.
	virtual Bool AppendAssets(FxAssetsVector& rvAssets) const = 0;

	// Timeline control of this Fx.
	virtual Bool Start(const Matrix4D& mTransform, UInt32 uFlags) = 0;
	virtual void Pause(Bool bPause = true) = 0;
	virtual void Stop(Bool bStopImmediately = false) = 0;

	// Update the current world position of this Fx.
	virtual Bool SetPosition(const Vector3D& vPosition) = 0;

	// Update the gravity of this Fx.
	virtual Bool SetGravity(Float fGravityAcceleration) = 0;

	// Update the full transform of this Fx.
	virtual Bool SetTransform(const Matrix4D& mTransform) = 0;

	// Update the parent transform for this fx if in world space
	virtual Bool SetParentIfWorldspace(const Matrix4D& mTransform) = 0;

	// Set the rally point of any particles in the FX.
	virtual Bool SetRallyPoint(const Vector3D& vRallyPoint) = 0;

	// Get our FilePath
	virtual FilePath GetFilePath() const = 0;

	// Drawing support.
	virtual void Draw(IFxRenderer& rRenderer) = 0;

	// Updating support.
	virtual void Tick(Float fDeltaTimeInSeconds) = 0;

	// Return features of the overall Fx.
	virtual Bool GetProperties(FxProperties& rProperties) const = 0;

	// Return true if this Fx needs calls to Render or not.
	//
	// May be O(n), should be cached if evaluation time is important.
	virtual Bool NeedsRender() const = 0;

protected:
	Fx()
	{
	}

private:
	SEOUL_DISABLE_COPY(Fx);
}; // class Fx

} // namespace Seoul

#endif // include guard
