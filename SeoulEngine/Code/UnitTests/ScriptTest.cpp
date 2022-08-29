/**
 * \file ScriptTest.cpp
 * \brief Declaration of unit tests for the Script functionality.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "CrashManager.h"
#include "FixedArray.h"
#include "HashSet.h"
#include "HashTable.h"
#include "List.h"
#include "Logger.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"
#include "ScriptArrayIndex.h"
#include "ScriptFunctionInterface.h"
#include "ScriptFunctionInvoker.h"
#include "ScriptTest.h"
#include "ScriptVm.h"
#include "SeoulMath.h"
#include "SeoulUUID.h"
#include "UnitTesting.h"
#include "Vector.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(ScriptTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestAny)
	SEOUL_METHOD(TestArrayIndex)
	SEOUL_METHOD(TestBasic)
	SEOUL_METHOD(TestBindStrongInstance)
	SEOUL_METHOD(TestBindStrongTable)
	SEOUL_METHOD(TestDataStore)
	SEOUL_METHOD(TestDataStoreNilConversion)
	SEOUL_METHOD(TestDataStorePrimitives)
	SEOUL_METHOD(TestDataStoreSpecial)
	SEOUL_METHOD(TestInterfaceArgs)
	SEOUL_METHOD(TestInterfaceArgsMultiple)
	SEOUL_METHOD(TestInterfaceFilePath)
	SEOUL_METHOD(TestInterfaceRaiseError)
	SEOUL_METHOD(TestInterfaceReturn)
	SEOUL_METHOD(TestInterfaceReturnMultiple)
	SEOUL_METHOD(TestInterfaceUserData)
	SEOUL_METHOD(TestInterfaceUserDataType)
	SEOUL_METHOD(TestInvokeArgs)
	SEOUL_METHOD(TestInvokeArgsMultiple)
	SEOUL_METHOD(TestInvokeFilePath)
	SEOUL_METHOD(TestInvokeReturn)
	SEOUL_METHOD(TestInvokeReturnMultiple)
	SEOUL_METHOD(TestInvokeUserData)
	SEOUL_METHOD(TestInvokeUserDataType)
	SEOUL_METHOD(TestMultiVmClone)
	SEOUL_METHOD(TestNullObject)
	SEOUL_METHOD(TestNullScriptVmObject)
	SEOUL_METHOD(TestNumberRanges)
	SEOUL_METHOD(TestReflectionArgs)
	SEOUL_METHOD(TestReflectionMultiSuccess)
	SEOUL_METHOD(TestReflectionReturn)
	SEOUL_METHOD(TestReflectionTypes)
	SEOUL_METHOD(TestSetGlobal)
	SEOUL_METHOD(TestWeakBinding)
	SEOUL_METHOD(TestRandom)

	SEOUL_METHOD(TestI32AddNV)
	SEOUL_METHOD(TestI32DivNV)
	SEOUL_METHOD(TestI32ModExtensionNV)
	SEOUL_METHOD(TestI32MulExtensionNV)
	SEOUL_METHOD(TestI32SubNV)

	SEOUL_METHOD(TestI32AddVN)
	SEOUL_METHOD(TestI32DivVN)
	SEOUL_METHOD(TestI32ModExtensionVN)
	SEOUL_METHOD(TestI32MulExtensionVN)
	SEOUL_METHOD(TestI32SubVN)

	SEOUL_METHOD(TestI32AddVV)
	SEOUL_METHOD(TestI32DivVV)
	SEOUL_METHOD(TestI32ModExtensionVV)
	SEOUL_METHOD(TestI32MulExtensionVV)
	SEOUL_METHOD(TestI32SubVV)

	SEOUL_METHOD(TestI32Truncate)

	SEOUL_METHOD(TestI32ModExtensionWrongTypes)
	SEOUL_METHOD(TestI32MulExtensionWrongTypes)
	SEOUL_METHOD(TestI32TruncateWrongTypes)
SEOUL_END_TYPE()

static Int32 s_iCount = 0;

struct ScriptTestStruct SEOUL_SEALED
{
	ScriptTestStruct(const String& sValue = String(), Int32 iNumber = 277)
		: m_sValue(sValue)
		, m_iNumber(iNumber)
	{
	}

	Bool operator==(const ScriptTestStruct& b) const
	{
		return (m_sValue == b.m_sValue && m_iNumber == b.m_iNumber);
	}

	String m_sValue;
	Int32 m_iNumber;
}; // struct ScriptTestStruct
SEOUL_BEGIN_TYPE(ScriptTestStruct)
	SEOUL_PROPERTY_N("Value", m_sValue)
	SEOUL_PROPERTY_N("Number", m_iNumber)
SEOUL_END_TYPE()

struct ScriptTestFilePathStruct SEOUL_SEALED
{
	FilePath m_FilePath;
}; // struct ScriptTestFilePathStruct
SEOUL_BEGIN_TYPE(ScriptTestFilePathStruct)
	SEOUL_PROPERTY_N("FilePath", m_FilePath)
SEOUL_END_TYPE()

struct ScriptTestPushUserData SEOUL_SEALED
{
	ScriptTestPushUserData(Int iCount = 22)
		: m_iCount(iCount)
	{
	}
	Int m_iCount;

	Bool TestMethod()
	{
		if (0 == m_iCount)
		{
			++m_iCount;
			++s_iCount;
			return true;
		}
		return false;
	}

	void TestMethod2()
	{
		if (0 == m_iCount)
		{
			++m_iCount;
			++s_iCount;
		}
	}
}; // struct ScriptTestPushUserData
SEOUL_BEGIN_TYPE(ScriptTestPushUserData)
	SEOUL_METHOD(TestMethod)
	SEOUL_METHOD(TestMethod2)
SEOUL_END_TYPE()

struct ScriptTestReturnUserData SEOUL_SEALED
{
	ScriptTestReturnUserData()
		: m_iCount(-1)
	{
	}

	Int32 m_iCount;
}; // struct ScriptTestReturnUserData
SEOUL_TYPE(ScriptTestReturnUserData);

#define SEOUL_ANY_TEST(type, var) { Reflection::Any any; if (!p->GetAny(1, TypeId<type>(), any) || !any.IsOfType<type>() || var != any.Cast<type>()) { p->PushReturnBoolean(false); return; } }
#define SEOUL_NONE_TEST() { SharedPtr<Script::VmObject> po; for (Int iNoneTest = 2; iNoneTest < 100; iNoneTest++) { if (p->GetObject(iNoneTest, po) || !p->IsNone(iNoneTest) || !p->IsNilOrNone(iNoneTest)) { p->PushReturnBoolean(false); return; } } }
struct ScriptTestInterfaceArgsStruct SEOUL_SEALED
{
	void TestArrayIndex(Script::FunctionInterface* p) const { SEOUL_NONE_TEST(); Script::ArrayIndex i; if (p->GetArrayIndex(1, i)) { SEOUL_ANY_TEST(Script::ArrayIndex, i); p->PushReturnBoolean(0 == i); } else { p->PushReturnBoolean(false); } }
	void TestArrayIndexInvalid(Script::FunctionInterface* p) const { SEOUL_NONE_TEST(); Script::ArrayIndex i; if (p->GetArrayIndex(1, i)) { SEOUL_ANY_TEST(Script::ArrayIndex, i); p->PushReturnBoolean(UIntMax == i); } else { p->PushReturnBoolean(false); } }
	void TestBoolean(Script::FunctionInterface* p) const { SEOUL_NONE_TEST(); Bool b; if (p->GetBoolean(1, b)) { SEOUL_ANY_TEST(Bool, b); p->PushReturnBoolean(true == b); } else { p->PushReturnBoolean(false); } }
	void TestEnum(Script::FunctionInterface* p) const { SEOUL_NONE_TEST(); MemoryBudgets::Type e; if (p->GetEnum(1, e)) { SEOUL_ANY_TEST(Int, e); p->PushReturnBoolean(MemoryBudgets::Analytics == e); } else { p->PushReturnBoolean(false); } }
	void TestFilePath(Script::FunctionInterface* p) const { SEOUL_NONE_TEST(); FilePath f; if (p->GetFilePath(1, f)) { SEOUL_ANY_TEST(FilePath, f); p->PushReturnBoolean(FilePath::CreateConfigFilePath("Test") == f); } else { p->PushReturnBoolean(false); } }
	void TestFloat32(Script::FunctionInterface* p) const { SEOUL_NONE_TEST(); Float32 f; if (p->GetNumber(1, f)) { SEOUL_ANY_TEST(Float32, f); p->PushReturnBoolean(1.25f == f); } else { p->PushReturnBoolean(false); } }
	void TestFunction(Script::FunctionInterface* p) const { SEOUL_NONE_TEST(); SharedPtr<Script::VmObject> po; if (p->IsFunction(1) && p->GetFunction(1, po)) { p->PushReturnBoolean(po.IsValid()); } else { p->PushReturnBoolean(false); } }
	void TestInteger(Script::FunctionInterface* p) const { SEOUL_NONE_TEST(); Int32 i; if (p->GetInteger(1, i)) { SEOUL_ANY_TEST(Int32, i); p->PushReturnBoolean(5 == i); } else { p->PushReturnBoolean(false); } }
	void TestLightUserData(Script::FunctionInterface* p) const { SEOUL_NONE_TEST(); void* pl; if (p->GetLightUserData(1, pl)) { SEOUL_ANY_TEST(void*, pl); p->PushReturnBoolean(nullptr == pl); } else { p->PushReturnBoolean(false); } }
	void TestNil(Script::FunctionInterface* p) const { SEOUL_NONE_TEST(); if (p->IsNil(1)) { p->PushReturnBoolean(true); } else { p->PushReturnBoolean(false); } }
	void TestNumber(Script::FunctionInterface* p) const { SEOUL_NONE_TEST(); Double f; if (p->GetNumber(1, f)) { SEOUL_ANY_TEST(Double, f); p->PushReturnBoolean(1.5 == f); } else { p->PushReturnBoolean(false); } }
	void TestObject(Script::FunctionInterface* p) const { SEOUL_NONE_TEST(); SharedPtr<Script::VmObject> po; if (p->GetObject(1, po)) { p->PushReturnBoolean(po.IsValid()); } else { p->PushReturnBoolean(false); } }
	void TestString(Script::FunctionInterface* p) const { SEOUL_NONE_TEST(); HString h; String s; if (p->GetString(1, s) && p->GetString(1, h)) { SEOUL_ANY_TEST(String, s); SEOUL_ANY_TEST(HString, h); p->PushReturnBoolean("Hello World" == s && HString("Hello World") == h); } else { p->PushReturnBoolean(false); } }
	void TestStringAlsoNumber(Script::FunctionInterface* p) const { SEOUL_NONE_TEST(); Double f; if (p->GetNumber(1, f)) { SEOUL_ANY_TEST(Double, f); p->PushReturnBoolean(1.75 == f); } else { p->PushReturnBoolean(false); } }
	void TestTable(Script::FunctionInterface* p) const { SEOUL_NONE_TEST(); ScriptTestStruct st; p->GetTableAsComplex(1, st); { p->PushReturnBoolean(22 == st.m_iNumber && "What is up?" == st.m_sValue); } }
	void TestUInt(Script::FunctionInterface* p) const { SEOUL_NONE_TEST(); UInt32 u; if (p->GetUInt32(1, u)) { SEOUL_ANY_TEST(UInt32, u); p->PushReturnBoolean(32u == u); } else { p->PushReturnBoolean(false); } }
	void TestUserData(Script::FunctionInterface* p) const { SEOUL_NONE_TEST(); ScriptTestPushUserData* ud = p->GetUserData<ScriptTestPushUserData>(1); if (nullptr != ud) { p->PushReturnBoolean(ud->TestMethod()); } else { p->PushReturnBoolean(false); } }
}; // struct ScriptTestInterfaceArgsStruct
#undef SEOUL_NONE_TEST
#undef SEOUL_ANY_TEST
SEOUL_BEGIN_TYPE(ScriptTestInterfaceArgsStruct)
	SEOUL_METHOD(TestArrayIndex)
	SEOUL_METHOD(TestArrayIndexInvalid)
	SEOUL_METHOD(TestBoolean)
	SEOUL_METHOD(TestEnum)
	SEOUL_METHOD(TestFilePath)
	SEOUL_METHOD(TestFloat32)
	SEOUL_METHOD(TestFunction)
	SEOUL_METHOD(TestInteger)
	SEOUL_METHOD(TestLightUserData)
	SEOUL_METHOD(TestNil)
	SEOUL_METHOD(TestNumber)
	SEOUL_METHOD(TestObject)
	SEOUL_METHOD(TestString)
	SEOUL_METHOD(TestStringAlsoNumber)
	SEOUL_METHOD(TestTable)
	SEOUL_METHOD(TestUInt)
	SEOUL_METHOD(TestUserData)
SEOUL_END_TYPE()

struct ScriptTestInterfaceReturnStruct SEOUL_SEALED
{
	void TestArrayIndex(Script::FunctionInterface* p) const { p->PushReturnArrayIndex(Script::ArrayIndex(0)); }
	void TestBoolean(Script::FunctionInterface* p) const { p->PushReturnBoolean(true); }
	void TestByteBuffer(Script::FunctionInterface* p) const { Script::ByteBuffer buffer; buffer.m_pData = (void*)"Hello Worldasdf"; buffer.m_zDataSizeInBytes = 11; p->PushReturnByteBuffer(buffer); }
	void TestEnum(Script::FunctionInterface* p) const { p->PushReturnString("Analytics"); }
	void TestEnum2(Script::FunctionInterface* p) const { p->PushReturnEnumAsNumber(MemoryBudgets::Analytics); }
	void TestFilePath(Script::FunctionInterface* p) const { p->PushReturnFilePath(FilePath::CreateConfigFilePath("Test")); }
	void TestInteger(Script::FunctionInterface* p) const { p->PushReturnInteger(5); }
	void TestLightUserData(Script::FunctionInterface* p) const { p->PushReturnLightUserData(nullptr); }
	void TestNil(Script::FunctionInterface* p) const { p->PushReturnNil(); }
	void TestNumber(Script::FunctionInterface* p) const { p->PushReturnNumber(1.5); }
	void TestObject(Script::FunctionInterface* p) const { SharedPtr<Script::VmObject> po; SEOUL_UNITTESTING_ASSERT(p->GetScriptVm()->TryGetGlobal(HString("TestObject"), po)); p->PushReturnObject(po); }
	void TestString(Script::FunctionInterface* p) const { p->PushReturnString("Hello World"); }
	void TestString2(Script::FunctionInterface* p) const { p->PushReturnString(HString("Hello World")); }
	void TestString3(Script::FunctionInterface* p) const { p->PushReturnString(String("Hello World")); }
	void TestString4(Script::FunctionInterface* p) const { p->PushReturnString("Hello Worldasdf", 11); }
	void TestStringAlsoNumber(Script::FunctionInterface* p) const { p->PushReturnString("1.75"); }
	void TestTable(Script::FunctionInterface* p) const { p->PushReturnAsTable(ScriptTestStruct("What is up?", 22)); }
	void TestUInt(Script::FunctionInterface* p) const { p->PushReturnUInt32(32u); }
	void TestUserData(Script::FunctionInterface* p) const { *p->PushReturnUserData<ScriptTestStruct>() = ScriptTestStruct("What is up?", 22); }
}; // struct ScriptTestInterfaceReturnStruct
SEOUL_BEGIN_TYPE(ScriptTestInterfaceReturnStruct)
	SEOUL_METHOD(TestArrayIndex)
	SEOUL_METHOD(TestBoolean)
	SEOUL_METHOD(TestByteBuffer)
	SEOUL_METHOD(TestEnum)
	SEOUL_METHOD(TestEnum2)
	SEOUL_METHOD(TestFilePath)
	SEOUL_METHOD(TestInteger)
	SEOUL_METHOD(TestLightUserData)
	SEOUL_METHOD(TestNil)
	SEOUL_METHOD(TestNumber)
	SEOUL_METHOD(TestObject)
	SEOUL_METHOD(TestString)
	SEOUL_METHOD(TestString2)
	SEOUL_METHOD(TestString3)
	SEOUL_METHOD(TestString4)
	SEOUL_METHOD(TestStringAlsoNumber)
	SEOUL_METHOD(TestTable)
	SEOUL_METHOD(TestUInt)
	SEOUL_METHOD(TestUserData)
SEOUL_END_TYPE()

struct ScriptTestReflectionArgsStruct SEOUL_SEALED
{
	Bool TestArrayIndex(Script::ArrayIndex i) const
	{
		return (i == 0);
	}
	Bool TestBoolean(Bool b) const { return (b == true); }
	Bool TestEnum(MemoryBudgets::Type e) const { return (e == MemoryBudgets::Analytics); }
	Bool TestFilePath(FilePath filePath) const { return (filePath == FilePath::CreateConfigFilePath("Test")); }
	Bool TestInteger(Int32 i) const { return (5 == i); }
	Bool TestLightUserData(void* p) const { return (nullptr == p); }
	Bool TestNil() const { return true; }
	Bool TestNumber(Double f) const { return (1.5 == f); }
	Bool TestString(const String& s) const { return ("Hello World" == s); }
	Bool TestStringAlsoNumber(Double f) const { return (1.75 == f); }
	Bool TestTable(const ScriptTestStruct& t) const { return (22 == t.m_iNumber && "What is up?" == t.m_sValue); }
	Bool TestUInt(UInt32 u) const { return (32u == u); }

	// TODO: Technical limitation of reflection prevents this. I kind of want to
	// eliminate this ambiguity completely by disallowing anything but simple types to
	// generic Reflection invoked methods. Basically, if you want a complex type passed
	// to native, you need to use a Script::FunctionInterface to explicitly define the
	// conversions you want applied to the arguments.
	Bool TestUserData(ScriptTestPushUserData ud) const { return ud.TestMethod(); }
}; // struct ScriptTestReflectionArgsStruct
SEOUL_BEGIN_TYPE(ScriptTestReflectionArgsStruct)
	SEOUL_METHOD(TestArrayIndex)
	SEOUL_METHOD(TestBoolean)
	SEOUL_METHOD(TestEnum)
	SEOUL_METHOD(TestFilePath)
	SEOUL_METHOD(TestInteger)
	SEOUL_METHOD(TestLightUserData)
	SEOUL_METHOD(TestNil)
	SEOUL_METHOD(TestNumber)
	SEOUL_METHOD(TestString)
	SEOUL_METHOD(TestStringAlsoNumber)
	SEOUL_METHOD(TestTable)
	SEOUL_METHOD(TestUInt)
	SEOUL_METHOD(TestUserData)
SEOUL_END_TYPE()

struct ScriptTestReflectionMultiStruct SEOUL_SEALED
{
	ScriptTestReflectionMultiStruct()
		: m_iCount(0)
		, m_iExpectedCount(-1)
	{
	}
	void Construct(Int iCount)
	{
		m_iExpectedCount = iCount;
	}

	~ScriptTestReflectionMultiStruct()
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(m_iExpectedCount, m_iCount);
	}

	Int m_iCount;
	Int m_iExpectedCount;

	void TestArg0() { m_iCount++; }
	void TestArg1(Int a0) { m_iCount++; SEOUL_UNITTESTING_ASSERT_EQUAL(0, a0); }
	void TestArg2(Int a0, Int a1)
	{
		m_iCount++;
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, a0);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, a1);
	}
	void TestArg3(Int a0, Int a1, Int a2)
	{
		m_iCount++;
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, a0);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, a1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, a2);
	}
	void TestArg4(Int a0, Int a1, Int a2, Int a3)
	{
		m_iCount++;
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, a0);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, a1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, a2);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, a3);
	}
	void TestArg5(Int a0, Int a1, Int a2, Int a3, Int a4)
	{
		m_iCount++;
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, a0);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, a1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, a2);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, a3);
		SEOUL_UNITTESTING_ASSERT_EQUAL(4, a4);
	}
	void TestArg6(Int a0, Int a1, Int a2, Int a3, Int a4, Int a5)
	{
		m_iCount++;
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, a0);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, a1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, a2);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, a3);
		SEOUL_UNITTESTING_ASSERT_EQUAL(4, a4);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, a5);
	}
	void TestArg7(Int a0, Int a1, Int a2, Int a3, Int a4, Int a5, Int a6)
	{
		m_iCount++;
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, a0);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, a1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, a2);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, a3);
		SEOUL_UNITTESTING_ASSERT_EQUAL(4, a4);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, a5);
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, a6);
	}
	void TestArg8(Int a0, Int a1, Int a2, Int a3, Int a4, Int a5, Int a6, Int a7)
	{
		m_iCount++;
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, a0);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, a1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, a2);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, a3);
		SEOUL_UNITTESTING_ASSERT_EQUAL(4, a4);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, a5);
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, a6);
		SEOUL_UNITTESTING_ASSERT_EQUAL(7, a7);
	}
	void TestArg9(Int a0, Int a1, Int a2, Int a3, Int a4, Int a5, Int a6, Int a7, Int a8)
	{
		m_iCount++;
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, a0);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, a1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, a2);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, a3);
		SEOUL_UNITTESTING_ASSERT_EQUAL(4, a4);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, a5);
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, a6);
		SEOUL_UNITTESTING_ASSERT_EQUAL(7, a7);
		SEOUL_UNITTESTING_ASSERT_EQUAL(8, a8);
	}
	void TestArg10(Int a0, Int a1, Int a2, Int a3, Int a4, Int a5, Int a6, Int a7, Int a8, Int a9)
	{
		m_iCount++;
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, a0);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, a1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, a2);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, a3);
		SEOUL_UNITTESTING_ASSERT_EQUAL(4, a4);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, a5);
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, a6);
		SEOUL_UNITTESTING_ASSERT_EQUAL(7, a7);
		SEOUL_UNITTESTING_ASSERT_EQUAL(8, a8);
		SEOUL_UNITTESTING_ASSERT_EQUAL(9, a9);
	}
	void TestArg11(Int a0, Int a1, Int a2, Int a3, Int a4, Int a5, Int a6, Int a7, Int a8, Int a9, Int a10)
	{
		m_iCount++;
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, a0);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, a1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, a2);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, a3);
		SEOUL_UNITTESTING_ASSERT_EQUAL(4, a4);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, a5);
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, a6);
		SEOUL_UNITTESTING_ASSERT_EQUAL(7, a7);
		SEOUL_UNITTESTING_ASSERT_EQUAL(8, a8);
		SEOUL_UNITTESTING_ASSERT_EQUAL(9, a9);
		SEOUL_UNITTESTING_ASSERT_EQUAL(10, a10);
	}
	void TestArg12(Int a0, Int a1, Int a2, Int a3, Int a4, Int a5, Int a6, Int a7, Int a8, Int a9, Int a10, Int a11)
	{
		m_iCount++;
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, a0);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, a1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, a2);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, a3);
		SEOUL_UNITTESTING_ASSERT_EQUAL(4, a4);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, a5);
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, a6);
		SEOUL_UNITTESTING_ASSERT_EQUAL(7, a7);
		SEOUL_UNITTESTING_ASSERT_EQUAL(8, a8);
		SEOUL_UNITTESTING_ASSERT_EQUAL(9, a9);
		SEOUL_UNITTESTING_ASSERT_EQUAL(10, a10);
		SEOUL_UNITTESTING_ASSERT_EQUAL(11, a11);
	}
	void TestArg13(Int a0, Int a1, Int a2, Int a3, Int a4, Int a5, Int a6, Int a7, Int a8, Int a9, Int a10, Int a11, Int a12)
	{
		m_iCount++;
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, a0);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, a1);
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, a2);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, a3);
		SEOUL_UNITTESTING_ASSERT_EQUAL(4, a4);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, a5);
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, a6);
		SEOUL_UNITTESTING_ASSERT_EQUAL(7, a7);
		SEOUL_UNITTESTING_ASSERT_EQUAL(8, a8);
		SEOUL_UNITTESTING_ASSERT_EQUAL(9, a9);
		SEOUL_UNITTESTING_ASSERT_EQUAL(10, a10);
		SEOUL_UNITTESTING_ASSERT_EQUAL(11, a11);
		SEOUL_UNITTESTING_ASSERT_EQUAL(12, a12);
	}
}; // struct ScriptTestReflectionMultiStruct
SEOUL_BEGIN_TYPE(ScriptTestReflectionMultiStruct)
	SEOUL_METHOD(Construct)
	SEOUL_METHOD(TestArg0)
	SEOUL_METHOD(TestArg1)
	SEOUL_METHOD(TestArg2)
	SEOUL_METHOD(TestArg3)
	SEOUL_METHOD(TestArg4)
	SEOUL_METHOD(TestArg5)
	SEOUL_METHOD(TestArg6)
	SEOUL_METHOD(TestArg7)
	SEOUL_METHOD(TestArg8)
	SEOUL_METHOD(TestArg9)
	SEOUL_METHOD(TestArg10)
	SEOUL_METHOD(TestArg11)
	SEOUL_METHOD(TestArg12)
	SEOUL_METHOD(TestArg13)
SEOUL_END_TYPE()

struct ScriptTestReflectionReturnStruct SEOUL_SEALED
{
	Script::ArrayIndex TestArrayIndex() const { return Script::ArrayIndex(0); }
	Bool TestBoolean() const { return true; }
	String TestEnum() const { return "Analytics"; }
	MemoryBudgets::Type TestEnum2() const { return MemoryBudgets::Analytics; }
	FilePath TestFilePath() const { return FilePath::CreateConfigFilePath("Test"); }
	Int32 TestInteger() const { return 5; }
	void* TestLightUserData() const { return nullptr; }
	Double TestNumber() const { return 1.5; }
	String TestString() const { return "Hello World"; }
	String TestStringAlsoNumber() const { return "1.75"; }
	ScriptTestStruct TestTable() const { return ScriptTestStruct("What is up?", 22); }
	UInt32 TestUInt() const { return 32u; }
}; // struct ScriptTestReflectionMultiStruct
SEOUL_BEGIN_TYPE(ScriptTestReflectionReturnStruct)
	SEOUL_METHOD(TestArrayIndex)
	SEOUL_METHOD(TestBoolean)
	SEOUL_METHOD(TestEnum)
	SEOUL_METHOD(TestEnum2)
	SEOUL_METHOD(TestFilePath)
	SEOUL_METHOD(TestInteger)
	SEOUL_METHOD(TestLightUserData)
	SEOUL_METHOD(TestNumber)
	SEOUL_METHOD(TestString)
	SEOUL_METHOD(TestStringAlsoNumber)
	SEOUL_METHOD(TestTable)
	SEOUL_METHOD(TestUInt)
SEOUL_END_TYPE()

static inline Bool operator==(const HashTable<String, Int>& a, const HashTable<String, Int>& b)
{
	if (a.GetSize() != b.GetSize())
	{
		return false;
	}

	for (auto i = a.Begin(); a.End() != i; ++i)
	{
		auto p = b.Find(i->First);
		if (nullptr == p)
		{
			return false;
		}

		if (*p != i->Second)
		{
			return false;
		}
	}

	return true;
}

static inline Bool operator==(const Vector<String>& a, const Vector<String>& b)
{
	if (a.GetSize() != b.GetSize())
	{
		return false;
	}

	for (UInt32 i = 0u; i < a.GetSize(); ++i)
	{
		if (a[i] != b[i])
		{
			return false;
		}
	}

	return true;
}

struct ScriptTestComplex SEOUL_SEALED
{
	static ScriptTestComplex TestA()
	{
		ScriptTestComplex ret;
		ret.m_i = 230498;
		ret.m_f = 982379.0f;
		ret.m_t.Clear();
		SEOUL_UNITTESTING_ASSERT(ret.m_t.Insert("F", 7).Second);
		SEOUL_UNITTESTING_ASSERT(ret.m_t.Insert("HU", 1048).Second);
		SEOUL_UNITTESTING_ASSERT(ret.m_t.Insert("H308", 293878).Second);
		ret.m_v.Clear();
		ret.m_v.PushBack("Hi");
		ret.m_v.PushBack("There");
		return ret;
	}

	static ScriptTestComplex TestB()
	{
		ScriptTestComplex ret;
		ret.m_i = 9347589;
		ret.m_f = 345909.0f;
		ret.m_t.Clear();
		SEOUL_UNITTESTING_ASSERT(ret.m_t.Insert("Ger", 8).Second);
		SEOUL_UNITTESTING_ASSERT(ret.m_t.Insert("BoU", -275).Second);
		SEOUL_UNITTESTING_ASSERT(ret.m_t.Insert("093k", 9832).Second);
		ret.m_v.Clear();
		ret.m_v.PushBack("Wonderful");
		return ret;
	}

	ScriptTestComplex()
		: m_i(75)
		, m_f(33)
		, m_t()
		, m_v()
	{
		SEOUL_UNITTESTING_ASSERT(m_t.Insert("H", 27).Second);
		SEOUL_UNITTESTING_ASSERT(m_t.Insert("L", 45).Second);
		SEOUL_UNITTESTING_ASSERT(m_t.Insert("Q", 200).Second);
		SEOUL_UNITTESTING_ASSERT(m_t.Insert("R", 33).Second);
		m_v.PushBack("8");
		m_v.PushBack("9");
		m_v.PushBack("77");
	}

	Int32 m_i;
	Float32 m_f;
	HashTable<String, Int> m_t;
	Vector<String> m_v;

	Bool operator==(const ScriptTestComplex& b) const
	{
		return (
			m_i == b.m_i &&
			m_f == b.m_f &&
			m_t == b.m_t &&
			m_v == b.m_v);
	}

	SEOUL_REFERENCE_COUNTED(ScriptTestComplex);
}; // struct ScriptTestComplex

SEOUL_SPEC_TEMPLATE_TYPE(CheckedPtr<ScriptTestComplex>)
SEOUL_SPEC_TEMPLATE_TYPE(SharedPtr<ScriptTestComplex>)
SEOUL_BEGIN_TYPE(ScriptTestComplex)
	SEOUL_PROPERTY_N("i", m_i)
	SEOUL_PROPERTY_N("f", m_f)
	SEOUL_PROPERTY_N("t", m_t)
	SEOUL_PROPERTY_N("v", m_v)
SEOUL_END_TYPE()

struct ScriptTestComplex2 SEOUL_SEALED
{
	ScriptTestComplex2()
		: m_p0()
		, m_p1()
	{
	}

	~ScriptTestComplex2()
	{
		SafeDelete(m_p1);
	}

	ScriptTestComplex2(const ScriptTestComplex2& b)
		: m_p0(b.m_p0)
		, m_p1(SEOUL_NEW(MemoryBudgets::Developer) ScriptTestComplex(*b.m_p1))
	{
	}

	ScriptTestComplex2& operator=(const ScriptTestComplex2& b)
	{
		m_p0 = b.m_p0;
		m_p1.Reset(SEOUL_NEW(MemoryBudgets::Developer) ScriptTestComplex(*b.m_p1));
		return *this;
	}

	SharedPtr<ScriptTestComplex> m_p0;
	CheckedPtr<ScriptTestComplex> m_p1;
}; // struct ScriptTestComplex2
SEOUL_BEGIN_TYPE(ScriptTestComplex2)
	SEOUL_PROPERTY_N("p0", m_p0)
	SEOUL_PROPERTY_N("p1", m_p1)
SEOUL_END_TYPE()

struct ScriptTestReflectionTypesStruct SEOUL_SEALED
{
	ScriptTestReflectionTypesStruct()
		: m_iCount(0)
	{
	}

	~ScriptTestReflectionTypesStruct()
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(36, m_iCount);
	}

	Int m_iCount;

	FilePath GetTestFilePath() const { return FilePath::CreateConfigFilePath("test"); }
	void* GetTestLightUserData() const { return (void*)75; }

	Script::ArrayIndex TestArrayIndex(Script::ArrayIndex i) { SEOUL_UNITTESTING_ASSERT_EQUAL(0, i); ++m_iCount; return Script::ArrayIndex(0); }
	Atomic32 TestAtomic32(Atomic32 v) { SEOUL_UNITTESTING_ASSERT_EQUAL(23, v); ++m_iCount; return Atomic32(23); }
	Bool TestBoolean(Bool b) { SEOUL_UNITTESTING_ASSERT_EQUAL(true, b); ++m_iCount; return true; }
	Color4 TestColor4(const Color4& c) { SEOUL_UNITTESTING_ASSERT_EQUAL(Color4(0.25f, 0.5f, 0.75f, 1.0f), c); ++m_iCount; return Color4(0.25f, 0.5f, 0.75f, 1.0f); }
	ScriptTestComplex2 TestComplex(const ScriptTestComplex2& v) { SEOUL_UNITTESTING_ASSERT(*v.m_p0 == ScriptTestComplex::TestA() && *v.m_p1 == ScriptTestComplex::TestB()); ++m_iCount; return v; }
	Byte const* TestCString(Byte const* s) { SEOUL_UNITTESTING_ASSERT_EQUAL(String("Fun Times"), s); ++m_iCount; return "Fun Times"; }
	MemoryBudgets::Type TestEnum(MemoryBudgets::Type e) { SEOUL_UNITTESTING_ASSERT_EQUAL(MemoryBudgets::Content, e); ++m_iCount; return MemoryBudgets::Content; }
	MemoryBudgets::Type TestEnum2(MemoryBudgets::Type e) { SEOUL_UNITTESTING_ASSERT_EQUAL(MemoryBudgets::Content, e); ++m_iCount; return MemoryBudgets::Content; }
	FilePath TestFilePath(const FilePath& filePath) { SEOUL_UNITTESTING_ASSERT_EQUAL(FilePath::CreateConfigFilePath("test"), filePath); ++m_iCount; return filePath; }
	FixedArray<UInt8, 3> TestFixedArray(const FixedArray<UInt8, 3>& a) { SEOUL_UNITTESTING_ASSERT(a[0] == 2 && a[1] == 3 && a[2] == 4); ++m_iCount; return a; }
	Float32 TestFloat32(Float32 f) { SEOUL_UNITTESTING_ASSERT_EQUAL(-39.0f, f); ++m_iCount; return -39.0f; }
	Float64 TestFloat64(Float64 f) { SEOUL_UNITTESTING_ASSERT_EQUAL(79.0, f); ++m_iCount; return 79.0; }
	HString TestHString(HString h) { SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Oh No"), h); ++m_iCount; return HString("Oh No"); }
	List<String> TestListSimple(const List<String>& l) { SEOUL_UNITTESTING_ASSERT(l.GetSize() == 2 && l.Front() == "Hi" && l.Back() == "There"); ++m_iCount; return l; }
	HashSet<Int> TestHashSetSimple(const HashSet<Int>& h) { SEOUL_UNITTESTING_ASSERT(h.HasKey(27) && h.HasKey(49) && h.GetSize() == 2); ++m_iCount; return h; }
	HashTable<Int32, Float64> TestHashTableSimple(const HashTable<Int32, Float64>& t) { SEOUL_UNITTESTING_ASSERT(t.GetSize() == 2 && *t.Find(1) == 0.25 && *t.Find(2) == 0.75); ++m_iCount; return t; }
	Int8 TestInt8(Int8 i) { SEOUL_UNITTESTING_ASSERT_EQUAL(-7, i); ++m_iCount; return -7; }
	Int16 TestInt16(Int16 i) { SEOUL_UNITTESTING_ASSERT_EQUAL(5, i); ++m_iCount; return 5; }
	Int32 TestInt32(Int32 i) { SEOUL_UNITTESTING_ASSERT_EQUAL(-19, i); ++m_iCount; return -19; }
	Int64 TestInt64(Int64 i) { SEOUL_UNITTESTING_ASSERT_EQUAL(755, i); ++m_iCount; return 755; }
	void* TestLightUserData(void* p) { SEOUL_UNITTESTING_ASSERT_EQUAL((void*)75, p); ++m_iCount; return p; }
	Pair<Int8, UInt64> TestPairSimple(const Pair<Int8, UInt64>& pair) { SEOUL_UNITTESTING_ASSERT(25 == pair.First && 37 == pair.Second); ++m_iCount; return pair; }
	Point2DInt TestPoint2DInt(const Point2DInt& point) { SEOUL_UNITTESTING_ASSERT_EQUAL(Point2DInt(7, 85), point); ++m_iCount; return Point2DInt(7, 85); }
	Quaternion TestQuaternion(const Quaternion& q) { SEOUL_UNITTESTING_ASSERT_EQUAL(Quaternion::Identity(), q); ++m_iCount; return Quaternion::Identity(); }
	String TestString(const String& s) { SEOUL_UNITTESTING_ASSERT_EQUAL("Delicious", s); ++m_iCount; return s; }
	UInt8 TestUInt8(UInt8 u) { SEOUL_UNITTESTING_ASSERT_EQUAL(33, u); ++m_iCount; return 33; }
	UInt16 TestUInt16(UInt16 u) { SEOUL_UNITTESTING_ASSERT_EQUAL(57, u); ++m_iCount; return 57; }
	UInt32 TestUInt32(UInt32 u) { SEOUL_UNITTESTING_ASSERT_EQUAL(99, u); ++m_iCount; return 99; }
	UInt64 TestUInt64(UInt64 u) { SEOUL_UNITTESTING_ASSERT_EQUAL(873, u); ++m_iCount; return 873; }
	UUID TestUUID(const UUID& v) { SEOUL_UNITTESTING_ASSERT_EQUAL(UUID::FromString("fe731c4a-b181-4b8f-a6cb-c8acec023d6a"), v); ++m_iCount; return UUID::FromString("fe731c4a-b181-4b8f-a6cb-c8acec023d6a"); }
	Vector<Float32> TestVectorSimple(const Vector<Float32>& v) { SEOUL_UNITTESTING_ASSERT(v.GetSize() == 3 && v[0] == 0.25f && v[1] == 0.5f && v[2] == 0.75f); ++m_iCount; return v; }
	Vector2D TestVector2D(const Vector2D& v) { SEOUL_UNITTESTING_ASSERT_EQUAL(Vector2D(8, -27), v); ++m_iCount; return Vector2D(8, -27); }
	Vector3D TestVector3D(const Vector3D& v) { SEOUL_UNITTESTING_ASSERT_EQUAL(Vector3D(3, 39, 85), v); ++m_iCount; return Vector3D(3, 39, 85); }
	Vector4D TestVector4D(const Vector4D& v) { SEOUL_UNITTESTING_ASSERT_EQUAL(Vector4D(3, 39, 85, 357), v); ++m_iCount; return Vector4D(3, 39, 85, 357); }
	WorldTime TestWorldTime(const WorldTime& w) { SEOUL_UNITTESTING_ASSERT_EQUAL(WorldTime::FromSecondsInt64(3), w); ++m_iCount; return w; }
	WorldTime TestWorldTime2(const WorldTime& w) { SEOUL_UNITTESTING_ASSERT_EQUAL(WorldTime::FromSecondsInt64(3), w); ++m_iCount; return w; }

private:
	SEOUL_DISABLE_COPY(ScriptTestReflectionTypesStruct);
}; // struct ScriptTestReflectionTypesStruct
SEOUL_BEGIN_TYPE(ScriptTestReflectionTypesStruct, TypeFlags::kDisableCopy)
	SEOUL_METHOD(GetTestFilePath)
	SEOUL_METHOD(GetTestLightUserData)

	SEOUL_METHOD(TestArrayIndex)
	SEOUL_METHOD(TestAtomic32)
	SEOUL_METHOD(TestBoolean)
	SEOUL_METHOD(TestColor4)
	SEOUL_METHOD(TestComplex)
	SEOUL_METHOD(TestCString)
	SEOUL_METHOD(TestEnum)
	SEOUL_METHOD(TestEnum2)
	SEOUL_METHOD(TestFilePath)
	SEOUL_METHOD(TestFixedArray)
	SEOUL_METHOD(TestFloat32)
	SEOUL_METHOD(TestFloat64)
	SEOUL_METHOD(TestHString)
	SEOUL_METHOD(TestListSimple)
	SEOUL_METHOD(TestHashSetSimple)
	SEOUL_METHOD(TestHashTableSimple)
	SEOUL_METHOD(TestInt8)
	SEOUL_METHOD(TestInt16)
	SEOUL_METHOD(TestInt32)
	SEOUL_METHOD(TestInt64)
	SEOUL_METHOD(TestLightUserData)
	SEOUL_METHOD(TestPairSimple)
	SEOUL_METHOD(TestPoint2DInt)
	SEOUL_METHOD(TestQuaternion)
	SEOUL_METHOD(TestString)
	SEOUL_METHOD(TestUInt8)
	SEOUL_METHOD(TestUInt16)
	SEOUL_METHOD(TestUInt32)
	SEOUL_METHOD(TestUInt64)
	SEOUL_METHOD(TestUUID)
	SEOUL_METHOD(TestVectorSimple)
	SEOUL_METHOD(TestVector2D)
	SEOUL_METHOD(TestVector3D)
	SEOUL_METHOD(TestVector4D)
	SEOUL_METHOD(TestWorldTime)
	SEOUL_METHOD(TestWorldTime2)
SEOUL_END_TYPE()

void ScriptTest::TestAny()
{
	Script::VmSettings settings;
	SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));
	SEOUL_UNITTESTING_ASSERT(pVm->RunCode(
		"function Test(any) return any end\n"));

	Reflection::Any any;

	// Bool
	{
		Script::FunctionInvoker invoker(*pVm, HString("Test"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		invoker.PushAny(true);
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<Bool>(), any));
		SEOUL_UNITTESTING_ASSERT(any.IsOfType<Bool>());
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, any.Cast<Bool>());
	}

	// CString
	{
		Script::FunctionInvoker invoker(*pVm, HString("Test"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		invoker.PushAny((Byte const*)"Test test");
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<Byte const*>(), any));
		SEOUL_UNITTESTING_ASSERT(any.IsOfType<Byte const*>());
		SEOUL_UNITTESTING_ASSERT(0 == strcmp("Test test", any.Cast<Byte const*>()));
	}

	// Complex
	{
		Script::FunctionInvoker invoker(*pVm, HString("Test"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		invoker.PushAny(ScriptTestStruct("Hi Hi", 2323333));
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<ScriptTestStruct>(), any));
		SEOUL_UNITTESTING_ASSERT(any.IsOfType<ScriptTestStruct>());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ScriptTestStruct("Hi Hi", 2323333), any.Cast<ScriptTestStruct>());
	}

	// Enum
	{
		Script::FunctionInvoker invoker(*pVm, HString("Test"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		invoker.PushAny(MemoryBudgets::Analytics);
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<MemoryBudgets::Type>(), any));
		SEOUL_UNITTESTING_ASSERT(any.IsOfType<MemoryBudgets::Type>());
		SEOUL_UNITTESTING_ASSERT_EQUAL(MemoryBudgets::Analytics, any.Cast<MemoryBudgets::Type>());
	}

	// FilePath
	{
		Script::FunctionInvoker invoker(*pVm, HString("Test"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		invoker.PushAny(FilePath::CreateConfigFilePath("Test"));
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<FilePath>(), any));
		SEOUL_UNITTESTING_ASSERT(any.IsOfType<FilePath>());
		SEOUL_UNITTESTING_ASSERT_EQUAL(FilePath::CreateConfigFilePath("Test"), any.Cast<FilePath>());
	}

	// Float32
	{
		Script::FunctionInvoker invoker(*pVm, HString("Test"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		invoker.PushAny(5.0f);
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<Float32>(), any));
		SEOUL_UNITTESTING_ASSERT(any.IsOfType<Float32>());
		SEOUL_UNITTESTING_ASSERT_EQUAL(5.0f, any.Cast<Float32>());
	}

	// Float64
	{
		Script::FunctionInvoker invoker(*pVm, HString("Test"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		invoker.PushAny(75.0);
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<Float64>(), any));
		SEOUL_UNITTESTING_ASSERT(any.IsOfType<Float64>());
		SEOUL_UNITTESTING_ASSERT_EQUAL(75.0, any.Cast<Float64>());
	}

	// HString
	{
		Script::FunctionInvoker invoker(*pVm, HString("Test"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		invoker.PushAny(HString("TTTTT"));
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<HString>(), any));
		SEOUL_UNITTESTING_ASSERT(any.IsOfType<HString>());
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("TTTTT"), any.Cast<HString>());
	}

	// Int8
	{
		Script::FunctionInvoker invoker(*pVm, HString("Test"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		invoker.PushAny((Int8)8);
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<Int8>(), any));
		SEOUL_UNITTESTING_ASSERT(any.IsOfType<Int8>());
		SEOUL_UNITTESTING_ASSERT_EQUAL(8, any.Cast<Int8>());
	}

	// Int16
	{
		Script::FunctionInvoker invoker(*pVm, HString("Test"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		invoker.PushAny((Int16)23);
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<Int16>(), any));
		SEOUL_UNITTESTING_ASSERT(any.IsOfType<Int16>());
		SEOUL_UNITTESTING_ASSERT_EQUAL(23, any.Cast<Int16>());
	}

	// Int32
	{
		Script::FunctionInvoker invoker(*pVm, HString("Test"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		invoker.PushAny((Int32)-73);
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<Int32>(), any));
		SEOUL_UNITTESTING_ASSERT(any.IsOfType<Int32>());
		SEOUL_UNITTESTING_ASSERT_EQUAL(-73, any.Cast<Int32>());
	}

	// Int64
	{
		Script::FunctionInvoker invoker(*pVm, HString("Test"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		invoker.PushAny((Int64)-33);
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<Int64>(), any));
		SEOUL_UNITTESTING_ASSERT(any.IsOfType<Int64>());
		SEOUL_UNITTESTING_ASSERT_EQUAL(-33, any.Cast<Int64>());
	}

	// LightUserData
	{
		Script::FunctionInvoker invoker(*pVm, HString("Test"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		invoker.PushAny((void*)1);
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<void*>(), any));
		SEOUL_UNITTESTING_ASSERT(any.IsOfType<void*>());
		SEOUL_UNITTESTING_ASSERT_EQUAL((void*)1, any.Cast<void*>());
	}

	// Nil
	{
		Script::FunctionInvoker invoker(*pVm, HString("Test"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		invoker.PushAny(Reflection::Any());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<void>(), any));
		SEOUL_UNITTESTING_ASSERT(!any.IsValid());
	}

	// Script::ByteBuffer
	{
		Script::ByteBuffer buffer;
		buffer.m_pData = (void*)"Hello World";
		buffer.m_zDataSizeInBytes = (UInt32)strlen((const char*)buffer.m_pData);

		Script::FunctionInvoker invoker(*pVm, HString("Test"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		invoker.PushAny(buffer);
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<String>(), any));
		SEOUL_UNITTESTING_ASSERT(any.IsOfType<String>());
		SEOUL_UNITTESTING_ASSERT_EQUAL("Hello World", any.Cast<String>());
	}

	// SharedPtr<Script::VmObject>
	{
		SharedPtr<Script::VmObject> pVmObject;
		SEOUL_UNITTESTING_ASSERT(pVm->TryGetGlobal(HString("Test"), pVmObject));
		Script::FunctionInvoker invoker(*pVm, HString("Test"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		invoker.PushAny(pVmObject);
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId< SharedPtr<Script::VmObject> >(), any));
		SEOUL_UNITTESTING_ASSERT(any.IsOfType< SharedPtr<Script::VmObject> >());

		// TODO: No way to compare these on the native side right now, even
		// though they bind the same Lua object.
		// SEOUL_UNITTESTING_ASSERT_EQUAL(pVmObject, any.Cast< SharedPtr<Script::VmObject> >());
		SEOUL_UNITTESTING_ASSERT(any.Cast< SharedPtr<Script::VmObject> >().IsValid());
	}

	// String
	{
		Script::FunctionInvoker invoker(*pVm, HString("Test"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		invoker.PushAny(String("T2t2t"));
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<String>(), any));
		SEOUL_UNITTESTING_ASSERT(any.IsOfType<String>());
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("T2t2t"), any.Cast<String>());
	}

	// UInt8
	{
		Script::FunctionInvoker invoker(*pVm, HString("Test"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		invoker.PushAny((UInt8)33);
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<UInt8>(), any));
		SEOUL_UNITTESTING_ASSERT(any.IsOfType<UInt8>());
		SEOUL_UNITTESTING_ASSERT_EQUAL(33, any.Cast<UInt8>());
	}

	// UInt16
	{
		Script::FunctionInvoker invoker(*pVm, HString("Test"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		invoker.PushAny((UInt16)233);
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<UInt16>(), any));
		SEOUL_UNITTESTING_ASSERT(any.IsOfType<UInt16>());
		SEOUL_UNITTESTING_ASSERT_EQUAL(233, any.Cast<UInt16>());
	}

	// UInt32
	{
		Script::FunctionInvoker invoker(*pVm, HString("Test"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		invoker.PushAny((UInt32)75);
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<UInt32>(), any));
		SEOUL_UNITTESTING_ASSERT(any.IsOfType<UInt32>());
		SEOUL_UNITTESTING_ASSERT_EQUAL(75, any.Cast<UInt32>());
	}

	// UInt64
	{
		Script::FunctionInvoker invoker(*pVm, HString("Test"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		invoker.PushAny((UInt64)53);
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<UInt64>(), any));
		SEOUL_UNITTESTING_ASSERT(any.IsOfType<UInt64>());
		SEOUL_UNITTESTING_ASSERT_EQUAL(53, any.Cast<UInt64>());
	}
}

void ScriptTest::TestArrayIndex()
{
	// Default.
	{
		Script::ArrayIndex const index;
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, (UInt32)index);
		SEOUL_UNITTESTING_ASSERT_EQUAL(Script::ArrayIndex(0), index);
	}

	// Copy.
	{
		Script::ArrayIndex const index(5);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, (UInt32)index);
		SEOUL_UNITTESTING_ASSERT_EQUAL(Script::ArrayIndex(5), index);

		Script::ArrayIndex const indexB(index);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, (UInt32)indexB);
		SEOUL_UNITTESTING_ASSERT_EQUAL(Script::ArrayIndex(5), indexB);
	}

	// Cast.
	{
		Script::ArrayIndex const index(28);
		UInt32 const u = (UInt32)index;
		SEOUL_UNITTESTING_ASSERT_EQUAL(28, u);
	}

	// Addition.
	{
		Script::ArrayIndex const indexA(28);
		Script::ArrayIndex const indexB(17);
		SEOUL_UNITTESTING_ASSERT_EQUAL(Script::ArrayIndex(45), (indexA + indexB));
		SEOUL_UNITTESTING_ASSERT_EQUAL(Script::ArrayIndex(28), indexA);
		SEOUL_UNITTESTING_ASSERT_EQUAL(Script::ArrayIndex(17), indexB);
	}

	// Subtraction.
	{
		Script::ArrayIndex const indexA(39);
		Script::ArrayIndex const indexB(5);
		SEOUL_UNITTESTING_ASSERT_EQUAL(Script::ArrayIndex(34), (indexA - indexB));
		SEOUL_UNITTESTING_ASSERT_EQUAL(Script::ArrayIndex(39), indexA);
		SEOUL_UNITTESTING_ASSERT_EQUAL(Script::ArrayIndex(5), indexB);
	}

	// Addition in-place.
	{
		Script::ArrayIndex index(28);
		Script::ArrayIndex const indexB(17);
		index += indexB;

		SEOUL_UNITTESTING_ASSERT_EQUAL(Script::ArrayIndex(45), index);
		SEOUL_UNITTESTING_ASSERT_EQUAL(Script::ArrayIndex(17), indexB);
	}

	// Subtraction in-place.
	{
		Script::ArrayIndex index(39);
		Script::ArrayIndex const indexB(5);
		index -= indexB;

		SEOUL_UNITTESTING_ASSERT_EQUAL(Script::ArrayIndex(34), index);
		SEOUL_UNITTESTING_ASSERT_EQUAL(Script::ArrayIndex(5), indexB);
	}

	// Pre-increment.
	{
		Script::ArrayIndex index(28);
		Script::ArrayIndex const indexB(++index);

		SEOUL_UNITTESTING_ASSERT_EQUAL(Script::ArrayIndex(29), index);
		SEOUL_UNITTESTING_ASSERT_EQUAL(Script::ArrayIndex(29), indexB);
	}

	// Post-increment.
	{
		Script::ArrayIndex index(28);
		Script::ArrayIndex const indexB(index++);

		SEOUL_UNITTESTING_ASSERT_EQUAL(Script::ArrayIndex(29), index);
		SEOUL_UNITTESTING_ASSERT_EQUAL(Script::ArrayIndex(28), indexB);
	}

	// Pre-decrement.
	{
		Script::ArrayIndex index(39);
		Script::ArrayIndex const indexB(--index);

		SEOUL_UNITTESTING_ASSERT_EQUAL(Script::ArrayIndex(38), index);
		SEOUL_UNITTESTING_ASSERT_EQUAL(Script::ArrayIndex(38), indexB);
	}

	// Post-decrement.
	{
		Script::ArrayIndex index(39);
		Script::ArrayIndex const indexB(index--);

		SEOUL_UNITTESTING_ASSERT_EQUAL(Script::ArrayIndex(38), index);
		SEOUL_UNITTESTING_ASSERT_EQUAL(Script::ArrayIndex(39), indexB);
	}
}

void ScriptTest::TestBasic()
{
	Script::VmSettings settings;
	SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));

	SEOUL_UNITTESTING_ASSERT(pVm->RunCode("function Test() return 'Hello World' end"));

	Script::FunctionInvoker invoker(*pVm, HString("Test"));
	SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
	SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

	String s;
	SEOUL_UNITTESTING_ASSERT(invoker.GetString(0, s));
	SEOUL_UNITTESTING_ASSERT_EQUAL("Hello World", s);
}

void ScriptTest::TestBindStrongInstance()
{
	Script::VmSettings settings;
	SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));

	SharedPtr<Script::VmObject> pBinding;
	ScriptTestPushUserData* pUserData = nullptr;
	SEOUL_UNITTESTING_ASSERT(pVm->BindStrongInstance(pBinding, pUserData));
	SEOUL_UNITTESTING_ASSERT(pBinding.IsValid());
	SEOUL_UNITTESTING_ASSERT(nullptr != pUserData);
	SEOUL_UNITTESTING_ASSERT_EQUAL(22, pUserData->m_iCount);
}

void ScriptTest::TestBindStrongTable()
{
	Script::VmSettings settings;
	SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));

	DataStore dataStore;
	dataStore.MakeTable();
	SEOUL_UNITTESTING_ASSERT(dataStore.SetStringToTable(dataStore.GetRootNode(), HString("TestIt"), "Testing Testing Testing."));

	SharedPtr<Script::VmObject> pBinding;
	SEOUL_UNITTESTING_ASSERT(pVm->BindStrongTable(pBinding, dataStore, dataStore.GetRootNode()));
	SEOUL_UNITTESTING_ASSERT(pBinding.IsValid());

	DataStore dataStore2;
	SEOUL_UNITTESTING_ASSERT(pBinding->TryToDataStore(dataStore2));

	SEOUL_UNITTESTING_ASSERT(DataStore::Equals(dataStore, dataStore.GetRootNode(), dataStore2, dataStore2.GetRootNode()));
}

// TODO: There are a few edge cases that we currently live with which are
// specifically avoided by these tests:
// - an empty array will become an empty table in script.
// - any null element in an array will effectively "break" the array on the lua
//   side (the # operator will ignore everything after that nil element):
//   - this latter one is particularly bad, as it requires the script side to
//     manually iterate the array with pairs() and count the elements by finding
//     the greatest integer key value in the table.
// - a null element in a table will disappear, since null values in lua are
//   exactly equivalent to erasing the element from the table (null values
//   are restored in a arrays only because a DataStore fills in unspecified
//   slots with null).
void ScriptTest::TestDataStore()
{
	Script::VmSettings settings;
	SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));
	SEOUL_UNITTESTING_ASSERT(pVm->RunCode(
		"function Test(t) return t end\n"));

	DataStore dataStore;
	dataStore.MakeArray();
	{
		SEOUL_UNITTESTING_ASSERT(dataStore.SetArrayToArray(dataStore.GetRootNode(), 0u));

		DataNode node;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(dataStore.GetRootNode(), 0u, node));

		SEOUL_UNITTESTING_ASSERT(dataStore.SetTableToArray(node, 0u));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetBooleanValueToArray(node, 1u, true));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetFilePathToArray(node, 2u, FilePath::CreateConfigFilePath("Test")));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToArray(node, 3u, FloatMax));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToArray(node, 4u, -FloatMax));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(node, 5u, IntMax));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(node, 6u, IntMin));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt64ValueToArray(node, 7u, FlInt64Max));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetNullValueToArray(node, 8u));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetStringToArray(node, 9u, "Hello World"));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetTableToArray(node, 10u));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetUInt32ValueToArray(node, 11u, UIntMax));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetUInt32ValueToArray(node, 12u, 0u));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetUInt64ValueToArray(node, 13u, (UInt64)FlInt64Max));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(node, 14u, 0));
	}
	{
		SEOUL_UNITTESTING_ASSERT(dataStore.SetTableToArray(dataStore.GetRootNode(), 1u));

		DataNode node;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(dataStore.GetRootNode(), 1u, node));

		SEOUL_UNITTESTING_ASSERT(dataStore.SetTableToTable(node, HString("0")));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetBooleanValueToTable(node, HString("1"), true));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetFilePathToTable(node, HString("2"), FilePath::CreateConfigFilePath("Test")));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToTable(node, HString("3"), FloatMax));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetFloat32ValueToTable(node, HString("4"), -FloatMax));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(node, HString("5"), IntMax));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(node, HString("6"), IntMin));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt64ValueToTable(node, HString("7"), FlInt64Max));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetStringToTable(node, HString("9"), "Hello World"));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetTableToTable(node, HString("10")));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetUInt32ValueToTable(node, HString("11"), UIntMax));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetUInt32ValueToTable(node, HString("12"), 0u));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetUInt64ValueToTable(node, HString("13"), (UInt64)FlInt64Max));
		SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(node, HString("14"), 0));
	}

	Script::FunctionInvoker invoker(*pVm, HString("Test"));
	SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
	SEOUL_UNITTESTING_ASSERT(invoker.PushDataNode(dataStore, dataStore.GetRootNode()));
	SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());

	DataStore dataStore2;
	SEOUL_UNITTESTING_ASSERT(invoker.GetTable(0, dataStore2));

	SEOUL_UNITTESTING_ASSERT(DataStore::Equals(
		dataStore,
		dataStore.GetRootNode(),
		dataStore2,
		dataStore2.GetRootNode()));
}

void ScriptTest::TestDataStoreNilConversion()
{
	// Null value in an array (as nil).
	{
		Script::VmSettings settings;
		SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));
		SEOUL_UNITTESTING_ASSERT(pVm->RunCode(
			"function Test(t)\n"
			"    local v = t[1]\n"
			"    if v then\n"
			"        error('Expected nil value')\n"
			"    end\n"
			"    return true\n"
			"end\n"));

		// DataStore table with a nil value.
		DataStore dataStore;
		dataStore.MakeArray();
		dataStore.SetNullValueToArray(dataStore.GetRootNode(), 0);

		Script::FunctionInvoker invoker(*pVm, HString("Test"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.PushDataNode(dataStore, dataStore.GetRootNode()));
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());

		Bool b = false;
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, b));
		SEOUL_UNITTESTING_ASSERT(b);
	}

	// Null value in a table (as nil).
	{
		Script::VmSettings settings;
		SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));
		SEOUL_UNITTESTING_ASSERT(pVm->RunCode(
			"function Test(t)\n"
			"    local v = t['test']\n"
			"    if v then\n"
			"        error('Expected nil value')\n"
			"    end\n"
			"    return true\n"
			"end\n"));

		// DataStore table with a nil value.
		DataStore dataStore;
		dataStore.MakeTable();
		dataStore.SetNullValueToTable(dataStore.GetRootNode(), HString("test"));

		Script::FunctionInvoker invoker(*pVm, HString("Test"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.PushDataNode(dataStore, dataStore.GetRootNode()));
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());

		Bool b = false;
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, b));
		SEOUL_UNITTESTING_ASSERT(b);
	}

	// Null value in an array (to empty table).
	{
		Script::VmSettings settings;
		SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));
		SEOUL_UNITTESTING_ASSERT(pVm->RunCode(
			"function Test(t)\n"
			"    local v = t[1]\n"
			"    if type(v) ~= 'table' or next(v) then\n"
			"        error('Expected empty table')\n"
			"    end\n"
			"    return true\n"
			"end\n"));

		// DataStore table with a nil value.
		DataStore dataStore;
		dataStore.MakeArray();
		dataStore.SetNullValueToArray(dataStore.GetRootNode(), 0);

		Script::FunctionInvoker invoker(*pVm, HString("Test"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.PushDataNode(dataStore, dataStore.GetRootNode(), true));
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());

		Bool b = false;
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, b));
		SEOUL_UNITTESTING_ASSERT(b);
	}

	// Null value in a table (to empty table).
	{
		Script::VmSettings settings;
		SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));
		SEOUL_UNITTESTING_ASSERT(pVm->RunCode(
			"function Test(t)\n"
			"    local v = t['test']\n"
			"    if type(v) ~= 'table' or next(v) then\n"
			"        error('Expected empty table')\n"
			"    end\n"
			"    return true\n"
			"end\n"));

		// DataStore table with a nil value.
		DataStore dataStore;
		dataStore.MakeTable();
		dataStore.SetNullValueToTable(dataStore.GetRootNode(), HString("test"));

		Script::FunctionInvoker invoker(*pVm, HString("Test"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.PushDataNode(dataStore, dataStore.GetRootNode(), true));
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());

		Bool b = false;
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, b));
		SEOUL_UNITTESTING_ASSERT(b);
	}
}

void ScriptTest::TestDataStorePrimitives()
{
	Script::VmSettings settings;
	SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));
	SEOUL_UNITTESTING_ASSERT(pVm->RunCode(
		"function TestBool(t) return t == true end\n"
		"function TestFilePath(t) return t:ToSerializedUrl() == 'config://test_file_path' end\n"
		"function TestFloat32(t) return t == 1.5 end\n"
		"function TestInt32(t) return t == 5 end\n"
		"function TestInt64(t) return t == 4294967296 end\n"
		"function TestNull(t) return t == nil end\n"
		"function TestString(t) return t == 'Hello World' end\n"
		"function TestUInt32(t) return t == 4294967295 end\n"));

	// Array test.
	{
		// Setup our DataStore.
		DataStore dataStore;
		dataStore.MakeArray();
		auto arr = dataStore.GetRootNode();
		dataStore.SetBooleanValueToArray(arr, 0u, true);
		dataStore.SetFilePathToArray(arr, 1u, FilePath::CreateConfigFilePath("test_file_path"));
		dataStore.SetFloat32ValueToArray(arr, 2u, 1.5f);
		dataStore.SetInt32ValueToArray(arr, 3u, 5);
		dataStore.SetInt64ValueToArray(arr, 4u, (Int64)UIntMax + (Int64)1);
		dataStore.SetNullValueToArray(arr, 5u);
		dataStore.SetStringToArray(arr, 6u, "Hello World");
		dataStore.SetUInt32ValueToArray(arr, 7u, UIntMax);

		// Test bool.
		{
			DataNode node;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(arr, 0u, node));

			Script::FunctionInvoker invoker(*pVm, HString("TestBool"));
			SEOUL_UNITTESTING_ASSERT(invoker.PushDataNode(dataStore, node));
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());

			Bool bReturn = false;
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0u, bReturn));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bReturn);
		}

		// Test FilePath.
		{
			DataNode node;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(arr, 1u, node));

			Script::FunctionInvoker invoker(*pVm, HString("TestFilePath"));
			SEOUL_UNITTESTING_ASSERT(invoker.PushDataNode(dataStore, node));
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());

			Bool bReturn = false;
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0u, bReturn));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bReturn);
		}

		// Test Float32.
		{
			DataNode node;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(arr, 2u, node));

			Script::FunctionInvoker invoker(*pVm, HString("TestFloat32"));
			SEOUL_UNITTESTING_ASSERT(invoker.PushDataNode(dataStore, node));
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());

			Bool bReturn = false;
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0u, bReturn));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bReturn);
		}

		// Test Int32.
		{
			DataNode node;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(arr, 3u, node));

			Script::FunctionInvoker invoker(*pVm, HString("TestInt32"));
			SEOUL_UNITTESTING_ASSERT(invoker.PushDataNode(dataStore, node));
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());

			Bool bReturn = false;
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0u, bReturn));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bReturn);
		}

		// Test Int64.
		{
			DataNode node;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(arr, 4u, node));

			Script::FunctionInvoker invoker(*pVm, HString("TestInt64"));
			SEOUL_UNITTESTING_ASSERT(invoker.PushDataNode(dataStore, node));
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());

			Bool bReturn = false;
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0u, bReturn));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bReturn);
		}

		// Test Null.
		{
			DataNode node;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(arr, 5u, node));

			Script::FunctionInvoker invoker(*pVm, HString("TestNull"));
			SEOUL_UNITTESTING_ASSERT(invoker.PushDataNode(dataStore, node));
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());

			Bool bReturn = false;
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0u, bReturn));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bReturn);
		}

		// Test String.
		{
			DataNode node;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(arr, 6u, node));

			Script::FunctionInvoker invoker(*pVm, HString("TestString"));
			SEOUL_UNITTESTING_ASSERT(invoker.PushDataNode(dataStore, node));
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());

			Bool bReturn = false;
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0u, bReturn));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bReturn);
		}

		// Test UInt32.
		{
			DataNode node;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(arr, 7u, node));

			Script::FunctionInvoker invoker(*pVm, HString("TestUInt32"));
			SEOUL_UNITTESTING_ASSERT(invoker.PushDataNode(dataStore, node));
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());

			Bool bReturn = false;
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0u, bReturn));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bReturn);
		}
	}

	// Table test.
	{
		// Setup our DataStore.
		DataStore dataStore;
		dataStore.MakeTable();
		auto tbl = dataStore.GetRootNode();
		dataStore.SetBooleanValueToTable(tbl, HString("0"), true);
		dataStore.SetFilePathToTable(tbl, HString("1"), FilePath::CreateConfigFilePath("test_file_path"));
		dataStore.SetFloat32ValueToTable(tbl, HString("2"), 1.5f);
		dataStore.SetInt32ValueToTable(tbl, HString("3"), 5);
		dataStore.SetInt64ValueToTable(tbl, HString("4"), (Int64)UIntMax + (Int64)1);
		dataStore.SetNullValueToTable(tbl, HString("5"));
		dataStore.SetStringToTable(tbl, HString("6"), "Hello World");
		dataStore.SetUInt32ValueToTable(tbl, HString("7"), UIntMax);

		// Test bool.
		{
			DataNode node;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(tbl, HString("0"), node));

			Script::FunctionInvoker invoker(*pVm, HString("TestBool"));
			SEOUL_UNITTESTING_ASSERT(invoker.PushDataNode(dataStore, node));
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());

			Bool bReturn = false;
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0u, bReturn));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bReturn);
		}

		// Test FilePath.
		{
			DataNode node;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(tbl, HString("1"), node));

			Script::FunctionInvoker invoker(*pVm, HString("TestFilePath"));
			SEOUL_UNITTESTING_ASSERT(invoker.PushDataNode(dataStore, node));
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());

			Bool bReturn = false;
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0u, bReturn));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bReturn);
		}

		// Test Float32.
		{
			DataNode node;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(tbl, HString("2"), node));

			Script::FunctionInvoker invoker(*pVm, HString("TestFloat32"));
			SEOUL_UNITTESTING_ASSERT(invoker.PushDataNode(dataStore, node));
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());

			Bool bReturn = false;
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0u, bReturn));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bReturn);
		}

		// Test Int32.
		{
			DataNode node;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(tbl, HString("3"), node));

			Script::FunctionInvoker invoker(*pVm, HString("TestInt32"));
			SEOUL_UNITTESTING_ASSERT(invoker.PushDataNode(dataStore, node));
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());

			Bool bReturn = false;
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0u, bReturn));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bReturn);
		}

		// Test Int64.
		{
			DataNode node;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(tbl, HString("4"), node));

			Script::FunctionInvoker invoker(*pVm, HString("TestInt64"));
			SEOUL_UNITTESTING_ASSERT(invoker.PushDataNode(dataStore, node));
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());

			Bool bReturn = false;
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0u, bReturn));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bReturn);
		}

		// Test Null.
		{
			DataNode node;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(tbl, HString("5"), node));

			Script::FunctionInvoker invoker(*pVm, HString("TestNull"));
			SEOUL_UNITTESTING_ASSERT(invoker.PushDataNode(dataStore, node));
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());

			Bool bReturn = false;
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0u, bReturn));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bReturn);
		}

		// Test String.
		{
			DataNode node;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(tbl, HString("6"), node));

			Script::FunctionInvoker invoker(*pVm, HString("TestString"));
			SEOUL_UNITTESTING_ASSERT(invoker.PushDataNode(dataStore, node));
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());

			Bool bReturn = false;
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0u, bReturn));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bReturn);
		}

		// Test UInt32.
		{
			DataNode node;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(tbl, HString("7"), node));

			Script::FunctionInvoker invoker(*pVm, HString("TestUInt32"));
			SEOUL_UNITTESTING_ASSERT(invoker.PushDataNode(dataStore, node));
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());

			Bool bReturn = false;
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0u, bReturn));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bReturn);
		}
	}
}

void ScriptTest::TestDataStoreSpecial()
{
	Script::VmSettings settings;
	SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));
	SEOUL_UNITTESTING_ASSERT(pVm->RunCode(
		"function Test(t) return t end\n"));

	// Empty array.
	{
		DataStore dataStore;
		dataStore.MakeArray();
		dataStore.SetArrayToArray(dataStore.GetRootNode(), 0u);

		Script::FunctionInvoker invoker(*pVm, HString("Test"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.PushDataNode(dataStore, dataStore.GetRootNode()));
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());

		DataStore dataStore2;
		SEOUL_UNITTESTING_ASSERT(invoker.GetTable(0, dataStore2));

		// TODO: Edge case - everything's a table in Lua, so an empty
		// array is just an empty table.
		DataNode value;
		SEOUL_UNITTESTING_ASSERT(dataStore2.GetValueFromArray(dataStore2.GetRootNode(), 0u, value));
		SEOUL_UNITTESTING_ASSERT(value.IsTable());
	}

	// Empty table.
	{
		DataStore dataStore;
		dataStore.MakeArray();
		dataStore.SetTableToArray(dataStore.GetRootNode(), 0u);

		Script::FunctionInvoker invoker(*pVm, HString("Test"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.PushDataNode(dataStore, dataStore.GetRootNode()));
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());

		DataStore dataStore2;
		SEOUL_UNITTESTING_ASSERT(invoker.GetTable(0, dataStore2));
		SEOUL_UNITTESTING_ASSERT(DataStore::Equals(
			dataStore,
			dataStore.GetRootNode(),
			dataStore2,
			dataStore2.GetRootNode()));
	}

	// Array with a large Int64 value, this will fail. Lua uses
	// double for numbers, which cannot represent a large Int64 value. We'd need
	// to add a BigNumber or BigInt to support this.
	{
		DataStore dataStore;
		dataStore.MakeArray();
		dataStore.SetInt64ValueToArray(dataStore.GetRootNode(), 0u, FlInt64Max + 1);

		Script::FunctionInvoker invoker(*pVm, HString("Test"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(!invoker.PushDataNode(dataStore, dataStore.GetRootNode()));
	}

	// Array with a max UInt64 value, this will fail. Lua uses
	// double for numbers, which cannot represent a UInt64 value. We'd need
	// to add a BigNumber or BigInt to support this.
	{
		DataStore dataStore;
		dataStore.MakeArray();
		dataStore.SetUInt64ValueToArray(dataStore.GetRootNode(), 0u, UInt64Max);

		Script::FunctionInvoker invoker(*pVm, HString("Test"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(!invoker.PushDataNode(dataStore, dataStore.GetRootNode()));
	}

	// Table with a large Int64 value, this will fail. Lua uses
	// double for numbers, which cannot represent a large Int64 value. We'd need
	// to add a BigNumber or BigInt to support this.
	{
		DataStore dataStore;
		dataStore.MakeTable();
		dataStore.SetInt64ValueToTable(dataStore.GetRootNode(), HString("0"), FlInt64Max + 1);

		Script::FunctionInvoker invoker(*pVm, HString("Test"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(!invoker.PushDataNode(dataStore, dataStore.GetRootNode()));
	}

	// Table with a max UInt64 value, this will fail. Lua uses
	// double for numbers, which cannot represent a UInt64 value. We'd need
	// to add a BigNumber or BigInt to support this.
	{
		DataStore dataStore;
		dataStore.MakeTable();
		dataStore.SetUInt64ValueToTable(dataStore.GetRootNode(), HString("0"), UInt64Max);

		Script::FunctionInvoker invoker(*pVm, HString("Test"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(!invoker.PushDataNode(dataStore, dataStore.GetRootNode()));
	}

	// Null in a table - nulls are "absorbed" on the script side, since a nil
	// value in a table is equivalent to an erase.
	{
		DataStore dataStore;
		dataStore.MakeTable();
		dataStore.SetNullValueToTable(dataStore.GetRootNode(), HString("1"));

		Script::FunctionInvoker invoker(*pVm, HString("Test"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.PushDataNode(dataStore, dataStore.GetRootNode()));
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());

		DataStore dataStore2;
		SEOUL_UNITTESTING_ASSERT(invoker.GetTable(0, dataStore2));
		SEOUL_UNITTESTING_ASSERT(dataStore2.GetRootNode().IsTable());
		UInt32 uTableSize = 0u;
		SEOUL_UNITTESTING_ASSERT(dataStore2.GetTableCount(dataStore2.GetRootNode(), uTableSize));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0u, uTableSize);
	}
}

void ScriptTest::TestInterfaceArgs()
{
	Script::VmSettings settings;
	SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));
	SEOUL_UNITTESTING_ASSERT(pVm->RunCode(
		"local native = SeoulNativeNewNativeUserData('ScriptTestInterfaceArgsStruct')\n"
		"function TestArrayIndex(i) return native:TestArrayIndex(i) end\n"
		"function TestArrayIndexInvalid(i) return native:TestArrayIndexInvalid(0) end\n"
		"function TestBoolean(b) return native:TestBoolean(b) end\n"
		"function TestEnum(i) return native:TestEnum(i) end\n" // 0 is MemoryBudgets::Analytics
		"function TestFilePath(filePath) return native:TestFilePath(filePath) end\n"
		"function TestFloat32(f) return native:TestFloat32(f) end\n"
		"function TestFunction(f) return native:TestFunction(f) end\n"
		"function TestInteger(i) return native:TestInteger(i) end\n"
		"function TestLightUserData(l) return native:TestLightUserData(l) end\n"
		"function TestNil(n) return native:TestNil(n) end\n"
		"function TestNumber(f) return native:TestNumber(f) end\n"
		"function TestObject(o) return native:TestObject(o) end\n"
		"function TestString(s) return native:TestString(s) end\n"
		"function TestStringAlsoNumber(s) return native:TestStringAlsoNumber(s) end\n"
		"function TestTable(t) return native:TestTable(t) end\n"
		"function TestUInt(u) return native:TestUInt(u) end\n"
		"function TestUserData(ud) return native:TestUserData(ud) end\n"));

	// Any.
	{
		Reflection::Any any;
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestArrayIndex"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(Script::ArrayIndex(0));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestArrayIndexInvalid"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(Script::ArrayIndex(0));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestBoolean"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(true);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestEnum"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(MemoryBudgets::Analytics);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestFilePath"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(FilePath::CreateConfigFilePath("Test"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestFloat32"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(1.25f);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestInteger"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(5);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestLightUserData"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny((void*)(nullptr));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestNil"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(Reflection::Any());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestNumber"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(1.5);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			SharedPtr<Script::VmObject> p;
			SEOUL_UNITTESTING_ASSERT(pVm->TryGetGlobal(HString("TestObject"), p));
			Script::FunctionInvoker invoker(*pVm, HString("TestObject"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(p);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestString"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny((Byte const*)"Hello World");
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestString"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(String("Hello World"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestString"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(HString("Hello World"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestStringAlsoNumber"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny((Byte const*)"1.75");
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestStringAlsoNumber"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(String("1.75"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestStringAlsoNumber"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(HString("1.75"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestTable"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(ScriptTestStruct("What is up?", 22));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestUInt"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(32u);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
	}

	// ArrayIndex
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestArrayIndex"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushArrayIndex(Script::ArrayIndex(0));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// ArrayIndex (invalid - lua returns 0)
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestArrayIndexInvalid"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushArrayIndex(Script::ArrayIndex(0));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// Boolean
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestBoolean"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushBoolean(true);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// Enum
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestEnum"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushEnumAsNumber(MemoryBudgets::Analytics);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// FilePath
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestFilePath"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushFilePath(FilePath::CreateConfigFilePath("Test"));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// Float32
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestFloat32"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushNumber(1.25f);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// Function
	{
		Bool bResult = false;
		SharedPtr<Script::VmObject> p;
		SEOUL_UNITTESTING_ASSERT(pVm->TryGetGlobal(HString("TestFunction"), p));
		Script::FunctionInvoker invoker(*pVm, HString("TestFunction"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushObject(p);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// Integer
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestInteger"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushInteger(5);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// Light user data.
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestLightUserData"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushLightUserData(nullptr);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// Nil
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestNil"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushNil();
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// Number
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestNumber"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushNumber(1.5);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// Object
	{
		Bool bResult = false;
		SharedPtr<Script::VmObject> p;
		SEOUL_UNITTESTING_ASSERT(pVm->TryGetGlobal(HString("TestObject"), p));
		Script::FunctionInvoker invoker(*pVm, HString("TestObject"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushObject(p);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// String
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestString"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushString((Byte const*)"Hello World");
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// String
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestString"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushString((Byte const*)"Hello World", 11);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// String
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestString"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushString(String("Hello World"));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// String
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestString"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushString(HString("Hello World"));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// StringAsNumber
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestStringAlsoNumber"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushString((Byte const*)"1.75");
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// StringAsNumber
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestStringAlsoNumber"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushString((Byte const*)"1.75", 4);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// StringAsNumber
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestStringAlsoNumber"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushString(String("1.75"));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// StringAsNumber
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestStringAlsoNumber"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushString(HString("1.75"));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// Table
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestTable"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushAsTable(ScriptTestStruct("What is up?", 22));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// Table
	{
		Bool bResult = false;
		DataStore dataStore;
		dataStore.MakeTable();
		dataStore.SetStringToTable(dataStore.GetRootNode(), HString("Value"), "What is up?");
		dataStore.SetInt32ValueToTable(dataStore.GetRootNode(), HString("Number"), 22);

		Script::FunctionInvoker invoker(*pVm, HString("TestTable"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.PushDataNode(dataStore, dataStore.GetRootNode()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// UInt
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestUInt"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushUInt32(32u);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// UserData
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestUserData"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushUserData<ScriptTestPushUserData>()->m_iCount = 0;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, s_iCount); s_iCount = 0;
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}
}

struct ScriptTestInterfaceArgsMultiple SEOUL_SEALED
{
	void TestMultiple(Script::FunctionInterface* p)
	{
		// 1 here, not because indices are 1-based (they are converted
		// to 0-based in native), but because argument 0 is self.
		for (Int i = 1; i < p->GetArgumentCount(); ++i)
		{
			Int32 v;
			if (!p->GetInteger(i, v))
			{
				p->RaiseError(i, "invalid");
				return;
			}

			if (v != i)
			{
				p->RaiseError(i, "invalid, %d ~= %d", i, v);
				return;
			}
		}
	}
};
SEOUL_BEGIN_TYPE(ScriptTestInterfaceArgsMultiple)
	SEOUL_METHOD(TestMultiple)
SEOUL_END_TYPE()

void ScriptTest::TestInterfaceArgsMultiple()
{
	static const Int iMaxArgs = 100;

	Script::VmSettings settings;
	SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));
	SEOUL_UNITTESTING_ASSERT(pVm->RunCode(
		"local native = SeoulNativeNewNativeUserData('ScriptTestInterfaceArgsMultiple')\n"
		"function TestMultiple(...)\n"
		"	native:TestMultiple(...)\n"
		"end\n"));

	for (Int i = 0; i < iMaxArgs; ++i)
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestMultiple"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		for (Int32 j = 0; j < i; ++j)
		{
			invoker.PushInteger(j + 1);
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(i, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetReturnCount());
	}
}

struct ScriptTestInterfaceFilePath SEOUL_SEALED
{
	void TestFilePath(Script::FunctionInterface* p)
	{
		FilePath filePath;
		if (!p->GetFilePath(1, filePath))
		{
			p->RaiseError(1, "expected Filepath");
			return;
		}

		p->PushReturnFilePath(filePath);
		{
			DataStore dataStore;
			dataStore.MakeTable();
			dataStore.SetFilePathToTable(dataStore.GetRootNode(), HString("FilePath"), filePath);
			p->PushReturnDataNode(dataStore, dataStore.GetRootNode());
		}
		Vector<FilePath> v;
		v.PushBack(filePath);
		p->PushReturnAsTable(v);
	}
};
SEOUL_BEGIN_TYPE(ScriptTestInterfaceFilePath)
	SEOUL_METHOD(TestFilePath)
SEOUL_END_TYPE()
void ScriptTest::TestInterfaceFilePath()
{
	Script::VmSettings settings;
	SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));
	SEOUL_UNITTESTING_ASSERT(pVm->RunCode(
		"function TestFilePath(udFilePath)\n"
		"	local native = SeoulNativeNewNativeUserData('ScriptTestInterfaceFilePath')\n"
		"	return native:TestFilePath(udFilePath)\n"
		"end\n"));

	Script::FunctionInvoker invoker(*pVm, HString("TestFilePath"));
	SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
	invoker.PushFilePath(FilePath::CreateConfigFilePath("Test.json"));
	SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, invoker.GetReturnCount());

	{
		FilePath out;
		SEOUL_UNITTESTING_ASSERT(invoker.GetFilePath(0, out));
		SEOUL_UNITTESTING_ASSERT_EQUAL(FilePath::CreateConfigFilePath("Test.json"), out);
	}
	{
		ScriptTestFilePathStruct out;
		invoker.GetTableAsComplex(1, out);
		SEOUL_UNITTESTING_ASSERT_EQUAL(FilePath::CreateConfigFilePath("Test.json"), out.m_FilePath);
	}
	{
		Vector<FilePath> v;
		invoker.GetTableAsComplex(2, v);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(FilePath::CreateConfigFilePath("Test.json"), v[0]);
	}
}

struct ScriptTestInterfaceRaiseErrorStruct SEOUL_SEALED
{
	void TestError1(Script::FunctionInterface* p)
	{
		p->RaiseError(1);
	}

	void TestError2(Script::FunctionInterface* p)
	{
		p->RaiseError(1, "testerror2");
	}

	void TestError3(Script::FunctionInterface* p)
	{
		p->RaiseError("testerror3");
	}

	void TestError4(Script::FunctionInterface* p)
	{
		p->RaiseError(-1, "testerror4");
	}
}; // struct ScriptTestInterfaceRaiseErrorStruct
SEOUL_BEGIN_TYPE(ScriptTestInterfaceRaiseErrorStruct)
	SEOUL_METHOD(TestError1)
	SEOUL_METHOD(TestError2)
	SEOUL_METHOD(TestError3)
	SEOUL_METHOD(TestError4)
SEOUL_END_TYPE()

struct ScriptTestInterfaceRaiseErrorChecker SEOUL_SEALED
{
	SEOUL_DELEGATE_TARGET(ScriptTestInterfaceRaiseErrorChecker);

	ScriptTestInterfaceRaiseErrorChecker()
		: m_iError(0)
		, m_bError1(false)
		, m_bError2(false)
		, m_bError3(false)
		, m_bError4(false)
	{
	}

	Int m_iError;
	Bool m_bError1;
	Bool m_bError2;
	Bool m_bError3;
	Bool m_bError4;

	void OnError(const CustomCrashErrorState& state)
	{
		switch (m_iError)
		{
		case 0: m_bError1 = state.m_sReason.EndsWith("invalid argument 2"); break;
		case 1: m_bError2 = state.m_sReason.EndsWith("invalid argument 2: testerror2"); break;
		case 2: m_bError3 = state.m_sReason.EndsWith("invocation error: testerror3"); break;
		case 3: m_bError4 = state.m_sReason.EndsWith("invocation error: testerror4"); break;
		default:
			break;
		};

		++m_iError;
	}
}; // struct ScriptTestInterfaceRaiseErrorChecker

// TODO: LuaJIT includes native coroutines, and I suspect
// it is not interacting well with address sanitizer when
// errors are triggered. Disabling for the sake of allowing the
// test to run but should revisit and try to fix this proper.
static void* SEOUL_DISABLE_ADDRESS_SANITIZER CustomMemoryAllocatorHook(void*, void* ptr, size_t, size_t nsize)
{
	using namespace Seoul;

	if (0 == nsize)
	{
		MemoryManager::Deallocate(ptr);
		return nullptr;
	}
	else
	{
		return MemoryManager::Reallocate(ptr, nsize, MemoryBudgets::Scripting);
	}
}

void ScriptTest::TestInterfaceRaiseError()
{
	ScriptTestInterfaceRaiseErrorChecker checker;
	Script::VmSettings settings;
	settings.m_CustomMemoryAllocatorHook = CustomMemoryAllocatorHook;
	settings.m_ErrorHandler = SEOUL_BIND_DELEGATE(&ScriptTestInterfaceRaiseErrorChecker::OnError, &checker);
	SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));
	SEOUL_UNITTESTING_ASSERT(pVm->RunCode(
		"local native = SeoulNativeNewNativeUserData('ScriptTestInterfaceRaiseErrorStruct')\n"
		"function TestError1() return native:TestError1() end\n"
		"function TestError2() return native:TestError2() end\n"
		"function TestError3() return native:TestError3() end\n"
		"function TestError4() return native:TestError4() end\n"));

	{
		Script::FunctionInvoker invoker(*pVm, HString("TestError1"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(!invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, checker.m_bError1);
	}
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestError2"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(!invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, checker.m_bError2);
	}
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestError3"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(!invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, checker.m_bError3);
	}
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestError4"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(!invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, checker.m_bError4);
	}
}

void ScriptTest::TestInterfaceReturn()
{
	static const Int kiTestArgFailures = 100;

	Script::VmSettings settings;
	SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));
	SEOUL_UNITTESTING_ASSERT(pVm->RunCode(
		"local native = SeoulNativeNewNativeUserData('ScriptTestInterfaceReturnStruct')\n"
		"function TestArrayIndex() return native:TestArrayIndex() end\n"
		"function TestBoolean() return native:TestBoolean() end\n"
		"function TestByteBuffer() return native:TestByteBuffer() end\n"
		"function TestEnum() return native:TestEnum() end\n"
		"function TestEnum2() return native:TestEnum2() end\n"
		"function TestFilePath() return native:TestFilePath() end\n"
		"function TestFunction() return native:TestFunction() end\n"
		"function TestInteger() return native:TestInteger() end\n"
		"function TestLightUserData() return native:TestLightUserData() end\n"
		"function TestNil() return native:TestNil() end\n"
		"function TestNumber() return native:TestNumber() end\n"
		"function TestObject() return native:TestObject() end\n"
		"function TestString() return native:TestString() end\n"
		"function TestString2() return native:TestString2() end\n"
		"function TestString3() return native:TestString3() end\n"
		"function TestString4() return native:TestString4() end\n"
		"function TestStringAlsoNumber() return native:TestStringAlsoNumber() end\n"
		"function TestTable() return native:TestTable() end\n"
		"function TestUInt() return native:TestUInt() end\n"
		"function TestUserData() return native:TestUserData() end\n"));

	// Any.
	{
		Reflection::Any any;
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestArrayIndex"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<Script::ArrayIndex>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<Script::ArrayIndex>());
			SEOUL_UNITTESTING_ASSERT_EQUAL(Script::ArrayIndex(0), any.Cast<Script::ArrayIndex>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestBoolean"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<Bool>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<Bool>());
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, any.Cast<Bool>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestByteBuffer"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<String>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<String>());
			SEOUL_UNITTESTING_ASSERT_EQUAL("Hello World", any.Cast<String>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestEnum"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<MemoryBudgets::Type>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<MemoryBudgets::Type>());
			SEOUL_UNITTESTING_ASSERT_EQUAL(MemoryBudgets::Analytics, any.Cast<MemoryBudgets::Type>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestEnum2"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<MemoryBudgets::Type>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<MemoryBudgets::Type>());
			SEOUL_UNITTESTING_ASSERT_EQUAL(MemoryBudgets::Analytics, any.Cast<MemoryBudgets::Type>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestFilePath"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<FilePath>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<FilePath>());
			SEOUL_UNITTESTING_ASSERT_EQUAL(FilePath::CreateConfigFilePath("Test"), any.Cast<FilePath>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestInteger"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<Int32>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<Int32>());
			SEOUL_UNITTESTING_ASSERT_EQUAL(5, any.Cast<Int32>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestLightUserData"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<void*>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<void*>());
			SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, any.Cast<void*>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestNil"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<void>(), any));
			SEOUL_UNITTESTING_ASSERT(!any.IsValid());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestNumber"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<Double>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<Double>());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.5, any.Cast<Double>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestObject"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId< SharedPtr<Script::VmObject> >(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType< SharedPtr<Script::VmObject> >());
			SEOUL_UNITTESTING_ASSERT(any.Cast< SharedPtr<Script::VmObject> >().IsValid());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestString"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<String>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<String>());
			SEOUL_UNITTESTING_ASSERT_EQUAL("Hello World", any.Cast<String>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestString2"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<String>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<String>());
			SEOUL_UNITTESTING_ASSERT_EQUAL("Hello World", any.Cast<String>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestString3"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<String>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<String>());
			SEOUL_UNITTESTING_ASSERT_EQUAL("Hello World", any.Cast<String>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestString4"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<String>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<String>());
			SEOUL_UNITTESTING_ASSERT_EQUAL("Hello World", any.Cast<String>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestStringAlsoNumber"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<Double>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<Double>());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.75, any.Cast<Double>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestTable"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<ScriptTestStruct>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<ScriptTestStruct>());
			SEOUL_UNITTESTING_ASSERT_EQUAL("What is up?", any.Cast<ScriptTestStruct>().m_sValue);
			SEOUL_UNITTESTING_ASSERT_EQUAL(22, any.Cast<ScriptTestStruct>().m_iNumber);
			for (Int i = 1; i < kiTestArgFailures; ++i)
			{
				SEOUL_UNITTESTING_ASSERT(!invoker.GetAny(i, TypeId<void>(), any));
			}
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestUInt"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<UInt32>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<UInt32>());
			SEOUL_UNITTESTING_ASSERT_EQUAL(32, any.Cast<UInt32>());
		}
	}

	// ArrayIndex.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestArrayIndex"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		Script::ArrayIndex index;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetArrayIndex(0, index));
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberExact(0));

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetArrayIndex(i, index));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNone(i));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNilOrNone(i));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(Script::ArrayIndex(0), index);
	}

	// Boolean.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestBoolean"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		Bool b = false;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, b));
		SEOUL_UNITTESTING_ASSERT(invoker.IsBoolean(0));

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetBoolean(i, b));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNone(i));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNilOrNone(i));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, b);
	}

	// ByteBuffer.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestByteBuffer"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		String s;
		Double fUnusedNumber;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetString(0, s));
		SEOUL_UNITTESTING_ASSERT(!invoker.GetNumber(0, fUnusedNumber));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringCoercible(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringExact(0));
		SEOUL_UNITTESTING_ASSERT(!invoker.IsNumberCoercible(0)); // This string is not convertible to a number.
		SEOUL_UNITTESTING_ASSERT(!invoker.IsNumberExact(0)); // This string is not convertible to a number.

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetString(i, s));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNone(i));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNilOrNone(i));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL("Hello World", s);
	}

	// Enum1.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestEnum"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		MemoryBudgets::Type e = MemoryBudgets::Saving;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetEnum(0, e));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringCoercible(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringExact(0));
		SEOUL_UNITTESTING_ASSERT(!invoker.IsNumberCoercible(0));
		SEOUL_UNITTESTING_ASSERT(!invoker.IsNumberExact(0));

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetEnum(i, e));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNone(i));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNilOrNone(i));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(MemoryBudgets::Analytics, e);
	}

	// Enum2.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestEnum2"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		MemoryBudgets::Type e = MemoryBudgets::Saving;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetEnum(0, e));
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberCoercible(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberExact(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringCoercible(0));
		SEOUL_UNITTESTING_ASSERT(!invoker.IsStringExact(0));

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetEnum(i, e));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNone(i));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNilOrNone(i));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(MemoryBudgets::Analytics, e);
	}

	// FilePath
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestFilePath"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		FilePath filePath;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetFilePath(0, filePath));
		SEOUL_UNITTESTING_ASSERT(invoker.IsUserData(0));

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetFilePath(i, filePath));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNone(i));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNilOrNone(i));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(FilePath::CreateConfigFilePath("Test"), filePath);
	}

	// Integer.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestInteger"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		Int32 iInteger = 0;
		String s;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		// NOTE: Is* checks must come first, as the GetString() will cause
		// Lua to replace the number value on the stack with a string value.
		// As a result, after that call, the number will then be a string.
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberCoercible(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberExact(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringCoercible(0)); // In Lua, numbers are also coercible to strings.
		SEOUL_UNITTESTING_ASSERT(!invoker.IsStringExact(0)); // But our exact variation should only allow actual strings.
		SEOUL_UNITTESTING_ASSERT(invoker.GetInteger(0, iInteger));
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, iInteger);
		SEOUL_UNITTESTING_ASSERT(invoker.GetString(0, s));
		SEOUL_UNITTESTING_ASSERT_EQUAL("5", s);
		// Check Lua behavior, although it is unfortunate... after the call to GetString(), the value
		// will now be a coercible string, no longer a number.
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberCoercible(0));
		SEOUL_UNITTESTING_ASSERT(!invoker.IsNumberExact(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringCoercible(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringExact(0));

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetInteger(i, iInteger));
			SEOUL_UNITTESTING_ASSERT(!invoker.GetString(i, s));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNone(i));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNilOrNone(i));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, iInteger);
		SEOUL_UNITTESTING_ASSERT_EQUAL("5", s);
	}

	// LightUserData
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestLightUserData"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		void* p = (void*)1;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetLightUserData(0, p));
		SEOUL_UNITTESTING_ASSERT(invoker.IsLightUserData(0));

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetLightUserData(i, p));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNone(i));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNilOrNone(i));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, p);
	}

	// Nil
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestNil"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.IsNil(0));

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.IsNil(i));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNone(i));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNilOrNone(i));
		}
		SEOUL_UNITTESTING_ASSERT(invoker.IsNil(0));
	}

	// Number.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestNumber"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		Double f = 1.0;
		String s;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		// NOTE: Is* checks must come first, as the GetString() will cause
		// Lua to replace the number value on the stack with a string value.
		// As a result, after that call, the number will then be a string.
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberCoercible(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberExact(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringCoercible(0)); // In Lua, numbers are also coercible to strings.
		SEOUL_UNITTESTING_ASSERT(!invoker.IsStringExact(0)); // But our exact variation should only allow actual strings.
		SEOUL_UNITTESTING_ASSERT(invoker.GetNumber(0, f));
		SEOUL_UNITTESTING_ASSERT(invoker.GetString(0, s));
		// Check Lua behavior, although it is unfortunate... after the call to GetString(), the value
		// will now be a coercible string, no longer a number.
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberCoercible(0));
		SEOUL_UNITTESTING_ASSERT(!invoker.IsNumberExact(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringCoercible(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringExact(0));

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetNumber(i, f));
			SEOUL_UNITTESTING_ASSERT(!invoker.GetString(i, s));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNone(i));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNilOrNone(i));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.5, f);
		SEOUL_UNITTESTING_ASSERT_EQUAL("1.5", s);
	}

	// Object.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestObject"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		SharedPtr<Script::VmObject> p;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetObject(0, p));
		SEOUL_UNITTESTING_ASSERT(invoker.IsFunction(0));

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetObject(i, p));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNone(i));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNilOrNone(i));
		}
		SEOUL_UNITTESTING_ASSERT(p.IsValid());
	}

	// String.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestString"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		String s;
		Double fUnusedNumber;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetString(0, s));
		SEOUL_UNITTESTING_ASSERT(!invoker.GetNumber(0, fUnusedNumber));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringCoercible(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringExact(0));
		SEOUL_UNITTESTING_ASSERT(!invoker.IsNumberCoercible(0)); // This string is not convertible to a number.
		SEOUL_UNITTESTING_ASSERT(!invoker.IsNumberExact(0)); // This string is not convertible to a number.

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetString(i, s));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNone(i));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNilOrNone(i));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL("Hello World", s);
	}

	// String2.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestString2"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		String s;
		Double fUnusedNumber;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetString(0, s));
		SEOUL_UNITTESTING_ASSERT(!invoker.GetNumber(0, fUnusedNumber));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringCoercible(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringExact(0));
		SEOUL_UNITTESTING_ASSERT(!invoker.IsNumberCoercible(0)); // This string is not convertible to a number.
		SEOUL_UNITTESTING_ASSERT(!invoker.IsNumberExact(0)); // This string is not convertible to a number.

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetString(i, s));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNone(i));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNilOrNone(i));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL("Hello World", s);
	}

	// String3.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestString3"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		String s;
		Double fUnusedNumber;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetString(0, s));
		SEOUL_UNITTESTING_ASSERT(!invoker.GetNumber(0, fUnusedNumber));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringCoercible(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringExact(0));
		SEOUL_UNITTESTING_ASSERT(!invoker.IsNumberCoercible(0)); // This string is not convertible to a number.
		SEOUL_UNITTESTING_ASSERT(!invoker.IsNumberExact(0)); // This string is not convertible to a number.

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetString(i, s));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNone(i));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNilOrNone(i));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL("Hello World", s);
	}

	// String4.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestString4"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		String s;
		Double fUnusedNumber;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetString(0, s));
		SEOUL_UNITTESTING_ASSERT(!invoker.GetNumber(0, fUnusedNumber));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringCoercible(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringExact(0));
		SEOUL_UNITTESTING_ASSERT(!invoker.IsNumberCoercible(0)); // This string is not convertible to a number.
		SEOUL_UNITTESTING_ASSERT(!invoker.IsNumberExact(0)); // This string is not convertible to a number.

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetString(i, s));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNone(i));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNilOrNone(i));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL("Hello World", s);
	}

	// String that is convertible to a number.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestStringAlsoNumber"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		String s;
		Double f;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetString(0, s));
		SEOUL_UNITTESTING_ASSERT(invoker.GetNumber(0, f));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringCoercible(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringExact(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberCoercible(0));
		SEOUL_UNITTESTING_ASSERT(!invoker.IsNumberExact(0));

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetString(i, s));
			SEOUL_UNITTESTING_ASSERT(!invoker.GetNumber(i, f));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNone(i));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNilOrNone(i));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL("1.75", s);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.75, f);
	}

	// Table.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestTable"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		// function TestTable() return { Value='What is up?', Number=22 } end\n"));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());

		{
			SharedPtr<Script::VmObject> p;
			SEOUL_UNITTESTING_ASSERT(invoker.GetObject(0, p));
			SEOUL_UNITTESTING_ASSERT(invoker.IsTable(0));
			SEOUL_UNITTESTING_ASSERT(p.IsValid());

			DataStore dataStore;
			SEOUL_UNITTESTING_ASSERT(p->TryToDataStore(dataStore));

			DataNode node;
			String s;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("Value"), node));
			SEOUL_UNITTESTING_ASSERT(dataStore.AsString(node, s));
			SEOUL_UNITTESTING_ASSERT_EQUAL("What is up?", s);
			Int32 i;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("Number"), node));
			SEOUL_UNITTESTING_ASSERT(dataStore.AsInt32(node, i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(22, i);
		}

		{
			ScriptTestStruct scriptTest;
			invoker.GetTableAsComplex(0, scriptTest);
			SEOUL_UNITTESTING_ASSERT_EQUAL("What is up?", scriptTest.m_sValue);
			SEOUL_UNITTESTING_ASSERT_EQUAL(22, scriptTest.m_iNumber);
		}

		DataStore dataStore;
		SEOUL_UNITTESTING_ASSERT(invoker.GetTable(0, dataStore));

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetTable(i, dataStore));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNone(i));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNilOrNone(i));
		}

		{
			DataNode node;
			String s;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("Value"), node));
			SEOUL_UNITTESTING_ASSERT(dataStore.AsString(node, s));
			SEOUL_UNITTESTING_ASSERT_EQUAL("What is up?", s);
			Int32 i;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("Number"), node));
			SEOUL_UNITTESTING_ASSERT(dataStore.AsInt32(node, i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(22, i);
		}
	}

	// UInt32.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestUInt"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		UInt32 uInteger = 0;
		String s;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		// NOTE: Is* checks must come first, as the GetString() will cause
		// Lua to replace the number value on the stack with a string value.
		// As a result, after that call, the number will then be a string.
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberCoercible(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberExact(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringCoercible(0)); // In Lua, numbers are also coercible to strings.
		SEOUL_UNITTESTING_ASSERT(!invoker.IsStringExact(0)); // But our exact variation should only allow actual strings.
		SEOUL_UNITTESTING_ASSERT(invoker.GetUInt32(0, uInteger));
		SEOUL_UNITTESTING_ASSERT(invoker.GetString(0, s));
		// Check Lua behavior, although it is unfortunate... after the call to GetString(), the value
		// will now be a coercible string, no longer a number.
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberCoercible(0));
		SEOUL_UNITTESTING_ASSERT(!invoker.IsNumberExact(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringCoercible(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringExact(0));

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetUInt32(i, uInteger));
			SEOUL_UNITTESTING_ASSERT(!invoker.GetString(i, s));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNone(i));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNilOrNone(i));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(32, uInteger);
		SEOUL_UNITTESTING_ASSERT_EQUAL("32", s);
	}

	// UserData.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestUserData"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		ScriptTestStruct scriptTest;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		ScriptTestStruct* p = invoker.GetUserData<ScriptTestStruct>(0);
		SEOUL_UNITTESTING_ASSERT(invoker.IsUserData(0));

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, invoker.GetUserData<ScriptTestStruct>(i));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNone(i));
			SEOUL_UNITTESTING_ASSERT(invoker.IsNilOrNone(i));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL("What is up?", p->m_sValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(22, p->m_iNumber);
	}
}

struct ScriptTestInterfaceReturnMultiple SEOUL_SEALED
{
	void TestMultiple(Script::FunctionInterface* p)
	{
		Int32 iCount;
		if (!p->GetInteger(1, iCount))
		{
			p->RaiseError(1, "expected count.");
			return;
		}

		for (Int i = 0; i < iCount; ++i)
		{
			p->PushReturnInteger(i + 1);
		}
	}
};
SEOUL_BEGIN_TYPE(ScriptTestInterfaceReturnMultiple)
	SEOUL_METHOD(TestMultiple)
SEOUL_END_TYPE()

void ScriptTest::TestInterfaceReturnMultiple()
{
	static const Int iMaxReturns = 100;

	Script::VmSettings settings;
	SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));
	SEOUL_UNITTESTING_ASSERT(pVm->RunCode(
		"local native = SeoulNativeNewNativeUserData('ScriptTestInterfaceReturnMultiple')\n"
		"function TestMultiple(count)\n"
		"	return native:TestMultiple(count)\n"
		"end\n"));

	for (Int i = 0; i < iMaxReturns; ++i)
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestMultiple"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		invoker.PushInteger(i);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(i, invoker.GetReturnCount());

		for (Int32 j = 0; j < i; ++j)
		{
			Int32 k = -1;
			SEOUL_UNITTESTING_ASSERT(invoker.GetInteger(j, k));
			SEOUL_UNITTESTING_ASSERT_EQUAL(j + 1, k);
		}
	}
}

struct ScriptTestInterfaceUserData SEOUL_SEALED
{
	void TestUserData(Script::FunctionInterface* p)
	{
		ScriptTestPushUserData* ud = p->GetUserData<ScriptTestPushUserData>(1);
		if (!ud->TestMethod())
		{
			p->RaiseError(1, "unexpected TestMethod result.");
			return;
		}

		if (!p->PushReturnUserData(TypeOf<ScriptTestReturnUserData>()))
		{
			p->RaiseError("failed returning user data.");
			return;
		}
	}
};
SEOUL_BEGIN_TYPE(ScriptTestInterfaceUserData)
	SEOUL_METHOD(TestUserData)
SEOUL_END_TYPE()
void ScriptTest::TestInterfaceUserData()
{
	Script::VmSettings settings;
	SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));
	SEOUL_UNITTESTING_ASSERT(pVm->RunCode(
		"function TestUserData(ud)\n"
		"	local native = SeoulNativeNewNativeUserData('ScriptTestInterfaceUserData')\n"
		"	return native:TestUserData(ud)\n"
		"end\n"));

	Script::FunctionInvoker invoker(*pVm, HString("TestUserData"));
	SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
	auto p = invoker.PushUserData<ScriptTestPushUserData>();
	p->m_iCount = 0;
	SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
	Int32 const iTest = s_iCount;
	s_iCount = 0;
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, iTest);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());

	ScriptTestReturnUserData* pReturn = invoker.GetUserData<ScriptTestReturnUserData>(0);
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, pReturn);
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1, pReturn->m_iCount);
}

struct ScriptTestInterfaceUserDataType SEOUL_SEALED
{
	void TestUserDataType(Script::FunctionInterface* p)
	{
		p->PushReturnUserDataType(TypeOf<ScriptTestPushUserData>());
	}
};
SEOUL_BEGIN_TYPE(ScriptTestInterfaceUserDataType)
	SEOUL_METHOD(TestUserDataType)
SEOUL_END_TYPE()
void ScriptTest::TestInterfaceUserDataType()
{
	Script::VmSettings settings;
	SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));
	SEOUL_UNITTESTING_ASSERT(pVm->RunCode(
		"function TestUserDataType()\n"
		"	local native = SeoulNativeNewNativeUserData('ScriptTestInterfaceUserDataType')\n"
		"	local tDescription = native:TestUserDataType();\n"
		"	if type(tDescription.TestMethod) ~= 'function' then\n"
		"		error 'Test Failed'\n"
		"	end\n"
		"end\n"));

	Script::FunctionInvoker invoker(*pVm, HString("TestUserDataType"));
	SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
	SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
}

void ScriptTest::TestInvokeArgs()
{
	Script::VmSettings settings;
	SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));
	SEOUL_UNITTESTING_ASSERT(pVm->RunCode(
		"function TestArrayIndex(i) return (i == 1) end\n"
		"function TestBoolean(b) return (b == true) end\n"
		"function TestByteBuffer(s) return ('Hello World' == s) end\n"
		"function TestEnum(i) return (i == 0) end\n" // 0 is MemoryBudgets::Analytics
		"function TestFilePath(filePath) return (filePath:GetDirectory() == 1 and filePath:GetType() == 0 and filePath:GetRelativeFilenameWithoutExtension():lower() == 'test') end\n" // 1 is Directory::kConfig, 0 is FileType::kUnknown
		"function TestInteger(i) return (i == 5) end\n"
		"function TestLightUserData(l) return (type(l) == 'userdata') end\n"
		"function TestNil(n) return type(n) == 'nil' end\n"
		"function TestNumber(f) return (f == 1.5) end\n"
		"function TestString(s) return (s == 'Hello World') end\n"
		"function TestStringAlsoNumber(s) return (tonumber(s) == 1.75) end\n"
		"function TestTable(t) return (t.Value == 'What is up?' and t.Number == 22) end\n"
		"function TestUInt(u) return (u == 32) end\n"
		"function TestUserData(ud) return (type(ud) == 'userdata' and ud:TestMethod()) end\n"
		"function TestUserDataType(t) return (type(t.TestMethod) == 'function') end\n"));

	// Any.
	{
		Reflection::Any any;
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestArrayIndex"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(Script::ArrayIndex(0));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestBoolean"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(true);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			// Intentional extra to make sure size is respected
			Script::ByteBuffer buffer;
			buffer.m_pData = (void*)"Hello Worldasdf";
			buffer.m_zDataSizeInBytes = 11;

			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestByteBuffer"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(buffer);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestEnum"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(MemoryBudgets::Analytics);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestFilePath"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(FilePath::CreateConfigFilePath("Test"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestInteger"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(5);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestLightUserData"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny((void*)(nullptr));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestNil"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(Reflection::Any());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestNumber"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(1.5);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestString"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny((Byte const*)"Hello World");
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestString"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(String("Hello World"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestString"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(HString("Hello World"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestStringAlsoNumber"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny((Byte const*)"1.75");
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestStringAlsoNumber"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(String("1.75"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestStringAlsoNumber"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(HString("1.75"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestTable"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(ScriptTestStruct("What is up?", 22));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestUInt"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(32u);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
	}

	// ArrayIndex
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestArrayIndex"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushArrayIndex(Script::ArrayIndex(0));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// Boolean
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestBoolean"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushBoolean(true);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// ByteBuffer
	{
		// Intentional extra to make sure size is respected
		Script::ByteBuffer buffer;
		buffer.m_pData = (void*)"Hello Worldasdf";
		buffer.m_zDataSizeInBytes = 11;

		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestByteBuffer"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushByteBuffer(buffer);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// Enum
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestEnum"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushEnumAsNumber(MemoryBudgets::Analytics);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// FilePath
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestFilePath"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushFilePath(FilePath::CreateConfigFilePath("Test"));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// Integer
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestInteger"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushInteger(5);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// Light user data.
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestLightUserData"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushLightUserData(nullptr);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// Nil
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestNil"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushNil();
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// Number
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestNumber"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushNumber(1.5);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// String
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestString"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushString((Byte const*)"Hello World");
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// String
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestString"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushString((Byte const*)"Hello World", 11);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// String
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestString"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushString(String("Hello World"));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// String
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestString"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushString(HString("Hello World"));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// StringAsNumber
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestStringAlsoNumber"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushString((Byte const*)"1.75");
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// StringAsNumber
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestStringAlsoNumber"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushString((Byte const*)"1.75", 4);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// StringAsNumber
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestStringAlsoNumber"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushString(String("1.75"));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// StringAsNumber
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestStringAlsoNumber"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushString(HString("1.75"));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// Table
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestTable"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushAsTable(ScriptTestStruct("What is up?", 22));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// Table
	{
		Bool bResult = false;
		DataStore dataStore;
		dataStore.MakeTable();
		dataStore.SetStringToTable(dataStore.GetRootNode(), HString("Value"), "What is up?");
		dataStore.SetInt32ValueToTable(dataStore.GetRootNode(), HString("Number"), 22);

		Script::FunctionInvoker invoker(*pVm, HString("TestTable"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.PushDataNode(dataStore, dataStore.GetRootNode()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// UInt
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestUInt"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushUInt32(32u);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// UserData
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestUserData"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushUserData<ScriptTestPushUserData>()->m_iCount = 0;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, s_iCount); s_iCount = 0;
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// UserDataType
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestUserDataType"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.PushUserDataType(TypeOf<ScriptTestPushUserData>()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, s_iCount);
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}
}

void ScriptTest::TestInvokeArgsMultiple()
{
	static const Int iMaxArgs = 100;

	Script::VmSettings settings;
	SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));
	SEOUL_UNITTESTING_ASSERT(pVm->RunCode(
		"function TestMultiple(...)\n"
		"	local t = {...}\n"
		"	for i,v in ipairs(t) do\n"
		"		if i ~= v then\n"
		"			error('invalid, ' .. i .. ' ~= ' .. v)\n"
		"		end\n"
		"	end\n"
		"end\n"));

	for (Int i = 0; i < iMaxArgs; ++i)
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestMultiple"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		for (Int32 j = 0; j < i; ++j)
		{
			invoker.PushInteger(j + 1);
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(i, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetReturnCount());
	}
}

// Regression for a bug, FilePath was not being handled correctly when part of a table that
// was retrieved from a Lua table into a complex native type.
void ScriptTest::TestInvokeFilePath()
{
	Script::VmSettings settings;
	SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));
	SEOUL_UNITTESTING_ASSERT(pVm->RunCode(
		"function TestFilePath(udFilePath)\n"
		"	return udFilePath, {\n"
		"		FilePath = udFilePath\n"
		"	}, {\n"
		"		[1] = udFilePath\n"
		"	}\n"
		"end\n"));

	Script::FunctionInvoker invoker(*pVm, HString("TestFilePath"));
	SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
	invoker.PushFilePath(FilePath::CreateConfigFilePath("Test.json"));
	SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, invoker.GetReturnCount());

	{
		FilePath out;
		SEOUL_UNITTESTING_ASSERT(invoker.GetFilePath(0, out));
		SEOUL_UNITTESTING_ASSERT_EQUAL(FilePath::CreateConfigFilePath("Test.json"), out);
	}
	{
		ScriptTestFilePathStruct out;
		invoker.GetTableAsComplex(1, out);
		SEOUL_UNITTESTING_ASSERT_EQUAL(FilePath::CreateConfigFilePath("Test.json"), out.m_FilePath);
	}
	{
		Vector<FilePath> v;
		invoker.GetTableAsComplex(2, v);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, v.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(FilePath::CreateConfigFilePath("Test.json"), v[0]);
	}
}

void ScriptTest::TestInvokeReturn()
{
	static const Int kiTestArgFailures = 100;

	Script::VmSettings settings;
	SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));
	SEOUL_UNITTESTING_ASSERT(pVm->RunCode(
		"function TestArrayIndex() return 1 end\n"
		"function TestArrayIndexInvalid() return 0 end\n"
		"function TestBoolean() return true end\n"
		"function TestEnum() return 'Analytics' end\n"
		"function TestEnum2() return 0 end\n"
		"function TestFloat32() return 1.25 end\n"
		"function TestFunction() return (function() print 'H'; return 'Hi There' end) end\n"
		"function TestInteger() return 5 end\n"
		"function TestNil() return nil end\n"
		"function TestNumber() return 1.5 end\n"
		"function TestString() return 'Hello World' end\n"
		"function TestStringAlsoNumber() return 1.75 end\n"
		"function TestTable() return { Value='What is up?', Number=22 } end\n"
		"function TestUInt() return 32 end\n"));

	// Any.
	{
		Reflection::Any any;
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestArrayIndex"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<Script::ArrayIndex>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<Script::ArrayIndex>());
			SEOUL_UNITTESTING_ASSERT_EQUAL(Script::ArrayIndex(0), any.Cast<Script::ArrayIndex>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestArrayIndexInvalid"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<Script::ArrayIndex>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<Script::ArrayIndex>());
			SEOUL_UNITTESTING_ASSERT_EQUAL(Script::ArrayIndex(UIntMax), any.Cast<Script::ArrayIndex>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestBoolean"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<Bool>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<Bool>());
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, any.Cast<Bool>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestEnum"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<MemoryBudgets::Type>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<MemoryBudgets::Type>());
			SEOUL_UNITTESTING_ASSERT_EQUAL(MemoryBudgets::Analytics, any.Cast<MemoryBudgets::Type>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestEnum2"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<MemoryBudgets::Type>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<MemoryBudgets::Type>());
			SEOUL_UNITTESTING_ASSERT_EQUAL(MemoryBudgets::Analytics, any.Cast<MemoryBudgets::Type>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestFloat32"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<Float32>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<Float32>());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.25f, any.Cast<Float32>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestFunction"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId< SharedPtr<Script::VmObject> >(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType< SharedPtr<Script::VmObject> >());
			SEOUL_UNITTESTING_ASSERT(any.Cast< SharedPtr<Script::VmObject> >().IsValid());
			{
				Script::FunctionInvoker inner(any.Cast< SharedPtr<Script::VmObject> >());
				SEOUL_UNITTESTING_ASSERT(inner.IsValid());
				SEOUL_UNITTESTING_ASSERT(inner.TryInvoke());

				{
					Reflection::Any innerAny;
					inner.GetAny(0, TypeId<void>(), innerAny);
					auto const& type = innerAny.GetType();
					SEOUL_LOG("%s", type.GetName().CStr());
				}

				String s;
				SEOUL_UNITTESTING_ASSERT(inner.GetString(0, s));
				SEOUL_UNITTESTING_ASSERT_EQUAL("Hi There", s);
			}
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestInteger"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<Int32>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<Int32>());
			SEOUL_UNITTESTING_ASSERT_EQUAL(5, any.Cast<Int32>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestNil"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<void>(), any));
			SEOUL_UNITTESTING_ASSERT(!any.IsValid());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestNumber"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<Double>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<Double>());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.5, any.Cast<Double>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestString"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<String>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<String>());
			SEOUL_UNITTESTING_ASSERT_EQUAL("Hello World", any.Cast<String>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestStringAlsoNumber"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<Double>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<Double>());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.75, any.Cast<Double>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestTable"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<ScriptTestStruct>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<ScriptTestStruct>());
			SEOUL_UNITTESTING_ASSERT_EQUAL("What is up?", any.Cast<ScriptTestStruct>().m_sValue);
			SEOUL_UNITTESTING_ASSERT_EQUAL(22, any.Cast<ScriptTestStruct>().m_iNumber);
			for (Int i = 1; i < kiTestArgFailures; ++i)
			{
				SEOUL_UNITTESTING_ASSERT(!invoker.GetAny(i, TypeId<void>(), any));
			}
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestUInt"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<UInt32>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<UInt32>());
			SEOUL_UNITTESTING_ASSERT_EQUAL(32, any.Cast<UInt32>());
		}
	}

	// ArrayIndex.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestArrayIndex"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		Script::ArrayIndex index;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetArrayIndex(0, index));
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberExact(0));

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetArrayIndex(i, index));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(Script::ArrayIndex(0), index);
	}

	// ArrayIndex (invalid - lua returns 0).
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestArrayIndexInvalid"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		Script::ArrayIndex index;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetArrayIndex(0, index));
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberExact(0));

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetArrayIndex(i, index));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(Script::ArrayIndex(UIntMax), index);
	}

	// Boolean.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestBoolean"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		Bool b = false;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, b));
		SEOUL_UNITTESTING_ASSERT(invoker.IsBoolean(0));

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetBoolean(i, b));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, b);
	}

	// Enum1.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestEnum"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		MemoryBudgets::Type e = MemoryBudgets::Saving;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetEnum(0, e));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringCoercible(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringExact(0));
		SEOUL_UNITTESTING_ASSERT(!invoker.IsNumberCoercible(0));
		SEOUL_UNITTESTING_ASSERT(!invoker.IsNumberExact(0));

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetEnum(i, e));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(MemoryBudgets::Analytics, e);
	}

	// Enum2.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestEnum2"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		MemoryBudgets::Type e = MemoryBudgets::Saving;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetEnum(0, e));
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberCoercible(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberExact(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringCoercible(0));
		SEOUL_UNITTESTING_ASSERT(!invoker.IsStringExact(0));

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetEnum(i, e));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(MemoryBudgets::Analytics, e);
	}

	// Float32.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestFloat32"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		Float32 f = 1.0f;
		String s;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		// NOTE: Is* checks must come first, as the GetString() will cause
		// Lua to replace the number value on the stack with a string value.
		// As a result, after that call, the number will then be a string.
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberCoercible(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberExact(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringCoercible(0)); // In Lua, numbers are also coercible to strings.
		SEOUL_UNITTESTING_ASSERT(!invoker.IsStringExact(0)); // But our exact variation should only allow actual strings.
		SEOUL_UNITTESTING_ASSERT(invoker.GetNumber(0, f));
		SEOUL_UNITTESTING_ASSERT(invoker.GetString(0, s));
		// Check Lua behavior, although it is unfortunate... after the call to GetString(), the value
		// will now be a coercible string, no longer a number.
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberCoercible(0));
		SEOUL_UNITTESTING_ASSERT(!invoker.IsNumberExact(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringCoercible(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringExact(0));

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetNumber(i, f));
			SEOUL_UNITTESTING_ASSERT(!invoker.GetString(i, s));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.25f, f);
		SEOUL_UNITTESTING_ASSERT_EQUAL("1.25", s);
	}

	// Function.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestFunction"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		SharedPtr< Script::VmObject > p;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetFunction(0, p));
		SEOUL_UNITTESTING_ASSERT(invoker.IsFunction(0));

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetFunction(i, p));
		}

		{
			Script::FunctionInvoker inner(p);
			SEOUL_UNITTESTING_ASSERT(inner.IsValid());
			SEOUL_UNITTESTING_ASSERT(inner.TryInvoke());

			String s;
			SEOUL_UNITTESTING_ASSERT(inner.GetString(0, s));
			SEOUL_UNITTESTING_ASSERT_EQUAL("Hi There", s);
		}
	}

	// Integer.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestInteger"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		Int32 iInteger = 0;
		String s;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		// NOTE: Is* checks must come first, as the GetString() will cause
		// Lua to replace the number value on the stack with a string value.
		// As a result, after that call, the number will then be a string.
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberCoercible(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberExact(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringCoercible(0)); // In Lua, numbers are also coercible to strings.
		SEOUL_UNITTESTING_ASSERT(!invoker.IsStringExact(0)); // But our exact variation should only allow actual strings.
		SEOUL_UNITTESTING_ASSERT(invoker.GetInteger(0, iInteger));
		SEOUL_UNITTESTING_ASSERT(invoker.GetString(0, s));
		// Check Lua behavior, although it is unfortunate... after the call to GetString(), the value
		// will now be a coercible string, no longer a number.
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberCoercible(0));
		SEOUL_UNITTESTING_ASSERT(!invoker.IsNumberExact(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringCoercible(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringExact(0));

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetInteger(i, iInteger));
			SEOUL_UNITTESTING_ASSERT(!invoker.GetString(i, s));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, iInteger);
		SEOUL_UNITTESTING_ASSERT_EQUAL("5", s);
	}

	// Nil.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestNil"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.IsNil(0));
	}

	// Number.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestNumber"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		Double f = 1.0;
		String s;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		// NOTE: Is* checks must come first, as the GetString() will cause
		// Lua to replace the number value on the stack with a string value.
		// As a result, after that call, the number will then be a string.
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberCoercible(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberExact(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringCoercible(0)); // In Lua, numbers are also coercible to strings.
		SEOUL_UNITTESTING_ASSERT(!invoker.IsStringExact(0)); // But our exact variation should only allow actual strings.
		SEOUL_UNITTESTING_ASSERT(invoker.GetNumber(0, f));
		SEOUL_UNITTESTING_ASSERT(invoker.GetString(0, s));
		// Check Lua behavior, although it is unfortunate... after the call to GetString(), the value
		// will now be a coercible string, no longer a number.
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberCoercible(0));
		SEOUL_UNITTESTING_ASSERT(!invoker.IsNumberExact(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringCoercible(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringExact(0));

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetNumber(i, f));
			SEOUL_UNITTESTING_ASSERT(!invoker.GetString(i, s));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.5, f);
		SEOUL_UNITTESTING_ASSERT_EQUAL("1.5", s);
	}

	// String.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestString"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		HString h;
		String s;
		Double fUnusedNumber;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetString(0, s));
		SEOUL_UNITTESTING_ASSERT(invoker.GetString(0, h));
		SEOUL_UNITTESTING_ASSERT(!invoker.GetNumber(0, fUnusedNumber));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringCoercible(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringExact(0));
		SEOUL_UNITTESTING_ASSERT(!invoker.IsNumberCoercible(0)); // This string is not convertible to a number.
		SEOUL_UNITTESTING_ASSERT(!invoker.IsNumberExact(0)); // This string is not convertible to a number.

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetString(i, s));
			SEOUL_UNITTESTING_ASSERT(!invoker.GetString(i, h));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL("Hello World", s);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HString("Hello World"), h);
	}

	// String that is convertible to a number.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestStringAlsoNumber"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		String s;
		Double f;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetString(0, s));
		SEOUL_UNITTESTING_ASSERT(invoker.GetNumber(0, f));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringCoercible(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringExact(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberCoercible(0)); // A string coercible to a number.
		SEOUL_UNITTESTING_ASSERT(!invoker.IsNumberExact(0)); // But our exact variation should only allow an actual number.

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetString(i, s));
			SEOUL_UNITTESTING_ASSERT(!invoker.GetNumber(i, f));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL("1.75", s);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.75, f);
	}

	// Table.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestTable"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		// function TestTable() return { Value='What is up?', Number=22 } end\n"));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());

		{
			SharedPtr<Script::VmObject> p;
			SEOUL_UNITTESTING_ASSERT(invoker.GetObject(0, p));
			SEOUL_UNITTESTING_ASSERT(invoker.IsTable(0));
			SEOUL_UNITTESTING_ASSERT(p.IsValid());

			DataStore dataStore;
			SEOUL_UNITTESTING_ASSERT(p->TryToDataStore(dataStore));

			DataNode node;
			String s;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("Value"), node));
			SEOUL_UNITTESTING_ASSERT(dataStore.AsString(node, s));
			SEOUL_UNITTESTING_ASSERT_EQUAL("What is up?", s);
			Int32 i;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("Number"), node));
			SEOUL_UNITTESTING_ASSERT(dataStore.AsInt32(node, i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(22, i);
		}

		{
			ScriptTestStruct scriptTest;
			invoker.GetTableAsComplex(0, scriptTest);
			SEOUL_UNITTESTING_ASSERT_EQUAL("What is up?", scriptTest.m_sValue);
			SEOUL_UNITTESTING_ASSERT_EQUAL(22, scriptTest.m_iNumber);
		}

		DataStore dataStore;
		SEOUL_UNITTESTING_ASSERT(invoker.GetTable(0, dataStore));

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetTable(i, dataStore));
		}

		{
			DataNode node;
			String s;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("Value"), node));
			SEOUL_UNITTESTING_ASSERT(dataStore.AsString(node, s));
			SEOUL_UNITTESTING_ASSERT_EQUAL("What is up?", s);
			Int32 i;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("Number"), node));
			SEOUL_UNITTESTING_ASSERT(dataStore.AsInt32(node, i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(22, i);
		}
	}

	// UInt32.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestUInt"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		UInt32 uInteger = 0;
		String s;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		// NOTE: Is* checks must come first, as the GetString() will cause
		// Lua to replace the number value on the stack with a string value.
		// As a result, after that call, the number will then be a string.
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberCoercible(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberExact(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringCoercible(0)); // In Lua, numbers are also coercible to strings.
		SEOUL_UNITTESTING_ASSERT(!invoker.IsStringExact(0)); // But our exact variation should only allow actual strings.
		SEOUL_UNITTESTING_ASSERT(invoker.GetUInt32(0, uInteger));
		SEOUL_UNITTESTING_ASSERT(invoker.GetString(0, s));
		// Check Lua behavior, although it is unfortunate... after the call to GetString(), the value
		// will now be a coercible string, no longer a number.
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberCoercible(0));
		SEOUL_UNITTESTING_ASSERT(!invoker.IsNumberExact(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringCoercible(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringExact(0));

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetUInt32(i, uInteger));
			SEOUL_UNITTESTING_ASSERT(!invoker.GetString(i, s));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(32, uInteger);
		SEOUL_UNITTESTING_ASSERT_EQUAL("32", s);
	}
}

void ScriptTest::TestInvokeReturnMultiple()
{
	static const Int iMaxReturns = 100;

	Script::VmSettings settings;
	SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));
	SEOUL_UNITTESTING_ASSERT(pVm->RunCode(
		"function TestMultiple(count)\n"
		"	local ret = {}\n"
		"   for i=1,count do\n"
		"		ret[i] = i\n"
		"	end\n"
		"	return table.unpack(ret)\n"
		"end\n"));

	for (Int i = 0; i < iMaxReturns; ++i)
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestMultiple"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		invoker.PushInteger(i);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(i, invoker.GetReturnCount());

		for (Int32 j = 0; j < i; ++j)
		{
			Int32 k = -1;
			SEOUL_UNITTESTING_ASSERT(invoker.GetInteger(j, k));
			SEOUL_UNITTESTING_ASSERT_EQUAL(j + 1, k);
		}
	}
}

void ScriptTest::TestInvokeUserData()
{
	Script::VmSettings settings;
	SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));
	SEOUL_UNITTESTING_ASSERT(pVm->RunCode(
		"function TestUserData(ud)\n"
		"	ud:TestMethod()\n"
		"	return SeoulNativeNewNativeUserData('ScriptTestReturnUserData')\n"
		"end\n"));

	Script::FunctionInvoker invoker(*pVm, HString("TestUserData"));
	SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
	auto p = invoker.PushUserData<ScriptTestPushUserData>();
	p->m_iCount = 0;
	SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
	Int32 const iTest = s_iCount;
	s_iCount = 0;
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, iTest);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());

	ScriptTestReturnUserData* pReturn = invoker.GetUserData<ScriptTestReturnUserData>(0);
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, pReturn);
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1, pReturn->m_iCount);
}

void ScriptTest::TestInvokeUserDataType()
{
	Script::VmSettings settings;
	SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));
	SEOUL_UNITTESTING_ASSERT(pVm->RunCode(
		"function TestUserDataType()\n"
		"	local tDescription = SeoulDescribeNativeUserData('ScriptTestPushUserData')\n"
		"	if type(tDescription.TestMethod) ~= 'function' then\n"
		"		error 'Test Failed'\n"
		"	end\n"
		"end\n"));

	Script::FunctionInvoker invoker(*pVm, HString("TestUserDataType"));
	SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
	SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
}

struct ScriptTestMultiVmClone
{
public:
	ScriptTestMultiVmClone()
		: m_pTarget()
	{
	}

	void Test(Script::FunctionInterface* pInterface)
	{
		// First, convert all arguments into Any. This both
		// tests the Any -> Script::VmObject case, as well
		// as prepares the need for a clone.
		Reflection::MethodArguments aArguments;
		SEOUL_UNITTESTING_ASSERT(pInterface->GetArgumentCount() - 1 <= (Int32)aArguments.GetSize());
		for (Int32 i = 1; i < pInterface->GetArgumentCount(); ++i)
		{
			SEOUL_UNITTESTING_ASSERT(pInterface->GetAny(i, TypeId<void>(), aArguments[i - 1]));
		}

		// Now invoke the test method in the target Vm.
		Script::FunctionInvoker invoker(*m_pTarget, HString("Test"));
		for (Int32 i = 0; i < pInterface->GetArgumentCount() - 1; ++i)
		{
			// This line will actually traverse the Clone path.
			invoker.PushAny(aArguments[i]);
		}

		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
	}

	SharedPtr<Script::Vm> m_pTarget;
}; // struct ScriptTestMultiVmClone

SEOUL_BEGIN_TYPE(ScriptTestMultiVmClone)
	SEOUL_METHOD(Test)
SEOUL_END_TYPE()

void TestLog(Byte const* s)
{
	SEOUL_LOG("%s", s);
}

void TestError(const CustomCrashErrorState& state)
{
	SEOUL_LOG("%s", state.m_sReason.CStr());
}

// Specialized test for the particular case
// of passing a Script::VmObject from one Vm to a different
// Vm. When this happens, the object cannot be just passed,
// it must be cloned (a new object in the target VM
// is created that is a copy of the source object).
void ScriptTest::TestMultiVmClone()
{
	Script::VmSettings settings;
	settings.m_StandardOutput = SEOUL_BIND_DELEGATE(TestLog);
	settings.m_ErrorHandler = SEOUL_BIND_DELEGATE(TestError);
	SharedPtr<Script::Vm> pVmFrom(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));
	SharedPtr<Script::Vm> pVmTo(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));

	// Populate from - this will call a method on the user data, which
	// will subsequently call into a function in to, which will
	// then check that the data was passed successfully.
	SEOUL_UNITTESTING_ASSERT(pVmFrom->RunCode(
		"function Test(ud, lud)\n"
		"  if type(lud) ~= 'userdata' then error('Expected light user data.') end\n"
		"  local t = {\n"
		"    [5.0] = 'Five',\n"
		"    [true] = 25.0,\n"
		"    [lud] = 'Light',\n"
		"    n = 1.0,\n"
		"    s = 'Hello World',\n"
		"    b = true,\n"
		"    ud = ud,\n"
		"    lud = lud,\n"
		"    f = function() end,\n"
		"    t = {\n"
		"       n = 2.0,\n"
		"       s = 'Goodbye World',\n"
		"       [99] = false,\n"
		"       b = false,\n"
		"       ud = ud,\n"
		"       lud = lud,\n"
		"       f = function() end,\n"
		"       t = {\n"
		"         n = 3.0\n"
		"       },\n"
		"     },\n"
		"  }\n"
		"  ud:Test(t, lud)\n"
		"end\n"));

	SEOUL_UNITTESTING_ASSERT(pVmTo->RunCode(
		"function assert(v, expect) if v ~= expect then error('Expected ' .. tostring(expect) .. ' got ' .. tostring(v), 1) end end\n"
		"function assert_type(v, expect) if type(v) ~= expect then error('Expected type ' .. tostring(expect) .. ' got ' .. tostring(type(v)), 1) end end\n"
		"function Test(t, lud, nothing)\n"
		"  if nothing then error('Expected nil got ' .. type(nothing)) end\n"
		"  assert(t[5.0], 'Five')\n"
		"  assert(t[true], 25)\n"
		"  assert(t[lud], 'Light')\n"
		"  assert(t.n, 1.0)\n"
		"  assert(t.s, 'Hello World')\n"
		"  assert(t.b, true)\n"
		"  assert(t.ud, nil)\n"
		"  assert(t.lud, lud)\n"
		"  assert_type(t.lud, 'userdata')\n"
		"  assert(t.f, nil)\n"
		"  assert(t.t.n, 2.0)\n"
		"  assert(t.t.s, 'Goodbye World')\n"
		"  assert(t.t[99], false)\n"
		"  assert(t.t.b, false)\n"
		"  assert(t.t.ud, nil)\n"
		"  assert(t.t.lud, lud)\n"
		"  assert_type(t.t.lud, 'userdata')\n"
		"  assert(t.t.f, nil)\n"
		"  assert(t.t.t.n, 3.0)\n"
		"end\n"));

	// Instantiate a userdata in from.
	SharedPtr<Script::VmObject> pBinding;
	ScriptTestMultiVmClone* pInstance = nullptr;
	SEOUL_UNITTESTING_ASSERT(pVmFrom->BindStrongInstance<ScriptTestMultiVmClone>(pBinding, pInstance));

	// Set the target VM to the instance.
	pInstance->m_pTarget = pVmTo;

	// Now invoke the method in from, which should call to.
	Script::FunctionInvoker invoker(*pVmFrom, HString("Test"));
	invoker.PushObject(pBinding);
	invoker.PushLightUserData((void*)((size_t)25));
	SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
}

struct ScriptTestNullObjectStruct SEOUL_SEALED
{
	SharedPtr<Script::VmObject> Test2(const SharedPtr<Script::VmObject>& p)
	{
		SEOUL_UNITTESTING_ASSERT(!p.IsValid());
		return p;
	}
	void Test3(Script::FunctionInterface* p)
	{
		SharedPtr<Script::VmObject> po;
		SEOUL_UNITTESTING_ASSERT(p->GetObject(1, po));
		SEOUL_UNITTESTING_ASSERT(!po.IsValid());
		p->PushReturnObject(po);
	}
}; // struct ScriptTestNullObjectStruct
SEOUL_BEGIN_TYPE(ScriptTestNullObjectStruct)
	SEOUL_METHOD(Test2)
	SEOUL_METHOD(Test3)
SEOUL_END_TYPE()

void ScriptTest::TestNullObject()
{
	Script::VmSettings settings;
	SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));
	SEOUL_UNITTESTING_ASSERT(pVm->RunCode(
		"local native = SeoulNativeNewNativeUserData('ScriptTestNullObjectStruct')\n"
		"function Test(o) return o end\n"
		"function Test2(o) return native:Test2(o) end\n"
		"function Test3(o) return native:Test3(o) end\n"));

	// Any
	{
		Reflection::Any any;
		SharedPtr<Script::VmObject> pVmObject;
		Script::FunctionInvoker invoker(*pVm, HString("Test"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		invoker.PushAny(pVmObject);
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId< SharedPtr<Script::VmObject> >(), any));
		SEOUL_UNITTESTING_ASSERT(any.IsOfType< SharedPtr<Script::VmObject> >());
		SEOUL_UNITTESTING_ASSERT(!any.Cast< SharedPtr<Script::VmObject> >().IsValid());
	}

	// Object
	{
		SharedPtr<Script::VmObject> pVmObject;
		Script::FunctionInvoker invoker(*pVm, HString("Test"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		invoker.PushObject(pVmObject);
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		SharedPtr<Script::VmObject> pVmObject2;
		SEOUL_UNITTESTING_ASSERT(invoker.GetObject(0, pVmObject2));
		SEOUL_UNITTESTING_ASSERT(!pVmObject2.IsValid());
	}

	// Native
	{
		SharedPtr<Script::VmObject> pVmObject;
		Script::FunctionInvoker invoker(*pVm, HString("Test2"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		invoker.PushObject(pVmObject);
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		SharedPtr<Script::VmObject> pVmObject2;
		SEOUL_UNITTESTING_ASSERT(invoker.GetObject(0, pVmObject2));
		SEOUL_UNITTESTING_ASSERT(!pVmObject2.IsValid());
	}

	// Native2
	{
		SharedPtr<Script::VmObject> pVmObject;
		Script::FunctionInvoker invoker(*pVm, HString("Test3"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		invoker.PushObject(pVmObject);
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		SharedPtr<Script::VmObject> pVmObject2;
		SEOUL_UNITTESTING_ASSERT(invoker.GetObject(0, pVmObject2));
		SEOUL_UNITTESTING_ASSERT(!pVmObject2.IsValid());
	}
}

void ScriptTest::TestNullScriptVmObject()
{
	SharedPtr<Script::VmObject> p;
	{
		Script::FunctionInvoker invoker(p);
		SEOUL_UNITTESTING_ASSERT(!invoker.IsValid());
	}
	{
		Script::FunctionInvoker invoker(p, HString("Test"));
		SEOUL_UNITTESTING_ASSERT(!invoker.IsValid());
	}
}

void ScriptTest::TestNumberRanges()
{
	Script::VmSettings settings;
	SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));
	SEOUL_UNITTESTING_ASSERT(pVm->RunCode(
		"function TestMaxInt32(i) if i ~= 2147483647 then error('invalid') else return 2147483647 end end\n"
		"function TestMinInt32(i) if i ~= -2147483648 then error('invalid') else return -2147483648 end end\n"
		"function TestMaxUInt32(u) if u ~= 4294967295 then error('invalid') else return 4294967295 end end\n"
		"function TestMaxInteger(u) if u ~= 9007199254740992 then error('invalid') else return 9007199254740992 end end\n"));

	// Any
	{
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestMaxInt32"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			invoker.PushAny(IntMax);
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			Reflection::Any any;
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<Int32>(), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(IntMax, any.Cast<Int32>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestMinInt32"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			invoker.PushAny(IntMin);
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			Reflection::Any any;
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<Int32>(), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(IntMin, any.Cast<Int32>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestMaxUInt32"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			invoker.PushAny(UIntMax);
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			Reflection::Any any;
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<UInt32>(), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL(UIntMax, any.Cast<UInt32>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestMaxInteger"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			invoker.PushAny((Double)FlInt64Max);
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			Reflection::Any any;
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<Double>(), any));
			SEOUL_UNITTESTING_ASSERT_EQUAL((Double)FlInt64Max, any.Cast<Double>());
		}
	}

	// Max Int32.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestMaxInt32"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		invoker.PushInteger(IntMax);
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		Int32 i;
		SEOUL_UNITTESTING_ASSERT(invoker.GetInteger(0, i));
		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMax, i);
	}

	// Min Int32.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestMinInt32"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		invoker.PushInteger(IntMin);
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		Int32 i;
		SEOUL_UNITTESTING_ASSERT(invoker.GetInteger(0, i));
		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMin, i);
	}

	// Max UInt32.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestMaxUInt32"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		invoker.PushUInt32(UIntMax);
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		UInt32 u;
		SEOUL_UNITTESTING_ASSERT(invoker.GetUInt32(0, u));
		SEOUL_UNITTESTING_ASSERT_EQUAL(UIntMax, u);
	}

	// Max integer - this is the maximum integer that can be represented
	// continguous with a double (there are no integer holes up to and including
	// this value).
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestMaxInteger"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		invoker.PushNumber((Double)FlInt64Max);
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		Double f;
		SEOUL_UNITTESTING_ASSERT(invoker.GetNumber(0, f));
		SEOUL_UNITTESTING_ASSERT_EQUAL((Double)FlInt64Max, f);
	}
}

void ScriptTest::TestReflectionArgs()
{
	Script::VmSettings settings;
	SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));
	SEOUL_UNITTESTING_ASSERT(pVm->RunCode(
		"local native = SeoulNativeNewNativeUserData('ScriptTestReflectionArgsStruct')\n"
		"function TestArrayIndex(i) return native:TestArrayIndex(i) end\n"
		"function TestBoolean(b) return native:TestBoolean(b) end\n"
		"function TestEnum(i) return native:TestEnum(i) end\n" // 0 is MemoryBudgets::Analytics
		"function TestFilePath(filePath) return native:TestFilePath(filePath) end\n"
		"function TestInteger(i) return native:TestInteger(i) end\n"
		"function TestLightUserData(l) return native:TestLightUserData(l) end\n"
		"function TestNil(n) return type(n) == 'nil' and native:TestNil() end\n"
		"function TestNumber(f) return native:TestNumber(f) end\n"
		"function TestString(s) return native:TestString(s) end\n"
		"function TestStringAlsoNumber(s) return native:TestStringAlsoNumber(s) end\n"
		"function TestTable(t) return native:TestTable(t) end\n"
		"function TestUInt(u) return native:TestUInt(u) end\n"
		"function TestUserData(ud) return native:TestUserData(ud) end\n"));

	// Any.
	{
		Reflection::Any any;
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestArrayIndex"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(Script::ArrayIndex(0));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestBoolean"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(true);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestEnum"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(MemoryBudgets::Analytics);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestFilePath"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(FilePath::CreateConfigFilePath("Test"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestInteger"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(5);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestLightUserData"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny((void*)(nullptr));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestNil"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(Reflection::Any());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestNumber"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(1.5);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestString"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny((Byte const*)"Hello World");
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestString"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(String("Hello World"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestString"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(HString("Hello World"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestStringAlsoNumber"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny((Byte const*)"1.75");
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestStringAlsoNumber"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(String("1.75"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestStringAlsoNumber"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(HString("1.75"));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestTable"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(ScriptTestStruct("What is up?", 22));
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
		{
			Bool bResult = false;
			Script::FunctionInvoker invoker(*pVm, HString("TestUInt"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
			invoker.PushAny(32u);
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
			SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
		}
	}

	// ArrayIndex
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestArrayIndex"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushArrayIndex(Script::ArrayIndex(0));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// Boolean
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestBoolean"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushBoolean(true);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// Enum
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestEnum"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushEnumAsNumber(MemoryBudgets::Analytics);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// FilePath
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestFilePath"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushFilePath(FilePath::CreateConfigFilePath("Test"));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// Integer
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestInteger"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushInteger(5);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// Light user data.
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestLightUserData"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushLightUserData(nullptr);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// Nil
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestNil"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushNil();
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// Number
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestNumber"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushNumber(1.5);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// String
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestString"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushString((Byte const*)"Hello World");
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// String
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestString"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushString((Byte const*)"Hello World", 11);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// String
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestString"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushString(String("Hello World"));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// String
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestString"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushString(HString("Hello World"));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// StringAsNumber
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestStringAlsoNumber"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushString((Byte const*)"1.75");
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// StringAsNumber
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestStringAlsoNumber"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushString((Byte const*)"1.75", 4);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// StringAsNumber
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestStringAlsoNumber"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushString(String("1.75"));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// StringAsNumber
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestStringAlsoNumber"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushString(HString("1.75"));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// Table
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestTable"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushAsTable(ScriptTestStruct("What is up?", 22));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// Table
	{
		Bool bResult = false;
		DataStore dataStore;
		dataStore.MakeTable();
		dataStore.SetStringToTable(dataStore.GetRootNode(), HString("Value"), "What is up?");
		dataStore.SetInt32ValueToTable(dataStore.GetRootNode(), HString("Number"), 22);

		Script::FunctionInvoker invoker(*pVm, HString("TestTable"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.PushDataNode(dataStore, dataStore.GetRootNode()));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// UInt
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestUInt"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushUInt32(32u);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}

	// TODO: Technical limitation of reflection prevents this.
	// UserData
	/*
	{
		Bool bResult = false;
		Script::FunctionInvoker invoker(*pVm, HString("TestUserData"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, invoker.GetArgumentCount());
		invoker.PushUserData<ScriptTestPushUserData>()->m_iCount = 0;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetArgumentCount());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, s_iCount); s_iCount = 0;
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, bResult));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bResult);
	}*/
}

