/**
 * \file NullGraphicsEffect.h
 * \brief Nop implementation of an Effect for
 * contexts without graphics hardware.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef NULL_GRAPHICS_EFFECT_H
#define NULL_GRAPHICS_EFFECT_H

#include "Effect.h"
#include "UnsafeHandle.h"

namespace Seoul
{

class NullGraphicsEffect SEOUL_SEALED : public Effect
{
public:
	virtual void UnsetAllTextures() SEOUL_OVERRIDE;

	virtual Bool OnCreate() SEOUL_OVERRIDE;
	virtual void OnLost() SEOUL_OVERRIDE;
	virtual void OnReset() SEOUL_OVERRIDE;

private:
	NullGraphicsEffect(
		FilePath filePath,
		void* pRawEffectFileData,
		UInt32 zFileSizeInBytes);
	~NullGraphicsEffect();

	void InternalParseEffectD3D();
	void InternalParseEffectD3D9(UInt8 const* pIn, UInt32 uSize);
	void InternalParseEffectGLSLFXLite();

	SEOUL_REFERENCE_COUNTED_SUBCLASS(NullGraphicsEffect);

	friend class NullGraphicsDevice;
	friend class NullGraphicsRenderCommandStreamBuilder;

	virtual EffectParameterType InternalGetParameterType(UnsafeHandle hHandle) const SEOUL_OVERRIDE;
}; // class NullGraphicsEffect

} // namespace Seoul

#endif // include guard
