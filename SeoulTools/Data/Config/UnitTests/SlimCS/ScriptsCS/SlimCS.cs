// Utility file that defines core dependencies of SlimCS
// generated from .lua code.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System.Diagnostics;
using System.Diagnostics.Contracts;

/// <summary>
/// Most of the SlimCS "standard library". Mainly a direct binding
/// to underlying Lua implementations.
/// </summary>
/// <remarks>
/// With some exceptions (e.g. dval<T>), functions and types in this file
/// marked external are direct binding placeholders to underlying LuaJIT
/// standard library functions.
///
/// LuaJIT is a Lua 5.1/5.2 implementation: https://www.lua.org/manual/5.1/manual.html
///
/// NOTE: This is meant as a placeholder. In the long term, we want to build out
/// a minimal SlimCS standard library that is a subset of either .NET or .NET Core.
///
/// That work is pending both due to inherent time required to implement a standard
/// library and due to a partial block on pending improvements to SlimCS code generation
/// (including inlining and elimination of wrapper overhead).
/// </remarks>
public static class SlimCS
{
	public sealed class TopLevelChunkAttribute : System.Attribute
	{
	}

	public abstract class dval<T>
	{
		public static extern implicit operator bool?(dval<T> d);
		public static extern implicit operator double?(dval<T> d);
		public static extern implicit operator string(dval<T> d);

		public static extern implicit operator dval<T>(bool b);
		public static extern implicit operator dval<T>(double f);
		public static extern implicit operator dval<T>(string s);

		public extern dval<T> this[object key] { get; set; }

		public static extern implicit operator T(dval<T> d);
		public static extern implicit operator dval<T>(T t);

		public extern override bool Equals(object obj);
		public extern override int GetHashCode();
	}

	/// <summary>
	/// Arbitrary key-value data structure, equivalent semantics
	/// to a Lua table.
	/// </summary>
	/// <remarks>
	/// Matches Lua table semantics, this includes:
	/// - if key does not exist in the table, null is returned.
	/// - setting a value of null to a slot is equivalent to erasing
	///   the entry from the table.
	/// - lenop() of a table returns the number of contiguous elements
	///   with integer keys, starting at 1 (e.g. if the table contains
	///   [1] = "One", [2] = "Two", [3] = "Three", lenop() returns 3).
	/// </remarks>
	public class Table
	{
		public extern dval<object> this[object key] { get; set; }
	}

	/// <summary>
	/// Specialization of Table with concrete key-value types.
	/// V is required to be a class.
	/// </summary>
	/// <typeparam name="K">Type parameter of the key.</typeparam>
	/// <typeparam name="V">Type parameter of the value.</typeparam>
	public sealed class Table<K, V> : Table
		where V : class
	{
		public extern V this[K key] { get; set; }
	}

	/// <summary>
	/// Specialization of Table with concrete key-values types.
	/// V is required to be a struct (in SlimCS, bool or double).
	/// </summary>
	/// <typeparam name="K">Type parameter of the key.</typeparam>
	/// <typeparam name="V">Type parameter of the value.</typeparam>
	/// <remarks>
	/// Exactly equivalent to Table`K,V except for cases where V
	/// is a bool or double. Must be separate only because C#
	/// generic type constraints are not sufficient to handle
	/// both cases in Table`K,V.
	/// </remarks>
	public sealed class TableV<K, V> : Table
		where V : struct
	{
		public extern V? this[K key] { get; set; }
	}

	public sealed class Tuple<A1, A2>
	{
		A1 m_A1;
		A2 m_A2;

		public Tuple((A1, A2) t)
		{
			(m_A1, m_A2) = t;
		}

		public void Deconstruct(out A1 a1, out A2 a2)
		{
			(a1, a2) = (m_A1, m_A2);
		}
	}

	public sealed class Tuple<A1, A2, A3>
	{
		A1 m_A1;
		A2 m_A2;
		A3 m_A3;

		public Tuple((A1, A2, A3) t)
		{
			(m_A1, m_A2, m_A3) = t;
		}

		public void Deconstruct(out A1 a1, out A2 a2, out A3 a3)
		{
			(a1, a2, a3) = (m_A1, m_A2, m_A3);
		}
	}

	public sealed class Tuple<A1, A2, A3, A4>
	{
		A1 m_A1;
		A2 m_A2;
		A3 m_A3;
		A4 m_A4;

