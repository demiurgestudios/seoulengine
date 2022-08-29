/**
 * \file SeoulFileWriters.cpp
 * \brief Provides global functions to write common Seoul Engine data
 * types in a checked manner.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Matrix3x4.h"
#include "SeoulFile.h"
#include "SeoulFileWriters.h"
#include "Vector.h"

namespace Seoul
{

Bool WriteBoolean(SyncFile& file, Bool bIn)
{
	UInt8 value = ((bIn) ? 1u : 0u);

	return WriteUInt8(file, value);
}

Bool WriteAABB(SyncFile& file, const AABB& in)
{
	return (
		WriteVector3D(file, in.m_vMin) &&
		WriteVector3D(file, in.m_vMax));
}

Bool WriteSphere(SyncFile& file, const Sphere& in)
{
	return (
		WriteVector3D(file, in.m_vCenter) &&
		WriteSingle(file, in.m_fRadius));
}

Bool WriteInt8(SyncFile& file, Int8 iIn)
{
	return (file.WriteRawData(&iIn, sizeof(Int8)) == sizeof(Int8));
}

Bool WriteUInt8(SyncFile& file, UInt8 uIn)
{
	return (file.WriteRawData(&uIn, sizeof(UInt8)) == sizeof(UInt8));
}

Bool WriteInt16(SyncFile& file, Int16 iIn)
{
	return (file.WriteRawData(&iIn, sizeof(Int16)) == sizeof(Int16));
}

Bool WriteUInt16(SyncFile& file, UInt16 uIn)
{
	return (file.WriteRawData(&uIn, sizeof(UInt16)) == sizeof(UInt16));
}

Bool WriteInt32(SyncFile& file, Int32 iIn)
{
	return (file.WriteRawData(&iIn, sizeof(Int32)) == sizeof(Int32));
}

Bool WriteUInt32(SyncFile& file, UInt32 uIn)
{
	return (file.WriteRawData(&uIn, sizeof(UInt32)) == sizeof(UInt32));
}

Bool WriteInt64(SyncFile& file, Int64 iIn)
{
	return (file.WriteRawData(&iIn, sizeof(Int64)) == sizeof(Int64));
}

Bool WriteUInt64(SyncFile& file, UInt64 uIn)
{
	return (file.WriteRawData(&uIn, sizeof(UInt64)) == sizeof(UInt64));
}

Bool WriteString(SyncFile& file, const String& sIn)
{
	Vector<Byte> buf(sIn.GetSize());
	if (!buf.IsEmpty())
	{
		memcpy(buf.Get(0u), sIn.CStr(), sIn.GetSize());
	}
	buf.PushBack('\0');

	return WriteBuffer<Byte>(file, buf);
}

Bool WriteHString(SyncFile& file, HString in)
{
	Vector<Byte> buf(in.GetSizeInBytes());
	if (!buf.IsEmpty())
	{
		memcpy(buf.Get(0u), in.CStr(), in.GetSizeInBytes());
	}
	buf.PushBack('\0');

	return WriteBuffer<Byte>(file, buf);
}

Bool WriteMatrix3x4(SyncFile& file, const Matrix3x4& mIn)
{
	return (file.WriteRawData(mIn.M, sizeof(Float) * 12u) == sizeof(Float) * 12u);
}

Bool WriteMatrix4D(SyncFile& file, const Matrix4D& mIn)
{
	// Data is serialized in row-major, but Matrix4Ds are column-major,
	// so we transpose the matrix before writing it.
	const Matrix4D m = mIn.Transpose();

	return (file.WriteRawData(m.GetData(), sizeof(float) * 16u) == sizeof(Float) * 16u);
}

Bool WriteSingle(SyncFile& file, Float fIn)
{
	return (file.WriteRawData(&fIn, sizeof(Float)) == sizeof(Float));
}

Bool WriteVector2D(SyncFile& file, const Vector2D& vIn)
{
	return (file.WriteRawData(vIn.GetData(), sizeof(Float) * 2u) == sizeof(Float) * 2u);
}

Bool WriteVector3D(SyncFile& file, const Vector3D& vIn)
{
	return (file.WriteRawData(vIn.GetData(), sizeof(Float) * 3u) == sizeof(Float) * 3u);
}

Bool WriteVector4D(SyncFile& file, const Vector4D& vIn)
{
	return (file.WriteRawData(vIn.GetData(), sizeof(Float) * 4u) == sizeof(Float) * 4u);
}

} // namespace Seoul
