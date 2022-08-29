/**
 * \file D3DCommonEffect.h
 * \brief Shared utility for parsing a D3D9/D3D11 universal Effect
 * file.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef D3D_COMMON_EFFECT_H
#define D3D_COMMON_EFFECT_H

#include "Prereqs.h"

namespace Seoul
{

Bool GetEffectData(
	Bool bD3D11,
	Byte const* pIn,
	UInt32 uIn,
	Byte const*& rpOut,
	UInt32& ruOut);

static inline Bool ValidateEffectData(
	Bool bD3D11,
	Byte const* pIn,
	UInt32 uIn)
{
	Byte const* pUnused = nullptr;
	UInt32 uUnused = 0u;
	return GetEffectData(bD3D11, pIn, uIn, pUnused, uUnused);
}

} // namespace Seoul

#endif // include guard
