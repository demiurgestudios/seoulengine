/**
 * \file FalconRenderFeatures.h
 * \brief Utility structure used to gather and track
 * Render::Feature values.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_RENDER_FEATURES_H
#define FALCON_RENDER_FEATURES_H

#include "FalconRenderFeature.h"
#include "Prereqs.h"

namespace Seoul::Falcon
{

namespace Render
{

/**
 * Utility structure that tracks and manages the rendering features
 * needed by the pending draw call being accumulated in the renderer.
 */
class Features SEOUL_SEALED
{
public:
	Features()
		: m_u(0u)
	{
	}

	/** True if features of A and B are compatible and can share a batch. */
	static Bool Compatible(UInt32 uFeaturesA, UInt32 uFeaturesB)
	{
		// Features are compatible if neither contains
		// an extended mode.
		auto const bA = (0u == (uFeaturesA & Feature::EXTENDED_MASK));
		auto const bB = (0u == (uFeaturesB & Feature::EXTENDED_MASK));

		// No extended blend mode, compatible.
		if (bA && bB) { return true; }
		else
		{
			// Only compatible if exactly the same extended blend mode.
			//
			// Non-extended bits are always compatible so we mask them away.
			return ((uFeaturesA & Feature::EXTENDED_MASK) == (uFeaturesB & Feature::EXTENDED_MASK));
		}
	}

	/*
	 *@return Cost value of the features - roughly, relative shader complexity, per "unit".
	 *
	 * "unit" here has no absolute definition, but can be used for relative calculations
	 * (e.g. a unit cost of 2 is twice as expensive as a unit cost of 1). */
	static Int32 Cost(UInt32 uFeatures)
	{
		using namespace Feature;

		// Mask out extended, since extended modes are actually
		// cheap in terms of the query of this method (pixel shader
		// cost), even though they necessitate a batch break unless
		// we're rendering multiple instances with the exact same
		// blend mode.
		uFeatures &= (~Feature::EXTENDED_MASK);

		// We only consider kDetail and kAlpha in the cost.
		//
		// TODO: If you change this to include kColorAdd or kColorMultiply,
		// you also need to update logic in FalconRenderDrawer::CheckForStateChange().
		//
		// Also, we use internal knowledge that kDetail is effectively
		// "all features", so this is a simple check of kDetail == 2,
		// kAlphaShape == 1, otherwise 0.
		if (kDetail == (kDetail & uFeatures))
		{
			return 2;
		}
		else if (kAlphaShape == (kAlphaShape & uFeatures))
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

	/** @return The raw bitmask of this Features struct. */
	UInt32 GetBits() const { return m_u; }

	// Check what features are currently needed for the render.
	Bool NeedsAlphaShape() const { return (0u != (Feature::kAlphaShape & m_u)); }
	Bool NeedsColorAdd() const { return (0u != (Feature::kColorAdd & m_u)); }
	Bool NeedsColorMultiply() const { return (0u != (Feature::kColorMultiply & m_u)); }
	Bool NeedsDetail() const { return (0u != (Feature::kDetail & m_u)); }
	Bool NeedsExtendedBlendMode() const { return (0u != (Feature::EXTENDED_MASK & m_u)); }
	Bool NeedsExtendedColorAlphaShape() const { return (0u != (Feature::kExtended_ColorAlphaShape & m_u)); }

	/** Set this features struct so all features are disabled/not needed. */
	void Reset()
	{
		 m_u = 0u;
	}

	// Mark these features as needed.
	void SetAlphaShape() { m_u |= Feature::kAlphaShape; }
	void SetColorAdd() { m_u |= Feature::kColorAdd; }
	void SetColorMultiply() { m_u |= Feature::kColorMultiply; }
	void SetDetail() { m_u |= Feature::kDetail; }

	// Mark an arbitrary feature flag.
	void SetFeature(UInt32 u) { m_u |= u; }

private:
	UInt32 m_u;
}; // struct Features

} // namespace Render

} // namespace Seoul::Falcon

#endif // include guard
