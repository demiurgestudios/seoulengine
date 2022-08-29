/**
 * \file CommandStream.h
 * \brief CommandStream is similar to StreamBuffer, but specialized for runtime
 * build and execute of opaque command data. It makes assumptions about data
 * and provides functionality that is not useful/robust in a serialization context,
 * which is what StreamBuffer is designed for.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef	COMMAND_STREAM_H
#define COMMAND_STREAM_H

#include "ReflectionAny.h"
#include "ReflectionTypeInfo.h"
#include "StreamBuffer.h"

namespace Seoul
{

/**
 * Utility class for wrangling typed writes.
 */
template <typename T, Bool IS_ENUM>
struct CommandStreamTypedWriteUtility
{
	typedef T Type;
};

template <typename T>
struct CommandStreamTypedWriteUtility<T, true>
{
	typedef Int Type;
};

/**
 * CommandStream is useful for implementing the following pattern.
 * - in one context (possibly a unique thread), write simple data and
 *   command op codes into a contiguous, opaque data buffer.
 * - in a separate context (possibly a different unique thread), process
 *   the data in a simple, VM style switch {} case loop, executing commands
 *   stored in the buffer.
 */
class CommandStream SEOUL_SEALED
{
public:
	CommandStream()
		: m_zOffsetMarker(0u)
		, m_bInRead(false)
	{
	}

	~CommandStream()
	{
	}

	/**
	 * Align the next read to zAlignmentInBytes - always succeeds, unless the alignment
	 * will place the read pointer outside the size of the buffer. Alignment must always match
	 * exactly with any alignment padding that was added during writing.
	 */
	void AlignReadOffset(UInt32 zAlignmentInBytes = 16u)
	{
		StreamBuffer::SizeType const nAlignedOffset = (StreamBuffer::SizeType)RoundUpToAlignment(m_Buffer.GetOffset(), zAlignmentInBytes);
		m_Buffer.SeekToOffset(nAlignedOffset);
		SEOUL_ASSERT(m_Buffer.GetOffset() == nAlignedOffset);
	}

	/**
	 * Align the next write to zAlignmentInBytes - useful when read operations will
	 * access data in this CommandStream directly by casting the raw bytes to the native
	 * type.
	 */
	void AlignWriteOffset(UInt32 zAlignmentInBytes = 16u)
	{
		SEOUL_ASSERT(!m_bInRead);

		StreamBuffer::SizeType const nAlignedOffset = (StreamBuffer::SizeType)RoundUpToAlignment(m_Buffer.GetOffset(), zAlignmentInBytes);
		m_Buffer.PadTo(nAlignedOffset, false);
		SEOUL_ASSERT(m_Buffer.GetOffset() == nAlignedOffset);
	}

	/**
	 * Convenience function for TypedRead(Reflection::Any&, UInt32), in contexts
	 * where the caller does not care about the value of rzSizeInBytes.
	 */
	Bool TypedRead(Reflection::Any& rValue)
	{
		UInt32 zIgnoredSizeInBytes = 0u;
		return TypedRead(rValue, zIgnoredSizeInBytes);
	}

	/**
	 * Reads a value into rValue and its size into rzSizeInBytes that was written with
	 * TypedWrite().
	 *
	 * @return True if thre read is successful, false otherwise. If this method returns true, rValue
	 * will contain the data typed at write, and rzSizeInBytes will contain its size.
	 *
	 * For all types except strings, rzSizeInBytes is equal to sizeof(T). For strings, it is
	 * the size of the string in bytes, excluding the null terminator.
	 *
	 * \warning String values stored in rValue are Byte const* and are only valid
	 * until data is written to this CommandStream again.
	 */
	Bool TypedRead(Reflection::Any& rValue, UInt32& rzSizeInBytes)
	{
		using namespace Reflection;

		// Read the type info - if this fails, the read fails.
		SimpleTypeInfo eTypeInfo = SimpleTypeInfo::kComplex;
		if (!Read(eTypeInfo))
		{
			return false;
		}

		// Now process the data based on the type info - for all types except strings, this is an aligned read
		// of the data.
		switch (eTypeInfo)
		{
			// For string types, we read the size in bytes from the buffer, then set the Byte const* pointer
			// to the WeakAny directly.
		case Reflection::SimpleTypeInfo::kCString: // fall-through
		case Reflection::SimpleTypeInfo::kString:
			{
				if (Read(rzSizeInBytes))
				{
					rValue = (Byte const*)(m_Buffer.GetBuffer() + m_Buffer.GetOffset());
					m_Buffer.SeekToOffset(m_Buffer.GetOffset() + rzSizeInBytes + 1u);
					return true;
				}

				return false;
			}
			break;

		case Reflection::SimpleTypeInfo::kBoolean: return InternalTypedRead<Bool>(rValue, rzSizeInBytes);
		case Reflection::SimpleTypeInfo::kEnum: return InternalTypedRead<Int>(rValue, rzSizeInBytes);
		case Reflection::SimpleTypeInfo::kHString: return InternalTypedRead<HString>(rValue, rzSizeInBytes);
		case Reflection::SimpleTypeInfo::kFloat32: return InternalTypedRead<Float32>(rValue, rzSizeInBytes);
		case Reflection::SimpleTypeInfo::kFloat64: return InternalTypedRead<Float64>(rValue, rzSizeInBytes);
		case Reflection::SimpleTypeInfo::kInt8: return InternalTypedRead<Int8>(rValue, rzSizeInBytes);
		case Reflection::SimpleTypeInfo::kInt16: return InternalTypedRead<Int16>(rValue, rzSizeInBytes);
		case Reflection::SimpleTypeInfo::kInt32: return InternalTypedRead<Int32>(rValue, rzSizeInBytes);
		case Reflection::SimpleTypeInfo::kInt64: return InternalTypedRead<Int64>(rValue, rzSizeInBytes);
		case Reflection::SimpleTypeInfo::kUInt8: return InternalTypedRead<UInt8>(rValue, rzSizeInBytes);
		case Reflection::SimpleTypeInfo::kUInt16: return InternalTypedRead<UInt16>(rValue, rzSizeInBytes);
		case Reflection::SimpleTypeInfo::kUInt32: return InternalTypedRead<UInt32>(rValue, rzSizeInBytes);
		case Reflection::SimpleTypeInfo::kUInt64: return InternalTypedRead<UInt64>(rValue, rzSizeInBytes);

			// Should never get here - if we do, the stream is corrupted, or a new simple type was added
			// without updating this switch case statement.
		default:
			SEOUL_FAIL("Unknown enum value.");
			return false;
		};
	}

