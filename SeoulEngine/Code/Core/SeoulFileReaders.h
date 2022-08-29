/**
 * \file SeoulFileReaders.h
 * \brief Provides global functions to read common Seoul Engine data
 * types in a checked manner.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SEOUL_FILE_READERS_H
#define SEOUL_FILE_READERS_H

#include "Color.h"
#include "FilePath.h"
#include "Geometry.h"
#include "Matrix4D.h"
#include "Quaternion.h"
#include "SeoulFile.h"
#include "SeoulHString.h"
#include "SeoulString.h"
#include "Vector2D.h"
#include "Vector3D.h"
#include "Vector4D.h"

namespace Seoul
{

Bool ReadBoolean(SyncFile& file, Bool& rbOut);
Bool ReadAABB(SyncFile& file, AABB& rOut);
Bool ReadFilePath(SyncFile& file, GameDirectory eDirectory, FilePath& rFilePath);
Bool ReadSphere(SyncFile& file, Sphere& rOut);
Bool ReadInt8(SyncFile& file, Int8& riOut);
Bool ReadUInt8(SyncFile& file, UInt8& rOut);
Bool ReadInt16(SyncFile& file, Int16& riOut);
Bool ReadUInt16(SyncFile& file, UInt16& rOut);
Bool ReadInt32(SyncFile& file, Int32& riOut);
Bool ReadUInt32(SyncFile& file, UInt32& rOut);
Bool ReadInt64(SyncFile& file, Int64& riOut);
Bool ReadUInt64(SyncFile& file, UInt64& rOut);
Bool ReadString(SyncFile& file, String& rsOut, UInt32 uMaxReadSize = kDefaultMaxReadSize);
Bool ReadHString(SyncFile& file, HString& rsOut, UInt32 uMaxReadSize = kDefaultMaxReadSize);
Bool ReadMatrix3x4(SyncFile& file, Matrix3x4& rOut);
Bool ReadMatrix4D(SyncFile& file, Matrix4D& rOut);
Bool ReadSingle(SyncFile& file, Float& rfOut);
Bool ReadVector2D(SyncFile& file, Vector2D& rvOut);
Bool ReadVector3D(SyncFile& file, Vector3D& rvOut);
Bool ReadVector4D(SyncFile& file, Vector4D& rvOut);

inline Bool ReadQuaternion(SyncFile& file, Quaternion& rqOut)
{
	Vector4D v;
	if (!ReadVector4D(file, v))
	{
		return false;
	}

	rqOut = Quaternion(v.X, v.Y, v.Z, v.W);
	return true;
}

template <typename T>
inline Bool ReadUInt8(SyncFile& file, T& rvOut)
{
	UInt8 uValue;
	if (ReadUInt8(file, uValue))
	{
		rvOut = (T)uValue;
		return true;
	}

	return false;
}

template <typename T>
inline Bool ReadUInt16(SyncFile& file, T& rvOut)
{
	UInt16 uValue;
	if (ReadUInt16(file, uValue))
	{
		rvOut = (T)uValue;
		return true;
	}

	return false;
}

template <typename T>
inline Bool ReadUInt32(SyncFile& file, T& rvOut)
{
	UInt32 uValue;
	if (ReadUInt32(file, uValue))
	{
		rvOut = (T)uValue;
		return true;
	}

	return false;
}

inline Bool ReadVector4D(SyncFile& file, Color4& rOut)
{
	Vector4D v;
	if (ReadVector4D(file, v))
	{
		rOut.R = v.X;
		rOut.G = v.Y;
		rOut.B = v.Z;
		rOut.A = v.W;
		return true;
	}

	return false;
}

inline Bool ReadVector3D(SyncFile& file, Color4& rOut)
{
	Vector3D v;
	if (ReadVector3D(file, v))
	{
		rOut.R = v.X;
		rOut.G = v.Y;
		rOut.B = v.Z;
		rOut.A = 1.0f;
		return true;
	}

	return false;
}

template <typename T>
Bool ReadEnum(SyncFile& file, T& reOut)
{
	UInt32 v;
	if (ReadUInt32(file, v))
	{
		reOut = (T)v;
		return true;
	}

	return false;
}

template <typename T, int MEMORY_BUDGETS>
Bool ReadBuffer(SyncFile& file, Vector<T, MEMORY_BUDGETS>& rvOut, UInt32 uMaxReadSize = kDefaultMaxReadSize)
{
	SEOUL_STATIC_ASSERT_MESSAGE(CanMemCpy<T>::Value, "SyncFile ReadBuffer(Vector<T>) can only be used on a type T that is memcpyable.");

	UInt32 size = 0u;
	if (!ReadUInt32(file, size))
	{
		return false;
	}

	if (size > uMaxReadSize)
	{
		return false;
	}

	rvOut.Resize(size);
	if (size == 0u)
	{
		return true;
	}

	if (file.ReadRawData(rvOut.Get(0u), sizeof(T) * size) != sizeof(T) * size)
	{
		return false;
	}

	return true;
}

// Constant declarations and helpers for checking data during deserialization.
/**
 * All chunks of data are delimited with a header that includes a special
 * character (@see ChunkDelimiter) and a number that identifies the type
 * of data. The list of values in FileChunkDataTypes requires a specific
 * order for each version of the file format.
 *
 * If you change this list you must also change the file format version.
 */
enum EDataDelimiter
{
	DataTypeScene = 3000,
	DataTypeMaterial,
	DataTypeMaterialLibrary,
	DataTypeMesh,
	DataTypeBone,
	DataTypeNurbsCurve,
	DataTypeLight,
	DataTypeVertexDecl,
	DataTypePrimitiveGroup,
	DataTypeMaterialParameter,
	DataTypeVertexElement,
	DataTypeAnimationClip,
	DataTypeAnimationSkeleton,
};

/*
 * Some chunks of data exist only to contain other chunks. These container
 * classes are indicated with unique types to simplify deserialization.
 * As with FileChunkDataTypes, any changes to this list must be accompanied
 * by a new file format version number.
 */
enum EListDelimiter
{
	ListTypeMaterial = 2000,
	ListTypeMesh,
	ListTypeBone,
	ListTypeNurbsCurve,
	ListTypeLight,
	ListTypePrimitiveGroup,
	ListTypeMaterialParameter,
	ListTypeVertexElement
};

/**
 * SOL file types - either a level or an object. Object files do
 * not contain data such as nurbs curves and lights while level files do
 * not contain data such as bones.
 */
enum ESeoulFileType
{
	FileType_Level = 0,
	FileType_Object = 1,
	FileType_Unknown
};

/**
 * Reads the next 4-bytes from file and returns true
 * if the deserialized tag matches the iTypeCode tag. Otherwise,
 * returns false.
 */
inline Bool VerifyDelimiter(Int32 iTypeCode, SyncFile& file)
{
	Int32 iTag;
	if (ReadInt32(file, iTag))
	{
		return (iTag == iTypeCode);
	}

	return false;
}

} // namespace Seoul

#endif // include guard
