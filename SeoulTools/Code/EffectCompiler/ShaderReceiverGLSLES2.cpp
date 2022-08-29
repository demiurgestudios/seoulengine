/**
 * \file ShaderReceiverGLSLES2.cpp
 * \brief Receiver that converts output from EffectConverter::Shader to
 * OpenGL GLSL ES 2.0 compatible source.
 *
 * \sa https://www.khronos.org/registry/gles/specs/2.0/GLSL_ES_Specification_1.0.17.pdf
 *     In particular, see section 4.5 for the exact requirements and behavior of precision
 *     specifiers in OpenGL GLSL ES 2.0.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

// TODO:
// - need to decide how/when/if to use the partial-precision bit on destination registers
//   to output lowp as appropriate, and when (if ever) we should output medp or highp explicitly.
// - need to factor in various destination register modifiers (in particular the 'saturate' bit).

#include "ShaderReceiverGLSLES2.h"
#include "ToString.h"

namespace Seoul
{

static const String ksGlslVersionString("#version 100");

Bool ShaderReceiverGLSLES2::CanSwizzle(RegisterType eRegisterType, UInt32 uRegisterNumber)
{
	if (RegisterType::kConst == eRegisterType)
	{
		return (m_aConstantRegisters[uRegisterNumber].m_uColsCount > 1u);
	}

	return true;
}

Bool ShaderReceiverGLSLES2::RegisterTypeToString(NameAndOffset& rOut, RegisterType eRegisterType, UInt32 uRegisterNumber)
{
	if (RegisterType::kConst == eRegisterType)
	{
		if (!m_aConstantRegisters[uRegisterNumber].m_NameAndOffset.IsDefined())
		{
			m_aConstantRegisters[uRegisterNumber].m_NameAndOffset.m_Name = HString(String::Printf("c%u", uRegisterNumber));
			m_aConstantRegisters[uRegisterNumber].m_uColsCount = 4u;
		}

		rOut = m_aConstantRegisters[uRegisterNumber].m_NameAndOffset;
		return true;
	}

	rOut.Reset();
	if (m_tRegisterTable.GetValue(RegisterEntry(eRegisterType, uRegisterNumber), rOut.m_Name))
	{
		return true;
	}

	switch (eRegisterType)
	{
	case RegisterType::kTemp:     rOut.m_Name = HString(String::Printf("r%u", uRegisterNumber)); break; // D3DSPR_TEM
	case RegisterType::kInput:    rOut.m_Name = HString(String::Printf("v%u", uRegisterNumber)); break; // D3DSPR_INPUT
	case RegisterType::kConst:    rOut.m_Name = HString(String::Printf("c%u", uRegisterNumber)); break; // D3DSPR_CONST
	case RegisterType::kAddress:  rOut.m_Name = HString(String::Printf("a%u", uRegisterNumber)); break; // D3DSPR_ADDR
	case RegisterType::kOutput:   rOut.m_Name = HString(String::Printf("o%u", uRegisterNumber)); break; // D3DSPR_TEXCRDOUT or D3DSPR_OUTPUT
	case RegisterType::kColorOut: rOut.m_Name = HString("gl_FragColor");                         break; // D3DSPR_COLOROUT
	case RegisterType::kSampler:  rOut.m_Name = HString(String::Printf("s%u", uRegisterNumber)); break; // D3DSPR_SAMPLER
	default:
		fprintf(stderr, "Unimplemented register type %d\n", eRegisterType);
		return false;
	};

	SEOUL_VERIFY(m_tRegisterTable.Insert(RegisterEntry(eRegisterType, uRegisterNumber), rOut.m_Name).Second);

	if (RegisterType::kAddress == eRegisterType || RegisterType::kTemp == eRegisterType)
	{
		m_vHeaderCode.PushBack(String::Printf("vec4 %s;", rOut.m_Name.CStr()));
	}

	return true;
}

inline Byte ToSwizzle(UInt32 uSwizzle)
{
	switch (uSwizzle)
	{
	case 0: return 'x';
	case 1: return 'y';
	case 2: return 'z';
	case 3: return 'w';
	default:
		return ' ';
	};
}

Bool ShaderReceiverGLSLES2::ToString(String& rs, DestinationRegister reg, Bool bIgnoreWriteMask /* = false */)
{
	UInt32 const uRegisterNumber = reg.GetRegisterNumber();
	Bool const bX = reg.UseX();
	Bool const bY = reg.UseY();
	Bool const bZ = reg.UseZ();
	Bool const bW = reg.UseW();
	RegisterType const eRegisterType = reg.GetRegisterType();

	NameAndOffset nameAndOffset;
	if (!RegisterTypeToString(nameAndOffset, eRegisterType, uRegisterNumber))
	{
		return false;
	}
	rs.Clear();
	rs.Append(nameAndOffset.ToString());

	if (bIgnoreWriteMask || (bX && bY && bZ && bW))
	{
		return true;
	}

	if (bX || bY || bZ || bW)
	{
		rs.Append('.');
	}

	if (bX) { rs.Append('x'); }
	if (bY) { rs.Append('y'); }
	if (bZ) { rs.Append('z'); }
	if (bW) { rs.Append('w'); }

	return true;
}