	/**
	 * Write a value that's already in the variant type to the buffer.
	 */
	void WriteAny(const Reflection::Any& rValue)
	{
		using namespace Reflection;

		SimpleTypeInfo eTypeInfo = rValue.GetTypeInfo().GetSimpleTypeInfo();
		switch (eTypeInfo)
		{
		case Reflection::SimpleTypeInfo::kCString: { TypedWrite(rValue.Cast<Byte const*>()); } break;
		case Reflection::SimpleTypeInfo::kString: { TypedWrite(rValue.Cast<String>()); } break;

		case Reflection::SimpleTypeInfo::kHString: { TypedWrite(rValue.Cast<HString>().CStr()); } break;
		case Reflection::SimpleTypeInfo::kBoolean: { TypedWrite(rValue.Cast<Bool>()); } break;
		case Reflection::SimpleTypeInfo::kEnum:	{ TypedWrite(rValue.Cast<Int>()); } break;

		case Reflection::SimpleTypeInfo::kFloat32: { TypedWrite(rValue.Cast<Float32>()); } break;
		case Reflection::SimpleTypeInfo::kFloat64: { TypedWrite(rValue.Cast<Float64>()); } break;

		case Reflection::SimpleTypeInfo::kInt8: { TypedWrite(rValue.Cast<Int8>()); } break;
		case Reflection::SimpleTypeInfo::kInt16: { TypedWrite(rValue.Cast<Int16>()); } break;
		case Reflection::SimpleTypeInfo::kInt32: { TypedWrite(rValue.Cast<Int32>()); } break;
		case Reflection::SimpleTypeInfo::kInt64: { TypedWrite(rValue.Cast<Int64>()); } break;

		case Reflection::SimpleTypeInfo::kUInt8: { TypedWrite(rValue.Cast<UInt8>()); } break;
		case Reflection::SimpleTypeInfo::kUInt16: { TypedWrite(rValue.Cast<UInt16>()); } break;
		case Reflection::SimpleTypeInfo::kUInt32: { TypedWrite(rValue.Cast<UInt32>()); } break;
		case Reflection::SimpleTypeInfo::kUInt64: { TypedWrite(rValue.Cast<UInt64>()); } break;

			// Should never get here - if we do, the stream is corrupted, or a new simple type was added
			// without updating this switch case statement.
		default:
			SEOUL_FAIL("Unknown enum value.");
		};
	}

	/**
	 * Write the string sValue to this CommandStream with dynamic typing info - must be read
	 * with a call to TypedRead().
	 *
	 * @param[in] sValue UTF8 string data.
	 * @param[in] zSizeInBytes Size of the string data in bytes, excluding null terminator.
	 */
	void TypedWrite(Byte const* sValue, UInt32 zSizeInBytes)
	{
		using namespace Reflection;

		SEOUL_ASSERT(!m_bInRead);

		Write(SimpleTypeInfo::kString);
		Write(zSizeInBytes);
		m_Buffer.Write(sValue, zSizeInBytes);
		Write('\0');
	}

	/**
	 * Convenience function for TypedWrite(Byte const*, UInt32), allows strings
	 * to be written without explicitly specified length.
	 */
	void TypedWrite(Byte const* sValue)
	{
		SEOUL_ASSERT(!m_bInRead);

		TypedWrite(sValue, StrLen(sValue));
	}

