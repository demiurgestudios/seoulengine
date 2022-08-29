/**
 * \file LexerParserTest.cpp
 * \brief Test SeoulEngine lexers and parsers - specifically, json files,
 * data stores in json files, and data store alone.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "LexerParserTest.h"
#include "Logger.h"
#include "GamePaths.h"
#include "GamePathsSettings.h"
#include "ReflectionDefine.h"
#include "UnitTesting.h"
#include "UnitTestsFileManagerHelper.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(LexerParserTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestDataStoreFromJsonFileBasic)
	SEOUL_METHOD(TestDataStoreFromJsonFileUnicode)
	SEOUL_METHOD(TestDataStoreBasic)
	SEOUL_METHOD(TestDataStoreNumbers)
	SEOUL_METHOD(TestDataStoreStrings)
	SEOUL_METHOD(TestDataStoreFromJsonFileErrors)
	SEOUL_METHOD(TestJSON)
	SEOUL_METHOD(TestDuplicateReject)
	SEOUL_METHOD(TestStringAsFilePath)
	SEOUL_METHOD(TestStringAsFilePathRegression)
SEOUL_END_TYPE()

/**
 * DataStore from .JSON file unicode tests - see file
 * for details.
 */
void LexerParserTest::TestDataStoreFromJsonFileUnicode()
{
	UnitTestsFileManagerHelper helper;
	FilePath filePath = FilePath::CreateConfigFilePath("UnitTests/DataStoreParser/UnicodeTest.json");

	DataStore dataStore;
	Bool const bResult = DataStoreParser::FromFile(filePath, dataStore, DataStoreParserFlags::kLogParseErrors);
	SEOUL_UNITTESTING_ASSERT(bResult);

	DataNode unicodeTest;
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("UnicodeTest"), unicodeTest));

	String sA;
	String sB;
	DataNode a;
	DataNode b;
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(unicodeTest, HString("unicode_runtime_testA"), a));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(unicodeTest, HString("unicode_runtime_testB"), b));
	SEOUL_UNITTESTING_ASSERT(DataStore::Equals(dataStore, a, dataStore, b));
	SEOUL_UNITTESTING_ASSERT(dataStore.AsString(a, sA));
	SEOUL_UNITTESTING_ASSERT(dataStore.AsString(b, sB));
	SEOUL_UNITTESTING_ASSERT_EQUAL(sA, sB);
}

/** Simple utility for NaN and inf testing. */
static void TestUtilInfinityAndNaN(
	const DataStore& dataStore,
	const DataNode& a,
	HString tableKey,
	Bool (*tester)(Float f))
{
	DataNode b;
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(a, tableKey, b));
	SEOUL_UNITTESTING_ASSERT(b.IsArray());
	UInt32 uArrayCount;
	SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(b, uArrayCount));
	SEOUL_UNITTESTING_ASSERT_GREATER_THAN(uArrayCount, 0u);
	for (UInt32 i = 0u; i < uArrayCount; ++i)
	{
		Float f;
		DataNode v;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(b, i, v));
		SEOUL_UNITTESTING_ASSERT(dataStore.AsFloat32(v, f));
		SEOUL_UNITTESTING_ASSERT(tester(f));
	}
}

static Bool IsNegativeInfinity(Float f) { return IsInf(f) && f < 0.0f; }
static Bool IsPositiveInfinity(Float f) { return IsInf(f) && f > 0.0f; }

/**
 * Test using DataStoreParser::FromFile to parse an JSON file into a DataStore.
 */
