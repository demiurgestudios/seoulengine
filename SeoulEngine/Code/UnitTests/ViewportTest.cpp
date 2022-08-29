/**
 * \file ViewportTest.cpp
 * \brief Unit tests for the Viewport struct. A Viewport
 * represents an area of a RenderTarget.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionDefine.h"
#include "UnitTesting.h"
#include "Viewport.h"
#include "ViewportTest.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(ViewportTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestBasic)
	SEOUL_METHOD(TestMethods)
SEOUL_END_TYPE();

void ViewportTest::TestBasic()
{
	Viewport viewport;
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, viewport.m_iTargetHeight);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, viewport.m_iTargetWidth);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, viewport.m_iViewportHeight);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, viewport.m_iViewportWidth);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, viewport.m_iViewportX);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, viewport.m_iViewportY);

	viewport = Viewport::Create(100, 200, 50, 55, 30, 100);
	SEOUL_UNITTESTING_ASSERT_EQUAL(200, viewport.m_iTargetHeight);
	SEOUL_UNITTESTING_ASSERT_EQUAL(100, viewport.m_iTargetWidth);
	SEOUL_UNITTESTING_ASSERT_EQUAL(100, viewport.m_iViewportHeight);
	SEOUL_UNITTESTING_ASSERT_EQUAL(30, viewport.m_iViewportWidth);
	SEOUL_UNITTESTING_ASSERT_EQUAL(50, viewport.m_iViewportX);
	SEOUL_UNITTESTING_ASSERT_EQUAL(55, viewport.m_iViewportY);

	auto b = viewport;
	SEOUL_UNITTESTING_ASSERT_EQUAL(viewport, b);
	SEOUL_UNITTESTING_ASSERT_EQUAL(200, b.m_iTargetHeight);
	SEOUL_UNITTESTING_ASSERT_EQUAL(100, b.m_iTargetWidth);
	SEOUL_UNITTESTING_ASSERT_EQUAL(100, b.m_iViewportHeight);
	SEOUL_UNITTESTING_ASSERT_EQUAL(30, b.m_iViewportWidth);
	SEOUL_UNITTESTING_ASSERT_EQUAL(50, b.m_iViewportX);
	SEOUL_UNITTESTING_ASSERT_EQUAL(55, b.m_iViewportY);

	b.m_iTargetHeight = 199;
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(viewport, b);
	b.m_iTargetHeight = 200; b.m_iTargetWidth = 75;
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(viewport, b);
	b.m_iTargetWidth = 100; b.m_iViewportHeight = 97;
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(viewport, b);
	b.m_iViewportHeight = 100; b.m_iViewportWidth = 28;
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(viewport, b);
	b.m_iViewportWidth = 30; b.m_iViewportX = 49;
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(viewport, b);
	b.m_iViewportX = 50; b.m_iViewportY = 56;
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(viewport, b);
	b.m_iViewportY = 55;
	SEOUL_UNITTESTING_ASSERT_EQUAL(viewport, b);
}

void ViewportTest::TestMethods()
{
	auto viewport = Viewport::Create(100, 50, 10, 5, 80, 40);
	SEOUL_UNITTESTING_ASSERT_EQUAL(2.0f, viewport.GetTargetAspectRatio());
	SEOUL_UNITTESTING_ASSERT_EQUAL(2.0f, viewport.GetViewportAspectRatio());
	viewport.m_iViewportWidth = 40;
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, viewport.GetViewportAspectRatio());
	viewport.m_iViewportWidth = 80;
	SEOUL_UNITTESTING_ASSERT_EQUAL(45, viewport.GetViewportBottom());
	SEOUL_UNITTESTING_ASSERT_EQUAL(50.0f, viewport.GetViewportCenterX());
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(25.0f, viewport.GetViewportCenterY(), 1e-4f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(90, viewport.GetViewportRight());

	SEOUL_UNITTESTING_ASSERT(viewport.Intersects(Point2DInt(50, 25)));
	SEOUL_UNITTESTING_ASSERT(viewport.Intersects(Vector2D(50, 25.0f)));

	// LT
	SEOUL_UNITTESTING_ASSERT(viewport.Intersects(Point2DInt(10, 5)));
	SEOUL_UNITTESTING_ASSERT(viewport.Intersects(Vector2D(10, 5)));

	// RB
	// Right and bottom edge is considered outside the viewport, since it's left + width and top + height.
	SEOUL_UNITTESTING_ASSERT(!viewport.Intersects(Point2DInt(90, 45)));
	SEOUL_UNITTESTING_ASSERT(!viewport.Intersects(Vector2D(90, 45)));

	// LB
	SEOUL_UNITTESTING_ASSERT(!viewport.Intersects(Point2DInt(10, 45)));
	SEOUL_UNITTESTING_ASSERT(!viewport.Intersects(Vector2D(10, 45)));

	// RT
	SEOUL_UNITTESTING_ASSERT(!viewport.Intersects(Point2DInt(90, 5)));
	SEOUL_UNITTESTING_ASSERT(!viewport.Intersects(Vector2D(90, 5)));
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
