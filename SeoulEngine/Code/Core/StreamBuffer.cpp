/**
 * \file StreamBuffer.cpp
 * \brief In memory buffer with file IO semantics. Useful for
 * preparing a byte array for serialization or deserializing an
 * entire file in one operation, and then reading individual fields
 * out from memory.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "SeoulFile.h"
#include "SeoulFileReaders.h"
#include "StreamBuffer.h"

namespace Seoul
{

Bool StreamBuffer::Load(SyncFile& file)
{
	const SizeType zSize = (SizeType)file.GetSize();
	if (zSize > kDefaultMaxReadSize)
	{
		return false;
	}
	else
	{
		m_zOffsetInBytes = 0u;
		InternalSetCapacity(zSize);
		m_zSizeInBytes = file.ReadRawData(m_pBuffer.Get(), zSize);
		return (zSize == m_zSizeInBytes);
	}
}

Bool StreamBuffer::Save(SyncFile& file) const
{
	if (0u == m_zSizeInBytes)
	{
		// Nothing to write
		return true;
	}
	else
	{
		return (file.WriteRawData(m_pBuffer.Get(), m_zSizeInBytes) == m_zSizeInBytes);
	}
}

Bool StreamBuffer::Read(String& sOutValue)
{
	UInt32 uSizeInBytes = 0;
	if (!Read<UInt32>(uSizeInBytes))
	{
		return false;
	}

	if (uSizeInBytes == 0)
	{
		sOutValue.Clear();
		return true;
	}
	else if (m_zOffsetInBytes + uSizeInBytes <= m_zSizeInBytes)
	{
		sOutValue.Assign(InternalGetHeadPointer(), uSizeInBytes);
		m_zOffsetInBytes += uSizeInBytes;
		return true;
	}
	else
	{
		return false;
	}
}

void StreamBuffer::Write(const String& sValue)
{
	UInt uSizeInBytes = sValue.GetSize();

	Write<UInt32>(uSizeInBytes);
	Write(sValue.CStr(), uSizeInBytes);
}

/**
 * Reads a little-endian 16-bit signed integer, endian swapping if necessary
 */
Bool StreamBuffer::ReadLittleEndian16(Int16& value)
{
	return ReadLittleEndian16((UInt16&)value);
}

/**
 * Reads a little-endian 16-bit unsigned integer, endian swapping if necessary
 */
Bool StreamBuffer::ReadLittleEndian16(UInt16& value)
{
	Bool bRet = Read(value);
#if SEOUL_BIG_ENDIAN
	if (bRet)
	{
		value = EndianSwap16(value);
	}
#endif

	return bRet;
}

/**
 * Reads a little-endian 32-bit float, endian swapping if necessary
 */
Bool StreamBuffer::ReadLittleEndian32(Float& value)
{
	union
	{
		Float32 out;
		UInt32 in;
	};

	if (ReadLittleEndian32(in))
	{
		value = out;
		return true;
	}

	return false;
}

/**
 * Reads a little-endian 32-bit signed integer, endian swapping if necessary
 */
Bool StreamBuffer::ReadLittleEndian32(Int32& value)
{
	return ReadLittleEndian32((UInt32&)value);
}

/**
 * Reads a little-endian 32-bit unsigned integer, endian swapping if necessary
 */
Bool StreamBuffer::ReadLittleEndian32(UInt32& value)
{
	Bool bRet = Read(value);
#if SEOUL_BIG_ENDIAN
	if (bRet)
	{
		value = EndianSwap32(value);
	}
#endif

	return bRet;
}

/**
 * Reads a little-endian 64-bit float, endian swapping if necessary
 */
Bool StreamBuffer::ReadLittleEndian64(Float64& value)
{
	union
	{
		Float64 out;
		UInt64 in;
	};

	if (ReadLittleEndian64(in))
	{
		value = out;
		return true;
	}

	return false;
}

/**
 * Reads a little-endian 64-bit signed integer, endian swapping if necessary
 */
