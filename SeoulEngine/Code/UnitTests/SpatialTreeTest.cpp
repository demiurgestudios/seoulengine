/**
 * \file SpatialTreeTest.cpp
 * \brief Unit test header file for the Seoul SpatialTree class
 * This file contains the unit test declarations for the Seoul SpatialTree class
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Logger.h"
#include "Frustum.h"
#include "SpatialTree.h"
#include "SpatialTreeTest.h"
#include "ReflectionDefine.h"
#include "UnitTesting.h"
#include <ctime>

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(SpatialTreeTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestDefaultState)
	SEOUL_METHOD(TestAddRemoveUpdateEmptyTree)
	SEOUL_METHOD(TestBuildAndQuery)
SEOUL_END_TYPE()

// Helper functions
static const Int32 kiMax = 0x7FFF;
static Float GetSignedRand()
{
	return (Float)((Int32)GlobalRandom::UniformRandomUInt32n(kiMax+1) - (kiMax / 2));
}

static Float GetUnsignedRand()
{
	return (Float)((Int32)GlobalRandom::UniformRandomUInt32n(kiMax+1));
}

static AABB GetRandomAABB()
{
	const Vector3D min(GetSignedRand(), GetSignedRand(), GetSignedRand());
	const Vector3D max(
		min.X + GetUnsignedRand(),
		min.Y + GetUnsignedRand(),
		min.Z + GetUnsignedRand());

	return AABB::CreateFromMinAndMax(min, max);
}

static Frustum GetRandomFrustum()
{
	return Frustum::CreateFromViewProjection(
		Matrix4D::CreateOrthographic(
		-GlobalRandom::UniformRandomFloat32(),
		GlobalRandom::UniformRandomFloat32(),
		-GlobalRandom::UniformRandomFloat32(),
		GlobalRandom::UniformRandomFloat32(),
		GlobalRandom::UniformRandomFloat32(),
		1000.0f),
		Matrix4D::Identity());
}
// /Helper functions

void SpatialTreeTest::TestDefaultState()
{
	SpatialTree<UInt32> tree;

	// Test that the default SpatialTree is in a state we expect.
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, tree.ComputeFreeNodeCount());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, tree.GetNodeCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(AABB::MaxAABB(), tree.GetRootAABB());
}

static UInt32 s_uObjectQueriedCount = 0u;

static Bool TestCountFunction(UInt32 uObject)
{
	s_uObjectQueriedCount++;
	return true;
}

static Bool AddRemoveUpdateTestFoundFunction(UInt32 uObject)
{
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, uObject);
	s_uObjectQueriedCount++;
	return true;
}

static Bool AddRemoveUpdateTestNotFoundFunction(UInt32 uObject)
{
	SEOUL_UNITTESTING_FAIL("Object was not correctly removed.");
	s_uObjectQueriedCount++;
	return true;
}

void SpatialTreeTest::TestAddRemoveUpdateEmptyTree()
{
	AABB oldAABB = GetRandomAABB();
	AABB newAABB = GetRandomAABB();

	SpatialTree<UInt32> tree;
	SpatialId const uNode = tree.Add(0u, oldAABB);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1u, tree.GetNodeCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, tree.ComputeFreeNodeCount());

	// Test that add succeeded
	s_uObjectQueriedCount = 0;
	tree.Query(AddRemoveUpdateTestFoundFunction, oldAABB);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1u, s_uObjectQueriedCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, tree.ComputeFreeNodeCount());

	tree.Update(0, newAABB);

	// Test that update succeeded
	s_uObjectQueriedCount = 0;
	tree.Query(AddRemoveUpdateTestFoundFunction, newAABB);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1u, s_uObjectQueriedCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, tree.ComputeFreeNodeCount());

	tree.Remove(uNode);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1u, tree.ComputeFreeNodeCount());

	// Test that remove succeeded
	s_uObjectQueriedCount = 0;
	tree.Query(AddRemoveUpdateTestNotFoundFunction, newAABB);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, s_uObjectQueriedCount);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1u, tree.ComputeFreeNodeCount());
}

static SpatialId s_TestId = 0u;
static Bool TestForSuccessfulRemoval(SpatialId uObject)
{
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(s_TestId, uObject);
	s_uObjectQueriedCount++;
	return true;
}

struct SpatialTreeTestObjectEntry
{
	SpatialTreeTestObjectEntry()
		: m_AABB()
		, m_uNodeId(kuInvalidSpatialId)
	{
	}

	SpatialTreeTestObjectEntry(const AABB& aabb, SpatialId uNodeId)
		: m_AABB(aabb)
		, m_uNodeId(uNodeId)
	{
	}

	AABB m_AABB;
	SpatialId m_uNodeId;
};

void SpatialTreeTest::TestBuildAndQuery()
{
	static const UInt32 objectCount = 100u;
	static const UInt32 addRemoveCount = 50u;
	typedef SpatialTree<UInt32> TestTree;
	TestTree tree;

	typedef Vector<SpatialTreeTestObjectEntry, MemoryBudgets::SpatialSorting> Objects;
	Objects vObjects(objectCount);
	AABB totalAABB = AABB::InverseMaxAABB();
	for (UInt32 i = 0u; i < objectCount; ++i)
	{
		SpatialTreeTestObjectEntry& r = vObjects[i];
		r.m_AABB = GetRandomAABB();
		totalAABB = AABB::CalculateMerged(r.m_AABB, totalAABB);
	}

	for (UInt32 i = 0u; i < vObjects.GetSize(); ++i)
	{
		SpatialTreeTestObjectEntry& r = vObjects[i];
		r.m_uNodeId = tree.Add((SpatialId)i, vObjects[i].m_AABB);
	}

	// Should be equal.
	SEOUL_UNITTESTING_ASSERT_EQUAL(totalAABB, tree.GetRootAABB());

	// Test that the built tree is in a state we expect.
	s_uObjectQueriedCount = 0;
	tree.Query(TestCountFunction, AABB::MaxAABB());
	SEOUL_UNITTESTING_ASSERT_EQUAL(objectCount, s_uObjectQueriedCount);

	s_uObjectQueriedCount = 0;
	tree.Query(TestCountFunction, AABB::InverseMaxAABB());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, s_uObjectQueriedCount);

	{
		// Do a query test with a random test AABB, make sure
		// the # of objects returned by the SpatialTree query method
		// is equal to a brute force check.
		AABB testAABB = GetRandomAABB();
		UInt32 testCount = 0u;
		for (UInt32 i = 0u; i < objectCount; ++i)
		{
			testCount += (vObjects[i].m_AABB.Intersects(testAABB)) ? 1u : 0u;
		}

		s_uObjectQueriedCount = 0;
		tree.Query(TestCountFunction, testAABB);
		SEOUL_UNITTESTING_ASSERT_EQUAL(testCount, s_uObjectQueriedCount);
	}

	{
		// Do a query test with a random test Frustum, make sure
		// the # of objects returned by the SpatialTree query method
		// is equal to a brute force check.
		Frustum testFrustum = GetRandomFrustum();
		UInt32 testCount = 0u;
		for (UInt32 i = 0u; i < objectCount; ++i)
		{
			testCount += (FrustumTestResult::kDisjoint != testFrustum.Intersects(vObjects[i].m_AABB)) ? 1u : 0u;
		}

		s_uObjectQueriedCount = 0;
		tree.Query(TestCountFunction, testFrustum);
		SEOUL_UNITTESTING_ASSERT_EQUAL(testCount, s_uObjectQueriedCount);
	}

	// Finally, add, update, remove a bunch of objects and make sure
	// the tree remains consistent and that removes succeed.
	s_TestId = objectCount;
	UInt32 expectedCapacity = (tree.GetNodeCapacity() + 2);
	for (UInt32 i = 0u; i < addRemoveCount; ++i)
	{
		s_uObjectQueriedCount = 0;
		tree.Query(TestCountFunction, AABB::MaxAABB());
		UInt32 startingCount = s_uObjectQueriedCount;

		AABB testAABB = GetRandomAABB();
		AABB newTestAABB = GetRandomAABB();

		SpatialId const uNodeIndex = tree.Add(s_TestId, testAABB);
		tree.Update(uNodeIndex, newTestAABB);
		tree.Remove(uNodeIndex);

		s_uObjectQueriedCount = 0;
		tree.Query(TestForSuccessfulRemoval, AABB::MaxAABB());
		SEOUL_UNITTESTING_ASSERT_EQUAL(startingCount, s_uObjectQueriedCount);
		SEOUL_UNITTESTING_ASSERT_EQUAL(expectedCapacity, tree.GetNodeCapacity());
	}

	// Remove all objects, then check free node.
	for (Int32 i = (Int32)objectCount - 1; i >= 0; --i)
	{
		tree.Remove((SpatialId)vObjects[i].m_uNodeId);
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(tree.GetNodeCapacity(), tree.ComputeFreeNodeCount());
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
