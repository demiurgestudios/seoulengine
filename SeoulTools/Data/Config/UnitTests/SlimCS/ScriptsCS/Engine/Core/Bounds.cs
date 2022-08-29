// Utility class, wraps a 2D rectangle specified by
// (left, top) and coordinates and a (height, width)
// pair.
//
// The Bounds constructor accepts the top-left and
// bottom right coordinates to define the rectangle.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System.Diagnostics.Contracts;

public class Bounds
{
	public Bounds(double fLeft, double fTop, double fRight, double fBottom)
	{
		x = fLeft;
		y = fTop;
		width = fRight - fLeft;
		height = fBottom - fTop;
	}

	[Pure]
	public override string ToString()
	{
		return $"{{x={x}, y={y}, width={width}, height={height}}}";
	}

	public double height;
	public double width;
	public double x;
	public double y;
}