void LexerParserTest::TestDataStoreFromJsonFileBasic()
{
	UnitTestsFileManagerHelper helper;
	FilePath filePath = FilePath::CreateConfigFilePath("UnitTests/DataStoreParser/BasicTest.json");

	DataStore dataStore;
	Bool bResult = DataStoreParser::FromFile(filePath, dataStore);

	SEOUL_UNITTESTING_ASSERT(bResult);

	// Multiline tests.
	String sA;
	String sB;
	DataNode a;
	DataNode b;
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("key_with_simple_multiline_value"), a));
	SEOUL_UNITTESTING_ASSERT(a.IsString());
	SEOUL_UNITTESTING_ASSERT(dataStore.AsString(a, sA));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("with the value"), sA);

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("key_with_single_line"), a));
	SEOUL_UNITTESTING_ASSERT(a.IsString());
	SEOUL_UNITTESTING_ASSERT(dataStore.AsString(a, sA));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("almost the end"), sA);

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("key_with_complex_multiline_value"), a));
	SEOUL_UNITTESTING_ASSERT(a.IsTable());
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("key_with_complex_singleline_value"), b));
	SEOUL_UNITTESTING_ASSERT(b.IsTable());
	SEOUL_UNITTESTING_ASSERT(DataStore::Equals(dataStore, a, dataStore, b));

	dataStore.ToString(a, sA);
	dataStore.ToString(b, sB);
	SEOUL_UNITTESTING_ASSERT_EQUAL(sA, sB);

	String sTest;
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("EmptyKeyTests"), a));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(a, HString(), b));
	SEOUL_UNITTESTING_ASSERT(b.IsString());
	SEOUL_UNITTESTING_ASSERT(dataStore.AsString(b, sTest));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("this_should_be_valid1"), sTest);
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(a, HString("table"), b));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(b, HString(""), b));
	SEOUL_UNITTESTING_ASSERT(dataStore.AsString(b, sTest));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("this_should_be_valid2"), sTest);
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(a, HString("identifier"), b));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(b, HString(), b));
	SEOUL_UNITTESTING_ASSERT(dataStore.AsString(b, sTest));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("this_should_be_valid3"), sTest);

	// Infinity and NaN testing.
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("InfinityAndNanTests"), a));
	TestUtilInfinityAndNaN(dataStore, a, HString("_nan_"), Seoul::IsNaN); // NaN
	TestUtilInfinityAndNaN(dataStore, a, HString("_neg_inf_"), Seoul::IsNegativeInfinity); // -Infinity
	TestUtilInfinityAndNaN(dataStore, a, HString("_inf_"), Seoul::IsPositiveInfinity); // +Infinity
}

/**
 * Verifies that all types of a DataStore are correctly parsed out of the value
 * portion of a key-value pair.
 */
void LexerParserTest::TestDataStoreBasic()
{
	static Byte const* const ksaTestData[] =
	{
		"{\"Value\": true}",
		"{\"Value\": false}",
		"{\"Value\": null}",
		"{\"Value\": \"_abcd2345\"}",
		"{\"Value\": 0.0}",
		"{\"Value\": 0}",
		"{\"Value\": 10}",
		"{\"Value\": -10}",
		"{\"Value\": \"config://hello_world.json\"}",
		"{\"Value\": \"Hello World\"}",
		"{\"Value\": [1, 2, 3, 4]}",
		"{\"Value\": {\"One\": 1, \"Two\": 2, \"Three\": 3, \"Four\": 4}}",
	};

	DataStore dataStore;
	dataStore.MakeArray();

	UInt32 uIndex = 0u;
	DataNode expectedValues = dataStore.GetRootNode();
	SEOUL_UNITTESTING_ASSERT(dataStore.SetBooleanValueToArray(expectedValues, uIndex++, true));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetBooleanValueToArray(expectedValues, uIndex++, false));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetNullValueToArray(expectedValues, uIndex++));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetStringToArray(expectedValues, uIndex++, String("_abcd2345")));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(expectedValues, uIndex++, 0));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(expectedValues, uIndex++, 0));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(expectedValues, uIndex++, 10));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(expectedValues, uIndex++, -10));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetFilePathToArray(expectedValues, uIndex++, FilePath::CreateConfigFilePath("hello_world.json")));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetStringToArray(expectedValues, uIndex++, "Hello World"));

	DataNode subArray;
	SEOUL_UNITTESTING_ASSERT(dataStore.SetArrayToArray(expectedValues, uIndex++, 4u));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(expectedValues, uIndex - 1u, subArray));
	SEOUL_UNITTESTING_ASSERT(subArray.IsArray());
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(subArray, 0u, 1));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(subArray, 1u, 2));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(subArray, 2u, 3));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToArray(subArray, 3u, 4));

	DataNode subTable;
	SEOUL_UNITTESTING_ASSERT(dataStore.SetTableToArray(expectedValues, uIndex++, 4u));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(expectedValues, uIndex - 1u, subTable));
	SEOUL_UNITTESTING_ASSERT(subTable.IsTable());
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(subTable, HString("One"), 1));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(subTable, HString("Two"), 2));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(subTable, HString("Three"), 3));
	SEOUL_UNITTESTING_ASSERT(dataStore.SetInt32ValueToTable(subTable, HString("Four"), 4));

	for (UInt32 i = 0u; i < SEOUL_ARRAY_COUNT(ksaTestData); ++i)
	{
		DataStore parserDataStore;
		SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(ksaTestData[i], parserDataStore, DataStoreParserFlags::kLogParseErrors));

		DataNode value;
		SEOUL_UNITTESTING_ASSERT(parserDataStore.GetValueFromTable(parserDataStore.GetRootNode(), HString("Value"), value));

		DataNode expectedValue;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(dataStore.GetRootNode(), i, expectedValue));

		SEOUL_UNITTESTING_ASSERT(DataStore::Equals(dataStore, expectedValue, parserDataStore, value));
	}
}

