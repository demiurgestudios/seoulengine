/**
 * \file EffectReceiverGLSLES2Util.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EFFECT_RECEIVER_GLSLES2_UTIL_H
#define EFFECT_RECEIVER_GLSLES2_UTIL_H

#include "D3DUtil.h"
#include "EffectConverter.h"
#include "GLSLFXLite.h"
#include "OGLES2StateManager.h"

namespace Seoul
{

inline static Bool IsNullOrEmpty(Byte const* sString)
{
	return !sString || !sString[0];
}

inline static Bool IsNullOrEmpty(HString s)
{
	return s.IsEmpty();
}

inline static GLSLFXparameterclass ConvertClass(const EffectConverter::Util::Parameter& in)
{
	using namespace EffectConverter::Util;

	// Any parameter with an element count > 0u is an array type.
	if (in.m_uElements > 0u)
	{
		return GLSLFX_PARAMETERCLASS_ARRAY;
	}

	switch (in.m_eClass)
	{
	case ParameterClass::kScalar: return GLSLFX_PARAMETERCLASS_SCALAR;
	case ParameterClass::kVector: return GLSLFX_PARAMETERCLASS_VECTOR;
	case ParameterClass::kMatrixRows: return GLSLFX_PARAMETERCLASS_MATRIX;
	case ParameterClass::kMatrixColumns: return GLSLFX_PARAMETERCLASS_MATRIX;
	case ParameterClass::kObject:
		switch (in.m_eType)
		{
		case ParameterType::kSampler: // fall-through
		case ParameterType::kSampler1D: // fall-through
		case ParameterType::kSampler2D: // fall-through
		case ParameterType::kSampler3D: // fall-through
		case ParameterType::kSamplerCube:
			return GLSLFX_PARAMETERCLASS_SAMPLER;

		case ParameterType::kPixelShader: // fall-through
		case ParameterType::kVertexShader: // fall-through
		case ParameterType::kPixelFragment: // fall-through
		case ParameterType::kVertexFragment:
			return GLSLFX_PARAMETERCLASS_OBJECT;

			// All remaining types are either unsupported or intentionally ignored.
		case ParameterType::kTexture: // fall-through
		case ParameterType::kTexture1D: // fall-through
		case ParameterType::kTexture2D: // fall-through
		case ParameterType::kTexture3D: // fall-through
		case ParameterType::kTextureCube: // fall-through
		case ParameterType::kUnsupported: // fall-through
		default:
			return GLSLFX_PARAMETERCLASS_UNKNOWN;
		};
	case ParameterClass::kStruct:
		return GLSLFX_PARAMETERCLASS_STRUCT;
	default:
		return GLSLFX_PARAMETERCLASS_UNKNOWN;
	};
}

inline static GLSLFXtype ConvertTypeBasedOnDimensionsAndClass(
	GLSLFXtype const eScalarType,
	GLSLFXtype const aeVectorTypes[4],
	GLSLFXtype const aeMatrixTypes[4][4],
	const EffectConverter::Util::Parameter& in)
{
	using namespace EffectConverter::Util;

	switch (in.m_eClass)
	{
		// Scalar is always just a scalar.
	case ParameterClass::kScalar:
		return eScalarType;

		// Vectors can only be converted if they have a columns count
		// between 1 and 4 and a row count of 1.
	case ParameterClass::kVector:
		if (1u == in.m_uRows &&
			in.m_uColumns >= 1u && in.m_uColumns <= 4u)
		{
			return aeVectorTypes[in.m_uColumns - 1u];
		}
		else
		{
			return GLSLFX_UNKNOWN_TYPE;
		}

		// Matrices are valid with row and column counts between 1 and 4.
	case ParameterClass::kMatrixRows: // fall-through
	case ParameterClass::kMatrixColumns:
		if (in.m_uColumns >= 1u && in.m_uColumns <= 4u &&
			in.m_uRows >= 1u && in.m_uColumns <= 4u)
		{
			return aeMatrixTypes[in.m_uRows - 1u][in.m_uColumns - 1u];
		}
		else
		{
			return GLSLFX_UNKNOWN_TYPE;
		}

	default:
		return GLSLFX_UNKNOWN_TYPE;
	};
}

inline static GLSLFXtype ConvertType(const EffectConverter::Util::Parameter& in)
{
	using namespace EffectConverter::Util;

	// Vector type sets for bools, ints, floats, and half-floats.
	static const GLSLFXtype kaeBoolVector[4] = { GLSLFX_BOOL1, GLSLFX_BOOL2, GLSLFX_BOOL3, GLSLFX_BOOL4 };
	static const GLSLFXtype kaeIntVector[4] = { GLSLFX_INT1, GLSLFX_INT2, GLSLFX_INT3, GLSLFX_INT4 };
	static const GLSLFXtype kaeFloatVector[4] = { GLSLFX_FLOAT1, GLSLFX_FLOAT2, GLSLFX_FLOAT3, GLSLFX_FLOAT4 };
	static const GLSLFXtype kaeHalfFloatVector[4] = { GLSLFX_HALF1, GLSLFX_HALF2, GLSLFX_HALF3, GLSLFX_HALF4 };

	// Matrix type sets for bools, ints, floats, and half-floats.
	static const GLSLFXtype kaeBoolMatrix[4][4] =
	{
		{ GLSLFX_BOOL1x1, GLSLFX_BOOL1x2, GLSLFX_BOOL1x3, GLSLFX_BOOL1x4 },
		{ GLSLFX_BOOL2x1, GLSLFX_BOOL2x2, GLSLFX_BOOL2x3, GLSLFX_BOOL2x4 },
		{ GLSLFX_BOOL3x1, GLSLFX_BOOL3x2, GLSLFX_BOOL3x3, GLSLFX_BOOL3x4 },
		{ GLSLFX_BOOL4x1, GLSLFX_BOOL4x2, GLSLFX_BOOL4x3, GLSLFX_BOOL4x4 }
	};

	static const GLSLFXtype kaeIntMatrix[4][4] =
	{
		{ GLSLFX_INT1x1, GLSLFX_INT1x2, GLSLFX_INT1x3, GLSLFX_INT1x4 },
		{ GLSLFX_INT2x1, GLSLFX_INT2x2, GLSLFX_INT2x3, GLSLFX_INT2x4 },
		{ GLSLFX_INT3x1, GLSLFX_INT3x2, GLSLFX_INT3x3, GLSLFX_INT3x4 },
		{ GLSLFX_INT4x1, GLSLFX_INT4x2, GLSLFX_INT4x3, GLSLFX_INT4x4 }
	};

	static const GLSLFXtype kaeFloatMatrix[4][4] =
	{
		{ GLSLFX_FLOAT1x1, GLSLFX_FLOAT1x2, GLSLFX_FLOAT1x3, GLSLFX_FLOAT1x4 },
		{ GLSLFX_FLOAT2x1, GLSLFX_FLOAT2x2, GLSLFX_FLOAT2x3, GLSLFX_FLOAT2x4 },
		{ GLSLFX_FLOAT3x1, GLSLFX_FLOAT3x2, GLSLFX_FLOAT3x3, GLSLFX_FLOAT3x4 },
		{ GLSLFX_FLOAT4x1, GLSLFX_FLOAT4x2, GLSLFX_FLOAT4x3, GLSLFX_FLOAT4x4 }
	};

	static const GLSLFXtype kaeHalfFloatMatrix[4][4] =
	{
		{ GLSLFX_HALF1x1, GLSLFX_HALF1x2, GLSLFX_HALF1x3, GLSLFX_HALF1x4 },
		{ GLSLFX_HALF2x1, GLSLFX_HALF2x2, GLSLFX_HALF2x3, GLSLFX_HALF2x4 },
		{ GLSLFX_HALF3x1, GLSLFX_HALF3x2, GLSLFX_HALF3x3, GLSLFX_HALF3x4 },
		{ GLSLFX_HALF4x1, GLSLFX_HALF4x2, GLSLFX_HALF4x3, GLSLFX_HALF4x4 }
	};

	switch (in.m_eType)
	{
	case ParameterType::kBool:
		return ConvertTypeBasedOnDimensionsAndClass(GLSLFX_BOOL, kaeBoolVector, kaeBoolMatrix, in);
	case ParameterType::kInt:
		return ConvertTypeBasedOnDimensionsAndClass(GLSLFX_INT, kaeIntVector, kaeIntMatrix, in);
	case ParameterType::kFloat:
		{
			// An Elements count of 0u indicates a scalar, but in this context,
			// we should treat is as 1u element.
			UInt32 const zPerComponentSizeInBytes =
				(in.GetSizeInBytes() / (in.m_uRows * in.m_uColumns * Max(1u, in.m_uElements)));

			// Float 32-bit
			if (4u == zPerComponentSizeInBytes)
			{
				return ConvertTypeBasedOnDimensionsAndClass(GLSLFX_FLOAT, kaeFloatVector, kaeFloatMatrix, in);
			}
			// Half 16-bit
			else if (2u == zPerComponentSizeInBytes)
			{
				return ConvertTypeBasedOnDimensionsAndClass(GLSLFX_HALF, kaeHalfFloatVector, kaeHalfFloatMatrix, in);
			}
			// Unknown
			else
			{
				return GLSLFX_UNKNOWN_TYPE;
			}
		}

	case ParameterType::kString:
		return GLSLFX_STRING;

	case ParameterType::kSampler1D:
		return GLSLFX_SAMPLER1D;
	case ParameterType::kSampler: // fall-through
	case ParameterType::kSampler2D:
		return GLSLFX_SAMPLER2D;
	case ParameterType::kSampler3D:
		return GLSLFX_SAMPLER3D;
	case ParameterType::kSamplerCube:
		return GLSLFX_SAMPLERCUBE;

	case ParameterType::kPixelShader: // fall-through
	case ParameterType::kVertexShader: // fall-through
	case ParameterType::kPixelFragment: // fall-through
	case ParameterType::kVertexFragment:
		return GLSLFX_PROGRAM_TYPE;

		// The following types are either intentionally ignored or
		// not supported.
	case ParameterType::kVoid: // fall-through
	case ParameterType::kTexture1D: // fall-through
	case ParameterType::kTexture: // fall-through
	case ParameterType::kTexture2D: // fall-through
	case ParameterType::kTexture3D: // fall-through
	case ParameterType::kTextureCube: // fall-through
	case ParameterType::kUnsupported: // fall-through
	default:
		return GLSLFX_UNKNOWN_TYPE;
	};
}

inline static GLSLFXLiteHandle CreateGLSLFXLiteString(
	Byte const* psString,
	Vector<Byte>& rvStrings)
{
	// Empty strings are use the invalid handle.
	if (IsNullOrEmpty(psString))
	{
		return 0u;
	}

	// Walk the list of existing strings - if one matches
	// psString, return that string instead of inserting
	// a new one.
	UInt32 nOffset = 0u;
	while (nOffset < rvStrings.GetSize())
	{
		// If this string matches psString, use it.
		if (0 == strcmp(rvStrings.Get(nOffset), psString))
		{
			return (nOffset + 1u);
		}
		else
		{
			// Add 1 to also skip the null terminator.
			nOffset += StrLen(rvStrings.Get(nOffset)) + 1u;
		}
	}

	UInt32 const zStringLength = StrLen(psString);
	UInt32 const zOriginalSize = rvStrings.GetSize();

	// Insert a new string
	rvStrings.Resize(zOriginalSize + zStringLength);
	memcpy(rvStrings.Get(zOriginalSize), psString, zStringLength);
	rvStrings.PushBack('\0');

	// The handle is the original size of the strings buffer + 1u.
	return (zOriginalSize + 1u);
}

inline static GLSLFXLiteHandle CreateGLSLFXLiteString(
	HString str,
	Vector<Byte>& rvStrings)
{
	return CreateGLSLFXLiteString(
		str.CStr(),
		rvStrings);
}

inline static Bool Convert(
	const EffectConverter::Util::Parameter& in,
	Vector<Byte>& rvStrings,
	GLSLFXLiteParameterDescription& rOut)
{
	GLSLFXLiteParameterDescription ret;
	memset(&ret, 0, sizeof(ret));

	// Convert standard parameters.
	ret.m_Class = ConvertClass(in);
	ret.m_uColumns = in.m_uColumns;
	ret.m_uElements = in.m_uElements;
	ret.m_uRows = in.m_uRows;
	ret.m_Type = ConvertType(in);
	ret.m_uSize = in.GetSizeInBytes();

	// If the resulting parameter passes all checks, it is a valid
	// parameter - add its semantic string and return true.
	if (GLSLFX_PARAMETERCLASS_UNKNOWN != ret.m_Class &&
		GLSLFX_UNKNOWN_TYPE != ret.m_Type &&
		!IsNullOrEmpty(in.m_Semantic) &&
		ret.m_uSize > 0u &&
		(0u == (ret.m_uSize % sizeof(GLSLFXLiteParameterData))))
	{
		ret.m_hName = CreateGLSLFXLiteString(in.m_Semantic, rvStrings);
		rOut = ret;

		return true;
	}
	else
	{
		return false;
	}
}

inline static void Convert(
	const EffectConverter::Util::Technique& in,
	Vector<Byte>& rvStrings,
	GLSLFXLiteTechniqueDescription& rOut)
{
	// Sanity check - make sure this conversion function is in-sync with
	// GLSLFXLiteEffectDescription structure.
	SEOUL_STATIC_ASSERT(sizeof(GLSLFXLiteTechniqueDescription) == 8u);
	memset(&rOut, 0, sizeof(rOut));

	rOut.m_uPasses = in.m_vPasses.GetSize();
	rOut.m_hName = CreateGLSLFXLiteString(in.m_Name, rvStrings);
}

inline static void Convert(
	const EffectConverter::Util::Pass& in,
	Vector<Byte>& rvStrings,
	GLSLFXLitePassDescription& rOut)
{
	// Sanity check - make sure this conversion function is in-sync with
	// GLSLFXLiteEffectDescription structure.
	SEOUL_STATIC_ASSERT(sizeof(GLSLFXLitePassDescription) == 4u);
	memset(&rOut, 0, sizeof(rOut));

	rOut.m_hName = CreateGLSLFXLiteString(in.m_Name, rvStrings);
}

inline static UInt32 ConvertToBlendOp(UInt32 uValue)
{
	switch (uValue)
	{
	case D3DBLENDOP_ADD: return GL_FUNC_ADD;
	case D3DBLENDOP_SUBTRACT: return GL_FUNC_SUBTRACT;
	case D3DBLENDOP_REVSUBTRACT: return GL_FUNC_REVERSE_SUBTRACT;
	case D3DBLENDOP_MIN: return GL_MIN_EXT;
	case D3DBLENDOP_MAX: return GL_MAX_EXT;
	default:
		return 0u;
	};
}

inline static UInt32 ConvertToBooleanValue(UInt32 uValue)
{
	return (FALSE != uValue) ? GL_TRUE : GL_FALSE;
}

inline static UInt32 ConvertToCompareFunction(UInt32 uValue)
{
	switch (uValue)
	{
	case D3DCMP_NEVER: return GL_NEVER;
	case D3DCMP_LESS: return GL_LESS;
	case D3DCMP_EQUAL: return GL_EQUAL;
	case D3DCMP_LESSEQUAL: return GL_LEQUAL;
	case D3DCMP_GREATER: return GL_GREATER;
	case D3DCMP_NOTEQUAL: return GL_NOTEQUAL;
	case D3DCMP_GREATEREQUAL: return GL_GEQUAL;
	case D3DCMP_ALWAYS: return GL_ALWAYS;
	default:
		return 0u;
	};
}

inline static UInt32 ConvertToCullMode(UInt32 uValue)
{
	switch (uValue)
	{
	case D3DCULL_NONE:
		return (UInt32)CullMode::kNone;
	case D3DCULL_CW:
		return (UInt32)CullMode::kClockwise;
	case D3DCULL_CCW:
		return (UInt32)CullMode::kCounterClockwise;
	default:
		return 0u;
	};
}

inline static UInt32 ConvertToBlend(UInt32 uValue)
{
	switch (uValue)
	{
	case D3DBLEND_ZERO: return GL_ZERO;
	case D3DBLEND_ONE: return GL_ONE;
	case D3DBLEND_SRCCOLOR: return GL_SRC_COLOR;
	case D3DBLEND_INVSRCCOLOR: return GL_ONE_MINUS_SRC_COLOR;
	case D3DBLEND_SRCALPHA: return GL_SRC_ALPHA;
	case D3DBLEND_INVSRCALPHA: return GL_ONE_MINUS_SRC_ALPHA;
	case D3DBLEND_DESTALPHA: return GL_DST_ALPHA;
	case D3DBLEND_INVDESTALPHA: return GL_ONE_MINUS_DST_ALPHA;
	case D3DBLEND_DESTCOLOR: return GL_DST_COLOR;
	case D3DBLEND_INVDESTCOLOR: return GL_ONE_MINUS_DST_COLOR;
	case D3DBLEND_SRCALPHASAT: return GL_SRC_ALPHA_SATURATE;
	case D3DBLEND_BLENDFACTOR: return GL_CONSTANT_ALPHA;
	case D3DBLEND_INVBLENDFACTOR: return GL_ONE_MINUS_CONSTANT_ALPHA;
	case D3DBLEND_SRCCOLOR2: return GL_CONSTANT_COLOR;
	case D3DBLEND_INVSRCCOLOR2: return GL_ONE_MINUS_CONSTANT_COLOR;
	default:
		return 0u;
	};
}

inline static UInt32 ConvertToFillMode(UInt32 uValue)
{
	switch (uValue)
	{
	case D3DFILL_POINT: return 0u; // OpenGL ES does not support GL_POINT;
	case D3DFILL_WIREFRAME: return 0u; // OpenGL ES does not support GL_LINE;
	case D3DFILL_SOLID: return 0u; // OpenGL ES does not support GL_FILL;
	default:
		return 0u;
	};
}

inline static UInt32 ConvertToStencilOp(UInt32 uValue)
{
	switch (uValue)
	{
	case D3DSTENCILOP_KEEP: return GL_KEEP;
	case D3DSTENCILOP_ZERO: return GL_ZERO;
	case D3DSTENCILOP_REPLACE: return GL_REPLACE;
	case D3DSTENCILOP_INCRSAT: return GL_INCR;
	case D3DSTENCILOP_DECRSAT: return GL_DECR;
	case D3DSTENCILOP_INVERT: return GL_INVERT;
	case D3DSTENCILOP_INCR: return GL_INCR_WRAP;
	case D3DSTENCILOP_DECR: return GL_DECR_WRAP;
	default:
		return 0u;
	};
}

inline static UInt32 ConvertToShadeMode(UInt32 uValue)
{
	switch (uValue)
	{
	case D3DSHADE_FLAT:
		return 0u; // OpenGL ES does not support GL_FLAT;
	case D3DSHADE_GOURAUD: // fall-through
	case D3DSHADE_PHONG:
		return 0u; // OpenGL ES does not support GL_SMOOTH;
	default:
		return 0u;
	};
}

inline static GLSLFXLiteRenderState Convert(const EffectConverter::Util::RenderState& state)
{
	GLSLFXLiteRenderState ret;
	memset(&ret, 0, sizeof(ret));

	switch (state.m_eState)
	{
	case EffectConverter::Util::RenderStateType::kAlphaBlendEnable:
		ret.m_uState = (UInt32)RenderState::kAlphaBlendEnable;
		ret.m_uValue = ConvertToBooleanValue(state.m_uValue);
		break;
	case EffectConverter::Util::RenderStateType::kAlphaFunc:
		ret.m_uState = (UInt32)RenderState::kAlphaFunction;
		ret.m_uValue = ConvertToCompareFunction(state.m_uValue);
		break;
	case EffectConverter::Util::RenderStateType::kAlphaRef:
		ret.m_uState = (UInt32)RenderState::kAlphaReference;
		ret.m_uValue = state.m_uValue;
		break;
	case EffectConverter::Util::RenderStateType::kAlphaTestEnable:
		ret.m_uState = (UInt32)RenderState::kAlphaTestEnable;
		ret.m_uValue = ConvertToBooleanValue(state.m_uValue);
		break;
	case EffectConverter::Util::RenderStateType::kCcwStencilZFail:
		ret.m_uState = (UInt32)RenderState::kBackFacingStencilDepthFail;
		ret.m_uValue = ConvertToStencilOp(state.m_uValue);
		break;
	case EffectConverter::Util::RenderStateType::kCcwStencilFail:
		ret.m_uState = (UInt32)RenderState::kBackFacingStencilFail;
		ret.m_uValue = ConvertToStencilOp(state.m_uValue);
		break;
	case EffectConverter::Util::RenderStateType::kCcwStencilFunc:
		ret.m_uState = (UInt32)RenderState::kBackFacingStencilFunc;
		ret.m_uValue = ConvertToCompareFunction(state.m_uValue);
		break;
	case EffectConverter::Util::RenderStateType::kCcwStencilPass:
		ret.m_uState = (UInt32)RenderState::kBackFacingStencilPass;
		ret.m_uValue = ConvertToStencilOp(state.m_uValue);
		break;
	case EffectConverter::Util::RenderStateType::kBlendFactor:
		ret.m_uState = (UInt32)RenderState::kBlendColor;
		ret.m_uValue = state.m_uValue;
		break;
	case EffectConverter::Util::RenderStateType::kBlendOp:
		ret.m_uState = (UInt32)RenderState::kBlendOp;
		ret.m_uValue = ConvertToBlendOp(state.m_uValue);
		break;
	case EffectConverter::Util::RenderStateType::kBlendOpAlpha:
		ret.m_uState = (UInt32)RenderState::kBlendOpAlpha;
		ret.m_uValue = ConvertToBlendOp(state.m_uValue);
		break;
	case EffectConverter::Util::RenderStateType::kColorWriteEnable:
		ret.m_uState = (UInt32)RenderState::kColorWriteEnable;
		ret.m_uValue = 0u;
		if ((0u != (D3DCOLORWRITEENABLE_RED & state.m_uValue)))
		{
			RenderStateUtil::SetComponent8<Components8Bit::kColorMaskR>(GL_TRUE, ret.m_uValue);
		}
		if ((0u != (D3DCOLORWRITEENABLE_GREEN & state.m_uValue)))
		{
			RenderStateUtil::SetComponent8<Components8Bit::kColorMaskG>(GL_TRUE, ret.m_uValue);
		}
		if ((0u != (D3DCOLORWRITEENABLE_BLUE & state.m_uValue)))
		{
			RenderStateUtil::SetComponent8<Components8Bit::kColorMaskB>(GL_TRUE, ret.m_uValue);
		}
		if ((0u != (D3DCOLORWRITEENABLE_ALPHA & state.m_uValue)))
		{
			RenderStateUtil::SetComponent8<Components8Bit::kColorMaskA>(GL_TRUE, ret.m_uValue);
		}
		break;
	case EffectConverter::Util::RenderStateType::kColorWriteEnable1:
		ret.m_uState = (UInt32)RenderState::kColorWriteEnable1;
		ret.m_uValue = 0u;
		if ((0u != (D3DCOLORWRITEENABLE_RED & state.m_uValue)))
		{
			RenderStateUtil::SetComponent8<Components8Bit::kColorMaskR>(GL_TRUE, ret.m_uValue);
		}
		if ((0u != (D3DCOLORWRITEENABLE_GREEN & state.m_uValue)))
		{
			RenderStateUtil::SetComponent8<Components8Bit::kColorMaskG>(GL_TRUE, ret.m_uValue);
		}
		if ((0u != (D3DCOLORWRITEENABLE_BLUE & state.m_uValue)))
		{
			RenderStateUtil::SetComponent8<Components8Bit::kColorMaskB>(GL_TRUE, ret.m_uValue);
		}
		if ((0u != (D3DCOLORWRITEENABLE_ALPHA & state.m_uValue)))
		{
			RenderStateUtil::SetComponent8<Components8Bit::kColorMaskA>(GL_TRUE, ret.m_uValue);
		}
		break;
	case EffectConverter::Util::RenderStateType::kColorWriteEnable2:
		ret.m_uState = (UInt32)RenderState::kColorWriteEnable2;
		ret.m_uValue = 0u;
		if ((0u != (D3DCOLORWRITEENABLE_RED & state.m_uValue)))
		{
			RenderStateUtil::SetComponent8<Components8Bit::kColorMaskR>(GL_TRUE, ret.m_uValue);
		}
		if ((0u != (D3DCOLORWRITEENABLE_GREEN & state.m_uValue)))
		{
			RenderStateUtil::SetComponent8<Components8Bit::kColorMaskG>(GL_TRUE, ret.m_uValue);
		}
		if ((0u != (D3DCOLORWRITEENABLE_BLUE & state.m_uValue)))
		{
			RenderStateUtil::SetComponent8<Components8Bit::kColorMaskB>(GL_TRUE, ret.m_uValue);
		}
		if ((0u != (D3DCOLORWRITEENABLE_ALPHA & state.m_uValue)))
		{
			RenderStateUtil::SetComponent8<Components8Bit::kColorMaskA>(GL_TRUE, ret.m_uValue);
		}
		break;
	case EffectConverter::Util::RenderStateType::kColorWriteEnable3:
		ret.m_uState = (UInt32)RenderState::kColorWriteEnable3;
		ret.m_uValue = 0u;
		if ((0u != (D3DCOLORWRITEENABLE_RED & state.m_uValue)))
		{
			RenderStateUtil::SetComponent8<Components8Bit::kColorMaskR>(GL_TRUE, ret.m_uValue);
		}
		if ((0u != (D3DCOLORWRITEENABLE_GREEN & state.m_uValue)))
		{
			RenderStateUtil::SetComponent8<Components8Bit::kColorMaskG>(GL_TRUE, ret.m_uValue);
		}
		if ((0u != (D3DCOLORWRITEENABLE_BLUE & state.m_uValue)))
		{
			RenderStateUtil::SetComponent8<Components8Bit::kColorMaskB>(GL_TRUE, ret.m_uValue);
		}
		if ((0u != (D3DCOLORWRITEENABLE_ALPHA & state.m_uValue)))
		{
			RenderStateUtil::SetComponent8<Components8Bit::kColorMaskA>(GL_TRUE, ret.m_uValue);
		}
		break;
	case EffectConverter::Util::RenderStateType::kCullMode:
		ret.m_uState = (UInt32)RenderState::kCull;
		ret.m_uValue = ConvertToCullMode(state.m_uValue);
		break;
	case EffectConverter::Util::RenderStateType::kDepthBias:
		ret.m_uState = (UInt32)RenderState::kDepthBias;
		ret.m_uValue = state.m_uValue;
		break;
	case EffectConverter::Util::RenderStateType::kZEnable:
		ret.m_uState = (UInt32)RenderState::kDepthEnable;
		ret.m_uValue = ConvertToBooleanValue(state.m_uValue);
		break;
	case EffectConverter::Util::RenderStateType::kZFunc:
		ret.m_uState = (UInt32)RenderState::kDepthFunction;
		ret.m_uValue = ConvertToCompareFunction(state.m_uValue);
		break;
	case EffectConverter::Util::RenderStateType::kZWriteEnable:
		ret.m_uState = (UInt32)RenderState::kDepthWriteEnable;
		ret.m_uValue = ConvertToBooleanValue(state.m_uValue);
		break;
	case EffectConverter::Util::RenderStateType::kDestBlend:
		ret.m_uState = (UInt32)RenderState::kDestinationBlend;
		ret.m_uValue = ConvertToBlend(state.m_uValue);
		break;
	case EffectConverter::Util::RenderStateType::kDestBlendAlpha:
		ret.m_uState = (UInt32)RenderState::kDestinationBlendAlpha;
		ret.m_uValue = ConvertToBlend(state.m_uValue);
		break;
	case EffectConverter::Util::RenderStateType::kFillMode:
		ret.m_uState = (UInt32)RenderState::kFillMode;
		ret.m_uValue = ConvertToFillMode(state.m_uValue);
		break;
	case EffectConverter::Util::RenderStateType::kSeparateAlphaBlendEnable:
		ret.m_uState = (UInt32)RenderState::kSeparateAlphaBlendEnable;
		ret.m_uValue = ConvertToBooleanValue(state.m_uValue);
		break;
	case EffectConverter::Util::RenderStateType::kShadeMode:
		ret.m_uState = (UInt32)RenderState::kShadeMode;
		ret.m_uValue = ConvertToShadeMode(state.m_uValue);
		break;
	case EffectConverter::Util::RenderStateType::kSlopeScaleDepthBias:
		ret.m_uState = (UInt32)RenderState::kSlopeScaleDepthBias;
		ret.m_uValue = state.m_uValue;
		break;
	case EffectConverter::Util::RenderStateType::kSrcBlend:
		ret.m_uState = (UInt32)RenderState::kSourceBlend;
		ret.m_uValue = ConvertToBlend(state.m_uValue);
		break;
	case EffectConverter::Util::RenderStateType::kSrcBlendAlpha:
		ret.m_uState = (UInt32)RenderState::kSourceBlendAlpha;
		ret.m_uValue = ConvertToBlend(state.m_uValue);
		break;
	case EffectConverter::Util::RenderStateType::kSrgbWriteEnable:
		ret.m_uState = (UInt32)RenderState::kSRGBWriteEnable;
		ret.m_uValue = ConvertToBooleanValue(state.m_uValue);
		break;
	case EffectConverter::Util::RenderStateType::kStencilZFail:
		ret.m_uState = (UInt32)RenderState::kStencilDepthFail;
		ret.m_uValue = ConvertToStencilOp(state.m_uValue);
		break;
	case EffectConverter::Util::RenderStateType::kStencilEnable:
		ret.m_uState = (UInt32)RenderState::kStencilEnable;
		ret.m_uValue = ConvertToBooleanValue(state.m_uValue);
		break;
	case EffectConverter::Util::RenderStateType::kStencilFail:
		ret.m_uState = (UInt32)RenderState::kStencilFail;
		ret.m_uValue = ConvertToStencilOp(state.m_uValue);
		break;
	case EffectConverter::Util::RenderStateType::kStencilFunc:
		ret.m_uState = (UInt32)RenderState::kStencilFunction;
		ret.m_uValue = ConvertToCompareFunction(state.m_uValue);
		break;
	case EffectConverter::Util::RenderStateType::kStencilMask:
		ret.m_uState = (UInt32)RenderState::kStencilMask;
		ret.m_uValue = state.m_uValue;
		break;
	case EffectConverter::Util::RenderStateType::kStencilPass:
		ret.m_uState = (UInt32)RenderState::kStencilPass;
		ret.m_uValue = ConvertToStencilOp(state.m_uValue);
		break;
	case EffectConverter::Util::RenderStateType::kStencilRef:
		ret.m_uState = (UInt32)RenderState::kStencilReference;
		ret.m_uValue = state.m_uValue;
		break;
	case EffectConverter::Util::RenderStateType::kStencilWriteMask:
		ret.m_uState = (UInt32)RenderState::kStencilWriteMask;
		ret.m_uValue = state.m_uValue;
		break;
	case EffectConverter::Util::RenderStateType::kTwoSidedStencilMode:
		ret.m_uState = (UInt32)RenderState::kTwoSidedStencilMode;
		ret.m_uValue = ConvertToBooleanValue(state.m_uValue);
		break;

	default:
		break;
	};

	return ret;
}

} // namespace Seoul

#endif // include guard