Bool ShaderReceiverGLSLES2::ToString(String& rs, DestinationRegister reg, UInt32 uComponent)
{
	UInt32 const uRegisterNumber = reg.GetRegisterNumber();
	RegisterType const eRegisterType = reg.GetRegisterType();

	NameAndOffset nameAndOffset;
	if (!RegisterTypeToString(nameAndOffset, eRegisterType, uRegisterNumber))
	{
		return false;
	}

	rs.Clear();
	rs.Append(nameAndOffset.ToString());
	rs.Append('.');

	switch (uComponent)
	{
	case 0u: rs.Append('x'); break;
	case 1u: rs.Append('y'); break;
	case 2u: rs.Append('z'); break;
	case 3u: rs.Append('w'); break;
	default:
		break;
	};

	return true;
}

static inline Bool FinishToString(
	String& rs,
	SourceModifier eSourceModifier,
	const String& sRegisterType,
	const String& sComponentSelection)
{
	switch (eSourceModifier)
	{
	case SourceModifier::kNone:                   rs.Assign(String::Printf( "%s%s", sRegisterType.CStr(), sComponentSelection.CStr())); break;
	case SourceModifier::kNegate:                 rs.Assign(String::Printf("-%s%s", sRegisterType.CStr(), sComponentSelection.CStr())); break;
	case SourceModifier::kBias:                   rs.Assign(String::Printf(" (%s%s - 0.5)", sRegisterType.CStr(), sComponentSelection.CStr())); break;
	case SourceModifier::kBiasAndNegate:          rs.Assign(String::Printf("-(%s%s - 0.5)", sRegisterType.CStr(), sComponentSelection.CStr())); break;
	case SourceModifier::kSign:                   rs.Assign(String::Printf( "sign(%s%s)", sRegisterType.CStr(), sComponentSelection.CStr())); break;
	case SourceModifier::kSignAndNegate:          rs.Assign(String::Printf("-sign(%s%s)", sRegisterType.CStr(), sComponentSelection.CStr())); break;
	case SourceModifier::kComplement:             rs.Assign(String::Printf(" (1.0 - %s%s)", sRegisterType.CStr(), sComponentSelection.CStr())); break;
	case SourceModifier::kX2:                     rs.Assign(String::Printf(" (%s%s * 2.0)", sRegisterType.CStr(), sComponentSelection.CStr())); break;
	case SourceModifier::kX2AndNegate:            rs.Assign(String::Printf("-(%s%s * 2.0)", sRegisterType.CStr(), sComponentSelection.CStr())); break;
	case SourceModifier::kAbsoluteValue:          rs.Assign(String::Printf( "abs(%s%s)", sRegisterType.CStr(), sComponentSelection.CStr())); break;
	case SourceModifier::kAbsoluteValueAndNegate: rs.Assign(String::Printf("-abs(%s%s)", sRegisterType.CStr(), sComponentSelection.CStr())); break;
	case SourceModifier::kPredicateNot:           rs.Assign(String::Printf("(!%s%s)", sRegisterType.CStr(), sComponentSelection.CStr())); break;

		// Unsupported.
	case SourceModifier::kDivideByZ: // fall-through
	case SourceModifier::kDivideByW: // fall-through
	default:
		fprintf(stderr, "Unsupported source modifier %d\n", eSourceModifier);
		return false;
	};

	return true;
}