/**
 * Test that parsing an INI file into a DataStore correctly handles a number of different
 * number format possibilities.
 */
void LexerParserTest::TestDataStoreNumbers()
{
#if SEOUL_PLATFORM_WINDOWS
	static const Int64 kBigInt64Test = 0x0123456789ABCDEFi64;
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
	static const Int64 kBigInt64Test = 0x0123456789ABCDEFll;
#else
#error "Define for this platform."
#endif

	static Byte const* const ksData =
		"{\"TestValue\": [0, 1.0, 2.1, -3, -4.1, 5.0, 2147483648, -2147483649, 18446744073709551615, 81985529216486895, 1e-5, 7E7, 10E+8, -67108864, 67108863, -67108865, 67108864]}";

	DataStore dataStore;
	Bool bResult = DataStoreParser::FromString(ksData, dataStore);

	SEOUL_UNITTESTING_ASSERT(bResult);
	SEOUL_UNITTESTING_ASSERT(dataStore.GetRootNode().IsTable());

	DataNode testValue;
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("TestValue"), testValue));
	SEOUL_UNITTESTING_ASSERT(testValue.IsArray());

	UInt32 uArrayCount = 0u;
	SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(testValue, uArrayCount));
	SEOUL_UNITTESTING_ASSERT_EQUAL(17u, uArrayCount);

	DataNode number;
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 0u, number));
	SEOUL_UNITTESTING_ASSERT(number.IsInt32Small());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, dataStore.AssumeInt32Small(number));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 1u, number));
	SEOUL_UNITTESTING_ASSERT(number.IsInt32Small());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, dataStore.AssumeInt32Small(number));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 2u, number));
	SEOUL_UNITTESTING_ASSERT(number.IsFloat31());
	SEOUL_UNITTESTING_ASSERT_EQUAL(2.1f, dataStore.AssumeFloat31(number));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 3u, number));
	SEOUL_UNITTESTING_ASSERT(number.IsInt32Small());
	SEOUL_UNITTESTING_ASSERT_EQUAL(-3, dataStore.AssumeInt32Small(number));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 4u, number));
	SEOUL_UNITTESTING_ASSERT(number.IsFloat32());
	SEOUL_UNITTESTING_ASSERT_EQUAL(-4.1f, dataStore.AssumeFloat32(number));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 5u, number));
	SEOUL_UNITTESTING_ASSERT(number.IsInt32Small());
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, dataStore.AssumeInt32Small(number));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 6u, number));
	SEOUL_UNITTESTING_ASSERT(number.IsUInt32());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt32)IntMax + 1u, dataStore.AssumeUInt32(number));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 7u, number));
	SEOUL_UNITTESTING_ASSERT(number.IsInt64());
	SEOUL_UNITTESTING_ASSERT_EQUAL((Int64)IntMin - 1, dataStore.AssumeInt64(number));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 8u, number));
	SEOUL_UNITTESTING_ASSERT(number.IsUInt64());
	SEOUL_UNITTESTING_ASSERT_EQUAL(UInt64Max, dataStore.AssumeUInt64(number));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 9u, number));
	SEOUL_UNITTESTING_ASSERT(number.IsInt64());
	SEOUL_UNITTESTING_ASSERT_EQUAL(kBigInt64Test, dataStore.AssumeInt64(number));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 10u, number));
	SEOUL_UNITTESTING_ASSERT(number.IsFloat31());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1e-5f, dataStore.AssumeFloat31(number));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 11u, number));
	SEOUL_UNITTESTING_ASSERT(number.IsInt32Big());
	SEOUL_UNITTESTING_ASSERT_EQUAL(70000000, dataStore.AssumeInt32Big(number));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 12u, number));
	SEOUL_UNITTESTING_ASSERT(number.IsInt32Big());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1000000000, dataStore.AssumeInt32Big(number));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 13u, number));
	SEOUL_UNITTESTING_ASSERT(number.IsInt32Small());
	SEOUL_UNITTESTING_ASSERT_EQUAL(kiDataNodeMinInt32SmallValue, dataStore.AssumeInt32Small(number));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 14u, number));
	SEOUL_UNITTESTING_ASSERT(number.IsInt32Small());
	SEOUL_UNITTESTING_ASSERT_EQUAL(kiDataNodeMaxInt32SmallValue, dataStore.AssumeInt32Small(number));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 15u, number));
	SEOUL_UNITTESTING_ASSERT(number.IsInt32Big());
	SEOUL_UNITTESTING_ASSERT_EQUAL(kiDataNodeMinInt32SmallValue - 1, dataStore.AssumeInt32Big(number));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 16u, number));
	SEOUL_UNITTESTING_ASSERT(number.IsInt32Big());
	SEOUL_UNITTESTING_ASSERT_EQUAL(kiDataNodeMaxInt32SmallValue + 1, dataStore.AssumeInt32Big(number));
}