void ScriptTest::TestReflectionMultiSuccess()
{
	Script::VmSettings settings;
	SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));
	SEOUL_UNITTESTING_ASSERT(pVm->RunCode(
		"local native = SeoulNativeNewNativeUserData('ScriptTestReflectionMultiStruct')\n"
		"native:Construct(14)\n"
		"native:TestArg0()\n"
		"native:TestArg1(0)\n"
		"native:TestArg2(0, 1)\n"
		"native:TestArg3(0, 1, 2)\n"
		"native:TestArg4(0, 1, 2, 3)\n"
		"native:TestArg5(0, 1, 2, 3, 4)\n"
		"native:TestArg6(0, 1, 2, 3, 4, 5)\n"
		"native:TestArg7(0, 1, 2, 3, 4, 5, 6)\n"
		"native:TestArg8(0, 1, 2, 3, 4, 5, 6, 7)\n"
		"native:TestArg9(0, 1, 2, 3, 4, 5, 6, 7, 8)\n"
		"native:TestArg10(0, 1, 2, 3, 4, 5, 6, 7, 8, 9)\n"
		"native:TestArg11(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10)\n"
		"native:TestArg12(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11)\n"
		"native:TestArg13(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12)\n"));
}

void ScriptTest::TestReflectionReturn()
{
	static const Int kiTestArgFailures = 100;

	Script::VmSettings settings;
	SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));
	SEOUL_UNITTESTING_ASSERT(pVm->RunCode(
		"local native = SeoulNativeNewNativeUserData('ScriptTestReflectionReturnStruct')\n"
		"function TestArrayIndex() return native:TestArrayIndex() end\n"
		"function TestBoolean() return native:TestBoolean() end\n"
		"function TestEnum() return native:TestEnum() end\n"
		"function TestEnum2() return native:TestEnum2() end\n"
		"function TestFilePath() return native:TestFilePath() end\n"
		"function TestFunction() return native:TestFunction() end\n"
		"function TestInteger() return native:TestInteger() end\n"
		"function TestLightUserData() return native:TestLightUserData() end\n"
		"function TestNumber() return native:TestNumber() end\n"
		"function TestString() return native:TestString() end\n"
		"function TestStringAlsoNumber() return native:TestStringAlsoNumber() end\n"
		"function TestTable() return native:TestTable() end\n"
		"function TestUInt() return native:TestUInt() end\n"));

	// Any.
	{
		Reflection::Any any;
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestArrayIndex"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<Script::ArrayIndex>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<Script::ArrayIndex>());
			SEOUL_UNITTESTING_ASSERT_EQUAL(Script::ArrayIndex(0), any.Cast<Script::ArrayIndex>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestBoolean"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<Bool>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<Bool>());
			SEOUL_UNITTESTING_ASSERT_EQUAL(true, any.Cast<Bool>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestEnum"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<MemoryBudgets::Type>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<MemoryBudgets::Type>());
			SEOUL_UNITTESTING_ASSERT_EQUAL(MemoryBudgets::Analytics, any.Cast<MemoryBudgets::Type>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestEnum2"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<MemoryBudgets::Type>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<MemoryBudgets::Type>());
			SEOUL_UNITTESTING_ASSERT_EQUAL(MemoryBudgets::Analytics, any.Cast<MemoryBudgets::Type>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestFilePath"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<FilePath>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<FilePath>());
			SEOUL_UNITTESTING_ASSERT_EQUAL(FilePath::CreateConfigFilePath("Test"), any.Cast<FilePath>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestInteger"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<Int32>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<Int32>());
			SEOUL_UNITTESTING_ASSERT_EQUAL(5, any.Cast<Int32>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestLightUserData"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<void*>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<void*>());
			SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, any.Cast<void*>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestNumber"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<Double>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<Double>());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.5, any.Cast<Double>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestString"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<String>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<String>());
			SEOUL_UNITTESTING_ASSERT_EQUAL("Hello World", any.Cast<String>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestStringAlsoNumber"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<Double>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<Double>());
			SEOUL_UNITTESTING_ASSERT_EQUAL(1.75, any.Cast<Double>());
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestTable"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<ScriptTestStruct>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<ScriptTestStruct>());
			SEOUL_UNITTESTING_ASSERT_EQUAL("What is up?", any.Cast<ScriptTestStruct>().m_sValue);
			SEOUL_UNITTESTING_ASSERT_EQUAL(22, any.Cast<ScriptTestStruct>().m_iNumber);
			for (Int i = 1; i < kiTestArgFailures; ++i)
			{
				SEOUL_UNITTESTING_ASSERT(!invoker.GetAny(i, TypeId<void>(), any));
			}
		}
		{
			Script::FunctionInvoker invoker(*pVm, HString("TestUInt"));
			SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
			SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
			SEOUL_UNITTESTING_ASSERT(invoker.GetAny(0, TypeId<UInt32>(), any));
			SEOUL_UNITTESTING_ASSERT(any.IsOfType<UInt32>());
			SEOUL_UNITTESTING_ASSERT_EQUAL(32, any.Cast<UInt32>());
		}
	}

	// ArrayIndex.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestArrayIndex"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		Script::ArrayIndex index;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetArrayIndex(0, index));
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberExact(0));

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetArrayIndex(i, index));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(Script::ArrayIndex(0), index);
	}

	// Boolean.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestBoolean"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		Bool b = false;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetBoolean(0, b));
		SEOUL_UNITTESTING_ASSERT(invoker.IsBoolean(0));

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetBoolean(i, b));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, b);
	}

	// Enum1.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestEnum"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		MemoryBudgets::Type e = MemoryBudgets::Saving;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetEnum(0, e));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringCoercible(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringExact(0));
		SEOUL_UNITTESTING_ASSERT(!invoker.IsNumberCoercible(0));
		SEOUL_UNITTESTING_ASSERT(!invoker.IsNumberExact(0));

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetEnum(i, e));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(MemoryBudgets::Analytics, e);
	}

	// Enum2.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestEnum2"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		MemoryBudgets::Type e = MemoryBudgets::Saving;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetEnum(0, e));
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberCoercible(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberExact(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringCoercible(0));
		SEOUL_UNITTESTING_ASSERT(!invoker.IsStringExact(0));

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetEnum(i, e));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(MemoryBudgets::Analytics, e);
	}

	// FilePath
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestFilePath"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		FilePath filePath;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetFilePath(0, filePath));
		SEOUL_UNITTESTING_ASSERT(invoker.IsUserData(0));

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetFilePath(i, filePath));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(FilePath::CreateConfigFilePath("Test"), filePath);
	}

	// Integer.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestInteger"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		Int32 iInteger = 0;
		String s;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		// NOTE: Is* checks must come first, as the GetString() will cause
		// Lua to replace the number value on the stack with a string value.
		// As a result, after that call, the number will then be a string.
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberCoercible(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberExact(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringCoercible(0)); // In Lua, numbers are also coercible to strings.
		SEOUL_UNITTESTING_ASSERT(!invoker.IsStringExact(0)); // But our exact variation should only allow actual strings.
		SEOUL_UNITTESTING_ASSERT(invoker.GetInteger(0, iInteger));
		SEOUL_UNITTESTING_ASSERT(invoker.GetString(0, s));
		// Check Lua behavior, although it is unfortunate... after the call to GetString(), the value
		// will now be a coercible string, no longer a number.
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberCoercible(0));
		SEOUL_UNITTESTING_ASSERT(!invoker.IsNumberExact(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringCoercible(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringExact(0));

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetInteger(i, iInteger));
			SEOUL_UNITTESTING_ASSERT(!invoker.GetString(i, s));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, iInteger);
		SEOUL_UNITTESTING_ASSERT_EQUAL("5", s);
	}

	// LightUserData
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestLightUserData"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		void* p = (void*)1;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetLightUserData(0, p));
		SEOUL_UNITTESTING_ASSERT(invoker.IsLightUserData(0));

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetLightUserData(i, p));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, p);
	}

	// Number.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestNumber"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		Double f = 1.0;
		String s;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		// NOTE: Is* checks must come first, as the GetString() will cause
		// Lua to replace the number value on the stack with a string value.
		// As a result, after that call, the number will then be a string.
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberCoercible(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberExact(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringCoercible(0)); // In Lua, numbers are also coercible to strings.
		SEOUL_UNITTESTING_ASSERT(!invoker.IsStringExact(0)); // But our exact variation should only allow actual strings.
		SEOUL_UNITTESTING_ASSERT(invoker.GetNumber(0, f));
		SEOUL_UNITTESTING_ASSERT(invoker.GetString(0, s));
		// Check Lua behavior, although it is unfortunate... after the call to GetString(), the value
		// will now be a coercible string, no longer a number.
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberCoercible(0));
		SEOUL_UNITTESTING_ASSERT(!invoker.IsNumberExact(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringCoercible(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringExact(0));

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetNumber(i, f));
			SEOUL_UNITTESTING_ASSERT(!invoker.GetString(i, s));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.5, f);
		SEOUL_UNITTESTING_ASSERT_EQUAL("1.5", s);
	}

	// String.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestString"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		String s;
		Double fUnusedNumber;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetString(0, s));
		SEOUL_UNITTESTING_ASSERT(!invoker.GetNumber(0, fUnusedNumber));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringCoercible(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringExact(0));
		SEOUL_UNITTESTING_ASSERT(!invoker.IsNumberCoercible(0)); // This string is not convertible to a number.
		SEOUL_UNITTESTING_ASSERT(!invoker.IsNumberExact(0)); // This string is not convertible to a number.

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetString(i, s));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL("Hello World", s);
	}

	// String that is convertible to a number.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestStringAlsoNumber"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		String s;
		Double f;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		SEOUL_UNITTESTING_ASSERT(invoker.GetString(0, s));
		SEOUL_UNITTESTING_ASSERT(invoker.GetNumber(0, f));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringCoercible(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringExact(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberCoercible(0)); // A string coercible to a number.
		SEOUL_UNITTESTING_ASSERT(!invoker.IsNumberExact(0)); // But our exact variation should only allow an actual number.

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetString(i, s));
			SEOUL_UNITTESTING_ASSERT(!invoker.GetNumber(i, f));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL("1.75", s);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.75, f);
	}

	// Table.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestTable"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		// function TestTable() return { Value='What is up?', Number=22 } end\n"));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());

		{
			SharedPtr<Script::VmObject> p;
			SEOUL_UNITTESTING_ASSERT(invoker.GetObject(0, p));
			SEOUL_UNITTESTING_ASSERT(invoker.IsTable(0));
			SEOUL_UNITTESTING_ASSERT(p.IsValid());

			DataStore dataStore;
			SEOUL_UNITTESTING_ASSERT(p->TryToDataStore(dataStore));

			DataNode node;
			String s;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("Value"), node));
			SEOUL_UNITTESTING_ASSERT(dataStore.AsString(node, s));
			SEOUL_UNITTESTING_ASSERT_EQUAL("What is up?", s);
			Int32 i;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("Number"), node));
			SEOUL_UNITTESTING_ASSERT(dataStore.AsInt32(node, i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(22, i);
		}

		{
			ScriptTestStruct scriptTest;
			invoker.GetTableAsComplex(0, scriptTest);
			SEOUL_UNITTESTING_ASSERT_EQUAL("What is up?", scriptTest.m_sValue);
			SEOUL_UNITTESTING_ASSERT_EQUAL(22, scriptTest.m_iNumber);
		}

		DataStore dataStore;
		SEOUL_UNITTESTING_ASSERT(invoker.GetTable(0, dataStore));

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetTable(i, dataStore));
		}

		{
			DataNode node;
			String s;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("Value"), node));
			SEOUL_UNITTESTING_ASSERT(dataStore.AsString(node, s));
			SEOUL_UNITTESTING_ASSERT_EQUAL("What is up?", s);
			Int32 i;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("Number"), node));
			SEOUL_UNITTESTING_ASSERT(dataStore.AsInt32(node, i));
			SEOUL_UNITTESTING_ASSERT_EQUAL(22, i);
		}
	}

	// UInt32.
	{
		Script::FunctionInvoker invoker(*pVm, HString("TestUInt"));
		SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
		SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());

		UInt32 uInteger = 0;
		String s;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, invoker.GetReturnCount());
		// NOTE: Is* checks must come first, as the GetString() will cause
		// Lua to replace the number value on the stack with a string value.
		// As a result, after that call, the number will then be a string.
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberCoercible(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberExact(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringCoercible(0)); // In Lua, numbers are also coercible to strings.
		SEOUL_UNITTESTING_ASSERT(!invoker.IsStringExact(0)); // But our exact variation should only allow actual strings.
		SEOUL_UNITTESTING_ASSERT(invoker.GetUInt32(0, uInteger));
		SEOUL_UNITTESTING_ASSERT(invoker.GetString(0, s));
		// Check Lua behavior, although it is unfortunate... after the call to GetString(), the value
		// will now be a coercible string, no longer a number.
		SEOUL_UNITTESTING_ASSERT(invoker.IsNumberCoercible(0));
		SEOUL_UNITTESTING_ASSERT(!invoker.IsNumberExact(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringCoercible(0));
		SEOUL_UNITTESTING_ASSERT(invoker.IsStringExact(0));

		for (Int i = 1; i < kiTestArgFailures; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(!invoker.GetUInt32(i, uInteger));
			SEOUL_UNITTESTING_ASSERT(!invoker.GetString(i, s));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(32, uInteger);
		SEOUL_UNITTESTING_ASSERT_EQUAL("32", s);
	}
}

void ScriptTestOutput(Byte const* s)
{
	SEOUL_LOG("ScriptTestError: %s", s);
	SEOUL_UNITTESTING_ASSERT(false);
}

void ScriptTest::TestReflectionTypes()
{
	{
		Script::VmSettings settings;
		settings.m_StandardOutput = SEOUL_BIND_DELEGATE(ScriptTestOutput);
		SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));
		SEOUL_UNITTESTING_ASSERT(pVm->RunCode(
			"local kTestComplex = {p0={i=230498,f=982379.0,t={F=7,HU=1048,H308=293878},v={'Hi','There'}},p1={i=9347589,f=345909.0,t={Ger=8,BoU=-275,['093k']=9832},v={'Wonderful'}}}\n"
			"local function TableEquals(a, b)\n"
			"	for k,v in pairs(a) do\n"
			"		if not b[k] or b[k] ~= v then return false end\n"
			"	end\n"
			"	for k,v in pairs(b) do\n"
			"		if not a[k] or a[k] ~= v then return false end\n"
			"	end\n"
			"	return true\n"
			"end\n"
			"local function SubEquals(a, b)\n"
			"	if a.i ~= b.i then return false end\n"
			"	if a.f ~= b.f then return false end\n"
			"	if not TableEquals(a.t, b.t) then return false end\n"
			"	if not TableEquals(a.v, b.v) then return false end\n"
			"	return true\n"
			"end\n"
			"local function ComplexEquals(a, b)\n"
			"	if not SubEquals(a.p0, b.p0) then return false end\n"
			"	if not SubEquals(a.p1, b.p1) then return false end\n"
			"	return true\n"
			"end\n"

			"local native = SeoulNativeNewNativeUserData('ScriptTestReflectionTypesStruct')\n"
			"local kFilePath = native:GetTestFilePath()\n"
			"local kLightUserData = native:GetTestLightUserData()\n"
			"if 1 ~= native:TestArrayIndex(1) then error('ArrayIndex') end\n"
			"if 23 ~= native:TestAtomic32(23) then error('Atomic32') end\n"
			"if true ~= native:TestBoolean(true) then error('Boolean') end\n"
			"do c = native:TestColor4({0.25, 0.5, 0.75, 1.0}); if c[1] ~= 0.25 or c[2] ~= 0.5 or c[3] ~= 0.75 or c[4] ~= 1.0 then error('Color4') end end\n"
			"do c = native:TestComplex(kTestComplex); if not ComplexEquals(c, kTestComplex) then error('Complex') end end\n"
			"if 'Fun Times' ~= native:TestCString('Fun Times') then error('TestCString') end\n"
			"if 8 ~= native:TestEnum(8) then error('TestEnum') end\n"
			"if 8 ~= native:TestEnum2('Content') then error('TestEnum2') end\n"
			"if kFilePath ~= native:TestFilePath(kFilePath) then error('TestFilePath') end\n"
			"do c = native:TestFixedArray({2, 3, 4}); if c[1] ~= 2 or c[2] ~= 3 or c[3] ~= 4 then error('TestFixedArray') end end\n"
			"if -39 ~= native:TestFloat32(-39) then error('TestFloat32') end\n"
			"if 79 ~= native:TestFloat64(79) then error('TestFloat64') end\n"
			"do c = native:TestHashSetSimple({27, 49}); if not ((c[1] == 27 and c[2] == 49) or (c[1] == 49 and c[2] == 27)) then error('TestHashSetSimple') end end\n"
			"do c = native:TestHashTableSimple({[1]=0.25,[2]=0.75}); if c[1] ~= 0.25 or c[2] ~= 0.75 then error('TestHashTableSimple') end end\n"
			"if 'Oh No' ~= native:TestHString('Oh No') then error('TestHString') end\n"
			"if kLightUserData ~= native:TestLightUserData(kLightUserData) then error('TestLightUserData') end\n"
			"do c = native:TestListSimple({'Hi', 'There'}); if c[1] ~= 'Hi' or c[2] ~= 'There' then error('TestListSimple') end end\n"
			"if -7 ~= native:TestInt8(-7) then error('TestInt8') end\n"
			"if 5 ~= native:TestInt16(5) then error('TestInt16') end\n"
			"if -19 ~= native:TestInt32(-19) then error('TestInt32') end\n"
			"if 755 ~= native:TestInt64(755) then error('TestInt64') end\n"
			"do c = native:TestPairSimple({25, 37}); if c[1] ~= 25 or c[2] ~= 37 then error('TestPairSimple') end end\n"
			"do c = native:TestPoint2DInt({7, 85}); if c[1] ~= 7 or c[2] ~= 85 then error('TestPoint2DInt') end end\n"
			"do c = native:TestQuaternion({0, 0, 0, 1}); if c[1] ~= 0 or c[2] ~= 0 or c[3] ~= 0 or c[4] ~= 1 then error('TestQuaternion') end end\n"
			"if 'Delicious' ~= native:TestString('Delicious') then error('TestString') end\n"
			"if 33 ~= native:TestUInt8(33) then error('TestUInt8') end\n"
			"if 57 ~= native:TestUInt16(57) then error('TestUInt16') end\n"
			"if 99 ~= native:TestUInt32(99) then error('TestUInt32') end\n"
			"if 873 ~= native:TestUInt64(873) then error('TestUInt64') end\n"
			"if 'fe731c4a-b181-4b8f-a6cb-c8acec023d6a' ~= native:TestUUID('fe731c4a-b181-4b8f-a6cb-c8acec023d6a') then error('TestUUID') end\n"
			"do c = native:TestVectorSimple({0.25, 0.5, 0.75}); if c[1] ~= 0.25 or c[2] ~= 0.5 or c[3] ~= 0.75 then error('TestVectorSimple') end end\n"
			"do c = native:TestVector2D({8, -27}); if c[1] ~= 8 or c[2] ~= -27 then error('TestVector2D') end end\n"
			"do c = native:TestVector3D({3, 39, 85}); if c[1] ~= 3 or c[2] ~= 39 or c[3] ~= 85 then error('TestVector3D') end end\n"
			"do c = native:TestVector4D({3, 39, 85, 357}); if c[1] ~= 3 or c[2] ~= 39 or c[3] ~= 85 or c[4] ~= 357 then error('TestVector4D') end end\n"
			"local tWorldTime = SeoulDescribeNativeUserData('WorldTime')\n"
			"local worldTime1 = tWorldTime:FromMicroseconds(3e+6)\n"
			"local worldTime2 = tWorldTime:ParseISO8601DateTime('1970-01-01 00:00:03')\n"
			"if worldTime1 ~= native:TestWorldTime(worldTime1) then error('TestWorldTime') end\n"
			"if worldTime1 ~= native:TestWorldTime2(worldTime2) then error('TestWorldTime2') end\n"));
	}
}

