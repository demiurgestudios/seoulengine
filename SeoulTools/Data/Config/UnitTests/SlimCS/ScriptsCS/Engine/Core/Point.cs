// Utility class, wraps a 2D position.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System.Diagnostics.Contracts;

using static SlimCS;

public class Point
{
	public Point(double fX = 0.0, double fY = 0.0)
	{
		x = fX;
		y = fY;
	}

	[Pure]
	public double GetLength()
	{
		return math.sqrt(x * x + y * y);
	}

	[Pure]
	public static Point operator+(Point point1, Point point2)
	{
		return new Point(point1.x + point2.x, point1.y + point2.y);
	}

	[Pure]
	public static Point operator-(Point point1, Point point2)
	{
		return new Point(point1.x - point2.x, point1.y - point2.y);
	}

	[Pure]
	public static Point operator*(Point v, double fScalar)
	{
		return new Point(v.x * fScalar, v.y * fScalar);
	}

	[Pure]
	public static Point operator/(Point v, double fScalar)
	{
		return new Point(v.x / fScalar, v.y / fScalar);
	}

	[Pure]
	public override string ToString()
	{
		return $"{{x={x}, y={y}}}";
	}

	public double x;
	public double y;
}
