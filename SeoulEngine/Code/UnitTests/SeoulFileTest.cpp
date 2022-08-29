/**
 * \file SeoulFileTest.cpp
 * \brief Unit tests for the File class.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "SeoulFileTest.h"
#include "Color.h"
#include "Directory.h"
#include "DiskFileSystem.h"
#include "Logger.h"
#include "Matrix3x4.h"
#include "Path.h"
#include "ReflectionDefine.h"
#include "SeoulFile.h"
#include "SeoulFileReaders.h"
#include "SeoulFileWriters.h"
#include "SeoulString.h"
#include "UnitTesting.h"
#include "Vector.h"

#include <stdint.h>

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(SeoulFileTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestBinaryReadWriteDiskSyncFile)
	SEOUL_METHOD(TestBinaryReadWriteFullyBufferedSyncFile)
	SEOUL_METHOD(TestDiskSyncFileReadStatic)
	SEOUL_METHOD(TestTextReadWriteDiskSyncFile)
	SEOUL_METHOD(TestTextReadWriteFullyBufferedSyncFile)
	SEOUL_METHOD(TestReadWriteBufferedSyncFile)
	SEOUL_METHOD(TestUtilityFunctions)
SEOUL_END_TYPE()

enum TestEnum
{
	kZero,
	kOne,
	kTwo,
	kThree
};

template <typename T>
Bool TestRead(Bool (*ReadFunc)(SyncFile&, T&), SyncFile& rFile, const T& expectedValue)
{
	T value;
	if (ReadFunc(rFile, value))
	{
		return (value == expectedValue);
	}

	return false;
}

template <typename T>
Bool TestRead(Bool (*ReadFunc)(SyncFile&, T&, UInt32), SyncFile& rFile, const T& expectedValue)
{
	T value;
	if (ReadFunc(rFile, value, kDefaultMaxReadSize))
	{
		return (value == expectedValue);
	}

	return false;
}

Bool TestRead(
	Bool (*ReadFunc)(SyncFile&, GameDirectory, FilePath&),
	SyncFile& rFile,
	GameDirectory eDirectory,
	const FilePath& expectedValue)
{
	FilePath value;
	if (ReadFunc(rFile, eDirectory, value))
	{
		return (value == expectedValue);
	}

	return false;
}

template <typename T>
inline Bool operator==(const Vector<T>& a, const Vector<T>& b)
{
	const UInt32 nCount = a.GetSize();
	if (nCount == b.GetSize())
	{
		for (UInt32 i = 0u; i < nCount; ++i)
		{
			if (a[i] != b[i])
			{
				return false;
			}
		}

		return true;
	}

	return false;
}

void SeoulFileTest::TestBinaryReadWriteDiskSyncFile()
{
	static const Sphere kTestSphere(Vector3D(1.0f, 1.0f, 1.0f), 10.0f);
	static const String kTestString("Hello World");
	static const HString kTestHString("Hello World, Again");
	static const Matrix3x4 kTestMatrix3x4(
		1.0f, 2.0f, 3.0f, 4.0f,
		5.0f, 6.0f, 7.0f, 8.0f,
		9.0f, 10.0f, 11.0f, 12.0f);
	static const Matrix4D kTestMatrix4D(
		1.0f, 2.0f, 3.0f, 4.0f,
		5.0f, 6.0f, 7.0f, 8.0f,
		9.0f, 10.0f, 11.0f, 12.0f,
		13.0f, 14.0f, 15.0f, 16.0f);
	static const FilePath kTestFilePath =
		FilePath::CreateContentFilePath("bozo.png");
	static const Vector2D kTestVector2D(
		4.0f, 5.0f);
	static const Vector3D kTestVector3D(
		6.0f, 7.0f, 8.0f);
	static const Vector4D kTestVector4D(
		9.0f, 10.0f, 11.0f, 12.0f);
	static const Color4 kTestColor4(
		13.0f, 14.0f, 15.0f, 16.0f);
	static const Color4 kTestColor4ForVector3D(
		13.0f, 14.0f, 15.0f, 1.0f);
	static const Vector<Float> kTestBuffer(12u, 17.0f);
	static const TestEnum kTestEnum = kThree;
	static const Vector<UInt16> kEmptyTestBuffer;
	static const Quaternion kTestQuaternion(Quaternion::CreateFromAxisAngle(Vector3D::UnitX(), DegreesToRadians(45.0f)));

	static const UInt64 kExpectedFileSizeInBytes =
		sizeof(UInt8) + // UInt8 is used when serializing booleans
		sizeof(AABB) +
		sizeof(Sphere) +
		sizeof(Int8) +
		sizeof(UInt8) +
		sizeof(Int16) +
		sizeof(UInt16) +
		sizeof(Int32) +
		sizeof(UInt32) +
		sizeof(Int64) +
		sizeof(UInt64) +
		kTestString.GetSize() + sizeof(UInt32) + 1u +		// plus 1 for null terminator, plus a UInt32 for the string size
		kTestHString.GetSizeInBytes() + sizeof(UInt32) + 1u +	// plus 1 for null terminator, plus a UInt32 for the string size
		sizeof(Matrix3x4) +
		sizeof(Matrix4D) +
		sizeof(Float) +
		sizeof(Vector2D) +
		sizeof(Vector3D) +
		sizeof(Vector4D) +
		kTestFilePath.GetRelativeFilename().GetSize() + sizeof(UInt32) + 1u +
		sizeof(Vector4D) +
		sizeof(Vector3D) +
		sizeof(UInt32) +	// UInt32 used for Enums
		kTestBuffer.GetSize() * sizeof(Float) + sizeof(UInt32) + // buffer size in bytes plus UInt32 for the size
		kEmptyTestBuffer.GetSize() * sizeof(UInt16) + sizeof(UInt32) +
		sizeof(Quaternion);

	String const sTempFilename(Path::GetTempFileAbsoluteFilename());
	// write
	{
		DiskSyncFile file(sTempFilename, File::kWriteTruncate);
		if (file.IsOpen())
		{
			SEOUL_UNITTESTING_ASSERT(file.CanWrite());
			SEOUL_UNITTESTING_ASSERT(!file.CanRead());

			SEOUL_UNITTESTING_ASSERT(WriteBoolean(file, true));
			SEOUL_UNITTESTING_ASSERT(WriteAABB(file, AABB::InverseMaxAABB()));
			SEOUL_UNITTESTING_ASSERT(WriteSphere(file, kTestSphere));
			SEOUL_UNITTESTING_ASSERT(WriteInt8(file, 1));
			SEOUL_UNITTESTING_ASSERT(WriteUInt8(file, 2));
			SEOUL_UNITTESTING_ASSERT(WriteInt16(file, 3));
			SEOUL_UNITTESTING_ASSERT(WriteUInt16(file, 4));
			SEOUL_UNITTESTING_ASSERT(WriteInt32(file, 5));
			SEOUL_UNITTESTING_ASSERT(WriteUInt32(file, 6));
			SEOUL_UNITTESTING_ASSERT(WriteInt64(file, 7));
			SEOUL_UNITTESTING_ASSERT(WriteUInt64(file, 8));
			SEOUL_UNITTESTING_ASSERT(WriteString(file, kTestString));
			SEOUL_UNITTESTING_ASSERT(WriteHString(file, kTestHString));
			SEOUL_UNITTESTING_ASSERT(WriteMatrix3x4(file, kTestMatrix3x4));
			SEOUL_UNITTESTING_ASSERT(WriteMatrix4D(file, kTestMatrix4D));
			SEOUL_UNITTESTING_ASSERT(WriteSingle(file, 9.0f));
			SEOUL_UNITTESTING_ASSERT(WriteVector2D(file, kTestVector2D));
			SEOUL_UNITTESTING_ASSERT(WriteVector3D(file, kTestVector3D));
			SEOUL_UNITTESTING_ASSERT(WriteVector4D(file, kTestVector4D));
			SEOUL_UNITTESTING_ASSERT(WriteFilePath(file, kTestFilePath));
			SEOUL_UNITTESTING_ASSERT(WriteVector4D(file, kTestColor4));
			SEOUL_UNITTESTING_ASSERT(WriteVector3D(file, kTestColor4));
			SEOUL_UNITTESTING_ASSERT(WriteEnum(file, kTestEnum));
			SEOUL_UNITTESTING_ASSERT(WriteBuffer(file, kTestBuffer));
			SEOUL_UNITTESTING_ASSERT(WriteBuffer(file, kEmptyTestBuffer));
			SEOUL_UNITTESTING_ASSERT(WriteQuaternion(file, kTestQuaternion));

			file.Flush();
		}
		else
		{
			SEOUL_LOG("%s: is being skipped because a temp file could not be generated.", __FUNCTION__);
			return;
		}
	}
	// end write test

	// read
	{
		DiskSyncFile file(sTempFilename, File::kRead);
		SEOUL_UNITTESTING_ASSERT(file.IsOpen());

		SEOUL_UNITTESTING_ASSERT(!file.CanWrite());
		SEOUL_UNITTESTING_ASSERT(file.CanRead());

		SEOUL_UNITTESTING_ASSERT(TestRead(ReadBoolean, file, true));
		SEOUL_UNITTESTING_ASSERT(TestRead(ReadAABB, file, AABB::InverseMaxAABB()));
		SEOUL_UNITTESTING_ASSERT(TestRead(ReadSphere, file, kTestSphere));
		SEOUL_UNITTESTING_ASSERT(TestRead(ReadInt8, file, (Int8)1));
		SEOUL_UNITTESTING_ASSERT(TestRead(ReadUInt8, file, (UInt8)2));
		SEOUL_UNITTESTING_ASSERT(TestRead(ReadInt16, file, (Int16)3));
		SEOUL_UNITTESTING_ASSERT(TestRead(ReadUInt16, file, (UInt16)4));
		SEOUL_UNITTESTING_ASSERT(TestRead(ReadInt32, file, (Int32)5));
		SEOUL_UNITTESTING_ASSERT(TestRead(ReadUInt32, file, (UInt32)6));
		SEOUL_UNITTESTING_ASSERT(TestRead(ReadInt64, file, (Int64)7));
		SEOUL_UNITTESTING_ASSERT(TestRead(ReadUInt64, file, (UInt64)8));
		SEOUL_UNITTESTING_ASSERT(TestRead(ReadString, file, kTestString));
		SEOUL_UNITTESTING_ASSERT(TestRead(ReadHString, file, kTestHString));
		SEOUL_UNITTESTING_ASSERT(TestRead(ReadMatrix3x4, file, kTestMatrix3x4));
		SEOUL_UNITTESTING_ASSERT(TestRead(ReadMatrix4D, file, kTestMatrix4D));
		SEOUL_UNITTESTING_ASSERT(TestRead(ReadSingle, file, 9.0f));
		SEOUL_UNITTESTING_ASSERT(TestRead<Vector2D>(ReadVector2D, file, kTestVector2D));
		SEOUL_UNITTESTING_ASSERT(TestRead<Vector3D>(ReadVector3D, file, kTestVector3D));
		SEOUL_UNITTESTING_ASSERT(TestRead<Vector4D>(ReadVector4D, file, kTestVector4D));
		SEOUL_UNITTESTING_ASSERT(TestRead(ReadFilePath, file, GameDirectory::kContent, kTestFilePath));
		SEOUL_UNITTESTING_ASSERT(TestRead<Color4>(ReadVector4D, file, kTestColor4));
		SEOUL_UNITTESTING_ASSERT(TestRead<Color4>(ReadVector3D, file, kTestColor4ForVector3D));
		SEOUL_UNITTESTING_ASSERT(TestRead<TestEnum>(ReadEnum, file, kTestEnum));
		SEOUL_UNITTESTING_ASSERT(TestRead< Vector<Float> >(ReadBuffer, file, kTestBuffer));
		SEOUL_UNITTESTING_ASSERT(TestRead< Vector<UInt16> >(ReadBuffer, file, kEmptyTestBuffer));
		SEOUL_UNITTESTING_ASSERT(TestRead<Quaternion>(ReadQuaternion, file, kTestQuaternion));

		SEOUL_UNITTESTING_ASSERT(kExpectedFileSizeInBytes == file.GetSize());
	}
	// end read test
}

void SeoulFileTest::TestBinaryReadWriteFullyBufferedSyncFile()
{
	static const Sphere kTestSphere(Vector3D(1.0f, 1.0f, 1.0f), 10.0f);
	static const String kTestString("Hello World");
	static const HString kTestHString("Hello World, Again");
	static const Matrix3x4 kTestMatrix3x4(
		1.0f, 2.0f, 3.0f, 4.0f,
		5.0f, 6.0f, 7.0f, 8.0f,
		9.0f, 10.0f, 11.0f, 12.0f);
	static const Matrix4D kTestMatrix4D(
		1.0f, 2.0f, 3.0f, 4.0f,
		5.0f, 6.0f, 7.0f, 8.0f,
		9.0f, 10.0f, 11.0f, 12.0f,
		13.0f, 14.0f, 15.0f, 16.0f);
	static const FilePath kTestFilePath =
		FilePath::CreateContentFilePath("bozo.png");
	static const Vector2D kTestVector2D(
		4.0f, 5.0f);
	static const Vector3D kTestVector3D(
		6.0f, 7.0f, 8.0f);
	static const Vector4D kTestVector4D(
		9.0f, 10.0f, 11.0f, 12.0f);
	static const Color4 kTestColor4(
		13.0f, 14.0f, 15.0f, 16.0f);
	static const Color4 kTestColor4ForVector3D(
		13.0f, 14.0f, 15.0f, 1.0f);
	static const Vector<Float> kTestBuffer(12u, 17.0f);
	static const TestEnum kTestEnum = kThree;
	static const Vector<UInt16> kEmptyTestBuffer;
	static const Quaternion kTestQuaternion(Quaternion::CreateFromAxisAngle(Vector3D::UnitX(), DegreesToRadians(45.0f)));

	static const UInt64 kExpectedFileSizeInBytes =
		sizeof(UInt8) + // UInt8 is used when serializing booleans
		sizeof(AABB) +
		sizeof(Sphere) +
		sizeof(Int8) +
		sizeof(UInt8) +
		sizeof(Int16) +
		sizeof(UInt16) +
		sizeof(Int32) +
		sizeof(UInt32) +
		sizeof(Int64) +
		sizeof(UInt64) +
		kTestString.GetSize() + sizeof(UInt32) + 1u +		// plus 1 for null terminator, plus a UInt32 for the string size
		kTestHString.GetSizeInBytes() + sizeof(UInt32) + 1u +	// plus 1 for null terminator, plus a UInt32 for the string size
		sizeof(Matrix3x4) +
		sizeof(Matrix4D) +
		sizeof(Float) +
		sizeof(Vector2D) +
		sizeof(Vector3D) +
		sizeof(Vector4D) +
		kTestFilePath.GetRelativeFilename().GetSize() + sizeof(UInt32) + 1u +
		sizeof(Vector4D) +
		sizeof(Vector3D) +
		sizeof(UInt32) +	// UInt32 used for Enums
		kTestBuffer.GetSize() * sizeof(Float) + sizeof(UInt32) +	// buffer size in bytes plus UInt32 for the size
		kEmptyTestBuffer.GetSize() * sizeof(UInt16) + sizeof(UInt32) +
		sizeof(Quaternion);

	String const sTempFilename(Path::GetTempFileAbsoluteFilename());
	// write
	{
		DiskSyncFile file(sTempFilename, File::kWriteTruncate);

		if (file.IsOpen())
		{
			SEOUL_UNITTESTING_ASSERT(file.CanWrite());
			SEOUL_UNITTESTING_ASSERT(!file.CanRead());

			SEOUL_UNITTESTING_ASSERT(WriteBoolean(file, true));
			SEOUL_UNITTESTING_ASSERT(WriteAABB(file, AABB::InverseMaxAABB()));
			SEOUL_UNITTESTING_ASSERT(WriteSphere(file, kTestSphere));
			SEOUL_UNITTESTING_ASSERT(WriteInt8(file, 1));
			SEOUL_UNITTESTING_ASSERT(WriteUInt8(file, 2));
			SEOUL_UNITTESTING_ASSERT(WriteInt16(file, 3));
			SEOUL_UNITTESTING_ASSERT(WriteUInt16(file, 4));
			SEOUL_UNITTESTING_ASSERT(WriteInt32(file, 5));
			SEOUL_UNITTESTING_ASSERT(WriteUInt32(file, 6));
			SEOUL_UNITTESTING_ASSERT(WriteInt64(file, 7));
			SEOUL_UNITTESTING_ASSERT(WriteUInt64(file, 8));
			SEOUL_UNITTESTING_ASSERT(WriteString(file, kTestString));
			SEOUL_UNITTESTING_ASSERT(WriteHString(file, kTestHString));
			SEOUL_UNITTESTING_ASSERT(WriteMatrix3x4(file, kTestMatrix3x4));
			SEOUL_UNITTESTING_ASSERT(WriteMatrix4D(file, kTestMatrix4D));
			SEOUL_UNITTESTING_ASSERT(WriteSingle(file, 9.0f));
			SEOUL_UNITTESTING_ASSERT(WriteVector2D(file, kTestVector2D));
			SEOUL_UNITTESTING_ASSERT(WriteVector3D(file, kTestVector3D));
			SEOUL_UNITTESTING_ASSERT(WriteVector4D(file, kTestVector4D));
			SEOUL_UNITTESTING_ASSERT(WriteFilePath(file, kTestFilePath));
			SEOUL_UNITTESTING_ASSERT(WriteVector4D(file, kTestColor4));
			SEOUL_UNITTESTING_ASSERT(WriteVector3D(file, kTestColor4));
			SEOUL_UNITTESTING_ASSERT(WriteEnum(file, kTestEnum));
			SEOUL_UNITTESTING_ASSERT(WriteBuffer(file, kTestBuffer));
			SEOUL_UNITTESTING_ASSERT(WriteBuffer(file, kEmptyTestBuffer));
			SEOUL_UNITTESTING_ASSERT(WriteQuaternion(file, kTestQuaternion));

			file.Flush();
		}
		else
		{
			SEOUL_LOG("%s: is being skipped because a temp file could not be generated.", __FUNCTION__);
			return;
		}
	}
	// end write test

	// read
	{
		DiskSyncFile diskSyncFile(sTempFilename, File::kRead);
		FullyBufferedSyncFile file(diskSyncFile);
		SEOUL_UNITTESTING_ASSERT(file.IsOpen());

		SEOUL_UNITTESTING_ASSERT(!file.CanWrite());
		SEOUL_UNITTESTING_ASSERT(file.CanRead());

		SEOUL_UNITTESTING_ASSERT(TestRead(ReadBoolean, file, true));
		SEOUL_UNITTESTING_ASSERT(TestRead(ReadAABB, file, AABB::InverseMaxAABB()));
		SEOUL_UNITTESTING_ASSERT(TestRead(ReadSphere, file, kTestSphere));
		SEOUL_UNITTESTING_ASSERT(TestRead(ReadInt8, file, (Int8)1));
		SEOUL_UNITTESTING_ASSERT(TestRead(ReadUInt8, file, (UInt8)2));
		SEOUL_UNITTESTING_ASSERT(TestRead(ReadInt16, file, (Int16)3));
		SEOUL_UNITTESTING_ASSERT(TestRead(ReadUInt16, file, (UInt16)4));
		SEOUL_UNITTESTING_ASSERT(TestRead(ReadInt32, file, (Int32)5));
		SEOUL_UNITTESTING_ASSERT(TestRead(ReadUInt32, file, (UInt32)6));
		SEOUL_UNITTESTING_ASSERT(TestRead(ReadInt64, file, (Int64)7));
		SEOUL_UNITTESTING_ASSERT(TestRead(ReadUInt64, file, (UInt64)8));
		SEOUL_UNITTESTING_ASSERT(TestRead(ReadString, file, kTestString));
		SEOUL_UNITTESTING_ASSERT(TestRead(ReadHString, file, kTestHString));
		SEOUL_UNITTESTING_ASSERT(TestRead(ReadMatrix3x4, file, kTestMatrix3x4));
		SEOUL_UNITTESTING_ASSERT(TestRead(ReadMatrix4D, file, kTestMatrix4D));
		SEOUL_UNITTESTING_ASSERT(TestRead(ReadSingle, file, 9.0f));
		SEOUL_UNITTESTING_ASSERT(TestRead<Vector2D>(ReadVector2D, file, kTestVector2D));
		SEOUL_UNITTESTING_ASSERT(TestRead<Vector3D>(ReadVector3D, file, kTestVector3D));
		SEOUL_UNITTESTING_ASSERT(TestRead<Vector4D>(ReadVector4D, file, kTestVector4D));
		SEOUL_UNITTESTING_ASSERT(TestRead(ReadFilePath, file, GameDirectory::kContent, kTestFilePath));
		SEOUL_UNITTESTING_ASSERT(TestRead<Color4>(ReadVector4D, file, kTestColor4));
		SEOUL_UNITTESTING_ASSERT(TestRead<Color4>(ReadVector3D, file, kTestColor4ForVector3D));
		SEOUL_UNITTESTING_ASSERT(TestRead<TestEnum>(ReadEnum, file, kTestEnum));
		SEOUL_UNITTESTING_ASSERT(TestRead< Vector<Float> >(ReadBuffer, file, kTestBuffer));
		SEOUL_UNITTESTING_ASSERT(TestRead< Vector<UInt16> >(ReadBuffer, file, kEmptyTestBuffer));
		SEOUL_UNITTESTING_ASSERT(TestRead<Quaternion>(ReadQuaternion, file, kTestQuaternion));

		SEOUL_UNITTESTING_ASSERT(kExpectedFileSizeInBytes == file.GetSize());
	}
	// end read test
}

void SeoulFileTest::TestDiskSyncFileReadStatic()
{
	Byte const* s = "Hello World";
	UInt32 const u = StrLen(s);

	auto const sTempFilename(Path::GetTempFileAbsoluteFilename());
	SEOUL_UNITTESTING_ASSERT(DiskSyncFile::WriteAll(sTempFilename, (void const*)s, u));
	void* p = MemoryManager::Allocate(u, MemoryBudgets::Developer);
	SEOUL_UNITTESTING_ASSERT(DiskSyncFile::Read(sTempFilename, p, u));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(s, p, u));

	// Partial.
	memset(p, 0, u);
	SEOUL_UNITTESTING_ASSERT(DiskSyncFile::Read(sTempFilename, p, u - 2));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(s, p, u - 2));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ((Byte*)p)[u-2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ((Byte*)p)[u-1]);

	MemoryManager::Deallocate(p);
}

void SeoulFileTest::TestTextReadWriteDiskSyncFile()
{
	static const String kExpectedString = "The quick\n brown\n fox jumps\n over the lazy\n dog.\n";

	String const sTempFilename(Path::GetTempFileAbsoluteFilename());
	// write
	{
		DiskSyncFile file(sTempFilename, File::kWriteTruncate);

		if (file.IsOpen())
		{
			SEOUL_UNITTESTING_ASSERT(file.CanWrite());
			SEOUL_UNITTESTING_ASSERT(!file.CanRead());

			BufferedSyncFile bufferedFile(&file, false);
			bufferedFile.Printf("The quick\r\n %s\r\n fox jumps\r over the %s\n dog.\r",
				"brown",
				"lazy");

			bufferedFile.Flush();
		}
		else
		{
			SEOUL_LOG("%s: is being skipped because a temp file could not be generated.", __FUNCTION__);
			return;
		}
	}
	// end write test

	// read
	{
		DiskSyncFile file(sTempFilename, File::kRead);
		SEOUL_UNITTESTING_ASSERT(file.IsOpen());
		SEOUL_UNITTESTING_ASSERT(!file.CanWrite());
		SEOUL_UNITTESTING_ASSERT(file.CanRead());

		String sTotalString;
		String sLine;
		Int iCount = 0;
		{
			BufferedSyncFile bufferedFile(&file, false);
			while (bufferedFile.ReadLine(sLine))
			{
				iCount++;
				sTotalString += sLine;
			}
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, iCount);
		SEOUL_UNITTESTING_ASSERT_EQUAL(kExpectedString, sTotalString);
	}
	// end read test
}

void SeoulFileTest::TestTextReadWriteFullyBufferedSyncFile()
{
	static const String kExpectedString = "The quick\n brown\n fox jumps\n over the lazy\n dog.\n";

	String const sTempFilename(Path::GetTempFileAbsoluteFilename());
	// write
	{
		DiskSyncFile file(sTempFilename, File::kWriteTruncate);

		if (file.IsOpen())
		{
			SEOUL_UNITTESTING_ASSERT(file.CanWrite());
			SEOUL_UNITTESTING_ASSERT(!file.CanRead());

			{
				BufferedSyncFile bufferedFile(&file, false);
				bufferedFile.Printf("The quick\r\n %s\r\n fox jumps\r over the %s\n dog.\r",
					"brown",
					"lazy");

				bufferedFile.Flush();
			}
		}
		else
		{
			SEOUL_LOG("%s: is being skipped because a temp file could not be generated.", __FUNCTION__);
			return;
		}
	}
	// end write test

	// read
	{
		DiskSyncFile diskSyncFile(sTempFilename, File::kRead);
		FullyBufferedSyncFile file(diskSyncFile);
		SEOUL_UNITTESTING_ASSERT(file.IsOpen());
		SEOUL_UNITTESTING_ASSERT(!file.CanWrite());
		SEOUL_UNITTESTING_ASSERT(file.CanRead());

		String sTotalString;
		String sLine;
		Int iCount = 0;
		while (file.ReadLine(sLine))
		{
			iCount++;
			sTotalString += sLine;
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, iCount);
		SEOUL_UNITTESTING_ASSERT_EQUAL(kExpectedString, sTotalString);
	}
	// end read test
}

/** Pseudo-file class which doesn't actually read from/write to disk */
class TestSyncFile : public SyncFile
{
public:
	TestSyncFile()
		: m_uExpectedReadSize(0)
		, m_uExpectedWriteSize(0)
		, m_uReadOffset(0)
	{
	}