		public Tuple((A1, A2, A3, A4) t)
		{
			(m_A1, m_A2, m_A3, m_A4) = t;
		}

		public void Deconstruct(out A1 a1, out A2 a2, out A3 a3, out A4 a4)
		{
			(a1, a2, a3, a4) = (m_A1, m_A2, m_A3, m_A4);
		}
	}

	/// <summary>
	/// Exactly equivalent to Lua ipairs().
	/// </summary>
	/// <param name="o">Arbitrary object to enumerate.</param>
	/// <returns>An ordered enumerable over an object.</returns>
	/// <remarks>
	/// Equivalent to the Lua ipairs() function. Like ipairs(),
	/// enumerates over keys 1..n, where n is the last integer
	/// key for which the value at n+1 is null.
	/// </remarks>
	[Pure]
	public static extern System.Collections.Generic.IEnumerable<(double, dval<object>)> ipairs(object o);

	/// <summary>
	/// Concrete typed equivalent to ipairs().
	/// </summary>
	/// <typeparam name="V">Value type parameter for the concrete table argument.</typeparam>
	/// <param name="t">Table to enumerate.</param>
	/// <returns>An ordered enumerable over a concrete table.</returns>
	[Pure]
	public static extern System.Collections.Generic.IEnumerable<(double, V)> ipairs<V>(Table<double, V> t)
		where V : class;

	/// <summary>
	/// Concrete typed equivalent to ipairs().
	/// </summary>
	/// <typeparam name="V">Value type parameter for the concrete table argument.</typeparam>
	/// <param name="t">Table to enumerate.</param>
	/// <returns>An ordered enumerable over a concrete table.</returns>
	[Pure]
	public static extern System.Collections.Generic.IEnumerable<(double, V)> ipairs<V>(TableV<double, V> t)
		where V : struct;

	/// <summary>
	/// Exactly equivalent to Lua pairs().
	/// </summary>
	/// <param name="o">Arbitrary object to enumerate.</param>
	/// <returns>An enumerable over an object.</returns>
	/// <remarks>
	/// Equivalent to the Lua pairs() function. Enumerates
	/// all key-value pairs of the given object.
	/// </remarks>
	[Pure]
	public static extern System.Collections.Generic.IEnumerable<(dval<object>, dval<object>)> pairs(object o);

	/// <summary>
	/// Concrete typed equivalent to pairs().
	/// </summary>
	/// <typeparam name="V">Value type parameter for the concrete table argument.</typeparam>
	/// <param name="t">Table to enumerate.</param>
	/// <returns>An enumerable over a concrete table.</returns>
	[Pure]
	public static extern System.Collections.Generic.IEnumerable<(K, V)> pairs<K, V>(Table<K, V> t)
		where V : class;

	/// <summary>
	/// Concrete typed equivalent to pairs().
	/// </summary>
	/// <typeparam name="V">Value type parameter for the concrete table argument.</typeparam>
	/// <param name="t">Table to enumerate.</param>
	/// <returns>An enumerable over a concrete table.</returns>
	[Pure]
	public static extern System.Collections.Generic.IEnumerable<(K, V)> pairs<K, V>(TableV<K, V> t)
		where V : struct;

	/// <summary>
	/// "Dynamic" cast. See remarks.
	/// </summary>
	/// <typeparam name="T">Type of object to cast to.</typeparam>
	/// <param name="d">Object to cast from.</param>
	/// <returns>The result of the cast operation (see remarks).</returns>
	/// <remarks>
	/// A "dynamic" cast exactly matches the semantics of the underlying Lua runtime that
	/// SlimCS compiles to. In short, this means a dyncast is a "nop" cast. No checking
	/// or coercion is performed on the input value. As a result, effectively, any type
	/// can be coerced to any other type. This includes illogical conversions such as
	/// converting a boolean value to a complex class.
	///
	/// Incorrect or invalid casts are allowed and will result in runtime errors. For example,
	/// if a boolean value is cast to an interface that includes a method Refresh(), an error
	/// will not be raised until an attempt is made to invoke Refresh(), and the error will
	/// be "attempt to index local '{varname}' (a boolean value)".
	///
	/// dyncast should be considered an "unsafe" cast. The reason to use dyncast is performance
	/// and flexibility. There is no overhead to a dyncast, which can be useful (e.g.) if you
	/// know that the cast will always be valid. dyncast also allows conversions that are
	/// otherwise impossible. This is mainly useful for implementing low-level functionality
	/// (e.g. extensions to the SlimCS class system).
	/// </remarks>
	[Pure]
	public extern static T dyncast<T>(dval<object> d);

