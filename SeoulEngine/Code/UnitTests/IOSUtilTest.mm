/**
 * \file IOSUtilTest.mm
 * \brief Unit tests for iOS specific utilities.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#import <Foundation/Foundation.h>

#include "DataStoreParser.h"
#include "IOSUtil.h"
#include "IOSUtilTest.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(IOSUtilTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestConvertToDataStore)
	SEOUL_METHOD(TestConvertToNSDictionary)
	SEOUL_METHOD(TestConvertToDataStoreArrayFull)
	SEOUL_METHOD(TestConvertToNSDictionaryArrayFull)
	SEOUL_METHOD(TestToDataStoreFails)
	
	SEOUL_METHOD(TestConvertToDataStoreNumberKeys)
	SEOUL_METHOD(TestConvertToNSDictionaryNumberKeys)
SEOUL_END_TYPE()

namespace BasicTest
{

static Byte const* GetString()
{
	return
R"__DeLiM__(
{
	"Array": [ "One", "Two", "Three" ],
	"BoolFalse": false,
	"BoolTrue": true,
	"Float31": 0.99,
	"Float32": 1.666,
	"Int32Small": 1,
	"Int32Big": 67108864,
	"Int64": 9223372036854775807,
	"String": "Hello World",
	"Table": { "A": "B", "C": 5 },
	"UInt32": 4294967295,
	"UInt64": 18446744073709551615,
}
)__DeLiM__";
}

static NSDictionary* GetDictionary() SEOUL_RETURNS_AUTORELEASED
{
	return @{
		@"Array": @[ @"One", @"Two", @"Three" ],
		@"BoolFalse": @NO,
		@"BoolTrue": @YES,
		@"Float31": @0.99f,
		@"Float32": @1.666f,
		@"Int32Small": @1,
		@"Int32Big": @(1 << 26),
		@"Int64": @9223372036854775807LL,
		@"String": @"Hello World",
		@"Table": @{ @"A": @"B", @"C": @5 },
		@"UInt32": @4294967295u,
		@"UInt64": @18446744073709551615LLU,
	};
}

#define TEST_TYPE_N(t, n) \
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString(#n), value)); \
	SEOUL_UNITTESTING_ASSERT_EQUAL(DataNode::k##t, value.GetType())
#define TEST_TYPE(t) TEST_TYPE_N(t, t)

void TestTypes(const DataStore& dataStore)
{
	auto const root = dataStore.GetRootNode();
	DataNode value;
	
	TEST_TYPE(Array);
	TEST_TYPE_N(Boolean, BoolFalse);
	TEST_TYPE_N(Boolean, BoolTrue);
	TEST_TYPE(Float31);
	TEST_TYPE(Float32);
	TEST_TYPE(Int32Small);
	TEST_TYPE(Int32Big);
	TEST_TYPE(Int64);
	TEST_TYPE(String);
	TEST_TYPE(Table);
	TEST_TYPE(UInt32);
	TEST_TYPE(UInt64);
}

#undef TEST_TYPE
#undef TEST_TYPE_N

} // namespace BasicTest

void IOSUtilTest::TestConvertToDataStore()
{
	using namespace BasicTest;

	NSDictionary* dict = GetDictionary();

	DataStore dataStore;
	SEOUL_UNITTESTING_ASSERT(ConvertToDataStore(dict, dataStore));
	TestTypes(dataStore);

	DataStore expected;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(GetString(), expected));
	TestTypes(dataStore);
	
	SEOUL_UNITTESTING_ASSERT(DataStore::Equals(
		expected,
		expected.GetRootNode(),
		dataStore,
		dataStore.GetRootNode()));
}

void IOSUtilTest::TestConvertToNSDictionary()
{
	using namespace BasicTest;

	DataStore input;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(GetString(), input));
	TestTypes(input);

	NSDictionary* dict = ConvertToNSDictionary(input);
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nil, dict);

	NSDictionary* expected = GetDictionary();
	SEOUL_UNITTESTING_ASSERT([expected isEqual: dict]);
}

namespace ArrayTest
{

static Byte const* GetString()
{
	return
R"__DeLiM__(
{
	"Array": [
		[ "One", "Two", "Three" ],
		false,
		true,
		0.99,
		1.666,
		1,
		67108864,
		9223372036854775807,
		"Hello World",
		{ "A": "B", "C": 5 },
		4294967295,
		18446744073709551615,
	]
}
)__DeLiM__";
}

static NSDictionary* GetDictionary() SEOUL_RETURNS_AUTORELEASED
{
	return @{
		@"Array": @[
			@[ @"One", @"Two", @"Three" ],
			@NO,
			@YES,
			@0.99f,
			@1.666f,
			@1,
			@(1 << 26),
			@9223372036854775807LL,
			@"Hello World",
			@{ @"A": @"B", @"C": @5 },
			@4294967295u,
			@18446744073709551615LLU,
		],
	};
}

#define TEST_TYPE(t) \
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(root, uIndex++, value)); \
	SEOUL_UNITTESTING_ASSERT_EQUAL(DataNode::k##t, value.GetType())

void TestTypes(const DataStore& dataStore)
{
	auto root = dataStore.GetRootNode();
	dataStore.GetValueFromTable(root, HString("Array"), root);
	
	UInt32 uIndex = 0u;
	DataNode value;
	
	TEST_TYPE(Array);
	TEST_TYPE(Boolean);
	TEST_TYPE(Boolean);
	TEST_TYPE(Float31);
	TEST_TYPE(Float32);
	TEST_TYPE(Int32Small);
	TEST_TYPE(Int32Big);
	TEST_TYPE(Int64);
	TEST_TYPE(String);
	TEST_TYPE(Table);
	TEST_TYPE(UInt32);
	TEST_TYPE(UInt64);
}

#undef TEST_TYPE

} // namespace ArrayTest

void IOSUtilTest::TestConvertToDataStoreArrayFull()
{
	using namespace ArrayTest;

	NSDictionary* dict = GetDictionary();

	DataStore dataStore;
	SEOUL_UNITTESTING_ASSERT(ConvertToDataStore(dict, dataStore));
	TestTypes(dataStore);

	DataStore expected;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(GetString(), expected));
	TestTypes(dataStore);
	
	SEOUL_UNITTESTING_ASSERT(DataStore::Equals(
		expected,
		expected.GetRootNode(),
		dataStore,
		dataStore.GetRootNode()));
}

void IOSUtilTest::TestConvertToNSDictionaryArrayFull()
{
	using namespace ArrayTest;

	DataStore input;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(GetString(), input));
	TestTypes(input);

	NSDictionary* dict = ConvertToNSDictionary(input);
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nil, dict);

	NSDictionary* expected = GetDictionary();
	SEOUL_UNITTESTING_ASSERT([expected isEqual: dict]);
}

void IOSUtilTest::TestToDataStoreFails()
{
	// nil value filter.
	DataStore dataStore;
	SEOUL_UNITTESTING_ASSERT(!ConvertToDataStore(nil, dataStore));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetRootNode().IsNull());
	
	NSURL* pURL = [NSURL fileURLWithPath:@"file://test"];
	SEOUL_UNITTESTING_ASSERT(!ConvertToDataStore(@{@"A": pURL}, dataStore));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetRootNode().IsNull());
}

namespace NumberKeyTest
{

static Byte const* GetString()
{
	return
R"__DeLiM__(
{
	"0": [ "One", "Two", "Three" ],
	"1": false,
	"2": true,
	"3": 0.99,
	"4": 1.666,
	"5": 1,
	"6": 67108864,
	"7": 9223372036854775807,
	"8": "Hello World",
	"9": { "A": "B", "C": 5 },
	"10": 4294967295,
	"11": 18446744073709551615,
}
)__DeLiM__";
}

static NSDictionary* GetDictionary() SEOUL_RETURNS_AUTORELEASED
{
	return @{
		@0: @[ @"One", @"Two", @"Three" ],
		@1: @NO,
		@2: @YES,
		@3: @0.99f,
		@4: @1.666f,
		@5: @1,
		@6: @(1 << 26),
		@7: @9223372036854775807LL,
		@8: @"Hello World",
		@9: @{ @"A": @"B", @"C": @5 },
		@10: @4294967295u,
		@11: @18446744073709551615LLU,
	};
}

static NSDictionary* GetExpectedDictionary() SEOUL_RETURNS_AUTORELEASED
{
	return @{
		@"0": @[ @"One", @"Two", @"Three" ],
		@"1": @NO,
		@"2": @YES,
		@"3": @0.99f,
		@"4": @1.666f,
		@"5": @1,
		@"6": @(1 << 26),
		@"7": @9223372036854775807LL,
		@"8": @"Hello World",
		@"9": @{ @"A": @"B", @"C": @5 },
		@"10": @4294967295u,
		@"11": @18446744073709551615LLU,
	};
}

#define TEST_TYPE(t) \
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString(String::Printf("%u", uIndex++)), value)); \
	SEOUL_UNITTESTING_ASSERT_EQUAL(DataNode::k##t, value.GetType())

void TestTypes(const DataStore& dataStore)
{
	auto const root = dataStore.GetRootNode();

	DataNode value;
	UInt32 uIndex = 0u;
	
	TEST_TYPE(Array);
	TEST_TYPE(Boolean);
	TEST_TYPE(Boolean);
	TEST_TYPE(Float31);
	TEST_TYPE(Float32);
	TEST_TYPE(Int32Small);
	TEST_TYPE(Int32Big);
	TEST_TYPE(Int64);
	TEST_TYPE(String);
	TEST_TYPE(Table);
	TEST_TYPE(UInt32);
	TEST_TYPE(UInt64);
}

#undef TEST_TYPE

} // namespace NumberKeyTest

void IOSUtilTest::TestConvertToDataStoreNumberKeys()
{
	using namespace NumberKeyTest;

	NSDictionary* dict = GetDictionary();

	DataStore dataStore;
	SEOUL_UNITTESTING_ASSERT(ConvertToDataStore(dict, dataStore));
	TestTypes(dataStore);

	DataStore expected;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(GetString(), expected));
	TestTypes(dataStore);
	
	SEOUL_UNITTESTING_ASSERT(DataStore::Equals(
		expected,
		expected.GetRootNode(),
		dataStore,
		dataStore.GetRootNode()));
}

void IOSUtilTest::TestConvertToNSDictionaryNumberKeys()
{
	using namespace NumberKeyTest;

	DataStore input;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(GetString(), input));
	TestTypes(input);

	NSDictionary* dict = ConvertToNSDictionary(input);
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nil, dict);

	NSDictionary* expected = GetExpectedDictionary();
	SEOUL_UNITTESTING_ASSERT([expected isEqual: dict]);
}

#endif // /#if SEOUL_UNIT_TESTS

} // namespace Seoul
