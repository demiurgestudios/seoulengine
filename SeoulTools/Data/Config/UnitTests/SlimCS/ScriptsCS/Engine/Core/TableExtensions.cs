// Extension methods for Table classes
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using static SlimCS;

public static class TableExtensions
{
	/// <summary>
	/// Returns whether the given table is empty. Also returns true if the table is null.
	/// </summary>
	public static bool IsNullOrEmpty(this Table table)
	{
		if (null == table)
		{
			return true;
		}

		return null == next(table);
	}

	/// <summary>
	/// Get a value from the table, returning <paramref name="def"/> if the value is null/not found
	/// </summary>
	/// <param name="table">Table on which to perform the lookup.</param>
	/// <param name="key">Name of item to lookup in the table.</param>
	/// <param name="def">The value to return if <paramref name="key"/> is null/not found</param>
	public static T Get<T>(this Table table, object key, T def)
	{
		var val = table[key];
		if (val == null)
		{
			return def;
		}

		return (T)val;
	}

	/// <summary>
	/// Get a value from the table, returning <paramref name="default"/> if the value is null/not found
	/// </summary>
	/// <param name="default">The value to return if <paramref name="key"/> is null/not found</param>
	public static V Get<K, V>(this Table<K, V> table, K key, V @default)
		where V : class
	{
		var val = table[key];
		if (val == null)
		{
			return @default;
		}

		return dyncast<V>(val);
	}

	/// <summary>
	/// Get a value from the table, returning <paramref name="default"/> if the value is not found
	/// </summary>
	/// <param name="default">The value to return if <paramref name="key"/> is not found</param>
	public static V Get<K, V>(this TableV<K, V> table, K key, V @default)
		where V : struct
	{
		var val = table[key];
		if (val == null)
		{
			return @default;
		}

		return dyncast<V>(val);
	}
}