	virtual UInt32 ReadRawData(void *pOut, UInt32 uSizeInBytes) SEOUL_OVERRIDE
	{
		if (m_uExpectedReadSize != 0)
		{
			SEOUL_UNITTESTING_ASSERT(uSizeInBytes >= m_uExpectedReadSize);
		}

		UInt32 uBytesToRead = Min(uSizeInBytes, m_vDataToBeRead.GetSize() - m_uReadOffset);
		if (uBytesToRead > 0)
		{
			memcpy(pOut, m_vDataToBeRead.Get(m_uReadOffset), uBytesToRead);
			m_uReadOffset += uBytesToRead;
		}

		return uBytesToRead;
	}

	virtual UInt32 WriteRawData(const void *pIn, UInt32 uSizeInBytes) SEOUL_OVERRIDE
	{
		if (m_uExpectedWriteSize != 0)
		{
			SEOUL_UNITTESTING_ASSERT(uSizeInBytes >= m_uExpectedWriteSize);
		}

		const Byte *pData = (const Byte *)pIn;
		m_vDataWritten.Insert(m_vDataWritten.End(), pData, pData + uSizeInBytes);
		return uSizeInBytes;
	}

	virtual String GetAbsoluteFilename() const SEOUL_OVERRIDE
	{
		SEOUL_UNITTESTING_FAIL("GetAbsoluteFilename() should not be called");
		return String();
	}

