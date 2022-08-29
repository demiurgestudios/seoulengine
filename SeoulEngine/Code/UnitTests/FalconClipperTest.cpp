/**
 * \file FalconTest.cpp
 * \brief Unit test for functionality in the Falcon project.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FalconClipper.h"
#include "FalconClipperTest.h"
#include "FalconTesselator.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

// For UnitTestingToString
SEOUL_BEGIN_TEMPLATE_TYPE(
	UnsafeBuffer,
	(T, MEMORY_BUDGETS),
	(typename T, Int MEMORY_BUDGETS),
	("UnsafeBuffer<%s, %d>", SEOUL_GET_FULLY_QUALIFIED_TYPE_NAME(T), MEMORY_BUDGETS))
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(FalconClipperTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestClipStackNone)
	SEOUL_METHOD(TestClipStackConvexOneLevel)
	SEOUL_METHOD(TestClipStackConvexOneLevelMatrix)
	SEOUL_METHOD(TestClipStackConvexTwoLevels)
	SEOUL_METHOD(TestClipStackConvexTwoLevels2)
	SEOUL_METHOD(TestClipStackRectangleOneLevel)
	SEOUL_METHOD(TestClipStackRectangleOneLevelMatrix)
	SEOUL_METHOD(TestClipStackRectangleOneLevelMulti)
	SEOUL_METHOD(TestClipStackRectangleOneLevelMultiNoneClipAllClip)
	SEOUL_METHOD(TestClipStackRectangleTwoLevels)
	SEOUL_METHOD(TestClipStackRectangleTwoLevelsAllClipped)
	SEOUL_METHOD(TestClipStackRectangleTwoLevelsMulti)
	SEOUL_METHOD(TestClipStackRectangleTwoLevelsNoneClipAllClip)
	SEOUL_METHOD(TestConvexRectangleBasic)
	SEOUL_METHOD(TestConvexRectanglePartial)
	SEOUL_METHOD(TestConvexVerticesBasic)
	SEOUL_METHOD(TestConvexVerticesPartial)
	SEOUL_METHOD(TestEmpty)
	SEOUL_METHOD(TestMeshRectangleConvex)
	SEOUL_METHOD(TestMeshRectangleConvexClipAll)
	SEOUL_METHOD(TestMeshRectangleConvexClipNone)
	SEOUL_METHOD(TestMeshRectangleNotSpecific)
	SEOUL_METHOD(TestMeshRectangleQuadList)
	SEOUL_METHOD(TestMeshRectangleQuadListMulti)
	SEOUL_METHOD(TestMeshVerticesConvex)
	SEOUL_METHOD(TestMeshVerticesNotSpecific)
	SEOUL_METHOD(TestMeshVerticesQuadList)
	SEOUL_METHOD(TestMeshRectangleNotSpecificNotClippingInitially)
	SEOUL_METHOD(TestMeshTextChunkNoClip)
	SEOUL_METHOD(TestMeshTextChunkClipRegression)
	SEOUL_METHOD(TestMeshTextChunkClipRegression2)
	SEOUL_METHOD(TestMirrorTransform)
	SEOUL_METHOD(TestClipperClipRegression)
	SEOUL_METHOD(TestZeroSizeClipRegression)
SEOUL_END_TYPE()

template <typename T, int MEMORY_BUDGETS>
static inline Bool operator==(const Vector<T, MEMORY_BUDGETS>& v0, const Vector<T, MEMORY_BUDGETS>& v1)
{
	if (v0.GetSize() != v1.GetSize())
	{
		return false;
	}

	for (UInt32 i = 0u; i < v0.GetSize(); ++i)
	{
		if (v0[i] != v1[i])
		{
			return false;
		}
	}

	return true;
}

class FalconClipperTestTesselationCallback SEOUL_SEALED : public Falcon::TesselationCallback
{
public:
	FalconClipperTestTesselationCallback()
		: m_vIndices()
		, m_vVertices()
	{
	}

	virtual void BeginShape() SEOUL_OVERRIDE
	{
	}

	virtual void AcceptLineStrip(
		const Falcon::LineStyle& lineStyle,
		const LineStrip& vLineStrip) SEOUL_OVERRIDE
	{
	}

	virtual void AcceptTriangleList(
		const Falcon::FillStyle& fillStyle,
		const Vertices& vVertices,
		const Indices& vIndices,
		Bool bConvex) SEOUL_OVERRIDE
	{
		m_vIndices = vIndices;
		m_vVertices = vVertices;
	}

	virtual void EndShape() SEOUL_OVERRIDE
	{
	}

	Indices m_vIndices;
	Vertices m_vVertices;

private:
	SEOUL_DISABLE_COPY(FalconClipperTestTesselationCallback);
}; // class FalconClipperTestTesselationCallback

static void TestClipStackNoneCommon(Falcon::ClipStack& stack)
{
	using namespace Falcon;

	SEOUL_UNITTESTING_ASSERT(!stack.HasClips());

	Clipper::Indices vIndices;
	vIndices.PushBack(0);
	vIndices.PushBack(1);
	vIndices.PushBack(2);
	vIndices.PushBack(0);
	vIndices.PushBack(2);
	vIndices.PushBack(3);

	Clipper::Vertices vVertices;
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-5, -5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-5,  5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 5,  5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 5, -5)));

	// Should not clip either indices or vertices.
	stack.MeshClip(
		TriangleListDescription::kConvex,
		vIndices,
		vVertices);

	SEOUL_UNITTESTING_ASSERT_EQUAL(6, vIndices.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vIndices[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[3]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[4]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, vIndices[5]);

	SEOUL_UNITTESTING_ASSERT_EQUAL(4, vVertices.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ShapeVertex::Create(Vector2D(-5, -5)), vVertices[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ShapeVertex::Create(Vector2D(-5,  5)), vVertices[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ShapeVertex::Create(Vector2D( 5,  5)), vVertices[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ShapeVertex::Create(Vector2D( 5, -5)), vVertices[3]);
}

void FalconClipperTest::TestClipStackNone()
{
	using namespace Falcon;

	ClipStack stack;
	TestClipStackNoneCommon(stack);
}

void FalconClipperTest::TestClipStackConvexOneLevel()
{
	using namespace Falcon;

	Vector2D aClipVertices[] =
	{
		Vector2D( 0, -3),
		Vector2D( 3,  0),
		Vector2D( 0,  3),
		Vector2D(-3,  0),
	};
	ClipStack stack;

	stack.AddConvexHull(aClipVertices, SEOUL_ARRAY_COUNT(aClipVertices));
	SEOUL_UNITTESTING_ASSERT(!stack.HasClips());
	SEOUL_UNITTESTING_ASSERT(stack.Push());
	SEOUL_UNITTESTING_ASSERT(stack.HasClips());

	SEOUL_UNITTESTING_ASSERT_EQUAL(-3, stack.GetTopClip().m_Bounds.m_fLeft);
	SEOUL_UNITTESTING_ASSERT_EQUAL( 3, stack.GetTopClip().m_Bounds.m_fRight);
	SEOUL_UNITTESTING_ASSERT_EQUAL(-3, stack.GetTopClip().m_Bounds.m_fTop);
	SEOUL_UNITTESTING_ASSERT_EQUAL( 3, stack.GetTopClip().m_Bounds.m_fBottom);
	SEOUL_UNITTESTING_ASSERT(!stack.GetTopClip().m_bSimple);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uFirstHull);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uFirstVertex);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1u, stack.GetTopClip().m_uHulls);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4u, stack.GetTopClip().m_uVertices);

	Clipper::Indices vIndices;
	vIndices.PushBack(0);
	vIndices.PushBack(1);
	vIndices.PushBack(2);
	vIndices.PushBack(0);
	vIndices.PushBack(2);
	vIndices.PushBack(3);

	Clipper::Vertices vVertices;
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 2, -2)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 2,  2)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-2,  2)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-2, -2)));

	stack.MeshClip(
		TriangleListDescription::kConvex,
		vIndices,
		vVertices);

	SEOUL_UNITTESTING_ASSERT_EQUAL(18, vIndices.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(8, vVertices.GetSize());

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vIndices[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[3]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[4]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, vIndices[5]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[6]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, vIndices[7]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, vIndices[8]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[9]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, vIndices[10]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, vIndices[11]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[12]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, vIndices[13]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(6, vIndices[14]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[15]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(6, vIndices[16]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(7, vIndices[17]);

	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-1, -2)), vVertices[0], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 1, -2)), vVertices[1], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 2, -1)), vVertices[2], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 2,  1)), vVertices[3], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 1,  2)), vVertices[4], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-1,  2)), vVertices[5], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-2,  1)), vVertices[6], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-2, -1)), vVertices[7], kfAboutEqualPosition);

	stack.Pop();
	TestClipStackNoneCommon(stack);
}

void FalconClipperTest::TestClipStackConvexOneLevelMatrix()
{
	using namespace Falcon;

	Vector2D aClipVertices[] =
	{
		Vector2D( 0, -3),
		Vector2D( 3,  0),
		Vector2D( 0,  3),
		Vector2D(-3,  0),
	};
	ClipStack stack;

	stack.AddConvexHull(Matrix2x3::CreateRotationFromDegrees(360.0f), aClipVertices, SEOUL_ARRAY_COUNT(aClipVertices));
	SEOUL_UNITTESTING_ASSERT(!stack.HasClips());
	SEOUL_UNITTESTING_ASSERT(stack.Push());
	SEOUL_UNITTESTING_ASSERT(stack.HasClips());

	SEOUL_UNITTESTING_ASSERT_EQUAL(-3, stack.GetTopClip().m_Bounds.m_fLeft);
	SEOUL_UNITTESTING_ASSERT_EQUAL( 3, stack.GetTopClip().m_Bounds.m_fRight);
	SEOUL_UNITTESTING_ASSERT_EQUAL(-3, stack.GetTopClip().m_Bounds.m_fTop);
	SEOUL_UNITTESTING_ASSERT_EQUAL( 3, stack.GetTopClip().m_Bounds.m_fBottom);
	SEOUL_UNITTESTING_ASSERT(!stack.GetTopClip().m_bSimple);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uFirstHull);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uFirstVertex);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1u, stack.GetTopClip().m_uHulls);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4u, stack.GetTopClip().m_uVertices);

	Clipper::Indices vIndices;
	vIndices.PushBack(0);
	vIndices.PushBack(1);
	vIndices.PushBack(2);
	vIndices.PushBack(0);
	vIndices.PushBack(2);
	vIndices.PushBack(3);

	Clipper::Vertices vVertices;
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 2, -2)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 2,  2)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-2,  2)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-2, -2)));

	stack.MeshClip(
		TriangleListDescription::kConvex,
		vIndices,
		vVertices);

	SEOUL_UNITTESTING_ASSERT_EQUAL(18, vIndices.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(8, vVertices.GetSize());

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vIndices[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[3]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[4]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, vIndices[5]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[6]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, vIndices[7]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, vIndices[8]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[9]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, vIndices[10]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, vIndices[11]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[12]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, vIndices[13]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(6, vIndices[14]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[15]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(6, vIndices[16]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(7, vIndices[17]);

	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-1, -2)), vVertices[0], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 1, -2)), vVertices[1], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 2, -1)), vVertices[2], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 2,  1)), vVertices[3], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 1,  2)), vVertices[4], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-1,  2)), vVertices[5], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-2,  1)), vVertices[6], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-2, -1)), vVertices[7], kfAboutEqualPosition);

	stack.Pop();
	TestClipStackNoneCommon(stack);
}

void FalconClipperTest::TestClipStackConvexTwoLevels()
{
	using namespace Falcon;

	ClipStack stack;

	stack.AddRectangle(Rectangle::Create(-100, 100, -100, 100));
	SEOUL_UNITTESTING_ASSERT(!stack.HasClips());
	SEOUL_UNITTESTING_ASSERT(stack.Push());
	SEOUL_UNITTESTING_ASSERT(stack.HasClips());

	SEOUL_UNITTESTING_ASSERT_EQUAL(-100, stack.GetTopClip().m_Bounds.m_fLeft);
	SEOUL_UNITTESTING_ASSERT_EQUAL( 100, stack.GetTopClip().m_Bounds.m_fRight);
	SEOUL_UNITTESTING_ASSERT_EQUAL(-100, stack.GetTopClip().m_Bounds.m_fTop);
	SEOUL_UNITTESTING_ASSERT_EQUAL( 100, stack.GetTopClip().m_Bounds.m_fBottom);
	SEOUL_UNITTESTING_ASSERT(stack.GetTopClip().m_bSimple);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uFirstHull);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uFirstVertex);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uHulls);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uVertices);

	Vector2D aClipVertices[] =
	{
		Vector2D( 0, -3),
		Vector2D( 3,  0),
		Vector2D( 0,  3),
		Vector2D(-3,  0),
	};
	stack.AddConvexHull(aClipVertices, SEOUL_ARRAY_COUNT(aClipVertices));
	SEOUL_UNITTESTING_ASSERT(stack.HasClips());
	SEOUL_UNITTESTING_ASSERT(stack.Push());
	SEOUL_UNITTESTING_ASSERT(stack.HasClips());

	SEOUL_UNITTESTING_ASSERT_EQUAL(-3, stack.GetTopClip().m_Bounds.m_fLeft);
	SEOUL_UNITTESTING_ASSERT_EQUAL( 3, stack.GetTopClip().m_Bounds.m_fRight);
	SEOUL_UNITTESTING_ASSERT_EQUAL(-3, stack.GetTopClip().m_Bounds.m_fTop);
	SEOUL_UNITTESTING_ASSERT_EQUAL( 3, stack.GetTopClip().m_Bounds.m_fBottom);
	SEOUL_UNITTESTING_ASSERT(!stack.GetTopClip().m_bSimple);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uFirstHull);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uFirstVertex);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1u, stack.GetTopClip().m_uHulls);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4u, stack.GetTopClip().m_uVertices);

	Clipper::Indices vIndices;
	vIndices.PushBack(0);
	vIndices.PushBack(1);
	vIndices.PushBack(2);
	vIndices.PushBack(0);
	vIndices.PushBack(2);
	vIndices.PushBack(3);

	Clipper::Vertices vVertices;
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 2, -2)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 2,  2)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-2,  2)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-2, -2)));

	stack.MeshClip(
		TriangleListDescription::kConvex,
		vIndices,
		vVertices);

	SEOUL_UNITTESTING_ASSERT_EQUAL(18, vIndices.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(8, vVertices.GetSize());

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vIndices[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[3]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[4]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, vIndices[5]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[6]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, vIndices[7]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, vIndices[8]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[9]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, vIndices[10]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, vIndices[11]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[12]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, vIndices[13]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(6, vIndices[14]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[15]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(6, vIndices[16]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(7, vIndices[17]);

	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-1, -2)), vVertices[0], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 1, -2)), vVertices[1], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 2, -1)), vVertices[2], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 2,  1)), vVertices[3], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 1,  2)), vVertices[4], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-1,  2)), vVertices[5], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-2,  1)), vVertices[6], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-2, -1)), vVertices[7], kfAboutEqualPosition);

	stack.Pop();
	stack.Pop();
	TestClipStackNoneCommon(stack);
}

void FalconClipperTest::TestClipStackConvexTwoLevels2()
{
	using namespace Falcon;

	ClipStack stack;

	Vector2D aClipVertices[] =
	{
		Vector2D( 0, -3),
		Vector2D( 3,  0),
		Vector2D( 0,  3),
		Vector2D(-3,  0),
	};
	stack.AddConvexHull(aClipVertices, SEOUL_ARRAY_COUNT(aClipVertices));
	SEOUL_UNITTESTING_ASSERT(!stack.HasClips());
	SEOUL_UNITTESTING_ASSERT(stack.Push());
	SEOUL_UNITTESTING_ASSERT(stack.HasClips());

	SEOUL_UNITTESTING_ASSERT_EQUAL(-3, stack.GetTopClip().m_Bounds.m_fLeft);
	SEOUL_UNITTESTING_ASSERT_EQUAL( 3, stack.GetTopClip().m_Bounds.m_fRight);
	SEOUL_UNITTESTING_ASSERT_EQUAL(-3, stack.GetTopClip().m_Bounds.m_fTop);
	SEOUL_UNITTESTING_ASSERT_EQUAL( 3, stack.GetTopClip().m_Bounds.m_fBottom);
	SEOUL_UNITTESTING_ASSERT(!stack.GetTopClip().m_bSimple);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uFirstHull);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uFirstVertex);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1u, stack.GetTopClip().m_uHulls);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4u, stack.GetTopClip().m_uVertices);

	stack.AddRectangle(Rectangle::Create(-100, 100, -100, 100));
	SEOUL_UNITTESTING_ASSERT(stack.HasClips());
	SEOUL_UNITTESTING_ASSERT(stack.Push());
	SEOUL_UNITTESTING_ASSERT(stack.HasClips());

	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(-3, stack.GetTopClip().m_Bounds.m_fLeft, kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL( 3, stack.GetTopClip().m_Bounds.m_fRight, kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(-3, stack.GetTopClip().m_Bounds.m_fTop, kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL( 3, stack.GetTopClip().m_Bounds.m_fBottom, kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT(!stack.GetTopClip().m_bSimple);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1u, stack.GetTopClip().m_uFirstHull);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4u, stack.GetTopClip().m_uFirstVertex);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1u, stack.GetTopClip().m_uHulls);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4u, stack.GetTopClip().m_uVertices);

	Clipper::Indices vIndices;
	vIndices.PushBack(0);
	vIndices.PushBack(1);
	vIndices.PushBack(2);
	vIndices.PushBack(0);
	vIndices.PushBack(2);
	vIndices.PushBack(3);

	Clipper::Vertices vVertices;
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 2, -2)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 2,  2)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-2,  2)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-2, -2)));

	stack.MeshClip(
		TriangleListDescription::kConvex,
		vIndices,
		vVertices);

	SEOUL_UNITTESTING_ASSERT_EQUAL(18, vIndices.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(8, vVertices.GetSize());

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vIndices[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[3]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[4]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, vIndices[5]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[6]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, vIndices[7]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, vIndices[8]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[9]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, vIndices[10]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, vIndices[11]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[12]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, vIndices[13]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(6, vIndices[14]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[15]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(6, vIndices[16]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(7, vIndices[17]);

	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-1, -2)), vVertices[0], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 1, -2)), vVertices[1], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 2, -1)), vVertices[2], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 2,  1)), vVertices[3], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 1,  2)), vVertices[4], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-1,  2)), vVertices[5], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-2,  1)), vVertices[6], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-2, -1)), vVertices[7], kfAboutEqualPosition);

	stack.Pop();
	stack.Pop();
	TestClipStackNoneCommon(stack);
}

void FalconClipperTest::TestClipStackRectangleOneLevel()
{
	using namespace Falcon;

	ClipStack stack;
	stack.AddRectangle(Rectangle::Create(0, 1, 0, 1));
	SEOUL_UNITTESTING_ASSERT(!stack.HasClips());
	SEOUL_UNITTESTING_ASSERT(stack.Push());
	SEOUL_UNITTESTING_ASSERT(stack.HasClips());

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, stack.GetTopClip().m_Bounds.m_fLeft);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, stack.GetTopClip().m_Bounds.m_fRight);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, stack.GetTopClip().m_Bounds.m_fTop);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, stack.GetTopClip().m_Bounds.m_fBottom);
	SEOUL_UNITTESTING_ASSERT(stack.GetTopClip().m_bSimple);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uFirstHull);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uFirstVertex);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uHulls);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uVertices);

	Clipper::Indices vIndices;
	vIndices.PushBack(0);
	vIndices.PushBack(1);
	vIndices.PushBack(2);
	vIndices.PushBack(0);
	vIndices.PushBack(2);
	vIndices.PushBack(3);

	Clipper::Vertices vVertices;
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 5, -5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 5,  5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-5,  5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-5, -5)));

	stack.MeshClip(TriangleListDescription::kQuadList, vIndices, vVertices);

	SEOUL_UNITTESTING_ASSERT_EQUAL(6, vIndices.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vIndices[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[3]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[4]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, vIndices[5]);

	SEOUL_UNITTESTING_ASSERT_EQUAL(4u, vVertices.GetSize());
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(1, 0)), vVertices[0], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(1, 1)), vVertices[1], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(0, 1)), vVertices[2], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(0, 0)), vVertices[3], kfAboutEqualPosition);

	stack.Pop();
	TestClipStackNoneCommon(stack);
}

void FalconClipperTest::TestClipStackRectangleOneLevelMatrix()
{
	using namespace Falcon;

	ClipStack stack;
	stack.AddRectangle(Matrix2x3::CreateRotationFromDegrees(360.0f), Rectangle::Create(0, 1, 0, 1));
	SEOUL_UNITTESTING_ASSERT(!stack.HasClips());
	SEOUL_UNITTESTING_ASSERT(stack.Push());
	SEOUL_UNITTESTING_ASSERT(stack.HasClips());

	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(0, stack.GetTopClip().m_Bounds.m_fLeft, kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, stack.GetTopClip().m_Bounds.m_fRight);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, stack.GetTopClip().m_Bounds.m_fTop);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(1, stack.GetTopClip().m_Bounds.m_fBottom, kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT(stack.GetTopClip().m_bSimple);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uFirstHull);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uFirstVertex);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uHulls);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uVertices);

	Clipper::Indices vIndices;
	vIndices.PushBack(0);
	vIndices.PushBack(1);
	vIndices.PushBack(2);
	vIndices.PushBack(0);
	vIndices.PushBack(2);
	vIndices.PushBack(3);

	Clipper::Vertices vVertices;
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 5, -5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 5,  5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-5,  5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-5, -5)));

	stack.MeshClip(TriangleListDescription::kQuadList, vIndices, vVertices);

	SEOUL_UNITTESTING_ASSERT_EQUAL(6, vIndices.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vIndices[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[3]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[4]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, vIndices[5]);

	SEOUL_UNITTESTING_ASSERT_EQUAL(4u, vVertices.GetSize());
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(1, 0)), vVertices[0], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(1, 1)), vVertices[1], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(0, 1)), vVertices[2], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(0, 0)), vVertices[3], kfAboutEqualPosition);

	stack.Pop();
	TestClipStackNoneCommon(stack);
}

void FalconClipperTest::TestClipStackRectangleOneLevelMulti()
{
	using namespace Falcon;

	ClipStack stack;
	stack.AddRectangle(Rectangle::Create(0, 1, 0, 1));
	stack.AddRectangle(Rectangle::Create(1, 2, 1, 2));
	SEOUL_UNITTESTING_ASSERT(!stack.HasClips());
	SEOUL_UNITTESTING_ASSERT(stack.Push());
	SEOUL_UNITTESTING_ASSERT(stack.HasClips());

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, stack.GetTopClip().m_Bounds.m_fLeft);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, stack.GetTopClip().m_Bounds.m_fRight);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, stack.GetTopClip().m_Bounds.m_fTop);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, stack.GetTopClip().m_Bounds.m_fBottom);
	SEOUL_UNITTESTING_ASSERT(!stack.GetTopClip().m_bSimple);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uFirstHull);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uFirstVertex);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2u, stack.GetTopClip().m_uHulls);
	SEOUL_UNITTESTING_ASSERT_EQUAL(8u, stack.GetTopClip().m_uVertices);

	Clipper::Indices vIndices;
	vIndices.PushBack(0);
	vIndices.PushBack(1);
	vIndices.PushBack(2);
	vIndices.PushBack(0);
	vIndices.PushBack(2);
	vIndices.PushBack(3);

	Clipper::Vertices vVertices;
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 5, -5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 5,  5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-5,  5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-5, -5)));

	stack.MeshClip(TriangleListDescription::kQuadList, vIndices, vVertices);

	SEOUL_UNITTESTING_ASSERT_EQUAL(12, vIndices.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vIndices[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[3]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[4]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, vIndices[5]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, vIndices[6]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, vIndices[7]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(6, vIndices[8]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, vIndices[9]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(6, vIndices[10]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(7, vIndices[11]);

	SEOUL_UNITTESTING_ASSERT_EQUAL(8u, vVertices.GetSize());
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(0, 0)), vVertices[0], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(1, 0)), vVertices[1], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(1, 1)), vVertices[2], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(0, 1)), vVertices[3], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(1, 1)), vVertices[4], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(2, 1)), vVertices[5], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(2, 2)), vVertices[6], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(1, 2)), vVertices[7], kfAboutEqualPosition);

	stack.Pop();
	TestClipStackNoneCommon(stack);
}

void FalconClipperTest::TestClipStackRectangleOneLevelMultiNoneClipAllClip()
{
	using namespace Falcon;

	ClipStack stack;
	stack.AddRectangle(Rectangle::Create(0, 1, 0, 1));
	stack.AddRectangle(Rectangle::Create(1, 2, 1, 2));
	SEOUL_UNITTESTING_ASSERT(!stack.HasClips());
	SEOUL_UNITTESTING_ASSERT(stack.Push());
	SEOUL_UNITTESTING_ASSERT(stack.HasClips());

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, stack.GetTopClip().m_Bounds.m_fLeft);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, stack.GetTopClip().m_Bounds.m_fRight);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, stack.GetTopClip().m_Bounds.m_fTop);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, stack.GetTopClip().m_Bounds.m_fBottom);
	SEOUL_UNITTESTING_ASSERT(!stack.GetTopClip().m_bSimple);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uFirstHull);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uFirstVertex);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2u, stack.GetTopClip().m_uHulls);
	SEOUL_UNITTESTING_ASSERT_EQUAL(8u, stack.GetTopClip().m_uVertices);

	Clipper::Indices vIndices;
	vIndices.PushBack(0);
	vIndices.PushBack(1);
	vIndices.PushBack(2);
	vIndices.PushBack(0);
	vIndices.PushBack(2);
	vIndices.PushBack(3);

	Clipper::Vertices vVertices;
	vVertices.PushBack(ShapeVertex::Create(Vector2D(2, 1)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(2, 2)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(1, 2)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(1, 1)));

	stack.MeshClip(TriangleListDescription::kQuadList, vIndices, vVertices);

	// Shape should be left unmodified.
	SEOUL_UNITTESTING_ASSERT_EQUAL(6, vIndices.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vIndices[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[3]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[4]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, vIndices[5]);

	SEOUL_UNITTESTING_ASSERT_EQUAL(4u, vVertices.GetSize());
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(2, 1)), vVertices[0], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(2, 2)), vVertices[1], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(1, 2)), vVertices[2], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(1, 1)), vVertices[3], kfAboutEqualPosition);

	stack.Pop();
	TestClipStackNoneCommon(stack);
}

void FalconClipperTest::TestClipStackRectangleTwoLevels()
{
	using namespace Falcon;

	ClipStack stack;
	stack.AddRectangle(Rectangle::Create(0, 1, 0, 1));
	SEOUL_UNITTESTING_ASSERT(!stack.HasClips());
	SEOUL_UNITTESTING_ASSERT(stack.Push());
	SEOUL_UNITTESTING_ASSERT(stack.HasClips());

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, stack.GetTopClip().m_Bounds.m_fLeft);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, stack.GetTopClip().m_Bounds.m_fRight);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, stack.GetTopClip().m_Bounds.m_fTop);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, stack.GetTopClip().m_Bounds.m_fBottom);
	SEOUL_UNITTESTING_ASSERT(stack.GetTopClip().m_bSimple);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uFirstHull);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uFirstVertex);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uHulls);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uVertices);

	stack.AddRectangle(Rectangle::Create(-5, 4, -3, 2));
	SEOUL_UNITTESTING_ASSERT(stack.HasClips());
	SEOUL_UNITTESTING_ASSERT(stack.Push());
	SEOUL_UNITTESTING_ASSERT(stack.HasClips());

	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(0, stack.GetTopClip().m_Bounds.m_fLeft, kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(1, stack.GetTopClip().m_Bounds.m_fRight, kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(0, stack.GetTopClip().m_Bounds.m_fTop, kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(1, stack.GetTopClip().m_Bounds.m_fBottom, kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT(stack.GetTopClip().m_bSimple);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uFirstHull);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uFirstVertex);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uHulls);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uVertices);

	Clipper::Indices vIndices;
	vIndices.PushBack(0);
	vIndices.PushBack(1);
	vIndices.PushBack(2);
	vIndices.PushBack(0);
	vIndices.PushBack(2);
	vIndices.PushBack(3);

	Clipper::Vertices vVertices;
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 5, -5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 5,  5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-5,  5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-5, -5)));

	stack.MeshClip(TriangleListDescription::kQuadList, vIndices, vVertices);

	SEOUL_UNITTESTING_ASSERT_EQUAL(6, vIndices.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vIndices[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[3]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[4]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, vIndices[5]);

	SEOUL_UNITTESTING_ASSERT_EQUAL(4u, vVertices.GetSize());
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(1, 0)), vVertices[0], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(1, 1)), vVertices[1], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(0, 1)), vVertices[2], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(0, 0)), vVertices[3], kfAboutEqualPosition);

	stack.Pop();
	stack.Pop();
	TestClipStackNoneCommon(stack);
}

void FalconClipperTest::TestClipStackRectangleTwoLevelsAllClipped()
{
	using namespace Falcon;

	ClipStack stack;
	stack.AddRectangle(Rectangle::Create(0, 1, 0, 1));
	SEOUL_UNITTESTING_ASSERT(!stack.HasClips());
	SEOUL_UNITTESTING_ASSERT(stack.Push());
	SEOUL_UNITTESTING_ASSERT(stack.HasClips());

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, stack.GetTopClip().m_Bounds.m_fLeft);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, stack.GetTopClip().m_Bounds.m_fRight);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, stack.GetTopClip().m_Bounds.m_fTop);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, stack.GetTopClip().m_Bounds.m_fBottom);
	SEOUL_UNITTESTING_ASSERT(stack.GetTopClip().m_bSimple);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uFirstHull);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uFirstVertex);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uHulls);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uVertices);

	stack.AddRectangle(Rectangle::Create(1, 2, 1, 2));
	SEOUL_UNITTESTING_ASSERT(stack.HasClips());

	// This push should fail, as the rectangle is entirely clipped, and
	// the clip stack should keep the first push only.
	SEOUL_UNITTESTING_ASSERT(!stack.Push());
	SEOUL_UNITTESTING_ASSERT(stack.HasClips());

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, stack.GetTopClip().m_Bounds.m_fLeft);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, stack.GetTopClip().m_Bounds.m_fRight);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, stack.GetTopClip().m_Bounds.m_fTop);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, stack.GetTopClip().m_Bounds.m_fBottom);
	SEOUL_UNITTESTING_ASSERT(stack.GetTopClip().m_bSimple);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uFirstHull);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uFirstVertex);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uHulls);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uVertices);

	Clipper::Indices vIndices;
	vIndices.PushBack(0);
	vIndices.PushBack(1);
	vIndices.PushBack(2);
	vIndices.PushBack(0);
	vIndices.PushBack(2);
	vIndices.PushBack(3);

	Clipper::Vertices vVertices;
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 5, -5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 5,  5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-5,  5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-5, -5)));

	stack.MeshClip(TriangleListDescription::kQuadList, vIndices, vVertices);

	SEOUL_UNITTESTING_ASSERT_EQUAL(6, vIndices.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vIndices[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[3]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[4]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, vIndices[5]);

	SEOUL_UNITTESTING_ASSERT_EQUAL(4u, vVertices.GetSize());
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(1, 0)), vVertices[0], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(1, 1)), vVertices[1], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(0, 1)), vVertices[2], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(0, 0)), vVertices[3], kfAboutEqualPosition);

	stack.Pop();
	TestClipStackNoneCommon(stack);
}

void FalconClipperTest::TestClipStackRectangleTwoLevelsMulti()
{
	using namespace Falcon;

	ClipStack stack;
	stack.AddRectangle(Rectangle::Create(0, 1, 0, 1));
	stack.AddRectangle(Rectangle::Create(1, 2, 1, 2));
	SEOUL_UNITTESTING_ASSERT(!stack.HasClips());
	SEOUL_UNITTESTING_ASSERT(stack.Push());
	SEOUL_UNITTESTING_ASSERT(stack.HasClips());

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, stack.GetTopClip().m_Bounds.m_fLeft);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, stack.GetTopClip().m_Bounds.m_fRight);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, stack.GetTopClip().m_Bounds.m_fTop);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, stack.GetTopClip().m_Bounds.m_fBottom);
	SEOUL_UNITTESTING_ASSERT(!stack.GetTopClip().m_bSimple);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uFirstHull);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uFirstVertex);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2u, stack.GetTopClip().m_uHulls);
	SEOUL_UNITTESTING_ASSERT_EQUAL(8u, stack.GetTopClip().m_uVertices);

	stack.AddRectangle(Rectangle::Create(-100, 100, -100, 100));
	SEOUL_UNITTESTING_ASSERT(stack.HasClips());
	SEOUL_UNITTESTING_ASSERT(stack.Push());
	SEOUL_UNITTESTING_ASSERT(stack.HasClips());

	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(0, stack.GetTopClip().m_Bounds.m_fLeft, kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(2, stack.GetTopClip().m_Bounds.m_fRight, kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(0, stack.GetTopClip().m_Bounds.m_fTop, kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(2, stack.GetTopClip().m_Bounds.m_fBottom, kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT(!stack.GetTopClip().m_bSimple);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2u, stack.GetTopClip().m_uFirstHull);
	SEOUL_UNITTESTING_ASSERT_EQUAL(8u, stack.GetTopClip().m_uFirstVertex);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2u, stack.GetTopClip().m_uHulls);
	SEOUL_UNITTESTING_ASSERT_EQUAL(8u, stack.GetTopClip().m_uVertices);

	Clipper::Indices vIndices;
	vIndices.PushBack(0);
	vIndices.PushBack(1);
	vIndices.PushBack(2);
	vIndices.PushBack(0);
	vIndices.PushBack(2);
	vIndices.PushBack(3);

	Clipper::Vertices vVertices;
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 5, -5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 5,  5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-5,  5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-5, -5)));

	stack.MeshClip(TriangleListDescription::kQuadList, vIndices, vVertices);

	SEOUL_UNITTESTING_ASSERT_EQUAL(12, vIndices.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vIndices[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[3]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[4]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, vIndices[5]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, vIndices[6]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, vIndices[7]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(6, vIndices[8]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, vIndices[9]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(6, vIndices[10]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(7, vIndices[11]);

	SEOUL_UNITTESTING_ASSERT_EQUAL(8u, vVertices.GetSize());
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(0, 1)), vVertices[0], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(0, 0)), vVertices[1], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(1, 0)), vVertices[2], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(1, 1)), vVertices[3], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(1, 2)), vVertices[4], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(1, 1)), vVertices[5], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(2, 1)), vVertices[6], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(2, 2)), vVertices[7], kfAboutEqualPosition);

	stack.Pop();
	stack.Pop();
	TestClipStackNoneCommon(stack);
}

void FalconClipperTest::TestClipStackRectangleTwoLevelsNoneClipAllClip()
{
	using namespace Falcon;

	ClipStack stack;
	stack.AddRectangle(Rectangle::Create(0, 1, 0, 1));
	stack.AddRectangle(Rectangle::Create(1, 2, 1, 2));
	SEOUL_UNITTESTING_ASSERT(!stack.HasClips());
	SEOUL_UNITTESTING_ASSERT(stack.Push());
	SEOUL_UNITTESTING_ASSERT(stack.HasClips());

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, stack.GetTopClip().m_Bounds.m_fLeft);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, stack.GetTopClip().m_Bounds.m_fRight);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, stack.GetTopClip().m_Bounds.m_fTop);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, stack.GetTopClip().m_Bounds.m_fBottom);
	SEOUL_UNITTESTING_ASSERT(!stack.GetTopClip().m_bSimple);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uFirstHull);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uFirstVertex);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2u, stack.GetTopClip().m_uHulls);
	SEOUL_UNITTESTING_ASSERT_EQUAL(8u, stack.GetTopClip().m_uVertices);

	stack.AddRectangle(Rectangle::Create(1, 2, 1, 2));
	SEOUL_UNITTESTING_ASSERT(stack.HasClips());

	// This push should succeed, as the second rectangle of the existing
	// stack frame entire encloses the rectangle.
	SEOUL_UNITTESTING_ASSERT(stack.Push());
	SEOUL_UNITTESTING_ASSERT(stack.HasClips());

	SEOUL_UNITTESTING_ASSERT_EQUAL(1, stack.GetTopClip().m_Bounds.m_fLeft);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, stack.GetTopClip().m_Bounds.m_fRight);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, stack.GetTopClip().m_Bounds.m_fTop);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, stack.GetTopClip().m_Bounds.m_fBottom);
	SEOUL_UNITTESTING_ASSERT(stack.GetTopClip().m_bSimple);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uFirstHull);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uFirstVertex);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uHulls);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uVertices);

	Clipper::Indices vIndices;
	vIndices.PushBack(0);
	vIndices.PushBack(1);
	vIndices.PushBack(2);
	vIndices.PushBack(0);
	vIndices.PushBack(2);
	vIndices.PushBack(3);

	Clipper::Vertices vVertices;
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 5, -5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 5,  5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-5,  5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-5, -5)));

	stack.MeshClip(TriangleListDescription::kQuadList, vIndices, vVertices);

	SEOUL_UNITTESTING_ASSERT_EQUAL(6, vIndices.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vIndices[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[3]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[4]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, vIndices[5]);

	SEOUL_UNITTESTING_ASSERT_EQUAL(4u, vVertices.GetSize());
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(2, 1)), vVertices[0], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(2, 2)), vVertices[1], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(1, 2)), vVertices[2], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(1, 1)), vVertices[3], kfAboutEqualPosition);

	stack.Pop();
	stack.Pop();
	TestClipStackNoneCommon(stack);
}

void FalconClipperTest::TestConvexRectangleBasic()
{
	using namespace Falcon;

	// ShapeVertex
	{
		ShapeVertex aVertices[] =
		{
			ShapeVertex::Create(Vector2D( 5, -5)),
			ShapeVertex::Create(Vector2D( 5,  5)),
			ShapeVertex::Create(Vector2D(-5,  5)),
			ShapeVertex::Create(Vector2D(-5, -5)),
		};

		SEOUL_UNITTESTING_ASSERT_EQUAL(4, Clipper::ConvexClip(
			Rectangle::Create(0, 1, 0, 1),
			aVertices,
			4,
			aVertices));

		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(1, 0)), aVertices[0], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(1, 1)), aVertices[1], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(0, 1)), aVertices[2], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(0, 0)), aVertices[3], kfAboutEqualPosition);
	}

	// Vector2D
	{
		Vector2D aVertices[] =
		{
			Vector2D( 5, -5),
			Vector2D( 5,  5),
			Vector2D(-5,  5),
			Vector2D(-5, -5),
		};

		SEOUL_UNITTESTING_ASSERT_EQUAL(4, Clipper::ConvexClip(
			Rectangle::Create(0, 1, 0, 1),
			aVertices,
			4,
			aVertices));

		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Vector2D(1, 0), aVertices[0], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Vector2D(1, 1), aVertices[1], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Vector2D(0, 1), aVertices[2], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Vector2D(0, 0), aVertices[3], kfAboutEqualPosition);
	}
}

void FalconClipperTest::TestConvexRectanglePartial()
{
	using namespace Falcon;

	// ShapeVertex
	{
		ShapeVertex aVertices[] =
		{
			ShapeVertex::Create(Vector2D( 0, -3)),
			ShapeVertex::Create(Vector2D( 3,  0)),
			ShapeVertex::Create(Vector2D( 0,  3)),
			ShapeVertex::Create(Vector2D(-3,  0)),
		};

		ShapeVertex aOutVertices[8];

		SEOUL_UNITTESTING_ASSERT_EQUAL(8, Clipper::ConvexClip(
			Rectangle::Create(-2, 2, -2, 2),
			aVertices,
			4,
			aOutVertices));

		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-2, -1)), aOutVertices[0], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-1, -2)), aOutVertices[1], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 1, -2)), aOutVertices[2], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 2, -1)), aOutVertices[3], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 2,  1)), aOutVertices[4], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 1,  2)), aOutVertices[5], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-1,  2)), aOutVertices[6], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-2,  1)), aOutVertices[7], kfAboutEqualPosition);
	}

	// Vector2D
	{
		Vector2D aVertices[] =
		{
			Vector2D( 0, -3),
			Vector2D( 3,  0),
			Vector2D( 0,  3),
			Vector2D(-3,  0),
		};

		Vector2D aOutVertices[8];

		SEOUL_UNITTESTING_ASSERT_EQUAL(8, Clipper::ConvexClip(
			Rectangle::Create(-2, 2, -2, 2),
			aVertices,
			4,
			aOutVertices));

		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Vector2D(-2, -1), aOutVertices[0], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Vector2D(-1, -2), aOutVertices[1], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Vector2D( 1, -2), aOutVertices[2], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Vector2D( 2, -1), aOutVertices[3], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Vector2D( 2,  1), aOutVertices[4], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Vector2D( 1,  2), aOutVertices[5], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Vector2D(-1,  2), aOutVertices[6], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Vector2D(-2,  1), aOutVertices[7], kfAboutEqualPosition);
	}
}

void FalconClipperTest::TestConvexVerticesBasic()
{
	using namespace Falcon;

	// ShapeVertex
	{
		Vector2D aClipVertices[] =
		{
			Vector2D( 1,  0),
			Vector2D( 0,  1),
			Vector2D(-1,  0),
			Vector2D( 0, -1),
		};
		Vector3D aClipPlanes[SEOUL_ARRAY_COUNT(aClipVertices)];
		Clipper::ComputeClipPlanes(aClipVertices, SEOUL_ARRAY_COUNT(aClipPlanes), aClipPlanes);

		ShapeVertex aVertices[] =
		{
			ShapeVertex::Create(Vector2D( 5, -5)),
			ShapeVertex::Create(Vector2D( 5,  5)),
			ShapeVertex::Create(Vector2D(-5,  5)),
			ShapeVertex::Create(Vector2D(-5, -5)),
		};

		SEOUL_UNITTESTING_ASSERT_EQUAL(4, Clipper::ConvexClip(
			aClipPlanes,
			SEOUL_ARRAY_COUNT(aClipPlanes),
			aVertices,
			4,
			aVertices));

		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 0, -1)), aVertices[0], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 1,  0)), aVertices[1], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 0,  1)), aVertices[2], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-1,  0)), aVertices[3], kfAboutEqualPosition);
	}

	// Vector2D
	{
		Vector2D aClipVertices[] =
		{
			Vector2D( 1,  0),
			Vector2D( 0,  1),
			Vector2D(-1,  0),
			Vector2D( 0, -1),
		};
		Vector3D aClipPlanes[SEOUL_ARRAY_COUNT(aClipVertices)];
		Clipper::ComputeClipPlanes(aClipVertices, SEOUL_ARRAY_COUNT(aClipPlanes), aClipPlanes);

		Vector2D aVertices[] =
		{
			Vector2D( 5, -5),
			Vector2D( 5,  5),
			Vector2D(-5,  5),
			Vector2D(-5, -5),
		};

		SEOUL_UNITTESTING_ASSERT_EQUAL(4, Clipper::ConvexClip(
			aClipPlanes,
			SEOUL_ARRAY_COUNT(aClipPlanes),
			aVertices,
			4,
			aVertices));

		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Vector2D( 0, -1), aVertices[0], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Vector2D( 1,  0), aVertices[1], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Vector2D( 0,  1), aVertices[2], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Vector2D(-1,  0), aVertices[3], kfAboutEqualPosition);
	}
}

void FalconClipperTest::TestConvexVerticesPartial()
{
	using namespace Falcon;

	// ShapeVertex
	{
		Vector2D aClipVertices[] =
		{
			Vector2D( 0, -3),
			Vector2D( 3,  0),
			Vector2D( 0,  3),
			Vector2D(-3,  0),
		};
		Vector3D aClipPlanes[SEOUL_ARRAY_COUNT(aClipVertices)];
		Clipper::ComputeClipPlanes(aClipVertices, SEOUL_ARRAY_COUNT(aClipPlanes), aClipPlanes);

		ShapeVertex aVertices[] =
		{
			ShapeVertex::Create(Vector2D( 2, -2)),
			ShapeVertex::Create(Vector2D( 2,  2)),
			ShapeVertex::Create(Vector2D(-2,  2)),
			ShapeVertex::Create(Vector2D(-2, -2)),
		};

		ShapeVertex aOutVertices[8];

		SEOUL_UNITTESTING_ASSERT_EQUAL(8, Clipper::ConvexClip(
			aClipPlanes,
			SEOUL_ARRAY_COUNT(aClipPlanes),
			aVertices,
			4,
			aOutVertices));

		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-1, -2)), aOutVertices[0], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 1, -2)), aOutVertices[1], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 2, -1)), aOutVertices[2], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 2,  1)), aOutVertices[3], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 1,  2)), aOutVertices[4], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-1,  2)), aOutVertices[5], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-2,  1)), aOutVertices[6], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-2, -1)), aOutVertices[7], kfAboutEqualPosition);
	}

	// Vector2D
	{
		Vector2D aClipVertices[] =
		{
			Vector2D( 0, -3),
			Vector2D( 3,  0),
			Vector2D( 0,  3),
			Vector2D(-3,  0),
		};
		Vector3D aClipPlanes[SEOUL_ARRAY_COUNT(aClipVertices)];
		Clipper::ComputeClipPlanes(aClipVertices, SEOUL_ARRAY_COUNT(aClipPlanes), aClipPlanes);

		Vector2D aVertices[] =
		{
			Vector2D( 2, -2),
			Vector2D( 2,  2),
			Vector2D(-2,  2),
			Vector2D(-2, -2),
		};

		Vector2D aOutVertices[8];

		SEOUL_UNITTESTING_ASSERT_EQUAL(8, Clipper::ConvexClip(
			aClipPlanes,
			SEOUL_ARRAY_COUNT(aClipPlanes),
			aVertices,
			4,
			aOutVertices));

		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Vector2D(-1, -2), aOutVertices[0], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Vector2D( 1, -2), aOutVertices[1], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Vector2D( 2, -1), aOutVertices[2], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Vector2D( 2,  1), aOutVertices[3], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Vector2D( 1,  2), aOutVertices[4], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Vector2D(-1,  2), aOutVertices[5], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Vector2D(-2,  1), aOutVertices[6], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(Vector2D(-2, -1), aOutVertices[7], kfAboutEqualPosition);
	}
}

void FalconClipperTest::TestEmpty()
{
	using namespace Falcon;

	// No vertices, no clip.
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, Clipper::ConvexClip(
		Rectangle::Create(0, 1, 0, 1),
		(ShapeVertex*)nullptr,
		0u,
		nullptr));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, Clipper::ConvexClip(
		Rectangle::Create(0, 1, 0, 1),
		(Vector2D*)nullptr,
		0u,
		nullptr));

	// No clip planes, all clipped.
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1, Clipper::ConvexClip(
		(Vector3D const*)nullptr,
		0u,
		(ShapeVertex*)nullptr,
		0u,
		nullptr));
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1, Clipper::ConvexClip(
		(Vector3D const*)nullptr,
		0u,
		(Vector2D*)nullptr,
		0u,
		nullptr));

	// No clip vertices, no clip planes.
	Clipper::ComputeClipPlanes(
		nullptr,
		0u,
		nullptr);
}

void FalconClipperTest::TestMeshRectangleConvex()
{
	using namespace Falcon;

	Clipper::Indices vIndices;
	vIndices.PushBack(0);
	vIndices.PushBack(1);
	vIndices.PushBack(2);
	vIndices.PushBack(0);
	vIndices.PushBack(2);
	vIndices.PushBack(3);

	Clipper::Vertices vVertices;
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 0, -3)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 3,  0)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 0,  3)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-3,  0)));

	auto pCache(Clipper::NewMeshClipCache<StandardVertex2D>());
	{
		Clipper::Indices vIndices2(vIndices);
		Clipper::Vertices vVertices2(vVertices);

		Clipper::MeshClip(
			*pCache,
			Rectangle::Create(-2, 2, -2, 2),
			TriangleListDescription::kQuadList,
			Rectangle::Create(-3, 3, -3, 3),
			vIndices2,
			vIndices2.GetSize(),
			vVertices2,
			vVertices2.GetSize());
		Clipper::MeshClip(
			*pCache,
			Rectangle::Create(-2, 2, -2, 2),
			TriangleListDescription::kQuadList,
			vIndices,
			vIndices.GetSize(),
			vVertices,
			vVertices.GetSize());

		SEOUL_UNITTESTING_ASSERT_EQUAL(vIndices, vIndices2);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vVertices, vVertices2);
	}
	Clipper::DestroyMeshClipCache(pCache);

	SEOUL_UNITTESTING_ASSERT_EQUAL(18, vIndices.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(8, vVertices.GetSize());

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vIndices[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[3]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[4]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, vIndices[5]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[6]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, vIndices[7]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, vIndices[8]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[9]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, vIndices[10]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, vIndices[11]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[12]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, vIndices[13]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(6, vIndices[14]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[15]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(6, vIndices[16]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(7, vIndices[17]);

	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-2, -1)), vVertices[0], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-1, -2)), vVertices[1], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 1, -2)), vVertices[2], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 2, -1)), vVertices[3], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 2,  1)), vVertices[4], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 1,  2)), vVertices[5], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-1,  2)), vVertices[6], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-2,  1)), vVertices[7], kfAboutEqualPosition);
}

void FalconClipperTest::TestMeshRectangleConvexClipAll()
{
	using namespace Falcon;

	Clipper::Indices vIndices;
	vIndices.PushBack(0);
	vIndices.PushBack(1);
	vIndices.PushBack(2);
	vIndices.PushBack(0);
	vIndices.PushBack(2);
	vIndices.PushBack(3);

	Clipper::Vertices vVertices;
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 0, -3)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 3,  0)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 0,  3)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-3,  0)));

	auto pCache(Clipper::NewMeshClipCache<StandardVertex2D>());
	{
		Clipper::Indices vIndices2(vIndices);
		Clipper::Vertices vVertices2(vVertices);

		Clipper::MeshClip(
			*pCache,
			Rectangle::Create(3, 5, 3, 5),
			TriangleListDescription::kQuadList,
			Rectangle::Create(-3, 3, -3, 3),
			vIndices2,
			vIndices2.GetSize(),
			vVertices2,
			vVertices2.GetSize());
		Clipper::MeshClip(
			*pCache,
			Rectangle::Create(3, 5, 3, 5),
			TriangleListDescription::kQuadList,
			vIndices,
			vIndices.GetSize(),
			vVertices,
			vVertices.GetSize());

		SEOUL_UNITTESTING_ASSERT_EQUAL(vIndices, vIndices2);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vVertices, vVertices2);
	}
	Clipper::DestroyMeshClipCache(pCache);

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vVertices.GetSize());
}

void FalconClipperTest::TestMeshRectangleConvexClipNone()
{
	using namespace Falcon;

	Clipper::Indices vIndices;
	vIndices.PushBack(0);
	vIndices.PushBack(1);
	vIndices.PushBack(2);
	vIndices.PushBack(0);
	vIndices.PushBack(2);
	vIndices.PushBack(3);

	Clipper::Vertices vVertices;
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 0, -3)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 3,  0)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 0,  3)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-3,  0)));

	auto pCache(Clipper::NewMeshClipCache<StandardVertex2D>());
	{
		Clipper::Indices vIndices2(vIndices);
		Clipper::Vertices vVertices2(vVertices);

		Clipper::MeshClip(
			*pCache,
			Rectangle::Create(-3, 3, -3, 3),
			TriangleListDescription::kQuadList,
			Rectangle::Create(-3, 3, -3, 3),
			vIndices2,
			vIndices2.GetSize(),
			vVertices2,
			vVertices2.GetSize());
		Clipper::MeshClip(
			*pCache,
			Rectangle::Create(-3, 3, -3, 3),
			TriangleListDescription::kQuadList,
			vIndices,
			vIndices.GetSize(),
			vVertices,
			vVertices.GetSize());

		SEOUL_UNITTESTING_ASSERT_EQUAL(vIndices, vIndices2);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vVertices, vVertices2);
	}
	Clipper::DestroyMeshClipCache(pCache);

	SEOUL_UNITTESTING_ASSERT_EQUAL(6, vIndices.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, vVertices.GetSize());

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vIndices[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[3]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[4]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, vIndices[5]);

	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 0, -3)), vVertices[0], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 3,  0)), vVertices[1], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 0,  3)), vVertices[2], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-3,  0)), vVertices[3], kfAboutEqualPosition);
}

static const UInt16 s_kaNotSpecificExpectedIndices[] =
{
	0, 1, 2,
	3, 4, 2,
	3, 2, 1,
	4, 3, 5,
	4, 5, 6,
	4, 6, 7,
	8, 9, 10,
	8, 10, 5,
	8, 5, 3,
	11, 12, 13,
	11, 13, 10,
	11, 10, 9,
	14, 15, 13,
	14, 13, 12,
	16, 17, 15,
	16, 15, 14,
};
static const Vector2D s_kaNotSpecificExpectedVertices[] =
{
	Vector2D(300.0f, 300.0f),
	Vector2D(250.0f, 300.0f),
	Vector2D(300.0f, 225.0f),
	Vector2D(200.0f, 250.0f),
	Vector2D(300.0f, 183.33333f),
	Vector2D(200.0f, 100.0f),
	Vector2D(275.0f, 100.0f),
	Vector2D(300.0f, 116.66666f),
	Vector2D(175.0f, 300.0f),
	Vector2D(158.33334f, 300.0f),
	Vector2D(191.66667f, 100.0f),
	Vector2D(125.0f, 300.0f),
	Vector2D(100.0f, 250.0f),
	Vector2D(175.0f, 100.0f),
	Vector2D(100.0f, 200.0f),
	Vector2D(166.66666f, 100.0f),
	Vector2D(100.0f, 116.66666f),
	Vector2D(124.99999f, 100.0f),
};

void FalconClipperTest::TestMeshRectangleNotSpecific()
{
	using namespace Falcon;

	FalconClipperTestTesselationCallback callback;

	{
		FillStyle style;
		style.m_eFillStyleType = FillStyleType::kSoldFill;

		// Generate triangles for a concave shape.
		Tesselator tesselator(callback);
		tesselator.BeginShape();
		tesselator.BeginPath(&style, nullptr, nullptr, Vector2D(50, 150));
		tesselator.AddLine(Vector2D(100, 200));
		tesselator.AddLine(Vector2D(100, 250));
		tesselator.AddLine(Vector2D(150, 350));
		tesselator.AddLine(Vector2D(200, 250));
		tesselator.AddLine(Vector2D(250, 300));
		tesselator.AddLine(Vector2D(350, 300));
		tesselator.AddLine(Vector2D(350, 150));
		tesselator.AddLine(Vector2D(200, 50));
		tesselator.AddLine(Vector2D(50, 150));
		tesselator.EndPath();
		tesselator.EndShape();
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(21, callback.m_vIndices.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(9, callback.m_vVertices.GetSize());

	Clipper::Indices vIndices(callback.m_vIndices.Begin(), callback.m_vIndices.End());
	Clipper::Vertices vVertices;
	vVertices.Reserve(callback.m_vVertices.GetSize());
	for (auto i = callback.m_vVertices.Begin(); callback.m_vVertices.End() != i; ++i)
	{
		vVertices.PushBack(ShapeVertex::Create(*i));
	}

	auto pCache(Clipper::NewMeshClipCache<StandardVertex2D>());
	{
		Clipper::Indices vIndices2(vIndices);
		Clipper::Vertices vVertices2(vVertices);

		Clipper::MeshClip(
			*pCache,
			Rectangle::Create(100, 300, 100, 300),
			TriangleListDescription::kNotSpecific,
			Rectangle::Create(50, 350, 50, 350),
			vIndices2,
			vIndices2.GetSize(),
			vVertices2,
			vVertices.GetSize());
		Clipper::MeshClip(
			*pCache,
			Rectangle::Create(100, 300, 100, 300),
			TriangleListDescription::kNotSpecific,
			vIndices,
			vIndices.GetSize(),
			vVertices,
			vVertices.GetSize());

		SEOUL_UNITTESTING_ASSERT_EQUAL(vIndices, vIndices2);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vVertices, vVertices2);
	}
	Clipper::DestroyMeshClipCache(pCache);

	SEOUL_UNITTESTING_ASSERT_EQUAL(SEOUL_ARRAY_COUNT(s_kaNotSpecificExpectedIndices), vIndices.GetSize());
	for (UInt32 i = 0u; i < vIndices.GetSize(); ++i)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(s_kaNotSpecificExpectedIndices[i], vIndices[i]);
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(SEOUL_ARRAY_COUNT(s_kaNotSpecificExpectedVertices), vVertices.GetSize());
	for (UInt32 i = 0u; i < vVertices.GetSize(); ++i)
	{
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(s_kaNotSpecificExpectedVertices[i], vVertices[i].m_vP, kfAboutEqualPosition);
	}
}

void FalconClipperTest::TestMeshRectangleQuadList()
{
	using namespace Falcon;

	Clipper::Indices vIndices;
	vIndices.PushBack(0);
	vIndices.PushBack(1);
	vIndices.PushBack(2);
	vIndices.PushBack(0);
	vIndices.PushBack(2);
	vIndices.PushBack(3);

	Clipper::Vertices vVertices;
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-5, -5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-5,  5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 5,  5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 5, -5)));

	auto pCache(Clipper::NewMeshClipCache<StandardVertex2D>());
	{
		Clipper::Indices vIndices2(vIndices);
		Clipper::Vertices vVertices2(vVertices);

		Clipper::MeshClip(
			*pCache,
			Rectangle::Create(0, 1, 0, 1),
			TriangleListDescription::kQuadList,
			Rectangle::Create(-5, 5, -5, 5),
			vIndices2,
			vIndices2.GetSize(),
			vVertices2,
			vVertices2.GetSize());
		Clipper::MeshClip(
			*pCache,
			Rectangle::Create(0, 1, 0, 1),
			TriangleListDescription::kQuadList,
			vIndices,
			vIndices.GetSize(),
			vVertices,
			vVertices.GetSize());

		SEOUL_UNITTESTING_ASSERT_EQUAL(vIndices, vIndices2);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vVertices, vVertices2);
	}
	Clipper::DestroyMeshClipCache(pCache);

	SEOUL_UNITTESTING_ASSERT_EQUAL(6, vIndices.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, vVertices.GetSize());

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vIndices[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[3]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[4]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, vIndices[5]);

	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(1, 0)), vVertices[0], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(0, 0)), vVertices[1], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(0, 1)), vVertices[2], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(1, 1)), vVertices[3], kfAboutEqualPosition);
}

void FalconClipperTest::TestMeshRectangleQuadListMulti()
{
	using namespace Falcon;

	Clipper::Indices vIndices;
	vIndices.PushBack(0);
	vIndices.PushBack(1);
	vIndices.PushBack(2);
	vIndices.PushBack(0);
	vIndices.PushBack(2);
	vIndices.PushBack(3);
	vIndices.PushBack(4);
	vIndices.PushBack(5);
	vIndices.PushBack(6);
	vIndices.PushBack(4);
	vIndices.PushBack(6);
	vIndices.PushBack(7);

	Clipper::Vertices vVertices;
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-5, -5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-5,  2)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 2,  2)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 2, -5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 2,  2)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 2,  5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 5,  5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 5,  2)));

	auto pCache(Clipper::NewMeshClipCache<StandardVertex2D>());
	{
		Clipper::Indices vIndices2(vIndices);
		Clipper::Vertices vVertices2(vVertices);

		Clipper::MeshClip(
			*pCache,
			Rectangle::Create(0, 3, 0, 3),
			TriangleListDescription::kQuadList,
			Rectangle::Create(-5, 5, -5, 5),
			vIndices2,
			vIndices2.GetSize(),
			vVertices2,
			vVertices2.GetSize());
		Clipper::MeshClip(
			*pCache,
			Rectangle::Create(0, 3, 0, 3),
			TriangleListDescription::kQuadList,
			vIndices,
			vIndices.GetSize(),
			vVertices,
			vVertices.GetSize());

		SEOUL_UNITTESTING_ASSERT_EQUAL(vIndices, vIndices2);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vVertices, vVertices2);
	}
	Clipper::DestroyMeshClipCache(pCache);

	SEOUL_UNITTESTING_ASSERT_EQUAL(12, vIndices.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(8, vVertices.GetSize());

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vIndices[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[3]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[4]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, vIndices[5]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, vIndices[6]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, vIndices[7]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(6, vIndices[8]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, vIndices[9]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(6, vIndices[10]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(7, vIndices[11]);

	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(0, 0)), vVertices[0], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(0, 2)), vVertices[1], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(2, 2)), vVertices[2], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(2, 0)), vVertices[3], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(3, 2)), vVertices[4], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(2, 2)), vVertices[5], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(2, 3)), vVertices[6], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(3, 3)), vVertices[7], kfAboutEqualPosition);
}

void FalconClipperTest::TestMeshVerticesConvex()
{
	using namespace Falcon;

	Vector2D aClipVertices[] =
	{
		Vector2D( 0, -3),
		Vector2D( 3,  0),
		Vector2D( 0,  3),
		Vector2D(-3,  0),
	};
	Vector3D aClipPlanes[SEOUL_ARRAY_COUNT(aClipVertices)];
	Clipper::ComputeClipPlanes(aClipVertices, SEOUL_ARRAY_COUNT(aClipPlanes), aClipPlanes);

	Clipper::Indices vIndices;
	vIndices.PushBack(0);
	vIndices.PushBack(1);
	vIndices.PushBack(2);
	vIndices.PushBack(0);
	vIndices.PushBack(2);
	vIndices.PushBack(3);

	Clipper::Vertices vVertices;
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 2, -2)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 2,  2)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-2,  2)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-2, -2)));

	auto pCache(Clipper::NewMeshClipCache<StandardVertex2D>());
	Clipper::MeshClip(
		*pCache,
		aClipPlanes,
		SEOUL_ARRAY_COUNT(aClipPlanes),
		TriangleListDescription::kQuadList,
		vIndices,
		vIndices.GetSize(),
		vVertices,
		vVertices.GetSize());
	Clipper::DestroyMeshClipCache(pCache);

	SEOUL_UNITTESTING_ASSERT_EQUAL(18, vIndices.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(8, vVertices.GetSize());

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vIndices[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[3]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[4]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, vIndices[5]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[6]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, vIndices[7]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, vIndices[8]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[9]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, vIndices[10]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, vIndices[11]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[12]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, vIndices[13]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(6, vIndices[14]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[15]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(6, vIndices[16]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(7, vIndices[17]);

	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-1, -2)), vVertices[0], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 1, -2)), vVertices[1], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 2, -1)), vVertices[2], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 2,  1)), vVertices[3], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 1,  2)), vVertices[4], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-1,  2)), vVertices[5], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-2,  1)), vVertices[6], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-2, -1)), vVertices[7], kfAboutEqualPosition);
}

void FalconClipperTest::TestMeshVerticesNotSpecific()
{
	using namespace Falcon;

	FalconClipperTestTesselationCallback callback;

	Vector2D aClipVertices[] =
	{
		Vector2D(100, 100),
		Vector2D(300, 100),
		Vector2D(300, 300),
		Vector2D(100, 300),
	};
	Vector3D aClipPlanes[SEOUL_ARRAY_COUNT(aClipVertices)];
	Clipper::ComputeClipPlanes(aClipVertices, SEOUL_ARRAY_COUNT(aClipPlanes), aClipPlanes);

	{
		FillStyle style;
		style.m_eFillStyleType = FillStyleType::kSoldFill;

		// Generate triangles for a concave shape.
		Tesselator tesselator(callback);
		tesselator.BeginShape();
		tesselator.BeginPath(&style, nullptr, nullptr, Vector2D(50, 150));
		tesselator.AddLine(Vector2D(100, 200));
		tesselator.AddLine(Vector2D(100, 250));
		tesselator.AddLine(Vector2D(150, 350));
		tesselator.AddLine(Vector2D(200, 250));
		tesselator.AddLine(Vector2D(250, 300));
		tesselator.AddLine(Vector2D(350, 300));
		tesselator.AddLine(Vector2D(350, 150));
		tesselator.AddLine(Vector2D(200, 50));
		tesselator.AddLine(Vector2D(50, 150));
		tesselator.EndPath();
		tesselator.EndShape();
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(21, callback.m_vIndices.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(9, callback.m_vVertices.GetSize());

	Clipper::Indices vIndices(callback.m_vIndices.Begin(), callback.m_vIndices.End());
	Clipper::Vertices vVertices;
	vVertices.Reserve(callback.m_vVertices.GetSize());
	for (auto i = callback.m_vVertices.Begin(); callback.m_vVertices.End() != i; ++i)
	{
		vVertices.PushBack(ShapeVertex::Create(*i));
		vVertices.Back().m_vT = Vector4D(*i, *i);
	}

	auto pCache(Clipper::NewMeshClipCache<StandardVertex2D>());
	Clipper::MeshClip(
		*pCache,
		aClipPlanes,
		SEOUL_ARRAY_COUNT(aClipPlanes),
		TriangleListDescription::kNotSpecific,
		vIndices,
		vIndices.GetSize(),
		vVertices,
		vVertices.GetSize());
	Clipper::DestroyMeshClipCache(pCache);

	SEOUL_UNITTESTING_ASSERT_EQUAL(SEOUL_ARRAY_COUNT(s_kaNotSpecificExpectedIndices), vIndices.GetSize());
	for (UInt32 i = 0u; i < vIndices.GetSize(); ++i)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(s_kaNotSpecificExpectedIndices[i], vIndices[i]);
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(SEOUL_ARRAY_COUNT(s_kaNotSpecificExpectedVertices), vVertices.GetSize());
	for (UInt32 i = 0u; i < vVertices.GetSize(); ++i)
	{
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(s_kaNotSpecificExpectedVertices[i], vVertices[i].m_vP, kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(s_kaNotSpecificExpectedVertices[i], vVertices[i].m_vT.GetXY(), kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(s_kaNotSpecificExpectedVertices[i], vVertices[i].m_vT.GetZW(), kfAboutEqualPosition);
	}
}

void FalconClipperTest::TestMeshVerticesQuadList()
{
	using namespace Falcon;

	Vector2D aClipVertices[] =
	{
		Vector2D( 1,  0),
		Vector2D( 0,  1),
		Vector2D(-1,  0),
		Vector2D( 0, -1),
	};
	Vector3D aClipPlanes[SEOUL_ARRAY_COUNT(aClipVertices)];
	Clipper::ComputeClipPlanes(aClipVertices, SEOUL_ARRAY_COUNT(aClipPlanes), aClipPlanes);

	Clipper::Indices vIndices;
	vIndices.PushBack(0);
	vIndices.PushBack(1);
	vIndices.PushBack(2);
	vIndices.PushBack(0);
	vIndices.PushBack(2);
	vIndices.PushBack(3);

	Clipper::Vertices vVertices;
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 5, -5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 5,  5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-5,  5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-5, -5)));

	auto pCache(Clipper::NewMeshClipCache<StandardVertex2D>());
	Clipper::MeshClip(
		*pCache,
		aClipPlanes,
		SEOUL_ARRAY_COUNT(aClipPlanes),
		TriangleListDescription::kQuadList,
		vIndices,
		vIndices.GetSize(),
		vVertices,
		vVertices.GetSize());
	Clipper::DestroyMeshClipCache(pCache);

	SEOUL_UNITTESTING_ASSERT_EQUAL(6, vIndices.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, vVertices.GetSize());

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vIndices[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[3]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[4]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, vIndices[5]);

	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 0, -1)), vVertices[0], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 1,  0)), vVertices[1], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D( 0,  1)), vVertices[2], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-1,  0)), vVertices[3], kfAboutEqualPosition);
}

void FalconClipperTest::TestMeshRectangleNotSpecificNotClippingInitially()
{
	using namespace Falcon;

	FalconClipperTestTesselationCallback callback;

	{
		FillStyle style;
		style.m_eFillStyleType = FillStyleType::kSoldFill;

		// Generate triangles for a concave shape.
		Tesselator tesselator(callback);
		tesselator.BeginShape();
		tesselator.BeginPath(&style, nullptr, nullptr, Vector2D(50, 150));
		tesselator.AddLine(Vector2D(100, 200));
		tesselator.AddLine(Vector2D(100, 250));
		tesselator.AddLine(Vector2D(150, 350));
		tesselator.AddLine(Vector2D(200, 250));
		tesselator.AddLine(Vector2D(250, 300));
		tesselator.AddLine(Vector2D(350, 300));
		tesselator.AddLine(Vector2D(350, 150));
		tesselator.AddLine(Vector2D(200, 50));
		tesselator.AddLine(Vector2D(50, 150));
		tesselator.EndPath();
		tesselator.EndShape();
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(21, callback.m_vIndices.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(9, callback.m_vVertices.GetSize());

	Clipper::Indices vIndices(callback.m_vIndices.Begin(), callback.m_vIndices.End());
	Clipper::Vertices vVertices;
	vVertices.Reserve(callback.m_vVertices.GetSize());
	for (auto i = callback.m_vVertices.Begin(); callback.m_vVertices.End() != i; ++i)
	{
		vVertices.PushBack(ShapeVertex::Create(*i));
	}

	auto pCache(Clipper::NewMeshClipCache<StandardVertex2D>());
	{
		Clipper::Indices vIndices2(vIndices);
		Clipper::Vertices vVertices2(vVertices);

		Clipper::MeshClip(
			*pCache,
			Rectangle::Create(250, 350, 150, 300),
			TriangleListDescription::kNotSpecific,
			Rectangle::Create(50, 350, 50, 350),
			vIndices2,
			vIndices2.GetSize(),
			vVertices2,
			vVertices2.GetSize());
		Clipper::MeshClip(
			*pCache,
			Rectangle::Create(250, 350, 150, 300),
			TriangleListDescription::kNotSpecific,
			vIndices,
			vIndices.GetSize(),
			vVertices,
			vVertices.GetSize());

		SEOUL_UNITTESTING_ASSERT_EQUAL(vIndices, vIndices2);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vVertices, vVertices2);
	}
	Clipper::DestroyMeshClipCache(pCache);

	SEOUL_UNITTESTING_ASSERT_EQUAL(9, vIndices.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, vVertices.GetSize());

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vIndices[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, vIndices[3]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vIndices[4]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[5]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, vIndices[6]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, vIndices[7]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vIndices[8]);

	SEOUL_UNITTESTING_ASSERT_EQUAL(ShapeVertex::Create(Vector2D(250, 300)), vVertices[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ShapeVertex::Create(Vector2D(350, 150)), vVertices[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(ShapeVertex::Create(Vector2D(350, 300)), vVertices[2]);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(250, 216.66666f)), vVertices[3], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(250, 150)), vVertices[4], kfAboutEqualPosition);
}

void FalconClipperTest::TestMeshTextChunkNoClip()
{
	using namespace Falcon;

	Clipper::Indices vIndices;
	vIndices.PushBack(0);
	vIndices.PushBack(1);
	vIndices.PushBack(2);
	vIndices.PushBack(0);
	vIndices.PushBack(2);
	vIndices.PushBack(3);
	vIndices.PushBack(4);
	vIndices.PushBack(5);
	vIndices.PushBack(6);
	vIndices.PushBack(4);
	vIndices.PushBack(6);
	vIndices.PushBack(7);

	Clipper::Vertices vVertices;
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-5, -5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-5,  2)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 2,  2)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 2, -5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 2,  2)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 2,  5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 5,  5)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D( 5,  2)));

	auto pCache(Clipper::NewMeshClipCache<StandardVertex2D>());
	{
		Clipper::Indices vIndices2(vIndices);
		Clipper::Vertices vVertices2(vVertices);

		// This clip should not affect the input vertices or indices at all.
		Clipper::MeshClip(
			*pCache,
			Rectangle::Create(-5, 5, -5, 5),
			TriangleListDescription::kTextChunk,
			Rectangle::Create(-5, 5, -5, 5),
			vIndices2,
			vIndices2.GetSize(),
			vVertices2,
			vVertices2.GetSize());
		Clipper::MeshClip(
			*pCache,
			Rectangle::Create(-5, 5, -5, 5),
			TriangleListDescription::kTextChunk,
			vIndices,
			vIndices.GetSize(),
			vVertices,
			vVertices.GetSize());

		SEOUL_UNITTESTING_ASSERT_EQUAL(vIndices, vIndices2);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vVertices, vVertices2);

		SEOUL_UNITTESTING_ASSERT_EQUAL(12, vIndices.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(8, vVertices.GetSize());

		SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[0]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, vIndices[1]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[2]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[3]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[4]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, vIndices[5]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(4, vIndices[6]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, vIndices[7]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, vIndices[8]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(4, vIndices[9]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, vIndices[10]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(7, vIndices[11]);

		SEOUL_UNITTESTING_ASSERT_EQUAL(ShapeVertex::Create(Vector2D(-5, -5)), vVertices[0]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(ShapeVertex::Create(Vector2D(-5,  2)), vVertices[1]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(ShapeVertex::Create(Vector2D( 2,  2)), vVertices[2]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(ShapeVertex::Create(Vector2D( 2, -5)), vVertices[3]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(ShapeVertex::Create(Vector2D( 2,  2)), vVertices[4]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(ShapeVertex::Create(Vector2D( 2,  5)), vVertices[5]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(ShapeVertex::Create(Vector2D( 5,  5)), vVertices[6]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(ShapeVertex::Create(Vector2D( 5,  2)), vVertices[7]);
	}

	{
		// This clip should clip the input vertices.
		Clipper::MeshClip(
			*pCache,
			Rectangle::Create(0, 3, 0, 3),
			TriangleListDescription::kTextChunk,
			vIndices,
			vIndices.GetSize(),
			vVertices,
			vVertices.GetSize());

		SEOUL_UNITTESTING_ASSERT_EQUAL(12, vIndices.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(8, vVertices.GetSize());

		SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[0]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, vIndices[1]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[2]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[3]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[4]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, vIndices[5]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(4, vIndices[6]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, vIndices[7]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, vIndices[8]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(4, vIndices[9]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(6, vIndices[10]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(7, vIndices[11]);

		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(0, 0)), vVertices[0], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(0, 2)), vVertices[1], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(2, 2)), vVertices[2], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(2, 0)), vVertices[3], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(3, 2)), vVertices[4], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(2, 2)), vVertices[5], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(2, 3)), vVertices[6], kfAboutEqualPosition);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(3, 3)), vVertices[7], kfAboutEqualPosition);
	}

	Clipper::DestroyMeshClipCache(pCache);
}

// Regression for a bug in quad list/text chunk clipping,
// when the first quad was clipped but the second
// (or any inner quad) was completely unclipped.
void FalconClipperTest::TestMeshTextChunkClipRegression()
{
	using namespace Falcon;

	static const UInt16 kaIndices[] =
	{
		0,
		1,
		2,
		0,
		2,
		3,
		4,
		5,
		6,
		4,
		6,
		7,
		8,
		9,
		10,
		8,
		10,
		11,
		12,
		13,
		14,
		12,
		14,
		15,
		16,
		17,
		18,
		16,
		18,
		19,
		20,
		21,
		22,
		20,
		22,
		23,
		24,
		25,
		26,
		24,
		26,
		27,
		28,
		29,
		30,
		28,
		30,
		31,
		32,
		33,
		34,
		32,
		34,
		35,
		36,
		37,
		38,
		36,
		38,
		39,
		40,
		41,
		42,
		40,
		42,
		43,
		44,
		45,
		46,
		44,
		46,
		47,
	};

	static ShapeVertex kaVertices[] =
	{
		ShapeVertex::Create(Vector2D(206.69096f, 197.35748f)),
		ShapeVertex::Create(Vector2D(206.69096f, 225.69083f)),
		ShapeVertex::Create(Vector2D(232.52429f, 225.69083f)),
		ShapeVertex::Create(Vector2D(232.52429f, 197.35748f)),
		ShapeVertex::Create(Vector2D(219.11096f, 199.85748f)),
		ShapeVertex::Create(Vector2D(219.11096f, 225.69083f)),
		ShapeVertex::Create(Vector2D(240.77762f, 225.69083f)),
		ShapeVertex::Create(Vector2D(240.77762f, 199.85748f)),
		ShapeVertex::Create(Vector2D(226.35762f, 199.85748f)),
		ShapeVertex::Create(Vector2D(226.35762f, 226.52415f)),
		ShapeVertex::Create(Vector2D(252.19095f, 226.52415f)),
		ShapeVertex::Create(Vector2D(252.19095f, 199.85748f)),
		ShapeVertex::Create(Vector2D(239.13095f, 196.52415f)),
		ShapeVertex::Create(Vector2D(239.13095f, 225.69083f)),
		ShapeVertex::Create(Vector2D(264.13095f, 225.69083f)),
		ShapeVertex::Create(Vector2D(264.13095f, 196.52415f)),
		ShapeVertex::Create(Vector2D(249.21762f, 199.85748f)),
		ShapeVertex::Create(Vector2D(249.21762f, 226.52415f)),
		ShapeVertex::Create(Vector2D(275.05096f, 226.52415f)),
		ShapeVertex::Create(Vector2D(275.05096f, 199.85748f)),
		ShapeVertex::Create(Vector2D(261.49097f, 199.85748f)),
		ShapeVertex::Create(Vector2D(261.49097f, 225.69083f)),
		ShapeVertex::Create(Vector2D(285.65762f, 225.69083f)),
		ShapeVertex::Create(Vector2D(285.65762f, 199.85748f)),
		ShapeVertex::Create(Vector2D(272.09763f, 211.52415f)),
		ShapeVertex::Create(Vector2D(272.09763f, 225.69083f)),
		ShapeVertex::Create(Vector2D(286.26428f, 225.69083f)),
		ShapeVertex::Create(Vector2D(286.26428f, 211.52415f)),
		ShapeVertex::Create(Vector2D(276.55762f, 197.35748f)),
		ShapeVertex::Create(Vector2D(276.55762f, 225.69083f)),
		ShapeVertex::Create(Vector2D(301.55762f, 225.69083f)),
		ShapeVertex::Create(Vector2D(301.55762f, 197.35748f)),
		ShapeVertex::Create(Vector2D(288.29095f, 199.85748f)),
		ShapeVertex::Create(Vector2D(288.29095f, 225.69083f)),
		ShapeVertex::Create(Vector2D(309.95761f, 225.69083f)),
		ShapeVertex::Create(Vector2D(309.95761f, 199.85748f)),
		ShapeVertex::Create(Vector2D(295.53763f, 199.85748f)),
		ShapeVertex::Create(Vector2D(295.53763f, 226.52415f)),
		ShapeVertex::Create(Vector2D(320.53763f, 226.52415f)),
		ShapeVertex::Create(Vector2D(320.53763f, 199.85748f)),
		ShapeVertex::Create(Vector2D(307.23096f, 196.52415f)),
		ShapeVertex::Create(Vector2D(307.23096f, 225.69083f)),
		ShapeVertex::Create(Vector2D(325.56427f, 225.69083f)),
		ShapeVertex::Create(Vector2D(325.56427f, 196.52415f)),
		ShapeVertex::Create(Vector2D(312.01764f, 196.52415f)),
		ShapeVertex::Create(Vector2D(312.01764f, 225.69083f)),
		ShapeVertex::Create(Vector2D(331.18430f, 225.69083f)),
		ShapeVertex::Create(Vector2D(331.18430f, 196.52415f)),
	};

	static Rectangle kClipRectangle = Rectangle::Create(
		187.35001f,
		809.95142f,
		198.05000f,
		1116.3018f);

	static const UInt16 kaExpectedIndices[] =
	{
		0, 1, 2,
		0, 2, 3,
		4, 5, 6,
		4, 6, 7,
		8, 9, 10,
		8, 10, 11,
		12, 13, 14,
		12, 14, 15,
		16, 17, 18,
		16, 18, 19,
		20, 21, 22,
		20, 22, 23,
		24, 25, 26,
		24, 26, 27,
		28, 29, 30,
		28, 30, 31,
		32, 33, 34,
		32, 34, 35,
		36, 37, 38,
		36, 38, 39,
		40, 41, 42,
		40, 42, 43,
		44, 45, 46,
		44, 46, 47,
	};

	static const ShapeVertex kaExpectedVertices[] =
	{
		ShapeVertex::Create(Vector2D(206.691f, 198.05f)),
		ShapeVertex::Create(Vector2D(206.691f, 225.691f)),
		ShapeVertex::Create(Vector2D(232.524f, 225.691f)),
		ShapeVertex::Create(Vector2D(232.524f, 198.05f)),
		ShapeVertex::Create(Vector2D(219.111f, 199.857f)),
		ShapeVertex::Create(Vector2D(219.111f, 225.691f)),
		ShapeVertex::Create(Vector2D(240.778f, 225.691f)),
		ShapeVertex::Create(Vector2D(240.778f, 199.857f)),
		ShapeVertex::Create(Vector2D(226.358f, 199.857f)),
		ShapeVertex::Create(Vector2D(226.358f, 226.524f)),
		ShapeVertex::Create(Vector2D(252.191f, 226.524f)),
		ShapeVertex::Create(Vector2D(252.191f, 199.857f)),
		ShapeVertex::Create(Vector2D(239.131f, 198.05f)),
		ShapeVertex::Create(Vector2D(239.131f, 225.691f)),
		ShapeVertex::Create(Vector2D(264.131f, 225.691f)),
		ShapeVertex::Create(Vector2D(264.131f, 198.05f)),
		ShapeVertex::Create(Vector2D(249.218f, 199.857f)),
		ShapeVertex::Create(Vector2D(249.218f, 226.524f)),
		ShapeVertex::Create(Vector2D(275.051f, 226.524f)),
		ShapeVertex::Create(Vector2D(275.051f, 199.857f)),
		ShapeVertex::Create(Vector2D(261.491f, 199.857f)),
		ShapeVertex::Create(Vector2D(261.491f, 225.691f)),
		ShapeVertex::Create(Vector2D(285.658f, 225.691f)),
		ShapeVertex::Create(Vector2D(285.658f, 199.857f)),
		ShapeVertex::Create(Vector2D(272.098f, 211.524f)),
		ShapeVertex::Create(Vector2D(272.098f, 225.691f)),
		ShapeVertex::Create(Vector2D(286.264f, 225.691f)),
		ShapeVertex::Create(Vector2D(286.264f, 211.524f)),
		ShapeVertex::Create(Vector2D(276.558f, 198.05f)),
		ShapeVertex::Create(Vector2D(276.558f, 225.691f)),
		ShapeVertex::Create(Vector2D(301.558f, 225.691f)),
		ShapeVertex::Create(Vector2D(301.558f, 198.05f)),
		ShapeVertex::Create(Vector2D(288.291f, 199.857f)),
		ShapeVertex::Create(Vector2D(288.291f, 225.691f)),
		ShapeVertex::Create(Vector2D(309.958f, 225.691f)),
		ShapeVertex::Create(Vector2D(309.958f, 199.857f)),
		ShapeVertex::Create(Vector2D(295.538f, 199.857f)),
		ShapeVertex::Create(Vector2D(295.538f, 226.524f)),
		ShapeVertex::Create(Vector2D(320.538f, 226.524f)),
		ShapeVertex::Create(Vector2D(320.538f, 199.857f)),
		ShapeVertex::Create(Vector2D(307.231f, 198.05f)),
		ShapeVertex::Create(Vector2D(307.231f, 225.691f)),
		ShapeVertex::Create(Vector2D(325.564f, 225.691f)),
		ShapeVertex::Create(Vector2D(325.564f, 198.05f)),
		ShapeVertex::Create(Vector2D(312.018f, 198.05f)),
		ShapeVertex::Create(Vector2D(312.018f, 225.691f)),
		ShapeVertex::Create(Vector2D(331.184f, 225.691f)),
		ShapeVertex::Create(Vector2D(331.184f, 198.05f)),
	};

	Clipper::Indices vIndices((UInt16 const*)kaIndices, (UInt16 const*)kaIndices + SEOUL_ARRAY_COUNT(kaIndices));
	Clipper::Vertices vVertices((ShapeVertex const*)kaVertices, (ShapeVertex const*)kaVertices + SEOUL_ARRAY_COUNT(kaVertices));

	auto pCache(Clipper::NewMeshClipCache<StandardVertex2D>());
	{
		Clipper::Indices vIndices2(vIndices);
		Clipper::Vertices vVertices2(vVertices);

		Falcon::Clipper::MeshClip(
			*pCache,
			kClipRectangle,
			TriangleListDescription::kTextChunk,
			Rectangle::Create(206.69096f, 331.18430f, 196.52415f, 226.52415f),
			vIndices2,
			vIndices2.GetSize(),
			vVertices2,
			vVertices2.GetSize());
		Falcon::Clipper::MeshClip(
			*pCache,
			kClipRectangle,
			TriangleListDescription::kTextChunk,
			vIndices,
			vIndices.GetSize(),
			vVertices,
			vVertices.GetSize());

		SEOUL_UNITTESTING_ASSERT_EQUAL(vIndices, vIndices2);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vVertices, vVertices2);
	}
	Clipper::DestroyMeshClipCache(pCache);

	SEOUL_UNITTESTING_ASSERT_EQUAL(SEOUL_ARRAY_COUNT(kaExpectedIndices), vIndices.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(kaExpectedIndices, vIndices.Data(), vIndices.GetSizeInBytes()));
	SEOUL_UNITTESTING_ASSERT_EQUAL(SEOUL_ARRAY_COUNT(kaExpectedVertices), vVertices.GetSize());
	for (UInt32 i = 0u; i < vVertices.GetSize(); ++i)
	{
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(kaExpectedVertices[i], vVertices[i], kfAboutEqualPosition);
	}
}

void FalconClipperTest::TestMeshTextChunkClipRegression2()
{
	using namespace Falcon;

	static const UInt16 kaIndices[] =
	{
		0,
		1,
		2,
		0,
		2,
		3,
		4,
		5,
		6,
		4,
		6,
		7,
		8,
		9,
		10,
		8,
		10,
		11,
	};

	static const ShapeVertex kaVertices[] =
	{
		ShapeVertex::Create(Vector2D(717.95099f, 215.78630f)),
		ShapeVertex::Create(Vector2D(717.95099f, 235.61963f)),
		ShapeVertex::Create(Vector2D(737.78430f, 235.61963f)),
		ShapeVertex::Create(Vector2D(737.78430f, 215.78630f)),
		ShapeVertex::Create(Vector2D(723.02832f, 195.95297f)),
		ShapeVertex::Create(Vector2D(723.02832f, 235.61963f)),
		ShapeVertex::Create(Vector2D(759.19495f, 235.61963f)),
		ShapeVertex::Create(Vector2D(759.19495f, 195.95297f)),
		ShapeVertex::Create(Vector2D(737.58832f, 195.95297f)),
		ShapeVertex::Create(Vector2D(737.58832f, 236.78630f)),
		ShapeVertex::Create(Vector2D(773.75500f, 236.78630f)),
		ShapeVertex::Create(Vector2D(773.75500f, 195.95297f)),
	};

	static const UInt16 kaExpectedIndices[] =
	{
		0,
		1,
		2,
		0,
		2,
		3,
		4,
		5,
		6,
		4,
		6,
		7,
		8,
		9,
		10,
		8,
		10,
		11,
	};

	static const ShapeVertex kaExpectedVertices[] =
	{
		ShapeVertex::Create(Vector2D(717.95099f, 215.78630f)),
		ShapeVertex::Create(Vector2D(717.95099f, 235.61963f)),
		ShapeVertex::Create(Vector2D(737.78430f, 235.61963f)),
		ShapeVertex::Create(Vector2D(737.78430f, 215.78630f)),
		ShapeVertex::Create(Vector2D(723.02832f, 198.05000f)),
		ShapeVertex::Create(Vector2D(723.02832f, 235.61963f)),
		ShapeVertex::Create(Vector2D(759.19495f, 235.61963f)),
		ShapeVertex::Create(Vector2D(759.19495f, 198.05000f)),
		ShapeVertex::Create(Vector2D(737.58832f, 198.05000f)),
		ShapeVertex::Create(Vector2D(737.58832f, 236.78630f)),
		ShapeVertex::Create(Vector2D(773.75500f, 236.78630f)),
		ShapeVertex::Create(Vector2D(773.75500f, 198.05000f)),
	};

	static const Rectangle kClipRectangle = Rectangle::Create(
		187.35001f,
		809.95142f,
		198.05000f,
		1116.3018f);

	Clipper::Indices vIndices((UInt16 const*)kaIndices, (UInt16 const*)kaIndices + SEOUL_ARRAY_COUNT(kaIndices));
	Clipper::Vertices vVertices((ShapeVertex const*)kaVertices, (ShapeVertex const*)kaVertices + SEOUL_ARRAY_COUNT(kaVertices));

	auto pCache(Clipper::NewMeshClipCache<StandardVertex2D>());
	{
		Clipper::Indices vIndices2(vIndices);
		Clipper::Vertices vVertices2(vVertices);

		Falcon::Clipper::MeshClip(
			*pCache,
			kClipRectangle,
			TriangleListDescription::kTextChunk,
			Rectangle::Create(717.95099f, 773.75500f, 195.95297f, 236.78630f),
			vIndices2,
			vIndices2.GetSize(),
			vVertices2,
			vVertices2.GetSize());
		Falcon::Clipper::MeshClip(
			*pCache,
			kClipRectangle,
			TriangleListDescription::kTextChunk,
			vIndices,
			vIndices.GetSize(),
			vVertices,
			vVertices.GetSize());

		SEOUL_UNITTESTING_ASSERT_EQUAL(vIndices, vIndices2);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vVertices, vVertices2);
	}
	Clipper::DestroyMeshClipCache(pCache);

	SEOUL_UNITTESTING_ASSERT_EQUAL(SEOUL_ARRAY_COUNT(kaExpectedIndices), vIndices.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(kaExpectedIndices, vIndices.Data(), vIndices.GetSizeInBytes()));
	SEOUL_UNITTESTING_ASSERT_EQUAL(SEOUL_ARRAY_COUNT(kaExpectedVertices), vVertices.GetSize());
	for (UInt32 i = 0u; i < vVertices.GetSize(); ++i)
	{
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(kaExpectedVertices[i], vVertices[i], kfAboutEqualPosition);
	}
}

void FalconClipperTest::TestMirrorTransform()
{
	using namespace Falcon;

	Vector2D aClipVertices[] =
	{
		Vector2D(0, -3),
		Vector2D(3,  0),
		Vector2D(0,  3),
		Vector2D(-3,  0),
	};
	ClipStack stack;

	stack.AddConvexHull(
		Matrix2x3::CreateRotationFromDegrees(360.0f) *
		Matrix2x3::CreateScale(-1.0f, 1.0f),
		aClipVertices,
		SEOUL_ARRAY_COUNT(aClipVertices));
	SEOUL_UNITTESTING_ASSERT(!stack.HasClips());
	SEOUL_UNITTESTING_ASSERT(stack.Push());
	SEOUL_UNITTESTING_ASSERT(stack.HasClips());

	SEOUL_UNITTESTING_ASSERT_EQUAL(-3, stack.GetTopClip().m_Bounds.m_fLeft);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, stack.GetTopClip().m_Bounds.m_fRight);
	SEOUL_UNITTESTING_ASSERT_EQUAL(-3, stack.GetTopClip().m_Bounds.m_fTop);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, stack.GetTopClip().m_Bounds.m_fBottom);
	SEOUL_UNITTESTING_ASSERT(!stack.GetTopClip().m_bSimple);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uFirstHull);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, stack.GetTopClip().m_uFirstVertex);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1u, stack.GetTopClip().m_uHulls);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4u, stack.GetTopClip().m_uVertices);

	Clipper::Indices vIndices;
	vIndices.PushBack(0);
	vIndices.PushBack(1);
	vIndices.PushBack(2);
	vIndices.PushBack(0);
	vIndices.PushBack(2);
	vIndices.PushBack(3);

	Clipper::Vertices vVertices;
	vVertices.PushBack(ShapeVertex::Create(Vector2D(2, -2)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(2, 2)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-2, 2)));
	vVertices.PushBack(ShapeVertex::Create(Vector2D(-2, -2)));

	stack.MeshClip(
		TriangleListDescription::kConvex,
		vIndices,
		vVertices);

	SEOUL_UNITTESTING_ASSERT_EQUAL(18, vIndices.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(8, vVertices.GetSize());

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, vIndices[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[3]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, vIndices[4]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, vIndices[5]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[6]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3, vIndices[7]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, vIndices[8]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[9]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(4, vIndices[10]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, vIndices[11]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[12]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, vIndices[13]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(6, vIndices[14]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, vIndices[15]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(6, vIndices[16]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(7, vIndices[17]);

	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-1, -2)), vVertices[0], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(1, -2)), vVertices[1], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(2, -1)), vVertices[2], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(2, 1)), vVertices[3], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(1, 2)), vVertices[4], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-1, 2)), vVertices[5], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-2, 1)), vVertices[6], kfAboutEqualPosition);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(ShapeVertex::Create(Vector2D(-2, -1)), vVertices[7], kfAboutEqualPosition);

	stack.Pop();
	TestClipStackNoneCommon(stack);
}

/**
 * Regression for a case where clipping a clipping mesh
 * produced more vertices than the existing code
 * expected, producing erroneous results.
 */