	/// <summary>
	/// Utility for common conversion of double? to double.
	/// </summary>
	/// <param name="f">Input nullable double.</param>
	/// <returns>The value f or 0, if f is null.</returns>
	[Pure]
	public static extern double asnum(double? f);

	/// <summary>
	/// A way of getting the class table from a type, must be called with typeof
	/// </summary>
	/// <param name="type"></param>
	/// <returns></returns>
	[Pure]
	public extern static Table astable(System.Type type);

	public static extern void collectgarbage(string opt = null, object arg = null);
	[Pure] public static extern string concat(params object[] args);
	public static extern void error(object message, double? level = null);

	/// <summary>
	/// Equivalent to the global table (_G) in the underlying Lua implementation.
	/// </summary>
	/// <remarks>
	/// _G gives you low-level access to the global state of the Lua virtual machine.
	/// Use with care.
	///
	/// In particular, because all classes are bound with deduping into the global table,
	/// you should only bind symbols with no possibility of collision with classes.
	/// Class are deduped by adding a number to the end of the name. For example,
	/// "Foo" may be deduped as "Foo0".
	/// </remarks>
	public static Table _G = new Table();

	[Pure] public static extern object getfenv(object o);
	[Pure] public static extern Table getmetatable(object o);

	/// <summary>
	/// Exactly equivalent to the Lua length operator (#).
	/// </summary>
	/// <typeparam name="T">Type of the object to apply length calculation to.</typeparam>
	/// <param name="v">Object to compute the length of.</param>
	/// <returns>Resulting length of the given object.</returns>
	/// <remarks>
	/// Like Lua #, the length operator is only reasonable
	/// for objects that are contiguous arrays, starting at an index
	/// of 1. For example, the length of { [1] = "One", [2] = "Two", [3] = "Three" }
	/// is 3, but the length of { ["One"] = "One", ["Two"] = "Two", ["Three"] = "Three" }
	/// is 0.
	///
	/// To compute the number of elements of an arbitrary object or table,
	/// enumerate the table and count the elements (which is O(n)).
	/// </remarks>
	[Pure]
	public static extern double lenop<T>(T v)
		where T : class;

	[Pure] public static extern object next(object t, object k = null);

	/// <summary>
	/// Equivalent to the not operator in Lua.
	/// </summary>
	/// <param name="o">Value to apply script truthiness to.</param>
	/// <returns>true if o is false o null, false otherwise.</returns>
	[Pure]
	public static extern bool not(object o);

	public static extern object pcall(object f, params object[] args);

	/// <summary>
	/// Custom hooks for reporting static initialization progress at
	/// runtime. This function is called once to specify the total
	/// number of progress steps.
	/// </summary>
	public static extern void __initprogresssteps__(int steps);

	/// <summary>
	/// Custom hooks for reporting static initialization progress at
	/// runtime. This function is called to advance one step.
	/// </summary>
	public static extern void __oninitprogress__();

	/// <summary>
	/// Binding to low-level lua assert function.
	/// </summary>
	[Conditional("DEBUG")]
	public static extern void assert(bool b, string msg);

	/// <summary>
	/// Avoid using print whenever possible -- it's impossible to filter these log lines out.
	/// Use CoreNative.Log instead unless you're debugging a low-level problem that can't depend
	/// on CoreNative.
	/// </summary>
	/// <param name="a">Set of arguments to print.</param>
	[Conditional("DEBUG")]
	public static extern void print(params object[] a);

	[Pure] public static extern bool rawequal(object v1, object v2);
	[Pure] public static extern dval<object> rawget(object t, object k);
	public static extern object rawset(object t, object k, object v);
	public static extern object require(string s);
	[Pure] public static extern object select(object index, params object[] args);
	public static extern object setfenv(object f, object table);
	public static extern Table setmetatable(object t, object metatable);

	[Pure] public static extern System.Collections.Generic.IEnumerable<int> range(object start, object last, object step = null);
	[Pure] public static extern double? tonumber(object o);
	[Pure] public static extern string tostring(object o);
	[Pure] public static extern string type(object o);

