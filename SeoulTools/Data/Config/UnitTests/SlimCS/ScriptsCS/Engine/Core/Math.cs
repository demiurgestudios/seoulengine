// Our math extensions. Adds functionality not available in the builtin
// math module.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System.Diagnostics.Contracts;

using static SlimCS;

public class Math
{
	[Pure]
	public static double Clamp(double fValue, double fMinimum, double fMaximum)
	{
		return math.min(math.max(fValue, fMinimum), fMaximum);
	}

	[Pure]
	public static double Lerp(double fA, double fB, double fAlpha)
	{
		return asnum(fA * (1 - fAlpha) + fB * fAlpha);
	}

	[Pure]
	public static double Round(double fValue)
	{
		if (fValue > 0)
		{
			return math.floor(fValue + 0.5);
		}
		else
		{
			return math.ceil(fValue - 0.5);
		}
	}
}