	virtual Bool IsOpen() const SEOUL_OVERRIDE
	{
		return true;
	}

	virtual Bool CanRead() const SEOUL_OVERRIDE
	{
		return true;
	}

	virtual Bool CanWrite() const SEOUL_OVERRIDE
	{
		return true;
	}

	virtual Bool Flush() SEOUL_OVERRIDE
	{
		return true;
	}

	virtual UInt64 GetSize() const SEOUL_OVERRIDE
	{
		SEOUL_UNITTESTING_FAIL("GetSize() should not be called");
		return 0;
	}

	virtual Bool CanSeek() const SEOUL_OVERRIDE
	{
		return true;
	}

	virtual Bool GetCurrentPositionIndicator(Int64& riPosition) const SEOUL_OVERRIDE
	{
		return false;
	}

	virtual Bool Seek(Int64 iPosition, File::SeekMode eMode) SEOUL_OVERRIDE
	{
		switch(eMode)
		{
		case File::kSeekFromStart:
			SEOUL_UNITTESTING_ASSERT(iPosition >= 0);
			SEOUL_UNITTESTING_ASSERT(iPosition <= m_vDataToBeRead.GetSize());
			SEOUL_UNITTESTING_ASSERT(iPosition <= UINT32_MAX);
			m_uReadOffset = (UInt32)iPosition;
			return true;

		case File::kSeekFromCurrent:
			SEOUL_UNITTESTING_ASSERT((Int64)m_uReadOffset + iPosition >= 0);
			SEOUL_UNITTESTING_ASSERT((Int64)m_uReadOffset + iPosition <= (Int64)m_vDataToBeRead.GetSize());
			SEOUL_UNITTESTING_ASSERT((Int64)m_uReadOffset + iPosition <= UINT32_MAX);
			m_uReadOffset = (UInt32)(m_uReadOffset + iPosition);
			return true;

		case File::kSeekFromEnd:
			SEOUL_UNITTESTING_ASSERT(iPosition <= 0);
			SEOUL_UNITTESTING_ASSERT((Int64)m_uReadOffset + iPosition >= 0);
			m_uReadOffset = (UInt32)(m_vDataToBeRead.GetSize() + iPosition);
			return true;

		default:
			SEOUL_UNITTESTING_FAIL("Invalid enum");
			return false;
		}
	}

public:
	UInt32 m_uExpectedReadSize;
	UInt32 m_uExpectedWriteSize;
	Vector<Byte> m_vDataToBeRead;
	UInt32 m_uReadOffset;
	Vector<Byte> m_vDataWritten;
};