	public static class coroutine
	{
		public extern static object create(object f);
		public extern static (bool, object) resume(object co, params object[] args);
		public extern static string status(object co);
		public extern static void yield(params object[] args);
	}

	public static class debug
	{
		public extern static Table getinfo(object thread, double level, string what = null);
		public extern static (string, object) getlocal(object thread, double level, double index);
		public extern static Table getregistry();
		public extern static (string, object) getupvalue(object func, double index);
		public extern static object getuservalue(object o);
		public extern static object setuservalue(object o, object v);
		public extern static string traceback();
	}

	public static class math
	{
		/// <summary>
		/// Returns the absolute value of x.
		/// </summary>
		[Pure]
		public extern static double abs(double? a);

		/// <summary>
		/// Returns the arc cosine of x (in radians).
		/// </summary>
		[Pure]
		public extern static double acos(double? a);

		/// <summary>
		/// Returns the arc sine of x (in radians).
		/// </summary>
		[Pure]
		public extern static double asin(double? a);

		/// <summary>
		/// Returns the arc tangent of x (in radians).
		/// </summary>
		[Pure]
		public extern static double atan(double? x);

		/// <summary>
		/// Returns the arc tangent of y/x (in radians),
		/// but uses the signs of both parameters to find the
		/// quadrant of the result. (It also handles correctly the
		/// case of x being zero.)
		/// </summary>
		[Pure]
		public extern static double atan2(double? x, double? y);

		/// <summary>
		/// Returns the smallest integer larger than or equal to x.
		/// </summary>
		[Pure]
		public extern static double ceil(double? a);

		/// <summary>
		/// Returns the cosine of x (assumed to be in radians).
		/// </summary>
		[Pure]
		public extern static double cos(double? a);

		/// <summary>
		/// Returns the hyperbolic cosine of x.
		/// </summary>
		[Pure]
		public extern static double cosh(double? a);

		/// <summary>
		/// Returns the angle x (given in radians) in degrees.
		/// </summary>
		[Pure]
		public extern static double deg(double? a);

		/// <summary>
		/// Returns the value e^x.
		/// </summary>
		[Pure]
		public extern static double exp(double? a);

		/// <summary>
		/// Returns the largest integer smaller than or equal to x.
		/// </summary>
		[Pure]
		public extern static double floor(double? a);

		/// <summary>
		/// Returns the remainder of the division of x by y that rounds the quotient towards zero.
		/// </summary>
		[Pure]
		public extern static double fmod(double? x, double? y);

		/// <summary>
		/// Returns m and e such that x = m2e, e is an integer and the absolute
		/// value of m is in the range [0.5, 1) (or zero when x is zero).
		/// </summary>
		[Pure]
		public extern static double frexp(double? a);

		/// <summary>
		/// The value HUGE_VAL, a value larger than or equal to any other numerical value.
		/// </summary>
		[Pure]
		public const double huge = double.PositiveInfinity;

		/// <summary>
		/// Returns m2e (e should be an integer).
		/// </summary>
		[Pure]
		public extern static double ldexp(double? m, double? e);

		/// <summary>
		/// Returns the natural logarithm of x.
		/// </summary>
		[Pure]
		public extern static double log(double? a);

		/// <summary>
		/// Returns the base-10 logarithm of x.
		/// </summary>
		[Pure]
		public extern static double log10(double? a);

		/// <summary>
		/// Returns the maximum value among its arguments.
		/// </summary>
		[Pure]
		public extern static double max(double? x, params double?[] args);

		/// <summary>
		/// Returns the minimum value among its arguments.
		/// </summary>
		[Pure]
		public extern static double min(double? a, params double?[] args);

		/// <summary>
		/// Returns two numbers, the integral part of x and the fractional part of x.
		/// </summary>
		[Pure]
		public extern static double modf(double? a);

		/// <summary>
		/// The value of pi.
		/// </summary>
		public const double pi = 3.14159265358979323846;

		/// <summary>
		/// Returns xy. (You can also use the expression x^y to compute this value.)
		/// </summary>
		[Pure]
		public extern static double pow(double? x, double? y);

		/// <summary>
		/// Returns the angle x (given in degrees) in radians.
		/// </summary>
		[Pure]
		public extern static double rad(double? a);

