/**
 * \file FalconRenderMode.h
 * \brief The global mode for the Falcon render backend at
 * any one time.
 *
 * Render::Modes other than kDefault are currently for
 * development only and mostly used for debugging and visualization.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_RENDER_MODE_H
#define FALCON_RENDER_MODE_H

#include "Prereqs.h"

namespace Seoul::Falcon
{

namespace Render
{

#if SEOUL_ENABLE_CHEATS
// Special render modes that can be enabled. All developer only.
namespace Mode
{

enum Enum
{
	// Standard rendering, no special mode.
	kDefault,

	// Occlusion visualization.
	kOcclusion,

	// Overdraw visualization.
	kOverdraw,

	// Texture resolution visualization (mip selection mode).
	kTextureResolution,

	// World bounds visualization.
	kWorldBounds,
};

/** @return true if a given renderer mode supports two-pass shadows, false otherwise. */
static inline Bool SupportsTwoPassShadow(Enum e)
{
	switch (e)
	{
	case kDefault:
	case kOcclusion:
	case kTextureResolution:
	case kWorldBounds:
		return true;
	case kOverdraw:
	default:
		return false;
	};
}

} // namespace Mode
#endif // /#if SEOUL_ENABLE_CHEATS

} // namespace Render

} // namespace Seoul::Falcon

#endif // include guard
