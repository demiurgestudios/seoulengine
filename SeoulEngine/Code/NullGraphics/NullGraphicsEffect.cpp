/**
 * \file NullGraphicsEffect.cpp
 * \brief Nop implementation of an Effect for
 * contexts without graphics hardware.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EffectConverter.h"
#include "GLSLFXLite.h"
#include "NullGraphicsEffect.h"
#include "StreamBuffer.h"
#include "ThreadId.h"
#include "UnsafeHandle.h"

namespace Seoul
{

NullGraphicsEffect::NullGraphicsEffect(
	FilePath filePath,
	void* pRawEffectFileData,
	UInt32 zFileSizeInBytes)
	: Effect(filePath, pRawEffectFileData, zFileSizeInBytes)
{
}

NullGraphicsEffect::~NullGraphicsEffect()
{
	SEOUL_ASSERT(IsRenderThread());

	m_hHandle.Reset();
}

void NullGraphicsEffect::UnsetAllTextures()
{
	SEOUL_ASSERT(IsRenderThread());
}

void NullGraphicsEffect::OnLost()
{
	SEOUL_ASSERT(IsRenderThread());

	BaseGraphicsObject::OnLost();
}

void NullGraphicsEffect::OnReset()
{
	SEOUL_ASSERT(IsRenderThread());

	BaseGraphicsObject::OnReset();
}

EffectParameterType NullGraphicsEffect::InternalGetParameterType(UnsafeHandle hHandle) const
{
	SEOUL_ASSERT(IsRenderThread());

	return EffectParameterType::kUnknown;
}

Bool NullGraphicsEffect::OnCreate()
{
	SEOUL_ASSERT(IsRenderThread());

	m_hHandle = this;

	// Some basic parsing to get effect info, based on platform.
	if (Platform::kAndroid == keCurrentPlatform ||
		Platform::kIOS == keCurrentPlatform ||
		Platform::kLinux == keCurrentPlatform)
	{
		InternalParseEffectGLSLFXLite();
	}
	else if (Platform::kPC == keCurrentPlatform)
	{
		InternalParseEffectD3D();
	}

	InternalFreeFileData();
	SEOUL_VERIFY(BaseGraphicsObject::OnCreate());

	return true;
}

// TODO: The body of this function is
// a cut and paste from D3DCommon. Not ideal.

/**
 * Parse parameter and technique tables from an expected
 * D3D effect data blob.
 */
void NullGraphicsEffect::InternalParseEffectD3D()
{
	static const UInt32 kuPCEffectSignature = 0x4850A36F;
	static const Int32 kiPCEffectVersion = 1;

	// Clear out tables.
	m_tParametersBySemantic.Clear();
	m_tTechniquesByName.Clear();

	UInt8 const* pIn = (UInt8 const*)m_pRawEffectFileData;
	UInt32 uIn = (UInt32)m_zFileSizeInBytes;

	// Data is too small, abort.
	if (uIn < 24u)
	{
		return;
	}

	// Read the data signature.
	UInt32 uSignature = 0u;
	memcpy(&uSignature, pIn + 0, sizeof(uSignature));
#if SEOUL_BIG_ENDIAN
	EndianSwap32(uSignature);
#endif // /#if SEOUL_BIG_ENDIAN

	// Invalid signature, abort.
	if (uSignature != kuPCEffectSignature)
	{
		return;
	}

	// Read the data version.
	Int32 iVersion = 0;
	memcpy(&iVersion, pIn + 4, sizeof(iVersion));
#if SEOUL_BIG_ENDIAN
	EndianSwap32(iVersion);
#endif // /#if SEOUL_BIG_ENDIAN

	// Invalid version, abort.
	if (iVersion != kiPCEffectVersion)
	{
		return;
	}

	// Get offsets to the D3D9 data.
	UInt32 uOffset = 0u;
	UInt32 uSize = 0u;
	memcpy(&uOffset, pIn + 16, sizeof(uOffset));
	memcpy(&uSize, pIn + 20, sizeof(uSize));

	// Failure if offset or size are invalid.
	if (uOffset < 24u || uOffset + uSize > uIn)
	{
		return;
	}

	// Otherwise, continue. Parse the D3D9 blob.
	InternalParseEffectD3D9(pIn + uOffset, uSize);
}

// TODO: The body of this function is
// mostly a cut and paste from D3D9Effect::InternalGetParameterType().
// Not ideal.

/** Convert data from D3D9 data into a runtime EffectParameterType value. */
static inline EffectParameterType Convert(
	const EffectConverter::Util::Parameter& param)
{
	using namespace EffectConverter::Util;

	// Use the general Array type if the
	// description has an element count. It will
	// be 0u if it's a single value and not an array.
	if (param.m_uElements > 0u)
	{
		return EffectParameterType::kArray;
	}

	switch (param.m_eClass)
	{
	case ParameterClass::kScalar:
		switch (param.m_eType)
		{
		case ParameterType::kFloat:
			return EffectParameterType::kFloat;
		case ParameterType::kInt:
			return EffectParameterType::kInt;
		case ParameterType::kBool:
			return EffectParameterType::kBool;
		default:
			break;
		};
		break;
	case ParameterClass::kVector:
		if (ParameterType::kFloat == param.m_eType)
		{
			if (1u == param.m_uRows)
			{
				switch (param.m_uColumns)
				{
				case 2u: return EffectParameterType::kVector2D;
				case 3u: return EffectParameterType::kVector3D;
				case 4u: return EffectParameterType::kVector4D;
				default:
					break;
				};
			}
		}
		break;
	case ParameterClass::kMatrixRows: // fall-through
	case ParameterClass::kMatrixColumns:
		if (ParameterType::kFloat == param.m_eType)
		{
			return EffectParameterType::kMatrix4D;
		}
		break;
	case ParameterClass::kObject:
		switch (param.m_eType)
		{
		case ParameterType::kTexture: // fall-through
		case ParameterType::kTexture1D: // fall-through
		case ParameterType::kTexture2D: // fall-through
		case ParameterType::kTexture3D: // fall-through
		case ParameterType::kTextureCube:
			return EffectParameterType::kTexture;
		default:
			break;
		};
	default:
		break;
	};

	return EffectParameterType::kUnknown;
}