		/// <summary>
		/// This function is an interface to the simple pseudo-random generator function rand provided by ANSI C.
		/// (No guarantees can be given for its statistical properties.)
		/// </summary>
		/// <remarks>
		/// When called without arguments, returns a uniform pseudo-random real
		/// umber in the range[0, 1). When called with an integer number m,
		/// math.random returns a uniform pseudo - random integer in
		/// the range[1, m].When called with two integer numbers m and n,
		/// math.random returns a uniform pseudo - random integer in the range[m, n].
		/// </remarks>
		[Pure]
		public extern static double random(double? lower = null, double? upper = null);

		/// <summary>
		/// Sets x as the "seed" for the pseudo-random generator: equal seeds produce equal sequences of numbers.
		/// </summary>
		[Pure]
		public extern static double randomseed(double? a);

		/// <summary>
		/// Returns the sine of x (assumed to be in radians).
		/// </summary>
		[Pure]
		public extern static double sin(double? a);

		/// <summary>
		/// Returns the hyperbolic sine of x.
		/// </summary>
		[Pure]
		public extern static double sinh(double? a);

		/// <summary>
		/// Returns the square root of x. (You can also use the expression x^0.5 to compute this value.)
		/// </summary>
		[Pure]
		public extern static double sqrt(double? a);

		/// <summary>
		/// Returns the tangent of x (assumed to be in radians).
		/// </summary>
		[Pure]
		public extern static double tan(double? a);

		/// <summary>
		/// Returns the hyperbolic tangent of x.
		/// </summary>
		[Pure]
		public extern static double tanh(double? a);
	}

	public static class package
	{
		/// <summary>
		/// A table used by require to control which modules are already
		/// loaded. When you require a module modname and package.loaded[modname]
		/// is not false, require simply returns the value stored there.
		/// </summary>
		public static readonly Table loaded = new Table();
	}

	public static class @string
	{
		/// <summary>
		/// Looks for the first match of pattern in the string s.
		/// </summary>
		/// <remarks>
		/// If it finds a match, then find returns the indices of s
		/// where this occurrence starts and ends; otherwise, it returns nil.
		/// A third, optional numerical argument init specifies where to start
		/// the search; its default value is 1 and can be negative. A value
		/// of true as a fourth, optional argument plain turns off the pattern
		/// matching facilities, so the function does a plain "find substring"
		/// operation, with no characters in pattern being considered "magic".
		/// Note that if plain is given, then init must be given as well.
		///
		/// If the pattern has captures, then in a successful match the captured
		/// values are also returned, after the two indices.
		/// </summary>
		[Pure]
		public extern static (double?, double?) find(string s, string pattern, double? init = null, bool plain = false);

		/// <summary>
		/// SeoulEngine extension to the string package. Equivalent to find(), starts from the end of the string.
		/// </summary>
		[Pure]
		public extern static (double?, double?) find_last(string s, string pattern, double? init = null, bool plain = false);

		[Pure] public extern static string format(string fmt, params object[] args);
		[Pure] public extern static System.Collections.Generic.IEnumerable<dval<object>> gmatch(string s, string pattern);

		[Pure] public extern static string gsub(string s, string pattern, string repl, double? n = null);

		[Pure] public extern static double len(string s);
		[Pure] public extern static string match(string s, string pattern, double? init = null);
		[Pure] public extern static string rep(string s, double n);
		[Pure] public extern static string reverse(string s);
		[Pure] public extern static string sub(string s, double i, double? j = null);
	}

	public static class table
	{
		public delegate bool SortDelegate(dval<object> a, dval<object> b);

		[Pure] public extern static string concat(object t, string sep = string.Empty, double? i = null, double? j = null);
		[Pure] public extern static double getn(object t);
		public extern static void insert(object t, object posOrVal = null, object val = null);
		[Pure] public extern static double maxn(object t);
		public extern static dval<object> remove(object t, double? pos = null);
		public extern static void sort(object t, SortDelegate comp = null);
		[Pure] public static extern dval<object> unpack(object t, double? i = null, double? j = null);
	}

	public delegate void Vvargs(params object[] aArgs);

	public delegate void Vfunc0();

	public delegate void VfuncT0();

	public static VfuncT0 function(VfuncT0 f)
	{
		return f;
	}