Bool ShaderReceiverGLSLES2::ToString(String& rs, SourceRegister reg, Bool bX, Bool bY, Bool bZ, Bool bW)
{
	UInt32 const uRegisterNumber = reg.GetRegisterNumber();
	UInt32 const uSwizzleX = reg.GetSwizzleX();
	UInt32 const uSwizzleY = reg.GetSwizzleY();
	UInt32 const uSwizzleZ = reg.GetSwizzleZ();
	UInt32 const uSwizzleW = reg.GetSwizzleW();
	SourceModifier const eSourceModifier = reg.GetSourceModifier();
	RegisterType const eRegisterType = reg.GetRegisterType();

	NameAndOffset nameAndOffset;
	if (!RegisterTypeToString(nameAndOffset, eRegisterType, uRegisterNumber))
	{
		return false;
	}

	String s;
	if (reg.UseRelativeAddressing())
	{
		String sLookup;
		if (!ToString(sLookup, *reg.GetRelativeAddress(), true, false, false, false))
		{
			return false;
		}

		s = String(nameAndOffset.m_Name);
		if (nameAndOffset.m_iOffset >= 0)
		{
			s.Append('[');
			s.Append(String::Printf("%d + ", nameAndOffset.m_iOffset));
			s.Append("int(");
			s.Append(sLookup);
			s.Append(")]");
		}
		else
		{
			s.Append("[int(");
			s.Append(sLookup);
			s.Append(")]");
		}
	}
	else
	{
		s = nameAndOffset.ToString();
	}

	String sComponentSelection;
	if (!bX || !bY || !bZ || !bW ||
		uSwizzleX != 0 ||
		uSwizzleY != 1 ||
		uSwizzleZ != 2 ||
		uSwizzleW != 3)
	{
		// Check for swizzle support.
		if (CanSwizzle(eRegisterType, uRegisterNumber))
		{
			if (bX || bY || bZ || bW)
			{
				sComponentSelection.Append('.');
			}
			if (bX) { sComponentSelection.Append(ToSwizzle(uSwizzleX)); }
			if (bY) { sComponentSelection.Append(ToSwizzle(uSwizzleY)); }
			if (bZ) { sComponentSelection.Append(ToSwizzle(uSwizzleZ)); }
			if (bW) { sComponentSelection.Append(ToSwizzle(uSwizzleW)); }
		}
	}

	return FinishToString(rs, eSourceModifier, s, sComponentSelection);
}

Bool ShaderReceiverGLSLES2::ComponentToString(String& rs, SourceRegister reg, UInt32 uComponent)
{
	UInt32 const uRegisterNumber = reg.GetRegisterNumber();
	UInt32 const uSwizzleX = reg.GetSwizzleX();
	UInt32 const uSwizzleY = reg.GetSwizzleY();
	UInt32 const uSwizzleZ = reg.GetSwizzleZ();
	UInt32 const uSwizzleW = reg.GetSwizzleW();
	SourceModifier const eSourceModifier = reg.GetSourceModifier();
	RegisterType const eRegisterType = reg.GetRegisterType();

	NameAndOffset nameAndOffset;
	if (!RegisterTypeToString(nameAndOffset, eRegisterType, uRegisterNumber))
	{
		return false;
	}

	String s;
	if (reg.UseRelativeAddressing())
	{
		String sLookup;
		if (!ToString(sLookup, *reg.GetRelativeAddress(), true, false, false, false))
		{
			return false;
		}

		s = String(nameAndOffset.m_Name);
		if (nameAndOffset.m_iOffset >= 0)
		{
			s.Append('[');
			s.Append(String::Printf("%d + ", nameAndOffset.m_iOffset));
			s.Append("int(");
			s.Append(sLookup);
			s.Append(")]");
		}
		else
		{
			s.Append("[int(");
			s.Append(sLookup);
			s.Append(")]");
		}
	}
	else
	{
		s = nameAndOffset.ToString();
	}

	String sComponentSelection;
	if (CanSwizzle(eRegisterType, uRegisterNumber))
	{
		sComponentSelection.Append('.');
		switch (uComponent)
		{
		case 0u: sComponentSelection.Append(ToSwizzle(uSwizzleX)); break;
		case 1u: sComponentSelection.Append(ToSwizzle(uSwizzleY)); break;
		case 2u: sComponentSelection.Append(ToSwizzle(uSwizzleZ)); break;
		case 3u: sComponentSelection.Append(ToSwizzle(uSwizzleW)); break;
		default:
			break;
		};
	}

	return FinishToString(rs, eSourceModifier, s, sComponentSelection);
}

ShaderReceiverGLSLES2::ShaderReceiverGLSLES2(const Converter& effectConverter)
	: m_rEffectConverter(effectConverter)
	, m_eShaderType(ShaderType::kPixel)
	, m_vHeaderCode()
	, m_vMainCode()
	, m_uFlags(0u)
{
}

ShaderReceiverGLSLES2::~ShaderReceiverGLSLES2()
{
}

Bool ShaderReceiverGLSLES2::TokenBeginShader(UInt32 uMajorVersion, UInt32 uMinorVersion, ShaderType eType)
{
	// Cache the shader type.
	m_eShaderType = eType;

	// Setup default header code.
	m_vHeaderCode.PushBack(ksGlslVersionString);

	// Note that, according to the OpenGL ES 2.0 specification, vertex shaders have the following
	// default precision specifiers:
	//     precision highp float;
	//     precision highp int;
	//     precision lowp sampler2D;
	//     precision lowp samplerCube;
	//
	// Fragment shaders have the following default precision specifiers:
	//     precision mediump int;
	//     precision lowp sampler2D;
	//     precision lowp samplerCube;
	//
	// That said, experience on Android indicates the following to avoid/workaround various bad
	// behavior and bugs in vendor drivers:
	// - don't specify default precision in the vertex shader.
	// - always explicitly specify default precision for int and float in the fragment shader.
	if (ShaderType::kPixel == m_eShaderType)
	{
		m_vHeaderCode.PushBack("precision lowp float;");
		m_vHeaderCode.PushBack("precision lowp int;");
	}

	return true;
}

