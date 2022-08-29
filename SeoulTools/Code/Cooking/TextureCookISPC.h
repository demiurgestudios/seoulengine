/**
 * \file TextureCookISPC.h
 * \brief Implementation of texture compression using Intel ISPC.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef TEXTURE_COOK_ISPC_H
#define TEXTURE_COOK_ISPC_H

#include "TextureCookISPCKernel_ispc.h"
#include "Prereqs.h"

namespace Seoul::Cooking
{

namespace CompressorISPC
{

namespace ETC1Quality
{

enum Enum
{
	kLowest = 0,
	kHighest = 6,
};

} // namespace ETC1Quality

void CompressBlocksDXT1(const ispc::Image& image, UInt8* pOutput);
void CompressBlocksDXT5(const ispc::Image& image, UInt8* pOutput);
void CompressBlocksETC1(const ispc::Image& image, UInt8* pOutput, Int32 iQuality = ETC1Quality::kHighest);

} // namespace CompressorISPC

} // namespace Seoul::Cooking

#endif // include guard