	public delegate void VfuncT1<in A0>(A0 a0 = default(A0));

	public static VfuncT1<A0> function<A0>(VfuncT1<A0> f)
	{
		return f;
	}

	public delegate void VfuncT2<in A0, in A1>(A0 a0 = default(A0), A1 a1 = default(A1));

	public static VfuncT2<A0, A1> function<A0, A1>(VfuncT2<A0, A1> f)
	{
		return f;
	}

	public delegate void VfuncT3<in A0, in A1, in A2>(A0 a0 = default(A0), A1 a1 = default(A1), A2 a2 = default(A2));

	public static VfuncT3<A0, A1, A2> function<A0, A1, A2>(VfuncT3<A0, A1, A2> f)
	{
		return f;
	}

	public delegate void VfuncT4<in A0, in A1, in A2, in A3>(A0 a0 = default(A0), A1 a1 = default(A1), A2 a2 = default(A2), A3 a3 = default(A3));

	public static VfuncT4<A0, A1, A2, A3> function<A0, A1, A2, A3>(VfuncT4<A0, A1, A2, A3> f)
	{
		return f;
	}

	public delegate void VfuncT5<in A0, in A1, in A2, in A3, in A4>(
		A0 a0 = default(A0),
		A1 a1 = default(A1),
		A2 a2 = default(A2),
		A3 a3 = default(A3),
		A4 a4 = default(A4));

	public static VfuncT5<A0, A1, A2, A3, A4> function<A0, A1, A2, A3, A4>(VfuncT5<A0, A1, A2, A3, A4> f)
	{
		return f;
	}

	public delegate void VfuncT6<in A0, in A1, in A2, in A3, in A4, in A5>(
		A0 a0 = default(A0),
		A1 a1 = default(A1),
		A2 a2 = default(A2),
		A3 a3 = default(A3),
		A4 a4 = default(A4),
		A5 a5 = default(A5));

	public static VfuncT6<A0, A1, A2, A3, A4, A5> function<A0, A1, A2, A3, A4, A5>(VfuncT6<A0, A1, A2, A3, A4, A5> f)
	{
		return f;
	}

	public delegate void VfuncT7<in A0, in A1, in A2, in A3, in A4, in A5, in A6>(
		A0 a0 = default(A0),
		A1 a1 = default(A1),
		A2 a2 = default(A2),
		A3 a3 = default(A3),
		A4 a4 = default(A4),
		A5 a5 = default(A5),
		A6 a6 = default(A6));

	public static VfuncT7<A0, A1, A2, A3, A4, A5, A6> function<A0, A1, A2, A3, A4, A5, A6>(VfuncT7<A0, A1, A2, A3, A4, A5, A6> f)
	{
		return f;
	}

	public delegate void VfuncT8<in A0, in A1, in A2, in A3, in A4, in A5, in A6, in A7>(
		A0 a0 = default(A0),
		A1 a1 = default(A1),
		A2 a2 = default(A2),
		A3 a3 = default(A3),
		A4 a4 = default(A4),
		A5 a5 = default(A5),
		A6 a6 = default(A6),
		A7 a7 = default(A7));

	public static VfuncT8<A0, A1, A2, A3, A4, A5, A6, A7> function<A0, A1, A2, A3, A4, A5, A6, A7>(VfuncT8<A0, A1, A2, A3, A4, A5, A6, A7> f)
	{
		return f;
	}

	public delegate void VfuncT9<in A0, in A1, in A2, in A3, in A4, in A5, in A6, in A7, in A8>(
		A0 a0 = default(A0),
		A1 a1 = default(A1),
		A2 a2 = default(A2),
		A3 a3 = default(A3),
		A4 a4 = default(A4),
		A5 a5 = default(A5),
		A6 a6 = default(A6),
		A7 a7 = default(A7),
		A8 a8 = default(A8));

	public static VfuncT9<A0, A1, A2, A3, A4, A5, A6, A7, A8> function<A0, A1, A2, A3, A4, A5, A6, A7, A8>(VfuncT9<A0, A1, A2, A3, A4, A5, A6, A7, A8> f)
	{
		return f;
	}

