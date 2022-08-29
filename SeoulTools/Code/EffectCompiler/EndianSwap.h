/**
 * \file EndianSwap.h
 * \brief Utilities for converting to/from big and little endian.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ENDIAN_SWAP_H
#define ENDIAN_SWAP_H

#include "GLSLFXLite.h"

namespace Seoul
{

template <typename T>
void EndianSwap(T& r);

template <>
inline void EndianSwap<GLSLFXLiteParameterDescription>(GLSLFXLiteParameterDescription& r)
{
	// Sanity check - make sure this conversion function is in-sync with
	// GLSLFXLiteParameterDescription structure.
	SEOUL_STATIC_ASSERT(sizeof(r) == 28u);

	r.m_Class = (GLSLFXparameterclass)EndianSwap32((UInt32)r.m_Class);
	r.m_uColumns = EndianSwap32(r.m_uColumns);
	r.m_uElements = EndianSwap32(r.m_uElements);
	r.m_uRows = EndianSwap32(r.m_uRows);
	r.m_uSize = EndianSwap32(r.m_uSize);
	r.m_Type = (GLSLFXtype)EndianSwap32((UInt32)r.m_Type);
	r.m_hName = EndianSwap16(r.m_hName);
}

template <>
inline void EndianSwap<GLSLFXLiteTechniqueDescription>(GLSLFXLiteTechniqueDescription& r)
{
	// Sanity check - make sure this conversion function is in-sync with
	// GLSLFXLiteTechniqueDescription structure.
	SEOUL_STATIC_ASSERT(sizeof(r) == 8u);

	r.m_uPasses = EndianSwap32(r.m_uPasses);
	r.m_hName = EndianSwap16(r.m_hName);
}

template <>
inline void EndianSwap<GLSLFXLitePassDescription>(GLSLFXLitePassDescription& r)
{
	// Sanity check - make sure this conversion function is in-sync with
	// GLSLFXLitePassDescription structure.
	SEOUL_STATIC_ASSERT(sizeof(r) == 4u);

	r.m_hName = EndianSwap16(r.m_hName);
}

template <>
inline void EndianSwap<GLSLFXLiteParameterData>(GLSLFXLiteParameterData& r)
{
	// Sanity check - make sure this conversion function is in-sync with
	// GLSLFXLiteParameterData structure.
	SEOUL_STATIC_ASSERT(sizeof(r) == 4u);

	r.m_iFixed = EndianSwap32(r.m_iFixed);
}

template <>
inline void EndianSwap<GLSLFXLiteGlobalParameterEntry>(GLSLFXLiteGlobalParameterEntry& r)
{
	// Sanity check - make sure this conversion function is in-sync with
	// GLSLFXLiteGlobalParameterEntry structure.
	SEOUL_STATIC_ASSERT(sizeof(r) == 8u);

	r.m_uIndex = EndianSwap16(r.m_uIndex);
	r.m_uCount = EndianSwap16(r.m_uCount);
	r.m_uDirtyStamp = EndianSwap32(r.m_uDirtyStamp);
}

template <>
inline void EndianSwap<GLSLFXLiteTechniqueEntry>(GLSLFXLiteTechniqueEntry& r)
{
	// Sanity check - make sure this conversion function is in-sync with
	// GLSLFXLiteTechniqueEntry structure.
	SEOUL_STATIC_ASSERT(sizeof(r) == 4u);

	r.m_hFirstPass = EndianSwap16(r.m_hFirstPass);
	r.m_hLastPass = EndianSwap16(r.m_hLastPass);
}

template <>
inline void EndianSwap<GLSLFXLitePassEntry>(GLSLFXLitePassEntry& r)
{
	// Sanity check - make sure this conversion function is in-sync with
	// GLSLFXLitePassEntry structure.
	SEOUL_STATIC_ASSERT(sizeof(r) == 16u);

	r.m_hFirstRenderState = EndianSwap16(r.m_hFirstRenderState);
	r.m_hLastRenderState = EndianSwap16(r.m_hLastRenderState);
	r.m_hPixelShader = EndianSwap16(r.m_hPixelShader);
	r.m_hVertexShader = EndianSwap16(r.m_hVertexShader);
	r.m_Program = EndianSwap32(r.m_Program);
	r.m_hParameterFirst = EndianSwap32(r.m_hParameterFirst);
	r.m_hParameterLast = EndianSwap32(r.m_hParameterLast);
}

template <>
inline void EndianSwap<GLSLFXLiteRenderState>(GLSLFXLiteRenderState& r)
{
	// Sanity check - make sure this conversion function is in-sync with
	// GLSLFXLiteRenderState structure.
	SEOUL_STATIC_ASSERT(sizeof(r) == 8u);

	r.m_uState = EndianSwap32(r.m_uState);
	r.m_uValue = EndianSwap32(r.m_uValue);
}

template <>
inline void EndianSwap<GLSLFXLiteShaderEntry>(GLSLFXLiteShaderEntry& r)
{
	// Sanity check - make sure this conversion function is in-sync with
	// GLSLFXLiteShaderEntry structure.
	SEOUL_STATIC_ASSERT(sizeof(r) == 12u);

	r.m_hShaderCodeFirst = EndianSwap32(r.m_hShaderCodeFirst);
	r.m_hShaderCodeLast = EndianSwap32(r.m_hShaderCodeLast);
	r.m_hDeprecatedName = EndianSwap16(r.m_hDeprecatedName);
	r.m_bIsVertexShader = EndianSwap16(r.m_bIsVertexShader);
}

template <>
inline void EndianSwap<GLSLFXLiteProgramParameter>(GLSLFXLiteProgramParameter& r)
{
	// Sanity check - make sure this conversion function is in-sync with
	// GLSLFXLiteShaderParameter structure.
	SEOUL_STATIC_ASSERT(sizeof(r) == 20);

	r.m_uDirtyStamp = EndianSwap32(r.m_uDirtyStamp);
	r.m_uGlobalParameterIndex = EndianSwap16(r.m_uGlobalParameterIndex);
	r.m_uParameterIndex = EndianSwap16(r.m_uParameterIndex);
	r.m_uParameterCount = EndianSwap16(r.m_uParameterCount);
	r.m_uParameterClass = EndianSwap16(r.m_uParameterClass);
	r.m_iHardwareIndex = EndianSwap32(r.m_iHardwareIndex);
	r.m_hParameterLookupName = EndianSwap16(r.m_hParameterLookupName);
}

/**
 * Endian swap an array of type T.
 */
template <typename T>
inline void EndianSwap(T* p, UInt32 zSize)
{
	SEOUL_ASSERT(p);
	for (UInt32 i = 0u; i < zSize; ++i)
	{
		EndianSwap(p[i]);
	}
}

template <>
inline void EndianSwap<Byte>(Byte* p, UInt32 zSize)
{
	// Nop - no need to endian swap a byte.
}

} // namespace Seoul

#endif // include guard
