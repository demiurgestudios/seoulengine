/**
 * \file OGLES2Effect.h
 * \brief Implementation of SeoulEngine Effect for the OGLES2. Uses a custom
 * shader Effect system, GLSLFXLite, to handle the low-level tasks
 * of management effect samplers, render states, and shader parameters.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef OGLES2_EFFECT_H
#define OGLES2_EFFECT_H

#include "Effect.h"
#include "UnsafeHandle.h"

namespace Seoul
{


class OGLES2Effect SEOUL_SEALED : public Effect
{
public:
	virtual void UnsetAllTextures() SEOUL_OVERRIDE;

	virtual Bool OnCreate() SEOUL_OVERRIDE;
	virtual void OnLost() SEOUL_OVERRIDE;
	virtual void OnReset() SEOUL_OVERRIDE;

private:
	OGLES2Effect(
		FilePath filePath,
		void* pRawEffectFileData,
		UInt32 zFileSizeInBytes);
	virtual ~OGLES2Effect();
	SEOUL_REFERENCE_COUNTED_SUBCLASS(OGLES2Effect);

	friend class OGLES2RenderDevice;
	friend class OGLES2RenderCommandStreamBuilder;

	virtual EffectParameterType InternalGetParameterType(UnsafeHandle hHandle) const SEOUL_OVERRIDE;

	void InternalPopulateParameterTable();
	void InternalPopulateTechniqueTable();
}; // class Effect

} // namespace Seoul

#endif // include guard