Bool StreamBuffer::ReadLittleEndian64(Int64& value)
{
	return ReadLittleEndian64((UInt64&)value);
}

/**
 * Reads a little-endian 64-bit unsigned integer, endian swapping if necessary
 */
Bool StreamBuffer::ReadLittleEndian64(UInt64& value)
{
	Bool bRet = Read(value);
#if SEOUL_BIG_ENDIAN
	if (bRet)
	{
		value = EndianSwap64(value);
	}
#endif

	return bRet;
}

/**
 * Reads a big-endian 16-bit signed integer, endian swapping if necessary
 */
Bool StreamBuffer::ReadBigEndian16(Int16& value)
{
	return ReadBigEndian16((UInt16&)value);
}

/**
 * Reads a big-endian 16-bit unsigned integer, endian swapping if necessary
 */
Bool StreamBuffer::ReadBigEndian16(UInt16& value)
{
	Bool bRet = Read(value);
#if SEOUL_LITTLE_ENDIAN
	if (bRet)
	{
		value = EndianSwap16(value);
	}
#endif

	return bRet;
}

/**
 * Reads a big-endian 32-bit float, endian swapping if necessary
 */
Bool StreamBuffer::ReadBigEndian32(Float& value)
{
	union
	{
		Float32 out;
		UInt32 in;
	};

	if (ReadBigEndian32(in))
	{
		value = out;
		return true;
	}

	return false;
}

/**
 * Reads a big-endian 32-bit signed integer, endian swapping if necessary
 */
Bool StreamBuffer::ReadBigEndian32(Int32& value)
{
	return ReadBigEndian32((UInt32&)value);
}

/**
 * Reads a big-endian 32-bit unsigned integer, endian swapping if necessary
 */
Bool StreamBuffer::ReadBigEndian32(UInt32& value)
{
	Bool bRet = Read(value);
#if SEOUL_LITTLE_ENDIAN
	if (bRet)
	{
		value = EndianSwap32(value);
	}
#endif

	return bRet;
}

/**
 * Reads a big-endian 64-bit float, endian swapping if necessary
 */
Bool StreamBuffer::ReadBigEndian64(Float64& value)
{
	union
	{
		Float64 out;
		UInt64 in;
	};

	if (ReadBigEndian64(in))
	{
		value = out;
		return true;
	}

	return false;
}

/**
 * Reads a big-endian 64-bit signed integer, endian swapping if necessary
 */
Bool StreamBuffer::ReadBigEndian64(Int64& value)
{
	return ReadBigEndian64((UInt64&)value);
}

/**
 * Reads a big-endian 64-bit unsigned integer, endian swapping if necessary
 */
Bool StreamBuffer::ReadBigEndian64(UInt64& value)
{
	Bool bRet = Read(value);
#if SEOUL_LITTLE_ENDIAN
	if (bRet)
	{
		value = EndianSwap64(value);
	}
#endif

	return bRet;
}

/**
 * Writes a little-endian 16-bit signed integer, endian swapping if necessary
 */
void StreamBuffer::WriteLittleEndian16(Int16 value)
{
	return WriteLittleEndian16((UInt16)value);
}

/**
 * Writes a little-endian 16-bit unsigned integer, endian swapping if necessary
 */
void StreamBuffer::WriteLittleEndian16(UInt16 value)
{
#if SEOUL_BIG_ENDIAN
	value = EndianSwap16(value);
#endif
	Write(value);
}

/**
 * Writes a little-endian 32-bit float, endian swapping if necessary
 */
void StreamBuffer::WriteLittleEndian32(Float32 value)
{
	union
	{
		Float32 in;
		UInt32 out;
	};

	in = value;
	return WriteLittleEndian32(out);
}

/**
 * Writes a little-endian 32-bit signed integer, endian swapping if necessary
 */
void StreamBuffer::WriteLittleEndian32(Int32 value)
{
	return WriteLittleEndian32((UInt32)value);
}

