/**
 * \file FalconSwfReader.h
 * \brief A structure similar to Seoul::StreamBuffer, specialized
 * for reading SWF data contained in a Falcon FCN file.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef	FALCON_SWF_READER_H
#define FALCON_SWF_READER_H

#include "FalconLabelName.h"
#include "FalconTypes.h"
#include "SeoulHString.h"
#include "SeoulString.h"

namespace Seoul::Falcon
{

// TODO: SwfReader needs to be refactored to report read failures. Options:
// - use a longjmp.
// - update all Read() functions to return a success/fail boolean.
// - set a read failure flag, and depend on the enclosing environment to check it occasionally.

class SwfReader SEOUL_SEALED
{
public:
	typedef UInt32 SizeType;

	SwfReader(UInt8 const* pData, SizeType zSizeInBytes)
		: m_pBuffer(pData)
		, m_zSizeInBytes(zSizeInBytes)
		, m_zOffsetInBits(0)
	{
	}

	void Align()
	{
		m_zOffsetInBits = (0 == (m_zOffsetInBits & 0x7)) ? m_zOffsetInBits : ((m_zOffsetInBits & (~(0x7))) + 8);
	}

	SizeType GetOffsetInBits() const
	{
		return m_zOffsetInBits;
	}

	SizeType GetOffsetInBytes() const
	{
		return (SizeType)(m_zOffsetInBits >> 3);
	}

	SizeType GetSizeInBytes() const
	{
		return m_zSizeInBytes;
	}

	Bool ReadBit()
	{
		SEOUL_ASSERT(GetOffsetInBytes() < m_zSizeInBytes);
		int b = (int)m_pBuffer[GetOffsetInBytes()];

		Bool bReturn = (0 != (b & (1 << (7 - (m_zOffsetInBits - (GetOffsetInBytes() * 8))))));
		m_zOffsetInBits++;
		return bReturn;
	}

	ColorTransform ReadColorTransform()
	{
		ColorTransform ret;

		Align();

		Bool bHasAdd = ReadBit();
		Bool bHasMul = ReadBit();

		int nBits = ReadUBits(4);
		if (bHasMul)
		{
			ret.m_fMulR = ReadFBits88(nBits).GetFloatValue();
			ret.m_fMulG = ReadFBits88(nBits).GetFloatValue();
			ret.m_fMulB = ReadFBits88(nBits).GetFloatValue();
		}
		else
		{
			ret.m_fMulR = 1.0f;
			ret.m_fMulG = 1.0f;
			ret.m_fMulB = 1.0f;
		}

		if (bHasAdd)
		{
			ret.m_AddR = ReadSBits(nBits);
			ret.m_AddG = ReadSBits(nBits);
			ret.m_AddB = ReadSBits(nBits);
		}
		else
		{
			ret.m_AddR = 0;
			ret.m_AddG = 0;
			ret.m_AddB = 0;
		}

		return ret;
	}

	ColorTransformWithAlpha ReadColorTransformWithAlpha()
	{
		ColorTransformWithAlpha ret;

		Align();

		Bool bHasAdd = ReadBit();
		Bool bHasMul = ReadBit();

		int nBits = ReadUBits(4);
		if (bHasMul)
		{
			ret.m_fMulR = ReadFBits88(nBits).GetFloatValue();
			ret.m_fMulG = ReadFBits88(nBits).GetFloatValue();
			ret.m_fMulB = ReadFBits88(nBits).GetFloatValue();
			ret.m_fMulA = ReadFBits88(nBits).GetFloatValue();
		}
		else
		{
			ret.m_fMulR = 1.0f;
			ret.m_fMulG = 1.0f;
			ret.m_fMulB = 1.0f;
			ret.m_fMulA = 1.0f;
		}

		if (bHasAdd)
		{
			ret.m_AddR = ReadSBits(nBits);
			ret.m_AddG = ReadSBits(nBits);
			ret.m_AddB = ReadSBits(nBits);
			ret.m_uBlendingFactor = 0; // 0 indicates normal blending by default.

			// TODO: Additive alpha cannot be implemented
			// (as far as I can figure) without a lot of shader math
			// and a divide to "unpremultiply" color values, which
			// is a lot of work for a weird case. We're currently
			// using this storage fo "blend factor", which is used
			// to select between "normal" and "additive" blending.
			(void)ReadSBits(nBits); // ret.m_AddA =
		}
		else
		{
			ret.m_AddR = 0;
			ret.m_AddG = 0;
			ret.m_AddB = 0;
			ret.m_uBlendingFactor = 0; // 0 indicates normal blending by default.
		}

		return ret;
	}

	Fixed88 ReadFBits88(int nBits)
	{
		Int32 iValue = ReadSBits(nBits);
		Fixed88 ret;
		ret.m_iValue = (Int16)iValue;
		return ret;
	}

	Fixed1616 ReadFBits1616(int nBits)
	{
		Int32 iValue = ReadSBits(nBits);
		Fixed1616 ret;
		ret.m_iValue = iValue;
		return ret;
	}

	Fixed88 ReadFixed88()
	{
		Fixed88 ret;
		ret.m_iMinor = ReadInt8();
		ret.m_iMajor = ReadInt8();
		return ret;
	}

	Fixed1616 ReadFixed1616()
	{
		Fixed1616 ret;
		ret.m_iMinor = ReadInt16();
		ret.m_iMajor = ReadInt16();
		return ret;
	}

	double ReadFloat64()
	{
		Align();

		SEOUL_STATIC_ASSERT(sizeof(double) == 8);

		SEOUL_ASSERT(GetOffsetInBytes() + 8 <= m_zSizeInBytes);

		double fReturn = 0.0;
		memcpy(&fReturn, m_pBuffer + GetOffsetInBytes(), sizeof(double));
		m_zOffsetInBits += (8 * sizeof(double));

		return fReturn;
	}

	Int8 ReadInt8()
	{
		return (Int8)ReadUInt8();
	}

	Int16 ReadInt16()
	{
		return (Int16)ReadUInt16();
	}

	Int32 ReadInt32()
	{
		return (Int32)ReadUInt32();
	}

	Matrix2x3 ReadMatrix()
	{
		Matrix2x3 ret;

		Align();

		Bool bHasScale = ReadBit();
		if (bHasScale)
		{
			int nScaleBits = ReadUBits(5);
			ret.M00 = (Float)ReadFBits1616(nScaleBits).GetDoubleValue();
			ret.M11 = (Float)ReadFBits1616(nScaleBits).GetDoubleValue();
		}
		else
		{
			ret.M00 = 1.0f;
			ret.M11 = 1.0f;
		}

		Bool bHasRotate = ReadBit();
		if (bHasRotate)
		{
			int nRotateBits = ReadUBits(5);
			ret.M10 = (Float)ReadFBits1616(nRotateBits).GetDoubleValue();
			ret.M01 = (Float)ReadFBits1616(nRotateBits).GetDoubleValue();
		}
		else
		{
			ret.M10 = 0.0f;
			ret.M01 = 0.0f;
		}

		int nTranslateBits = ReadUBits(5);
		ret.TX = (Float)ReadSBits(nTranslateBits);
		ret.TY = (Float)ReadSBits(nTranslateBits);

		return ret;
	}

	Int32 ReadSBits(int nBits)
	{
		if (0 == nBits) { return 0; }
	
		int iShiftBy = (32 - nBits);
		int iUnsigned = ReadUBits(nBits);
		int iIntermediate = (iUnsigned << iShiftBy); // shift the result so the sign is the highest bit.
		int iResult = (iIntermediate >> iShiftBy); // shift back, arithmetic shift.

		return iResult;
	}

	String ReadSizedString()
	{
		// Length includes the null terminator.
		int nLength = (int)ReadUInt8();
		SEOUL_ASSERT(nLength >= 1);

		if (nLength == 1)
		{
			SetOffsetInBytes(GetOffsetInBytes() + nLength);
			return String();
		}
		else
		{
			SEOUL_ASSERT(GetOffsetInBytes() + nLength <= m_zSizeInBytes);
			String sReturn((const char*)(m_pBuffer + GetOffsetInBytes()), (nLength - 1));
			SetOffsetInBytes(GetOffsetInBytes() + nLength);
			return sReturn;
		}
	}

	HString ReadSizedHString()
	{
		// Length includes the null terminator.
		int nLength = (int)ReadUInt8();
		SEOUL_ASSERT(nLength >= 1);

		if (nLength < 1)
		{
			return HString();
		}

		if (nLength == 1)
		{
			SetOffsetInBytes(GetOffsetInBytes() + nLength);
			return HString();
		}
		else
		{
			SEOUL_ASSERT(GetOffsetInBytes() + nLength <= m_zSizeInBytes);
			HString sReturn((Byte const*)(m_pBuffer + GetOffsetInBytes()), (UInt32)(nLength - 1));
			SetOffsetInBytes(GetOffsetInBytes() + nLength);
			return sReturn;
		}
	}

	String ReadString()
	{
		Align();

		SizeType zStartingOffsetInBytes = GetOffsetInBytes();

		UInt8 c = ReadUInt8();
		while (c != 0)
		{
			c = ReadUInt8();
		}

		String sReturn((const char*)(m_pBuffer + zStartingOffsetInBytes), (GetOffsetInBytes() - zStartingOffsetInBytes - 1));
		return sReturn;
	}

	LabelName ReadFrameLabel()
	{
		Align();

		SizeType zStartingOffsetInBytes = GetOffsetInBytes();

		UInt8 c = ReadUInt8();
		while (c != 0)
		{
			c = ReadUInt8();
		}

		LabelName sReturn((const char*)(m_pBuffer + zStartingOffsetInBytes), (GetOffsetInBytes() - zStartingOffsetInBytes - 1));
		return sReturn;
	}

	HString ReadHString()
	{
		Align();

		SizeType zStartingOffsetInBytes = GetOffsetInBytes();

		UInt8 c = ReadUInt8();
		while (c != 0)
		{
			c = ReadUInt8();
		}

		HString sReturn((const char*)(m_pBuffer + zStartingOffsetInBytes), (GetOffsetInBytes() - zStartingOffsetInBytes - 1));
		return sReturn;
	}

	UInt32 ReadUBits(Int32 nBits)
	{
		UInt32 v = 0;

		while (nBits > 0)
		{
			v |= (ReadBit() ? 1 : 0) << ((UInt32)(nBits - 1));
			--nBits;
		}

		return v;
	}

	UInt8 ReadUInt8()
	{
		Align();

		SEOUL_ASSERT(GetOffsetInBytes() < m_zSizeInBytes);
		UInt8 ret = m_pBuffer[GetOffsetInBytes()];
		m_zOffsetInBits += 8;
		return ret;
	}

	UInt16 ReadUInt16()
	{
		Align();

		SwfReader::SizeType nOffsetInBytes = (GetOffsetInBytes());
		SEOUL_ASSERT(GetOffsetInBytes() + 1 < m_zSizeInBytes);
		UInt16 ret = (UInt16)((m_pBuffer[nOffsetInBytes + 1] << 8) | (m_pBuffer[nOffsetInBytes]));
		m_zOffsetInBits += 16;

		return ret;
	}

	UInt32 ReadEncodedUInt32()
	{
		Align();

		SEOUL_ASSERT(GetOffsetInBytes() < m_zSizeInBytes);
		UInt32 uReturn = m_pBuffer[GetOffsetInBytes()];
		SetOffsetInBytes(GetOffsetInBytes() + 1);
		if (0 == (uReturn & 0x00000080))
		{
			return uReturn;
		}

		SEOUL_ASSERT(GetOffsetInBytes() < m_zSizeInBytes);
		uReturn = (uReturn & 0x0000007F) | ((UInt32)m_pBuffer[GetOffsetInBytes()] << 7);
		SetOffsetInBytes(GetOffsetInBytes() + 1);
		if (0 == (uReturn & 0x00004000))
		{
			return uReturn;
		}

		SEOUL_ASSERT(GetOffsetInBytes() < m_zSizeInBytes);
		uReturn = (uReturn & 0x00003FFF) | ((UInt32)m_pBuffer[GetOffsetInBytes()] << 14);
		SetOffsetInBytes(GetOffsetInBytes() + 1);
		if (0 == (uReturn & 0x00200000))
		{
			return uReturn;
		}

		SEOUL_ASSERT(GetOffsetInBytes() < m_zSizeInBytes);
		uReturn = (uReturn & 0x001FFFFF) | ((UInt32)m_pBuffer[GetOffsetInBytes()] << 21);
		SetOffsetInBytes(GetOffsetInBytes() + 1);
		if (0 == (uReturn & 0x10000000))
		{
			return uReturn;
		}

		SEOUL_ASSERT(GetOffsetInBytes() < m_zSizeInBytes);
		uReturn = (uReturn & 0x0FFFFFFF) | ((UInt32)m_pBuffer[GetOffsetInBytes()] << 28);
		SetOffsetInBytes(GetOffsetInBytes() + 1);
		return uReturn;
	}

	UInt32 ReadUInt32()
	{
		Align();

		SwfReader::SizeType nOffsetInBytes = (GetOffsetInBytes());

		SEOUL_ASSERT(GetOffsetInBytes() + 3 < m_zSizeInBytes);
		UInt32 ret = (UInt32)(
			(m_pBuffer[nOffsetInBytes + 3] << 24) |
			(m_pBuffer[nOffsetInBytes + 2] << 16) |
			(m_pBuffer[nOffsetInBytes + 1] << 8) |
			(m_pBuffer[nOffsetInBytes + 0]));
		m_zOffsetInBits += 32;

		return ret;
	}

	Rectangle ReadRectangle()
	{
		Align();

		int nBits = ReadUBits(5);
		Rectangle ret;
		ret.m_fLeft = TwipsToPixels(ReadSBits(nBits));
		ret.m_fRight = TwipsToPixels(ReadSBits(nBits));
		ret.m_fTop = TwipsToPixels(ReadSBits(nBits));
		ret.m_fBottom = TwipsToPixels(ReadSBits(nBits));
		return ret;
	}

	RGBA ReadRGB()
	{
		RGBA ret;
		ret.m_R = ReadUInt8();
		ret.m_G = ReadUInt8();
		ret.m_B = ReadUInt8();
		ret.m_A = 255;

		return ret;
	}

	RGBA ReadRGBA()
	{
		RGBA ret = ReadRGB();
		ret.m_A = ReadUInt8();
		return ret;
	}

	void SetOffsetInBits(SizeType zOffsetInBits)
	{
		m_zOffsetInBits = zOffsetInBits;
	}

	void SetOffsetInBytes(SizeType zOffsetInBytes)
	{
		m_zOffsetInBits = (SizeType)((zOffsetInBytes << 3) & (~(0x7)));
	}

private:
	UInt8 const* m_pBuffer;
	SizeType m_zSizeInBytes;
	SizeType m_zOffsetInBits;
}; // class SwfReader

} // namespace Seoul::Falcon

#endif // include guard