Bool ShaderReceiverGLSLES2::TokenComment(Byte const* sComment, UInt32 nStringLengthInBytes)
{
	return true;
}

Bool ShaderReceiverGLSLES2::TokenConstantTable(const Constants& vConstants)
{
	for (UInt32 i = 0u; i < vConstants.GetSize(); ++i)
	{
		Constant constant = vConstants[i];
		HString name = constant.m_Name;

		for (UInt32 j = 0u; j < m_rEffectConverter.GetParameters().GetSize(); ++j)
		{
			if (m_rEffectConverter.GetParameters()[j].m_Name == name)
			{
				name = m_rEffectConverter.GetParameters()[j].m_Semantic;
				break;
			}
		}

		String sArrayPart;
		if (ConstantType::kBool4 == constant.m_eType ||
			ConstantType::kFloat4 == constant.m_eType ||
			ConstantType::kInt4 == constant.m_eType)
		{
			if (constant.m_uRegisterCount > 1)
			{
				sArrayPart = String::Printf("[%u]", constant.m_uRegisterCount);
			}
		}

		switch (constant.m_eType)
		{
		case ConstantType::kBool4:
			m_vHeaderCode.PushBack(String::Printf("uniform bvec4 %s%s;", name.CStr(), sArrayPart.CStr()));
			break;
		case ConstantType::kFloat4:
			switch (constant.m_uColsCount)
			{
			case 1:
				m_vHeaderCode.PushBack(String::Printf("uniform float %s%s;", name.CStr(), sArrayPart.CStr()));
				break;
			case 2:
				m_vHeaderCode.PushBack(String::Printf("uniform vec2 %s%s;", name.CStr(), sArrayPart.CStr()));
				break;
			case 3:
				m_vHeaderCode.PushBack(String::Printf("uniform vec3 %s%s;", name.CStr(), sArrayPart.CStr()));
				break;
			case 4:
				m_vHeaderCode.PushBack(String::Printf("uniform vec4 %s%s;", name.CStr(), sArrayPart.CStr()));
				break;
			};
			break;
		case ConstantType::kInt4:
			m_vHeaderCode.PushBack(String::Printf("uniform ivec4 %s%s;", name.CStr(), sArrayPart.CStr()));
			break;
		case ConstantType::kSampler2D:
			m_vHeaderCode.PushBack(String::Printf("uniform sampler2D %s;", name.CStr()));
			SEOUL_VERIFY(m_tRegisterTable.Insert(RegisterEntry(RegisterType::kSampler, constant.m_uRegisterNumber), name).Second);
			break;
		case ConstantType::kSampler3D:
			m_vHeaderCode.PushBack(String::Printf("uniform sampler3D %s;", name.CStr()));
			SEOUL_VERIFY(m_tRegisterTable.Insert(RegisterEntry(RegisterType::kSampler, constant.m_uRegisterNumber), name).Second);
			break;
		case ConstantType::kSamplerCube:
			m_vHeaderCode.PushBack(String::Printf("uniform samplerCube %s;", name.CStr()));
			SEOUL_VERIFY(m_tRegisterTable.Insert(RegisterEntry(RegisterType::kSampler, constant.m_uRegisterNumber), name).Second);
			break;
		default:
			return false;
		}

		if (ConstantType::kBool4 == constant.m_eType ||
			ConstantType::kFloat4 == constant.m_eType ||
			ConstantType::kInt4 == constant.m_eType)
		{
			if (constant.m_uRegisterCount == 1)
			{
				m_aConstantRegisters[constant.m_uRegisterNumber].m_NameAndOffset.Reset();
				m_aConstantRegisters[constant.m_uRegisterNumber].m_NameAndOffset.m_Name = name;
				m_aConstantRegisters[constant.m_uRegisterNumber].m_uColsCount = constant.m_uColsCount;
			}
			else
			{
				for (UInt32 i = 0u; i < constant.m_uRegisterCount; ++i)
				{
					m_aConstantRegisters[constant.m_uRegisterNumber + i].m_NameAndOffset.m_Name = name;
					m_aConstantRegisters[constant.m_uRegisterNumber + i].m_NameAndOffset.m_iOffset = (Int32)i;
					m_aConstantRegisters[constant.m_uRegisterNumber + i].m_uColsCount = constant.m_uColsCount;
				}
			}
		}

		m_tConstantRegisterLookupTable.Overwrite(name, constant.m_uRegisterNumber);
	}

	return true;
}