/**
 * Writes a little-endian 32-bit unsigned integer, endian swapping if necessary
 */
void StreamBuffer::WriteLittleEndian32(UInt32 value)
{
#if SEOUL_BIG_ENDIAN
	value = EndianSwap32(value);
#endif
	Write(value);
}

/**
 * Writes a little-endian 64-bit float, endian swapping if necessary
 */
void StreamBuffer::WriteLittleEndian64(Float64 value)
{
	union
	{
		Float64 in;
		UInt64 out;
	};

	in = value;
	return WriteLittleEndian64(out);
}

/**
 * Writes a little-endian 64-bit signed integer, endian swapping if necessary
 */
void StreamBuffer::WriteLittleEndian64(Int64 value)
{
	return WriteLittleEndian64((UInt64)value);
}

/**
 * Writes a little-endian 64-bit unsigned integer, endian swapping if necessary
 */
void StreamBuffer::WriteLittleEndian64(UInt64 value)
{
#if SEOUL_BIG_ENDIAN
	value = EndianSwap64(value);
#endif
	Write(value);
}

/**
 * Writes a big-endian 16-bit signed integer, endian swapping if necessary
 */
void StreamBuffer::WriteBigEndian16(Int16 value)
{
	return WriteBigEndian16((UInt16)value);
}

/**
 * Writes a big-endian 16-bit unsigned integer, endian swapping if necessary
 */
void StreamBuffer::WriteBigEndian16(UInt16 value)
{
#if SEOUL_LITTLE_ENDIAN
	value = EndianSwap16(value);
#endif
	Write(value);
}

/**
 * Writes a big-endian 32-bit float, endian swapping if necessary
 */
void StreamBuffer::WriteBigEndian32(Float32 value)
{
	union
	{
		Float32 in;
		UInt32 out;
	};

	in = value;
	return WriteBigEndian32(out);
}

/**
 * Writes a big-endian 32-bit signed integer, endian swapping if necessary
 */
void StreamBuffer::WriteBigEndian32(Int32 value)
{
	return WriteBigEndian32((UInt32)value);
}

/**
 * Writes a big-endian 32-bit unsigned integer, endian swapping if necessary
 */
void StreamBuffer::WriteBigEndian32(UInt32 value)
{
#if SEOUL_LITTLE_ENDIAN
	value = EndianSwap32(value);
#endif
	Write(value);
}

/**
 * Writes a big-endian 64-bit float, endian swapping if necessary
 */
void StreamBuffer::WriteBigEndian64(Float64 value)
{
	union
	{
		Float64 in;
		UInt64 out;
	};

	in = value;
	return WriteBigEndian64(out);
}

/**
 * Writes a big-endian 64-bit signed integer, endian swapping if necessary
 */
void StreamBuffer::WriteBigEndian64(Int64 value)
{
	return WriteBigEndian64((UInt64)value);
}

/**
 * Writes a big-endian 64-bit unsigned integer, endian swapping if necessary
 */
void StreamBuffer::WriteBigEndian64(UInt64 value)
{
#if SEOUL_LITTLE_ENDIAN
	value = EndianSwap64(value);
#endif
	Write(value);
}

/**
 * Pad this StreamBuffer to zSizeInBytes. Is a nop
 * if the streambuffer is already that big or larger. The
 * pad area will be set to 0s.
 */
void StreamBuffer::PadTo(SizeType zSizeInBytes, Bool bInitializeToZero /* = true */)
{
	if (zSizeInBytes > m_zOffsetInBytes)
	{
		SizeType const zToPadInBytes = (zSizeInBytes - m_zOffsetInBytes);
		InternalGrowToAtLeast(zSizeInBytes);

		if (bInitializeToZero)
		{
			memset(InternalGetHeadPointer(), 0, zToPadInBytes);
		}

		m_zOffsetInBytes += zToPadInBytes;
		m_zSizeInBytes = Max(zSizeInBytes, m_zSizeInBytes);
	}
}

} // namespace Seoul
