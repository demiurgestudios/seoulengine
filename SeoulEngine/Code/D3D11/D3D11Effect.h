/**
 * \file D3D11Effect.h
 * \brief Implementation of SeoulEngine Effect for the D3D11. Uses a custom
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
#ifndef D3D11_EFFECT_H
#define D3D11_EFFECT_H

#include "Effect.h"
#include "UnsafeHandle.h"

namespace Seoul
{


class D3D11Effect SEOUL_SEALED : public Effect
{
public:
	virtual void UnsetAllTextures() SEOUL_OVERRIDE;

	virtual Bool OnCreate() SEOUL_OVERRIDE;
	virtual void OnLost() SEOUL_OVERRIDE;
	virtual void OnReset() SEOUL_OVERRIDE;

private:
	D3D11Effect(
		FilePath filePath,
		void* pRawEffectFileData,
		UInt32 uFileSizeInBytes);
	virtual ~D3D11Effect();
	SEOUL_REFERENCE_COUNTED_SUBCLASS(D3D11Effect);

	friend class D3D11Device;
	friend class D3D11RenderCommandStreamBuilder;

	virtual EffectParameterType InternalGetParameterType(UnsafeHandle hHandle) const SEOUL_OVERRIDE;

	void InternalPopulateParameterTable();
	void InternalPopulateTechniqueTable();

	SEOUL_DISABLE_COPY(D3D11Effect);
}; // class Effect

} // namespace Seoul

#endif // include guard