void SeoulFileTest::TestReadWriteBufferedSyncFile()
{
	UInt32 uBufferSizes[] = {0, 1, 8, 32, 4096};
	for (size_t i = 0; i < SEOUL_ARRAY_COUNT(uBufferSizes); i++)
	{
		TestSyncFile rawFile;
		const int N = 100;

		// Write the file
		{
			rawFile.m_uExpectedWriteSize = uBufferSizes[i];

			BufferedSyncFile bufferedFile(&rawFile, false, uBufferSizes[i]);

			char testData[N+1];
			memset(testData, 'a', N);
			testData[N] = '\n';
			for (int j = 1; j <= N; j++)
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL((UInt32)j, bufferedFile.WriteRawData(testData + N + 1 - j, j));
			}

			bufferedFile.Printf("foo %d %c %s\n", 42, '@', "bar");
			bufferedFile.Printf("this is a long line: %64s.", "see?");

			rawFile.m_uExpectedWriteSize = 0;
		}

		// Verify that the contents were written correctly
		const char sExpectedRemaining[] = "foo 42 @ bar\nthis is a long line:                                                             see?.";
		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt32)(N*(N+1)/2 + sizeof(sExpectedRemaining)-1), rawFile.m_vDataWritten.GetSize());
		UInt32 uOffset = 0;
		for (int j = 1; j <= N; j++)
		{
			for (int k = 0; k < j-1; k++)
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL('a', rawFile.m_vDataWritten[uOffset]);
				uOffset++;
			}

			SEOUL_UNITTESTING_ASSERT_EQUAL('\n', rawFile.m_vDataWritten[uOffset]);
			uOffset++;
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(rawFile.m_vDataWritten.Get(uOffset), sExpectedRemaining, sizeof(sExpectedRemaining)-1));

		// Read the file back in
		rawFile.m_vDataToBeRead.Swap(rawFile.m_vDataWritten);

		{
			rawFile.m_uExpectedReadSize = uBufferSizes[i];

			BufferedSyncFile bufferedFile(&rawFile, false, uBufferSizes[i]);

			char testData[N+1];
			for (int j = 1; j <= N; j++)
			{
				SEOUL_UNITTESTING_ASSERT_EQUAL((UInt32)j, bufferedFile.ReadRawData(testData, j));
				for (int k = 0; k < j-1; k++)
				{
					SEOUL_UNITTESTING_ASSERT_EQUAL('a', testData[k]);
				}
				SEOUL_UNITTESTING_ASSERT_EQUAL('\n', testData[j-1]);
			}

			SEOUL_UNITTESTING_ASSERT(bufferedFile.Seek(0, File::kSeekFromStart));

			String sLine;
			for (int j = 1; j <= N; j++)
			{
				SEOUL_UNITTESTING_ASSERT(bufferedFile.ReadLine(sLine));
				SEOUL_UNITTESTING_ASSERT_EQUAL((UInt32)j, sLine.GetSize());
				for (int k = 0; k < j-1; k++)
				{
					SEOUL_UNITTESTING_ASSERT_EQUAL('a', sLine[k]);
				}
				SEOUL_UNITTESTING_ASSERT_EQUAL('\n', sLine[j-1]);

			}

			SEOUL_UNITTESTING_ASSERT(bufferedFile.ReadLine(sLine));
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(sLine.CStr(), sExpectedRemaining, 13));

			SEOUL_UNITTESTING_ASSERT(bufferedFile.ReadLine(sLine));
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(sLine.CStr(), sExpectedRemaining + 13, sizeof(sExpectedRemaining)-1 - 13));

			SEOUL_UNITTESTING_ASSERT(!bufferedFile.ReadLine(sLine));
			SEOUL_UNITTESTING_ASSERT_EQUAL(0u, bufferedFile.ReadRawData(testData, N));

		}
	}
}

