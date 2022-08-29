// Utility class, wraps a 2D vector.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System.Diagnostics.Contracts;

using static SlimCS;

public class Vector2D
{
	public Vector2D(double fX, double fY)
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
	public static Vector2D operator+(Vector2D v1, Vector2D v2)
	{
		return new Vector2D(v1.x + v2.x, v1.y + v2.y);
	}

	[Pure]
	public static Vector2D operator-(Vector2D v1, Vector2D v2)
	{
		return new Vector2D(v1.x - v2.x, v1.y - v2.y);
	}

	[Pure]
	public static Vector2D operator*(Vector2D v, double fScalar)
	{
		return new Vector2D(v.x * fScalar, v.y * fScalar);
	}

	[Pure]
	public static Vector2D operator/(Vector2D v, double fScalar)
	{
		return new Vector2D(v.x / fScalar, v.y / fScalar);
	}

	[Pure]
	public override string ToString()
	{
		return $"{{x={x}, y={y}}}";
	}

	public double x;
	public double y;
}
