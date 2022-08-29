/**
* \file NamedTypeTest.cpp
* \brief Unit test code for the Seoul NamedType class
*
* Copyright (c) Demiurge Studios, Inc.
* 
* This source code is licensed under the MIT license.
* Full license details can be found in the LICENSE file
* in the root directory of this source tree.
*/

#include "Atomic32.h"
#include "Core.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "UnitTesting.h"
#include "NamedType.h"
#include "NamedTypeTest.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(NamedTypeTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestOperators)
SEOUL_END_TYPE()

// this is how you make a NamedType
struct TestNamedTypeIntTag {};
using TestNamedTypeInt = NamedType<Int, TestNamedTypeIntTag>;

struct TestNamedTypeIntOtherTag {};
using TestNamedTypeOtherInt = NamedType<Int, TestNamedTypeIntOtherTag>;

struct TestNamedTypeIntComparableTag {};
using TestNamedTypeComparableInt = NamedType<Int, TestNamedTypeIntComparableTag>;

Bool operator==(const TestNamedTypeInt& a, const TestNamedTypeComparableInt& b)
{
	return (Int)a == (Int)b;
}

// use SFINAE to check for absence of operators
// (Subsitution Failure Is Not An Error)
namespace CheckOperator
{
	struct NoEquality
	{
		// this is only here to give a different sizeof than Bool
		Bool b[2];
	};

	// this won't be compiled for T, Arg if the operator== is already defined
	// for these types because you can't have two functions with the same parameters
	// that return different values
	template<typename T, typename Arg>
	NoEquality operator== (const T&, const Arg&);

	// varargs used here because it's the lowest priority function
	// chosen for precedence, so if the arguments satisfy NoEquality
	// it won't call the function that returns Bool
	Bool CheckEquality(...);
	NoEquality& CheckEquality(const NoEquality&);

	template <typename T, typename Arg = T>
	constexpr Bool EqualityExists()
	{
		// *(T*)(0) constructs a null pointer of the type and dereferences it
		// this is okay because sizeof is a compile time type evaluation only
		// and does not execute the expression
		// and nullpointer dereference is a runtime failure only
		// this is also why you do not need to actually define CheckEquality
		return sizeof(CheckEquality(*(T*)(0) == *(Arg*)(0)))
			!= sizeof(NoEquality);
	}
}

/**
* Test the basic functionality of the List class
*/
void NamedTypeTest::TestOperators()
{
	// constructor from underlying type
	TestNamedTypeInt testSpecificInt(1);
	SEOUL_UNITTESTING_ASSERT(testSpecificInt == TestNamedTypeInt(1));

	// default constructor
	// should zero initialize underlying type
	TestNamedTypeInt anotherSpecificInt;
	SEOUL_UNITTESTING_ASSERT(anotherSpecificInt == TestNamedTypeInt(0));

	// operator !=
	SEOUL_UNITTESTING_ASSERT(testSpecificInt != TestNamedTypeInt(0));

	// assignment operator
	testSpecificInt = TestNamedTypeInt(2);
	SEOUL_UNITTESTING_ASSERT(testSpecificInt == TestNamedTypeInt(2));

	// cast
	SEOUL_UNITTESTING_ASSERT(testSpecificInt == (TestNamedTypeInt)2);

	// cast back
	SEOUL_UNITTESTING_ASSERT((Int)testSpecificInt == 2);

	// lack of equality operators between different NamedTypes
	SEOUL_UNITTESTING_ASSERT(!(CheckOperator::EqualityExists<TestNamedTypeInt, TestNamedTypeOtherInt>()));

	// lack of equality operator between NamedType and underlying type
	SEOUL_UNITTESTING_ASSERT(!(CheckOperator::EqualityExists<TestNamedTypeInt, Int>()));

	// equality operator exists for the same NamedType
	SEOUL_UNITTESTING_ASSERT((CheckOperator::EqualityExists<TestNamedTypeInt, TestNamedTypeInt>()));

	// equality operator exists between NamedTypes when it is explicitly defined
	SEOUL_UNITTESTING_ASSERT((CheckOperator::EqualityExists<TestNamedTypeInt, TestNamedTypeComparableInt>()));
}

#endif // /#if SEOUL_UNIT_TESTS

} // namespace Seoul
