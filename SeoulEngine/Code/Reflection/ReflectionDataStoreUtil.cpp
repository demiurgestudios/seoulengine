/**
 * \file ReflectionDataStoreUtil.cpp
 * \brief Utilities for convert data in a DataStore to/from concrete
 * C++ types using Reflection.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionDataStoreUtil.h"
#include "ReflectionDefine.h"

namespace Seoul
{

static inline UInt32 HexValue(Byte c)
{
	switch (c)
	{
	case 'F': case 'f': return 15u;
	case 'E': case 'e': return 14u;
	case 'D': case 'd': return 13u;
	case 'C': case 'c': return 12u;
	case 'B': case 'b': return 11u;
	case 'A': case 'a': return 10u;
	case '9': return 9u;
	case '8': return 8u;
	case '7': return 7u;
	case '6': return 6u;
	case '5': return 5u;
	case '4': return 4u;
	case '3': return 3u;
	case '2': return 2u;
	case '1': return 1u;
	case '0': return 0u;
	default:
		return 0u;
	};
}

static inline UInt8 ColorValue(Byte const* s)
{
	return (UInt8)(HexValue(s[0]) * 16u + HexValue(s[1]));
}

Bool DataNodeHandler<RGBA, false>::FromDataNode(Reflection::SerializeContext&, const DataStore& dataStore, const DataNode& dataNode, RGBA& rValue)
{
	Byte const* s = nullptr;
	UInt32 u = 0u;
	if (!dataStore.AsString(dataNode, s, u) || 8u != u)
	{
		return false;
	}

	rValue.m_R = ColorValue(s + 0);
	rValue.m_G = ColorValue(s + 2);
	rValue.m_B = ColorValue(s + 4);
	rValue.m_A = ColorValue(s + 6);
	return true;
}

Bool DataNodeHandler<RGBA, false>::ToArray(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, RGBA rgba)
{
	return rDataStore.SetStringToArray(
		array,
		uIndex,
		String::Printf("%02x%02x%02x%02x", (UInt32)rgba.m_R, (UInt32)rgba.m_G, (UInt32)rgba.m_B, (UInt32)rgba.m_A));
}

Bool DataNodeHandler<RGBA, false>::ToTable(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& table, HString key, RGBA rgba)
{
	return rDataStore.SetStringToTable(
		table,
		key,
		String::Printf("%02x%02x%02x%02x", (UInt32)rgba.m_R, (UInt32)rgba.m_G, (UInt32)rgba.m_B, (UInt32)rgba.m_A));
}

Bool DataNodeHandler<RGBA, false>::FromScript(lua_State* pVm, Int iOffset, RGBA& r)
{
	SEOUL_FAIL("TODO: Implement.");
	return false;
}

void DataNodeHandler<RGBA, false>::ToScript(lua_State* pVm, const RGBA& v)
{
	SEOUL_FAIL("TODO: Implement.");
}

struct Matrix4DFromTransform SEOUL_SEALED
{
	Matrix4DFromTransform()
		: m_qRotation(Quaternion::Identity())
		, m_vPosition(Vector3D::Zero())
		, m_vScale(Vector3D::One())
	{
	}

	Matrix4D ToMatrix4D() const
	{
		Matrix4D const mReturn =
			Matrix4D::CreateTranslation(m_vPosition) *
			m_qRotation.GetMatrix4D() *
			Matrix4D::CreateScale(m_vScale);

		return mReturn;
	}

	Quaternion m_qRotation;
	Vector3D m_vPosition;
	Vector3D m_vScale;
}; // struct Matrix4DFromTransform

SEOUL_BEGIN_TYPE(Matrix4DFromTransform)
	SEOUL_ATTRIBUTE(NotRequired)
	SEOUL_PROPERTY_N("Position", m_vPosition)
	SEOUL_PROPERTY_N("Rotation", m_qRotation)
	SEOUL_PROPERTY_N("Scale", m_vScale)
SEOUL_END_TYPE()

Bool DataNodeHandler<Matrix4D, false>::FromDataNode(Reflection::SerializeContext& context, const DataStore& dataStore, const DataNode& dataNode, Matrix4D& rmValue)
{
	if (dataNode.IsArray())
	{
		DataNode value;
		return (
			dataStore.GetValueFromArray(dataNode, 0u, value) && dataStore.AsFloat32(value, rmValue.M00) &&
			dataStore.GetValueFromArray(dataNode, 1u, value) && dataStore.AsFloat32(value, rmValue.M01) &&
			dataStore.GetValueFromArray(dataNode, 2u, value) && dataStore.AsFloat32(value, rmValue.M02) &&
			dataStore.GetValueFromArray(dataNode, 3u, value) && dataStore.AsFloat32(value, rmValue.M03) &&
			dataStore.GetValueFromArray(dataNode, 4u, value) && dataStore.AsFloat32(value, rmValue.M10) &&
			dataStore.GetValueFromArray(dataNode, 5u, value) && dataStore.AsFloat32(value, rmValue.M11) &&
			dataStore.GetValueFromArray(dataNode, 6u, value) && dataStore.AsFloat32(value, rmValue.M12) &&
			dataStore.GetValueFromArray(dataNode, 7u, value) && dataStore.AsFloat32(value, rmValue.M13) &&
			dataStore.GetValueFromArray(dataNode, 8u, value) && dataStore.AsFloat32(value, rmValue.M20) &&
			dataStore.GetValueFromArray(dataNode, 9u, value) && dataStore.AsFloat32(value, rmValue.M21) &&
			dataStore.GetValueFromArray(dataNode, 10u, value) && dataStore.AsFloat32(value, rmValue.M22) &&
			dataStore.GetValueFromArray(dataNode, 11u, value) && dataStore.AsFloat32(value, rmValue.M23) &&
			dataStore.GetValueFromArray(dataNode, 12u, value) && dataStore.AsFloat32(value, rmValue.M30) &&
			dataStore.GetValueFromArray(dataNode, 13u, value) && dataStore.AsFloat32(value, rmValue.M31) &&
			dataStore.GetValueFromArray(dataNode, 14u, value) && dataStore.AsFloat32(value, rmValue.M32) &&
			dataStore.GetValueFromArray(dataNode, 15u, value) && dataStore.AsFloat32(value, rmValue.M33));
	}
	else if (dataNode.IsTable())
	{
		Matrix4DFromTransform mTransform;
		if (!Reflection::DeserializeObject(context, dataStore, dataNode, &mTransform))
		{
			return false;
		}

		rmValue = mTransform.ToMatrix4D();
		return true;
	}
	else
	{
		return false;
	}
}

Bool DataNodeHandler<Matrix4D, false>::ToArray(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, const Matrix4D& mValue)
{
	DataNode matrixArray;
	return (
		rDataStore.SetArrayToArray(array, uIndex, 16u) &&
		rDataStore.GetValueFromArray(array, uIndex, matrixArray) &&
		rDataStore.SetFloat32ValueToArray(matrixArray, 0u, mValue.M00) &&
		rDataStore.SetFloat32ValueToArray(matrixArray, 1u, mValue.M01) &&
		rDataStore.SetFloat32ValueToArray(matrixArray, 2u, mValue.M02) &&
		rDataStore.SetFloat32ValueToArray(matrixArray, 3u, mValue.M03) &&
		rDataStore.SetFloat32ValueToArray(matrixArray, 4u, mValue.M10) &&
		rDataStore.SetFloat32ValueToArray(matrixArray, 5u, mValue.M11) &&
		rDataStore.SetFloat32ValueToArray(matrixArray, 6u, mValue.M12) &&
		rDataStore.SetFloat32ValueToArray(matrixArray, 7u, mValue.M13) &&
		rDataStore.SetFloat32ValueToArray(matrixArray, 8u, mValue.M20) &&
		rDataStore.SetFloat32ValueToArray(matrixArray, 9u, mValue.M21) &&
		rDataStore.SetFloat32ValueToArray(matrixArray, 10u, mValue.M22) &&
		rDataStore.SetFloat32ValueToArray(matrixArray, 11u, mValue.M23) &&
		rDataStore.SetFloat32ValueToArray(matrixArray, 12u, mValue.M30) &&
		rDataStore.SetFloat32ValueToArray(matrixArray, 13u, mValue.M31) &&
		rDataStore.SetFloat32ValueToArray(matrixArray, 14u, mValue.M32) &&
		rDataStore.SetFloat32ValueToArray(matrixArray, 15u, mValue.M33));
}

Bool DataNodeHandler<Matrix4D, false>::ToTable(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& table, HString key, const Matrix4D& mValue)
{
	DataNode matrixArray;
	return (
		rDataStore.SetArrayToTable(table, key, 16u) &&
		rDataStore.GetValueFromTable(table, key, matrixArray) &&
		rDataStore.SetFloat32ValueToArray(matrixArray, 0u, mValue.M00) &&
		rDataStore.SetFloat32ValueToArray(matrixArray, 1u, mValue.M01) &&
		rDataStore.SetFloat32ValueToArray(matrixArray, 2u, mValue.M02) &&
		rDataStore.SetFloat32ValueToArray(matrixArray, 3u, mValue.M03) &&
		rDataStore.SetFloat32ValueToArray(matrixArray, 4u, mValue.M10) &&
		rDataStore.SetFloat32ValueToArray(matrixArray, 5u, mValue.M11) &&
		rDataStore.SetFloat32ValueToArray(matrixArray, 6u, mValue.M12) &&
		rDataStore.SetFloat32ValueToArray(matrixArray, 7u, mValue.M13) &&
		rDataStore.SetFloat32ValueToArray(matrixArray, 8u, mValue.M20) &&
		rDataStore.SetFloat32ValueToArray(matrixArray, 9u, mValue.M21) &&
		rDataStore.SetFloat32ValueToArray(matrixArray, 10u, mValue.M22) &&
		rDataStore.SetFloat32ValueToArray(matrixArray, 11u, mValue.M23) &&
		rDataStore.SetFloat32ValueToArray(matrixArray, 12u, mValue.M30) &&
		rDataStore.SetFloat32ValueToArray(matrixArray, 13u, mValue.M31) &&
		rDataStore.SetFloat32ValueToArray(matrixArray, 14u, mValue.M32) &&
		rDataStore.SetFloat32ValueToArray(matrixArray, 15u, mValue.M33));
}

void DataNodeHandler<Matrix4D, false>::FromScript(lua_State* pVm, Int iOffset, Matrix4D& r)
{
	SEOUL_FAIL("TODO: Implement.");
}

void DataNodeHandler<Matrix4D, false>::ToScript(lua_State* pVm, const Matrix4D& v)
{
	SEOUL_FAIL("TODO: Implement.");
}

Bool DataNodeHandler<ContentKey, false>::FromDataNode(
	Reflection::SerializeContext&,
	const DataStore& dataStore,
	const DataNode& dataNode,
	ContentKey& rValue)
{
	return rValue.SetFromDataStore(dataStore, dataNode);
}

Bool DataNodeHandler<ContentKey, false>::ToArray(
	Reflection::SerializeContext&,
	DataStore& rDataStore,
	const DataNode& array,
	UInt32 uIndex,
	const ContentKey& value)
{
	return value.SetToDataStoreArray(rDataStore, array, uIndex);
}

Bool DataNodeHandler<ContentKey, false>::ToTable(
	Reflection::SerializeContext&,
	DataStore& rDataStore,
	const DataNode& table,
	HString key,
	const ContentKey& value)
{
	return value.SetToDataStoreTable(rDataStore, table, key);
}

void DataNodeHandler<ContentKey, false>::FromScript(lua_State* pVm, Int iOffset, ContentKey& r)
{
	SEOUL_FAIL("TODO: Implement.");
}

void DataNodeHandler<ContentKey, false>::ToScript(lua_State* pVm, const ContentKey& v)
{
	SEOUL_FAIL("TODO: Implement.");
}

Bool DataNodeHandler<UUID, false>::FromDataNode(Reflection::SerializeContext& context, const DataStore& dataStore, const DataNode& dataNode, UUID& r)
{
	String s;
	if (dataStore.AsString(dataNode, s))
	{
		r = UUID::FromString(s);
		return true;
	}

	return false;
}

Bool DataNodeHandler<UUID, false>::ToArray(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, const UUID& v)
{
	String const s(v.ToString());
	return rDataStore.SetStringToArray(array, uIndex, s);
}

Bool DataNodeHandler<UUID, false>::ToTable(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& table, HString key, const UUID& v)
{
	String const s(v.ToString());
	return rDataStore.SetStringToTable(table, key, s);
}

void DataNodeHandler<UUID, false>::FromScript(lua_State* pVm, Int iOffset, UUID& r)
{
	size_t z = 0;
	Byte const* s = lua_tolstring(pVm, iOffset, &z);
	String const ss(s, (UInt32)z);
	r = UUID::FromString(ss);
}

void DataNodeHandler<UUID, false>::ToScript(lua_State* pVm, const UUID& v)
{
	String const s(v.ToString());
	lua_pushlstring(pVm, s.CStr(), (size_t)s.GetSize());
}

} // namespace Seoul
