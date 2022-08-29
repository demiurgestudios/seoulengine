/**
 * \file SeoulFileWriters.h
 * \brief Provides global functions to write common Seoul Engine data
 * types in a checked manner.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SEOUL_FILE_WRITERS_H
#define SEOUL_FILE_WRITERS_H

#include "Color.h"
#include "FilePath.h"
#include "Geometry.h"
#include "Matrix4D.h"
#include "Quaternion.h"
#include "SeoulHString.h"
#include "SeoulString.h"
#include "Vector2D.h"
#include "Vector3D.h"
#include "Vector4D.h"

namespace Seoul
{

class SyncFile;
Bool WriteBoolean(SyncFile& file, Bool bIn);
Bool WriteAABB(SyncFile& file, const AABB& in);
Bool WriteSphere(SyncFile& file, const Sphere& in);
Bool WriteInt8(SyncFile& file, Int8 iIn);
Bool WriteUInt8(SyncFile& file, UInt8 uIn);
Bool WriteInt16(SyncFile& file, Int16 iIn);
Bool WriteUInt16(SyncFile& file, UInt16 uIn);
Bool WriteInt32(SyncFile& file, Int32 iIn);
Bool WriteUInt32(SyncFile& file, UInt32 uIn);
Bool WriteInt64(SyncFile& file, Int64 iIn);
Bool WriteUInt64(SyncFile& file, UInt64 uIn);
Bool WriteString(SyncFile& file, const String& sIn);
Bool WriteHString(SyncFile& file, HString in);
Bool WriteMatrix3x4(SyncFile& file, const Matrix3x4& mIn);
Bool WriteMatrix4D(SyncFile& file, const Matrix4D& mIn);
Bool WriteSingle(SyncFile& file, Float fIn);
Bool WriteVector2D(SyncFile& file, const Vector2D& vIn);
Bool WriteVector3D(SyncFile& file, const Vector3D& vIn);
Bool WriteVector4D(SyncFile& file, const Vector4D& vIn);

inline Bool WriteQuaternion(SyncFile& file, const Quaternion& qIn)
{
	return WriteVector4D(file, Vector4D(qIn.X, qIn.Y, qIn.Z, qIn.W));
}

/**
 * Helper function, serializes a FilePath filePath
 * to the file file.
 */
inline Bool WriteFilePath(SyncFile& file, FilePath filePath)
{
	return WriteString(file, filePath.GetRelativeFilename());
}

inline Bool WriteVector4D(SyncFile& file, const Color4& in)
{
	Vector4D v(in.R, in.G, in.B, in.A);

	return WriteVector4D(file, v);
}

inline Bool WriteVector3D(SyncFile& file, const Color4& in)
{
	Vector3D v(in.R, in.G, in.B);

	return WriteVector3D(file, v);
}

template <typename T>
Bool WriteEnum(SyncFile& file, T eIn)
{
	UInt32 v = (UInt32)eIn;

	return WriteUInt32(file, v);
}

template <typename T, int MEMORY_BUDGETS>
Bool WriteBuffer(SyncFile& file, const Vector<T, MEMORY_BUDGETS>& vIn)
{
	SEOUL_STATIC_ASSERT_MESSAGE(CanMemCpy<T>::Value, "SyncFile WriteBuffer(Vector<T>) can only be used on a type T that is memcpyable.");

	UInt32 const size = vIn.GetSize();
	if (!WriteUInt32(file, size))
	{
		return false;
	}

	if (0u == size)
	{
		return true;
	}

	return (file.WriteRawData(vIn.Get(0u), sizeof(T) * size) == sizeof(T) * size);
}

} // namespace Seoul

#endif // include guard
