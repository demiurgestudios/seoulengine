/**
 * \file Geometry.h
 * \brief Various primitive geometry types are declared in this file,
 * including Sphere and Plane.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "AABB.h"
#include "Axis.h"
#include "Plane.h"
#include "Point2DInt.h"
#include "SeoulMath.h"
#include "SeoulTypes.h"
#include "Sphere.h"
#include "Vector2D.h"
#include "Vector3D.h"
#include "Vector4D.h"
namespace Seoul { struct AABB; }
namespace Seoul { struct Matrix4D; }
namespace Seoul { struct Sphere; }
namespace Seoul { template <typename T, int MEMORY_BUDGETS> class Vector; }

namespace Seoul
{

/**
 * 2D rectangular region, floating point,
 * typically in relative coordinates (e.g. [0, 1] or [-1, 1]).
 */
struct Rectangle2D SEOUL_SEALED
{
	Float32 m_fLeft;
	Float32 m_fTop;
	Float32 m_fRight;
	Float32 m_fBottom;

	// Return true if a intersects b at all, false otherwise.
	static inline Bool Intersects(const Rectangle2D& a, const Vector2D& v)
	{
		if (a.m_fLeft > v.X) { return false; }
		if (a.m_fTop > v.Y) { return false; }
		if (a.m_fRight < v.X) { return false; }
		if (a.m_fBottom < v.Y) { return false; }

		return true;
	}

	Rectangle2D()
		: m_fLeft(0)
		, m_fTop(0)
		, m_fRight(0)
		, m_fBottom(0)
	{
	}

	Rectangle2D(Float32 fLeft, Float32 fTop, Float32 fRight, Float32 fBottom)
		: m_fLeft(fLeft)
		, m_fTop(fTop)
		, m_fRight(fRight)
		, m_fBottom(fBottom)
	{
	}

	Rectangle2D(const Rectangle2D& rectangle)
		: m_fLeft(rectangle.m_fLeft)
		, m_fTop(rectangle.m_fTop)
		, m_fRight(rectangle.m_fRight)
		, m_fBottom(rectangle.m_fBottom)
	{
	}

	Rectangle2D& operator=(const Rectangle2D& rectangle)
	{
		m_fLeft = rectangle.m_fLeft;
		m_fTop = rectangle.m_fTop;
		m_fRight = rectangle.m_fRight;
		m_fBottom = rectangle.m_fBottom;

		return *this;
	}

	Float GetHeight() const { return (m_fBottom - m_fTop); }
	Float GetWidth() const { return (m_fRight - m_fLeft); }
}; // struct Rectangle2D

/**
 * 2D rectangular region, typically in pixels.
 */
struct Rectangle2DInt SEOUL_SEALED
{
	Int m_iLeft;
	Int m_iTop;
	Int m_iRight;
	Int m_iBottom;

	Rectangle2DInt()
		: m_iLeft(0)
		, m_iTop(0)
		, m_iRight(0)
		, m_iBottom(0)
	{
	}

	Rectangle2DInt(Int iLeft, Int iTop, Int iRight, Int iBottom)
		: m_iLeft(iLeft)
		, m_iTop(iTop)
		, m_iRight(iRight)
		, m_iBottom(iBottom)
	{
	}

	Rectangle2DInt(const Rectangle2DInt& rectangle)
		: m_iLeft(rectangle.m_iLeft)
		, m_iTop(rectangle.m_iTop)
		, m_iRight(rectangle.m_iRight)
		, m_iBottom(rectangle.m_iBottom)
	{
	}

	Rectangle2DInt& operator=(const Rectangle2DInt& rectangle)
	{
		m_iLeft = rectangle.m_iLeft;
		m_iTop = rectangle.m_iTop;
		m_iRight = rectangle.m_iRight;
		m_iBottom = rectangle.m_iBottom;

		return *this;
	}

	Bool operator==(const Rectangle2DInt& b) const
	{
		return
			(m_iLeft == b.m_iLeft) &&
			(m_iTop == b.m_iTop) &&
			(m_iRight == b.m_iRight) &&
			(m_iBottom == b.m_iBottom);
	}
	Bool operator!=(const Rectangle2DInt& b) const
	{
		return !(*this == b);
	}

	Bool operator<(const Rectangle2DInt& b) const
	{
		if (m_iLeft < b.m_iLeft)
		{
			return true;
		}
		else if (m_iLeft > b.m_iLeft)
		{
			return false;
		}

		if (m_iTop < b.m_iTop)
		{
			return true;
		}
		else if (m_iTop > b.m_iTop)
		{
			return false;
		}

		if (m_iRight < b.m_iRight)
		{
			return true;
		}
		else if (m_iRight > b.m_iRight)
		{
			return false;
		}

		if (m_iBottom < b.m_iBottom)
		{
			return true;
		}

		return false;
	}

	Rectangle2DInt& Expand(Int iMargin)
	{
		m_iLeft -= iMargin;
		m_iTop -= iMargin;
		m_iRight += iMargin;
		m_iBottom += iMargin;

		return *this;
	}

	Int GetHeight() const
	{
		return (m_iBottom - m_iTop);
	}

	Int GetWidth() const
	{
		return (m_iRight - m_iLeft);
	}
}; // struct Rectangle2DInt

} // namespace Seoul

#endif // include guard
