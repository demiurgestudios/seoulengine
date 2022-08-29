// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.Collections.Generic;
using System.Text;

namespace SlimCSLib
{
	public static class Utilities
	{
		/// <summary>
		/// Append a single value to the end of the first array.
		/// </summary>
		/// <typeparam name="T">Specialization of the Array to modify.</typeparam>
		/// <param name="a">Array to modify.</param>
		/// <param name="val">Value to append.</param>
		public static void ArrayAppend<T>(ref T[] a, T val)
		{
			int iOrigLength = a.Length;
			Array.Resize(ref a, iOrigLength + 1);
			a[iOrigLength] = val;
		}

		/// <summary>
		/// Append a second array to the end of the first array.
		/// </summary>
		/// <typeparam name="T">Specialization of the Array to modify.</typeparam>
		/// <param name="a">Array to modify.</param>
		/// <param name="values">Values to append.</param>
		public static void ArrayAppend<T>(ref T[] a, params T[] values)
		{
			int iOrigLength = a.Length;
			Array.Resize(ref a, iOrigLength + values.Length);
			Array.Copy(values, 0, a, iOrigLength, values.Length);
		}

		/// <summary>
		/// Append a second list to the end of an array.
		/// </summary>
		/// <typeparam name="T">Specialization of the Array to modify.</typeparam>
		/// <param name="a">Array to modify.</param>
		/// <param name="values">Values to append.</param>
		public static void ArrayAppend<T>(ref T[] a, IReadOnlyList<T> values)
		{
			int iOrigLength = a.Length;
			Array.Resize(ref a, iOrigLength + values.Count);
			for (int i = 0; i < values.Count; ++i)
			{
				a[iOrigLength + i] = values[i];
			}
		}

		/// <summary>
		/// Inserts a new value into the given array at the given index.
		/// </summary>
		/// <typeparam name="T">Specialization of the Array to modify.</typeparam>
		/// <param name="a">Array to modify. Length must be >= iInsert.</param>
		/// <param name="iInsert">Insertion index.</param>
		/// <param name="val">Value to insert.</param>
		public static void ArrayInsert<T>(ref T[] a, int iInsert, T val)
		{
			if (iInsert > a.Length)
			{
				throw new ArgumentOutOfRangeException("insert index " + iInsert.ToString() +
					"is not <= array size " + a.Length.ToString());
			}

			Array.Resize(ref a, a.Length + 1);
			for (int i = a.Length - 1; i > iInsert; --i)
			{
				a[i] = a[i - 1];
			}
			a[iInsert] = val;
		}

		/// <summary>
		/// Generic Clamp - NaN safe (will return min if value is NaN).
		/// </summary>
		public static T Clamp<T>(T value, T min, T max)
			where T : IComparable<T>
		{
			return Min(Max(value, min), max);
		}

		/// <summary>
		/// Given an array of command-line arguments, generates a string
		/// that will be properly escaped for the Windows command-line.
		/// </summary>
		public static string FormatArgumentsForCommandLine(params string[] asArguments)
		{
			StringBuilder result = new StringBuilder();
			for (int i = 0; i < asArguments.Length; ++i)
			{
				if (0 != i)
				{
					result.Append(' ');
				}

				result.Append(FormatArgumentForCommandLine(asArguments[i]));
			}

			return result.ToString();
		}