void ScriptTest::TestSetGlobal()
{
	Script::VmSettings settings;
	SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));

	DataStore dataStore;
	dataStore.MakeTable();
	SEOUL_UNITTESTING_ASSERT(dataStore.SetStringToTable(dataStore.GetRootNode(), HString("TestIt"), "Testing Testing Testing."));

	SharedPtr<Script::VmObject> pBinding1;
	SEOUL_UNITTESTING_ASSERT(pVm->BindStrongTable(pBinding1, dataStore, dataStore.GetRootNode()));
	SEOUL_UNITTESTING_ASSERT(pBinding1.IsValid());

	SEOUL_UNITTESTING_ASSERT(pVm->TrySetGlobal(HString("TestGlobal"), pBinding1));

	SharedPtr<Script::VmObject> pBinding2;
	SEOUL_UNITTESTING_ASSERT(pVm->TryGetGlobal(HString("TestGlobal"), pBinding2));
	SEOUL_UNITTESTING_ASSERT(pBinding2.IsValid());

	DataStore dataStore2;
	SEOUL_UNITTESTING_ASSERT(pBinding2->TryToDataStore(dataStore2));

	SEOUL_UNITTESTING_ASSERT(DataStore::Equals(dataStore, dataStore.GetRootNode(), dataStore2, dataStore2.GetRootNode()));
}

