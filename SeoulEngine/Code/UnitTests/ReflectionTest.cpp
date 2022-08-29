/**
 * \file ReflectionTest.cpp
 * \brief Unit tests to verify basic functionality of the Reflection
 * namespace.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "HashTable.h"
#include "Logger.h"
#include "ReflectionArray.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDeclare.h"
#include "ReflectionDefine.h"
#include "ReflectionRegistry.h"
#include "ReflectionSerialize.h"
#include "ReflectionTable.h"
#include "ReflectionTest.h"
#include "ScopedPtr.h"
#include "SeoulUUID.h"
#include "StandardVertex2D.h"
#include "StreamBuffer.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(ReflectionTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestReflectionDeserializeBasic)
	SEOUL_METHOD(TestReflectionDeserializeComplexProperties)
	SEOUL_METHOD(TestReflectionSerializeToArrayBasic)
	SEOUL_METHOD(TestReflectionSerializeToTableBasic)
	SEOUL_METHOD(TestReflectionTypeBasic)
	SEOUL_METHOD(TestReflectionTypeAdvanced)
	SEOUL_METHOD(TestReflectionTypeAttributes)
	SEOUL_METHOD(TestReflectionTypeConstructors)
	SEOUL_METHOD(TestReflectionArray)
	SEOUL_METHOD(TestReflectionConstArray)
	SEOUL_METHOD(TestReflectionFixedArray)
	SEOUL_METHOD(TestReflectionEnum)
	SEOUL_METHOD(TestReflectionTable)
	SEOUL_METHOD(TestReflectionConstTable)
	SEOUL_METHOD(TestReflectionTypeMethods)
	SEOUL_METHOD(TestReflectionTypeProperties)
	SEOUL_METHOD(TestReflectionFieldProperies)
SEOUL_END_TYPE();

namespace TestEnum
{
	enum Enum
	{
		kZero = 0,
		kOne,
		kTwo,
		kThree
	};
}

SEOUL_BEGIN_ENUM(TestEnum::Enum)
	SEOUL_ALIAS("zero", "Zero")
	SEOUL_ENUM_N("Zero", TestEnum::kZero)
	SEOUL_ENUM_N("One", TestEnum::kOne)
	SEOUL_ENUM_N("Two", TestEnum::kTwo)
	SEOUL_ENUM_N("Three", TestEnum::kThree)
SEOUL_END_ENUM()

static const Int kMemoryBudgetsType = MemoryBudgets::Strings;
typedef Vector<Float, kMemoryBudgetsType> TestVector;

typedef HashTable<HString, String, kMemoryBudgetsType> TestTable;

class BaseReflectionTest SEOUL_ABSTRACT
{
public:
	static Int s_iTestValue;

	BaseReflectionTest()
		: m_iBaseValue(200)
		, m_vfBaseValues(20, -2.0f)
	{
	}

	virtual ~BaseReflectionTest()
	{
	}

	Int GetBaseValue() const
	{
		return m_iBaseValue;
	}

	void SetBaseValue(Int iBaseValue)
	{
		m_iBaseValue = iBaseValue;
	}

	void BaseTestMethodA()
	{
		m_iBaseValue = -2;
	}

	void BaseTestMethodB(Int iValue)
	{
		m_iBaseValue = iValue;
	}

	static void BaseTestMethodC(Int iValue)
	{
		s_iTestValue = iValue;
	}

	const TestVector& GetBaseValues() const
	{
		return m_vfBaseValues;
	}

	void SetBaseValues(const TestVector& vfValues)
	{
		m_vfBaseValues = vfValues;
	}


private:
	SEOUL_REFLECTION_FRIENDSHIP(BaseReflectionTest);

	Int m_iBaseValue;
	TestVector m_vfBaseValues;
};

Int BaseReflectionTest::s_iTestValue = -1;

SEOUL_BEGIN_TYPE(BaseReflectionTest, TypeFlags::kDisableNew)
	SEOUL_ALIAS("iBaseValue", "BaseValue")
	SEOUL_ALIAS("BaseTestItMethodA", "BaseTestMethodA")

	SEOUL_METHOD(BaseTestMethodA)
	SEOUL_METHOD(BaseTestMethodB)
	SEOUL_METHOD(BaseTestMethodC)

	SEOUL_PROPERTY_N("BaseValue", m_iBaseValue)
	SEOUL_PROPERTY_N("BaseValues", m_vfBaseValues)
SEOUL_END_TYPE()

class ReflectionTestUtility SEOUL_SEALED : public BaseReflectionTest
{
public:
	ReflectionTestUtility()
		: m_fValue(100.0f)
		, m_eValue(TestEnum::kThree)
	{
	}

	Float GetValue() const
	{
		return m_fValue;
	}

	void SetValue(Float fValue)
	{
		m_fValue = fValue;
	}

	void TestMethodA()
	{
		m_fValue = -1.0f;
	}

	void TestMethodB(Float fValue)
	{
		m_fValue = fValue;
	}

	TestEnum::Enum GetEnumValue() const
	{
		return m_eValue;
	}

	void SetEnumValue(TestEnum::Enum eValue)
	{
		m_eValue = eValue;
	}

private:
	Float m_fValue;
	TestEnum::Enum m_eValue;

	SEOUL_REFLECTION_FRIENDSHIP(ReflectionTestUtility);
};

static Byte const* const kTestDescription("This is a test description.");

SEOUL_BEGIN_TYPE(ReflectionTestUtility)
	SEOUL_ATTRIBUTE(Description, "This is a test description.")
	SEOUL_PARENT(BaseReflectionTest)

	SEOUL_METHOD(TestMethodA)
	SEOUL_METHOD(TestMethodB)

	SEOUL_PROPERTY_N("Value", m_fValue)
	SEOUL_PROPERTY_N("EnumValue", m_eValue)
SEOUL_END_TYPE()

class ReflectionTestUtilityComplex SEOUL_SEALED
{
public:
	ReflectionTestUtilityComplex()
		: m_fValue(100.0f)
		, m_fValue2(43.3f)
		, m_eValue(TestEnum::kThree)
	{
	}

	Float GetValue() const
	{
		return m_fValue;
	}

	void SetValue(Float fValue)
	{
		m_fValue = fValue;
	}

	Float GetValue2() const
	{
		return m_fValue2;
	}

	void SetValue2(Float fValue)
	{
		m_fValue2 = fValue;
	}

	TestEnum::Enum GetEnumValue() const
	{
		return m_eValue;
	}

	void SetEnumValue(TestEnum::Enum eValue)
	{
		m_eValue = eValue;
	}

private:
	Float m_fValue;
	Float m_fValue2;
	TestEnum::Enum m_eValue;

	SEOUL_REFLECTION_FRIENDSHIP(ReflectionTestUtilityComplex);
}; // class ReflectionTestUtilityComplex

SEOUL_BEGIN_TYPE(ReflectionTestUtilityComplex)
	SEOUL_PROPERTY_PAIR_N("EnumValue", GetEnumValue, SetEnumValue)
	SEOUL_PROPERTY_PAIR_N("Value", GetValue, SetValue)
	SEOUL_PROPERTY_PAIR_N("Value2", GetValue2, SetValue2)
SEOUL_END_TYPE()

using namespace Reflection;

void ReflectionTest::TestReflectionDeserializeBasic()
{
	DataStore dataStore;
	dataStore.MakeArray();
	auto const root = dataStore.GetRootNode();
	DataNode value;

	{
		Atomic32 v(0);

		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(root, 0u, 72));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(Reflection::DeserializeObject(
			ContentKey(),
			dataStore,
			value,
			&v));
		SEOUL_UNITTESTING_ASSERT_EQUAL(Atomic32(72), v);
	}

	{
		Bool b = false;

		SEOUL_UNITTESTING_ASSERT(dataStore.SetBooleanValueToArray(root, 0u, true));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(Reflection::DeserializeObject(
			ContentKey(),
			dataStore,
			value,
			&b));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, b);
	}
	{
		Int8 i = 0;

		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(root, 0u, -72));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(Reflection::DeserializeObject(
			ContentKey(),
			dataStore,
			value,
			&i));
		SEOUL_UNITTESTING_ASSERT_EQUAL(-72, i);
	}
	{
		Int16 i = 0;

		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(root, 0u, -72));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(Reflection::DeserializeObject(
			ContentKey(),
			dataStore,
			value,
			&i));
		SEOUL_UNITTESTING_ASSERT_EQUAL(-72, i);
	}
	{
		Int32 i = 0;

		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(root, 0u, -72));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(Reflection::DeserializeObject(
			ContentKey(),
			dataStore,
			value,
			&i));
		SEOUL_UNITTESTING_ASSERT_EQUAL(-72, i);
	}
	{
		Int64 i = 0;

		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(root, 0u, -72));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(Reflection::DeserializeObject(
			ContentKey(),
			dataStore,
			value,
			&i));
		SEOUL_UNITTESTING_ASSERT_EQUAL(-72, i);
	}
	{
		UInt8 i = 0;

		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(root, 0u, 72));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(Reflection::DeserializeObject(
			ContentKey(),
			dataStore,
			value,
			&i));
		SEOUL_UNITTESTING_ASSERT_EQUAL(72, i);
	}
	{
		UInt16 i = 0;

		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(root, 0u, 72));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(Reflection::DeserializeObject(
			ContentKey(),
			dataStore,
			value,
			&i));
		SEOUL_UNITTESTING_ASSERT_EQUAL(72, i);
	}
	{
		UInt32 i = 0;

		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(root, 0u, 72));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(Reflection::DeserializeObject(
			ContentKey(),
			dataStore,
			value,
			&i));
		SEOUL_UNITTESTING_ASSERT_EQUAL(72, i);
	}
	{
		UInt64 i = 0;

		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(root, 0u, 72));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(Reflection::DeserializeObject(
			ContentKey(),
			dataStore,
			value,
			&i));
		SEOUL_UNITTESTING_ASSERT_EQUAL(72, i);
	}
	{
		Float32 f = 0;

		SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToArray(root, 0u, 53.0f));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(Reflection::DeserializeObject(
			ContentKey(),
			dataStore,
			value,
			&f));
		SEOUL_UNITTESTING_ASSERT_EQUAL(53.0f, f);
	}
	{
		Float64 f = 0;

		SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToArray(root, 0u, 53.0));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(Reflection::DeserializeObject(
			ContentKey(),
			dataStore,
			value,
			&f));
		SEOUL_UNITTESTING_ASSERT_EQUAL(53.0, f);
	}
	{
		HashSet<Int> s;

		SEOUL_UNITTESTING_ASSERT(dataStore.SetArrayToArray(root, 0u));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(value, 0u, 23));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(value, 1u, 87));
		SEOUL_UNITTESTING_ASSERT(Reflection::DeserializeObject(
			ContentKey(),
			dataStore,
			value,
			&s));
		SEOUL_UNITTESTING_ASSERT_EQUAL(2u, s.GetSize());
		SEOUL_UNITTESTING_ASSERT(s.HasKey(23));
		SEOUL_UNITTESTING_ASSERT(s.HasKey(87));
	}
	{
		List<Int> l;

		SEOUL_UNITTESTING_ASSERT(dataStore.SetArrayToArray(root, 0u));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(value, 0u, 23));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(value, 1u, 87));
		SEOUL_UNITTESTING_ASSERT(Reflection::DeserializeObject(
			ContentKey(),
			dataStore,
			value,
			&l));
		SEOUL_UNITTESTING_ASSERT_EQUAL(2u, l.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(23, l.Front());
		SEOUL_UNITTESTING_ASSERT_EQUAL(87, l.Back());
	}
	{
		Color4 c;

		SEOUL_UNITTESTING_ASSERT(dataStore.SetArrayToArray(root, 0u));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToArray(value, 0u, 0.25f));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToArray(value, 1u, 0.5f));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToArray(value, 2u, 0.75f));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToArray(value, 3u, 1.0f));
		SEOUL_UNITTESTING_ASSERT(Reflection::DeserializeObject(
			ContentKey(),
			dataStore,
			value,
			&c));
		SEOUL_UNITTESTING_ASSERT_EQUAL(Color4(0.25f, 0.5f, 0.75f, 1.0f), c);
	}
	{
		ColorARGBu8 c;

		SEOUL_UNITTESTING_ASSERT(dataStore.SetArrayToArray(root, 0u));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToArray(value, 0u, 0.25f));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToArray(value, 1u, 0.5f));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToArray(value, 2u, 0.75f));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToArray(value, 3u, 1.0f));
		SEOUL_UNITTESTING_ASSERT(Reflection::DeserializeObject(
			ContentKey(),
			dataStore,
			value,
			&c));
		SEOUL_UNITTESTING_ASSERT_EQUAL(ColorARGBu8::Create(64, 128, 191, 255), c);
	}
	{
		FilePath f;

		SEOUL_UNITTESTING_ASSERT(dataStore.SetFilePathToArray(root, 0u, FilePath::CreateConfigFilePath("test")));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(Reflection::DeserializeObject(
			ContentKey(),
			dataStore,
			value,
			&f));
		SEOUL_UNITTESTING_ASSERT_EQUAL(FilePath::CreateConfigFilePath("test"), f);
	}
	{
		HString h;

		SEOUL_UNITTESTING_ASSERT(dataStore.SetStringToArray(root, 0u, "test"));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(Reflection::DeserializeObject(
			ContentKey(),
			dataStore,
			value,
			&h));
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("test"), h);
	}
	{
		String s;

		SEOUL_UNITTESTING_ASSERT(dataStore.SetStringToArray(root, 0u, "test"));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(Reflection::DeserializeObject(
			ContentKey(),
			dataStore,
			value,
			&s));
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("test"), s);
	}
	{
		Pair<Int, Int> pair;

		SEOUL_UNITTESTING_ASSERT(dataStore.SetArrayToArray(root, 0u));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(value, 0u, 23));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(value, 1u, 87));
		SEOUL_UNITTESTING_ASSERT(Reflection::DeserializeObject(
			ContentKey(),
			dataStore,
			value,
			&pair));
		SEOUL_UNITTESTING_ASSERT_EQUAL(23, pair.First);
		SEOUL_UNITTESTING_ASSERT_EQUAL(87, pair.Second);
	}
	{
		Point2DInt point;

		SEOUL_UNITTESTING_ASSERT(dataStore.SetArrayToArray(root, 0u));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(value, 0u, 23));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(value, 1u, 87));
		SEOUL_UNITTESTING_ASSERT(Reflection::DeserializeObject(
			ContentKey(),
			dataStore,
			value,
			&point));
		SEOUL_UNITTESTING_ASSERT_EQUAL(23, point.X);
		SEOUL_UNITTESTING_ASSERT_EQUAL(87, point.Y);
	}
	{
		Quaternion const qExpected(Quaternion::CreateFromDirection(Vector3D::UnitX()));
		Quaternion q;

		SEOUL_UNITTESTING_ASSERT(dataStore.SetArrayToArray(root, 0u));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToArray(value, 0u, qExpected.X));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToArray(value, 1u, qExpected.Y));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToArray(value, 2u, qExpected.Z));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToArray(value, 3u, qExpected.W));
		SEOUL_UNITTESTING_ASSERT(Reflection::DeserializeObject(
			ContentKey(),
			dataStore,
			value,
			&q));
		SEOUL_UNITTESTING_ASSERT_EQUAL(qExpected, q);
	}
	{
		UUID const uuidExpected(UUID::FromString("851e5aac-2891-481d-b600-404f2b72b4c8"));
		UUID uuid;

		SEOUL_UNITTESTING_ASSERT(dataStore.SetStringToArray(root, 0u, "851e5aac-2891-481d-b600-404f2b72b4c8"));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(Reflection::DeserializeObject(
			ContentKey(),
			dataStore,
			value,
			&uuid));
		SEOUL_UNITTESTING_ASSERT_EQUAL(uuidExpected, uuid);
	}
	{
		Vector2D v;

		SEOUL_UNITTESTING_ASSERT(dataStore.SetArrayToArray(root, 0u));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToArray(value, 0u, 23.0f));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToArray(value, 1u, 87.0f));
		SEOUL_UNITTESTING_ASSERT(Reflection::DeserializeObject(
			ContentKey(),
			dataStore,
			value,
			&v));
		SEOUL_UNITTESTING_ASSERT_EQUAL(Vector2D(23, 87), v);
	}
	{
		Vector3D v;

		SEOUL_UNITTESTING_ASSERT(dataStore.SetArrayToArray(root, 0u));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToArray(value, 0u, 23.0f));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToArray(value, 1u, 87.0f));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToArray(value, 2u, 95.0f));
		SEOUL_UNITTESTING_ASSERT(Reflection::DeserializeObject(
			ContentKey(),
			dataStore,
			value,
			&v));
		SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D(23, 87, 95), v);
	}
	{
		Vector4D v;

		SEOUL_UNITTESTING_ASSERT(dataStore.SetArrayToArray(root, 0u));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToArray(value, 0u, 23.0f));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToArray(value, 1u, 87.0f));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToArray(value, 2u, 95.0f));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToArray(value, 3u, 200.0f));
		SEOUL_UNITTESTING_ASSERT(Reflection::DeserializeObject(
			ContentKey(),
			dataStore,
			value,
			&v));
		SEOUL_UNITTESTING_ASSERT_EQUAL(Vector4D(23, 87, 95, 200), v);
	}
	{
		WorldTime v;

		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt64ValueToArray(root, 0u, WorldTime::FromSecondsInt64(25).GetMicroseconds()));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(Reflection::DeserializeObject(
			ContentKey(),
			dataStore,
			value,
			&v));
		SEOUL_UNITTESTING_ASSERT_EQUAL(WorldTime::FromSecondsInt64(25), v);
	}
	{
		Matrix4D m;

		SEOUL_UNITTESTING_ASSERT(dataStore.SetArrayToArray(root, 0u));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(value, 0u, 1));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(value, 1u, 2));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(value, 2u, 3));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(value, 3u, 4));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(value, 4u, 5));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(value, 5u, 6));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(value, 6u, 7));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(value, 7u, 8));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(value, 8u, 9));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(value, 9u, 10));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(value, 10u, 11));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(value, 11u, 12));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(value, 12u, 13));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(value, 13u, 14));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(value, 14u, 15));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(value, 15u, 16));
		SEOUL_UNITTESTING_ASSERT(Reflection::DeserializeObject(
			ContentKey(),
			dataStore,
			value,
			&m));
		SEOUL_UNITTESTING_ASSERT_EQUAL(Matrix4D(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), m);
	}
	{
		RGBA rgba = RGBA::TransparentBlack();

		SEOUL_UNITTESTING_ASSERT(dataStore.SetStringToArray(root, 0u, "01020304"));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(Reflection::DeserializeObject(
			ContentKey(),
			dataStore,
			value,
			&rgba));
		SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::Create(1, 2, 3, 4), rgba);

		SEOUL_UNITTESTING_ASSERT(dataStore.SetStringToArray(root, 0u, "FFFFFFFF"));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(Reflection::DeserializeObject(
			ContentKey(),
			dataStore,
			value,
			&rgba));
		SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::Create(255, 255, 255, 255), rgba);
	}
}

void ReflectionTest::TestReflectionDeserializeComplexProperties()
{
	ReflectionTestUtilityComplex complex;
	complex.SetEnumValue(TestEnum::kZero);
	complex.SetValue(50.0f);
	complex.SetValue2(4.1f);

	DataStore dataStore;
	dataStore.MakeTable();
	SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToTable(
		ContentKey(),
		dataStore,
		dataStore.GetRootNode(),
		HString("A"),
		&complex));

	dataStore.ReplaceRootWithTableElement(dataStore.GetRootNode(), HString("A"));
	
	ReflectionTestUtilityComplex complex2;
	SEOUL_UNITTESTING_ASSERT(Reflection::DeserializeObject(
		ContentKey(),
		dataStore,
		dataStore.GetRootNode(),
		&complex2));

	SEOUL_UNITTESTING_ASSERT_EQUAL(complex.GetEnumValue(), complex2.GetEnumValue());
	SEOUL_UNITTESTING_ASSERT_EQUAL(complex.GetValue(), complex2.GetValue());
	SEOUL_UNITTESTING_ASSERT_EQUAL(complex.GetValue2(), complex2.GetValue2());
}

void ReflectionTest::TestReflectionSerializeToArrayBasic()
{
	DataStore dataStore;
	dataStore.MakeArray();
	auto const root = dataStore.GetRootNode();
	DataNode value;

	{
		Atomic32 const v(72);

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToArray(
			ContentKey(),
			dataStore,
			root,
			0u,
			&v));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
		SEOUL_UNITTESTING_ASSERT_EQUAL(72, dataStore.AssumeInt32Small(value));
	}

	{
		Bool b = true;

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToArray(
			ContentKey(),
			dataStore,
			root,
			0u,
			&b));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsBoolean());
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, dataStore.AssumeBoolean(value));
	}

	{
		Int8 i = -72;

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToArray(
			ContentKey(),
			dataStore,
			root,
			0u,
			&i));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
		SEOUL_UNITTESTING_ASSERT_EQUAL(-72, dataStore.AssumeInt32Small(value));
	}
	{
		Int16 i = -72;

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToArray(
			ContentKey(),
			dataStore,
			root,
			0u,
			&i));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
		SEOUL_UNITTESTING_ASSERT_EQUAL(-72, dataStore.AssumeInt32Small(value));
	}
	{
		Int32 i = -72;

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToArray(
			ContentKey(),
			dataStore,
			root,
			0u,
			&i));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
		SEOUL_UNITTESTING_ASSERT_EQUAL(-72, dataStore.AssumeInt32Small(value));
	}
	{
		Int64 i = -72;

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToArray(
			ContentKey(),
			dataStore,
			root,
			0u,
			&i));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
		SEOUL_UNITTESTING_ASSERT_EQUAL(-72, dataStore.AssumeInt32Small(value));
	}
	{
		UInt8 i = 72;

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToArray(
			ContentKey(),
			dataStore,
			root,
			0u,
			&i));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
		SEOUL_UNITTESTING_ASSERT_EQUAL(72, dataStore.AssumeInt32Small(value));
	}
	{
		UInt16 i = 72;

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToArray(
			ContentKey(),
			dataStore,
			root,
			0u,
			&i));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
		SEOUL_UNITTESTING_ASSERT_EQUAL(72, dataStore.AssumeInt32Small(value));
	}
	{
		UInt32 i = 72;

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToArray(
			ContentKey(),
			dataStore,
			root,
			0u,
			&i));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
		SEOUL_UNITTESTING_ASSERT_EQUAL(72, dataStore.AssumeInt32Small(value));
	}
	{
		UInt64 i = 72;

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToArray(
			ContentKey(),
			dataStore,
			root,
			0u,
			&i));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
		SEOUL_UNITTESTING_ASSERT_EQUAL(72, dataStore.AssumeInt32Small(value));
	}
	{
		Float32 f = 53.5f;

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToArray(
			ContentKey(),
			dataStore,
			root,
			0u,
			&f));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsFloat31());
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(53.5f, dataStore.AssumeFloat31(value), 1e-6f);
	}
	{
		Float64 f = 53.5;

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToArray(
			ContentKey(),
			dataStore,
			root,
			0u,
			&f));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsFloat31());
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(53.5f, dataStore.AssumeFloat31(value), 1e-6f);
	}
	{
		HashSet<Int> s;
		SEOUL_UNITTESTING_ASSERT(s.Insert(23).Second);
		SEOUL_UNITTESTING_ASSERT(s.Insert(87).Second);

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToArray(
			ContentKey(),
			dataStore,
			root,
			0u,
			&s));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsArray());
		UInt32 uCount = 0u;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(value, uCount));
		SEOUL_UNITTESTING_ASSERT_EQUAL(2u, uCount);

		DataNode subvalue;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 0u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(87, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 1u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(23, dataStore.AssumeInt32Small(subvalue));
	}
	{
		List<Int> l;
		l.PushBack(23);
		l.PushBack(87);

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToArray(
			ContentKey(),
			dataStore,
			root,
			0u,
			&l));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsArray());
		UInt32 uCount = 0u;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(value, uCount));
		SEOUL_UNITTESTING_ASSERT_EQUAL(2u, uCount);

		DataNode subvalue;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 0u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(23, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 1u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(87, dataStore.AssumeInt32Small(subvalue));
	}
	{
		Color4 const c(0.25f, 0.5f, 0.75f, 1.0f);

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToArray(
			ContentKey(),
			dataStore,
			root,
			0u,
			&c));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsArray());
		UInt32 uCount = 0u;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(value, uCount));
		SEOUL_UNITTESTING_ASSERT_EQUAL(4u, uCount);

		DataNode subvalue;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 0u, subvalue));
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(0.25f, dataStore.AssumeFloat31(subvalue), 1e-6f);
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 1u, subvalue));
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(0.5f, dataStore.AssumeFloat31(subvalue), 1e-6f);
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 2u, subvalue));
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(0.75f, dataStore.AssumeFloat31(subvalue), 1e-6f);
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 3u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, dataStore.AssumeInt32Small(subvalue));
	}
	{
		ColorARGBu8 const c(ColorARGBu8::Create(64, 128, 191, 255));

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToArray(
			ContentKey(),
			dataStore,
			root,
			0u,
			&c));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsArray());
		UInt32 uCount = 0u;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(value, uCount));
		SEOUL_UNITTESTING_ASSERT_EQUAL(4u, uCount);

		DataNode subvalue;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 0u, subvalue));
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(0.25f, dataStore.AssumeFloat32(subvalue), 1e-3f);
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 1u, subvalue));
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(0.5f, dataStore.AssumeFloat32(subvalue), 1e-2f);
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 2u, subvalue));
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(0.75f, dataStore.AssumeFloat31(subvalue), 1e-3f);
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 3u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, dataStore.AssumeInt32Small(subvalue));
	}
	{
		FilePath const f(FilePath::CreateConfigFilePath("test"));

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToArray(
			ContentKey(),
			dataStore,
			root,
			0u,
			&f));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsFilePath());

		FilePath filePath;
		SEOUL_UNITTESTING_ASSERT(dataStore.AsFilePath(value, filePath));
		SEOUL_UNITTESTING_ASSERT_EQUAL(FilePath::CreateConfigFilePath("test"), filePath);
	}
	{
		HString const h("test");

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToArray(
			ContentKey(),
			dataStore,
			root,
			0u,
			&h));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsString());

		String s;
		SEOUL_UNITTESTING_ASSERT(dataStore.AsString(value, s));
		SEOUL_UNITTESTING_ASSERT_EQUAL("test", s);
	}
	{
		String const sExpected("test");

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToArray(
			ContentKey(),
			dataStore,
			root,
			0u,
			&sExpected));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsString());

		String s;
		SEOUL_UNITTESTING_ASSERT(dataStore.AsString(value, s));
		SEOUL_UNITTESTING_ASSERT_EQUAL("test", s);
	}
	{
		Pair<Int, Int> pair;
		pair.First = 23;
		pair.Second = 87;

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToArray(
			ContentKey(),
			dataStore,
			root,
			0u,
			&pair));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsArray());
		UInt32 uCount = 0u;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(value, uCount));
		SEOUL_UNITTESTING_ASSERT_EQUAL(2u, uCount);

		DataNode subvalue;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 0u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(23, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 1u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(87, dataStore.AssumeInt32Small(subvalue));
	}
	{
		Point2DInt point;
		point.X = 23;
		point.Y = 87;

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToArray(
			ContentKey(),
			dataStore,
			root,
			0u,
			&point));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsArray());
		UInt32 uCount = 0u;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(value, uCount));
		SEOUL_UNITTESTING_ASSERT_EQUAL(2u, uCount);

		DataNode subvalue;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 0u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(23, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 1u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(87, dataStore.AssumeInt32Small(subvalue));
	}
	{
		Quaternion const qExpected(Quaternion::CreateFromDirection(Vector3D::UnitX()));

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToArray(
			ContentKey(),
			dataStore,
			root,
			0u,
			&qExpected));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsArray());
		UInt32 uCount = 0u;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(value, uCount));
		SEOUL_UNITTESTING_ASSERT_EQUAL(4u, uCount);

		DataNode subvalue;
		Float fValue = -5.0f;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 0u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 1u, subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.AsFloat32(subvalue, fValue));
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(-0.70710677f, fValue, 1e-6f);
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 2u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 3u, subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.AsFloat32(subvalue, fValue));
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(0.70710677f, fValue, 1e-6f);
	}
	{
		UUID const uuidExpected(UUID::FromString("851e5aac-2891-481d-b600-404f2b72b4c8"));

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToArray(
			ContentKey(),
			dataStore,
			root,
			0u,
			&uuidExpected));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsString());

		String s;
		SEOUL_UNITTESTING_ASSERT(dataStore.AsString(value, s));
		SEOUL_UNITTESTING_ASSERT_EQUAL("851e5aac-2891-481d-b600-404f2b72b4c8", s);
	}
	{
		Vector2D v;
		v.X = 23.0f;
		v.Y = 87.0f;

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToArray(
			ContentKey(),
			dataStore,
			root,
			0u,
			&v));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsArray());
		UInt32 uCount = 0u;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(value, uCount));
		SEOUL_UNITTESTING_ASSERT_EQUAL(2u, uCount);

		DataNode subvalue;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 0u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(23, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 1u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(87, dataStore.AssumeInt32Small(subvalue));
	}
	{
		Vector3D v;
		v.X = 23.0f;
		v.Y = 87.0f;
		v.Z = 95.0f;

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToArray(
			ContentKey(),
			dataStore,
			root,
			0u,
			&v));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsArray());
		UInt32 uCount = 0u;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(value, uCount));
		SEOUL_UNITTESTING_ASSERT_EQUAL(3u, uCount);

		DataNode subvalue;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 0u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(23, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 1u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(87, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 2u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(95, dataStore.AssumeInt32Small(subvalue));
	}
	{
		Vector4D v;
		v.X = 23.0f;
		v.Y = 87.0f;
		v.Z = 95.0f;
		v.W = 200.0f;

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToArray(
			ContentKey(),
			dataStore,
			root,
			0u,
			&v));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsArray());
		UInt32 uCount = 0u;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(value, uCount));
		SEOUL_UNITTESTING_ASSERT_EQUAL(4u, uCount);

		DataNode subvalue;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 0u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(23, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 1u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(87, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 2u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(95, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 3u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(200, dataStore.AssumeInt32Small(subvalue));
	}
	{
		WorldTime const v(WorldTime::FromSecondsInt64(25));

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToArray(
			ContentKey(),
			dataStore,
			root,
			0u,
			&v));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));

		Int64 iValue;
		SEOUL_UNITTESTING_ASSERT(dataStore.AsInt64(value, iValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(WorldTime::FromSecondsInt64(25).GetMicroseconds(), iValue);
	}
	{
		Matrix4D const m(
			1, 2, 3, 4,
			5, 6, 7, 8,
			9, 10, 11, 12,
			13, 14, 15, 16);

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToArray(
			ContentKey(),
			dataStore,
			root,
			0u,
			&m));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsArray());
		UInt32 uCount = 0u;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(value, uCount));
		SEOUL_UNITTESTING_ASSERT_EQUAL(16u, uCount);

		DataNode subvalue;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 0u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 1u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 2u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 3u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(4, dataStore.AssumeInt32Small(subvalue));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 4u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 5u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 6u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(7, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 7u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(8, dataStore.AssumeInt32Small(subvalue));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 8u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(9, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 9u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(10, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 10u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(11, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 11u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(12, dataStore.AssumeInt32Small(subvalue));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 12u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(13, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 13u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(14, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 14u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(15, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 15u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(16, dataStore.AssumeInt32Small(subvalue));
	}
	{
		RGBA const rgba = RGBA::Create(1, 2, 3, 4);

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToArray(
			ContentKey(),
			dataStore,
			root,
			0u,
			&rgba));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsString());

		String s;
		SEOUL_UNITTESTING_ASSERT(dataStore.AsString(value, s));
		SEOUL_UNITTESTING_ASSERT_EQUAL("01020304", s);
	}
}

void ReflectionTest::TestReflectionSerializeToTableBasic()
{
	static const HString kKey("testKey");

	DataStore dataStore;
	dataStore.MakeTable();
	auto const root = dataStore.GetRootNode();
	DataNode value;

	{
		Atomic32 const v(72);

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToTable(
			ContentKey(),
			dataStore,
			root,
			kKey,
			&v));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
		SEOUL_UNITTESTING_ASSERT_EQUAL(72, dataStore.AssumeInt32Small(value));
	}

	{
		Bool b = true;

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToTable(
			ContentKey(),
			dataStore,
			root,
			kKey,
			&b));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsBoolean());
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, dataStore.AssumeBoolean(value));
	}

	{
		Int8 i = -72;

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToTable(
			ContentKey(),
			dataStore,
			root,
			kKey,
			&i));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
		SEOUL_UNITTESTING_ASSERT_EQUAL(-72, dataStore.AssumeInt32Small(value));
	}
	{
		Int16 i = -72;

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToTable(
			ContentKey(),
			dataStore,
			root,
			kKey,
			&i));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
		SEOUL_UNITTESTING_ASSERT_EQUAL(-72, dataStore.AssumeInt32Small(value));
	}
	{
		Int32 i = -72;

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToTable(
			ContentKey(),
			dataStore,
			root,
			kKey,
			&i));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
		SEOUL_UNITTESTING_ASSERT_EQUAL(-72, dataStore.AssumeInt32Small(value));
	}
	{
		Int64 i = -72;

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToTable(
			ContentKey(),
			dataStore,
			root,
			kKey,
			&i));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
		SEOUL_UNITTESTING_ASSERT_EQUAL(-72, dataStore.AssumeInt32Small(value));
	}
	{
		UInt8 i = 72;

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToTable(
			ContentKey(),
			dataStore,
			root,
			kKey,
			&i));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
		SEOUL_UNITTESTING_ASSERT_EQUAL(72, dataStore.AssumeInt32Small(value));
	}
	{
		UInt16 i = 72;

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToTable(
			ContentKey(),
			dataStore,
			root,
			kKey,
			&i));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
		SEOUL_UNITTESTING_ASSERT_EQUAL(72, dataStore.AssumeInt32Small(value));
	}
	{
		UInt32 i = 72;

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToTable(
			ContentKey(),
			dataStore,
			root,
			kKey,
			&i));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
		SEOUL_UNITTESTING_ASSERT_EQUAL(72, dataStore.AssumeInt32Small(value));
	}
	{
		UInt64 i = 72;

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToTable(
			ContentKey(),
			dataStore,
			root,
			kKey,
			&i));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsInt32Small());
		SEOUL_UNITTESTING_ASSERT_EQUAL(72, dataStore.AssumeInt32Small(value));
	}
	{
		Float32 f = 53.5f;

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToTable(
			ContentKey(),
			dataStore,
			root,
			kKey,
			&f));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsFloat31());
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(53.5f, dataStore.AssumeFloat31(value), 1e-6f);
	}
	{
		Float64 f = 53.5;

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToTable(
			ContentKey(),
			dataStore,
			root,
			kKey,
			&f));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsFloat31());
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(53.5f, dataStore.AssumeFloat31(value), 1e-6f);
	}
	{
		HashSet<Int> s;
		SEOUL_UNITTESTING_ASSERT(s.Insert(23).Second);
		SEOUL_UNITTESTING_ASSERT(s.Insert(87).Second);

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToTable(
			ContentKey(),
			dataStore,
			root,
			kKey,
			&s));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsArray());
		UInt32 uCount = 0u;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(value, uCount));
		SEOUL_UNITTESTING_ASSERT_EQUAL(2u, uCount);

		DataNode subvalue;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 0u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(87, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 1u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(23, dataStore.AssumeInt32Small(subvalue));
	}
	{
		List<Int> l;
		l.PushBack(23);
		l.PushBack(87);

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToTable(
			ContentKey(),
			dataStore,
			root,
			kKey,
			&l));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsArray());
		UInt32 uCount = 0u;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(value, uCount));
		SEOUL_UNITTESTING_ASSERT_EQUAL(2u, uCount);

		DataNode subvalue;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 0u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(23, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 1u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(87, dataStore.AssumeInt32Small(subvalue));
	}
	{
		Color4 const c(0.25f, 0.5f, 0.75f, 1.0f);

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToTable(
			ContentKey(),
			dataStore,
			root,
			kKey,
			&c));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsArray());
		UInt32 uCount = 0u;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(value, uCount));
		SEOUL_UNITTESTING_ASSERT_EQUAL(4u, uCount);

		DataNode subvalue;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 0u, subvalue));
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(0.25f, dataStore.AssumeFloat31(subvalue), 1e-6f);
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 1u, subvalue));
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(0.5f, dataStore.AssumeFloat31(subvalue), 1e-6f);
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 2u, subvalue));
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(0.75f, dataStore.AssumeFloat31(subvalue), 1e-6f);
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 3u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, dataStore.AssumeInt32Small(subvalue));
	}
	{
		ColorARGBu8 const c(ColorARGBu8::Create(64, 128, 191, 255));

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToTable(
			ContentKey(),
			dataStore,
			root,
			kKey,
			&c));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsArray());
		UInt32 uCount = 0u;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(value, uCount));
		SEOUL_UNITTESTING_ASSERT_EQUAL(4u, uCount);

		DataNode subvalue;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 0u, subvalue));
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(0.25f, dataStore.AssumeFloat32(subvalue), 1e-3f);
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 1u, subvalue));
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(0.5f, dataStore.AssumeFloat32(subvalue), 1e-2f);
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 2u, subvalue));
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(0.75f, dataStore.AssumeFloat31(subvalue), 1e-3f);
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 3u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, dataStore.AssumeInt32Small(subvalue));
	}
	{
		FilePath const f(FilePath::CreateConfigFilePath("test"));

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToTable(
			ContentKey(),
			dataStore,
			root,
			kKey,
			&f));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsFilePath());

		FilePath filePath;
		SEOUL_UNITTESTING_ASSERT(dataStore.AsFilePath(value, filePath));
		SEOUL_UNITTESTING_ASSERT_EQUAL(FilePath::CreateConfigFilePath("test"), filePath);
	}
	{
		HString const h("test");

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToTable(
			ContentKey(),
			dataStore,
			root,
			kKey,
			&h));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsString());

		String s;
		SEOUL_UNITTESTING_ASSERT(dataStore.AsString(value, s));
		SEOUL_UNITTESTING_ASSERT_EQUAL("test", s);
	}
	{
		String const sExpected("test");

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToTable(
			ContentKey(),
			dataStore,
			root,
			kKey,
			&sExpected));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsString());

		String s;
		SEOUL_UNITTESTING_ASSERT(dataStore.AsString(value, s));
		SEOUL_UNITTESTING_ASSERT_EQUAL("test", s);
	}
	{
		Pair<Int, Int> pair;
		pair.First = 23;
		pair.Second = 87;

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToTable(
			ContentKey(),
			dataStore,
			root,
			kKey,
			&pair));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsArray());
		UInt32 uCount = 0u;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(value, uCount));
		SEOUL_UNITTESTING_ASSERT_EQUAL(2u, uCount);

		DataNode subvalue;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 0u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(23, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 1u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(87, dataStore.AssumeInt32Small(subvalue));
	}
	{
		Point2DInt point;
		point.X = 23;
		point.Y = 87;

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToTable(
			ContentKey(),
			dataStore,
			root,
			kKey,
			&point));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsArray());
		UInt32 uCount = 0u;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(value, uCount));
		SEOUL_UNITTESTING_ASSERT_EQUAL(2u, uCount);

		DataNode subvalue;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 0u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(23, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 1u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(87, dataStore.AssumeInt32Small(subvalue));
	}
	{
		Quaternion const qExpected(Quaternion::CreateFromDirection(Vector3D::UnitX()));

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToTable(
			ContentKey(),
			dataStore,
			root,
			kKey,
			&qExpected));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsArray());
		UInt32 uCount = 0u;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(value, uCount));
		SEOUL_UNITTESTING_ASSERT_EQUAL(4u, uCount);

		DataNode subvalue;
		Float fValue = -2.0f;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 0u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 1u, subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.AsFloat32(subvalue, fValue));
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(-0.70710677f, fValue, 1e-6f);
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 2u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 3u, subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.AsFloat32(subvalue, fValue));
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(0.70710677f, fValue, 1e-6f);
	}
	{
		UUID const uuidExpected(UUID::FromString("851e5aac-2891-481d-b600-404f2b72b4c8"));

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToTable(
			ContentKey(),
			dataStore,
			root,
			kKey,
			&uuidExpected));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsString());

		String s;
		SEOUL_UNITTESTING_ASSERT(dataStore.AsString(value, s));
		SEOUL_UNITTESTING_ASSERT_EQUAL("851e5aac-2891-481d-b600-404f2b72b4c8", s);
	}
	{
		Vector2D v;
		v.X = 23.0f;
		v.Y = 87.0f;

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToTable(
			ContentKey(),
			dataStore,
			root,
			kKey,
			&v));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsArray());
		UInt32 uCount = 0u;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(value, uCount));
		SEOUL_UNITTESTING_ASSERT_EQUAL(2u, uCount);

		DataNode subvalue;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 0u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(23, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 1u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(87, dataStore.AssumeInt32Small(subvalue));
	}
	{
		Vector3D v;
		v.X = 23.0f;
		v.Y = 87.0f;
		v.Z = 95.0f;

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToTable(
			ContentKey(),
			dataStore,
			root,
			kKey,
			&v));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsArray());
		UInt32 uCount = 0u;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(value, uCount));
		SEOUL_UNITTESTING_ASSERT_EQUAL(3u, uCount);

		DataNode subvalue;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 0u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(23, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 1u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(87, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 2u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(95, dataStore.AssumeInt32Small(subvalue));
	}
	{
		Vector4D v;
		v.X = 23.0f;
		v.Y = 87.0f;
		v.Z = 95.0f;
		v.W = 200.0f;

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToTable(
			ContentKey(),
			dataStore,
			root,
			kKey,
			&v));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsArray());
		UInt32 uCount = 0u;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(value, uCount));
		SEOUL_UNITTESTING_ASSERT_EQUAL(4u, uCount);

		DataNode subvalue;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 0u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(23, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 1u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(87, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 2u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(95, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 3u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(200, dataStore.AssumeInt32Small(subvalue));
	}
	{
		WorldTime const v(WorldTime::FromSecondsInt64(25));

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToTable(
			ContentKey(),
			dataStore,
			root,
			kKey,
			&v));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));

		Int64 iValue;
		SEOUL_UNITTESTING_ASSERT(dataStore.AsInt64(value, iValue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(WorldTime::FromSecondsInt64(25).GetMicroseconds(), iValue);
	}
	{
		Matrix4D const m(
			1, 2, 3, 4,
			5, 6, 7, 8,
			9, 10, 11, 12,
			13, 14, 15, 16);

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToTable(
			ContentKey(),
			dataStore,
			root,
			kKey,
			&m));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsArray());
		UInt32 uCount = 0u;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(value, uCount));
		SEOUL_UNITTESTING_ASSERT_EQUAL(16u, uCount);

		DataNode subvalue;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 0u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 1u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 2u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 3u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(4, dataStore.AssumeInt32Small(subvalue));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 4u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 5u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 6u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(7, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 7u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(8, dataStore.AssumeInt32Small(subvalue));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 8u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(9, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 9u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(10, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 10u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(11, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 11u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(12, dataStore.AssumeInt32Small(subvalue));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 12u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(13, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 13u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(14, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 14u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(15, dataStore.AssumeInt32Small(subvalue));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 15u, subvalue));
		SEOUL_UNITTESTING_ASSERT_EQUAL(16, dataStore.AssumeInt32Small(subvalue));
	}
	{
		RGBA const rgba = RGBA::Create(1, 2, 3, 4);

		SEOUL_UNITTESTING_ASSERT(Reflection::SerializeObjectToTable(
			ContentKey(),
			dataStore,
			root,
			kKey,
			&rgba));

		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, kKey, value));
		SEOUL_UNITTESTING_ASSERT(value.IsString());

		String s;
		SEOUL_UNITTESTING_ASSERT(dataStore.AsString(value, s));
		SEOUL_UNITTESTING_ASSERT_EQUAL("01020304", s);
	}
}

void ReflectionTest::TestReflectionTypeBasic()
{
	const Registry& registry = Reflection::Registry::GetRegistry();

	Type const* pType = registry.GetType(HString("ReflectionTestUtility"));
	SEOUL_UNITTESTING_ASSERT(nullptr != pType);
	SEOUL_UNITTESTING_ASSERT(HString("ReflectionTestUtility") == pType->GetName());
	SEOUL_UNITTESTING_ASSERT(TypeId<ReflectionTestUtility>() == pType->GetTypeInfo());
	SEOUL_UNITTESTING_ASSERT(pType->GetAttributes().GetCount() == 1);
	SEOUL_UNITTESTING_ASSERT(pType->GetMethodCount() == 2);
	SEOUL_UNITTESTING_ASSERT(pType->GetParentCount() == 1);
	SEOUL_UNITTESTING_ASSERT(pType->GetPropertyCount() == 2);

	WeakAny testInstanceWeakAny = pType->New(MemoryBudgets::TBD);
	SEOUL_UNITTESTING_ASSERT(testInstanceWeakAny.IsValid());
	SEOUL_UNITTESTING_ASSERT(testInstanceWeakAny.IsOfType<ReflectionTestUtility*>());

	ScopedPtr<ReflectionTestUtility> pTestInstance(testInstanceWeakAny.Cast<ReflectionTestUtility*>());
	SEOUL_UNITTESTING_ASSERT(pTestInstance->GetBaseValue() == 200);
	SEOUL_UNITTESTING_ASSERT(pTestInstance->GetValue() == 100.0f);
}

void ReflectionTest::TestReflectionTypeAdvanced()
{
	const Registry& registry = Reflection::Registry::GetRegistry();

	Type const* pType = registry.GetType(HString("ReflectionTestUtility"));
	ScopedPtr<ReflectionTestUtility> pTestInstance(pType->New<ReflectionTestUtility>(MemoryBudgets::TBD));

	WeakAny testInstance(pTestInstance.Get());
	BaseReflectionTest* pBaseTestInstance = nullptr;
	SEOUL_UNITTESTING_ASSERT(pType->CastTo(testInstance, pBaseTestInstance));

	pType = registry.GetType(HString("BaseReflectionTest"));
	testInstance = pBaseTestInstance;
}

void ReflectionTest::TestReflectionTypeAttributes()
{
	const Registry& registry = Reflection::Registry::GetRegistry();

	Type const* pType = registry.GetType(HString("ReflectionTestUtility"));

	const AttributeCollection& attributes = pType->GetAttributes();
	SEOUL_UNITTESTING_ASSERT(attributes.GetCount() == 1u);
	SEOUL_UNITTESTING_ASSERT(attributes.HasAttribute(Attributes::Description::StaticId()));
	SEOUL_UNITTESTING_ASSERT(attributes.HasAttribute<Attributes::Description>());

	Attributes::Description const* pDescription = attributes.GetAttribute<Attributes::Description>();
	SEOUL_UNITTESTING_ASSERT(nullptr != pDescription);

	SEOUL_UNITTESTING_ASSERT(pDescription->GetId() == Attributes::Description::StaticId());
	SEOUL_UNITTESTING_ASSERT(0 == strcmp(pDescription->m_DescriptionText.CStr(), kTestDescription));
}

void ReflectionTest::TestReflectionTypeConstructors()
{
	Float32 f32Value = 0.0f;
	Float64 f64Value = 1.0;

	Int8 i8Value = 2;
	Int16 i16Value = 3;
	Int32 i32Value = 4;
	Int64 i64Value = 5;

	UInt8 u8Value = 6;
	UInt16 u16Value = 7;
	UInt32 u32Value = 8;

	// Construct f32 from smaller int types.
	SEOUL_UNITTESTING_ASSERT(TypeConstruct(i8Value, f32Value));
	SEOUL_UNITTESTING_ASSERT(i8Value == 2);
	SEOUL_UNITTESTING_ASSERT(f32Value == 2.0f);

	SEOUL_UNITTESTING_ASSERT(TypeConstruct(i16Value, f32Value));
	SEOUL_UNITTESTING_ASSERT(i16Value == 3);
	SEOUL_UNITTESTING_ASSERT(f32Value == 3.0f);

	SEOUL_UNITTESTING_ASSERT(TypeConstruct(i32Value, f32Value));
	SEOUL_UNITTESTING_ASSERT(i32Value == 4);
	SEOUL_UNITTESTING_ASSERT(f32Value == 4.0f);

	SEOUL_UNITTESTING_ASSERT(TypeConstruct(u8Value, f32Value));
	SEOUL_UNITTESTING_ASSERT(u8Value == 6);
	SEOUL_UNITTESTING_ASSERT(f32Value == 6.0f);

	SEOUL_UNITTESTING_ASSERT(TypeConstruct(u16Value, f32Value));
	SEOUL_UNITTESTING_ASSERT(u16Value == 7);
	SEOUL_UNITTESTING_ASSERT(f32Value == 7.0f);

	f32Value = 0.0f;

	// Construct f64 from smaller int types.
	SEOUL_UNITTESTING_ASSERT(TypeConstruct(i8Value, f64Value));
	SEOUL_UNITTESTING_ASSERT(i8Value == 2);
	SEOUL_UNITTESTING_ASSERT(f64Value == 2.0f);

	SEOUL_UNITTESTING_ASSERT(TypeConstruct(i16Value, f64Value));
	SEOUL_UNITTESTING_ASSERT(i16Value == 3);
	SEOUL_UNITTESTING_ASSERT(f64Value == 3.0f);

	SEOUL_UNITTESTING_ASSERT(TypeConstruct(i32Value, f64Value));
	SEOUL_UNITTESTING_ASSERT(i32Value == 4);
	SEOUL_UNITTESTING_ASSERT(f64Value == 4.0f);

	SEOUL_UNITTESTING_ASSERT(TypeConstruct(i64Value, f64Value));
	SEOUL_UNITTESTING_ASSERT(i64Value == 5);
	SEOUL_UNITTESTING_ASSERT(f64Value == 5.0f);

	SEOUL_UNITTESTING_ASSERT(TypeConstruct(u8Value, f64Value));
	SEOUL_UNITTESTING_ASSERT(u8Value == 6);
	SEOUL_UNITTESTING_ASSERT(f64Value == 6.0f);

	SEOUL_UNITTESTING_ASSERT(TypeConstruct(u16Value, f64Value));
	SEOUL_UNITTESTING_ASSERT(u16Value == 7);
	SEOUL_UNITTESTING_ASSERT(f64Value == 7.0f);

	SEOUL_UNITTESTING_ASSERT(TypeConstruct(u32Value, f64Value));
	SEOUL_UNITTESTING_ASSERT(u32Value == 8);
	SEOUL_UNITTESTING_ASSERT(f64Value == 8.0f);
}

void ReflectionTest::TestReflectionArray()
{
	const Reflection::Array& a = ArrayOf< TestVector >();
	SEOUL_UNITTESTING_ASSERT(a.CanResize());
	SEOUL_UNITTESTING_ASSERT(a.GetElementTypeInfo() == TypeId<Float>());
	SEOUL_UNITTESTING_ASSERT(a.GetElementTypeInfo() == TypeId<TestVector::ValueType>());

	TestVector v;
	WeakAny vector(&v);

	UInt32 zSize = UIntMax;
	SEOUL_UNITTESTING_ASSERT(a.TryGetSize(vector, zSize));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, zSize);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, v.GetSize());

	SEOUL_UNITTESTING_ASSERT(a.TryResize(vector, 10u));
	SEOUL_UNITTESTING_ASSERT_EQUAL(10u, v.GetSize());
	SEOUL_UNITTESTING_ASSERT(a.TryGetSize(vector, zSize));
	SEOUL_UNITTESTING_ASSERT_EQUAL(10u, zSize);

	WeakAny valuePtr;
	SEOUL_UNITTESTING_ASSERT(a.TryGetElementPtr(vector, 2u, valuePtr));
	SEOUL_UNITTESTING_ASSERT(valuePtr.GetTypeInfo() == TypeId<Float*>());

	*valuePtr.Cast<Float*>() = 7.5f;

	SEOUL_UNITTESTING_ASSERT_EQUAL(7.5f, v[2u]);

	SEOUL_UNITTESTING_ASSERT(a.TryGetElementConstPtr(vector, 2u, valuePtr));
	SEOUL_UNITTESTING_ASSERT(valuePtr.GetTypeInfo() == TypeId<Float const*>());
	SEOUL_UNITTESTING_ASSERT_EQUAL(7.5f, *valuePtr.Cast<Float const*>());

	SEOUL_UNITTESTING_ASSERT(a.TrySet(vector, 2u, 2.7f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(2.7f, v[2u]);
	SEOUL_UNITTESTING_ASSERT(a.TryGetElementConstPtr(vector, 2u, valuePtr));
	SEOUL_UNITTESTING_ASSERT(valuePtr.GetTypeInfo() == TypeId<Float const*>());
	SEOUL_UNITTESTING_ASSERT_EQUAL(2.7f, *valuePtr.Cast<Float const*>());
}

void ReflectionTest::TestReflectionConstArray()
{
	const Reflection::Array& a = ArrayOf< TestVector >();
	SEOUL_UNITTESTING_ASSERT(a.CanResize());
	SEOUL_UNITTESTING_ASSERT(a.GetElementTypeInfo() == TypeId<Float>());
	SEOUL_UNITTESTING_ASSERT(a.GetElementTypeInfo() == TypeId<TestVector::ValueType>());

	TestVector v;
	WeakAny vector((TestVector const*)&v);

	UInt32 zSize = UIntMax;
	SEOUL_UNITTESTING_ASSERT(a.TryGetSize(vector, zSize));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, zSize);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, v.GetSize());

	SEOUL_UNITTESTING_ASSERT(!a.TryResize(vector, 10u));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, v.GetSize());

	zSize = UIntMax;
	SEOUL_UNITTESTING_ASSERT(a.TryGetSize(vector, zSize));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, zSize);

	WeakAny valuePtr;
	SEOUL_UNITTESTING_ASSERT(!a.TryGetElementPtr(vector, 2u, valuePtr));
	SEOUL_UNITTESTING_ASSERT(valuePtr.GetTypeInfo() == TypeId<void>());

	SEOUL_UNITTESTING_ASSERT(!a.TrySet(vector, 2u, 2.7f));

	v.Resize(3u);
	SEOUL_UNITTESTING_ASSERT(a.TryGetElementConstPtr(vector, 2u, valuePtr));
	SEOUL_UNITTESTING_ASSERT(valuePtr.GetTypeInfo() == TypeId<Float const*>());

	SEOUL_UNITTESTING_ASSERT(!a.TryGetElementPtr(vector, 2u, valuePtr));
	SEOUL_UNITTESTING_ASSERT(!a.TrySet(vector, 2u, 2.7f));
}

void ReflectionTest::TestReflectionFixedArray()
{
	typedef FixedArray<Float, 10u> TestArray;

	const Reflection::Array& a = ArrayOf< TestArray >();
	SEOUL_UNITTESTING_ASSERT(!a.CanResize());
	SEOUL_UNITTESTING_ASSERT(a.GetElementTypeInfo() == TypeId<Float>());
	SEOUL_UNITTESTING_ASSERT(a.GetElementTypeInfo() == TypeId<TestArray::ValueType>());

	TestArray arr;
	WeakAny array(&arr);

	UInt32 zSize = UIntMax;
	SEOUL_UNITTESTING_ASSERT(a.TryGetSize(array, zSize));
	SEOUL_UNITTESTING_ASSERT_EQUAL(10u, zSize);
	SEOUL_UNITTESTING_ASSERT_EQUAL(10u, arr.GetSize());

	SEOUL_UNITTESTING_ASSERT(!a.TryResize(array, 20u));
	SEOUL_UNITTESTING_ASSERT_EQUAL(10u, arr.GetSize());

	zSize = UIntMax;
	SEOUL_UNITTESTING_ASSERT(a.TryGetSize(array, zSize));
	SEOUL_UNITTESTING_ASSERT_EQUAL(10u, zSize);

	WeakAny valuePtr;
	SEOUL_UNITTESTING_ASSERT(a.TryGetElementPtr(array, 2u, valuePtr));
	SEOUL_UNITTESTING_ASSERT(valuePtr.GetTypeInfo() == TypeId<Float*>());

	*valuePtr.Cast<Float*>() = 7.5f;

	SEOUL_UNITTESTING_ASSERT_EQUAL(7.5f, arr[2u]);

	SEOUL_UNITTESTING_ASSERT(a.TryGetElementConstPtr(array, 2u, valuePtr));
	SEOUL_UNITTESTING_ASSERT(valuePtr.GetTypeInfo() == TypeId<Float const*>());
	SEOUL_UNITTESTING_ASSERT_EQUAL(7.5f, *valuePtr.Cast<Float const*>());

	SEOUL_UNITTESTING_ASSERT(a.TrySet(array, 2u, 2.7f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(2.7f, arr[2u]);
	SEOUL_UNITTESTING_ASSERT(a.TryGetElementConstPtr(array, 2u, valuePtr));
	SEOUL_UNITTESTING_ASSERT(valuePtr.GetTypeInfo() == TypeId<Float const*>());
	SEOUL_UNITTESTING_ASSERT_EQUAL(2.7f, *valuePtr.Cast<Float const*>());
}

void ReflectionTest::TestReflectionEnum()
{
	const Enum& e = EnumOf<TestEnum::Enum>();
	SEOUL_UNITTESTING_ASSERT(e.GetName() == HString("TestEnum::Enum"));
	SEOUL_UNITTESTING_ASSERT(e.GetTypeInfo() == TypeId<TestEnum::Enum>());
	SEOUL_UNITTESTING_ASSERT(e.GetNames().GetSize() == e.GetValues().GetSize() && e.GetNames().GetSize() == 4);
	SEOUL_UNITTESTING_ASSERT(e.GetNames()[0] == HString("Zero"));
	SEOUL_UNITTESTING_ASSERT(e.GetNames()[1] == HString("One"));
	SEOUL_UNITTESTING_ASSERT(e.GetNames()[2] == HString("Two"));
	SEOUL_UNITTESTING_ASSERT(e.GetNames()[3] == HString("Three"));
	SEOUL_UNITTESTING_ASSERT(e.GetValues()[0] == 0);
	SEOUL_UNITTESTING_ASSERT(e.GetValues()[1] == 1);
	SEOUL_UNITTESTING_ASSERT(e.GetValues()[2] == 2);
	SEOUL_UNITTESTING_ASSERT(e.GetValues()[3] == 3);

	const Registry& registry = Reflection::Registry::GetRegistry();

	Type const* pType = registry.GetType(HString("ReflectionTestUtility"));
	ScopedPtr<ReflectionTestUtility> pTestInstance(pType->New<ReflectionTestUtility>(MemoryBudgets::TBD));

	Any any;
	Property const* pProperty = pType->GetProperty(HString("EnumValue"));
	SEOUL_UNITTESTING_ASSERT(nullptr != pProperty);

	SEOUL_UNITTESTING_ASSERT(pProperty->TryGet(pTestInstance.Get(), any));
	SEOUL_UNITTESTING_ASSERT(any.IsOfType<TestEnum::Enum>());
	SEOUL_UNITTESTING_ASSERT(any.GetType().TryGetEnum());
	SEOUL_UNITTESTING_ASSERT(any.GetType().TryGetEnum() == &e);
	SEOUL_UNITTESTING_ASSERT(TestEnum::kThree == any.Cast<TestEnum::Enum>());

	SEOUL_UNITTESTING_ASSERT(pProperty->TrySet(pTestInstance.Get(), HString("One")));
	SEOUL_UNITTESTING_ASSERT(TestEnum::kOne == pTestInstance->GetEnumValue());

	SEOUL_UNITTESTING_ASSERT(pProperty->TryGet(pTestInstance.Get(), any));

	HString name;
	SEOUL_UNITTESTING_ASSERT(pProperty->GetMemberTypeInfo().GetType().TryGetEnum()->TryGetName(any, name));
	SEOUL_UNITTESTING_ASSERT(name == HString("One"));

	// Alias test.
	TestEnum::Enum eValue = TestEnum::kOne;
	SEOUL_UNITTESTING_ASSERT(e.TryGetValue(HString("Zero"), eValue));
	SEOUL_UNITTESTING_ASSERT_EQUAL(TestEnum::kZero, eValue);

	eValue = TestEnum::kOne;
	SEOUL_UNITTESTING_ASSERT(e.TryGetValue(HString("zero"), eValue));
	SEOUL_UNITTESTING_ASSERT_EQUAL(TestEnum::kZero, eValue);

	name = HString();
	SEOUL_UNITTESTING_ASSERT(e.TryGetName(eValue, name));
	SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Zero"), name);
}


void ReflectionTest::TestReflectionTable()
{
	static const HString kKey("HelloWorld");
	static const String kValue("HiThere");
	static const String kValue2("HiThere2");

	const Reflection::Table& t = TableOf< TestTable >();
	SEOUL_UNITTESTING_ASSERT(t.CanErase());
	SEOUL_UNITTESTING_ASSERT(t.GetKeyTypeInfo() == TypeId<HString>());
	SEOUL_UNITTESTING_ASSERT(t.GetValueTypeInfo() == TypeId<String>());
	SEOUL_UNITTESTING_ASSERT(t.GetKeyTypeInfo() == TypeId<TestTable::KeyType>());
	SEOUL_UNITTESTING_ASSERT(t.GetValueTypeInfo() == TypeId<TestTable::ValueType>());

	TestTable table;
	WeakAny const weakTable(&table);
	WeakAny const constWeakTable(const_cast<TestTable const*>(&table));

	WeakAny valuePtr;
	SEOUL_UNITTESTING_ASSERT(!t.TryGetValuePtr(weakTable, kKey, valuePtr));
	SEOUL_UNITTESTING_ASSERT(t.TryGetValuePtr(weakTable, kKey, valuePtr, true));
	SEOUL_UNITTESTING_ASSERT(valuePtr.GetTypeInfo() == TypeId<String*>());
	SEOUL_UNITTESTING_ASSERT(*valuePtr.Cast<String*>() == String());

	*valuePtr.Cast<String*>() = kValue;

	SEOUL_UNITTESTING_ASSERT_EQUAL(kValue, *table.Find(kKey));

	SEOUL_UNITTESTING_ASSERT(t.TryGetValueConstPtr(weakTable, kKey, valuePtr));
	SEOUL_UNITTESTING_ASSERT(valuePtr.GetTypeInfo() == TypeId<String const*>());
	SEOUL_UNITTESTING_ASSERT_EQUAL(kValue, *valuePtr.Cast<String const*>());

	// Overwrite the value, make sure it has updated.
	SEOUL_UNITTESTING_ASSERT(t.TryOverwrite(weakTable, kKey, kValue2));
	SEOUL_UNITTESTING_ASSERT(t.TryGetValueConstPtr(weakTable, kKey, valuePtr));
	SEOUL_UNITTESTING_ASSERT(valuePtr.GetTypeInfo() == TypeId<String const*>());
	SEOUL_UNITTESTING_ASSERT_EQUAL(kValue2, *valuePtr.Cast<String const*>());

	// Overwrite should fail.
	SEOUL_UNITTESTING_ASSERT(!t.TryOverwrite(constWeakTable, kKey, kValue));
	SEOUL_UNITTESTING_ASSERT(t.TryGetValueConstPtr(constWeakTable, kKey, valuePtr));
	SEOUL_UNITTESTING_ASSERT(valuePtr.GetTypeInfo() == TypeId<String const*>());
	SEOUL_UNITTESTING_ASSERT_EQUAL(kValue2, *valuePtr.Cast<String const*>());

	SEOUL_UNITTESTING_ASSERT(t.TryErase(weakTable, kKey));
	SEOUL_UNITTESTING_ASSERT(!t.TryGetValuePtr(weakTable, kKey, valuePtr));
	SEOUL_UNITTESTING_ASSERT(!t.TryGetValueConstPtr(weakTable, kKey, valuePtr));
}

void ReflectionTest::TestReflectionConstTable()
{
	static const HString kKey("HelloWorld");
	static const String kValue("HiThere");

	const Reflection::Table& t = TableOf< TestTable >();
	SEOUL_UNITTESTING_ASSERT(t.CanErase());
	SEOUL_UNITTESTING_ASSERT(t.GetKeyTypeInfo() == TypeId<HString>());
	SEOUL_UNITTESTING_ASSERT(t.GetValueTypeInfo() == TypeId<String>());
	SEOUL_UNITTESTING_ASSERT(t.GetKeyTypeInfo() == TypeId<TestTable::KeyType>());
	SEOUL_UNITTESTING_ASSERT(t.GetValueTypeInfo() == TypeId<TestTable::ValueType>());

	TestTable table;
	WeakAny weakTable(const_cast<TestTable const*>(&table));

	WeakAny valuePtr;
	SEOUL_UNITTESTING_ASSERT(!t.TryGetValuePtr(weakTable, kKey, valuePtr));
	SEOUL_UNITTESTING_ASSERT(!t.TryGetValuePtr(weakTable, kKey, valuePtr, true));
	SEOUL_UNITTESTING_ASSERT(valuePtr.GetTypeInfo() == TypeId<void>());

	SEOUL_UNITTESTING_ASSERT(table.Insert(kKey, String()).Second);

	SEOUL_UNITTESTING_ASSERT(!t.TryGetValuePtr(weakTable, kKey, valuePtr));
	SEOUL_UNITTESTING_ASSERT(!t.TryGetValuePtr(weakTable, kKey, valuePtr, true));
	SEOUL_UNITTESTING_ASSERT(valuePtr.GetTypeInfo() == TypeId<void>());

	SEOUL_UNITTESTING_ASSERT(table.Overwrite(kKey, kValue).Second);

	SEOUL_UNITTESTING_ASSERT_EQUAL(kValue, *table.Find(kKey));

	SEOUL_UNITTESTING_ASSERT(t.TryGetValueConstPtr(weakTable, kKey, valuePtr));
	SEOUL_UNITTESTING_ASSERT(valuePtr.GetTypeInfo() == TypeId<String const*>());
	SEOUL_UNITTESTING_ASSERT_EQUAL(kValue, *valuePtr.Cast<String const*>());

	SEOUL_UNITTESTING_ASSERT(!t.TryErase(weakTable, kKey));
	SEOUL_UNITTESTING_ASSERT(!t.TryGetValuePtr(weakTable, kKey, valuePtr));
	SEOUL_UNITTESTING_ASSERT(t.TryGetValueConstPtr(weakTable, kKey, valuePtr));
}

void ReflectionTest::TestReflectionTypeMethods()
{
	const Registry& registry = Reflection::Registry::GetRegistry();

	Type const* pType = registry.GetType(HString("ReflectionTestUtility"));
	ScopedPtr<ReflectionTestUtility> pTestInstance(pType->New<ReflectionTestUtility>(MemoryBudgets::TBD));

	Any any;

	// ReflectionTestUtility
	Method const* pMethod = pType->GetMethod(HString("TestMethodA"));
	SEOUL_UNITTESTING_ASSERT(nullptr != pMethod);

	SEOUL_UNITTESTING_ASSERT(pMethod->TryInvoke(any, pTestInstance.Get()));
	SEOUL_UNITTESTING_ASSERT(!any.IsValid());
	SEOUL_UNITTESTING_ASSERT(pTestInstance->GetValue() == -1.0f);

	pMethod = pType->GetMethod(HString("TestMethodB"));
	SEOUL_UNITTESTING_ASSERT(nullptr != pMethod);

	MethodArguments aArguments;
	aArguments[0] = 3.0f;
	SEOUL_UNITTESTING_ASSERT(pMethod->TryInvoke(any, pTestInstance.Get(), aArguments));
	SEOUL_UNITTESTING_ASSERT(!any.IsValid());
	SEOUL_UNITTESTING_ASSERT(pTestInstance->GetValue() == 3.0f);

	aArguments[0] = 3;
	SEOUL_UNITTESTING_ASSERT(pMethod->TryInvoke(any, pTestInstance.Get(), aArguments));
	SEOUL_UNITTESTING_ASSERT(!any.IsValid());
	SEOUL_UNITTESTING_ASSERT(pTestInstance->GetValue() == 3.0f);

	// BaseReflectionTest
	pMethod = pType->GetMethod(HString("BaseTestMethodA"));
	SEOUL_UNITTESTING_ASSERT(nullptr != pMethod);

	// Alias test
	Method const* pMethodAlias = pType->GetMethod(HString("BaseTestItMethodA"));
	SEOUL_UNITTESTING_ASSERT(nullptr != pMethodAlias && pMethodAlias == pMethod);

	SEOUL_UNITTESTING_ASSERT(pMethod->TryInvoke(any, pTestInstance.Get()));
	SEOUL_UNITTESTING_ASSERT(!any.IsValid());
	SEOUL_UNITTESTING_ASSERT_EQUAL(-2, pTestInstance->GetBaseValue());

	pMethod = pType->GetMethod(HString("BaseTestMethodB"));
	SEOUL_UNITTESTING_ASSERT(nullptr != pMethod);

	aArguments[0] = 5;
	SEOUL_UNITTESTING_ASSERT(pMethod->TryInvoke(any, pTestInstance.Get(), aArguments));
	SEOUL_UNITTESTING_ASSERT(!any.IsValid());
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, pTestInstance->GetBaseValue());

	aArguments[0] = 107;
	pMethod = pType->GetMethod(HString("BaseTestMethodC"));
	SEOUL_UNITTESTING_ASSERT(nullptr != pMethod);
	SEOUL_UNITTESTING_ASSERT(TypeId<BaseReflectionTest>() == pMethod->GetTypeInfo().m_rClassTypeInfo);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1u, pMethod->GetTypeInfo().m_uArgumentCount);
	SEOUL_UNITTESTING_ASSERT(pMethod->TryInvoke(any, (BaseReflectionTest*)nullptr, aArguments));
	SEOUL_UNITTESTING_ASSERT(!any.IsValid());
	SEOUL_UNITTESTING_ASSERT_EQUAL(107, BaseReflectionTest::s_iTestValue);
}

void ReflectionTest::TestReflectionTypeProperties()
{
	const Registry& registry = Reflection::Registry::GetRegistry();

	Type const* pType = registry.GetType(HString("ReflectionTestUtility"));
	ScopedPtr<ReflectionTestUtility> pTestInstance(pType->New<ReflectionTestUtility>(MemoryBudgets::TBD));

	// ReflectionTestUtility
	Property const* pProperty = pType->GetProperty(HString("Value"));
	SEOUL_UNITTESTING_ASSERT(nullptr != pProperty);

	SEOUL_UNITTESTING_ASSERT(pProperty->TrySet(pTestInstance.Get(), -1.0f));
	SEOUL_UNITTESTING_ASSERT(pTestInstance->GetValue() == -1.0f);

	Any any;
	SEOUL_UNITTESTING_ASSERT(pProperty->TryGet(pTestInstance.Get(), any));
	SEOUL_UNITTESTING_ASSERT(any.IsOfType<Float>());
	SEOUL_UNITTESTING_ASSERT(any.Cast<Float>() == -1.0f);

	// BaseReflectionTest
	pProperty = pType->GetProperty(HString("BaseValue"));
	SEOUL_UNITTESTING_ASSERT(nullptr != pProperty);

	// Alias check
	Property const* pPropertyAlias = pType->GetProperty(HString("iBaseValue"));
	SEOUL_UNITTESTING_ASSERT(nullptr != pPropertyAlias && pPropertyAlias == pProperty);

	SEOUL_UNITTESTING_ASSERT(pProperty->TrySet(pTestInstance.Get(), 5));
	SEOUL_UNITTESTING_ASSERT(pTestInstance->GetBaseValue() == 5);

	SEOUL_UNITTESTING_ASSERT(pProperty->TryGet(pTestInstance.Get(), any));
	SEOUL_UNITTESTING_ASSERT(any.IsOfType<Int>());
	SEOUL_UNITTESTING_ASSERT(any.Cast<Int>() == 5);

	pProperty = pType->GetProperty(HString("BaseValues"));
	SEOUL_UNITTESTING_ASSERT(nullptr != pProperty);

	SEOUL_UNITTESTING_ASSERT(pProperty->TryGet(pTestInstance.Get(), any));
	SEOUL_UNITTESTING_ASSERT(any.IsOfType< TestVector >());

	TestVector& vVector = any.Cast< TestVector >();
	SEOUL_UNITTESTING_ASSERT(vVector.GetSize() == 20);
	for (UInt32 i = 0u; i < vVector.GetSize(); ++i)
	{
		SEOUL_UNITTESTING_ASSERT(vVector[i] == -2.0f);
	}

	// Check name generation.
	HString typeName(String::Printf("Vector<Float, %d>", kMemoryBudgetsType));
	SEOUL_UNITTESTING_ASSERT(Registry::GetRegistry().GetType(typeName) != nullptr);
}

void ReflectionTest::TestReflectionFieldProperies()
{
	const Registry& registry = Reflection::Registry::GetRegistry();

	Type const* pType = registry.GetType(HString("ReflectionTestUtility"));
	SEOUL_UNITTESTING_ASSERT(nullptr != pType);
	SEOUL_UNITTESTING_ASSERT(HString("ReflectionTestUtility") == pType->GetName());
	SEOUL_UNITTESTING_ASSERT(TypeId<ReflectionTestUtility>() == pType->GetTypeInfo());
	SEOUL_UNITTESTING_ASSERT(pType->GetAttributes().GetCount() == 1);
	SEOUL_UNITTESTING_ASSERT(pType->GetMethodCount() == 2);
	SEOUL_UNITTESTING_ASSERT(pType->GetParentCount() == 1);
	SEOUL_UNITTESTING_ASSERT(pType->GetPropertyCount() == 2);

	WeakAny testInstanceWeakAny = pType->New(MemoryBudgets::TBD);
	SEOUL_UNITTESTING_ASSERT(testInstanceWeakAny.IsValid());
	SEOUL_UNITTESTING_ASSERT(testInstanceWeakAny.IsOfType<ReflectionTestUtility*>());

	ScopedPtr<ReflectionTestUtility> pTestInstance(testInstanceWeakAny.Cast<ReflectionTestUtility*>());
	SEOUL_UNITTESTING_ASSERT(pTestInstance->GetBaseValue() == 200);
	SEOUL_UNITTESTING_ASSERT(pTestInstance->GetValue() == 100.0f);

	auto p = pType->GetProperty(HString("Value"));
	
	// Subclass.
	Reflection::Any any;
	SEOUL_UNITTESTING_ASSERT(p->TryGet(testInstanceWeakAny, any));
	SEOUL_UNITTESTING_ASSERT(any.IsOfType<Float>());
	SEOUL_UNITTESTING_ASSERT_EQUAL(100.0f, any.Cast<Float>());
	SEOUL_UNITTESTING_ASSERT(p->TrySet(testInstanceWeakAny, 25.0f));

	{
		Float const* pfFloat = nullptr;
		SEOUL_UNITTESTING_ASSERT(p->TryGetConstPtr(testInstanceWeakAny, pfFloat));
		SEOUL_UNITTESTING_ASSERT_EQUAL(25.0f, *pfFloat);
	}
	{
		Float* pfFloat = nullptr;
		SEOUL_UNITTESTING_ASSERT(p->TryGetPtr(testInstanceWeakAny, pfFloat));
		SEOUL_UNITTESTING_ASSERT_EQUAL(25.0f, *pfFloat);
		*pfFloat = 17.0f;
	}

	// Parent class.
	p = pType->GetProperty(HString("BaseValue"));
	SEOUL_UNITTESTING_ASSERT(p->TryGet(testInstanceWeakAny, any));
	SEOUL_UNITTESTING_ASSERT(any.IsOfType<Int>());
	SEOUL_UNITTESTING_ASSERT_EQUAL(200, any.Cast<Int>());
	SEOUL_UNITTESTING_ASSERT(p->TrySet(testInstanceWeakAny, 37));

	{
		Int const* piInt = nullptr;
		SEOUL_UNITTESTING_ASSERT(p->TryGetConstPtr(testInstanceWeakAny, piInt));
		SEOUL_UNITTESTING_ASSERT_EQUAL(37, *piInt);
	}
	{
		Int* piInt = nullptr;
		SEOUL_UNITTESTING_ASSERT(p->TryGetPtr(testInstanceWeakAny, piInt));
		SEOUL_UNITTESTING_ASSERT_EQUAL(37, *piInt);
		*piInt = -53;
	}

	// Failure cases.
	{
		WeakAny invalidThis = this;
		SEOUL_UNITTESTING_ASSERT(!p->TryGet(invalidThis, any));

		SEOUL_UNITTESTING_ASSERT(!p->TrySet(invalidThis, 33));
		SEOUL_UNITTESTING_ASSERT(!p->TrySet(testInstanceWeakAny, String("33")));

		Int* piInt = nullptr;
		SEOUL_UNITTESTING_ASSERT(!p->TryGetPtr(invalidThis, piInt));
		Int const* piConstInt = nullptr;
		SEOUL_UNITTESTING_ASSERT(!p->TryGetConstPtr(invalidThis, piConstInt));
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
