// Glue required by SlimCS. Implements the body of various
// builtin functionality (e.g. string.IsNullOrEmpty).
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System.Diagnostics.Contracts;

public static class SlimCSStringLib
{
	[Pure]
	public static bool IsNullOrEmpty(string s)
	{
		return null == s || string.Empty == s;
	}

	[Pure]
	public static string ReplaceAll(this string s, string pattern, string replacement)
	{
		return CoreNative.StringReplace(s, pattern, replacement);
	}
}
