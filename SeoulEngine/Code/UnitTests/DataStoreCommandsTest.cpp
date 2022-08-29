/**
 * \file DataStoreCommandsTest.cpp
 * \brief Unit test header file for DataStore
 * commands functionality.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DataStoreCommandsTest.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

// Leave defined, needed for test below.
static const HString kNotSetExists("$not-set-exists");

SEOUL_BEGIN_TYPE(DataStoreCommandsTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestAppendToExistingArrayInArray)
	SEOUL_METHOD(TestAppendToImplicitArrayInArray)
	SEOUL_METHOD(TestBasic)
	SEOUL_METHOD(TestBasic2)
	SEOUL_METHOD(TestErrors)
	SEOUL_METHOD(TestInheritance)
	SEOUL_METHOD(TestMutations)
	SEOUL_METHOD(TestMutationImplicit)
	SEOUL_METHOD(TestOverwriteInArray)
	SEOUL_METHOD(TestOverwriteInTable)
	SEOUL_METHOD(TestSearch)
SEOUL_END_TYPE()

namespace
{

struct Resolver
{
	SEOUL_DELEGATE_TARGET(Resolver);

	Byte const* m_s;

	SharedPtr<DataStore> Resolve(const String& sFileName, Bool bResolveCommands)
	{
		if (sFileName != "a.json") { return SharedPtr<DataStore>(); }

		DataStore ds;
		if (!DataStoreParser::FromString(m_s, ds)) { return SharedPtr<DataStore>(); }
		if (bResolveCommands && DataStoreParser::IsJsonCommandFile(ds))
		{
			if (!DataStoreParser::ResolveCommandFile(
				SEOUL_BIND_DELEGATE(&Resolver::Resolve, this),
				"a.json",
				ds,
				ds))
			{
				return SharedPtr<DataStore>();
			}
		}

		SharedPtr<DataStore> p(SEOUL_NEW(MemoryBudgets::TBD) DataStore);
		p->Swap(ds);
		return p;
	}
};

void TestEqual(const DataStore& a, Byte const* b)
{
	DataStore dsb;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(b, dsb));
	SEOUL_UNITTESTING_ASSERT(DataStore::Equals(a, a.GetRootNode(), dsb, dsb.GetRootNode()));
}

}

void DataStoreCommandsTest::TestAppendToExistingArrayInArray()
{
	auto const a = "";
	auto const b = R"(
		[
			["$set", "a", 0, 0, true],
			["$append", "a", 0, false]
		]
	)";

	DataStore ds;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(b, ds));

	SEOUL_UNITTESTING_ASSERT(DataStoreParser::IsJsonCommandFile(ds));
	Resolver resolver;
	resolver.m_s = a;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::ResolveCommandFile(
		SEOUL_BIND_DELEGATE(&Resolver::Resolve, &resolver),
		"b.json",
		ds,
		ds));

	TestEqual(ds, R"({"a": [[true, false]]})");
}

void DataStoreCommandsTest::TestAppendToImplicitArrayInArray()
{
	auto const a = "";
	auto const b = R"(
		[
			["$append", "a", 0, true],
			["$append", "a", 0, false]
		]
	)";

	DataStore ds;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(b, ds));

	SEOUL_UNITTESTING_ASSERT(DataStoreParser::IsJsonCommandFile(ds));
	Resolver resolver;
	resolver.m_s = a;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::ResolveCommandFile(
		SEOUL_BIND_DELEGATE(&Resolver::Resolve, &resolver),
		"b.json",
		ds,
		ds));

	TestEqual(ds, R"({"a": [[true, false]]})");
}

void DataStoreCommandsTest::TestBasic()
{
	auto const a = R"(
		[
			["$set", "a", false],
			["$object", "b"],
			["$set", "a", true]
		]
	)";
	auto const b = R"(
		[
			["$include", "a.json"],
			["$set", "a", 0],
			["$set", "b", false]
		]
	)";

	DataStore ds;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(b, ds));

	SEOUL_UNITTESTING_ASSERT(DataStoreParser::IsJsonCommandFile(ds));
	Resolver resolver;
	resolver.m_s = a;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::ResolveCommandFile(
		SEOUL_BIND_DELEGATE(&Resolver::Resolve, &resolver),
		"b.json",
		ds,
		ds));

	TestEqual(ds, 
	R"(
			{
				"a": false,
				"b": {
					"a": 0,
					"b": false
				}
			}
	)");
}

void DataStoreCommandsTest::TestBasic2()
{
	auto const a = R"(
		[
			["$set", "a", false],
			["$object", "b"],
			["$set", "a", true]
		]
	)";
	auto const b = R"(
		[
			["$include", "a.json"],
			["$object", "b"],
			["$set", "a", 0],
			["$set", "b", false]
		]
	)";

	DataStore ds;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(b, ds));

	SEOUL_UNITTESTING_ASSERT(DataStoreParser::IsJsonCommandFile(ds));
	Resolver resolver;
	resolver.m_s = a;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::ResolveCommandFile(
		SEOUL_BIND_DELEGATE(&Resolver::Resolve, &resolver),
		"b.json",
		ds,
		ds));

	TestEqual(ds,
		R"(
			{
				"a": false,
				"b": {
					"a": 0,
					"b": false
				}
			}
	)");
}

static void TestError(Byte const* s, Byte const* sInclude = "")
{
	DataStore ds;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(s, ds));
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::IsJsonCommandFile(ds));
	
	Resolver resolver;
	resolver.m_s = sInclude;
	SEOUL_UNITTESTING_ASSERT(!DataStoreParser::ResolveCommandFile(
		SEOUL_BIND_DELEGATE(&Resolver::Resolve, &resolver),
		"b.json",
		ds,
		ds));
}

void DataStoreCommandsTest::TestErrors()
{
	// Attempt to search in a non-array.
	TestError(R"(
		[
			["$set", "a", { "b": true }],
			["$set", "a", ["$search", "b", true], {"b": false}]
		]
	)");

	// Array search with too few arguments.
	TestError(R"(
		[
			["$set", "a", [{ "b": true }]],
			["$set", "a", ["$search", "b"], {"b": false}]
		]
	)");
	TestError(R"(
		[
			["$set", "a", [{ "b": true }]],
			["$set", "a", ["$search"], {"b": false}]
		]
	)");

	// Array search, not found.
	TestError(R"(
		[
			["$set", "a", [{ "b": true }]],
			["$set", "a", ["$search", "b", false], {"b": true}]
		]
	)");

	// Mutations, incorrect arguments.
	TestError(R"([["$set", "a", ["Hell World"]], ["$append"]])");
	TestError(R"([["$set", "a", ["Hell World"]], ["$append", "a"]])");
	TestError(R"([["$set", "a", ["Hell World"]], ["$erase"]])");
	TestError(R"([["$set", "a", ["Hell World"]], ["$set"]])");
	TestError(R"([["$set", "a", ["Hell World"]], ["$set", "a"]])");

	// Mutations, path argument is incorrect type.
	TestError(R"([["$append", 0, true]])");
	TestError(R"([["$erase", 0]])");
	TestError(R"([["$set", 0, true]])");

	// Mutation, path part is index, container is table.
	TestError(R"([["$set", "a", {"b": "Hell World"}], ["$append", "a", 0, false]])");
	TestError(R"([["$set", "a", {"b": "Hell World"}], ["$erase", "a", 0]])");
	TestError(R"([["$set", "a", {"b": "Hell World"}], ["$set", "a", 0, false]])");

	// Mutation, path part is key, container is array.
	TestError(R"([["$set", "a", ["Hell World"]], ["$append", "a", "b", false]])");
	TestError(R"([["$set", "a", ["Hell World"]], ["$erase", "a", "b"]])");
	TestError(R"([["$set", "a", ["Hell World"]], ["$set", "a", "b", false]])");

	// Mutation, path part invalid type.
	TestError(R"([["$set", "a", {"b": "Hell World"}], ["$append", "a", 0.5, false]])");
	TestError(R"([["$set", "a", {"b": "Hell World"}], ["$erase", "a", 0.5]])");
	TestError(R"([["$set", "a", {"b": "Hell World"}], ["$set", "a", 0.5, false]])");

	// Unknown command.
	TestError(R"([["$set", "a", false], ["$not-set", "a", true]])");
	TestError(R"([["$set", "a", false], ["$not-set-exists", "a", true]])");

	// Command is not a string.
	TestError(R"([["$set", "a", false], [0.5, "a", true]])");

	// Include, insufficient arguments.
	TestError(R"([["$include"]])");

	// Include, not found, two different cases (needs override and doesn't).
	TestError(R"([["$include", "invalid.json"]])");
	TestError(R"([["$include", "invalid.json"], ["$object", "a"]])");

	// Object, insufficient/incorrect arguments.
	TestError(R"([["$object"]])");
	TestError(R"([["$object", 0]])");
	TestError(R"([["$object", "a", 0]])");

	// Parent does not exist.
	TestError(R"([["$object", "a", "b"]])");

	// Erase of element that doesn't exist.
	TestError(R"([["$set", "a", ["b"]], ["$erase", "a", 2]])");
	TestError(R"([["$set", "a", {"b": true}], ["$erase", "a", "c"]])");

	// Append to a container that is not a table.
	TestError(R"([["$set", "a", "b", "c", true], ["$append", "a", "b", false]])");
	TestError(R"([["$set", "a", 0, "c", true], ["$append", "a", 0, false]])");

	// Nested command list has an error.
	TestError(R"([["$include", "a.json"]])", R"([["$set"]])");

	// Attempt to include a root array.
	TestError(R"([["$include", "a.json"]])", R"([1, 2, 3])");

	// Object reference that exists but is not a table.
	TestError(R"([["$set", "a", [true]], ["$object", "a"]])");

	// Unexpected existing container types.
	TestError(R"([["$set", "a", "b", [true]],      ["$set", "a", "b", "c", "d", false]])");
	TestError(R"([["$set", "a", "b", {"d": true}], ["$set", "a", "b", 0, "d", false]])");
}

void DataStoreCommandsTest::TestInheritance()
{
	auto const a = R"(
		[
			["$object", "b"],
			["$set", "a", true]
		]
	)";
	auto const b = R"(
		[
			["$include", "a.json"],
			["$object", "a", "b"]
		]
	)";

	DataStore ds;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(b, ds));

	SEOUL_UNITTESTING_ASSERT(DataStoreParser::IsJsonCommandFile(ds));
	Resolver resolver;
	resolver.m_s = a;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::ResolveCommandFile(
		SEOUL_BIND_DELEGATE(&Resolver::Resolve, &resolver),
		"b.json",
		ds,
		ds));

	TestEqual(ds,
		R"(
			{
				"a": { "a": true },
				"b": { "a": true }
			}
	)");
}

void DataStoreCommandsTest::TestMutations()
{
	auto const a = R"(
		[
			["$set", "a", false],
			["$object", "b"],
			["$set", "a", true]
		]
	)";
	auto const b = R"(
		[
			["$include", "a.json"],
			["$erase", "a"],
			["$set", "b", false],
			["$append", "c", 1],
			["$append", "c", 2],
			["$append", "c", 3]
		]
	)";

	DataStore ds;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(b, ds));

	SEOUL_UNITTESTING_ASSERT(DataStoreParser::IsJsonCommandFile(ds));
	Resolver resolver;
	resolver.m_s = a;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::ResolveCommandFile(
		SEOUL_BIND_DELEGATE(&Resolver::Resolve, &resolver),
		"b.json",
		ds,
		ds));

	TestEqual(ds,
		R"(
			{
				"a": false,
				"b": {
					"b": false,
					"c": [1, 2, 3]
				}
			}
	)");
}

void DataStoreCommandsTest::TestMutationImplicit()
{
	auto const a = "";
	auto const b = R"(
		[
			["$set", "a", 0, "b", 1, "c", 2, 0, "d", "e", true]
		]
	)";

	DataStore ds;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(b, ds));

	SEOUL_UNITTESTING_ASSERT(DataStoreParser::IsJsonCommandFile(ds));
	Resolver resolver;
	resolver.m_s = a;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::ResolveCommandFile(
		SEOUL_BIND_DELEGATE(&Resolver::Resolve, &resolver),
		"b.json",
		ds,
		ds));

	TestEqual(ds, R"({"a": [{"b": [null, {"c": [null, null, [{"d": {"e": true}}]]}]}]})");
}

void DataStoreCommandsTest::TestOverwriteInArray()
{
	auto const a = "";
	auto const b = R"(
		[
			["$set", "a", 0, { "a": false }],
			["$set", "a", 0, { "b": true }]
		]
	)";

	DataStore ds;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(b, ds));

	SEOUL_UNITTESTING_ASSERT(DataStoreParser::IsJsonCommandFile(ds));
	Resolver resolver;
	resolver.m_s = a;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::ResolveCommandFile(
		SEOUL_BIND_DELEGATE(&Resolver::Resolve, &resolver),
		"b.json",
		ds,
		ds));

	TestEqual(ds, R"({"a": [{"b": true}]})");
}

void DataStoreCommandsTest::TestOverwriteInTable()
{
	auto const a = "";
	auto const b = R"(
		[
			["$set", "a", { "a": false }],
			["$set", "a", { "b": true }]
		]
	)";

	DataStore ds;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(b, ds));

	SEOUL_UNITTESTING_ASSERT(DataStoreParser::IsJsonCommandFile(ds));
	Resolver resolver;
	resolver.m_s = a;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::ResolveCommandFile(
		SEOUL_BIND_DELEGATE(&Resolver::Resolve, &resolver),
		"b.json",
		ds,
		ds));

	TestEqual(ds, R"({"a": {"b": true}})");
}

void DataStoreCommandsTest::TestSearch()
{
	auto const a = R"(
		[
			["$append", "c", {"id": 1, "value": "a"}],
			["$append", "c", {"id": 2, "value": "b"}],
			["$append", "c", {"id": 3, "value": "c"}]
		]
	)";
	auto const b = R"(
		[
			["$include", "a.json"],
			["$set", "c", ["$search", "id", 3], "value", 3],
			["$set", "c", ["$search", "id", 2], "value", 2],
			["$set", "c", ["$search", "id", 1], "value", 1]
		]
	)";

	DataStore ds;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(b, ds));

	SEOUL_UNITTESTING_ASSERT(DataStoreParser::IsJsonCommandFile(ds));
	Resolver resolver;
	resolver.m_s = a;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::ResolveCommandFile(
		SEOUL_BIND_DELEGATE(&Resolver::Resolve, &resolver),
		"b.json",
		ds,
		ds));

	TestEqual(ds,
		R"(
			{
				"c": [
					{ "id": 1, "value": 1},
					{ "id": 2, "value": 2},
					{ "id": 3, "value": 3}
				]
			}
		)");
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