void SeoulFileTest::TestUtilityFunctions()
{
	static const Byte kTestData[] =
	{
		"float4 FragmentDownsample(vsScreenSpaceQuadNoRayOut input) : COLOR0"
		"{"
			"static const float kSizeRatio = 4.0;"

			"const float2 baseUV = input.TexCoords;"
			"const float2 fullSizeDimensions = (kSizeRatio * GetScreenDimensions());"

			"const float4 tap0 = OffsetTex2D(ColorSampler, baseUV, float2(-1.0, -1.0), fullSizeDimensions);"
			"const float4 tap1 = OffsetTex2D(ColorSampler, baseUV, float2(-1.0,  1.0), fullSizeDimensions);"
			"const float4 tap2 = OffsetTex2D(ColorSampler, baseUV, float2( 1.0, -1.0), fullSizeDimensions);"
			"const float4 tap3 = OffsetTex2D(ColorSampler, baseUV, float2( 1.0,  1.0), fullSizeDimensions);"

			"const float4 tapAverage = (tap0 + tap1 + tap2 + tap3) / 4.0;"
			"const float4 ret = float4(saturate(tapAverage.rgb - float3(BloomThreshold, BloomThreshold, BloomThreshold)), tapAverage.a);"

			"return ret;"
		"}"
	};

	String const sTempFilename(Path::GetTempFileAbsoluteFilename());

	// write test data
	{
		DiskSyncFile file(sTempFilename, File::kWriteTruncate);
		if (!file.CanWrite())
		{
			SEOUL_LOG("%s: is being skipped because a temp file could not be generated.", __FUNCTION__);
			return;
		}

		SEOUL_UNITTESTING_ASSERT(sizeof(kTestData) == file.WriteRawData(kTestData, sizeof(kTestData)));
	}

	// read all test
	{
		void* pAllDataBuffer = nullptr;
		UInt32 zSizeInBytes = 0u;
		SEOUL_UNITTESTING_ASSERT(DiskSyncFile::ReadAll(
			sTempFilename,
			pAllDataBuffer,
			zSizeInBytes,
			0u,
			MemoryBudgets::TBD));
		SEOUL_UNITTESTING_ASSERT_EQUAL(sizeof(kTestData), (size_t)zSizeInBytes);
		SEOUL_UNITTESTING_ASSERT(0 == memcmp(kTestData, pAllDataBuffer, sizeof(kTestData)));
		MemoryManager::Deallocate(pAllDataBuffer);
	}

	// directory exists test
	{
		SEOUL_UNITTESTING_ASSERT(!Directory::DirectoryExists(sTempFilename));
		SEOUL_UNITTESTING_ASSERT(Directory::DirectoryExists(Path::GetDirectoryName(sTempFilename)));
	}

	// file exists test
	{
		SEOUL_UNITTESTING_ASSERT(DiskSyncFile::FileExists(sTempFilename));
		SEOUL_UNITTESTING_ASSERT(!DiskSyncFile::FileExists(Path::GetDirectoryName(sTempFilename)));
	}

	// file size test
	{
		SEOUL_UNITTESTING_ASSERT(DiskSyncFile::GetFileSize(sTempFilename) == sizeof(kTestData));
	}

	// file modified time test
	{
		SEOUL_UNITTESTING_ASSERT(0u != DiskSyncFile::GetModifiedTime(sTempFilename));
	}

	// file listing test
	{
		Vector<String> vResults;
		SEOUL_UNITTESTING_ASSERT(Directory::GetDirectoryListing(
			Path::GetDirectoryName(sTempFilename),
			vResults,
			false,
			false,
			Path::GetExtension(sTempFilename)));
		SEOUL_UNITTESTING_ASSERT(!vResults.IsEmpty());

		UInt32 nResultsCount = vResults.GetSize();
		for (UInt32 i = 0u; i < nResultsCount; ++i)
		{
			if (Path::GetFileName(vResults[i]).ToLowerASCII() ==
				Path::GetFileName(sTempFilename).ToLowerASCII())
			{
				return;
			}
		}

		SEOUL_UNITTESTING_ASSERT_MESSAGE(false, "Temp file not in list of results.");
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
