/**
 * \file NavigationTest.cpp
 * \brief Navigation unit test .
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Logger.h"
#include "NavigationTest.h"
#include "NavigationTestData.h"
#include "NavigationCoverageRasterizer.h"
#include "NavigationGrid.h"
#include "NavigationQuery.h"
#include "NavigationQueryState.h"
#include "ReflectionDefine.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

#if SEOUL_WITH_NAVIGATION

SEOUL_BEGIN_TYPE(NavigationTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestConnectedLargeData)
	SEOUL_METHOD(TestConnectedRandom)
	SEOUL_METHOD(TestCoverageRasterizerBasic)
	SEOUL_METHOD(TestFindNearestBasic)
	SEOUL_METHOD(TestFindNearestLarge)
	SEOUL_METHOD(TestFindNearestLargeData)
	SEOUL_METHOD(TestFindNearestConnectedBasic)
	SEOUL_METHOD(TestFindNearestConnectedLarge)
	SEOUL_METHOD(TestFindNearestConnectedLargeData)
	SEOUL_METHOD(TestFindPathBasic)
	SEOUL_METHOD(TestFindStraightPathBasic)
	SEOUL_METHOD(TestGridBasic)
	SEOUL_METHOD(TestRayTestBasic)
	SEOUL_METHOD(TestRobustFindStraightPathBasic)
SEOUL_END_TYPE()

static void SetAndTestCell(Navigation::Grid& grid, UInt32 uX, UInt32 uY, UInt8 uCell)
{
	grid.SetCell(uX, uY, uCell);
	SEOUL_UNITTESTING_ASSERT_EQUAL(uCell, grid.GetCell(uX, uY));
}

static void SetAndTestGrid(Navigation::Grid& grid, UInt8 const* p)
{
	UInt32 const uWidth = grid.GetWidth();
	UInt32 const uHeight = grid.GetHeight();
	UInt32 uIndex = 0u;
	for (UInt32 uY = 0u; uY < uHeight; ++uY)
	{
		for (UInt32 uX = 0u; uX < uWidth; ++uX)
		{
			SetAndTestCell(grid, uX, uY, p[uIndex++]);
		}
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(grid.GetGrid(), p, (uWidth * uHeight * sizeof(UInt8))));
}

void NavigationTest::TestConnectedLargeData()
{
	// Since each test is slow, we perform the check N times.
	static const Int32 kiIterations = 500;

	Vector<Byte> v;
	SEOUL_UNITTESTING_ASSERT(Base64Decode(s_kNavigationTestDataLarge, v));
	Navigation::Grid* pGrid = Navigation::Grid::CreateFromFileInMemory((UInt8 const*)v.Get(0u), v.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT(nullptr != pGrid);
	{
		Navigation::Query query(*pGrid, 0u, 1u, 0u);
		Navigation::Query queryNoConnectivity(*pGrid, Navigation::QueryConfig::kDisableConnectivity, 1u, 0u);
		Navigation::QueryState state;

		UInt32 const uWidth = pGrid->GetWidth();
		UInt32 const uHeight = pGrid->GetHeight();

		// Test connectivity basic info.
		for (UInt32 uY = 0u; uY < uHeight; ++uY)
		{
			for (UInt32 uX = 0u; uX < uWidth; ++uX)
			{
				SEOUL_UNITTESTING_ASSERT(!queryNoConnectivity.IsConnected(
					state,
					Navigation::Position(uX, uY),
					Navigation::Position(uX, uY)));
				if (query.IsPassable(Navigation::Position(uX, uY)))
				{
					SEOUL_UNITTESTING_ASSERT(query.IsConnected(
						state,
						Navigation::Position(uX, uY),
						Navigation::Position(uX, uY)));
				}
				else
				{
					SEOUL_UNITTESTING_ASSERT(!query.IsConnected(
						state,
						Navigation::Position(uX, uY),
						Navigation::Position(uX, uY)));
				}
			}
		}

		// Test path vs. connectivity.
		for (Int32 iIteration = 0; iIteration < kiIterations; ++iIteration)
		{
			UInt32 uX0 = (UInt32)(GlobalRandom::UniformRandomFloat32() * (Float32)(uWidth - 1u));
			UInt32 uY0 = (UInt32)(GlobalRandom::UniformRandomFloat32() * (Float32)(uHeight - 1u));
			UInt32 uX1 = (UInt32)(GlobalRandom::UniformRandomFloat32() * (Float32)(uWidth - 1u));
			UInt32 uY1 = (UInt32)(GlobalRandom::UniformRandomFloat32() * (Float32)(uHeight - 1u));
			Bool const bFindPath = queryNoConnectivity.FindPath(
				state,
				Navigation::Position(uX0, uY0),
				Navigation::Position(uX1, uY1));

			if (bFindPath)
			{
				SEOUL_UNITTESTING_ASSERT(query.IsConnected(
					state,
					Navigation::Position(uX0, uY0),
					Navigation::Position(uX1, uY1)));
				SEOUL_UNITTESTING_ASSERT(query.IsPassable(Navigation::Position(uX0, uY0)));
				SEOUL_UNITTESTING_ASSERT(query.IsPassable(Navigation::Position(uX1, uY1)));
				for (UInt32 i = 0u; i < state.m_vWaypoints.GetSize(); ++i)
				{
					if (0u != i)
					{
						Navigation::Position unusedHit;
						SEOUL_UNITTESTING_ASSERT(!query.RayTest(
							state,
							state.m_vWaypoints[i - 1],
							state.m_vWaypoints[i],
							true,
							unusedHit));
					}
					SEOUL_UNITTESTING_ASSERT(query.IsPassable(state.m_vWaypoints[i]));
				}
			}
			else
			{
				SEOUL_UNITTESTING_ASSERT(!query.IsConnected(
					state,
					Navigation::Position(uX0, uY0),
					Navigation::Position(uX1, uY1)));
			}

			UInt32 const uFindPathWaypointCount = state.m_vWaypoints.GetSize();
			Bool const bFindStraightPath = queryNoConnectivity.FindPath(
				state,
				Navigation::Position(uX0, uY0),
				Navigation::Position(uX1, uY1));
			SEOUL_UNITTESTING_ASSERT(state.m_vWaypoints.GetSize() <= uFindPathWaypointCount);

			if (bFindStraightPath)
			{
				SEOUL_UNITTESTING_ASSERT(query.IsConnected(
					state,
					Navigation::Position(uX0, uY0),
					Navigation::Position(uX1, uY1)));
				SEOUL_UNITTESTING_ASSERT(query.IsPassable(Navigation::Position(uX0, uY0)));
				SEOUL_UNITTESTING_ASSERT(query.IsPassable(Navigation::Position(uX1, uY1)));
				for (UInt32 i = 0u; i < state.m_vWaypoints.GetSize(); ++i)
				{
					if (0u != i)
					{
						Navigation::Position unusedHit;
						SEOUL_UNITTESTING_ASSERT(!query.RayTest(
							state,
							state.m_vWaypoints[i - 1],
							state.m_vWaypoints[i],
							true,
							unusedHit));
					}
					SEOUL_UNITTESTING_ASSERT(query.IsPassable(state.m_vWaypoints[i]));
				}
			}
			else
			{
				SEOUL_UNITTESTING_ASSERT(!query.IsConnected(
					state,
					Navigation::Position(uX0, uY0),
					Navigation::Position(uX1, uY1)));
			}
		}
	}
	Navigation::Grid::Destroy(pGrid);
}

static void TestRasterizeTriangle(
	Navigation::CoverageRasterizer& r,
	Float32 v0x, Float32 v0y, Float32 v0z,
	Float32 v1x, Float32 v1y, Float32 v1z,
	Float32 v2x, Float32 v2y, Float32 v2z,
	const Vector<UInt32, MemoryBudgets::Navigation>& vExpected)
{
	static const UInt32 kWidth = 8u;
	static const UInt32 kHeight = 4u;

	r.RasterizeTriangle(Vector3D(v0x, v0y, v0z), Vector3D(v1x, v1y, v1z), Vector3D(v2x, v2y, v2z));
	for (UInt32 uY = 0u; uY < kHeight; ++uY)
	{
		for (UInt32 uX = 0u; uX < kWidth; ++uX)
		{
			UInt32 const uSampleCount = r.GetSampleCount(uX, uY);
			UInt32 const uExpected = vExpected[uY * kWidth + uX];
			SEOUL_UNITTESTING_ASSERT_EQUAL(uExpected, uSampleCount);
		}
	}
}

void NavigationTest::TestCoverageRasterizerBasic()
{
	static const UInt32 kWidth = 8u;
	static const UInt32 kHeight = 4u;
	static const UInt32 kWidthPixels = (kWidth * Navigation::CoverageRasterizer::kiRasterRes);
	static const UInt32 kHeightPixels = (kHeight * Navigation::CoverageRasterizer::kiRasterRes);
	static const Float32 kHeightValue = 5.0f;

	Vector<Float32, MemoryBudgets::Navigation> vHeightValues(kWidthPixels * kHeightPixels, 0.0f);
	for (UInt32 uY = 0u; uY < kHeightPixels; ++uY)
	{
		for (UInt32 uX = 0u; uX < kWidthPixels; ++uX)
		{
			UInt32 const uIndex = (uY * kWidthPixels + uX);
			if (uX < 16u && uY < 8u)
			{
				vHeightValues[uIndex] = kHeightValue;
			}
			else
			{
				vHeightValues[uIndex] = 0.0f;
			}
		}
	}

	Navigation::CoverageRasterizer r(kWidth, kHeight, Vector3D(3, 5, 7), vHeightValues.Get(0u));

	// Test that sample counts are all zero.
	for (UInt32 uY = 0u; uY < kHeight; ++uY)
	{
		for (UInt32 uX = 0u; uX < kWidth; ++uX)
		{
			UInt32 const uCount = r.GetSampleCount(uX, uY);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0u, uCount);
		}
	}

	// Setup expected counts. Initially 0.
	Vector<UInt32, MemoryBudgets::Navigation> vExpected(kWidth * kHeight, 0u);

	// Rasterize some triangles. These should have no affect on sample counts.
	TestRasterizeTriangle(r, 3, 5, 7, 4, 5, 7, 3, 6, 7, vExpected); // below height of upper corner.
	TestRasterizeTriangle(r, 3, 5, 12, 3, 5, 12, 3, 6, 12, vExpected); // degenerate.
	TestRasterizeTriangle(r, 3, 5, 12.0f-1e-5f, 4, 5, 12.0f-1e-5f, 3, 6, 12.0f-1e-5f, vExpected); // barely below height of upper corner.

	// Now rasterize expecting specific results.
	vExpected[0] = 10;
	TestRasterizeTriangle(r, 3, 5, 12, 4, 5, 12, 3, 6, 12, vExpected); // at height, should get sample count of 10 at (0, 0) (triangle on upper-left of grid cell (0, 0))
	r.Clear();
	vExpected[0] = 6;
	TestRasterizeTriangle(r, 3, 6, 12, 4, 5, 12, 4, 6, 12, vExpected); // at height, should get sample count of 6 at (0, 0) (triangle on lower-right of grid cell (0, 0))
	vExpected[0] = 16;
	TestRasterizeTriangle(r, 3, 5, 12, 4, 5, 12, 3, 6, 12, vExpected); // at height, should get sample count of 16 at (0, 0) (fill grid cell (0, 0))
	r.Clear();
	vExpected[0] = 6;
	TestRasterizeTriangle(r, 3, 5, 12, 3.5, 5, 12, 3, 6, 12, vExpected); // at height, should get a sample count of 6 at (0, 0) (triangle on upper-left quarter of grid cell (0, 0))
	vExpected[0] = 8;
	TestRasterizeTriangle(r, 3, 6, 12, 3.5, 5, 12, 3.5, 6, 12, vExpected); // at height, should get a sample count of 8 at (0, 0) (quad on left-half of grid cell (0, 0))

}

void NavigationTest::TestConnectedRandom()
{
	static const UInt32 kuWidth = 13;
	static const UInt32 kuHeight = 16;

	Navigation::Grid* pGrid = Navigation::Grid::Create(kuWidth, kuHeight);
	{
		Vector<UInt8> vValues(kuWidth * kuHeight, (UInt8)0);
		for (UInt32 uY = 0u; uY < kuHeight; ++uY)
		{
			for (UInt32 uX = 0u; uX < kuWidth; ++uX)
			{
				UInt8 const uValue = (GlobalRandom::UniformRandomFloat32() >= 0.5f ? 1u : 0u);
				vValues[uY * kuWidth + uX] = uValue;
			}
		}
		SetAndTestGrid(*pGrid, vValues.Get(0u));

		Navigation::Query query(*pGrid, 0u, 1u, 0u);
		Navigation::Query queryNoConnectivity(*pGrid, Navigation::QueryConfig::kDisableConnectivity, 1u, 0u);
		Navigation::QueryState state;

		// Test connectivity basic info.
		for (UInt32 uY = 0u; uY < kuHeight; ++uY)
		{
			for (UInt32 uX = 0u; uX < kuWidth; ++uX)
			{
				SEOUL_UNITTESTING_ASSERT(!queryNoConnectivity.IsConnected(
					state,
					Navigation::Position(uX, uY),
					Navigation::Position(uX, uY)));
				if (query.IsPassable(Navigation::Position(uX, uY)))
				{
					SEOUL_UNITTESTING_ASSERT(query.IsConnected(
						state,
						Navigation::Position(uX, uY),
						Navigation::Position(uX, uY)));
				}
				else
				{
					SEOUL_UNITTESTING_ASSERT(!query.IsConnected(
						state,
						Navigation::Position(uX, uY),
						Navigation::Position(uX, uY)));
				}
			}
		}

		// Test path vs. connectivity.
		for (UInt32 uY0 = 0u; uY0 < kuHeight; ++uY0)
		{
			for (UInt32 uX0 = 0u; uX0 < kuWidth; ++uX0)
			{
				for (UInt32 uY1 = 0u; uY1 < kuHeight; ++uY1)
				{
					for (UInt32 uX1 = 0u; uX1 < kuWidth; ++uX1)
					{
						if (uX1 == uX0 && uY1 == uY0)
						{
							continue;
						}

						Bool const bFindPath = queryNoConnectivity.FindPath(
							state,
							Navigation::Position(uX0, uY0),
							Navigation::Position(uX1, uY1));

						if (bFindPath)
						{
							SEOUL_UNITTESTING_ASSERT(query.IsConnected(
								state,
								Navigation::Position(uX0, uY0),
								Navigation::Position(uX1, uY1)));
							SEOUL_UNITTESTING_ASSERT(query.IsPassable(Navigation::Position(uX0, uY0)));
							SEOUL_UNITTESTING_ASSERT(query.IsPassable(Navigation::Position(uX1, uY1)));
							for (UInt32 i = 0u; i < state.m_vWaypoints.GetSize(); ++i)
							{
								if (0u != i)
								{
									Navigation::Position unusedHit;
									SEOUL_UNITTESTING_ASSERT(!query.RayTest(
										state,
										state.m_vWaypoints[i - 1],
										state.m_vWaypoints[i],
										true,
										unusedHit));
								}
								SEOUL_UNITTESTING_ASSERT(query.IsPassable(state.m_vWaypoints[i]));
							}
						}
						else
						{
							SEOUL_UNITTESTING_ASSERT(!query.IsConnected(
								state,
								Navigation::Position(uX0, uY0),
								Navigation::Position(uX1, uY1)));
						}

						UInt32 const uFindPathWaypointCount = state.m_vWaypoints.GetSize();
						Bool const bFindStraightPath = queryNoConnectivity.FindStraightPath(
							state,
							Navigation::Position(uX0, uY0),
							Navigation::Position(uX1, uY1));
						SEOUL_UNITTESTING_ASSERT(state.m_vWaypoints.GetSize() <= uFindPathWaypointCount);

						if (bFindStraightPath)
						{
							SEOUL_UNITTESTING_ASSERT(query.IsConnected(
								state,
								Navigation::Position(uX0, uY0),
								Navigation::Position(uX1, uY1)));
							SEOUL_UNITTESTING_ASSERT(query.IsPassable(Navigation::Position(uX0, uY0)));
							SEOUL_UNITTESTING_ASSERT(query.IsPassable(Navigation::Position(uX1, uY1)));
							for (UInt32 i = 0u; i < state.m_vWaypoints.GetSize(); ++i)
							{
								if (0u != i)
								{
									Navigation::Position unusedHit;
									SEOUL_UNITTESTING_ASSERT(!query.RayTest(
										state,
										state.m_vWaypoints[i - 1],
										state.m_vWaypoints[i],
										true,
										unusedHit));
								}
								SEOUL_UNITTESTING_ASSERT(query.IsPassable(state.m_vWaypoints[i]));
							}
						}
						else
						{
							SEOUL_UNITTESTING_ASSERT(!query.IsConnected(
								state,
								Navigation::Position(uX0, uY0),
								Navigation::Position(uX1, uY1)));
						}
					}
				}
			}
		}
	}
	Navigation::Grid::Destroy(pGrid);
}

static void TestPath(
	Navigation::QueryState& state,
	const Navigation::Query& query,
	UInt32 uStartX,
	UInt32 uStartY,
	UInt32 uEndX,
	UInt32 uEndY,
	const Navigation::Positions& vExpected,
	Bool bStraightPath)
{
	Bool const bSuccess = (bStraightPath
		? query.FindStraightPath(state, Navigation::Position(uStartX, uStartY), Navigation::Position(uEndX, uEndY))
		: query.FindPath(state, Navigation::Position(uStartX, uStartY), Navigation::Position(uEndX, uEndY)));

	if (vExpected.IsEmpty())
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(false, bSuccess);
	}
	else
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bSuccess);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vExpected.GetSize(), state.m_vWaypoints.GetSize());
		for (UInt32 i = 0u; i < vExpected.GetSize(); ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(vExpected[i].m_uX, state.m_vWaypoints[i].m_uX);
			SEOUL_UNITTESTING_ASSERT_EQUAL(vExpected[i].m_uY, state.m_vWaypoints[i].m_uY);
		}
	}
}

static void TestRobustPath(
	Navigation::QueryState& state,
	const Navigation::Query& query,
	UInt32 uStartX,
	UInt32 uStartY,
	UInt32 uEndX,
	UInt32 uEndY,
	const Navigation::Positions& vExpected,
	UInt32 uMaxStartDistance,
	UInt32 uMaxEndDistance)
{
	Bool const bSuccess = query.RobustFindStraightPath(
		state,
		Navigation::Position(uStartX, uStartY),
		Navigation::Position(uEndX, uEndY),
		uMaxStartDistance,
		uMaxEndDistance);

	if (vExpected.IsEmpty())
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(false, bSuccess);
	}
	else
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bSuccess);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vExpected.GetSize(), state.m_vWaypoints.GetSize());
		for (UInt32 i = 0u; i < vExpected.GetSize(); ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(vExpected[i].m_uX, state.m_vWaypoints[i].m_uX);
			SEOUL_UNITTESTING_ASSERT_EQUAL(vExpected[i].m_uY, state.m_vWaypoints[i].m_uY);
		}
	}
}

static void TestNearest(
	Navigation::QueryState& state,
	const Navigation::Query& query,
	UInt32 uStartX,
	UInt32 uStartY,
	UInt32 uMaxDistance,
	Bool bExpectNearest,
	UInt32 uExpectX,
	UInt32 uExpectY)
{
	// Test nearest.
	{
		Navigation::Position position;
		Bool const bSuccess = query.FindNearest(
			state,
			Navigation::Position(uStartX, uStartY),
			uMaxDistance,
			position);

		SEOUL_UNITTESTING_ASSERT_EQUAL(bExpectNearest, bSuccess);
		if (bExpectNearest)
		{
			SEOUL_UNITTESTING_ASSERT(query.IsPassable(Navigation::Position(uExpectX, uExpectY)));
			SEOUL_UNITTESTING_ASSERT(query.IsPassable(position));
			SEOUL_UNITTESTING_ASSERT_EQUAL(uExpectX, position.m_uX);
			SEOUL_UNITTESTING_ASSERT_EQUAL(uExpectY, position.m_uY);
		}
	}
}

void NavigationTest::TestFindNearestBasic()
{
	Navigation::Grid* pGrid = Navigation::Grid::Create(8u, 4u);

	{
		// Populate
		static UInt8 const aValues[] =
		{
			1, 0, 1, 1, 0, 1, 1, 1,
			0, 0, 0, 1, 1, 0, 1, 1,
			1, 0, 1, 1, 1, 0, 1, 0,
			1, 0, 0, 1, 0, 1, 1, 1,
		};
		SetAndTestGrid(*pGrid, &aValues[0]);

		Navigation::Query query(*pGrid, 0u, 1u, 0u);
		Navigation::QueryState state;

		// Test nearest - we expect the search pattern
		// to be a clockwsie "zig-zag" starting from the top.
		// e.g.
		//     (0, -1), (1, 0), (0,  1), (-1,  0),
		//     (1, -1), (1, 1), (-1, 1), (-1, -1),
		TestNearest(state, query, 0, 0, 100, true, 1, 0);
		TestNearest(state, query, 6, 3, 100, true, 7, 2);
		TestNearest(state, query, 3, 2, 100, true, 4, 3);

		// Self checks.
		TestNearest(state, query, 1, 0, 0, true, 1, 0);
		TestNearest(state, query, 1, 0, 100, true, 1, 0);

		// Near checks.
		TestNearest(state, query, 7, 1, 1, true, 7, 2);
		TestNearest(state, query, 7, 0, 2, true, 7, 2);

		// Failure checks.
		TestNearest(state, query, 0, 0, 0, false, 0, 0);
		TestNearest(state, query, 7, 0, 0, false, 0, 0);
		TestNearest(state, query, 7, 0, 1, false, 0, 0);
	}

	Navigation::Grid::Destroy(pGrid);
}

void NavigationTest::TestFindNearestLarge()
{
	static const UInt32 kuWidth = 372u;
	static const UInt32 kuHeight = 483u;
	static const UInt32 kuSolidX0 = 93u;
	static const UInt32 kuSolidX1 = 279;
	static const UInt32 kuSolidY0 = 120;
	static const UInt32 kuSolidY1 = 362;

	Navigation::Grid* pGrid = Navigation::Grid::Create(kuWidth, kuHeight);
	{
		Vector<UInt8> vValues(kuWidth * kuHeight, (UInt8)0);

		// Large grid with an impassable center.
		for (UInt32 uY = 0u; uY < kuHeight; ++uY)
		{
			for (UInt32 uX = 0u; uX < kuWidth; ++uX)
			{
				UInt32 uValue = 0u;
				if (uX >= kuSolidX0 &&
					uX <= kuSolidX1 &&
					uY >= kuSolidY0 &&
					uY <= kuSolidY1)
				{
					uValue = 1u;
				}
				vValues[uY * kuWidth + uX] = uValue;
			}
		}
		SetAndTestGrid(*pGrid, vValues.Get(0u));

		Navigation::Query query(*pGrid, 0u, 1u, 0u);
		Navigation::QueryState state;

		// Check all impassable cells.
		for (UInt32 uY = 0u; uY < kuHeight; ++uY)
		{
			for (UInt32 uX = 0u; uX < kuWidth; ++uX)
			{
				if (uX >= kuSolidX0 &&
					uX <= kuSolidX1 &&
					uY >= kuSolidY0 &&
					uY <= kuSolidY1)
				{
					SEOUL_UNITTESTING_ASSERT(!query.IsPassable(Navigation::Position(uX, uY)));

					Int32 const iX0 = (Int32)uX - (Int32)kuSolidX0;
					Int32 const iX1 = (Int32)kuSolidX1 - (Int32)uX;
					Int32 const iY0 = (Int32)uY - (Int32)kuSolidY0;
					Int32 const iY1 = (Int32)kuSolidY1 - (Int32)uY;
					Int32 const iMin = Min(iX0, iX1, iY0, iY1);

					UInt32 uExpectedX = 0u;
					UInt32 uExpectedY = 0u;
					if (iY0 == iMin)
					{
						uExpectedX = uX;
						uExpectedY = kuSolidY0 - 1;
					}
					else if (iX1 == iMin)
					{
						uExpectedX = kuSolidX1 + 1;
						uExpectedY = uY;
					}
					else if (iY1 == iMin)
					{
						uExpectedX = uX;
						uExpectedY = kuSolidY1 + 1;
					}
					else if (iX0 == iMin)
					{
						uExpectedX = kuSolidX0 - 1;
						uExpectedY = uY;
					}

					TestNearest(
						state,
						query,
						uX,
						uY,
						Max(kuWidth, kuHeight),
						true,
						uExpectedX,
						uExpectedY);
				}
			}
		}
	}
	Navigation::Grid::Destroy(pGrid);
}

static void TestNearestConnected(
	Navigation::QueryState& state,
	const Navigation::Query& query,
	UInt32 uStartX,
	UInt32 uStartY,
	UInt32 uMaxDistance,
	UInt32 uConnectedX,
	UInt32 uConnectedY,
	Bool bExpectNearest,
	UInt32 uExpectX,
	UInt32 uExpectY)
{
	// Test nearest.
	{
		Navigation::Position position;
		Bool const bSuccess = query.FindNearestConnected(
			state,
			Navigation::Position(uStartX, uStartY),
			uMaxDistance,
			Navigation::Position(uConnectedX, uConnectedY),
			position);

		SEOUL_UNITTESTING_ASSERT_EQUAL(bExpectNearest, bSuccess);
		if (bExpectNearest)
		{
			SEOUL_UNITTESTING_ASSERT(query.IsConnected(state,
				position,
				Navigation::Position(uConnectedX, uConnectedY)));
			SEOUL_UNITTESTING_ASSERT(query.IsPassable(position));
			SEOUL_UNITTESTING_ASSERT(query.IsPassable(Navigation::Position(uConnectedX, uConnectedY)));
			SEOUL_UNITTESTING_ASSERT_EQUAL(uExpectX, position.m_uX);
			SEOUL_UNITTESTING_ASSERT_EQUAL(uExpectY, position.m_uY);
		}
	}
}

void NavigationTest::TestFindNearestLargeData()
{
	// Since each test is slow, we perform the check N times.
	static const Int32 kiIterations = 10000;

	Vector<Byte> v;
	SEOUL_UNITTESTING_ASSERT(Base64Decode(s_kNavigationTestDataLarge, v));
	Navigation::Grid* pGrid = Navigation::Grid::CreateFromFileInMemory((UInt8 const*)v.Get(0u), v.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT(nullptr != pGrid);
	{
		Navigation::Query query(*pGrid, 0u, 1u, 0u);
		Navigation::QueryState state;

		// Test nearest
		for (Int32 iIteration = 0; iIteration < kiIterations; ++iIteration)
		{
			UInt32 uX = (UInt32)(GlobalRandom::UniformRandomFloat32() * (Float32)(pGrid->GetWidth() - 1u));
			UInt32 uY = (UInt32)(GlobalRandom::UniformRandomFloat32() * (Float32)(pGrid->GetHeight() - 1u));

			// Only try to get near impassable cells.
			if (query.IsPassable(Navigation::Position(uX, uY)))
			{
				--iIteration;
				continue;
			}

			Navigation::Position position(UIntMax, UIntMax);
			SEOUL_UNITTESTING_ASSERT(query.FindNearest(
				state,
				Navigation::Position(uX, uY),
				Max(pGrid->GetWidth(), pGrid->GetHeight()),
				position));

			SEOUL_UNITTESTING_ASSERT(position.m_uX < pGrid->GetWidth());
			SEOUL_UNITTESTING_ASSERT(position.m_uY < pGrid->GetHeight());
			SEOUL_UNITTESTING_ASSERT(query.IsPassable(position));
		}
	}
	Navigation::Grid::Destroy(pGrid);
}

void NavigationTest::TestFindNearestConnectedBasic()
{
	Navigation::Grid* pGrid = Navigation::Grid::Create(8u, 4u);

	{
		// Populate
		static UInt8 const aValues[] =
		{
			1, 0, 1, 1, 0, 1, 1, 1,
			0, 0, 0, 0, 1, 0, 1, 1,
			1, 0, 1, 0, 1, 0, 1, 0,
			1, 0, 0, 0, 0, 1, 1, 1,
		};
		SetAndTestGrid(*pGrid, &aValues[0]);

		Navigation::Query query(*pGrid, 0u, 1u, 0u);
		Navigation::QueryState state;

		//
		// Basic checks - very similar or identical to TestNearest.
		//

		// Directionality tests.
		TestNearestConnected(state, query, 2, 2, 100, 2, 1, true, 2, 1);
		TestNearestConnected(state, query, 2, 2, 100, 1, 2, true, 1, 2);
		TestNearestConnected(state, query, 2, 2, 100, 2, 3, true, 2, 3);
		TestNearestConnected(state, query, 2, 2, 100, 3, 2, true, 3, 2);
		// Directionality, corner connectedto - should pick horizontals.
		TestNearestConnected(state, query, 2, 2, 100, 1, 1, true, 2, 1);
		TestNearestConnected(state, query, 2, 2, 100, 3, 1, true, 2, 1);
		TestNearestConnected(state, query, 2, 2, 100, 3, 3, true, 3, 2);
		TestNearestConnected(state, query, 2, 2, 100, 1, 3, true, 2, 3);

		// Test nearest - we expect it to find the nearest
		// point that is closest to the conenctedTo.
		TestNearestConnected(state, query, 0, 0, 100, 0, 1, true, 0, 1);
		TestNearestConnected(state, query, 6, 3, 100, 7, 2, true, 7, 2);
		TestNearestConnected(state, query, 5, 2, 100, 4, 3, true, 4, 3);

		// Self checks.
		TestNearestConnected(state, query, 1, 0, 0, 1, 0, true, 1, 0);
		TestNearestConnected(state, query, 1, 0, 100, 1, 0, true, 1, 0);

		// Near checks.
		TestNearestConnected(state, query, 7, 1, 1, 7, 2, true, 7, 2);
		TestNearestConnected(state, query, 7, 0, 2, 7, 2, true, 7, 2);

		// Failure checks.
		TestNearestConnected(state, query, 0, 0, 0, 0, 0, false, 0, 0);
		TestNearestConnected(state, query, 7, 0, 0, 0, 0, false, 0, 0);
		TestNearestConnected(state, query, 7, 0, 1, 0, 0, false, 0, 0);

		//
		// Explicit connection checks.
		//
		TestNearestConnected(state, query, 0, 0, 100, 5, 2, true, 5, 1);
	}

	Navigation::Grid::Destroy(pGrid);
}

void NavigationTest::TestFindNearestConnectedLarge()
{
	static const UInt32 kuWidth = 372u;
	static const UInt32 kuHeight = 483u;
	static const UInt32 kuSolidX0 = 93u;
	static const UInt32 kuSolidX1 = 279;
	static const UInt32 kuSolidY0 = 120;
	static const UInt32 kuSolidY1 = 362;

	Navigation::Grid* pGrid = Navigation::Grid::Create(kuWidth, kuHeight);
	{
		Vector<UInt8> vValues(kuWidth * kuHeight, (UInt8)0);

		// Large grid with an impassable center.
		for (UInt32 uY = 0u; uY < kuHeight; ++uY)
		{
			for (UInt32 uX = 0u; uX < kuWidth; ++uX)
			{
				UInt32 uValue = 0u;
				if (uX >= kuSolidX0 &&
					uX <= kuSolidX1 &&
					uY >= kuSolidY0 &&
					uY <= kuSolidY1)
				{
					uValue = 1u;
				}
				vValues[uY * kuWidth + uX] = uValue;
			}
		}
		SetAndTestGrid(*pGrid, vValues.Get(0u));

		Navigation::Query query(*pGrid, 0u, 1u, 0u);
		Navigation::QueryState state;

		// Check all impassable cells.
		for (UInt32 uY = 0u; uY < kuHeight; ++uY)
		{
			for (UInt32 uX = 0u; uX < kuWidth; ++uX)
			{
				if (uX >= kuSolidX0 &&
					uX <= kuSolidX1 &&
					uY >= kuSolidY0 &&
					uY <= kuSolidY1)
				{
					SEOUL_UNITTESTING_ASSERT(!query.IsPassable(Navigation::Position(uX, uY)));

					Int32 const iX0 = (Int32)uX - (Int32)kuSolidX0;
					Int32 const iX1 = (Int32)kuSolidX1 - (Int32)uX;
					Int32 const iY0 = (Int32)uY - (Int32)kuSolidY0;
					Int32 const iY1 = (Int32)kuSolidY1 - (Int32)uY;
					Int32 const iMin = Min(iX0, iX1, iY0, iY1);

					UInt32 uExpectedX = 0u;
					UInt32 uExpectedY = 0u;
					if (iX0 == iMin)
					{
						uExpectedX = kuSolidX0 - 1;
						uExpectedY = uY;
					}
					else if (iX1 == iMin)
					{
						uExpectedX = kuSolidX1 + 1;
						uExpectedY = uY;
					}
					else if (iY0 == iMin)
					{
						uExpectedX = uX;
						uExpectedY = kuSolidY0 - 1;
					}
					else if (iY1 == iMin)
					{
						uExpectedX = uX;
						uExpectedY = kuSolidY1 + 1;
					}
					UInt32 uConnectedX = uExpectedX;
					UInt32 uConnectedY = uExpectedY;

					TestNearestConnected(
						state,
						query,
						uX,
						uY,
						Max(kuWidth, kuHeight),
						uConnectedX,
						uConnectedY,
						true,
						uExpectedX,
						uExpectedY);
				}
			}
		}
	}
	Navigation::Grid::Destroy(pGrid);
}

void NavigationTest::TestFindNearestConnectedLargeData()
{
	// Since each test is slow, we perform the check N times.
	static const Int32 kiIterations = 1000;

	Vector<Byte> v;
	SEOUL_UNITTESTING_ASSERT(Base64Decode(s_kNavigationTestDataLarge, v));
	Navigation::Grid* pGrid = Navigation::Grid::CreateFromFileInMemory((UInt8 const*)v.Get(0u), v.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT(nullptr != pGrid);
	{
		Navigation::Query query(*pGrid, 0u, 1u, 0u);
		Navigation::QueryState state;

		// Test nearest
		for (Int32 iIteration = 0; iIteration < kiIterations; ++iIteration)
		{
			UInt32 uX = (UInt32)(GlobalRandom::UniformRandomFloat32() * (Float32)(pGrid->GetWidth() - 1u));
			UInt32 uY = (UInt32)(GlobalRandom::UniformRandomFloat32() * (Float32)(pGrid->GetHeight() - 1u));

			// Only try to get near impassable cells.
			if (query.IsPassable(Navigation::Position(uX, uY)))
			{
				--iIteration;
				continue;
			}

			// Pick a random cell that is passable.
			UInt32 uConnectedX = 0u;
			UInt32 uConnectedY = 0u;
			do
			{
				uConnectedX = (UInt32)(GlobalRandom::UniformRandomFloat32() * (Float32)(pGrid->GetWidth() - 1u));
				uConnectedY = (UInt32)(GlobalRandom::UniformRandomFloat32() * (Float32)(pGrid->GetHeight() - 1u));
			} while (!query.IsPassable(Navigation::Position(uConnectedX, uConnectedY))) ;

			Navigation::Position position(UIntMax, UIntMax);
			SEOUL_UNITTESTING_ASSERT(query.FindNearestConnected(
				state,
				Navigation::Position(uX, uY),
				Max(pGrid->GetWidth(), pGrid->GetHeight()),
				Navigation::Position(uConnectedX, uConnectedY),
				position));

			SEOUL_UNITTESTING_ASSERT(position.m_uX < pGrid->GetWidth());
			SEOUL_UNITTESTING_ASSERT(position.m_uY < pGrid->GetHeight());
			SEOUL_UNITTESTING_ASSERT(query.IsConnected(
				state,
				Navigation::Position(uConnectedX, uConnectedY),
				position));
			SEOUL_UNITTESTING_ASSERT(query.IsPassable(position));
		}
	}
	Navigation::Grid::Destroy(pGrid);
}

void NavigationTest::TestFindPathBasic()
{
	Navigation::Grid* pGrid = Navigation::Grid::Create(8u, 4u);

	{
		// Populate
		static UInt8 const aValues[] =
		{
			1, 0, 1, 1, 0, 1, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			1, 0, 1, 1, 1, 0, 1, 0,
			1, 0, 0, 0, 0, 1, 0, 1,
		};
		SetAndTestGrid(*pGrid, &aValues[0]);

		Navigation::Query query(*pGrid, 0u, 1u, 0u);
		Navigation::QueryState state;

		// Test paths - reachable destination.
		Navigation::Positions vExpected;
		vExpected.Clear();
		vExpected.PushBack(Navigation::Position(1, 0));
		vExpected.PushBack(Navigation::Position(1, 0));
		TestPath(state, query, 1, 0, 1, 0, vExpected, false);

		vExpected.Clear();
		vExpected.PushBack(Navigation::Position(6, 0));
		vExpected.PushBack(Navigation::Position(7, 1));
		vExpected.PushBack(Navigation::Position(7, 2));
		TestPath(state, query, 6, 0, 7, 2, vExpected, false);

		vExpected.Clear();
		vExpected.PushBack(Navigation::Position(1, 0));
		vExpected.PushBack(Navigation::Position(2, 1));
		vExpected.PushBack(Navigation::Position(3, 1));
		TestPath(state, query, 1, 0, 3, 1, vExpected, false);

		vExpected.Clear();
		vExpected.PushBack(Navigation::Position(1, 0));
		vExpected.PushBack(Navigation::Position(2, 1));
		vExpected.PushBack(Navigation::Position(3, 1));
		vExpected.PushBack(Navigation::Position(4, 1));
		vExpected.PushBack(Navigation::Position(5, 1));
		vExpected.PushBack(Navigation::Position(6, 1));
		vExpected.PushBack(Navigation::Position(7, 1));
		TestPath(state, query, 1, 0, 7, 1, vExpected, false);

		vExpected.Clear();
		vExpected.PushBack(Navigation::Position(4, 3));
		vExpected.PushBack(Navigation::Position(2, 3));
		vExpected.PushBack(Navigation::Position(1, 2));
		vExpected.PushBack(Navigation::Position(2, 1));
		vExpected.PushBack(Navigation::Position(3, 1));
		vExpected.PushBack(Navigation::Position(4, 1));
		vExpected.PushBack(Navigation::Position(5, 2));
		TestPath(state, query, 4, 3, 5, 2, vExpected, false);

		vExpected.Clear();
		vExpected.PushBack(Navigation::Position(5, 2));
		vExpected.PushBack(Navigation::Position(4, 1));
		vExpected.PushBack(Navigation::Position(2, 1));
		vExpected.PushBack(Navigation::Position(1, 2));
		vExpected.PushBack(Navigation::Position(2, 3));
		vExpected.PushBack(Navigation::Position(3, 3));
		TestPath(state, query, 5, 2, 3, 3, vExpected, false);

		vExpected.Clear();
		vExpected.PushBack(Navigation::Position(1, 0));
		vExpected.PushBack(Navigation::Position(2, 1));
		vExpected.PushBack(Navigation::Position(3, 1));
		vExpected.PushBack(Navigation::Position(4, 1));
		vExpected.PushBack(Navigation::Position(5, 1));
		vExpected.PushBack(Navigation::Position(6, 1));
		vExpected.PushBack(Navigation::Position(7, 2));
		TestPath(state, query, 1, 0, 7, 2, vExpected, false);

		// Unreachable destination.
		vExpected.Clear();
		TestPath(state, query, 1, 0, 0, 3, vExpected, false);
		TestPath(state, query, 1, 0, 6, 3, vExpected, false);
	}

	Navigation::Grid::Destroy(pGrid);
}

void NavigationTest::TestFindStraightPathBasic()
{
	Navigation::Grid* pGrid = Navigation::Grid::Create(8u, 4u);

	{
		// Populate
		static UInt8 const aValues[] =
		{
			1, 0, 1, 1, 0, 1, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			1, 0, 1, 1, 1, 0, 1, 0,
			1, 0, 0, 0, 0, 1, 0, 1,
		};
		SetAndTestGrid(*pGrid, &aValues[0]);

		Navigation::Query query(*pGrid, 0u, 1u, 0u);
		Navigation::QueryState state;

		// Test paths - reachable destination.
		Navigation::Positions vExpected;
		vExpected.Clear();
		vExpected.PushBack(Navigation::Position(1, 0));
		vExpected.PushBack(Navigation::Position(1, 0));
		TestPath(state, query, 1, 0, 1, 0, vExpected, true);

		vExpected.Clear();
		vExpected.PushBack(Navigation::Position(6, 0));
		vExpected.PushBack(Navigation::Position(7, 2));
		TestPath(state, query, 6, 0, 7, 2, vExpected, true);

		vExpected.Clear();
		vExpected.PushBack(Navigation::Position(1, 0));
		vExpected.PushBack(Navigation::Position(2, 1));
		vExpected.PushBack(Navigation::Position(3, 1));
		TestPath(state, query, 1, 0, 3, 1, vExpected, true);

		vExpected.Clear();
		vExpected.PushBack(Navigation::Position(1, 0));
		vExpected.PushBack(Navigation::Position(2, 1));
		vExpected.PushBack(Navigation::Position(7, 1));
		TestPath(state, query, 1, 0, 7, 1, vExpected, true);

		vExpected.Clear();
		vExpected.PushBack(Navigation::Position(4, 3));
		vExpected.PushBack(Navigation::Position(2, 3));
		vExpected.PushBack(Navigation::Position(1, 2));
		vExpected.PushBack(Navigation::Position(2, 1));
		vExpected.PushBack(Navigation::Position(4, 1));
		vExpected.PushBack(Navigation::Position(5, 2));
		TestPath(state, query, 4, 3, 5, 2, vExpected, true);

		vExpected.Clear();
		vExpected.PushBack(Navigation::Position(5, 2));
		vExpected.PushBack(Navigation::Position(4, 1));
		vExpected.PushBack(Navigation::Position(2, 1));
		vExpected.PushBack(Navigation::Position(1, 2));
		vExpected.PushBack(Navigation::Position(2, 3));
		vExpected.PushBack(Navigation::Position(3, 3));
		TestPath(state, query, 5, 2, 3, 3, vExpected, true);

		vExpected.Clear();
		vExpected.PushBack(Navigation::Position(1, 0));
		vExpected.PushBack(Navigation::Position(2, 1));
		vExpected.PushBack(Navigation::Position(6, 1));
		vExpected.PushBack(Navigation::Position(7, 2));
		TestPath(state, query, 1, 0, 7, 2, vExpected, true);

		// Unreachable destination.
		vExpected.Clear();
		TestPath(state, query, 1, 0, 0, 3, vExpected, true);
		TestPath(state, query, 1, 0, 6, 3, vExpected, true);
	}

	Navigation::Grid::Destroy(pGrid);
}

void NavigationTest::TestGridBasic()
{
	static const UInt32 kuTestWidth = 4u;
	static const UInt32 kuTestClampedWidth = 3u;
	static const UInt32 kuTestHeight = 8u;
	static const UInt32 kuTestClampedHeight = 12u;

	// Create empty grid, verify contents.
	Navigation::Grid* pGrid = Navigation::Grid::Create(kuTestWidth, kuTestHeight);
	SEOUL_UNITTESTING_ASSERT(nullptr != pGrid);
	SEOUL_UNITTESTING_ASSERT_EQUAL(kuTestWidth, pGrid->GetWidth());
	SEOUL_UNITTESTING_ASSERT_EQUAL(kuTestHeight, pGrid->GetHeight());
	for (UInt32 y = 0u; y < kuTestHeight; ++y)
	{
		for (UInt32 x = 0u; x < kuTestWidth; ++x)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(0u, pGrid->GetCell(x, y));
		}
	}

	// Now populate with random values.
	for (UInt32 y = 0u; y < kuTestHeight; ++y)
	{
		for (UInt32 x = 0u; x < kuTestWidth; ++x)
		{
			UInt8 const uValue = (UInt8)Round(GlobalRandom::UniformRandomFloat32() * 255.0f);
			pGrid->SetCell(x, y, uValue);
			SEOUL_UNITTESTING_ASSERT_EQUAL(uValue, pGrid->GetCell(x, y));

			// Sanity check indexing.
			SEOUL_UNITTESTING_ASSERT_EQUAL(uValue, pGrid->GetGrid()[y * pGrid->GetWidth() + x]);
		}
	}

	// Save the grid.
	UInt32 uDataSizeInBytes = 0u;
	UInt8* pData = pGrid->Save(uDataSizeInBytes);
	SEOUL_UNITTESTING_ASSERT(nullptr != pData);
	SEOUL_UNITTESTING_ASSERT(uDataSizeInBytes > 0u);

	// Load into a new grid.
	Navigation::Grid* pCopy = Navigation::Grid::CreateFromFileInMemory(pData, uDataSizeInBytes);
	SEOUL_UNITTESTING_ASSERT(nullptr != pCopy);

	// Free the saved data.
	MemoryManager::Deallocate(pData);

	// Compare, must be exactly equal.
	SEOUL_UNITTESTING_ASSERT_EQUAL(kuTestWidth, pCopy->GetWidth());
	SEOUL_UNITTESTING_ASSERT_EQUAL(kuTestHeight, pCopy->GetHeight());
	SEOUL_UNITTESTING_ASSERT_EQUAL(pGrid->GetWidth(), pCopy->GetWidth());
	SEOUL_UNITTESTING_ASSERT_EQUAL(pGrid->GetHeight(), pCopy->GetHeight());

	// Test value equality.
	for (UInt32 y = 0u; y < kuTestHeight; ++y)
	{
		for (UInt32 x = 0u; x < kuTestWidth; ++x)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(pGrid->GetCell(x, y), pCopy->GetCell(x, y));
		}
	}

	// Free the copy;
	Navigation::Grid::Destroy(pCopy);
	SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, pCopy);

	// Create a clamped copy and test it.
	{
		pCopy = Navigation::Grid::CreateFromGrid(kuTestClampedWidth, kuTestClampedHeight, *pGrid);
		SEOUL_UNITTESTING_ASSERT(nullptr != pCopy);
		SEOUL_UNITTESTING_ASSERT_EQUAL(kuTestClampedWidth, pCopy->GetWidth());
		SEOUL_UNITTESTING_ASSERT_EQUAL(kuTestClampedHeight, pCopy->GetHeight());

		// Test
		for (UInt32 y = 0u; y < kuTestClampedHeight; ++y)
		{
			for (UInt32 x = 0u; x < kuTestClampedWidth; ++x)
			{
				if (x < pGrid->GetWidth() && y < pGrid->GetHeight())
				{
					SEOUL_UNITTESTING_ASSERT_EQUAL(pGrid->GetCell(x, y), pCopy->GetCell(x, y));
				}
				else
				{
					SEOUL_UNITTESTING_ASSERT_EQUAL(0u, pCopy->GetCell(x, y));
				}
			}
		}

		// Free the copy.
		Navigation::Grid::Destroy(pCopy);
		SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, pCopy);
	}

	// Free the grid.
	Navigation::Grid::Destroy(pGrid);
	SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, pGrid);
}

static void TestRay(
	const Navigation::Query& query,
	UInt32 uStartX,
	UInt32 uStartY,
	UInt32 uEndX,
	UInt32 uEndY,
	Bool bExpectHit,
	UInt32 uExpectX,
	UInt32 uExpectY)
{
	Navigation::QueryState state;

	// Test with hit starting cell.
	{
		Navigation::Position position;
		Bool const bSuccess = query.RayTest(
			state,
			Navigation::Position(uStartX, uStartY),
			Navigation::Position(uEndX, uEndY),
			true,
			position);

		SEOUL_UNITTESTING_ASSERT_EQUAL(bExpectHit, bSuccess);
		SEOUL_UNITTESTING_ASSERT_EQUAL(uExpectX, position.m_uX);
		SEOUL_UNITTESTING_ASSERT_EQUAL(uExpectY, position.m_uY);
	}

	// Test without hit starting cell.
	{
		Navigation::Position position;
		Bool const bSuccess = query.RayTest(
			state,
			Navigation::Position(uStartX, uStartY),
			Navigation::Position(uEndX, uEndY),
			false,
			position);

		SEOUL_UNITTESTING_ASSERT_EQUAL(bExpectHit, bSuccess);
		SEOUL_UNITTESTING_ASSERT_EQUAL(uExpectX, position.m_uX);
		SEOUL_UNITTESTING_ASSERT_EQUAL(uExpectY, position.m_uY);
	}
}

static void TestRay2(
	const Navigation::Query& query,
	UInt32 uStartX,
	UInt32 uStartY,
	UInt32 uEndX,
	UInt32 uEndY,
	Bool bHitStartingCell,
	Bool bExpectHit,
	UInt32 uExpectX,
	UInt32 uExpectY)
{
	Navigation::QueryState state;

	Navigation::Position position;
	Bool const bSuccess = query.RayTest(
		state,
		Navigation::Position(uStartX, uStartY),
		Navigation::Position(uEndX, uEndY),
		bHitStartingCell,
		position);

	SEOUL_UNITTESTING_ASSERT_EQUAL(bExpectHit, bSuccess);
	SEOUL_UNITTESTING_ASSERT_EQUAL(uExpectX, position.m_uX);
	SEOUL_UNITTESTING_ASSERT_EQUAL(uExpectY, position.m_uY);
}

void NavigationTest::TestRayTestBasic()
{
	Navigation::Grid* pGrid = Navigation::Grid::Create(8u, 4u);

	{
		// Populate
		static UInt8 const aValues[] =
		{
			1, 0, 1, 1, 0, 1, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			1, 0, 1, 1, 1, 0, 1, 0,
			1, 0, 0, 0, 0, 0, 1, 1,
		};
		SetAndTestGrid(*pGrid, &aValues[0]);

		Navigation::Query query(*pGrid, 0u, 1u, 0u);

		// Test rays - basic, start open, various results.
		TestRay(query, 1, 0, 2, 1, false, 0, 0);
		TestRay(query, 1, 0, 3, 1, true, 2, 0);
		TestRay(query, 1, 0, 4, 1, true, 2, 0);
		TestRay(query, 1, 0, 5, 1, true, 2, 0);
		TestRay(query, 1, 0, 6, 1, true, 2, 0);
		TestRay(query, 1, 0, 7, 1, true, 2, 0);
		TestRay(query, 1, 0, 1, 3, false, 0, 0);
		TestRay(query, 1, 0, 0, 0, true, 0, 0);
		TestRay(query, 1, 0, 0, 1, false, 0, 0);
		TestRay(query, 1, 1, 1, 0, false, 0, 0);
		TestRay(query, 1, 1, 0, 1, false, 0, 0);
		TestRay(query, 1, 1, 1, 2, false, 0, 0);
		TestRay(query, 1, 1, 1, 3, false, 0, 0);
		TestRay(query, 1, 1, 2, 1, false, 0, 0);
		TestRay(query, 1, 1, 3, 1, false, 0, 0);
		TestRay(query, 5, 1, 5, 0, true, 5, 0);
		TestRay(query, 5, 1, 6, 1, false, 0, 0);
		TestRay(query, 5, 1, 5, 2, false, 0, 0);
		TestRay(query, 5, 1, 4, 1, false, 0, 0);
		TestRay(query, 5, 1, 4, 3, false, 0, 0);

		// Test rays - start on blocking cell.
		TestRay2(query, 5, 0, 4, 3, false, true, 4, 2);
		TestRay2(query, 5, 0, 4, 3, true, true, 5, 0);
		TestRay2(query, 5, 0, 4, 3, true, true, 5, 0);
		TestRay2(query, 7, 3, 6, 2, false, true, 6, 2);
		TestRay2(query, 7, 3, 6, 2, true, true, 7, 3);
		TestRay2(query, 0, 2, 3, 2, false, true, 2, 2);
		TestRay2(query, 0, 2, 3, 2, true, true, 0, 2);
		TestRay2(query, 6, 2, 3, 2, false, true, 4, 2);
		TestRay2(query, 6, 2, 3, 2, true, true, 6, 2);
		TestRay2(query, 6, 3, 6, 0, false, true, 6, 2);
		TestRay2(query, 6, 3, 6, 0, true, true, 6, 3);
		TestRay2(query, 0, 0, 0, 3, false, true, 0, 2);
		TestRay2(query, 0, 0, 0, 3, true, true, 0, 0);
	}

	Navigation::Grid::Destroy(pGrid);
}

void NavigationTest::TestRobustFindStraightPathBasic()
{
	Navigation::Grid* pGrid = Navigation::Grid::Create(8u, 4u);

	{
		// Populate
		static UInt8 const aValues[] =
		{
			1, 0, 1, 1, 0, 1, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			1, 0, 1, 1, 1, 0, 1, 0,
			1, 0, 0, 0, 0, 1, 0, 1,
		};
		SetAndTestGrid(*pGrid, &aValues[0]);

		Navigation::Query query(*pGrid, 0u, 1u, 0u);
		Navigation::QueryState state;

		// Test paths - reachable destination.
		Navigation::Positions vExpected;
		vExpected.Clear();
		vExpected.PushBack(Navigation::Position(1, 0));
		vExpected.PushBack(Navigation::Position(1, 0));
		TestRobustPath(state, query, 1, 0, 1, 0, vExpected, 0u, 0u);

		vExpected.Clear();
		vExpected.PushBack(Navigation::Position(6, 0));
		vExpected.PushBack(Navigation::Position(7, 2));
		TestRobustPath(state, query, 6, 0, 7, 2, vExpected, 0u, 0u);

		vExpected.Clear();
		vExpected.PushBack(Navigation::Position(1, 0));
		vExpected.PushBack(Navigation::Position(2, 1));
		vExpected.PushBack(Navigation::Position(3, 1));
		TestRobustPath(state, query, 1, 0, 3, 1, vExpected, 0u, 0u);

		vExpected.Clear();
		vExpected.PushBack(Navigation::Position(1, 0));
		vExpected.PushBack(Navigation::Position(2, 1));
		vExpected.PushBack(Navigation::Position(7, 1));
		TestRobustPath(state, query, 1, 0, 7, 1, vExpected, 0u, 0u);

		vExpected.Clear();
		vExpected.PushBack(Navigation::Position(4, 3));
		vExpected.PushBack(Navigation::Position(2, 3));
		vExpected.PushBack(Navigation::Position(1, 2));
		vExpected.PushBack(Navigation::Position(2, 1));
		vExpected.PushBack(Navigation::Position(4, 1));
		vExpected.PushBack(Navigation::Position(5, 2));
		TestRobustPath(state, query, 4, 3, 5, 2, vExpected, 0u, 0u);

		vExpected.Clear();
		vExpected.PushBack(Navigation::Position(5, 2));
		vExpected.PushBack(Navigation::Position(4, 1));
		vExpected.PushBack(Navigation::Position(2, 1));
		vExpected.PushBack(Navigation::Position(1, 2));
		vExpected.PushBack(Navigation::Position(2, 3));
		vExpected.PushBack(Navigation::Position(3, 3));
		TestRobustPath(state, query, 5, 2, 3, 3, vExpected, 0u, 0u);

		vExpected.Clear();
		vExpected.PushBack(Navigation::Position(1, 0));
		vExpected.PushBack(Navigation::Position(2, 1));
		vExpected.PushBack(Navigation::Position(6, 1));
		vExpected.PushBack(Navigation::Position(7, 2));
		TestRobustPath(state, query, 1, 0, 7, 2, vExpected, 0u, 0u);

		// Impassable start, distance success.
		vExpected.Clear();
		vExpected.PushBack(Navigation::Position(1, 0));
		vExpected.PushBack(Navigation::Position(2, 1));
		vExpected.PushBack(Navigation::Position(6, 1));
		vExpected.PushBack(Navigation::Position(7, 2));
		TestRobustPath(state, query, 0, 0, 7, 2, vExpected, 1u, 0u);
		// Impassable start, distance fail.
		vExpected.Clear();
		TestRobustPath(state, query, 0, 0, 7, 2, vExpected, 0u, 0u);

		// Unreachable destination, impassable end.
		vExpected.Clear();
		vExpected.PushBack(Navigation::Position(1, 0));
		vExpected.PushBack(Navigation::Position(1, 3));
		TestRobustPath(state, query, 1, 0, 0, 3, vExpected, 0u, 1u);
		vExpected.Clear();
		vExpected.PushBack(Navigation::Position(0, 1));
		vExpected.PushBack(Navigation::Position(6, 1));
		TestRobustPath(state, query, 0, 1, 6, 2, vExpected, 0u, 1u);
		// Impassable end, distance fail.
		vExpected.Clear();
		TestRobustPath(state, query, 1, 0, 0, 3, vExpected, 0u, 0u);

		// Unreachable destination, passable end.
		vExpected.Clear();
		TestRobustPath(state, query, 1, 0, 6, 3, vExpected, 0u, 0u);
	}

	Navigation::Grid::Destroy(pGrid);
}

#endif // /#if SEOUL_WITH_NAVIGATION

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
