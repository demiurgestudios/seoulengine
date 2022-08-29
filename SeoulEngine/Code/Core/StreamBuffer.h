/**
 * \file StreamBuffer.h
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

#pragma once
#ifndef	STREAM_BUFFER_H
#define STREAM_BUFFER_H

#include "Prereqs.h"
#include "SeoulMath.h"
#include "SeoulString.h"
#include "SeoulTypes.h"
#include "Vector.h"

#include <string.h>

namespace Seoul
{

// Forward declarations
class SyncFile;

/**
 * StreamBuffer class
 *
 * StreamBuffer is a byte array that allows reading and writing in stream format
 */
class StreamBuffer SEOUL_SEALED
{
public:
	typedef Vector<Byte> ByteBuffer;
	typedef ByteBuffer::SizeType SizeType;

	StreamBuffer(MemoryBudgets::Type eType = MemoryBudgets::TBDContainer)
		: m_pBuffer(nullptr)
		, m_zCapacityInBytes(0u)
		, m_zSizeInBytes(0u)
		, m_zOffsetInBytes(0u)
		, m_eType(eType)
	{
	}

	StreamBuffer(UInt32 zInitialCapacity, MemoryBudgets::Type eType = MemoryBudgets::TBDContainer)
		: m_pBuffer(nullptr)
		, m_zCapacityInBytes(0u)
		, m_zSizeInBytes(0u)
		, m_zOffsetInBytes(0u)
		, m_eType(eType)
	{
		InternalSetCapacity(zInitialCapacity);
	}

	~StreamBuffer()
	{
		MemoryManager::Deallocate(m_pBuffer);
	}

	// Load the contents of file into this StreamBuffer. Completely
	// overwrites the contents of this StreamBuffer and sets the
	// head pointer to the beginning of the data.
	Bool Load(SyncFile& file);

	// Save the entire contents of this StreamBuffer to file. Ignores
	// the head pointer - all data in the entire StreamBuffer is saved.
	Bool Save(SyncFile& file) const;

	/**
	 * @return True if this StreamBuffer has more data that can
	 * be read, false otherwise.
	 */
	Bool HasMoreData() const
	{
		return (m_zOffsetInBytes < m_zSizeInBytes);
	}

	/**
	 * Erases all data from this StreamBuffer and sets the head
	 * pointer to the beginning of the data buffer.
	 */
	void Clear()
	{
		m_zSizeInBytes = 0u;
		m_zOffsetInBytes = 0u;
	}

	/**
	 * Populate this StreamBuffer to be an exact copy of the contents of buffer,
	 * while maintaining its existing MemoryBudgets association.
	 */
	void CopyFrom(const StreamBuffer& buffer)
	{
		// Setup a copy.
		StreamBuffer copy;
		copy.m_eType = m_eType;
		copy.InternalSetCapacity(buffer.m_zCapacityInBytes);
		copy.m_zOffsetInBytes = buffer.m_zOffsetInBytes;
		copy.m_zSizeInBytes = buffer.m_zSizeInBytes;

		if (copy.m_zSizeInBytes > 0u)
		{
			memcpy(copy.m_pBuffer, buffer.m_pBuffer, copy.m_zSizeInBytes);
		}

		// Swap in the copy for this.
		Swap(copy);
	}

	/**
	 * @return True if this StreamBuffer contains no data, false otherwise.
	 */
	Bool IsEmpty() const
	{
		return (0u == m_zSizeInBytes);
	}

	/**
	 * @return Total size in bytes of all data in this StreamBuffer.
	 */
	SizeType GetTotalDataSizeInBytes() const
	{
		return m_zSizeInBytes;
	}

	/**
	 * Access raw buffer - for example, in order to write to file
	 */
	Byte* GetBuffer()
	{
		return m_pBuffer;
	}

	/**
	 * Access raw buffer - for example, in order to write to file
	 */
	const Byte* GetBuffer() const
	{
		return m_pBuffer;
	}

	/** @return The total space currently reserved for this StreamBuffer. */
	UInt32 GetCapacity() const
	{
		return m_zCapacityInBytes;
	}

	// Generic read/write functions
	/**
	 * Reads zBytes of data into the buffer pointed to by pData
	 *
	 * @return True if zBytes of data were read, false otherwise
	 */
	Bool Read(void *pData, SizeType zBytes)
	{
		SEOUL_ASSERT(pData != nullptr);

		// Special handling of  0 byte read,
		// in case the buffer is empty and has no data pointer.
		if (0u == zBytes)
		{
			return true;
		}

		if (m_zOffsetInBytes + zBytes <= m_zSizeInBytes)
		{
			memcpy(pData, InternalGetHeadPointer(), zBytes);
			m_zOffsetInBytes += zBytes;
			return true;
		}

		return false;
	}

