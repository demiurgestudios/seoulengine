/**
 * \file Point2DInt.h
 * \brief Geometric structure, defines a 2D integer resolution
 * position.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef POINT2D_INT_H
#define POINT2D_INT_H

#include "Prereqs.h"

namespace Seoul
{

/** 2D position, typically in pixels. */
struct Point2DInt SEOUL_SEALED
{
	Int X;
	Int Y;

	Point2DInt()
		: X(0)
		, Y(0)
	{
	}

	Point2DInt(Int x, Int y)
		: X(x)
		, Y(y)
	{
	}

	Point2DInt(const Point2DInt & point)
		: X(point.X)
		, Y(point.Y)
	{
	}

	Point2DInt & operator=(const Point2DInt & point)
	{
		X = point.X;
		Y = point.Y;

		return *this;
	}

	Bool operator==(const Point2DInt& point) const
	{
		return (X == point.X && Y == point.Y);
	}

	Bool operator!=(const Point2DInt& point) const
	{
		return !(*this == point);
	}

	Point2DInt operator+(const Point2DInt & other) const
	{
		return Point2DInt(
			X + other.X,
			Y + other.Y);
	}

	Point2DInt& operator+=(const Point2DInt& other)
	{
		X += other.X;
		Y += other.Y;
		return *this;
	}

	Point2DInt operator-(const Point2DInt & other) const
	{
		return Point2DInt(
			X - other.X,
			Y - other.Y);
	}

	Point2DInt& operator-=(const Point2DInt& other)
	{
		X -= other.X;
		Y -= other.Y;
		return *this;
	}
}; // struct Point2DInt

} // namespace Seoul

#endif // include guard