void ScriptTest::TestWeakBinding()
{
	Script::VmSettings settings;
	SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));
	SEOUL_UNITTESTING_ASSERT(pVm->RunCode(
		"function TestUserData(ud)\n"
		"	ud:TestMethod2()\n"
		"end\n"));

	ScriptTestPushUserData ud; ud.m_iCount = 0;
	SharedPtr<Script::VmObject> p;
	SEOUL_UNITTESTING_ASSERT(pVm->BindWeakInstance(&ud, p));

	Script::FunctionInvoker invoker(*pVm, HString("TestUserData"));
	SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
	invoker.PushObject(p);
	SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, s_iCount); s_iCount = 0;
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, ud.m_iCount);
}

/** Test that our global override of Lua's math.random() and math.randomseed() works as expected. */
void ScriptTest::TestRandom()
{
	static Byte const* s_k = "math.randomseed(29347)\n"
		"if 0.069695632643939853 ~= math.random() then error('Unexpected math.random() value.') end\n"
		"math.randomseed(1259830)\n"
		"if 278 ~= math.random(300) then error('Unexpected math.random(u) value.') end\n"
		"math.randomseed(982938409)\n"
		"if 576047 ~= math.random(7385, 1036693) then error('Unexpected math.random(l, u) value.') end\n"
		"if 0 ~= math.random(0, 0) then error('Unexpected math.random(0, 0) value.') end\n"
		"if 1 ~= math.random(1) then error('Unexpected math.random(1) value.') end\n"
		"for i=-100,100 do if i ~= math.random(i, i) then error('Unexpected math.random(i, i) value.') end end\n";

	Script::VmSettings settings;
	settings.m_StandardOutput = SEOUL_BIND_DELEGATE(TestLog);
	settings.m_ErrorHandler = SEOUL_BIND_DELEGATE(TestError);
	SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));
	SEOUL_UNITTESTING_ASSERT(pVm->RunCode(s_k));
}