	/**
	 * Writes zBytes of data from the buffer pointed to byte pData
	 */
	void Write(const void *pData, SizeType zBytes)
	{
		// Special handling of  0 byte write,
		// in case the buffer is empty and has no data pointer.
		if (0u == zBytes)
		{
			return;
		}

		InternalGrowToAtLeast(m_zOffsetInBytes + zBytes);

		memcpy(InternalGetHeadPointer(), pData, zBytes);
		m_zOffsetInBytes += zBytes;
		m_zSizeInBytes = Max(m_zSizeInBytes, m_zOffsetInBytes);
	}

	/**
	 * Read an arbitrary POD value from the stream buffer from
	 * the current head position.
	 */
	template<typename T>
	Bool Read(T& rOutValue)
	{
		// Sanity check - make sure the generic versions
		// of these methods are not being used on
		// non-simple types (classes which cannot
		// be initialized with memset and cannot be copied
		// with memcpy).
		SEOUL_STATIC_ASSERT_MESSAGE(CanMemCpy<T>::Value, "StreamBuffer::Read() can only be used on a type T that is memcpyable.");

		return Read(reinterpret_cast<Byte*>(&rOutValue), sizeof(rOutValue));
	}

	/**
	 * Write an arbitrary POD value to this stream buffer at
	 * the current head position, increasing the stream buffer size if
	 * necessary.
	 */
	template<typename T>
	void Write(const T& value)
	{
		// Sanity check - make sure the generic versions
		// of these methods are not being used on
		// non-simple types (classes which cannot
		// be initialized with memset and cannot be copied
		// with memcpy).
		SEOUL_STATIC_ASSERT_MESSAGE(CanMemCpy<T>::Value, "StreamBuffer::Write() can only be used on a type T that is memcpyable.");

		Write(reinterpret_cast<const Byte*>(&value), sizeof(value));
	}

	// Read a Seoul::String from this StreamBuffer.
	Bool Read(String& sOutValue);

	// Write a Seoul::String to this StreamBuffer.
	void Write(const String& sValue);

	//@{
	/**
	 * Helper functions for deserializing multibyte integers with specific
	 * endiannesses
	 */
	Bool ReadLittleEndian16(Int16& value);
	Bool ReadLittleEndian16(UInt16& value);
	Bool ReadLittleEndian32(Float32& value);
	Bool ReadLittleEndian32(Int32& value);
	Bool ReadLittleEndian32(UInt32& value);
	Bool ReadLittleEndian64(Float64& value);
	Bool ReadLittleEndian64(Int64& value);
	Bool ReadLittleEndian64(UInt64& value);
	Bool ReadBigEndian16(Int16& value);
	Bool ReadBigEndian16(UInt16& value);
	Bool ReadBigEndian32(Float32& value);
	Bool ReadBigEndian32(Int32& value);
	Bool ReadBigEndian32(UInt32& value);
	Bool ReadBigEndian64(Float64& value);
	Bool ReadBigEndian64(Int64& value);
	Bool ReadBigEndian64(UInt64& value);
	Bool ReadNativeEndian16(Int16& value)   { return Read(value); }
	Bool ReadNativeEndian16(UInt16& value)  { return Read(value); }
	Bool ReadNativeEndian32(Float32& value) { return Read(value); }
	Bool ReadNativeEndian32(Int32& value)   { return Read(value); }
	Bool ReadNativeEndian32(UInt32& value)  { return Read(value); }
	Bool ReadNativeEndian64(Float64& value) { return Read(value); }
	Bool ReadNativeEndian64(Int64& value)   { return Read(value); }
	Bool ReadNativeEndian64(UInt64& value)  { return Read(value); }
	//@}

	//@{
	/**
	 * Helper functions for serializing multibyte integers with specific
	 * endiannesses
	 */
	void WriteLittleEndian16(Int16 value);
	void WriteLittleEndian16(UInt16 value);
	void WriteLittleEndian32(Int32 value);
	void WriteLittleEndian32(Float32 value);
	void WriteLittleEndian32(UInt32 value);
	void WriteLittleEndian64(Float64 value);
	void WriteLittleEndian64(Int64 value);
	void WriteLittleEndian64(UInt64 value);
	void WriteBigEndian16(Int16 value);
	void WriteBigEndian16(UInt16 value);
	void WriteBigEndian32(Float32 value);
	void WriteBigEndian32(Int32 value);
	void WriteBigEndian32(UInt32 value);
	void WriteBigEndian64(Float64 value);
	void WriteBigEndian64(Int64 value);
	void WriteBigEndian64(UInt64 value);
	void WriteNativeEndian16(Int16 value)   { Write(value); }
	void WriteNativeEndian16(UInt16 value)  { Write(value); }
	void WriteNativeEndian32(Float32 value) { Write(value); }
	void WriteNativeEndian32(Int32 value)   { Write(value); }
	void WriteNativeEndian32(UInt32 value)  { Write(value); }
	void WriteNativeEndian64(Double value) { Write(value); }
	void WriteNativeEndian64(Int64 value)   { Write(value); }
	void WriteNativeEndian64(UInt64 value)  { Write(value); }
	//@}

