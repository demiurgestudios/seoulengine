/**
 * \file SeoulFileReaders.cpp
 * \brief Provides global functions to read common Seoul Engine data
 * types in a checked manner.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DataStoreParser.h"
#include "Matrix3x4.h"
#include "SeoulFile.h"
#include "SeoulFileReaders.h"
#include "Vector.h"

namespace Seoul
{

Bool ReadBoolean(SyncFile& file, Bool& rbOut)
{
	UInt8 value;
	if (ReadUInt8(file, value))
	{
		rbOut = ((0u == value) ? false : true);
		return true;
	}

	return false;
}

Bool ReadAABB(SyncFile& file, AABB& rOut)
{
	return (
		ReadVector3D(file, rOut.m_vMin) &&
		ReadVector3D(file, rOut.m_vMax));
}

Bool ReadSphere(SyncFile& file, Sphere& rOut)
{
	return (
		ReadVector3D(file, rOut.m_vCenter) &&
		ReadSingle(file, rOut.m_fRadius));
}

Bool ReadInt8(SyncFile& file, Int8& riOut)
{
	return (file.ReadRawData(&riOut, sizeof(Int8)) == sizeof(Int8));
}

Bool ReadUInt8(SyncFile& file, UInt8& rOut)
{
	return (file.ReadRawData(&rOut, sizeof(UInt8)) == sizeof(UInt8));
}

Bool ReadInt16(SyncFile& file, Int16& riOut)
{
	return (file.ReadRawData(&riOut, sizeof(Int16)) == sizeof(Int16));
}

Bool ReadUInt16(SyncFile& file, UInt16& rOut)
{
	return (file.ReadRawData(&rOut, sizeof(UInt16)) == sizeof(UInt16));
}

Bool ReadInt32(SyncFile& file, Int32& riOut)
{
	return (file.ReadRawData(&riOut, sizeof(Int32)) == sizeof(Int32));
}

Bool ReadUInt64(SyncFile& file, UInt64& rOut)
{
	return (file.ReadRawData(&rOut, sizeof(UInt64)) == sizeof(UInt64));
}

Bool ReadInt64(SyncFile& file, Int64& riOut)
{
	return (file.ReadRawData(&riOut, sizeof(Int64)) == sizeof(Int64));
}

Bool ReadUInt32(SyncFile& file, UInt32& rOut)
{
	return (file.ReadRawData(&rOut, sizeof(UInt32)) == sizeof(UInt32));
}

Bool ReadString(SyncFile& file, String& rsOut, UInt32 uMaxReadSize)
{
	Vector<Byte, MemoryBudgets::Io> buf;
	if (ReadBuffer<Byte>(file, buf, uMaxReadSize))
	{
		// Verify that the input string was written correctly and has
		// a null terminator.
		if (!(buf.IsEmpty() || '\0' == buf[buf.GetSize() - 1u]))
		{
			return false;
		}

		// If an empty buffer, return the empty string.
		if (buf.IsEmpty())
		{
			rsOut.Clear();
		}
		// Otherwise populate the string from the buffer data.
		else
		{
			rsOut.Assign(buf.Get(0u), buf.GetSize());
		}

		return true;
	}

	return false;
}

Bool ReadHString(SyncFile& file, HString& rsOut, UInt32 uMaxReadSize)
{
	Vector<Byte> buf;
	if (ReadBuffer<Byte>(file, buf, uMaxReadSize))
	{
		// Verify that the input string was written correctly and has
		// a null terminator.
		SEOUL_ASSERT(buf.IsEmpty() || '\0' == buf[buf.GetSize() - 1u]);

		rsOut = HString(buf.Get(0u));
		return true;
	}

	return false;
}

Bool ReadMatrix3x4(SyncFile& file, Matrix3x4& rOut)
{
	return (file.ReadRawData(rOut.M, sizeof(Float) * 12u) == sizeof(Float) * 12u);
}

Bool ReadMatrix4D(SyncFile& file, Matrix4D& rOut)
{
	Matrix4D m;
	if (file.ReadRawData(m.GetData(), sizeof(Float) * 16u) == sizeof(Float) * 16u)
	{
		// Data is serialized in row-major, but Matrix4Ds are column-major,
		// so we transpose the matrix before returning.
		rOut = m.Transpose();

		return true;
	}

	return false;
}

Bool ReadSingle(SyncFile& file, Float& rfOut)
{
	return (file.ReadRawData(&rfOut, sizeof(Float)) == sizeof(Float));
}

Bool ReadVector2D(SyncFile& file, Vector2D& rvOut)
{
	return (file.ReadRawData(rvOut.GetData(), sizeof(Float) * 2u) == sizeof(Float) * 2u);
}

Bool ReadVector3D(SyncFile& file, Vector3D& rvOut)
{
	return (file.ReadRawData(rvOut.GetData(), sizeof(Float) * 3u) == sizeof(Float) * 3u);
}

Bool ReadVector4D(SyncFile& file, Vector4D& rvOut)
{
	return (file.ReadRawData(rvOut.GetData(), sizeof(Float) * 4u) == sizeof(Float) * 4u);
}

/**
 * Helper method, reads in a string as a FilePath data structure.
 *
 * @param[in] file File stream to read from.
 * @param[in] eDirectory Base directory of the file path.
 * @param[out] rFilePath FilePath structure to store results in.
 *
 * @return True if reading was successful, false otherwise. This
 * function can return false if reading failed, or if the resulting
 * file path is invalid. If this function returns false, the state
 * of the file pointer of file is undefined.
 */
Bool ReadFilePath(SyncFile& file, GameDirectory eDirectory, FilePath& rFilePath)
{
	String s;

	// The filename can be a relative filename, absolute filename,
	// or a SeoulEngine style content:// specifier.
	if (ReadString(file, s))
	{
		// If the filename is not empty, we try to construct
		// a file path from the string.
		if (!s.IsEmpty())
		{
			// First, try to parse it as a specifier.
			{
				if (DataStoreParser::StringAsFilePath(s, rFilePath))
				{
					return true;
				}
			}

			FilePath ret = FilePath::CreateFilePath(eDirectory, s);

			// If the file path is valid, set the path and return true.
			if (ret.IsValid())
			{
				rFilePath = ret;
			}
			// Otherwise, no file path, but indicate that the read was successful.
			else
			{
				rFilePath.Reset();
			}

			return true;
		}
		// If the filename is empty, this is valid. We reset the file path
		// and return true.
		else
		{
			rFilePath.Reset();
			return true;
		}
	}

	// Either the file path read failed, or the file path was invalid.
	return false;
}

} // namespace Seoul