/**
 * Test that parsing an JSON file into a DataStore correctly handles strings with
 * various escape sequences.
 */
void LexerParserTest::TestDataStoreStrings()
{
	static Byte const* const ksData =
		"{\"TestValue\": ["
			"\"\\\"Hello World\\\"\", "
			"\"   The\\bquick\\fbrown fox, \\n jumped over the lazy dog.\", "
			"\"Hello\\tWorld\\n,this is\\rthe radio show\", "
			"\"\\\\T\\\\h\\\\i\\\\s\\\\ \\\\i\\\\s\\\\ \\\\m\\u0075\\\\c\\\\h\\\\ \\\\e\\\\s\\\\c\\\\a\\\\p\\\\e\" ]}";

	static const String s0("\"Hello World\"");
	static const String s1("   The\bquick\fbrown fox, \n jumped over the lazy dog.");
	static const String s2("Hello\tWorld\n,this is\rthe radio show");
	static const String s3("\\T\\h\\i\\s\\ \\i\\s\\ \\mu\\c\\h\\ \\e\\s\\c\\a\\p\\e");

	DataStore dataStore;
	Bool bResult = DataStoreParser::FromString(ksData, dataStore);

	SEOUL_UNITTESTING_ASSERT(bResult);
	SEOUL_UNITTESTING_ASSERT(dataStore.GetRootNode().IsTable());

	DataNode testValue;
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("TestValue"), testValue));
	SEOUL_UNITTESTING_ASSERT(testValue.IsArray());

	UInt32 uArrayCount = 0u;
	SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(testValue, uArrayCount));
	SEOUL_UNITTESTING_ASSERT_EQUAL(4u, uArrayCount);

	DataNode string;
	String sString;
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 0u, string));
	SEOUL_UNITTESTING_ASSERT(string.IsString());
	SEOUL_UNITTESTING_ASSERT(dataStore.AsString(string, sString));
	SEOUL_UNITTESTING_ASSERT_EQUAL(s0, sString);

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 1u, string));
	SEOUL_UNITTESTING_ASSERT(string.IsString());
	SEOUL_UNITTESTING_ASSERT(dataStore.AsString(string, sString));
	SEOUL_UNITTESTING_ASSERT_EQUAL(s1, sString);

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 2u, string));
	SEOUL_UNITTESTING_ASSERT(string.IsString());
	SEOUL_UNITTESTING_ASSERT(dataStore.AsString(string, sString));
	SEOUL_UNITTESTING_ASSERT_EQUAL(s2, sString);

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(testValue, 3u, string));
	SEOUL_UNITTESTING_ASSERT(string.IsString());
	SEOUL_UNITTESTING_ASSERT(dataStore.AsString(string, sString));
	SEOUL_UNITTESTING_ASSERT_EQUAL(s3, sString);
}

/**
 * Verify that a number of DataStore syntax cases fail correctly.
 */
