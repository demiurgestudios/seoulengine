/**
 * \file ClearFlags.h
 * \brief Enum that defines the various graphics device buffer clear targets
 * that can be selected (color, depth, or stencil).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef CLEAR_FLAGS_H
#define CLEAR_FLAGS_H

#include "Prereqs.h"

namespace Seoul
{

enum class ClearFlags : UInt32
{
	kColorTarget = (1 << 0),
	kDepthTarget = (1 << 1),
	kStencilTarget = (1 << 2),

	kClearAll = (kColorTarget | kDepthTarget | kStencilTarget)
};
static inline constexpr ClearFlags operator|(ClearFlags a, ClearFlags b)
{
	return (ClearFlags)((UInt32)a | (UInt32)b);
}
static inline constexpr ClearFlags operator&(ClearFlags a, ClearFlags b)
{
	return (ClearFlags)((UInt32)a & (UInt32)b);
}

} // namespace Seoul

#endif // include guard