Bool ShaderReceiverGLSLES2::TokenEndShader()
{
	// Immediately after the version line.
	auto iInsert = m_vHeaderCode.Find(ksGlslVersionString);
	SEOUL_ASSERT(m_vHeaderCode.End() != iInsert);
	++iInsert;

	// Prepend some additional header bits if requested by the shader body.
	if (kFlagDerivatives == (kFlagDerivatives & m_uFlags))
	{
		m_vHeaderCode.Insert(iInsert, "#extension GL_OES_standard_derivatives : enable");
	}

	return true;
}

inline Byte const* ToTextureFunctionString(SamplerType eType)
{
	switch (eType)
	{
	case SamplerType::k2D: return "texture2D";
	case SamplerType::kCube: return "textureCube";
	case SamplerType::kVolume: return "texture3D";
	default:
		return "";
	};
}

inline Byte const* ToAttributeString(VertexElement::EUsage eUsage, UInt32 uUsageIndex)
{
	switch (eUsage)
	{
	case VertexElement::UsagePosition: return "seoul_attribute_Vertex";
	case VertexElement::UsageBlendWeight: return "seoul_attribute_BlendWeight";
	case VertexElement::UsageBlendIndices: return "seoul_attribute_BlendIndices";
	case VertexElement::UsageNormal: return "seoul_attribute_Normal";
	case VertexElement::UsageTexcoord:
		switch (uUsageIndex)
		{
		case 0u: return "seoul_attribute_MultiTexCoord0";
		case 1u: return "seoul_attribute_MultiTexCoord1";
		case 2u: return "seoul_attribute_MultiTexCoord2";
		case 3u: return "seoul_attribute_MultiTexCoord3";
		case 4u: return "seoul_attribute_MultiTexCoord4";
		case 5u: return "seoul_attribute_MultiTexCoord5";
		case 6u: return "seoul_attribute_MultiTexCoord6";
		case 7u: return "seoul_attribute_MultiTexCoord7";
		default:
			return "";
		};
		break;
	case VertexElement::UsageTangent: return "seoul_attribute_Tangent";
	case VertexElement::UsageBinormal: return "seoul_attribute_Binormal";
	case VertexElement::UsageColor:
		switch (uUsageIndex)
		{
		case 0: return "seoul_attribute_Color";
		case 1: return "seoul_attribute_SecondaryColor";
		default:
			return "";
		};
		break;
	default:
		return "";
	};

	return "";
}

inline Byte const* ToVaryingString(VertexElement::EUsage eUsage, UInt32 uUsageIndex)
{
	switch (eUsage)
	{
	case VertexElement::UsagePosition: return "gl_Position";
	case VertexElement::UsageBlendWeight: return "seoul_varying_BlendWeight";
	case VertexElement::UsageBlendIndices: return "seoul_varying_BlendIndices";
	case VertexElement::UsageNormal: return "seoul_varying_Normal";
	case VertexElement::UsageTexcoord:
		switch (uUsageIndex)
		{
		case 0u: return "seoul_varying_MultiTexCoord0";
		case 1u: return "seoul_varying_MultiTexCoord1";
		case 2u: return "seoul_varying_MultiTexCoord2";
		case 3u: return "seoul_varying_MultiTexCoord3";
		case 4u: return "seoul_varying_MultiTexCoord4";
		case 5u: return "seoul_varying_MultiTexCoord5";
		case 6u: return "seoul_varying_MultiTexCoord6";
		case 7u: return "seoul_varying_MultiTexCoord7";
		default:
			return "";
		};
		break;
	case VertexElement::UsageTangent: return "seoul_varying_Tangent";
	case VertexElement::UsageBinormal: return "seoul_varying_Binormal";
	case VertexElement::UsageColor:
		switch (uUsageIndex)
		{
		case 0: return "seoul_varying_Color";
		case 1: return "seoul_varying_SecondaryColor";
		default:
			return "";
		};
		break;
	default:
		return "";
	};

	return "";
}

inline Byte const* ToFragmentVaryingPrecision(VertexElement::EUsage eUsage)
{
	switch (eUsage)
	{
	case VertexElement::UsagePosition: // fall-through
	case VertexElement::UsageBlendWeight: // fall-through
	case VertexElement::UsageNormal: // fall-through
	case VertexElement::UsageTexcoord: // fall-through
	case VertexElement::UsageTangent: // fall-through
	case VertexElement::UsageBinormal: // fall-through
	case VertexElement::UsageTessfactor: // fall-through
	case VertexElement::UsagePositionT: // fall-through
	case VertexElement::UsageFog: // fall-through
	case VertexElement::UsageDepth: // fall-through
	case VertexElement::UsageSample: // fall-through
		return "mediump";

	case VertexElement::UsageBlendIndices: // fall-through
	case VertexElement::UsagePSize: // fall-through
	case VertexElement::UsageColor: // fall-through
	default:
		return "lowp";
	};
}