void LexerParserTest::TestDataStoreFromJsonFileErrors()
{
	static Byte const* const ksErrorCases[] =
	{
		// Array/table tests
		"{\"TestValue\": [1, 2, 3]a", // character after array
		"{\"TestValue\": {\"key\": 1, \"key2\": 2, \"key3\": 3}a", // character after table
		"{\"TestValue\": [1, 2, 3}", // incorrect array terminator
		"{\"TestValue\": {\"key\": 1, \"key2\": 2, \"key3\": 3]", // incorrect able terminator
		"{\"TestValue\": [1, 2, 3", // no array terminator
		"{\"TestValue\": {\"key\": 1, \"key2\": 2, \"key3\": 3", // no table terminator
		"{\"TestValue\": [1, 2 3]", // no comma in array
		"{\"TestValue\": {\"key\": 1, \"key2\": 2, }", // stray comma in table
	};

	for (size_t i = 0u; i < SEOUL_ARRAY_COUNT(ksErrorCases); ++i)
	{
		DataStore dataStore;
		SEOUL_UNITTESTING_ASSERT(!DataStoreParser::FromString(ksErrorCases[i], dataStore));
	}
}

void LexerParserTest::TestJSON()
{
	static Byte const ksTextStringJSON[] =
		"{\n"
			"\"first\": \"John\",\n"
			"\"last\": \"Doe\",\n"
			"\"age\": 39,\n"
			"\"sex\": \"M\",\n"
			"\"salary\": 70000,\n"
			"\"registered\": true,\n"
			"\"address\": null,\n"
			"\"interests\": [ \"Reading\", \"Mountain Biking\", \"Hacking\" ],\n"
			"\"favorites\": {\n"
				"\"color\": \"Blue\",\n"
				"\"sport\": \"Soccer\",\n"
				"\"food\": \"Spaghetti\"\n"
			"}\n"
		"}";

	// Place the string into a local buffer that is not null terminated, to verify
	// that the length passed into lexerContext is respected.
	Byte aUnterminatedBuffer[sizeof(ksTextStringJSON)];
	memcpy(aUnterminatedBuffer, ksTextStringJSON, sizeof(ksTextStringJSON) - 1);
	aUnterminatedBuffer[sizeof(ksTextStringJSON) - 1] = '6';

	DataStore dataStore;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(aUnterminatedBuffer, StrLen(ksTextStringJSON), dataStore));

	String sOutputValue;
	dataStore.ToString(dataStore.GetRootNode(), sOutputValue, true, 0);

	DataStore dataStore2;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(sOutputValue, dataStore2));

	SEOUL_UNITTESTING_ASSERT(DataStore::Equals(dataStore, dataStore.GetRootNode(), dataStore2, dataStore2.GetRootNode()));
}

// Regression - make sure our JSON parser does not allow duplicate key-value pairs.
void LexerParserTest::TestDuplicateReject()
{
	DataStore dataStore;

	// Null
	{
		auto const s = R"({"a": null, "a": null})";
		SEOUL_UNITTESTING_ASSERT(!DataStoreParser::FromString(s, dataStore));
		auto const sOk = R"({"a": null, "b": null})";
		SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(sOk, dataStore));
	}

	// Bool
	{
		auto const s = R"({"a": null, "a": true})";
		SEOUL_UNITTESTING_ASSERT(!DataStoreParser::FromString(s, dataStore));
		auto const sOk = R"({"a": null, "b": true})";
		SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(sOk, dataStore));
	}

	// Int
	{
		auto const s = R"({"a": null, "a": 0})";
		SEOUL_UNITTESTING_ASSERT(!DataStoreParser::FromString(s, dataStore));
		auto const sOk = R"({"a": null, "b": 0})";
		SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(sOk, dataStore));
	}

	// UInt
	{
		auto const s = R"({"a": null, "a": 2147483648})";
		SEOUL_UNITTESTING_ASSERT(!DataStoreParser::FromString(s, dataStore));
		auto const sOk = R"({"a": null, "b": 2147483648})";
		SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(sOk, dataStore));
	}

	// Int64
	{
		auto const s = R"({"a": null, "a": 9223372036854775807})";
		SEOUL_UNITTESTING_ASSERT(!DataStoreParser::FromString(s, dataStore));
		auto const sOk = R"({"a": null, "b": 9223372036854775807})";
		SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(sOk, dataStore));
	}

	// UInt64
	{
		auto const s = R"({"a": null, "a": 9223372036854775808})";
		SEOUL_UNITTESTING_ASSERT(!DataStoreParser::FromString(s, dataStore));
		auto const sOk = R"({"a": null, "b": 9223372036854775808})";
		SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(sOk, dataStore));
	}

	// Double
	{
		auto const s = R"({"a": null, "a": 1.5})";
		SEOUL_UNITTESTING_ASSERT(!DataStoreParser::FromString(s, dataStore));
		auto const sOk = R"({"a": null, "b": 1.5})";
		SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(sOk, dataStore));
	}

	// FilePath
	{
		auto const s = R"({"a": null, "a": "content://A.png"})";
		SEOUL_UNITTESTING_ASSERT(!DataStoreParser::FromString(s, dataStore));
		auto const sOk = R"({"a": null, "b": "content://A.png"})";
		SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(sOk, dataStore));
	}

	// String
	{
		auto const s = R"({"a": null, "a": "asdf"})";
		SEOUL_UNITTESTING_ASSERT(!DataStoreParser::FromString(s, dataStore));
		auto const sOk = R"({"a": null, "b": "asdf"})";
		SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(sOk, dataStore));
	}

	// Object
	{
		auto const s = R"({"a": null, "a": {}})";
		SEOUL_UNITTESTING_ASSERT(!DataStoreParser::FromString(s, dataStore));
		auto const sOk = R"({"a": null, "b": {}})";
		SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(sOk, dataStore));
	}

	// Array
	{
		auto const s = R"({"a": null, "a": []})";
		SEOUL_UNITTESTING_ASSERT(!DataStoreParser::FromString(s, dataStore));
		auto const Ok = R"({"a": null, "b": []})";
		SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(Ok, dataStore));
	}
}