	public delegate void VfuncT10<in A0, in A1, in A2, in A3, in A4, in A5, in A6, in A7, in A8, in A9>(
		A0 a0 = default(A0),
		A1 a1 = default(A1),
		A2 a2 = default(A2),
		A3 a3 = default(A3),
		A4 a4 = default(A4),
		A5 a5 = default(A5),
		A6 a6 = default(A6),
		A7 a7 = default(A7),
		A8 a8 = default(A8),
		A9 a9 = default(A9));

	public static VfuncT10<A0, A1, A2, A3, A4, A5, A6, A7, A8, A9> function<A0, A1, A2, A3, A4, A5, A6, A7, A8, A9>(
		VfuncT10<A0, A1, A2, A3, A4, A5, A6, A7, A8, A9> f)
	{
		return f;
	}

	public delegate void VfuncT11<in A0, in A1, in A2, in A3, in A4, in A5, in A6, in A7, in A8, in A9, in A10>(
		A0 a0 = default(A0),
		A1 a1 = default(A1),
		A2 a2 = default(A2),
		A3 a3 = default(A3),
		A4 a4 = default(A4),
		A5 a5 = default(A5),
		A6 a6 = default(A6),
		A7 a7 = default(A7),
		A8 a8 = default(A8),
		A9 a9 = default(A9),
		A10 a10 = default(A10));

	public static VfuncT11<A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10> function<A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>(
		VfuncT11<A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10> f)
	{
		return f;
	}

	public delegate void VfuncT12<in A0, in A1, in A2, in A3, in A4, in A5, in A6, in A7, in A8, in A9, in A10, in A11>(
		A0 a0 = default(A0),
		A1 a1 = default(A1),
		A2 a2 = default(A2),
		A3 a3 = default(A3),
		A4 a4 = default(A4),
		A5 a5 = default(A5),
		A6 a6 = default(A6),
		A7 a7 = default(A7),
		A8 a8 = default(A8),
		A9 a9 = default(A9),
		A10 a10 = default(A10),
		A11 a11 = default(A11));

	public static VfuncT12<A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11> function<A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>(
		VfuncT12<A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11> f)
	{
		return f;
	}

	public delegate void VfuncT13<in A0, in A1, in A2, in A3, in A4, in A5, in A6, in A7, in A8, in A9, in A10, in A11, in A12>(
		A0 a0 = default(A0),
		A1 a1 = default(A1),
		A2 a2 = default(A2),
		A3 a3 = default(A3),
		A4 a4 = default(A4),
		A5 a5 = default(A5),
		A6 a6 = default(A6),
		A7 a7 = default(A7),
		A8 a8 = default(A8),
		A9 a9 = default(A9),
		A10 a10 = default(A10),
		A11 a11 = default(A11),
		A12 a12 = default(A12));

	public static VfuncT13<A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12> function<A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>(
		VfuncT13<A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12> f)
	{
		return f;
	}

	public delegate void VfuncT14<in A0, in A1, in A2, in A3, in A4, in A5, in A6, in A7, in A8, in A9, in A10, in A11, in A12, in A13>(
		A0 a0 = default(A0),
		A1 a1 = default(A1),
		A2 a2 = default(A2),
		A3 a3 = default(A3),
		A4 a4 = default(A4),
		A5 a5 = default(A5),
		A6 a6 = default(A6),
		A7 a7 = default(A7),
		A8 a8 = default(A8),
		A9 a9 = default(A9),
		A10 a10 = default(A10),
		A11 a11 = default(A11),
		A12 a12 = default(A12),
		A13 a13 = default(A13));

	public static VfuncT14<A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13> function<A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>(
		VfuncT14<A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13> f)
	{
		return f;
	}

	public delegate void VfuncT15<in A0, in A1, in A2, in A3, in A4, in A5, in A6, in A7, in A8, in A9, in A10, in A11, in A12, in A13, in A14>(
		A0 a0 = default(A0),
		A1 a1 = default(A1),
		A2 a2 = default(A2),
		A3 a3 = default(A3),
		A4 a4 = default(A4),
		A5 a5 = default(A5),
		A6 a6 = default(A6),
		A7 a7 = default(A7),
		A8 a8 = default(A8),
		A9 a9 = default(A9),
		A10 a10 = default(A10),
		A11 a11 = default(A11),
		A12 a12 = default(A12),
		A13 a13 = default(A13),
		A14 a14 = default(A14));

	public static VfuncT15<A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14>
		function<A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14>(VfuncT15<A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14> f)
	{
		return f;
	}
}