/**
 * Once we've found the D3D9 blob of the total data, parse it into
 * parameter and technique tables.
 */
void NullGraphicsEffect::InternalParseEffectD3D9(UInt8 const* pIn, UInt32 uSize)
{
	// Use an EffectConverter to parse the data.
	// On failure, abort.
	EffectConverter::Converter converter;
	if (!converter.ProcessBytecode(pIn, uSize))
	{
		return;
	}

	// Parameters.
	{
		auto const& tParameters = converter.GetParameters();
		auto const iBegin = tParameters.Begin();
		auto const iEnd = tParameters.End();

		m_tParametersBySemantic.Clear();
		size_t zHandle = 0;
		for (auto i = iBegin; iEnd != i; ++i)
		{
			ParameterEntry entry;
			entry.m_eType = Convert(*i);
			entry.m_hHandle.V = ++zHandle;
			(void)m_tParametersBySemantic.Insert(
				i->m_Semantic,
				entry);
		}
	}

	// Techniques.
	{
		auto const& tTechniques = converter.GetTechniques();
		auto const iBegin = tTechniques.Begin();
		auto const iEnd = tTechniques.End();

		m_tTechniquesByName.Clear();
		size_t zHandle = 0;
		for (auto i = iBegin; iEnd != i; ++i)
		{
			TechniqueEntry entry;
			entry.m_hHandle.V = ++zHandle;
			entry.m_uPassCount = i->m_vPasses.GetSize();
			(void)m_tTechniquesByName.Insert(
				i->m_Name,
				entry);
		}
	}
}

// TODO: The body of this function is
// mostly a cut and paste from OGLES2Effect::InternalGetParameterType().
// Not ideal.

/** Convert data from GLSLFXLite data into a runtime EffectParameterType value. */
static inline EffectParameterType Convert(const GLSLFXLiteParameterDescription& desc)
{
	// Any array type is type EffectParameterType::kArray, independent
	// from its per-element type.
	if (GLSLFX_PARAMETERCLASS_ARRAY == desc.m_Class)
	{
		return EffectParameterType::kArray;
	}

	switch (desc.m_Class)
	{
	case GLSLFX_PARAMETERCLASS_SCALAR:
		switch (desc.m_Type)
		{
		case GLSLFX_FLOAT:
			return EffectParameterType::kFloat;
		case GLSLFX_INT:
			return EffectParameterType::kInt;
		case GLSLFX_BOOL:
			return EffectParameterType::kBool;
		default:
			break;
		};
		break;
	case GLSLFX_PARAMETERCLASS_VECTOR:
		switch (desc.m_Type)
		{
		case GLSLFX_FLOAT1: return EffectParameterType::kFloat;
		case GLSLFX_FLOAT2: return EffectParameterType::kVector2D;
		case GLSLFX_FLOAT3: return EffectParameterType::kVector3D;
		case GLSLFX_FLOAT4: return EffectParameterType::kVector4D;
		default:
			break;
		};
		break;
	case GLSLFX_PARAMETERCLASS_MATRIX:
		if (GLSLFX_FLOAT4x4 == desc.m_Type)
		{
			return EffectParameterType::kMatrix4D;
		}
		break;
	case GLSLFX_PARAMETERCLASS_SAMPLER:
		return EffectParameterType::kTexture;
	default:
		break;
	};

	return EffectParameterType::kUnknown;
}

// TODO: This body is mostly a cut and paste
// of the GLSLFXLite() constructor in the OGLES2
// project. Not ideal.

/**
 * Parse GLSLFXLite data into parameter and technique
 * tables.
 */
void NullGraphicsEffect::InternalParseEffectGLSLFXLite()
{
	using namespace GLSLFXLiteUtil;

	// Get data, and finalize.
	GLSLFXLiteDataSerialized* p = (GLSLFXLiteDataSerialized*)m_pRawEffectFileData;
	GLSLFXLiteDataRuntime data;
	SetupSerializedData(p, data);

	// Parameters.
	{
		m_tParametersBySemantic.Clear();
		size_t zHandle = 0;
		for (UInt32 i = 0u; i < data.m_Description.m_uParameters; ++i)
		{
			auto const& param = data.m_pParameters[i];

			ParameterEntry entry;
			entry.m_eType = Convert(param);
			entry.m_hHandle.V = ++zHandle;
			(void)m_tParametersBySemantic.Insert(
				GetHString(data.m_pStrings, param.m_hName),
				entry);
		}
	}

	// Techniques.
	{
		m_tTechniquesByName.Clear();
		size_t zHandle = 0;
		for (UInt32 i = 0u; i < data.m_Description.m_uTechniques; ++i)
		{
			auto const& tech = data.m_pTechniques[i];

			TechniqueEntry entry;
			entry.m_hHandle = ++zHandle;
			entry.m_uPassCount = tech.m_uPasses;
			(void)m_tTechniquesByName.Insert(
				GetHString(data.m_pStrings, tech.m_hName),
				entry);
		}
	}
}

} // namespace Seoul