void FalconClipperTest::TestClipperClipRegression()
{
	using namespace Falcon;

	ClipStack stack;

	// Top.
	Rectangle clipRect;
	clipRect.m_fLeft = 0.000000000f;
	clipRect.m_fRight = 1006.65192f;
	clipRect.m_fTop = -253.149994f;
	clipRect.m_fBottom = 1284.84143f;
	stack.AddRectangle(clipRect);
	SEOUL_UNITTESTING_ASSERT(stack.Push());

	// Next - should result in 5 vertices.
	Matrix2x3 m;
	m.M00 = 0.923917651f;
	m.M10 = -0.382591456f;
	m.M01 = 0.245094851f;
	m.M11 = 0.591877997f;
	m.M02 = 531.849976f;
	m.TX = 531.849976f;
	m.M12 = 1280.01733f;
	m.TY = 1280.01733f;

	Rectangle rect;
	rect.m_fLeft = -21.7500000f;
	rect.m_fRight = 23.2500000f;
	rect.m_fTop = -766.400024f;
	rect.m_fBottom = 0.649999976f;

	stack.AddRectangle(m, rect);
	SEOUL_UNITTESTING_ASSERT(stack.Push());

	SEOUL_UNITTESTING_ASSERT(stack.HasClips());

	auto const& top = stack.GetTopClip();
	SEOUL_UNITTESTING_ASSERT(!top.m_bSimple);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, top.m_uFirstHull);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, top.m_uFirstVertex);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, top.m_uHulls);
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, top.m_uVertices);

	auto const& v = stack.GetVertices();
	SEOUL_UNITTESTING_ASSERT_EQUAL(
		Vector2D(365.490356f, 817.506714f),
		v[0]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(
		Vector2D(553.490356f, 1271.50684f),
		v[1]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(
		Vector2D(521.288635f, 1284.84143f),
		v[2]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(
		Vector2D(510.306549f, 1284.84143f),
		v[3]);
	SEOUL_UNITTESTING_ASSERT_EQUAL(
		Vector2D(323.914063f, 834.723389f),
		v[4]);

	stack.Pop();
	stack.Pop();
}

/**
 * This test pushes some zero size shapes and then a real
 * one to make sure that the ClipStack isn't left in a bad
 * state.
 */
void FalconClipperTest::TestZeroSizeClipRegression()
{
	using namespace Falcon;

	ClipStack stack;

	// Top.
	Vector2D vertices[] = {
		Vector2D(613.587158f, 710.030396f),
		Vector2D(613.587158f, 710.030396f),
		Vector2D(613.587158f, 710.030396f),
		Vector2D(613.587158f, 710.030396f),
	};
	Vector2D vertices2[] = {
		Vector2D(613.587158f, 710.030396f),
		Vector2D(613.587158f, 710.030396f),
		Vector2D(613.587158f, 710.030396f),
		Vector2D(613.587158f, 710.030396f),
	};
	Vector2D vertices3[] = {
		Vector2D(613.587158f, 710.030396f),
		Vector2D(613.587158f, 710.030396f),
		Vector2D(613.587158f, 710.030396f),
		Vector2D(613.587158f, 710.030396f),
		Vector2D(613.587158f, 710.030396f),
		Vector2D(613.587158f, 710.030396f),
	};
	Rectangle rect = Rectangle::Create(157.040192f, 846.989990f, 959.122986f, 1151.17310f);

	stack.AddConvexHull(vertices, 4);
	SEOUL_UNITTESTING_ASSERT(!stack.Push());
	stack.AddConvexHull(vertices2, 4);
	SEOUL_UNITTESTING_ASSERT(!stack.Push());
	stack.AddConvexHull(vertices3, 6);
	SEOUL_UNITTESTING_ASSERT(!stack.Push());
	stack.AddRectangle(rect);
	SEOUL_UNITTESTING_ASSERT(stack.Push());
	stack.Pop();
	SEOUL_UNITTESTING_ASSERT(stack.IsFullyClear());
}

#endif // /#if SEOUL_UNIT_TESTS

} // namespace Seoul