struct ScriptTestI32OpReference SEOUL_SEALED
{
	static inline UInt64 ToRawUInt64(Double f)
	{
		union
		{
			Double fIn;
			UInt64 uOut;
		};
		fIn = f;
		return uOut;
	}

	void Add(Double a, Double b, Double r)
	{
		auto const exp = (Double)((Int32)a + (Int32)b);
		SEOUL_UNITTESTING_ASSERT_EQUAL_MESSAGE(
			exp, r,
			"(%f + %f = %f) != %f (%" PRIu64 " != %" PRIu64 ")", a, b, exp, r, ToRawUInt64(exp), ToRawUInt64(r));
	}

	void Div(Double a, Double b, Double r)
	{
		auto const iA = (Int32)a;
		auto const iB = (Int32)b;

		// Undefined cases (these will generate a hardware exception in general
		// but our implementation instead evalutes to 0 or IntMin).
		Double exp;
#if defined(__arm__) || defined(__arm64__) || defined(__aarch64__)
		if (iA == IntMin && iB == -1) { exp = (Double)IntMax; }
		else if (iB == 0)
		{
			if (iA > 0) { exp = (Double)IntMax; }
			else if (iA == 0) { exp = (Double)0; }
			else { exp = (Double)IntMin; }
		}
#else
		if (iA == IntMin && iB == -1) { exp = (Double)IntMin; }
		else if (iB == 0) { exp = (Double)IntMin; }
#endif
		else { exp = (Double)(iA / iB); }

		SEOUL_UNITTESTING_ASSERT_EQUAL_MESSAGE(
			exp, r,
			"(%f / %f = %f) != %f (%" PRIu64 " != %" PRIu64 ")", a, b, exp, r, ToRawUInt64(exp), ToRawUInt64(r));
	}