Bool ShaderReceiverGLSLES2::TokenDclInstruction(const DestinationRegister& destination, DclToken dclToken)
{
	RegisterType const eType = destination.GetRegisterType();
	switch (eType)
	{
	case RegisterType::kInput:
		if (m_eShaderType == ShaderType::kVertex)
		{
			m_vHeaderCode.PushBack(String::Printf("attribute vec4 %s;", ToAttributeString(dclToken.GetUsage(), dclToken.GetUsageIndex())));
			SEOUL_VERIFY(m_tRegisterTable.Insert(RegisterEntry(destination.GetRegisterType(), destination.GetRegisterNumber()), HString(ToAttributeString(dclToken.GetUsage(), dclToken.GetUsageIndex()))).Second);
		}
		else
		{
			m_vHeaderCode.PushBack(String::Printf("varying %s vec4 %s;",
				ToFragmentVaryingPrecision(dclToken.GetUsage()),
				ToVaryingString(dclToken.GetUsage(), dclToken.GetUsageIndex())));
			SEOUL_VERIFY(m_tRegisterTable.Insert(RegisterEntry(destination.GetRegisterType(), destination.GetRegisterNumber()), HString(ToVaryingString(dclToken.GetUsage(), dclToken.GetUsageIndex()))).Second);
		}
		return true;
	case RegisterType::kOutput:
		// Position output is implicit - it is always gl_Position and does not need to be explicitly declared
		// (it is an error to do so).
		if (dclToken.GetUsage() != VertexElement::UsagePosition)
		{
			m_vHeaderCode.PushBack(String::Printf("varying vec4 %s;", ToVaryingString(dclToken.GetUsage(), dclToken.GetUsageIndex())));
		}
		SEOUL_VERIFY(m_tRegisterTable.Insert(RegisterEntry(destination.GetRegisterType(), destination.GetRegisterNumber()), HString(ToVaryingString(dclToken.GetUsage(), dclToken.GetUsageIndex()))).Second);
		return true;
	case RegisterType::kSampler:
		m_aSamplers[destination.GetRegisterNumber()] = dclToken.GetSamplerType();
		return true;
	default:
		return false;
	};
}

Bool ShaderReceiverGLSLES2::TokenDefInstruction(const DestinationRegister& destination, const Vector4D& v)
{
	String sDestination;
	if (!ToString(sDestination, destination))
	{
		return false;
	}
	m_vHeaderCode.PushBack(String::Printf("const vec4 %s = vec4(%g, %g, %g, %g);", sDestination.CStr(), v.X, v.Y, v.Z, v.W));
	return true;
}

namespace
{

struct InstructionWriter SEOUL_SEALED
{
	InstructionWriter(Vector<String>& r, const DestinationRegister& dest)
		: m_r(r)
		, m_Dest(dest)
	{
	}

	void Write(const String& sDestination, Byte const* sFormat, ...) SEOUL_PRINTF_FORMAT_ATTRIBUTE(1, 2)
	{
		va_list argList;
		va_start(argList, sFormat);

		if (m_Dest.UseSaturate())
		{
			m_r.PushBack(sDestination + " = clamp(" + String::VPrintf(sFormat, argList) + ", 0.0, 1.0);");
		}
		else
		{
			m_r.PushBack(sDestination + " = " + String::VPrintf(sFormat, argList) + ";");
		}

		va_end(argList);
	}

	void WriteDiscard(const String& sDestination)
	{
		m_r.PushBack(String::Printf("if(any(lessThan((%s).xyz, vec3(0)))) { discard; }", sDestination.CStr()));
	}

	Vector<String>& m_r;
	const DestinationRegister& m_Dest;
}; // struct InstructionWriter

} // namespace anonymous

