/**
 * \file IShaderReceiver.h
 * \brief Inherit from this interface to receive
 * events from an EffectConverter specific to shader data.
 * Includes bytecode and share constants.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ISHADER_RECEIVER_H
#define ISHADER_RECEIVER_H

#include "Prereqs.h"
#include "SeoulHString.h"
#include "SharedPtr.h"
#include "StreamBuffer.h"
#include "Vector4D.h"
#include "VertexElement.h"

namespace Seoul::EffectConverter
{

enum class OpCode : UInt16
{
	kNop,
	kMov,
	kAdd,
	kSub,
	kMad,
	kMul,
	kRcp,
	kRsq,
	kDp3,
	kDp4,
	kMin,
	kMax,
	kSlt,
	kSge,
	kExp,
	kLog,
	kLit,
	kDst,
	kLrp,
	kFrc,
	kM4x4,
	kM4x3,
	kM3x4,
	kM3x3,
	kM3x2,
	kCall,
	kCallnz,
	kLoop,
	kRet,
	kEndloop,
	kLabel,
	kDcl,
	kPow,
	kCrs,
	kSgn,
	kAbs,
	kNrm,
	kSincos,
	kRep,
	kEndrep,
	kIf,
	kIfc,
	kElse,
	kEndif,
	kBreak,
	kBreakc,
	kMova,
	kDefb,
	kDefi,
	kReserved0,
	kReserved1,
	kReserved2,
	kReserved3,
	kReserved4,
	kReserved5,
	kReserved6,
	kReserved7,
	kReserved8,
	kReserved9,
	kReserved10,
	kReserved11,
	kReserved12,
	kReserved13,
	kReserved14,
	kTexcrd,
	kTexkill,
	kTexld,
	kTexbem,
	kTexbeml,
	kTexreg2ar,
	kTexreg2gb,
	kTexm3x2pad,
	kTexm3x2tex,
	kTexm3x3pad,
	kTexm3x3tex,
	kReserved15,
	kTexm3x3spec,
	kTexm3x3vspec,
	kExpp,
	kLogp,
	kCnd,
	kDef,
	kTexreg2rgb,
	kTexdp3tex,
	kTexm3x2depth,
	kTexdp3,
	kTexm3x3,
	kTexdepth,
	kCmp,
	kBem,
	kDp2add,
	kDsx,
	kDsy,
	kTexldd,
	kSetp,
	kTexldl,
	kBreakp,
	BASIC_COUNT,

	// Special come after COUNT.
	kSpecialPhase = 0xFFFD,
	kSpecialComment = 0xFFFE,
	kSpecialEnd = 0xFFFF,
};

enum class RegisterType : UInt32
{
	kTemp = 0,
	kInput = 1,
	kConst = 2,
	kAddress = 3, // Vertex Shader
	kTexture = 3, // Pixel Shader
	kRastOut = 4,
	kAttributeOut = 5,
	kTexCoordOut = 6,
	kOutput = 6,
	kConstInt = 7,
	kColorOut = 8,
	kDepthOut = 9,
	kSampler = 10,
	kConst2 = 11,
	kConst3 = 12,
	kConst4 = 13,
	kConstBool = 14,
	kLoop = 15,
	kTempFloat16 = 16,
	kMiscType = 17,
	kLabel = 18,
	kPredicate = 19,
};

enum class SourceModifier
{
	kNone                   = 0x0,
	kNegate                 = 0x1,
	kBias                   = 0x2,
	kBiasAndNegate          = 0x3,
	kSign                   = 0x4,
	kSignAndNegate          = 0x5,
	kComplement             = 0x6,
	kX2                     = 0x7,
	kX2AndNegate            = 0x8,
	kDivideByZ              = 0x9,
	kDivideByW              = 0xa,
	kAbsoluteValue          = 0xb,
	kAbsoluteValueAndNegate = 0xc,
	kPredicateNot           = 0xd,
};

static const UInt32 kDclUsageShift = 0u;
static const UInt32 kDclUsageMask = 0x0000000F;

static const UInt32 kDclUsageIndexShift = 16u;
static const UInt32 kDclUsageIndexMask = 0x000F0000;

static const UInt32 kTextureTypeShift = 27u;
static const UInt32 kTextureTypeMask = 0x78000000;

enum class SamplerType
{
	kUnknown = 0,
	k2D = 2,
	kCube = 3,
	kVolume = 4,
};

struct DclToken SEOUL_SEALED
{
	SamplerType GetSamplerType() const
	{
		SamplerType const eSamplerType = (SamplerType)((m_uData & kTextureTypeMask) >> kTextureTypeShift);
		return eSamplerType;
	}

	VertexElement::EUsage GetUsage() const
	{
		VertexElement::EUsage const eUsage = (VertexElement::EUsage)((m_uData & kDclUsageMask) >> kDclUsageShift);
		return eUsage;
	}

	UInt32 GetUsageIndex() const
	{
		UInt32 const uUsageIndex = ((m_uData & kDclUsageIndexMask) >> kDclUsageIndexShift);
		return uUsageIndex;
	}

	UInt32 m_uData;
}; // struct DclToken

class SourceRegister SEOUL_SEALED
{
public:
	SourceRegister()
		: m_uData(0u)
		, m_pSub(nullptr)
	{
	}

	SourceRegister(const SourceRegister& b)
		: m_uData(b.m_uData)
		, m_pSub(b.m_pSub)
	{
	}

	SourceRegister& operator=(const SourceRegister& b)
	{
		m_uData = b.m_uData;
		m_pSub = b.m_pSub;
		return *this;
	}

	Bool operator==(const SourceRegister& b) const
	{
		if (m_uData != b.m_uData)
		{
			return false;
		}

		if (m_pSub == b.m_pSub)
		{
			return true;
		}
		else if (m_pSub.IsValid() && b.m_pSub.IsValid())
		{
			return (*m_pSub == *(b.m_pSub));
		}
		else
		{
			return false;
		}
	}

	Bool operator!=(const SourceRegister& b) const
	{
		return !(*this == b);
	}

	UInt32 GetRegisterNumber() const
	{
		return (m_uData & 0x000007FF); // bits 0 through 10
	}

	RegisterType GetRegisterType() const
	{
		return (RegisterType)(((m_uData >> 28) & 0x7) | ((m_uData >> 8) & 0x18)); // bits 28-30, 11-12
	}

	const SharedPtr<SourceRegister>& GetRelativeAddress() const
	{
		return m_pSub;
	}

	SourceModifier GetSourceModifier() const
	{
		return (SourceModifier)((m_uData >> 24) & 0xF); // bits 24 through 27
	}

	UInt32 GetSwizzleX() const
	{
		return ((m_uData >> 16) & 0x03);
	}

	UInt32 GetSwizzleY() const
	{
		return ((m_uData >> 18) & 0x03);
	}

	UInt32 GetSwizzleZ() const
	{
		return ((m_uData >> 20) & 0x03);
	}

	UInt32 GetSwizzleW() const
	{
		return ((m_uData >> 22) & 0x03);
	}

	UInt32 GetSwizzle(UInt32 i) const
	{
		return ((m_uData >> (16 + (i * 2))) & 0x03);
	}

	Bool IsValid() const
	{
		return (m_uData != 0u);
	}

	Bool Read(StreamBuffer& stream)
	{
		if (!stream.Read(m_uData))
		{
			return false;
		}

		if (UseRelativeAddressing())
		{
			m_pSub.Reset(SEOUL_NEW(MemoryBudgets::TBD) SourceRegister);
			if (!m_pSub->Read(stream))
			{
				return false;
			}
		}

		return true;
	}

	Bool UseRelativeAddressing() const
	{
		// Bit 13
		return (0u != ((m_uData >> 13) & 0x01));
	}

private:
	SEOUL_REFERENCE_COUNTED(SourceRegister);

	UInt32 m_uData;
	SharedPtr<SourceRegister> m_pSub;
}; // class SourceRegister

class DestinationRegister SEOUL_SEALED
{
public:
	DestinationRegister()
		: m_uData(0u)
	{
	}

	DestinationRegister(const DestinationRegister& b)
		: m_uData(b.m_uData)
	{
	}

	explicit DestinationRegister(UInt32 uData)
		: m_uData(uData)
	{
	}

	DestinationRegister& operator=(const DestinationRegister& b)
	{
		m_uData = b.m_uData;
		return *this;
	}

	Bool operator==(DestinationRegister b) const
	{
		return (m_uData == b.m_uData);
	}

	Bool operator!=(DestinationRegister b) const
	{
		return !(*this == b);
	}

	UInt32 GetComponentCount() const
	{
		UInt32 nReturn = 0u;
		nReturn += UseX() ? 1 : 0;
		nReturn += UseY() ? 1 : 0;
		nReturn += UseZ() ? 1 : 0;
		nReturn += UseW() ? 1 : 0;
		return nReturn;
	}

	UInt32 GetRegisterNumber() const
	{
		return (m_uData & 0x000007FF); // bits 0 through 10
	}

	RegisterType GetRegisterType() const
	{
		return (RegisterType)(((m_uData >> 28) & 0x7) | ((m_uData >> 8) & 0x18)); // bits 28-30, 11-12
	}

	Bool IsValid() const
	{
		return (m_uData != 0u);
	}

	Bool Read(StreamBuffer& stream)
	{
		if (!stream.Read(m_uData))
		{
			return false;
		}

		if (UseRelativeAddressing())
		{
			fprintf(stderr, "Relative addressing on destination register not yet supported.\n");
			return false;
		}

		return true;
	}

	Bool UseCentroid() const
	{
		// Bit 22
		return (0u != ((m_uData >> 22) & 0x01));
	}

	Bool UsePartialPrecision() const
	{
		// Bit 21
		return (0u != ((m_uData >> 21) & 0x01));
	}

	Bool UseRelativeAddressing() const
	{
		// Bit 13
		return (0u != ((m_uData >> 13) & 0x01));
	}

	Bool UseSaturate() const
	{
		// Bit 20
		return (0u != ((m_uData >> 20) & 0x01));
	}

	Bool UseX() const
	{
		// Bit 16
		return (0u != ((m_uData >> 16) & 0x01));
	}

	Bool UseY() const
	{
		// Bit 17
		return (0u != ((m_uData >> 17) & 0x01));
	}

	Bool UseZ() const
	{
		// Bit 18
		return (0u != ((m_uData >> 18) & 0x01));
	}

	Bool UseW() const
	{
		// Bit 19
		return (0u != ((m_uData >> 19) & 0x01));
	}

private:
	UInt32 m_uData;
}; // class DestinationRegister

enum class ConstantType
{
	kUnknown,
	kBool4,
	kFloat4,
	kInt4,
	kSampler2D,
	kSamplerCube,
	kSampler3D,
};

struct Constant
{
	ConstantType m_eType;
	HString m_Name;
	UInt16 m_uRegisterNumber;
	UInt16 m_uRegisterCount;
	UInt16 m_uRowCount;
	UInt16 m_uColsCount;
	UInt16 m_uElementsCount;
}; // struct Constant
typedef Vector<Constant> Constants;

enum class ShaderType
{
	kPixel,
	kVertex,
};

class IShaderReceiver SEOUL_ABSTRACT
{
public:
	IShaderReceiver()
	{
	}

	virtual ~IShaderReceiver()
	{
	}

	virtual Bool TokenBeginShader(UInt32 uMajorVersion, UInt32 uMinorVersion, ShaderType eType) = 0;
	virtual Bool TokenComment(Byte const* sComment, UInt32 nStringLengthInBytes) = 0;
	virtual Bool TokenConstantTable(const Constants& vConstants) = 0;
	virtual Bool TokenDclInstruction(const DestinationRegister& destination, DclToken dclToken) = 0;
	virtual Bool TokenDefInstruction(const DestinationRegister& destination, const Vector4D& v) = 0;
	virtual Bool TokenInstruction(
		OpCode eOpCode,
		const DestinationRegister& destination,
		const SourceRegister& sourceA,
		const SourceRegister& sourceB = SourceRegister(),
		const SourceRegister& sourceC = SourceRegister(),
		const SourceRegister& sourceD = SourceRegister()) = 0;
	virtual Bool TokenEndShader() = 0;

private:
	SEOUL_DISABLE_COPY(IShaderReceiver);
}; // class IShaderReceiver

inline static Byte const* ToString(OpCode eOp)
{
	switch (eOp)
	{
	case OpCode::kNop: return "nop";
	case OpCode::kMov: return "mov";
	case OpCode::kAdd: return "add";
	case OpCode::kSub: return "sub";
	case OpCode::kMad: return "mad";
	case OpCode::kMul: return "mul";
	case OpCode::kRcp: return "rcp";
	case OpCode::kRsq: return "rsq";
	case OpCode::kDp3: return "dp3";
	case OpCode::kDp4: return "dp4";
	case OpCode::kMin: return "min";
	case OpCode::kMax: return "max";
	case OpCode::kSlt: return "slt";
	case OpCode::kSge: return "sge";
	case OpCode::kExp: return "exp";
	case OpCode::kLog: return "log";
	case OpCode::kLit: return "lit";
	case OpCode::kDst: return "dst";
	case OpCode::kLrp: return "lrp";
	case OpCode::kFrc: return "frc";
	case OpCode::kM4x4: return "m4x4";
	case OpCode::kM4x3: return "m4x3";
	case OpCode::kM3x4: return "m3x4";
	case OpCode::kM3x3: return "m3x3";
	case OpCode::kM3x2: return "m3x2";
	case OpCode::kCall: return "call";
	case OpCode::kCallnz: return "callnz";
	case OpCode::kLoop: return "loop";
	case OpCode::kRet: return "ret";
	case OpCode::kEndloop: return "endloop";
	case OpCode::kLabel: return "label";
	case OpCode::kDcl: return "dcl";
	case OpCode::kPow: return "pow";
	case OpCode::kCrs: return "crs";
	case OpCode::kSgn: return "sgn";
	case OpCode::kAbs: return "abs";
	case OpCode::kNrm: return "nrm";
	case OpCode::kSincos: return "sincos";
	case OpCode::kRep: return "rep";
	case OpCode::kEndrep: return "endrep";
	case OpCode::kIf: return "if";
	case OpCode::kIfc: return "ifc";
	case OpCode::kElse: return "else";
	case OpCode::kEndif: return "endif";
	case OpCode::kBreak: return "break";
	case OpCode::kBreakc: return "breakc";
	case OpCode::kMova: return "mova";
	case OpCode::kDefb: return "defb";
	case OpCode::kDefi: return "defi";
	case OpCode::kTexcrd: return "texcrd";
	case OpCode::kTexkill: return "texkill";
	case OpCode::kTexld: return "texld";
	case OpCode::kTexbem: return "texbem";
	case OpCode::kTexbeml: return "texbeml";
	case OpCode::kTexreg2ar: return "texreg2ar";
	case OpCode::kTexreg2gb: return "texreg2gb";
	case OpCode::kTexm3x2pad: return "texm3x2pad";
	case OpCode::kTexm3x2tex: return "texm3x2tex";
	case OpCode::kTexm3x3pad: return "texm3x3pad";
	case OpCode::kTexm3x3tex: return "texm3x3tex";
	case OpCode::kTexm3x3spec: return "texm3x3spec";
	case OpCode::kTexm3x3vspec: return "texm3x3vspec";
	case OpCode::kExpp: return "expp";
	case OpCode::kLogp: return "logp";
	case OpCode::kCnd: return "cnd";
	case OpCode::kDef: return "def";
	case OpCode::kTexreg2rgb: return "texreg2rgb";
	case OpCode::kTexdp3tex: return "texdp3tex";
	case OpCode::kTexm3x2depth: return "texm3x2depth";
	case OpCode::kTexdp3: return "texdp3";
	case OpCode::kTexm3x3: return "texm3x3";
	case OpCode::kTexdepth: return "texdepth";
	case OpCode::kCmp: return "cmp";
	case OpCode::kBem: return "bem";
	case OpCode::kDp2add: return "dp2add";
	case OpCode::kDsx: return "dsx";
	case OpCode::kDsy: return "dsy";
	case OpCode::kTexldd: return "texldd";
	case OpCode::kSetp: return "setp";
	case OpCode::kTexldl: return "texldl";
	case OpCode::kBreakp: return "breakp";
	default:
		return "<unknown>";
	};
}

inline static Bool GetRegisterCounts(
	OpCode eOp,
	UInt32& ruDestination,
	UInt32& ruSource)
{
	switch (eOp)
	{
	case OpCode::kNop: ruDestination = 0u; ruSource = 0u; return true;
	case OpCode::kMov: ruDestination = 1u; ruSource = 1u; return true;
	case OpCode::kAdd: ruDestination = 1u; ruSource = 2u; return true;
	case OpCode::kSub: ruDestination = 1u; ruSource = 2u; return true;
	case OpCode::kMad: ruDestination = 1u; ruSource = 3u; return true;
	case OpCode::kMul: ruDestination = 1u; ruSource = 2u; return true;
	case OpCode::kRcp: ruDestination = 1u; ruSource = 1u; return true;
	case OpCode::kRsq: ruDestination = 1u; ruSource = 1u; return true;
	case OpCode::kDp3: ruDestination = 1u; ruSource = 2u; return true;
	case OpCode::kDp4: ruDestination = 1u; ruSource = 2u; return true;
	case OpCode::kMin: ruDestination = 1u; ruSource = 2u; return true;
	case OpCode::kMax: ruDestination = 1u; ruSource = 2u; return true;
	case OpCode::kSlt: ruDestination = 1u; ruSource = 2u; return true;
	case OpCode::kSge: ruDestination = 1u; ruSource = 2u; return true;
	case OpCode::kExp: ruDestination = 1u; ruSource = 1u; return true;
	case OpCode::kLog: ruDestination = 1u; ruSource = 1u; return true;
	case OpCode::kLit: ruDestination = 1u; ruSource = 1u; return true;
	case OpCode::kDst: ruDestination = 1u; ruSource = 2u; return true;
	case OpCode::kLrp: ruDestination = 1u; ruSource = 3u; return true;
	case OpCode::kFrc: ruDestination = 1u; ruSource = 1u; return true;
	case OpCode::kM4x4: ruDestination = 1u; ruSource = 2u; return true;
	case OpCode::kM4x3: ruDestination = 1u; ruSource = 2u; return true;
	case OpCode::kM3x4: ruDestination = 1u; ruSource = 2u; return true;
	case OpCode::kM3x3: ruDestination = 1u; ruSource = 2u; return true;
	case OpCode::kM3x2: ruDestination = 1u; ruSource = 2u; return true;
	case OpCode::kCall: ruDestination = 0u; ruSource = 1u; return true;
	case OpCode::kCallnz: ruDestination = 0u; ruSource = 2u; return true;
	case OpCode::kLoop: ruDestination = 0u; ruSource = 2u; return true;
	case OpCode::kRet: ruDestination = 0u; ruSource = 0u; return true;
	case OpCode::kEndloop: ruDestination = 0u; ruSource = 0u; return true;
	case OpCode::kLabel: ruDestination = 0u; ruSource = 1u; return true;
	case OpCode::kDcl: return false; // Requires special handling.
	case OpCode::kPow: ruDestination = 1u; ruSource = 2u; return true;
	case OpCode::kCrs: ruDestination = 1u; ruSource = 2u; return true;
	case OpCode::kSgn: ruDestination = 1u; ruSource = 3u; return true;
	case OpCode::kAbs: ruDestination = 1u; ruSource = 1u; return true;
	case OpCode::kNrm: ruDestination = 1u; ruSource = 1u; return true;
	case OpCode::kSincos: ruDestination = 1u; ruSource = 1u; return true;
	case OpCode::kRep: ruDestination = 0u; ruSource = 1u; return true;
	case OpCode::kEndrep: ruDestination = 0u; ruSource = 0u; return true;
	case OpCode::kIf: ruDestination = 0u; ruSource = 1u; return true;
	case OpCode::kIfc: ruDestination = 0u; ruSource = 2u; return true;
	case OpCode::kElse: ruDestination = 0u; ruSource = 0u; return true;
	case OpCode::kEndif: ruDestination = 0u; ruSource = 0u; return true;
	case OpCode::kBreak: ruDestination = 0u; ruSource = 0u; return true;
	case OpCode::kBreakc: ruDestination = 0u; ruSource = 2u; return true;
	case OpCode::kMova: ruDestination = 1u; ruSource = 1u; return true;
	case OpCode::kDefb: return false; // Requires special handling.
	case OpCode::kDefi: return false; // Requires special handling.
	case OpCode::kTexcrd: ruDestination = 1u; ruSource = 1u; return true;
	case OpCode::kTexkill: ruDestination = 1u; ruSource = 0u; return true;
	case OpCode::kTexld: ruDestination = 1u; ruSource = 2u; return true;
	case OpCode::kTexbem: ruDestination = 1u; ruSource = 1u; return true;
	case OpCode::kTexbeml: ruDestination = 1u; ruSource = 1u; return true;
	case OpCode::kTexreg2ar: ruDestination = 1u; ruSource = 1u; return true;
	case OpCode::kTexreg2gb: ruDestination = 1u; ruSource = 1u; return true;
	case OpCode::kTexm3x2pad: ruDestination = 1u; ruSource = 1u; return true;
	case OpCode::kTexm3x2tex: ruDestination = 1u; ruSource = 1u; return true;
	case OpCode::kTexm3x3pad: ruDestination = 1u; ruSource = 1u; return true;
	case OpCode::kTexm3x3tex: ruDestination = 1u; ruSource = 1u; return true;
	case OpCode::kTexm3x3spec: ruDestination = 1u; ruSource = 2u; return true;
	case OpCode::kTexm3x3vspec: ruDestination = 1u; ruSource = 1u; return true;
	case OpCode::kExpp: ruDestination = 1u; ruSource = 1u; return true;
	case OpCode::kLogp: ruDestination = 1u; ruSource = 1u; return true;
	case OpCode::kCnd: ruDestination = 1u; ruSource = 3u; return true;
	case OpCode::kDef: return false; // Requires special handling.
	case OpCode::kTexreg2rgb: ruDestination = 1u; ruSource = 1u; return true;
	case OpCode::kTexdp3tex: ruDestination = 1u; ruSource = 1u; return true;
	case OpCode::kTexm3x2depth: ruDestination = 1u; ruSource = 1u; return true;
	case OpCode::kTexdp3: ruDestination = 1u; ruSource = 1u; return true;
	case OpCode::kTexm3x3: ruDestination = 1u; ruSource = 1u; return true;
	case OpCode::kTexdepth: ruDestination = 1u; ruSource = 0u; return true;
	case OpCode::kCmp: ruDestination = 1u; ruSource = 3u; return true;
	case OpCode::kBem: ruDestination = 1u; ruSource = 2u; return true;
	case OpCode::kDp2add: ruDestination = 1u; ruSource = 3u; return true;
	case OpCode::kDsx: ruDestination = 1u; ruSource = 1u; return true;
	case OpCode::kDsy: ruDestination = 1u; ruSource = 1u; return true;
	case OpCode::kTexldd: ruDestination = 1u; ruSource = 4u; return true;
	case OpCode::kSetp: ruDestination = 1u; ruSource = 2u; return true;
	case OpCode::kTexldl: ruDestination = 1u; ruSource = 2u; return true;
	case OpCode::kBreakp: ruDestination = 0u; ruSource = 1u; return true;
	default:
		return false;
	};
}

} // namespace Seoul::EffectConverter

#endif // include guard
