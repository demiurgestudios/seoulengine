/**
 * \file StreamBufferTest.cpp
 * \brief Unit tests for the serialization/deserialization StreamBuffer
 * class.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DiskFileSystem.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "SeoulFile.h"
#include "StreamBuffer.h"
#include "StreamBufferTest.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(StreamBufferTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestBasicStreamBuffer)
	SEOUL_METHOD(TestBigEndianStreamBuffer)
	SEOUL_METHOD(TestLittleEndianStreamBuffer)
	SEOUL_METHOD(TestRelinquishBuffer)
SEOUL_END_TYPE()

template <typename T>
void TestRead(StreamBuffer& buffer, const T& expectedValue)
{
	T value = T();
	SEOUL_UNITTESTING_ASSERT(buffer.Read(value));
	SEOUL_UNITTESTING_ASSERT(expectedValue == value);
}

template <typename T>
void TestReadBigEndian16(StreamBuffer& buffer, const T& expectedValue)
{
	T value = T();
	SEOUL_UNITTESTING_ASSERT(buffer.ReadBigEndian16(value));
	SEOUL_UNITTESTING_ASSERT(expectedValue == value);
}

template <typename T>
void TestReadBigEndian32(StreamBuffer& buffer, const T& expectedValue)
{
	T value = T();
	SEOUL_UNITTESTING_ASSERT(buffer.ReadBigEndian32(value));
	SEOUL_UNITTESTING_ASSERT(expectedValue == value);
}

template <typename T>
void TestReadBigEndian64(StreamBuffer& buffer, const T& expectedValue)
{
	T value = T();
	SEOUL_UNITTESTING_ASSERT(buffer.ReadBigEndian64(value));
	SEOUL_UNITTESTING_ASSERT(expectedValue == value);
}

template <typename T>
void TestReadLittleEndian16(StreamBuffer& buffer, const T& expectedValue)
{
	T value = T();
	SEOUL_UNITTESTING_ASSERT(buffer.ReadLittleEndian16(value));
	SEOUL_UNITTESTING_ASSERT(expectedValue == value);
}

template <typename T>
void TestReadLittleEndian32(StreamBuffer& buffer, const T& expectedValue)
{
	T value = T();
	SEOUL_UNITTESTING_ASSERT(buffer.ReadLittleEndian32(value));
	SEOUL_UNITTESTING_ASSERT(expectedValue == value);
}

template <typename T>
void TestReadLittleEndian64(StreamBuffer& buffer, const T& expectedValue)
{
	T value = T();
	SEOUL_UNITTESTING_ASSERT(buffer.ReadLittleEndian64(value));
	SEOUL_UNITTESTING_ASSERT(expectedValue == value);
}

inline Bool LoadBuffer(const String& sFilename, StreamBuffer& buffer)
{
	DiskSyncFile syncFile(sFilename, File::kRead);
	return buffer.Load(syncFile);
}

inline Bool SaveBuffer(const String& sFilename, StreamBuffer& buffer)
{
	DiskSyncFile syncFile(sFilename, File::kWriteTruncate);
	return buffer.Save(syncFile);
}

void StreamBufferTest::TestBasicStreamBuffer()
{
	StreamBuffer buffer;
	SEOUL_UNITTESTING_ASSERT(0u == buffer.GetTotalDataSizeInBytes());
	SEOUL_UNITTESTING_ASSERT(!buffer.HasMoreData());

	buffer.Write((UInt8)0);
	buffer.Write((Int8)1);
	buffer.Write((UInt16)2);
	buffer.Write((Int16)3);
	buffer.Write((UInt32)4);
	buffer.Write((Int32)5);
	buffer.Write((UInt64)6);
	buffer.Write((Int64)7);
	buffer.Write((Float)8);
	buffer.Write((Double)9);

	buffer.Write((Bool)false);
	buffer.Write((wchar_t)L'A');

	buffer.Write(String("Hello World"));

	String const sTempFilename(Path::GetTempFileAbsoluteFilename());
	if (SaveBuffer(sTempFilename, buffer))
	{
		buffer.Clear();
		SEOUL_UNITTESTING_ASSERT(0u == buffer.GetTotalDataSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!buffer.HasMoreData());

		SEOUL_UNITTESTING_ASSERT(LoadBuffer(sTempFilename, buffer));

		TestRead(buffer, (UInt8)0);
		TestRead(buffer, (Int8)1);
		TestRead(buffer, (UInt16)2);
		TestRead(buffer, (Int16)3);
		TestRead(buffer, (UInt32)4);
		TestRead(buffer, (Int32)5);
		TestRead(buffer, (UInt64)6);
		TestRead(buffer, (Int64)7);
		TestRead(buffer, (Float)8);
		TestRead(buffer, (Double)9);

		TestRead(buffer, (Bool)false);
		TestRead(buffer, (wchar_t)L'A');

		TestRead(buffer, String("Hello World"));
	}
	else
	{
		SEOUL_LOG("%s: is being skipped because a file could not be saved.", __FUNCTION__);
	}
}

void StreamBufferTest::TestBigEndianStreamBuffer()
{
	StreamBuffer buffer;
	SEOUL_UNITTESTING_ASSERT(0u == buffer.GetTotalDataSizeInBytes());
	SEOUL_UNITTESTING_ASSERT(!buffer.HasMoreData());

	buffer.WriteBigEndian16((UInt16)2);
	buffer.WriteBigEndian16((Int16)3);
	buffer.WriteBigEndian32((UInt32)4);
	buffer.WriteBigEndian32((Int32)5);
	buffer.WriteBigEndian64((UInt64)6);
	buffer.WriteBigEndian64((Int64)7);
	buffer.WriteBigEndian32((Float)8);
	buffer.WriteBigEndian64((Double)9);

	String const sTempFilename(Path::GetTempFileAbsoluteFilename());
	if (SaveBuffer(sTempFilename, buffer))
	{
		buffer.Clear();
		SEOUL_UNITTESTING_ASSERT(0u == buffer.GetTotalDataSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!buffer.HasMoreData());

		SEOUL_UNITTESTING_ASSERT(LoadBuffer(sTempFilename, buffer));

		TestReadBigEndian16(buffer, (UInt16)2);
		TestReadBigEndian16(buffer, (Int16)3);
		TestReadBigEndian32(buffer, (UInt32)4);
		TestReadBigEndian32(buffer, (Int32)5);
		TestReadBigEndian64(buffer, (UInt64)6);
		TestReadBigEndian64(buffer, (Int64)7);
		TestReadBigEndian32(buffer, (Float)8);
		TestReadBigEndian64(buffer, (Double)9);
	}
	else
	{
		SEOUL_LOG("%s: is being skipped because a file could not be saved.", __FUNCTION__);
	}
}

void StreamBufferTest::TestLittleEndianStreamBuffer()
{
	StreamBuffer buffer;
	SEOUL_UNITTESTING_ASSERT(0u == buffer.GetTotalDataSizeInBytes());
	SEOUL_UNITTESTING_ASSERT(!buffer.HasMoreData());

	buffer.WriteLittleEndian16((UInt16)2);
	buffer.WriteLittleEndian16((Int16)3);
	buffer.WriteLittleEndian32((UInt32)4);
	buffer.WriteLittleEndian32((Int32)5);
	buffer.WriteLittleEndian64((UInt64)6);
	buffer.WriteLittleEndian64((Int64)7);
	buffer.WriteLittleEndian32((Float)8);
	buffer.WriteLittleEndian64((Double)9);

	String const sTempFilename(Path::GetTempFileAbsoluteFilename());
	if (SaveBuffer(sTempFilename, buffer))
	{
		buffer.Clear();
		SEOUL_UNITTESTING_ASSERT(0u == buffer.GetTotalDataSizeInBytes());
		SEOUL_UNITTESTING_ASSERT(!buffer.HasMoreData());

		SEOUL_UNITTESTING_ASSERT(LoadBuffer(sTempFilename, buffer));

		TestReadLittleEndian16(buffer, (UInt16)2);
		TestReadLittleEndian16(buffer, (Int16)3);
		TestReadLittleEndian32(buffer, (UInt32)4);
		TestReadLittleEndian32(buffer, (Int32)5);
		TestReadLittleEndian64(buffer, (UInt64)6);
		TestReadLittleEndian64(buffer, (Int64)7);
		TestReadLittleEndian32(buffer, (Float)8);
		TestReadLittleEndian64(buffer, (Double)9);
	}
	else
	{
		SEOUL_LOG("%s: is being skipped because a file could not be saved.", __FUNCTION__);
	}
}

void StreamBufferTest::TestRelinquishBuffer()
{
	// void*
	{
		StreamBuffer buffer;
		buffer.Write((UInt32)45);

		UInt32 const u = buffer.GetTotalDataSizeInBytes();

		void* pU = nullptr;
		UInt32 uU = 0u;
		buffer.RelinquishBuffer(pU, uU);

		SEOUL_UNITTESTING_ASSERT_EQUAL(u, uU);
		SEOUL_UNITTESTING_ASSERT_EQUAL(sizeof(UInt32), uU);

		UInt32 uValue;
		memcpy(&uValue, pU, sizeof(uValue));

		SEOUL_UNITTESTING_ASSERT_EQUAL(45u, uValue);

		MemoryManager::Deallocate(pU);
	}

	// Byte*
	{
		StreamBuffer buffer;
		buffer.Write((UInt32)45);

		UInt32 const u = buffer.GetTotalDataSizeInBytes();

		Byte* pU = nullptr;
		UInt32 uU = 0u;
		buffer.RelinquishBuffer(pU, uU);

		SEOUL_UNITTESTING_ASSERT_EQUAL(u, uU);
		SEOUL_UNITTESTING_ASSERT_EQUAL(sizeof(UInt32), uU);

		UInt32 uValue;
		memcpy(&uValue, pU, sizeof(uValue));

		SEOUL_UNITTESTING_ASSERT_EQUAL(45u, uValue);

		MemoryManager::Deallocate(pU);
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