Bool ShaderReceiverGLSLES2::TokenInstruction(
	OpCode eOpCode,
	const DestinationRegister& destination,
	const SourceRegister& sourceA,
	const SourceRegister& sourceB /*= SourceRegister()*/,
	const SourceRegister& sourceC /*= SourceRegister()*/,
	const SourceRegister& sourceD /*= SourceRegister()*/)
{
	UInt32 const nDestinationCount = destination.GetComponentCount();

	Bool const bSourceUseX = destination.UseX() || (OpCode::kDp4 == eOpCode || OpCode::kDp3 == eOpCode);
	Bool const bSourceUseY = destination.UseY() || (OpCode::kDp4 == eOpCode || OpCode::kDp3 == eOpCode);
	Bool const bSourceUseZ = destination.UseZ() || (OpCode::kDp4 == eOpCode || OpCode::kDp3 == eOpCode);
	Bool const bSourceUseW = destination.UseW() || (OpCode::kDp4 == eOpCode);

	String sDestination;
	if (destination.IsValid()) { if (!ToString(sDestination, destination)) { return false; } }

	String sSourceA;
	if (sourceA.IsValid()) { if (!ToString(sSourceA, sourceA, bSourceUseX, bSourceUseY, bSourceUseZ, bSourceUseW)) { return false; } }

	String sSourceB;
	if (sourceB.IsValid()) { if (!ToString(sSourceB, sourceB, bSourceUseX, bSourceUseY, bSourceUseZ, bSourceUseW)) { return false; } }

	String sSourceC;
	if (sourceC.IsValid()) { if (!ToString(sSourceC, sourceC, bSourceUseX, bSourceUseY, bSourceUseZ, bSourceUseW)) { return false; } }

	String sSourceD;
	if (sourceD.IsValid()) { if (!ToString(sSourceD, sourceD, bSourceUseX, bSourceUseY, bSourceUseZ, bSourceUseW)) { return false; } }

	InstructionWriter writer(m_vMainCode, destination);

	switch (eOpCode)
	{
	case OpCode::kAdd:
		writer.Write(sDestination, "%s + %s", sSourceA.CStr(), sSourceB.CStr());
		return true;
	case OpCode::kAbs:
		writer.Write(sDestination, "abs(%s)", sSourceA.CStr());
		return true;
	case OpCode::kCmp:
		{
			Bool bSourceAEqual = true;
			for (UInt32 i = 1u; i < nDestinationCount; ++i)
			{
				if (sourceA.GetSwizzle(i) != sourceA.GetSwizzle(0))
				{
					bSourceAEqual = false;
					break;
				}
			}

			if (bSourceAEqual)
			{
				if (!ToString(sSourceA, sourceA, true, false, false, false))
				{
					return false;
				}
			}

			if (bSourceAEqual || nDestinationCount == 1)
			{
				writer.Write(sDestination, "(%s >= 0.0) ? %s : %s", sSourceA.CStr(), sSourceB.CStr(), sSourceC.CStr());
			}
			else
			{
				if (bSourceUseX)
				{
					if (!ToString(sDestination, destination, 0u)) { return false; }
					if (sourceA.IsValid()) { if (!ComponentToString(sSourceA, sourceA, 0u)) { return false; } }
					if (sourceB.IsValid()) { if (!ComponentToString(sSourceB, sourceB, 0u)) { return false; } }
					if (sourceC.IsValid()) { if (!ComponentToString(sSourceC, sourceC, 0u)) { return false; } }

					writer.Write(
						sDestination,
						"(%s >= 0.0) ? %s: %s",
						sourceA.IsValid() ? sSourceA.CStr() : "",
						sourceB.IsValid() ? sSourceB.CStr() : "",
						sourceC.IsValid() ? sSourceC.CStr() : "");
				}
				if (bSourceUseY)
				{
					if (!ToString(sDestination, destination, 1u)) { return false; }
					if (sourceA.IsValid()) { if (!ComponentToString(sSourceA, sourceA, 1u)) { return false; } }
					if (sourceB.IsValid()) { if (!ComponentToString(sSourceB, sourceB, 1u)) { return false; } }
					if (sourceC.IsValid()) { if (!ComponentToString(sSourceC, sourceC, 1u)) { return false; } }

					writer.Write(
						sDestination,
						"(%s >= 0.0) ? %s: %s",
						sourceA.IsValid() ? sSourceA.CStr() : "",
						sourceB.IsValid() ? sSourceB.CStr() : "",
						sourceC.IsValid() ? sSourceC.CStr() : "");
				}
				if (bSourceUseZ)
				{
					if (!ToString(sDestination, destination, 2u)) { return false; }
					if (sourceA.IsValid()) { if (!ComponentToString(sSourceA, sourceA, 2u)) { return false; } }
					if (sourceB.IsValid()) { if (!ComponentToString(sSourceB, sourceB, 2u)) { return false; } }
					if (sourceC.IsValid()) { if (!ComponentToString(sSourceC, sourceC, 2u)) { return false; } }

					writer.Write(
						sDestination,
						"(%s >= 0.0) ? %s: %s",
						sourceA.IsValid() ? sSourceA.CStr() : "",
						sourceB.IsValid() ? sSourceB.CStr() : "",
						sourceC.IsValid() ? sSourceC.CStr() : "");
				}
				if (bSourceUseW)
				{
					if (!ToString(sDestination, destination, 3u)) { return false; }
					if (sourceA.IsValid()) { if (!ComponentToString(sSourceA, sourceA, 3u)) { return false; } }
					if (sourceB.IsValid()) { if (!ComponentToString(sSourceB, sourceB, 3u)) { return false; } }
					if (sourceC.IsValid()) { if (!ComponentToString(sSourceC, sourceC, 3u)) { return false; } }

					writer.Write(
						sDestination,
						"(%s >= 0.0) ? %s: %s",
						sourceA.IsValid() ? sSourceA.CStr() : "",
						sourceB.IsValid() ? sSourceB.CStr() : "",
						sourceC.IsValid() ? sSourceC.CStr() : "");
				}
			}
		}
		return true;
	case OpCode::kDp2add:
		writer.Write(sDestination, "dot(vec2(%s), vec2(%s)) + %s", sSourceA.CStr(), sSourceB.CStr(), sSourceC.CStr());
		return true;
	case OpCode::kDp3: // fall-through
	case OpCode::kDp4:
		writer.Write(sDestination, "dot(%s, %s)", sSourceA.CStr(), sSourceB.CStr());
		return true;
	case OpCode::kDsx:
		writer.Write(sDestination, "dFdx(%s)", sSourceA.CStr());
		m_uFlags |= kFlagDerivatives;
		return true;
	case OpCode::kDsy:
		writer.Write(sDestination, "dFdy(%s)", sSourceA.CStr());
		m_uFlags |= kFlagDerivatives;
		return true;
	case OpCode::kFrc:
		writer.Write(sDestination, "fract(%s)", sSourceA.CStr());
		return true;
	case OpCode::kLrp:
		// Not a typo - the arguments to mix() are reversed compared to the operands of lrp
		writer.Write(sDestination, "mix(%s, %s, %s)", sSourceC.CStr(), sSourceB.CStr(), sSourceA.CStr());
		return true;
	case OpCode::kMax:
		writer.Write(sDestination, "max(%s, %s)", sSourceA.CStr(), sSourceB.CStr());
		return true;
	case OpCode::kMin:
		writer.Write(sDestination, "min(%s, %s)", sSourceA.CStr(), sSourceB.CStr());
		return true;
	case OpCode::kMad:
		writer.Write(sDestination, "(%s * %s) + %s", sSourceA.CStr(), sSourceB.CStr(), sSourceC.CStr());
		return true;
	case OpCode::kMov:
		writer.Write(sDestination, "%s", sSourceA.CStr());
		return true;
	case OpCode::kMova:
		writer.Write(sDestination, "floor(%s)", sSourceA.CStr());
		return true;
	case OpCode::kMul:
		writer.Write(sDestination, "%s * %s", sSourceA.CStr(), sSourceB.CStr());
		return true;
	case OpCode::kNrm:
		writer.Write(sDestination, "normalize(%s)", sSourceA.CStr());
		return true;
	case OpCode::kRcp:
		writer.Write(sDestination, "(%s == 0.0) ? 0.0 : (1.0 / %s)", sSourceA.CStr(), sSourceA.CStr());
		return true;
	case OpCode::kRsq:
		writer.Write(sDestination, "(%s == 0.0) ? 0.0 : inversesqrt(%s)", sSourceA.CStr(), sSourceA.CStr());
		return true;
	case OpCode::kNop:
		return true;
	case OpCode::kPow:
		writer.Write(sDestination, "pow(%s, %s)", sSourceA.CStr(), sSourceB.CStr());
		return true;
	case OpCode::kRet:
		return true;
	case OpCode::kSlt:
		if (nDestinationCount == 1)
		{
			writer.Write(sDestination, "float(%s < %s)", sSourceA.CStr(), sSourceB.CStr());
		}
		else
		{
			writer.Write(sDestination, "vec%u(lessThan(%s, %s))", nDestinationCount, sSourceA.CStr(), sSourceB.CStr());
		}
		return true;
	case OpCode::kSub:
		writer.Write(sDestination, "%s - %s", sSourceA.CStr(), sSourceB.CStr());
		return true;
	case OpCode::kTexkill:
		writer.WriteDiscard(sDestination);
		return true;
	case OpCode::kTexld:
		{
			SamplerType const eSamplerType = m_aSamplers[sourceB.GetRegisterNumber()];
			if (!ToString(
				sSourceA,
				sourceA,
				bSourceUseX,
				bSourceUseY,
				bSourceUseZ && (eSamplerType != SamplerType::k2D),
				bSourceUseW && (eSamplerType == SamplerType::kVolume)))
			{
				return false;
			}

			writer.Write(sDestination, "%s(%s, %s)",
				ToTextureFunctionString(eSamplerType),
				sSourceB.CStr(),
				sSourceA.CStr());
		}
		return true;
	default:
		fprintf(stderr, "Unimplemented opcode: '%s'\n", EffectConverter::ToString(eOpCode));
		return false;
	};
}

} // namespace Seoul