void LexerParserTest::TestStringAsFilePath()
{
	FilePath filePath;

	// Directory only.
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::StringAsFilePath("content://", filePath));
	SEOUL_UNITTESTING_ASSERT(filePath.IsValid());
	SEOUL_UNITTESTING_ASSERT_EQUAL(GameDirectory::kContent, filePath.GetDirectory());
	SEOUL_UNITTESTING_ASSERT_EQUAL(FileType::kUnknown, filePath.GetType());
	SEOUL_UNITTESTING_ASSERT_EQUAL(FilePathRelativeFilename(), filePath.GetRelativeFilenameWithoutExtension());

	// No extension.
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::StringAsFilePath("content://Authored", filePath));
	SEOUL_UNITTESTING_ASSERT(filePath.IsValid());
	SEOUL_UNITTESTING_ASSERT_EQUAL(GameDirectory::kContent, filePath.GetDirectory());
	SEOUL_UNITTESTING_ASSERT_EQUAL(FileType::kUnknown, filePath.GetType());
	SEOUL_UNITTESTING_ASSERT_EQUAL(FilePathRelativeFilename("Authored"), filePath.GetRelativeFilenameWithoutExtension());

	// Typical.
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::StringAsFilePath("content://Authored/Test.fx", filePath));
	SEOUL_UNITTESTING_ASSERT(filePath.IsValid());
	SEOUL_UNITTESTING_ASSERT_EQUAL(GameDirectory::kContent, filePath.GetDirectory());
	SEOUL_UNITTESTING_ASSERT_EQUAL(FileType::kEffect, filePath.GetType());
	SEOUL_UNITTESTING_ASSERT_EQUAL(FilePathRelativeFilename("Authored" + Path::DirectorySeparatorChar() + "Test"), filePath.GetRelativeFilenameWithoutExtension());
}

// Regression for a bug where the (Byte const*, UInt32 u) form
// of StringAsFilePath would not respect the size of u.
void LexerParserTest::TestStringAsFilePathRegression()
{
	Byte const* s = "content://Authored/Effects/Text.fxh, \"Some Other Data\"";

	FilePath filePath;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::StringAsFilePath(s, 35, filePath));
	SEOUL_UNITTESTING_ASSERT_EQUAL(GameDirectory::kContent, filePath.GetDirectory());
	SEOUL_UNITTESTING_ASSERT_EQUAL(FileType::kEffectHeader, filePath.GetType());
	SEOUL_UNITTESTING_ASSERT_EQUAL(FilePath::CreateContentFilePath("Authored/Effects/Text.fxh"), filePath);

	// Intentional truncate, take advantage of .fxh vs. .fx for this test.
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::StringAsFilePath(s, 34, filePath));
	SEOUL_UNITTESTING_ASSERT_EQUAL(GameDirectory::kContent, filePath.GetDirectory());
	SEOUL_UNITTESTING_ASSERT_EQUAL(FileType::kEffect, filePath.GetType());
	SEOUL_UNITTESTING_ASSERT_EQUAL(FilePath::CreateContentFilePath("Authored/Effects/Text.fx"), filePath);
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
