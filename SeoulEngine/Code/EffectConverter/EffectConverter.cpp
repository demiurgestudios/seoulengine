/**
 * \file EffectConverter.cpp
 * \brief Parse a D3D9 Effect bytecode blob, including a subset of Effect
 * metadata (parameters, passes, render states, and techniques) as well
 * as the opcodes in pixel and vertex shaders.
 *
 * \sa https://msdn.microsoft.com/en-us/library/ff552891%28VS.85%29.aspx
 * \sa https://github.com/James-Jones/HLSLCrossCompiler
 * \sa https://www.virtualbox.org/svn/vbox/trunk/src/VBox/Devices/Graphics/shaderlib/glsl_shader.c
 * \sa https://github.com/tgjones/slimshader-cpp
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EffectConverter.h"
#include "HashFunctions.h"
#include "IEffectReceiver.h"
#include "IShaderReceiver.h"
#include "StreamBuffer.h"

namespace Seoul::EffectConverter
{

using namespace Util;

static const UInt32 kEffectMagicNumber = 0xFEFF0901;

static const UInt32 kPixelShaderType = 0x92;
static const UInt32 kPixelShaderVersionCode = 0xFFFF;
static const UInt32 kVertexShaderType = 0x93;
static const UInt32 kVertexShaderVersionCode = 0xFFFE;

static const UInt32 kCTabInfoSize = 20u;
static const UInt32 kCTabMagicNumber = 0x42415443; // 'CTAB'
static const UInt32 kCTabSize = 28u;
static const UInt32 kPreshaderMagicNumber = 0x53455250; // 'PRES'

struct CommentToken
{
	CommentToken(UInt32 uValue) : m_uRawValue(uValue) {}

	UInt32 m_uRawValue;

	UInt32 GetSizeInBytes() const
	{
		return sizeof(UInt32) * ((m_uRawValue & 0xFFFF0000) >> 16);
	}
}; // struct CommentToken

class ScopedOffset SEOUL_SEALED
{
public:
	ScopedOffset(StreamBuffer& stream)
		: m_rStream(stream)
		, m_nOffset(m_rStream.GetOffset())
	{
	}

	~ScopedOffset()
	{
		m_rStream.SeekToOffset(m_nOffset);
	}

private:
	StreamBuffer& m_rStream;
	StreamBuffer::SizeType m_nOffset;
}; // class ScopedOffset

namespace TokenType
{
	static const UInt16 kComment = 0xFFFE;
	static const UInt16 kEnd = 0xFFFF;

	inline UInt16 Get(UInt32 uToken)
	{
		return (UInt16)(uToken & 0x0000FFFF);
	}
} // namespace tokenType

struct VersionToken
{
	UInt32 m_uRawValue;

	UInt32 GetMinor() const
	{
		return (m_uRawValue & 0x000000FF);
	}

	UInt32 GetMajor() const
	{
		return (m_uRawValue & 0x0000FF00) >> 8;
	}

	UInt32 GetShaderType() const
	{
		return (m_uRawValue & 0xFFFF0000) >> 16;
	}
}; // struct VersionToken

static Bool InternalStaticParseInstructionToken(
	IShaderReceiver& rReceiver,
	UInt32 uToken,
	StreamBuffer& stream)
{
	OpCode const eOpCode = (OpCode)(TokenType::Get(uToken));
	// UInt32 const uControls = ((uToken >> 16u) & 0xFF);
	// UInt32 const uInstructionTokens = ((uToken >> 24u) & 0x0F);
	// UInt32 const bCoIssued = (uToken & 0x40000000) ? 1u : 0u;
	// UInt32 const bPredicated = (uToken & 0x10000000) ? 1u : 0u;

	if ((Int)eOpCode >= (Int)OpCode::BASIC_COUNT)
	{
		return false;
	}

	// Unsupported op codes.
	switch (eOpCode)
	{
	case OpCode::kBreakp:
	case OpCode::kCall:
	case OpCode::kCallnz:
	case OpCode::kElse:
	case OpCode::kEndif:
	case OpCode::kEndloop:
	case OpCode::kEndrep:
	case OpCode::kIf:
	case OpCode::kIfc:
	case OpCode::kLabel:
	case OpCode::kLoop:
	case OpCode::kRep:
	case OpCode::kSetp:
	case OpCode::kTexbem:
	case OpCode::kTexbeml:
	case OpCode::kTexcrd:
	case OpCode::kTexdepth:
	case OpCode::kTexdp3:
	case OpCode::kTexdp3tex:
	case OpCode::kTexldd:
	case OpCode::kTexldl:
	case OpCode::kTexm3x2depth:
	case OpCode::kTexm3x2pad:
	case OpCode::kTexm3x3spec:
	case OpCode::kTexm3x3tex:
	case OpCode::kTexm3x3vspec:
	case OpCode::kTexreg2ar:
	case OpCode::kTexreg2gb:
	case OpCode::kTexreg2rgb:

	case OpCode::kSpecialPhase:
		fprintf(stderr, "Unsupported op code %d\n", (Int)eOpCode);
		return false;

	default:
		break;
	};

	if (OpCode::kDcl == eOpCode)
	{
		DclToken dclToken;
		if (!stream.Read(dclToken)) { return false; }

		DestinationRegister destination;
		if (!destination.Read(stream)) { return false; }

		return rReceiver.TokenDclInstruction(
			destination,
			dclToken);
	}
	else if (OpCode::kDef == eOpCode)
	{
		DestinationRegister destination;
		if (!destination.Read(stream)) { return false; }

		Vector4D v;
		if (!stream.Read(v.GetData(), sizeof(v))) { return false; }

		return rReceiver.TokenDefInstruction(
			destination,
			v);
	}

	UInt32 uDestination = 0u;
	UInt32 uSource = 0u;
	if (!GetRegisterCounts(eOpCode, uDestination, uSource))
	{
		fprintf(stderr, "Failed getting register counts for op %d\n", (Int)eOpCode);
		return false;
	}

	// No instructions should have more than 1 output.
	if (uDestination > 1u)
	{
		fprintf(stderr, "Invalid destination register count %u for op %d\n", uDestination, (Int)eOpCode);
		return false;
	}

	// No instructions should have more than 4 inputs.
	if (uSource > 4u)
	{
		fprintf(stderr, "Invalid source register count %u for op %d\n", uSource, (Int)eOpCode);
		return false;
	}

	// Populate destination and source registers.
	DestinationRegister destination;
	if (uDestination > 0u) { if (!destination.Read(stream)) { return false; } }

	SourceRegister sourceA;
	if (uSource > 0u) { if (!sourceA.Read(stream)) { return false; } }

	SourceRegister sourceB;
	if (uSource > 1u) { if (!sourceB.Read(stream)) { return false; } }

	SourceRegister sourceC;
	if (uSource > 2u) { if (!sourceC.Read(stream)) { return false; } }

	SourceRegister sourceD;
	if (uSource > 3u) { if (!sourceD.Read(stream)) { return false; } }

	return rReceiver.TokenInstruction(
		eOpCode,
		destination,
		sourceA,
		sourceB,
		sourceC,
		sourceD);
}

Bool Util::Shader::Convert(IShaderReceiver& rReceiver) const
{
	const Vector<Byte>& vCode = m_vShaderCode;
	StreamBuffer stream(vCode.GetSize());
	stream.Write(vCode.Get(0u), vCode.GetSize());
	stream.SeekToOffset(0u);

	VersionToken versionToken;
	if (!stream.Read(versionToken)) { return false; }

	// Converter currently supports only pixel shader and vertex shader 3.0.
	if (versionToken.GetMajor() != 3 ||
		versionToken.GetMinor() != 0)
	{
		fprintf(stderr, "Unsupported shader version %u.%u.", versionToken.GetMajor(), versionToken.GetMinor());
		return false;
	}

	// Converter currently supports only vertex and pixel shaders.
	if (versionToken.GetShaderType() != kPixelShaderVersionCode &&
		versionToken.GetShaderType() != kVertexShaderVersionCode)
	{
		fprintf(stderr, "Unsupported shader type %u.", versionToken.GetShaderType());
		return false;
	}

	if (!rReceiver.TokenBeginShader(
		versionToken.GetMajor(),
		versionToken.GetMinor(),
		(versionToken.GetShaderType() == kPixelShaderVersionCode ? ShaderType::kPixel : ShaderType::kVertex)))
	{
		return false;
	}

	UInt32 uToken = 0u;
	while (stream.Read(uToken) && (TokenType::kEnd != TokenType::Get(uToken)))
	{
		if (TokenType::Get(uToken) == TokenType::kComment)
		{
			CommentToken token(uToken);

			UInt32 uFirstCommentToken = 0u;
			if (!stream.Read(uFirstCommentToken)) { return false; }
			stream.SeekToOffset(stream.GetOffset() - sizeof(uFirstCommentToken));
			UInt32 const uEndOffset = (stream.GetOffset() + token.GetSizeInBytes());

			switch (uFirstCommentToken)
			{
			case kCTabMagicNumber:
				{
					Constants vConstants;
					if (!InternalReadConstantTable(stream, vConstants)) { return false; }
					if (!rReceiver.TokenConstantTable(vConstants)) { return false; }
				}
				break;
			case kPreshaderMagicNumber:
				// TODO: Add support for preshaders.
				break;
			default:
				if (!rReceiver.TokenComment((Byte const*)(stream.GetBuffer() + stream.GetOffset()), token.GetSizeInBytes())) { return false; }
				break;
			};

			stream.SeekToOffset(uEndOffset);
		}
		else if (!InternalStaticParseInstructionToken(rReceiver, uToken, stream))
		{
			return false;
		}
	}

	if (TokenType::kEnd == TokenType::Get(uToken))
	{
		if (!rReceiver.TokenEndShader())
		{
			return false;
		}
	}
	else
	{
		return false;
	}

	return true;
}

Bool Util::Shader::InternalReadConstantTable(
	StreamBuffer& stream,
	Constants& rvConstants)
{
	UInt32 uCtabMagicNumber = 0u;
	if (!stream.Read(uCtabMagicNumber)) { return false; }
	if (uCtabMagicNumber != kCTabMagicNumber) { return false; }

	UInt32 const uStartingOffset = stream.GetOffset();

	UInt32 nSize = 0u;
	if (!stream.Read(nSize)) { return false; }
	if (nSize != kCTabSize) { return false; }

	UInt32 uCreator = 0u;
	if (!stream.Read(uCreator)) { return false; }

	UInt32 uVersion = 0u;
	if (!stream.Read(uVersion)) { return false; }

	UInt32 nConstants = 0u;
	if (!stream.Read(nConstants)) { return false; }

	UInt32 uConstantsInfoOffset = 0u;
	if (!stream.Read(uConstantsInfoOffset)) { return false; }

	UInt32 uTarget = 0u;
	if (!stream.Read(uTarget)) { return false; }

	for (UInt32 i = 0u; i < nConstants; ++i)
	{
		ScopedOffset scope(stream);
		stream.SeekToOffset(uStartingOffset + uConstantsInfoOffset + (i * kCTabInfoSize));

		UInt32 uNameOffset = 0u;
		if (!stream.Read(uNameOffset)) { return false; }

		UInt16 uRegisterSet = 0u;
		if (!stream.Read(uRegisterSet)) { return false; }

		UInt16 uRegisterIndex = 0u;
		if (!stream.Read(uRegisterIndex)) { return false; }

		UInt32 nRegisterCount = 0u;
		if (!stream.Read(nRegisterCount)) { return false; }

		UInt32 uTypeOffset = 0u;
		if (!stream.Read(uTypeOffset)) { return false; }

		UInt32 uDefaultValueOffset = 0u;
		if (!stream.Read(uDefaultValueOffset)) { return false; }

		UInt16 uParameterClass = 0u;
		ParameterType eParameterType = ParameterType::kUnsupported;
		UInt16 uParameterRows = 0u;
		UInt16 uParameterCols = 0u;
		UInt16 uParameterElements = 0u;

		Constant constant;
		constant.m_uRegisterNumber = uRegisterIndex;
		constant.m_uRegisterCount = nRegisterCount;

		{
			ScopedOffset nameScope(stream);
			stream.SeekToOffset(uStartingOffset + uNameOffset);

			constant.m_Name = HString((Byte const*)(stream.GetBuffer() + stream.GetOffset()));
		}

		{
			ScopedOffset typeScope(stream);
			stream.SeekToOffset(uStartingOffset + uTypeOffset);

			if (!stream.Read(uParameterClass)) { return false; }

			UInt16 uParameterType = 0u;
			if (!stream.Read(uParameterType)) { return false; }
			eParameterType = (ParameterType)uParameterType;

			if (!stream.Read(uParameterRows)) { return false; }
			if (!stream.Read(uParameterCols)) { return false; }
			if (!stream.Read(uParameterElements)) { return false; }

			constant.m_uColsCount = uParameterCols;
			constant.m_uElementsCount = uParameterElements;
			constant.m_uRowCount = uParameterRows;
		}

		switch (uRegisterSet)
		{
		case 0: constant.m_eType = ConstantType::kBool4; break;
		case 1: constant.m_eType = ConstantType::kInt4; break;
		case 2: constant.m_eType = ConstantType::kFloat4; break;
		default:
			{
				switch (eParameterType)
				{
				case ParameterType::kSampler1D: // fall-through
				case ParameterType::kSampler2D:
					constant.m_eType = ConstantType::kSampler2D;
					break;
				case ParameterType::kSampler3D:
					constant.m_eType = ConstantType::kSampler3D;
					break;
				case ParameterType::kSamplerCube:
					constant.m_eType = ConstantType::kSamplerCube;
					break;
				default:
					return false;
				};
			}
		};

		rvConstants.PushBack(constant);
	}

	return true;
}

Converter::Converter()
	: m_nShaders(0u)
	, m_vParameters()
	, m_vTechniques()
{
}

Converter::~Converter()
{
	InternalDestroyParameters();
}

Bool Converter::ConvertTo(IEffectReceiver& rReceiver) const
{
	// Pass parameters to the receiver.
	UInt32 const uParameters = m_vParameters.GetSize();
	for (UInt32 i = 0u; i < uParameters; ++i)
	{
		if (!rReceiver.AddParameter(*this, m_vParameters[i]))
		{
			return false;
		}
	}

	// Pass techniques to the receiver.
	UInt32 const uTechniques = m_vTechniques.GetSize();
	for (UInt32 i = 0u; i < uTechniques; ++i)
	{
		if (!rReceiver.AddTechnique(*this, m_vTechniques[i]))
		{
			return false;
		}
	}

	return true;
}

Bool Converter::ProcessBytecode(UInt8 const* pByteCode, UInt32 zByteCodeSizeInBytes)
{
	m_vTechniques.Clear();
	InternalDestroyParameters();
	m_nShaders = 0u;

	return InternalProcess(pByteCode, zByteCodeSizeInBytes);
}

void Converter::InternalDestroyParameters()
{
	Int32 const nCount = (Int32)m_vParameters.GetSize();
	for (Int32 i = (nCount - 1); i >= 0; --i)
	{
		Parameter& rParameter = m_vParameters[i];
		MemoryManager::Deallocate(rParameter.m_pDefaultValue);
		rParameter.m_pDefaultValue = nullptr;
	}
	m_vParameters.Clear();
}

Bool Converter::InternalProcess(UInt8 const* pByteCode, UInt32 zByteCodeSizeInBytes)
{
	StreamBuffer stream(zByteCodeSizeInBytes);
	stream.Write(pByteCode, zByteCodeSizeInBytes);
	stream.SeekToOffset(0u);

	{
		UInt32 uMagicValue = 0u;
		if (!stream.Read(uMagicValue) || (kEffectMagicNumber != uMagicValue)) { return false; }
	}

	UInt32 uBaseOffset = 0u;
	{
		UInt32 uOffset = 0u;
		if (!stream.Read(uOffset) || uOffset > stream.GetTotalDataSizeInBytes()) { return false; }
		uBaseOffset = stream.GetOffset();
		stream.SeekToOffset(uBaseOffset + uOffset);
	}

	UInt32 nParameters = 0u;
	if (!stream.Read(nParameters)) { return false; }

	UInt32 nTechniques = 0u;
	if (!stream.Read(nTechniques)) { return false; }

	UInt32 uUnknown0 = 0u;
	if (!stream.Read(uUnknown0)) { return false; }

	UInt32 uUnknown1 = 0u;
	if (!stream.Read(uUnknown1)) { return false; }

	if (!InternalReadParameters(stream, uBaseOffset, nParameters)) { return false; }

	if (!InternalReadTechniques(stream, uBaseOffset, nTechniques)) { return false; }

	UInt32 uSmallObjects = 0u;
	if (!stream.Read(uSmallObjects)) { return false; }

	UInt32 uUnknown2 = 0u;
	if (!stream.Read(uUnknown2)) { return false; }

	if (!InternalReadSmallObjects(stream, uBaseOffset, uSmallObjects)) { return false; }

	for (UInt32 i = 0u; i < m_nShaders; ++i)
	{
		Shader shader;
		if (!InternalReadShader(stream, shader, uBaseOffset)) { return false; }

		if (shader.m_uTechniqueIndex >= m_vTechniques.GetSize() ||
			shader.m_uPassIndex >= m_vTechniques[shader.m_uTechniqueIndex].m_vPasses.GetSize())
		{
			return false;
		}

		m_vTechniques[shader.m_uTechniqueIndex].m_vPasses[shader.m_uPassIndex].m_vShaders.PushBack(shader);

		// Final step, run shader Convert() to process bytecode and finalize:
		// - whether a parameter is in use or not.
		ParameterFinalizeReceiver receiver(*this);
		if (!shader.Convert(receiver)) { return false; }
	}

	return true;
}

Bool Converter::InternalReadHString(
	StreamBuffer& stream,
	UInt32 uBaseOffset,
	UInt32 uHStringOffset,
	HString& outValue)
{
	ScopedOffset scope(stream);
	stream.SeekToOffset(uBaseOffset + uHStringOffset);

	String sString;
	if (!stream.Read(sString)) { return false; }

	outValue = HString(sString);

	return true;
}

Bool Converter::InternalReadAnnotations(StreamBuffer& stream, UInt32 uBaseOffset, UInt32 nAnnotations)
{
	for (UInt32 i = 0u; i < nAnnotations; ++i)
	{
		UInt32 uUnusedTypeOffset = 0u;
		if (!stream.Read(uUnusedTypeOffset)) { return false; }

		UInt32 uUnusedValueOffset = 0u;
		if (!stream.Read(uUnusedValueOffset)) { return false; }
	}

	return true;
}

Bool Converter::InternalReadParameters(StreamBuffer& stream, UInt32 uBaseOffset, UInt32 nParameters)
{
	for (UInt32 i = 0u; i < nParameters; ++i)
	{
		UInt32 uTypeOffset = 0u;
		if (!stream.Read(uTypeOffset)) { return false; }

		UInt32 uValueOffset = 0u;
		if (!stream.Read(uValueOffset)) { return false; }

		UInt32 uFlags = 0u;
		if (!stream.Read(uFlags)) { return false; }

		UInt32 nAnnotations = 0u;
		if (!stream.Read(nAnnotations)) { return false; }
		if (!InternalReadAnnotations(stream, uBaseOffset, nAnnotations)) { return false; }

		Parameter parameter;

		// Setup type/description info of the parameter.
		{
			ScopedOffset parameterScope(stream);
			stream.SeekToOffset(uBaseOffset + uTypeOffset);

			UInt32 uParameterType = 0u;
			if (!stream.Read(uParameterType)) { return false; }

			UInt32 uParameterClass = 0u;
			if (!stream.Read(uParameterClass)) { return false; }

			UInt32 uParameterNameOffset = 0u;
			if (!stream.Read(uParameterNameOffset)) { return false; }

			UInt32 uParameterSemanticOffset = 0u;
			if (!stream.Read(uParameterSemanticOffset)) { return false; }

			UInt32 uParameterElements = 0u;
			if (!stream.Read(uParameterElements)) { return false; }

			UInt32 uParameterColumns = 0u;
			UInt32 uParameterRows = 0u;
			if (HasColumnsAndRows((ParameterClass)uParameterClass))
			{
				if (!stream.Read(uParameterColumns)) { return false; }
				if (!stream.Read(uParameterRows)) { return false; }
			}

			parameter.m_eClass = (ParameterClass)uParameterClass;
			parameter.m_uColumns = uParameterColumns;
			parameter.m_uElements = uParameterElements;
			parameter.m_uRows = uParameterRows;
			parameter.m_eType = (ParameterType)uParameterType;
			if (!InternalReadHString(stream, uBaseOffset, uParameterNameOffset, parameter.m_Name)) { return false; }
			if (!InternalReadHString(stream, uBaseOffset, uParameterSemanticOffset, parameter.m_Semantic)) { return false; }
		}

		// Setup value information of the parameter.
		{
			ScopedOffset parameterScope(stream);
			stream.SeekToOffset(uBaseOffset + uValueOffset);

			// Change behavior based on value type.
			switch (parameter.m_eClass)
			{
			case ParameterClass::kMatrixColumns: // fall-through
			case ParameterClass::kMatrixRows: // fall-through
			case ParameterClass::kScalar: // fall-through
			case ParameterClass::kVector:
				{
					parameter.m_zSizeInBytes = (UInt32)(Max(parameter.m_uElements, 1u) * parameter.m_uRows * parameter.m_uColumns * 4);
					parameter.m_pDefaultValue = MemoryManager::Allocate(parameter.m_zSizeInBytes, MemoryBudgets::TBD);
					memcpy(parameter.m_pDefaultValue, stream.GetBuffer() + stream.GetOffset(), parameter.m_zSizeInBytes);
				}
				break;

			case ParameterClass::kObject:
				// Keep texture objects, sampler objects, and simple value types.
				// All other types are unsupported and are treated as an error.
				switch (parameter.m_eType)
				{
				case ParameterType::kTexture: // fall-through
				case ParameterType::kTexture1D: // fall-through
				case ParameterType::kTexture2D: // fall-through
				case ParameterType::kTexture3D: // fall-through
				case ParameterType::kTextureCube: // fall-through
					// "NULL" data, size of 4.
					parameter.m_zSizeInBytes = 4u;
					parameter.m_pDefaultValue = MemoryManager::Allocate(parameter.m_zSizeInBytes, MemoryBudgets::TBD);
					*((UInt32*)parameter.m_pDefaultValue) = 0u;
					break;

				case ParameterType::kSampler: // fall-through
				case ParameterType::kSampler1D: // fall-through
				case ParameterType::kSampler2D: // fall-through
				case ParameterType::kSampler3D: // fall-through
				case ParameterType::kSamplerCube: // fall-through
					// "NULL" data, size of 4.
					parameter.m_zSizeInBytes = 4u;
					parameter.m_pDefaultValue = MemoryManager::Allocate(parameter.m_zSizeInBytes, MemoryBudgets::TBD);
					*((UInt32*)parameter.m_pDefaultValue) = 0u;
					break;

					// All other types unsupported.
				case ParameterType::kPixelShader: // fall-through
				case ParameterType::kVertexShader: // fall-through
				case ParameterType::kPixelFragment: // fall-through
				case ParameterType::kVertexFragment: // fall-through
				case ParameterType::kUnsupported: // fall-through
				default:
					return false;
				};
				break;

				// Unsupported parameter types.
			case ParameterClass::kStruct: // fall-through
			default:
				return false;
			};
		}

		m_vParameters.PushBack(parameter);
	}

	return true;
}

Bool Converter::InternalReadTechniques(StreamBuffer& stream, UInt32 uBaseOffset, UInt32 nTechniques)
{
	for (UInt32 i = 0u; i < nTechniques; ++i)
	{
		UInt32 uNameOffset = 0u;
		if (!stream.Read(uNameOffset)) { return false; }

		UInt32 nAnnotations = 0u;
		if (!stream.Read(nAnnotations)) { return false; }

		UInt32 nPasses = 0u;
		if (!stream.Read(nPasses)) { return false; }

		if (!InternalReadAnnotations(stream, uBaseOffset, nAnnotations)) { return false; }

		Technique technique;
		if (!InternalReadHString(stream, uBaseOffset, uNameOffset, technique.m_Name)) { return false; }

		if (!InternalReadPasses(stream, technique, uBaseOffset, nPasses)) { return false; }

		m_vTechniques.PushBack(technique);
	}

	return true;
}

Bool Converter::InternalReadPasses(StreamBuffer& stream, Technique& technique, UInt32 uBaseOffset, UInt32 nPasses)
{
	for (UInt32 i = 0u; i < nPasses; ++i)
	{
		UInt32 uNameOffset = 0u;
		if (!stream.Read(uNameOffset)) { return false; }

		UInt32 nAnnotations = 0u;
		if (!stream.Read(nAnnotations)) { return false; }

		UInt32 uStates = 0u;
		if (!stream.Read(uStates)) { return false; }

		Pass pass;
		if (!InternalReadHString(stream, uBaseOffset, uNameOffset, pass.m_Name)) { return false; }

		if (!InternalReadAnnotations(stream, uBaseOffset, nAnnotations)) { return false; }

		if (!InternalReadRenderStates(stream, pass, uBaseOffset, uStates)) { return false; }

		technique.m_vPasses.PushBack(pass);
	}

	return true;
}

Bool Converter::InternalReadRenderStates(StreamBuffer& stream, Pass& pass, UInt32 uBaseOffset, UInt32 uRenderStates)
{
	for (UInt32 i = 0u; i < uRenderStates; ++i)
	{
		UInt32 uStateType = 0u;
		if (!stream.Read(uStateType)) { return false; }

		UInt32 uUnknown0 = 0u;
		if (!stream.Read(uUnknown0)) { return false; }

		UInt32 uEndOffset = 0u;
		if (!stream.Read(uEndOffset)) { return false; }

		UInt32 uStartOffset = 0u;
		if (!stream.Read(uStartOffset)) { return false; }

		if (uStateType == kPixelShaderType)
		{
			++m_nShaders;
		}
		else if (uStateType == kVertexShaderType)
		{
			++m_nShaders;
		}
		else
		{
			RenderState renderState;
			renderState.m_eState = (RenderStateType)uStateType;

			{
				ScopedOffset offset(stream);
				stream.SeekToOffset(uBaseOffset + uStartOffset);

				if (!stream.Read(renderState.m_uValue)) { return false; }
			}

			pass.m_vRenderStates.PushBack(renderState);
		}
	}

	return true;
}

Bool Converter::InternalReadShader(StreamBuffer& stream, Shader& shader, UInt32 uBaseOffset)
{
	if (!stream.Read(shader.m_uTechniqueIndex)) { return false; }

	if (!stream.Read(shader.m_uPassIndex)) { return false; }

	UInt32 uUnknown0 = 0u;
	if (!stream.Read(uUnknown0)) { return false; }

	UInt32 uUnknown1 = 0u;
	if (!stream.Read(uUnknown1)) { return false; }

	UInt32 uUnknown2 = 0u;
	if (!stream.Read(uUnknown2)) { return false; }

	UInt32 zShaderSizeInBytes = 0u;
	if (!stream.Read(zShaderSizeInBytes)) { return false; }
	if (zShaderSizeInBytes < sizeof(VersionToken)) { return false; }

	shader.m_vShaderCode.Resize(zShaderSizeInBytes);
	if (!stream.Read(shader.m_vShaderCode.Get(0u), zShaderSizeInBytes)) { return false; }

	VersionToken token;
	memcpy(&token, shader.m_vShaderCode.Get(0u), sizeof(token));
	shader.m_eType = (token.GetShaderType() == kPixelShaderVersionCode ? ShaderType::kPixel : ShaderType::kVertex);

	return true;
}

Bool Converter::InternalReadSmallObjects(StreamBuffer& stream, UInt32 uBaseOffset, UInt32 uSmallObjects)
{
	for (UInt32 i = 0u; i < uSmallObjects; ++i)
	{
		UInt32 uUnusedParameterIndex = 0u;
		if (!stream.Read(uUnusedParameterIndex)) { return false; }

		HString unusedStringValue;
		if (!InternalReadHString(stream, uBaseOffset, (stream.GetOffset() - uBaseOffset), unusedStringValue)) { return false; }

		UInt32 uAdditionalDataSize = 0u;
		if (!stream.Read(uAdditionalDataSize)) { return false; }
		stream.SeekToOffset(stream.GetOffset() + (UInt32)RoundUpToAlignment(uAdditionalDataSize, 4u));
	}

	return true;
}

Converter::ParameterFinalizeReceiver::ParameterFinalizeReceiver(Converter& rConverter)
	: m_rConverter(rConverter)
{
}

Converter::ParameterFinalizeReceiver::~ParameterFinalizeReceiver()
{
}

Bool Converter::ParameterFinalizeReceiver::TokenBeginShader(UInt32 uMajorVersion, UInt32 uMinorVersion, ShaderType eType)
{
	return true;
}

Bool Converter::ParameterFinalizeReceiver::TokenComment(Byte const* sComment, UInt32 nStringLengthInBytes)
{
	return true;
}

Bool Converter::ParameterFinalizeReceiver::TokenConstantTable(const Constants& vConstants)
{
	UInt32 const uParameters = m_rConverter.m_vParameters.GetSize();
	UInt32 const uConstants = vConstants.GetSize();
	for (UInt32 i = 0u; i < uConstants; ++i)
	{
		Constant const constant = vConstants[i];
		for (UInt32 j = 0u; j < uParameters; ++j)
		{
			Parameter& rParameter = m_rConverter.m_vParameters[j];

			// This is not the parameter we're looking for.
			if (rParameter.m_Name != constant.m_Name)
			{
				continue;
			}

			// Parameter is in use if a constant references it.
			rParameter.m_bInUse = true;
		}
	}

	return true;
}

Bool Converter::ParameterFinalizeReceiver::TokenDclInstruction(const DestinationRegister& destination, DclToken dclToken)
{
	return true;
}

Bool Converter::ParameterFinalizeReceiver::TokenDefInstruction(const DestinationRegister& destination, const Vector4D& v)
{
	return true;
}

Bool Converter::ParameterFinalizeReceiver::TokenInstruction(
	OpCode eOpCode,
	const DestinationRegister& destination,
	const SourceRegister& sourceA,
	const SourceRegister& sourceB /*= SourceRegister()*/,
	const SourceRegister& sourceC /*= SourceRegister()*/,
	const SourceRegister& sourceD /* = SourceRegister()*/)
{
	return true;
}

Bool Converter::ParameterFinalizeReceiver::TokenEndShader()
{
	return true;
}

} // namespace Seoul::EffectConverter
