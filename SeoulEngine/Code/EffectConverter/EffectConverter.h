/**
 * \file EffectConverter.h
 * \brief Parse a D3D9 Effect bytecode blob, including a subset of Effect
 * metadata (parameters, passes, render states, and techniques) as well
 * as the opcodes in pixel and vertex shaders.
 *
 * \sa https://msdn.microsoft.com/en-us/library/ff552891%28VS.85%29.aspx
 * \sa https://github.com/James-Jones/HLSLCrossCompiler
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EFFECT_CONVERTER_H
#define EFFECT_CONVERTER_H

#include "Prereqs.h"
#include "IShaderReceiver.h"
#include "SeoulHString.h"
#include "StaticAssert.h"
#include "Vector.h"

namespace Seoul
{

namespace EffectConverter { class IEffectReceiver; }
namespace EffectConverter { class IShaderReceiver; }
class StreamBuffer;

// NOTE: Most of the enums in EffectConverter::Util match the equivalent
// enums defined in the D3D9 headers. They are reproduced because:
// - some values stored in the Effect bytecode *do not* match the
//   D3D9 header enums. The most noteworthy is RenderStateType -
//   RenderStateType in Effect bytecode is compact (starts at 0 with
//   no holes) and the Wrap* enums have, for whatever reason, but
//   put contiguously together (in the D3D9 header enum, they are
//   split into a group of Wrap0-Wrap7, and a second group of
//   Wrap8-Wrap15.
// - we may, eventually, decide to make our effect system platform
//   agnostic and reuse these enums across all platforms.

namespace EffectConverter
{

namespace Util
{

enum class ParameterClass
{
	kScalar,
	kVector,
	kMatrixRows,
	kMatrixColumns,
	kObject,
	kStruct,
};

inline static Bool HasColumnsAndRows(ParameterClass eClass)
{
	switch (eClass)
	{
	case ParameterClass::kScalar: return true;
	case ParameterClass::kVector: return true;
	case ParameterClass::kMatrixRows: return true;
	case ParameterClass::kMatrixColumns: return true;
	case ParameterClass::kObject: // fall-through
	case ParameterClass::kStruct: // fall-through
	default:
		return false;
	};
}

enum class ParameterType
{
	kVoid,
	kBool,
	kInt,
	kFloat,
	kString,
	kTexture,
	kTexture1D,
	kTexture2D,
	kTexture3D,
	kTextureCube,
	kSampler,
	kSampler1D,
	kSampler2D,
	kSampler3D,
	kSamplerCube,
	kPixelShader,
	kVertexShader,
	kPixelFragment,
	kVertexFragment,
	kUnsupported,
};

enum class RenderStateType
{
	kZEnable,
	kFillMode,
	kShadeMode,
	kZWriteEnable,
	kAlphaTestEnable,
	kLastPixel,
	kSrcBlend,
	kDestBlend,
	kCullMode,
	kZFunc,
	kAlphaRef,
	kAlphaFunc,
	kDitherEnable,
	kAlphaBlendEnable,
	kFogEnable,
	kSpecularEnable,
	kFogColor,
	kFogTableMode,
	kFogStart,
	kFogEnd,
	kFogDensity,
	kRangeFogEnable,
	kStencilEnable,
	kStencilFail,
	kStencilZFail,
	kStencilPass,
	kStencilFunc,
	kStencilRef,
	kStencilMask,
	kStencilWriteMask,
	kTextureFactor,
	kWrap0,
	kWrap1,
	kWrap2,
	kWrap3,
	kWrap4,
	kWrap5,
	kWrap6,
	kWrap7,
	kWrap8,
	kWrap9,
	kWrap10,
	kWrap11,
	kWrap12,
	kWrap13,
	kWrap14,
	kWrap15,
	kClipping,
	kLighting,
	kAmbient,
	kFogVertexMode,
	kColorVertex,
	kLocalViewer,
	kNormalizeNormals,
	kDiffuseMaterialSource,
	kSpecularMaterialSource,
	kAmbientMaterialSource,
	kEmissiveMaterialSource,
	kVertexBlend,
	kClipPlaneEnable,
	kPointSize,
	kPointSizeMin,
	kPointSpriteEnable,
	kPointScaleEnable,
	kPointScaleA,
	kPointScaleB,
	kPointScaleC,
	kMultisampleAntiAlias,
	kMultisampleMask,
	kPatchEdgeStyle,
	kDebugMonitorToken,
	kPointSizeMax,
	kIndexedVertexBlendEnable,
	kColorWriteEnable,
	kTweenFactor,
	kBlendOp,
	kPositionDegree,
	kNormalDegree,
	kScissorTestEnable,
	kSlopeScaleDepthBias,
	kAntiAliasedLineEnable,
	kMinTessellationLevel,
	kMaxTessellationLevel,
	kAdaptiveTessX,
	kAdaptiveTessY,
	kAdaptiveTessZ,
	kAdaptiveTessW,
	kEnableAdaptiveTessellation,
	kTwoSidedStencilMode,
	kCcwStencilFail,
	kCcwStencilZFail,
	kCcwStencilPass,
	kCcwStencilFunc,
	kColorWriteEnable1,
	kColorWriteEnable2,
	kColorWriteEnable3,
	kBlendFactor,
	kSrgbWriteEnable,
	kDepthBias,
	kSeparateAlphaBlendEnable,
	kSrcBlendAlpha,
	kDestBlendAlpha,
	kBlendOpAlpha,
};

struct Parameter
{
	Parameter()
		: m_eType((ParameterType)0)
		, m_eClass((ParameterClass)0)
		, m_Name()
		, m_Semantic()
		, m_uElements(0u)
		, m_uRows(0u)
		, m_uColumns(0u)
		, m_zSizeInBytes(0u)
		, m_pDefaultValue(nullptr)
		, m_bInUse(false)
	{
	}

	ParameterType m_eType;
	ParameterClass m_eClass;
	HString m_Name;
	HString m_Semantic;
	UInt32 m_uElements;
	UInt32 m_uRows;
	UInt32 m_uColumns;
	UInt32 m_zSizeInBytes;
	void* m_pDefaultValue;
	Bool m_bInUse;

	Bool GetDefaultValue(void* pOut, UInt32 zOutSizeInBytes) const
	{
		// No default value.
		if (nullptr == m_pDefaultValue)
		{
			return false;
		}

		UInt32 const zSizeInBytes = GetSizeInBytes();
		if (nullptr == pOut ||
			zOutSizeInBytes < zSizeInBytes)
		{
			return false;
		}

		memcpy(pOut, m_pDefaultValue, zSizeInBytes);
		return true;
	}

	UInt32 GetSizeInBytes() const
	{
		return m_zSizeInBytes;
	}
}; // struct Parameter
typedef Vector<Parameter> Parameters;

struct Shader
{
	Shader()
		: m_vShaderCode()
		, m_eType(ShaderType::kPixel)
		, m_uTechniqueIndex(0u)
		, m_uPassIndex(0u)
	{
	}

	Bool Convert(IShaderReceiver& rReceiver) const;

	Vector<Byte> m_vShaderCode;
	ShaderType m_eType;
	UInt32 m_uTechniqueIndex;
	UInt32 m_uPassIndex;

private:
	static Bool InternalReadConstantTable(StreamBuffer& stream, Constants& rvConstants);
};
typedef Vector<Shader> Shaders;

struct RenderState
{
	RenderState()
		: m_eState((RenderStateType)0)
		, m_uValue(0u)
	{
	}

	RenderStateType m_eState;
	UInt32 m_uValue;
};
typedef Vector<RenderState> RenderStates;

struct Pass
{
	Shaders m_vShaders;
	RenderStates m_vRenderStates;
	HString m_Name;
};
typedef Vector<Pass> Passes;

struct Texture
{
	Texture()
		: m_uParameter(0u)
		, m_Name()
	{
	}

	UInt32 m_uParameter;
	HString m_Name;
};
typedef Vector<Texture> Textures;

struct Technique
{
	Passes m_vPasses;
	HString m_Name;
};
typedef Vector<Technique> Techniques;

} // namespace EffectConverter::Util

class Converter SEOUL_SEALED
{
public:
	Converter();
	~Converter();

	const Util::Parameters& GetParameters() const
	{
		return m_vParameters;
	}

	const Util::Techniques& GetTechniques() const
	{
		return m_vTechniques;
	}

	Bool ConvertTo(IEffectReceiver& rReceiver) const;

	Bool ProcessBytecode(UInt8 const* pByteCode, UInt32 zByteCodeSizeInBytes);

private:
	class ParameterFinalizeReceiver SEOUL_SEALED : public IShaderReceiver
	{
	public:
		ParameterFinalizeReceiver(Converter& rConverter);
		~ParameterFinalizeReceiver();

		virtual Bool TokenBeginShader(UInt32 uMajorVersion, UInt32 uMinorVersion, ShaderType eType) SEOUL_OVERRIDE;
		virtual Bool TokenComment(Byte const* sComment, UInt32 nStringLengthInBytes) SEOUL_OVERRIDE;
		virtual Bool TokenConstantTable(const Constants& vConstants) SEOUL_OVERRIDE;
		virtual Bool TokenDclInstruction(const DestinationRegister& destination, DclToken dclToken) SEOUL_OVERRIDE;
		virtual Bool TokenDefInstruction(const DestinationRegister& destination, const Vector4D& v) SEOUL_OVERRIDE;
		virtual Bool TokenInstruction(
			OpCode eOpCode,
			const DestinationRegister& destination,
			const SourceRegister& sourceA,
			const SourceRegister& sourceB = SourceRegister(),
			const SourceRegister& sourceC = SourceRegister(),
			const SourceRegister& sourceD = SourceRegister()) SEOUL_OVERRIDE;
		virtual Bool TokenEndShader() SEOUL_OVERRIDE;

	private:
		Converter& m_rConverter;
	}; // class ParameterFinalizeReceiver

	void InternalDestroyParameters();
	Bool InternalProcess(UInt8 const* pByteCode, UInt32 zByteCodeSizeInBytes);
	Bool InternalReadAnnotations(StreamBuffer& stream, UInt32 uBaseOffset, UInt32 nAnnotations);
	Bool InternalReadHString(StreamBuffer& stream, UInt32 uBaseOffset, UInt32 uHStringOffset, HString& outValue);
	Bool InternalReadParameters(StreamBuffer& stream, UInt32 uBaseOffset, UInt32 nParameters);
	Bool InternalReadTechniques(StreamBuffer& stream, UInt32 uBaseOffset, UInt32 nTechniques);
	Bool InternalReadPasses(StreamBuffer& stream, Util::Technique& technique, UInt32 uBaseOffset, UInt32 nPasses);
	Bool InternalReadRenderStates(StreamBuffer& stream, Util::Pass& pass, UInt32 uBaseOffset, UInt32 uRenderStates);
	Bool InternalReadShader(StreamBuffer& stream, Util::Shader& shader, UInt32 uBaseOffset);
	Bool InternalReadSmallObjects(StreamBuffer& stream, UInt32 uBaseOffset, UInt32 uSmallObjects);

	UInt32 m_nShaders;
	Util::Parameters m_vParameters;
	Util::Techniques m_vTechniques;

	SEOUL_DISABLE_COPY(Converter);
}; // class Converter

} // namespace EffectConverter

} // namespace Seoul

#endif // include guard