	void PadTo(SizeType zSizeInBytes, Bool bInitializeToZero = true);

	/** Gets the current buffer offset */
	SizeType GetOffset() const
	{
		return m_zOffsetInBytes;
	}

	/** Sets the current buffer offset, clamped to our buffer size */
	void SeekToOffset(SizeType zOffset)
	{
		m_zOffsetInBytes = Min(zOffset, m_zSizeInBytes);
	}

	/** Exchange the contents of this StreamBuffer with rBuffer. */
	void Swap(StreamBuffer& rBuffer)
	{
		m_pBuffer.Swap(rBuffer.m_pBuffer);
		Seoul::Swap(m_zCapacityInBytes, rBuffer.m_zCapacityInBytes);
		Seoul::Swap(m_zSizeInBytes, rBuffer.m_zSizeInBytes);
		Seoul::Swap(m_zOffsetInBytes, rBuffer.m_zOffsetInBytes);
		Seoul::Swap(m_eType, rBuffer.m_eType);
	}

	/** Assign the StreamBuffer's current buffer and size to rpData. */
	void RelinquishBuffer(void*& rpData, SizeType& rzDataSizeInBytes)
	{
		rpData = MemoryManager::Reallocate(m_pBuffer.Get(), m_zSizeInBytes, m_eType);
		rzDataSizeInBytes = m_zSizeInBytes;

		// Clear
		m_pBuffer = nullptr;
		m_zCapacityInBytes = 0u;
		m_zSizeInBytes = 0u;
		m_zOffsetInBytes = 0u;
	}
	void RelinquishBuffer(Byte*& rpData, SizeType& rzDataSizeInBytes)
	{
		void* p = nullptr;
		RelinquishBuffer(p, rzDataSizeInBytes);
		rpData = (Byte*)p;
	}

	/** 
	 * Reduce the capacity of the buffer - will clamp to the size (the capacity can never
	 * become smaller than the size). This is also a no-op if the specified size is
	 * larger than the current capacity (this function will never grow the capacity).
	 */
	void ShrinkTo(UInt32 zCapacityInBytes)
	{
		if (zCapacityInBytes >= m_zCapacityInBytes)
		{
			return;
		}

		InternalSetCapacity(Max(m_zSizeInBytes, zCapacityInBytes));
	}

	/** Swap the internal buffer of StreamBuffer with a raw byte buffer allocated with MemoryManager::Allocate. */
	void TakeOwnership(Byte* pData, SizeType zSizeInBytes)
	{
		// Destroy our existing data.
		{
			StreamBuffer empty;
			Swap(empty);
		}

		// Assign new data.
		m_pBuffer = pData;
		m_zCapacityInBytes = zSizeInBytes;
		m_zSizeInBytes = zSizeInBytes;
		m_zOffsetInBytes = 0u;
	}

	/** Reduce the size (not the capacity) of the buffer. */
	void TruncateTo(UInt32 zSizeInBytes)
	{
		// Nothing to do.
		if (m_zSizeInBytes <= zSizeInBytes)
		{
			return;
		}

		// Reduce and clamp
		m_zSizeInBytes = zSizeInBytes;
		m_zOffsetInBytes = Min(m_zOffsetInBytes, m_zSizeInBytes);
	}

private:
	CheckedPtr<Byte> m_pBuffer;
	SizeType m_zCapacityInBytes;
	SizeType m_zSizeInBytes;
	SizeType m_zOffsetInBytes;
	MemoryBudgets::Type m_eType;

	void InternalGrowToAtLeast(SizeType zSize)
	{
		if (zSize <= m_zCapacityInBytes)
		{
			return;
		}

		// Compute expontential growth. Expontential growth achieves
		// amortized constant time of growth.
		InternalSetCapacity(zSize + (zSize / 2));
	}

	void InternalSetCapacity(SizeType zCapacity)
	{
		SEOUL_ASSERT(m_zSizeInBytes <= zCapacity); // Must be enforced by callers.
		m_zCapacityInBytes = zCapacity;
		m_pBuffer = (Byte*)MemoryManager::Reallocate(
			m_pBuffer.Get(),
			m_zCapacityInBytes,
			m_eType);
	}

	Byte* InternalGetHeadPointer()
	{
		return (m_pBuffer.Get() + m_zOffsetInBytes);
	}

	SEOUL_DISABLE_COPY(StreamBuffer);
}; // class StreamBuffer

} // namespace Seoul

#endif // include guard