	void Mod(Double a, Double b, Double r)
	{
		auto const iA = (Int32)a;
		auto const iB = (Int32)b;

		// Undefined cases (these will generate a hardware exception in general
		// but our implementation instead evalutes to 0).
		Double exp;
		if (iA == IntMin && iB == -1) { exp = (Double)0; }
		else if (iB == 0) { exp = (Double)0; }
		else { exp = (Double)(iA % iB); }

		SEOUL_UNITTESTING_ASSERT_EQUAL_MESSAGE(
			exp, r,
			"(%f %% %f = %f) != %f (%" PRIu64 " != %" PRIu64 ")", a, b, exp, r, ToRawUInt64(exp), ToRawUInt64(r));
	}

	void Mul(Double a, Double b, Double r)
	{
		auto const exp = (Double)((Int32)a * (Int32)b);
		SEOUL_UNITTESTING_ASSERT_EQUAL_MESSAGE(
			exp, r,
			"(%f * %f = %f) != %f (%" PRIu64 " != %" PRIu64 ")", a, b, exp, r, ToRawUInt64(exp), ToRawUInt64(r));
	}

	void Sub(Double a, Double b, Double r)
	{
		auto const exp = (Double)((Int32)a - (Int32)b);
		SEOUL_UNITTESTING_ASSERT_EQUAL_MESSAGE(
			exp, r,
			"(%f - %f = %f) != %f (%" PRIu64 " != %" PRIu64 ")", a, b, exp, r, ToRawUInt64(exp), ToRawUInt64(r));
	}

