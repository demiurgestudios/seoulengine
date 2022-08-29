/**
 * \file VertexElement.h
 * \brief Describes one element of a vertex format (or vertex declaration
 * in DX nomenclature).
 *
 * A vertex element can be the description of a position, normal, texture
 * coordinate, or other per-vertex attributes.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef VERTEX_ELEMENT_H
#define VERTEX_ELEMENT_H

#include "Prereqs.h"

namespace Seoul
{

/** Describes one element of a vertex format.
 *
 *  This structure is arranged to match the D3D9 VertexElement
 *  structure exactly.
 */
struct VertexElement SEOUL_SEALED
{
	Bool operator==(const VertexElement& b) const
	{
		return (
			m_Stream == b.m_Stream &&
			m_Offset == b.m_Offset &&
			m_Type == b.m_Type &&
			m_Method == b.m_Method &&
			m_Usage == b.m_Usage &&
			m_UsageIndex == b.m_UsageIndex);
	}

	Bool operator !=(const VertexElement& b) const
	{
		return !(*this == b);
	}

	enum EMethod
	{
		MethodDefault = 0,
		MethodPartialU,
		MethodPartialV,
		MethodCrossUV, // Normal
		MethodUV,
		MethodLookup, // Lookup a displacement map
		MethodLookupPresampled, // Lookup a pre-sampled displacement map
	};

	enum EType
	{
		TypeFloat1    =  0,  // 1D float expanded to (value, 0., 0., 1.)
		TypeFloat2    =  1,  // 2D float expanded to (value, value, 0., 1.)
		TypeFloat3    =  2,  // 3D float expanded to (value, value, value, 1.)
		TypeFloat4    =  3,  // 4D float
		TypeColor     =  4,  // 4D packed unsigned bytes mapped to 0. to 1. range
		// Input is in D3DCOLOR format (ARGB) expanded to (R, G, B, A)
		TypeUByte4    =  5,  // 4D unsigned byte
		TypeShort2    =  6,  // 2D signed short expanded to (value, value, 0., 1.)
		TypeShort4    =  7,  // 4D signed short
		TypeUByte4N   =  8,  // Each of 4 bytes is normalized by dividing to 255.0
		TypeShort2N   =  9,  // 2D signed short normalized (v[0]/32767.0,v[1]/32767.0,0,1)
		TypeShort4N   = 10,  // 4D signed short normalized (v[0]/32767.0,v[1]/32767.0,v[2]/32767.0,v[3]/32767.0)
		TypeUShort2N  = 11,  // 2D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,0,1)
		TypeUShort4N  = 12,  // 4D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,v[2]/65535.0,v[3]/65535.0)
		TypeUDec3     = 13,  // 3D unsigned 10 10 10 format expanded to (value, value, value, 1)
		TypeDec3N     = 14,  // 3D signed 10 10 10 format normalized and expanded to (v[0]/511.0, v[1]/511.0, v[2]/511.0, 1)
		TypeFloat16_2 = 15,  // Two 16-bit floating point values, expanded to (value, value, 0, 1)
		TypeFloat16_4 = 16,  // Four 16-bit floating point values
		TypeUnused    = 17,  // When the type field in a decl is unused.
	};

	/**
	 * Given a VertexElement type, returns the number of
	 * components of that type. For example, Float3 has 3 components.
	 */
	static UInt ComponentCountFromType(EType aType)
	{
		switch (aType)
		{
		case TypeFloat1: return 1;
		case TypeFloat2: return 2;
		case TypeFloat3: return 3;
		case TypeFloat4: return 4;
		// Not a typo - although color has 4 byte components, we need to treat it like it's
		// a single component of 4 bytes for endian correctness.
		case TypeColor: return 1;
		// Not a typo - although ubyte4 has 4 byte components, we need to treat it like it's
		// a single component of 4 bytes for endian correctness.
		case TypeUByte4: return 1;
		case TypeShort2: return 2;
		case TypeShort4: return 4;
		// Not a typo - although ubyte4 has 4 byte components, we need to treat it like it's
		// a single component of 4 bytes for endian correctness.
		case TypeUByte4N: return 1;
		case TypeShort2N: return 2;
		case TypeShort4N: return 4;
		case TypeUShort2N: return 2;
		case TypeUShort4N: return 4;
		case TypeUDec3: return 3;
		case TypeDec3N: return 3;
		case TypeFloat16_2: return 2;
		case TypeFloat16_4: return 4;
		case TypeUnused: // fall-through
		default:
			return 0;
		}
	}

	/**
	 * Given a VertexElement type, returns the size
	 * in bytes of that element.
	 */
	static UInt SizeInBytesFromType(EType aType)
	{
		switch (aType)
		{
		case TypeFloat1: return 4;
		case TypeFloat2: return 8;
		case TypeFloat3: return 12;
		case TypeFloat4: return 16;
		case TypeColor: return 4;
		case TypeUByte4: return 4;
		case TypeShort2: return 4;
		case TypeShort4: return 8;
		case TypeUByte4N: return 4;
		case TypeShort2N: return 4;
		case TypeShort4N: return 8;
		case TypeUShort2N: return 4;
		case TypeUShort4N: return 8;
		case TypeUDec3: return 4;
		case TypeDec3N: return 4;
		case TypeFloat16_2: return 4;
		case TypeFloat16_4: return 8;
		case TypeUnused: // fall-through
		default:
			return 0;
		}
	}

	enum EUsage
	{
		UsagePosition = 0,
		UsageBlendWeight,   // 1
		UsageBlendIndices,  // 2
		UsageNormal,        // 3
		UsagePSize,         // 4
		UsageTexcoord,      // 5
		UsageTangent,       // 6
		UsageBinormal,      // 7
		UsageTessfactor,    // 8
		UsagePositionT,     // 9
		UsageColor,         // 10
		UsageFog,           // 11
		UsageDepth,         // 12
		UsageSample,        // 13
	};

	UInt16 m_Stream;
	UInt16 m_Offset;
	EType m_Type;
	EMethod m_Method;
	EUsage m_Usage;
	UInt32 m_UsageIndex;
}; // struct VertexElement

static const VertexElement VertexElementEnd =
{
	0xFF,
	0u,
	VertexElement::TypeUnused,
	VertexElement::MethodDefault,
	VertexElement::UsagePosition,
	0u};

} // namespace Seoul

#endif // include guard