		/// <summary>
		/// Escapes the argument for the Windows command-line.
		/// </summary>
		public static string FormatArgumentForCommandLine(string sArgument)
		{
			StringBuilder result = new StringBuilder();

			bool bRequiresQuotes = false;
			int nPreceedingBackslashes = 0;
			for (int i = 0; i < sArgument.Length; ++i)
			{
				// If the current element is a double quote, we need to handle it specially.
				if (sArgument[i] == '"')
				{
					// Little tricky at first glance - this implements the following formatting rules:
					// - Backslashes are interpreted literally, unless they immediately precede a double quotation mark.
					// - If an even number of backslashes is followed by a double quotation mark, one backslash is placed
					//   in the argv array for every pair of backslashes, and the double quotation mark is interpreted as
					//   a string delimiter.
					// - If an odd number of backslashes is followed by a double quotation mark, one backslash is
					//   placed in the argv array for every pair of backslashes, and the double quotation mark is
					//   "escaped" by the remaining backslash, causing a literal double quotation mark (") to be placed
					//   in argv.
					//
					// In our case, we're going the other way - we're generating a command-line that, when converted by
					// the above rules into an argv array, we want the exact input argument sArgument to be restored. So,
					// reading between the lines a bit, the only case we need to handle specially is when a (") is present.
					// Further, since we want to get the exact same result, we always want to double the number of
					// backslashes and then add one more for quotes present in the input. For example:
					//
					// sArgument = "Hello World" . \"Hello World\" (zero backslashes * 2 + 1)
					// sArgument = \"Hello World"\ . \\\"Hello World\"\ (1 backslash * 2 + 1, zero backslashes * 2 + 1)
					result.Append('\\', nPreceedingBackslashes + 1);
					result.Append('"');

					nPreceedingBackslashes = 0;
				}
				// Otherwise, just append the character and potentially count backslashes in preparation
				// for a quote.
				else
				{
					// If the character is a backslash, increment the backslash count.
					if (sArgument[i] == '\\')
					{
						++nPreceedingBackslashes;
					}
					// Otherwise...
					else
					{
						// ...reset the backslash count.
						nPreceedingBackslashes = 0;

						// If this character is a tab or a space (the 2 argument delimiters according to docs), then we will need
						// to quote this entire argument for it to be interpreted correctly as a single command-line argument
						// instead of multiple.
						if (sArgument[i] == '\t' || sArgument[i] == ' ')
						{
							bRequiresQuotes = true;
						}
					}

					// Append this character.
					result.Append(sArgument[i]);
				}
			}

			// If we need to quote the entire argument, do so now.
			if (bRequiresQuotes)
			{
				// Leading double quote.
				result.Insert(0, '"');

				// See the comment above for the bulk of why this happens - for the very last quote, we *want* it to be interpreted
				// as a string delimiter (instead of passed through literally), so we need to double the number of preceeding
				// backslashes before adding the quote (we don't add 1 in this case, because that would case the quote to be escaped
				// and passed through literally, instead of acting as a string delimiter).
				result.Append('\\', nPreceedingBackslashes);
				result.Append('"');
			}

			return result.ToString();
		}

		/// <summary>
		/// Mix iMixin into an existing rolling iHash.
		/// </summary>
		public static int IncrementalHash(int iHash, int iMixin)
		{
			return (int)(iHash ^ (iMixin + 0x9e3779b9 + (iHash << 6) + (iHash >> 2)));
		}

		/// <summary>
		/// Reduce sFilename (absolute path) to a Lua module require path.
		/// </summary>
		/// <param name="sFilename">Absolute filename to reduce.</param>
		/// <param name="sDirectory">Directory that defines the base.</param>
		/// <returns></returns>
		public static string GetModulePath(string sFilename, string sDirectory)
		{
			var sRelative = sFilename;
			if (!string.IsNullOrEmpty(sDirectory))
			{
				sRelative = FileUtility.PathRelativeTo(sDirectory, sFilename);
			}
			return FileUtility.RemoveExtension(sRelative).Replace('\\', '/');
		}

		/// <summary>
		/// Suite of generic min/max routines.
		/// </summary>
		public static T Max<T>(T a, T b)
			where T : IComparable<T>
		{
			return ((a.CompareTo(b) > 0) ? a : b);
		}

		/// <summary>
		/// Suite of generic min/max routines.
		/// </summary>
		public static T Max<T>(T a, T b, T c)
			where T : IComparable<T>
		{
			return Max(a, Max(b, c));
		}

		/// <summary>
		/// Suite of generic min/max routines.
		/// </summary>
		public static T Max<T>(T a, T b, T c, T d)
			where T : IComparable<T>
		{
			return Max(a, b, Max(c, d));
		}

		/// <summary>
		/// Suite of generic min/max routines.
		/// </summary>
		public static T Min<T>(T a, T b)
			where T : IComparable<T>
		{
			return ((a.CompareTo(b) < 0) ? a : b);
		}

		/// <summary>
		/// Suite of generic min/max routines.
		/// </summary>
		public static T Min<T>(T a, T b, T c)
			where T : IComparable<T>
		{
			return Min(a, Min(b, c));
		}

		/// <summary>
		/// Suite of generic min/max routines.
		/// </summary>
		public static T Min<T>(T a, T b, T c, T d)
			where T : IComparable<T>
		{
			return Min(a, b, Min(c, d));
		}
	}
}