	void Truncate(Double a, Double r)
	{
		auto const exp = (Double)((Int32)a);
		SEOUL_UNITTESTING_ASSERT_EQUAL_MESSAGE(
			exp, r,
			"(truncate(%f) = %f) != %f (%" PRIu64 " != %" PRIu64 ")", a, exp, r, ToRawUInt64(exp), ToRawUInt64(r));
	}
}; // struct ScriptTestI32OpReference
SEOUL_BEGIN_TYPE(ScriptTestI32OpReference)
	SEOUL_METHOD(Add)
	SEOUL_METHOD(Div)
	SEOUL_METHOD(Mod)
	SEOUL_METHOD(Mul)
	SEOUL_METHOD(Sub)
	SEOUL_METHOD(Truncate)
SEOUL_END_TYPE()

static void TestI32Ops(HString func)
{
	auto const sScript =
		R"_(
			local native_orig = SeoulNativeNewNativeUserData('ScriptTestI32OpReference')
			local function check1(a)
				if type(a) ~= 'number' then error(tostring(a) .. ' is not a number, is type "' .. tostring(type(a)) .. '".') end
			end
			local function check(...)
				local args = {...}
				for _,v in ipairs(args) do
					check1(v)
				end
			end
			local native = {
				Add = function(_, a, b, r) check(a, b, r); native_orig:Add(a, b, r) end,
				Div = function(_, a, b, r) check(a, b, r); native_orig:Div(a, b, r) end,
				Mod = function(_, a, b, r) check(a, b, r); native_orig:Mod(a, b, r) end,
				Mul = function(_, a, b, r) check(a, b, r); native_orig:Mul(a, b, r) end,
				Sub = function(_, a, b, r) check(a, b, r); native_orig:Sub(a, b, r) end,
				Truncate = function(_, a, r) check(a, r); native_orig:Truncate(a, r) end,
			}
			local __i32mod__ = math.i32mod
			local __i32mul__ = math.i32mul
			local __i32narrow__ = bit.tobit
			local __i32truncate__ = math.i32truncate
			local min = -2147483648
			local max = 2147483647
			local nums = {
				min,
				min+10,min+9,min+8,min+7,min+6,min+5,min+4,min+3,min+2,min+1,
				-105,-104,-103,-102,-101,-100,-99,-98,-97,-96,
				-10, -9, -8, -7, -6, -5, -4, -3, -2, -1,
				 -0,
				  0,
				 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,
				 96, 97, 98, 99,100,101,102,103,104,105,
				max-10,max-9,max-8,max-7,max-6,max-5,max-4,max-3,max-2,max-1,
				max,
			}

			function AddNV()
				for _,b in ipairs(nums) do
					native:Add(-2, b, __i32narrow__(-2 + b))       -- integer add is narrowed number add
					native:Add(-1, b, __i32narrow__(-1 + b))       -- integer add is narrowed number add
					native:Add(-0, b, __i32narrow__(-0 + b))       -- integer add is narrowed number add
					native:Add( 0, b, __i32narrow__( 0 + b))       -- integer add is narrowed number add
					native:Add( 1, b, __i32narrow__( 1 + b))       -- integer add is narrowed number add
					native:Add( 2, b, __i32narrow__( 2 + b))       -- integer add is narrowed number add
				end
			end
			function DivNV()
				for _,b in ipairs(nums) do
					native:Div(-2, b, __i32truncate__(-2 / b))     -- integer div is i32truncated number divide
					native:Div(-1, b, __i32truncate__(-1 / b))     -- integer div is i32truncated number divide
					native:Div(-0, b, __i32truncate__(-0 / b))     -- integer div is i32truncated number divide
					native:Div( 0, b, __i32truncate__( 0 / b))     -- integer div is i32truncated number divide
					native:Div( 1, b, __i32truncate__( 1 / b))     -- integer div is i32truncated number divide
					native:Div( 2, b, __i32truncate__( 2 / b))     -- integer div is i32truncated number divide
				end
			end
			function ModExtNV()
				for _,b in ipairs(nums) do
					native:Mod(-2, b, __i32mod__(-2, b))           -- interp. friendly mod - uses an extension we added to math.
					native:Mod(-1, b, __i32mod__(-1, b))           -- interp. friendly mod - uses an extension we added to math.
					native:Mod(-0, b, __i32mod__(-0, b))           -- interp. friendly mod - uses an extension we added to math.
					native:Mod( 0, b, __i32mod__( 0, b))           -- interp. friendly mod - uses an extension we added to math.
					native:Mod( 1, b, __i32mod__( 1, b))           -- interp. friendly mod - uses an extension we added to math.
					native:Mod( 2, b, __i32mod__( 2, b))           -- interp. friendly mod - uses an extension we added to math.
				end
			end
			function MulExtNV()
				for _,b in ipairs(nums) do
					native:Mul(-2, b, __i32mul__(-2, b))           -- interp. friendly mul - uses an extension we added to math.
					native:Mul(-1, b, __i32mul__(-1, b))           -- interp. friendly mul - uses an extension we added to math.
					native:Mul(-0, b, __i32mul__(-0, b))           -- interp. friendly mul - uses an extension we added to math.
					native:Mul( 0, b, __i32mul__( 0, b))           -- interp. friendly mul - uses an extension we added to math.
					native:Mul( 1, b, __i32mul__( 1, b))           -- interp. friendly mul - uses an extension we added to math.
					native:Mul( 2, b, __i32mul__( 2, b))           -- interp. friendly mul - uses an extension we added to math.
				end
			end
			function SubNV()
				for _,b in ipairs(nums) do
					native:Sub(-2, b, __i32narrow__(-2 - b))       -- integer sub is narrowed number sub
					native:Sub(-1, b, __i32narrow__(-1 - b))       -- integer sub is narrowed number sub
					native:Sub(-0, b, __i32narrow__(-0 - b))       -- integer sub is narrowed number sub
					native:Sub( 0, b, __i32narrow__( 0 - b))       -- integer sub is narrowed number sub
					native:Sub( 1, b, __i32narrow__( 1 - b))       -- integer sub is narrowed number sub
					native:Sub( 2, b, __i32narrow__( 2 - b))       -- integer sub is narrowed number sub
				end
			end

			function AddVN()
				for _,a in ipairs(nums) do
					native:Add(a, -2, __i32narrow__(a + -2))       -- integer add is narrowed number add
					native:Add(a, -1, __i32narrow__(a + -1))       -- integer add is narrowed number add
					native:Add(a, -0, __i32narrow__(a + -0))       -- integer add is narrowed number add
					native:Add(a,  0, __i32narrow__(a +  0))       -- integer add is narrowed number add
					native:Add(a,  1, __i32narrow__(a +  1))       -- integer add is narrowed number add
					native:Add(a,  2, __i32narrow__(a +  2))       -- integer add is narrowed number add
				end
			end
			function DivVN()
				for _,a in ipairs(nums) do
					native:Div(a, -2, __i32truncate__(a / -2))     -- integer div is i32truncated number divide
					native:Div(a, -1, __i32truncate__(a / -1))     -- integer div is i32truncated number divide
					native:Div(a, -0, __i32truncate__(a / -0))     -- integer div is i32truncated number divide
					native:Div(a,  0, __i32truncate__(a /  0))     -- integer div is i32truncated number divide
					native:Div(a,  1, __i32truncate__(a /  1))     -- integer div is i32truncated number divide
					native:Div(a,  2, __i32truncate__(a /  2))     -- integer div is i32truncated number divide
				end
			end
			function ModExtVN()
				for _,a in ipairs(nums) do
					native:Mod(a, -2, __i32mod__(a, -2))           -- interp. friendly mod - uses an extension we added to math.
					native:Mod(a, -1, __i32mod__(a, -1))           -- interp. friendly mod - uses an extension we added to math.
					native:Mod(a, -0, __i32mod__(a, -0))           -- interp. friendly mod - uses an extension we added to math.
					native:Mod(a,  0, __i32mod__(a,  0))           -- interp. friendly mod - uses an extension we added to math.
					native:Mod(a,  1, __i32mod__(a,  1))           -- interp. friendly mod - uses an extension we added to math.
					native:Mod(a,  2, __i32mod__(a,  2))           -- interp. friendly mod - uses an extension we added to math.
				end
			end
			function MulExtVN()
				for _,a in ipairs(nums) do
					native:Mul(a, -2, __i32mul__(a, -2))           -- interp. friendly mul - uses an extension we added to math.
					native:Mul(a, -1, __i32mul__(a, -1))           -- interp. friendly mul - uses an extension we added to math.
					native:Mul(a, -0, __i32mul__(a, -0))           -- interp. friendly mul - uses an extension we added to math.
					native:Mul(a,  0, __i32mul__(a,  0))           -- interp. friendly mul - uses an extension we added to math.
					native:Mul(a,  1, __i32mul__(a,  1))           -- interp. friendly mul - uses an extension we added to math.
					native:Mul(a,  2, __i32mul__(a,  2))           -- interp. friendly mul - uses an extension we added to math.
				end
			end
			function SubVN()
				for _,a in ipairs(nums) do
					native:Sub(a, -2, __i32narrow__(a - -2))       -- integer sub is narrowed number sub
					native:Sub(a, -1, __i32narrow__(a - -1))       -- integer sub is narrowed number sub
					native:Sub(a, -0, __i32narrow__(a - -0))       -- integer sub is narrowed number sub
					native:Sub(a,  0, __i32narrow__(a -  0))       -- integer sub is narrowed number sub
					native:Sub(a,  1, __i32narrow__(a -  1))       -- integer sub is narrowed number sub
					native:Sub(a,  2, __i32narrow__(a -  2))       -- integer sub is narrowed number sub
				end
			end

			function AddVV()
				for _,a in ipairs(nums) do
					for _,b in ipairs(nums) do
						native:Add(a, b, __i32narrow__(a + b))       -- integer add is narrowed number add
					end
				end
			end
			function DivVV()
				for _,a in ipairs(nums) do
					for _,b in ipairs(nums) do
						native:Div(a, b, __i32truncate__(a / b))     -- integer div is i32truncated number divide
					end
				end
			end
			function ModExtVV()
				for _,a in ipairs(nums) do
					for _,b in ipairs(nums) do
						native:Mod(a, b, __i32mod__(a, b))           -- interp. friendly mod - uses an extension we added to math.
					end
				end
			end
			function MulExtVV()
				for _,a in ipairs(nums) do
					for _,b in ipairs(nums) do
						native:Mul(a, b, __i32mul__(a, b))           -- interp. friendly mul - uses an extension we added to math.
					end
				end
			end
			function SubVV()
				for _,a in ipairs(nums) do
					for _,b in ipairs(nums) do
						native:Sub(a, b, __i32narrow__(a - b))       -- integer sub is narrowed number sub
					end
				end
			end

			local trunc_nums = {
				4294967295,
				-4294967295 - 1.0,
				-2147483649,
				-2147483648,
				2147483648,
				4294967296,
				-9007199254740993,
				-9007199254740992,
				-9007199254740991,
				 9007199254740993,
				 9007199254740992,
				 9007199254740991,

				 0 / 0,
				-1 / 0,
				 1 / 0,
			}

			function Truncate()
				for _,a in ipairs(nums) do
					native:Truncate(a, __i32truncate__(a))
				end
				for _,a in ipairs(trunc_nums) do
					native:Truncate(a, __i32truncate__(a))
				end
				for i=0,100 do
					local v = 1 / i
					native:Truncate(v, __i32truncate__(v))
				end
				for i=-4294967296,4294967296,100000 do
					native:Truncate(i, __i32truncate__(i))
				end
			end

			local errtypes = {
				true,
				false,
				"not a number",
				function() end,
				{},
			}

			function StartsWith(str, subStr)
				return string.sub(str, 1, string.len(subStr)) == subStr
			end

			local function ExpectError(status, res)
				-- Success is a failure in this case.
				if status then
					error('expected failure succeeded with "' .. tostring(res) .. '"')
				end

				-- Only expecting type mismatch failures.
				if not StartsWith(res, 'bad argument #') then
					error('expected failure failed with the wrong error "' .. tostring(res) .. '"')
				end
			end

			function ModExtErrors()
				for _,a in ipairs(errtypes) do
					for _,b in ipairs(errtypes) do
						ExpectError(pcall(__i32mod__, a, b))
					end

					ExpectError(pcall(__i32mod__))
					ExpectError(pcall(__i32mod__, 1))
				end
			end

			function MulExtErrors()
				for _,a in ipairs(errtypes) do
					for _,b in ipairs(errtypes) do
						ExpectError(pcall(__i32mul__, a, b))
					end

					ExpectError(pcall(__i32mul__))
					ExpectError(pcall(__i32mul__, 1))
				end
			end

			function TruncateErrors()
				for _,a in ipairs(errtypes) do
					ExpectError(pcall(__i32truncate__, a))
				end

				ExpectError(pcall(__i32truncate__))
			end

			function Wrap(name)
				_G[name]()
			end
		)_";

	Script::VmSettings settings;
	settings.m_StandardOutput = SEOUL_BIND_DELEGATE(TestLog);
	settings.m_ErrorHandler = SEOUL_BIND_DELEGATE(TestError);
	SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));
	SEOUL_UNITTESTING_ASSERT(pVm->RunCode(sScript));

	HString const k("Wrap");
	Script::FunctionInvoker invoker(*pVm, k);
	SEOUL_UNITTESTING_ASSERT(invoker.IsValid());
	invoker.PushString(func);
	SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
}

