/**
 * \file TextureCookCrunch.h
 * \brief Implementation of texture compression using Crunch.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef TEXTURE_COOK_CRUNCH_H
#define TEXTURE_COOK_CRUNCH_H

#include "Image.h"
#include "Prereqs.h"

namespace Seoul::Cooking
{

namespace CompressorCrunch
{

Bool CompressBlocksETC1(
	UInt8 const* pInput,
	UInt32 uWidth,
	UInt32 uHeight,
	Int32 iQuality,
	UInt8*& rpOutput,
	UInt32& ruOutSize);

} // namespace CompressorCrunch

} // namespace Seoul::Cooking

#endif // include guard
