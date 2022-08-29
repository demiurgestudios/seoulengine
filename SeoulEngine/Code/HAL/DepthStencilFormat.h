/**
 * \file DepthStencilFormat.h
 * \brief Enum defining valid pixel formats for depth-stencil formats.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef DEPTH_STENCIL_FORMAT_H
#define DEPTH_STENCIL_FORMAT_H

#include "Prereqs.h"

namespace Seoul
{

class HString;

enum class DepthStencilFormat : Int32
{
	kInvalid = 0,
	kD16_LOCKABLE,
	kD32,
	kD15S1,
	kD24S8,
	kD24FS8,
	kD24X8,
	kD24X4S4,
	kD16,
	kD16S8,
	DSF_COUNT
};

/**
 * Returns true if eFormat is a depth-stencil format that
 * includes a stencil buffer, false otherwise.
 */
inline Bool DepthStencilFormatHasStencilBuffer(DepthStencilFormat eFormat)
{
	switch (eFormat)
	{
		case DepthStencilFormat::kD16_LOCKABLE: // fall-through
		case DepthStencilFormat::kD32: // fall-through
		case DepthStencilFormat::kD24X8: // fall-through
		case DepthStencilFormat::kD16:
			return false;

		case DepthStencilFormat::kD24X4S4: // fall-through
		case DepthStencilFormat::kD15S1: // fall-through
		case DepthStencilFormat::kD24S8: // fall-through
		case DepthStencilFormat::kD24FS8: // fall-through
		case DepthStencilFormat::kD16S8:
			return true;

		default:
			return false;
	};
}

UInt32 DepthStencilFormatBytesPerPixel(DepthStencilFormat eFormat);

extern DepthStencilFormat HStringToDepthStencilFormat(HString hstring);

} // namespace Seoul

#endif // include guard