/**
 * Test our 32-bit int extensions to lua.
 */

// Constant(number)-Variable variations.
void ScriptTest::TestI32AddNV() { TestI32Ops(HString("AddNV")); }
void ScriptTest::TestI32DivNV() { TestI32Ops(HString("DivNV")); }
void ScriptTest::TestI32ModExtensionNV() { TestI32Ops(HString("ModExtNV")); }
void ScriptTest::TestI32MulExtensionNV() { TestI32Ops(HString("MulExtNV")); }
void ScriptTest::TestI32SubNV() { TestI32Ops(HString("SubNV")); }

// Variable-Constant(number) variations.
void ScriptTest::TestI32AddVN() { TestI32Ops(HString("AddVN")); }
void ScriptTest::TestI32DivVN() { TestI32Ops(HString("DivVN")); }
void ScriptTest::TestI32ModExtensionVN() { TestI32Ops(HString("ModExtVN")); }
void ScriptTest::TestI32MulExtensionVN() { TestI32Ops(HString("MulExtVN")); }
void ScriptTest::TestI32SubVN() { TestI32Ops(HString("SubVN")); }

// Variable-Variable variations.
void ScriptTest::TestI32AddVV() { TestI32Ops(HString("AddVV")); }
void ScriptTest::TestI32DivVV() { TestI32Ops(HString("DivVV")); }
void ScriptTest::TestI32ModExtensionVV() { TestI32Ops(HString("ModExtVV")); }
void ScriptTest::TestI32MulExtensionVV() { TestI32Ops(HString("MulExtVV")); }
void ScriptTest::TestI32SubVV() { TestI32Ops(HString("SubVV")); }

// Truncate function.
void ScriptTest::TestI32Truncate() { TestI32Ops(HString("Truncate")); }

// Error cases.
void ScriptTest::TestI32ModExtensionWrongTypes() { TestI32Ops(HString("ModExtErrors")); }
void ScriptTest::TestI32MulExtensionWrongTypes() { TestI32Ops(HString("MulExtErrors")); }
void ScriptTest::TestI32TruncateWrongTypes() { TestI32Ops(HString("TruncateErrors")); }

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