	/**
	 * Convenience function for TypedWrite(Byte const*, UInt32), allows Seoul::String
	 * to be written.
	 */
	void TypedWrite(const String& sValue)
	{
		SEOUL_ASSERT(!m_bInRead);

		TypedWrite(sValue.CStr(), sValue.GetSize());
	}

	/**
	 * Write a simple type to the buffer with dynamic type information - the type T *must* be
	 * a simple type (TypeId<T>().GetSimpleType() != SimpleTypeInfo::kComplex). The value must be
	 * read from the buffer using TypedRead.
	 */
	template <typename T>
	void TypedWrite(const T& value)
	{
		using namespace Reflection;

		typedef typename CommandStreamTypedWriteUtility<T, IsEnum<T>::Value>::Type TypeToWrite;

		SEOUL_STATIC_ASSERT_MESSAGE(TypeInfoDetail::SimpleTypeInfoT<T>::Value != SimpleTypeInfo::kComplex, "CommandStreams cannot encode complex types, see SimpleTypeInfo for types that are not complex.");
		SEOUL_STATIC_ASSERT_MESSAGE(TypeInfoDetail::SimpleTypeInfoT<T>::Value != SimpleTypeInfo::kCString, "Invalid call of generic CommandStream::TypedWrite with a cstring, check for modifiers that may prevent resolution to the TypedWriter overload meant for cstrings.");
		SEOUL_STATIC_ASSERT_MESSAGE(TypeInfoDetail::SimpleTypeInfoT<T>::Value != SimpleTypeInfo::kString, "Invalid call of generic CommandStream::TypedWrite with a Seoul::String, check for modifiers that may prevent resolution to the TypedWriter overload meant for Seoul::Strings.");

		SEOUL_ASSERT(!m_bInRead);

		Write(SimpleTypeId<T>());
		AlignWriteOffset(SEOUL_ALIGN_OF(TypeToWrite));
		Write((TypeToWrite)value);
	}

	/**
	 * Read rValue from this CommandStream, previously written with a call to Write().
	 */
	template <typename T>
	Bool Read(T& rValue)
	{
		return m_Buffer.Read(&rValue, sizeof(rValue));
	}

	/**
	 * Write value to this CommandStream - the value is written without alignment (so it
	 * will occassionally occupy less space than an aligned write), but it *must* be read with a
	 * call to Read().
	 */
	template <typename T>
	void Write(const T& value)
	{
		SEOUL_ASSERT(!m_bInRead);

		m_Buffer.Write(&value, sizeof(value));
	}

	/**
	 * After populating this CommandStream with write operations, call this method before invoking
	 * any read operations.
	 */
	void BeginRead()
	{
		SEOUL_ASSERT(!m_bInRead);
		m_zOffsetMarker = m_Buffer.GetOffset();
		m_Buffer.SeekToOffset(0);
		m_bInRead = true;
	}

	/**
	 * After reading is complete, invoke this method before calling any new write operations.
	 */
	void EndRead()
	{
		SEOUL_ASSERT(m_bInRead);
		SEOUL_ASSERT(m_zOffsetMarker == m_Buffer.GetOffset());

		m_bInRead = false;
		Clear();
	}

	/**
	 * Clear the CommandStream without reading.
	 */
	void Clear()
	{
		m_zOffsetMarker = 0u;
		m_Buffer.Clear();
	}

	/** @return The current offset in bytes into this CommandStream. */
	UInt32 GetOffsetMarker() const
	{
		return m_zOffsetMarker;
	}

	/**
	 * @return True if this CommandStream is within a BeginRead()/EndRead() pair,
	 * false otherwise.
	 */
	Bool IsInRead() const
	{
		return m_bInRead;
	}

	/**
	 * Swap the state of this CommandStream with another.
	 */
	void Swap(CommandStream& rCommandStream)
	{
		Seoul::Swap(m_zOffsetMarker, rCommandStream.m_zOffsetMarker);
		m_Buffer.Swap(rCommandStream.m_Buffer);
	}

private:
	StreamBuffer m_Buffer;
	UInt32 m_zOffsetMarker;
	Bool m_bInRead;

	/**
	 * Helper for writing dynamically typed, simple data.
	 */
	template <typename T>
	Bool InternalTypedRead(Reflection::Any& rValue, UInt32& rzSizeInBytes)
	{
		AlignReadOffset(SEOUL_ALIGN_OF(T));

		if (m_Buffer.GetOffset() + sizeof(T) <= m_Buffer.GetTotalDataSizeInBytes())
		{
			rzSizeInBytes = sizeof(T);
			rValue = *((T*)(m_Buffer.GetBuffer() + m_Buffer.GetOffset()));
			m_Buffer.SeekToOffset(m_Buffer.GetOffset() + sizeof(T));
			return true;
		}

		return false;
	}

	SEOUL_DISABLE_COPY(CommandStream);
}; // class CommandStream

} // namespace Seoul

#endif // include guard
