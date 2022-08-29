/**
 * \file ShaderReceiverGLSLES2.h
 * \brief Receiver that converts output from EffectConverter::Shader to
 * OpenGL GLSL ES 2.0 compatible source.
 *
 * \sa https://www.khronos.org/registry/gles/specs/2.0/GLSL_ES_Specification_1.0.17.pdf
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SHADER_RECEIVER_GLSLES2_H
#define SHADER_RECEIVER_GLSLES2_H

#include "HashTable.h"
#include "FixedArray.h"
#include "EffectConverter.h"
#include "HashFunctions.h"
#include "IShaderReceiver.h"
#include "Prereqs.h"
#include "SeoulString.h"
#include "Vector.h"

namespace Seoul
{

using namespace EffectConverter;

typedef HashTable<HString, Int32> ConstantRegisterLookupTable;

struct RegisterEntry
{
	RegisterEntry()
		: m_eType(RegisterType::kTemp)
		, m_uNumber(0u)
	{
	}

	RegisterEntry(RegisterType eType, UInt32 uNumber)
		: m_eType(eType)
		, m_uNumber(uNumber)
	{
	}

	Bool operator==(const RegisterEntry& entry) const
	{
		return (
			m_eType == entry.m_eType &&
			m_uNumber == entry.m_uNumber);
	}

	Bool operator!=(const RegisterEntry& entry) const
	{
		return !(*this == entry);
	}

	RegisterType m_eType;
	UInt32 m_uNumber;
};

template <>
struct DefaultHashTableKeyTraits<RegisterEntry>
{
	inline static Float GetLoadFactor()
	{
		return 0.75f;
	}

	inline static RegisterEntry GetNullKey()
	{
		return RegisterEntry((RegisterType)UIntMax, UIntMax);
	}

	static const Bool kCheckHashBeforeEquals = false;
};

inline UInt32 GetHash(const RegisterEntry& entry)
{
	UInt32 uHash = 0u;
	IncrementalHash(uHash, GetHash((UInt32)entry.m_eType));
	IncrementalHash(uHash, GetHash(entry.m_uNumber));
	return uHash;
}

class ShaderReceiverGLSLES2 SEOUL_SEALED : public IShaderReceiver
{
public:
	ShaderReceiverGLSLES2(const Converter& effectConverter);
	~ShaderReceiverGLSLES2();

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

	void Clear()
	{
		m_aConstantRegisters.Fill(ConstantEntry());
		m_tConstantRegisterLookupTable.Clear();
		m_tRegisterTable.Clear();
		m_aSamplers.Fill(SamplerType::kUnknown);
		m_eShaderType = ShaderType::kPixel;
		m_vHeaderCode.Clear();
		m_vMainCode.Clear();
	}

	String GetCode() const
	{
		String sReturn;

		for (UInt32 i = 0u; i < m_vHeaderCode.GetSize(); ++i)
		{
			sReturn.Append(m_vHeaderCode[i]);
			sReturn.Append('\n');
		}

		sReturn.Append("void main()\n{\n");

		for (UInt32 i = 0u; i < m_vMainCode.GetSize(); ++i)
		{
			sReturn.Append('\t');
			sReturn.Append(m_vMainCode[i]);
			sReturn.Append('\n');
		}

		sReturn.Append('}');
		sReturn.Append('\n');

		return sReturn;
	}

	const ConstantRegisterLookupTable& GetConstantRegisterLookupTable() const
	{
		return m_tConstantRegisterLookupTable;
	}

private:
	enum Flag
	{
		kFlagNone = 0,
		kFlagDerivatives = (1 << 0),
	};

	const Converter& m_rEffectConverter;

	struct NameAndOffset
	{
		NameAndOffset()
			: m_Name()
			, m_iOffset(-1)
		{
		}

		Bool IsDefined() const
		{
			return !m_Name.IsEmpty();
		}

		void Reset()
		{
			m_Name = HString();
			m_iOffset = -1;
		}

		String ToString() const
		{
			String sReturn(m_Name);
			if (m_iOffset >= 0)
			{
				sReturn.Append('[');
				sReturn.Append(String::Printf("%d", m_iOffset));
				sReturn.Append(']');
			}
			return sReturn;
		}

		HString m_Name;
		Int32 m_iOffset;
	};

	struct ConstantEntry
	{
		ConstantEntry()
			: m_NameAndOffset()
			, m_uColsCount(0u)
		{
		}

		NameAndOffset m_NameAndOffset;
		UInt16 m_uColsCount;
	}; // struct ConstantEntry
	typedef FixedArray<ConstantEntry, 1024u> ConstantRegisters;
	ConstantRegisters m_aConstantRegisters;

	ConstantRegisterLookupTable m_tConstantRegisterLookupTable;

	typedef HashTable<RegisterEntry, HString> RegisterTable;
	RegisterTable m_tRegisterTable;

	Bool CanSwizzle(RegisterType eRegisterType, UInt32 uRegisterNumber);
	Bool RegisterTypeToString(NameAndOffset& rOut, RegisterType eRegisterType, UInt32 uRegisterNumber);
	Bool ToString(String& rs, DestinationRegister reg, Bool bIgnoreWriteMask = false);
	Bool ToString(String& rs, DestinationRegister reg, UInt32 uComponent);
	Bool ToString(String& rs, SourceRegister reg, Bool bX, Bool bY, Bool bZ, Bool bW);

	Bool ComponentToString(String& rs, SourceRegister reg, UInt32 uComponent);

	typedef FixedArray<SamplerType, 32u> Samplers;
	Samplers m_aSamplers;

	ShaderType m_eShaderType;

	Vector<String> m_vHeaderCode;
	Vector<String> m_vMainCode;
	UInt32 m_uFlags;

	SEOUL_DISABLE_COPY(ShaderReceiverGLSLES2);
}; // class ShaderReceiverGLSLES2

} // namespace Seoul

#endif // include guard
