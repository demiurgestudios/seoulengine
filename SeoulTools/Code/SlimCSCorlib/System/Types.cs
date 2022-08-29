// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System.Diagnostics.Contracts;

namespace System
{
	/// <summary>
	/// The exception that is thrown when one of the arguments provided to a method is
	/// not valid.
	/// </summary>
	public class ArgumentException : Exception
	{
		/// <summary>
		/// Initializes a new instance of the System.ArgumentException class.
		/// </summary>
		public ArgumentException()
			: base("Value does not fall within the expected range.")
		{
		}

		/// <summary>
		/// Initializes a new instance of the System.ArgumentException class with a specified
		/// error message.
		/// </summary>
		/// <param name="message">The error message that explains the reason for the exception.</param>
		public ArgumentException(String message)
			: base(message)
		{
		}
	}

	/// <summary>
	/// Provides methods for creating, manipulating, searching, and sorting arrays, thereby
	/// serving as the base class for all arrays in the common language runtime.To browse
	/// the .NET Framework source code for this type, see the Reference Source.
	/// </summary>
	public abstract class Array
	{
		/// <summary>
		/// Gets the total number of elements in all the dimensions of the System.Array.
		/// </summary>
		/// <returns>
		/// </returns>
		public int Length
		{
			[System.Diagnostics.Contracts.Pure]
			get;
		}
	}

	/// <summary>
	/// Represents the base class for custom attributes.
	/// </summary>
	public abstract class Attribute { }

	/// <summary>
	/// Represents a Boolean (true or false) value.
	/// </summary>
	public struct Boolean : IComparable<Boolean>
	{
		/// <summary>
		/// Represents the Boolean value true as a string. This field is read-only.
		/// </summary>
		public const string TrueString = "True";
		/// <summary>
		/// Represents the Boolean value false as a string. This field is read-only.
		/// </summary>
		public const string FalseString = "False";

		/// <summary>
		/// Compares this instance to a specified System.Boolean object and returns an integer
		/// that indicates their relationship to one another.
		/// </summary>
		/// <param name="other">A System.Boolean object to compare to this instance.</param>
		/// <returns>
		/// A signed integer that indicates the relative values of this instance and value.Return
		/// Value Condition Less than zero This instance is false and value is true. Zero
		/// This instance and value are equal (either both are true or both are false). Greater
		/// than zero This instance is true and value is false.
		/// </returns>
		public extern int CompareTo(Boolean other);
	}

	/// <summary>
	/// Represents an 8-bit unsigned integer.
	/// </summary>
	public struct Byte : IComparable<Byte>
	{
		/// <summary>
		/// Represents the largest possible value of a System.Byte. This field is constant.
		/// </summary>
		public const Byte MaxValue = 255;
		/// <summary>
		/// Represents the smallest possible value of a System.Byte. This field is constant.
		/// /// </summary>
		public const Byte MinValue = 0;

		/// <summary>
		/// Compares this instance to a specified 8-bit unsigned integer and returns an indication
		/// of their relative values.
		/// </summary>
		/// <param name="other">An 8-bit unsigned integer to compare.</param>
		/// <returns>
		/// A signed integer that indicates the relative order of this instance and value.Return
		/// Value Description Less than zero This instance is less than value. Zero This
		/// instance is equal to value. Greater than zero This instance is greater than value.
		/// </returns>
		public extern int CompareTo(Byte other);
	}

	/// <summary>
	/// Represents a character as a UTF-16 code unit.
	/// </summary>
	public struct Char : IComparable<Char>
	{
		/// <summary>
		/// Represents the largest possible value of a System.Char. This field is constant.
		/// </summary>
		public const Char MaxValue = '\uffff';
		/// <summary>
		/// Represents the smallest possible value of a System.Char. This field is constant.
		/// </summary>
		public const Char MinValue = '\0';

		/// <summary>
		/// Compares this instance to a specified System.Char object and indicates whether
		/// this instance precedes, follows, or appears in the same position in the sort
		/// order as the specified System.Char object.
		/// </summary>
		/// <param name="other">A System.Char object to compare.</param>
		/// <returns>
		/// A signed number indicating the position of this instance in the sort order in
		/// relation to the value parameter.Return Value Description Less than zero This
		/// instance precedes value. Zero This instance has the same position in the sort
		/// order as value. Greater than zero This instance follows value.
		/// </returns>
		public extern int CompareTo(Char other);
	}

	/// <summary>
	/// Represents a delegate, which is a data structure that refers to a static method
	/// or to a class instance and an instance method of that class.
	/// </summary>
	public abstract class Delegate { }

	/// <summary>
	/// Represents a double-precision floating-point number.
	/// </summary>
	public struct Double : IComparable<Double>
	{
		/// <summary>
		/// Represents the smallest possible value of a System.Double. This field is constant.
		/// </summary>
		public const Double MinValue = -1.7976931348623157E+308;
		/// <summary>
		/// Represents the largest possible value of a System.Double. This field is constant.
		/// </summary>
		public const Double MaxValue = 1.7976931348623157E+308;
		/// <summary>
		/// Represents the smallest positive System.Double value that is greater than zero.
		/// This field is constant.
		/// </summary>
		public const Double Epsilon = 4.94065645841247E-324;
		/// <summary>
		/// Represents negative infinity. This field is constant.
		/// </summary>
		public const Double NegativeInfinity = -1D / 0D;
		/// <summary>
		/// Represents positive infinity. This field is constant.
		/// </summary>
		public const Double PositiveInfinity = 1D / 0D;
		/// <summary>
		/// Represents a value that is not a number (NaN). This field is constant.
		/// </summary>
		public const Double NaN = 0D / 0D;

		/// <summary>
		/// Compares this instance to a specified double-precision floating-point number
		/// and returns an integer that indicates whether the value of this instance is less
		/// than, equal to, or greater than the value of the specified double-precision floating-point
		/// number.
		/// </summary>
		/// <param name="other">A double-precision floating-point number to compare.</param>
		/// <returns>
		/// A signed number indicating the relative values of this instance and value.Return
		/// Value Description Less than zero This instance is less than value.-or- This instance
		/// is not a number (System.Double.NaN) and value is a number. Zero This instance
		/// is equal to value.-or- Both this instance and value are not a number (System.Double.NaN),
		/// System.Double.PositiveInfinity, or System.Double.NegativeInfinity. Greater than
		/// zero This instance is greater than value.-or- This instance is a number and value
		/// is not a number (System.Double.NaN).
		/// </returns>
		public extern int CompareTo(Double other);
	}

	/// <summary>
	/// Provides the base class for enumerations.
	/// </summary>
	public abstract class Enum { }

	/// <summary>
	/// Represents errors that occur during application execution.To browse the .NET
	/// Framework source code for this type, see the Reference Source.
	/// </summary>
	public class Exception
	{
		/// <summary>
		/// Initializes a new instance of the System.Exception class.
		/// </summary>
		public Exception()
		{
		}

		/// <summary>
		/// Initializes a new instance of the System.Exception class with a specified error
		/// message.
		/// </summary>
		/// <param name="sMessage">The message that describes the error.</param>
		public Exception(string sMessage)
		{
		}

		/// <summary>
		/// Gets a message that describes the current exception.
		/// </summary>
		/// <returns>
		/// The error message that explains the reason for the exception, or an empty string
		/// ("").
		/// </returns>
		public extern virtual string Message { get; }
	}

	/// <summary>
	/// Indicates that an enumeration can be treated as a bit field; that is, a set of
	/// flags.
	/// </summary>
	public sealed class FlagsAttribute : Attribute
	{
		public FlagsAttribute()
		{
		}
	}

	/// <summary>
	/// Provides a mechanism for releasing unmanaged resources.To browse the .NET Framework
	/// source code for this type, see the Reference Source.
	/// </summary>
	public interface IDisposable
	{
		/// <summary>
		/// Performs application-defined tasks associated with freeing, releasing, or resetting
		/// unmanaged resources.
		/// </summary>
		void Dispose();
	}

	/// <summary>
	/// Represents a 16-bit signed integer.
	/// </summary>
	public struct Int16 : IComparable<Int16>
	{
		/// <summary>
		/// Represents the largest possible value of an System.Int16. This field is constant.
		/// </summary>
		public const Int16 MaxValue = 32767;
		/// <summary>
		/// Represents the smallest possible value of System.Int16. This field is constant.
		/// </summary>
		public const Int16 MinValue = -32768;

		/// <summary>
		/// Compares this instance to a specified 16-bit signed integer and returns an integer
		/// that indicates whether the value of this instance is less than, equal to, or
		/// greater than the value of the specified 16-bit signed integer.
		/// </summary>
		/// <param name="other">An integer to compare.</param>
		/// <returns>
		/// A signed number indicating the relative values of this instance and value.Return
		/// Value Description Less than zero This instance is less than value. Zero This
		/// instance is equal to value. Greater than zero This instance is greater than value.
		/// </returns>
		public extern int CompareTo(Int16 other);
	}

	/// <summary>
	/// Represents a 32-bit signed integer.To browse the .NET Framework source code for
	/// this type, see the Reference Source.
	/// </summary>
	public struct Int32 : IComparable<Int32>
	{
		/// <summary>
		/// Represents the largest possible value of an System.Int32. This field is constant.
		/// </summary>
		public const Int32 MaxValue = 2147483647;
		/// <summary>
		/// Represents the smallest possible value of System.Int32. This field is constant.
		/// </summary>
		public const Int32 MinValue = -2147483648;

		/// <summary>
		/// Compares this instance to a specified 32-bit signed integer and returns an indication
		///  of their relative values.
		/// </summary>
		/// <param name="other">An integer to compare.</param>
		/// <returns>
		/// A signed number indicating the relative values of this instance and value.Return
		/// Value Description Less than zero This instance is less than value. Zero This
		/// instance is equal to value. Greater than zero This instance is greater than value.
		/// </returns>
		public extern int CompareTo(Int32 other);
	}

	/// <summary>
	/// Represents a 64-bit signed integer.
	/// </summary>
	public struct Int64 : IComparable<Int64>
	{
		/// <summary>
		/// Represents the largest possible value of an Int64. This field is constant.
		/// </summary>
		public const Int64 MaxValue = 9223372036854775807;
		/// <summary>
		/// Represents the smallest possible value of an Int64. This field is constant.
		/// </summary>
		public const Int64 MinValue = -9223372036854775808;

		/// <summary>
		/// Compares this instance to a specified 64-bit signed integer and returns an indication
		/// of their relative values.
		/// </summary>
		/// <param name="other">An integer to compare.</param>
		/// <returns>
		/// A signed number indicating the relative values of this instance and value.Return
		/// Value Description Less than zero This instance is less than value. Zero This
		/// instance is equal to value. Greater than zero This instance is greater than value.
		/// </returns>
		public extern int CompareTo(Int64 other);
	}

	/// <summary>
	/// A platform-specific type that is used to represent a pointer or a handle.
	/// </summary>
	public struct IntPtr { }

	/// <summary>
	/// Represents a multicast delegate; that is, a delegate that can have more than
	/// one element in its invocation list.
	/// </summary>
	public abstract class MulticastDelegate : Delegate { }

	/// <summary>
	/// Represents a value type that can be assigned null.
	/// </summary>
	/// <typeparam name="T">The underlying value type of the System.Nullable`1 generic type.</typeparam>
	public struct Nullable<T> where T : struct
	{
		/// <summary>
		/// Initializes a new instance of the System.Nullable`1 structure to the specified
		/// value.
		/// </summary>
		/// <param name="value">A value type.</param>
		public Nullable(T value) { }

		/// <summary>
		/// Retrieves the value of the current System.Nullable`1 object, or the object's
		/// default value.
		/// </summary>
		/// <returns>
		/// The value of the System.Nullable`1.Value property if the System.Nullable`1.HasValue
		/// property is true; otherwise, the default value of the current System.Nullable`1
		/// object. The type of the default value is the type argument of the current System.Nullable`1
		/// object, and the value of the default value consists solely of binary zeroes.
		/// </returns>
		public extern T GetValueOrDefault();

		/// <summary>
		/// Gets a value indicating whether the current System.Nullable`1 object has a valid
		/// value of its underlying type.
		/// </summary>
		/// <returns>
		/// true if the current System.Nullable`1 object has a value; false if the current
		/// System.Nullable`1 object has no value.
		/// </returns>
		public extern bool HasValue { get; }

		/// <summary>
		/// Gets the value of the current System.Nullable`1 object if it has been assigned
		///  a valid underlying value.
		/// </summary>
		/// <returns>
		/// The value of the current System.Nullable`1 object if the System.Nullable`1.HasValue
		/// property is true. An exception is thrown if the System.Nullable`1.HasValue property
		/// is false.
		/// </returns>
		public extern T Value { get; }
	}

	/// <summary>
	/// The exception that is thrown when there is an attempt to dereference a null object
	/// reference.
	/// </summary>
	public class NullReferenceException : Exception
	{
		/// <summary>
		/// Initializes a new instance of the System.NullReferenceException class, setting
		/// the System.Exception.Message property of the new instance to a system-supplied
		/// message that describes the error, such as "The value 'null' was found where an
		/// instance of an object was required." This message takes into account the current
		/// system culture.
		/// </summary>
		public NullReferenceException()
			: base("Object reference not set to an instance of an object.")
		{
		}

		/// <summary>
		/// Initializes a new instance of the System.NullReferenceException class with a
		/// specified error message.
		/// </summary>
		/// <param name="message">
		/// A System.String that describes the error. The content of message is intended
		/// to be understood by humans. The caller of this constructor is required to ensure
		/// that this string has been localized for the current system culture.
		/// </param>
		public NullReferenceException(String message)
			: base(message)
		{
		}
	}

	/// <summary>
	/// Supports all classes in the .NET Framework class hierarchy and provides low-level
	/// services to derived classes. This is the ultimate base class of all classes in
	/// the .NET Framework; it is the root of the type hierarchy.To browse the .NET Framework
	/// source code for this type, see the Reference Source.
	/// </summary>
	public class Object
	{
		/// <summary>
		/// Initializes a new instance of the System.Object class.
		/// </summary>
		public Object()
		{
		}

		/// <summary>
		/// Determines whether the specified object is equal to the current object.
		/// </summary>
		/// <param name="o">The object to compare with the current object.</param>
		/// <returns>true if the specified object is equal to the current object; otherwise, false.</returns>
		public extern virtual bool Equals(Object o);

		/// <summary>
		/// Serves as the default hash function.
		/// </summary>
		/// <returns>A hash code for the current object.</returns>
		public extern virtual int GetHashCode();

		/// <summary>
		/// Gets the System.Type of the current instance.
		/// </summary>
		/// <returns>The exact runtime type of the current instance.</returns>
		public extern Type GetType();

		/// <summary>
		/// Returns a string that represents the current object.
		/// </summary>
		/// <returns>A string that represents the current object.</returns>
		public extern virtual String ToString();
	}

	/// <summary>
	/// Marks the program elements that are no longer in use. This class cannot be inherited.
	/// </summary>
	public sealed class ObsoleteAttribute : Attribute
	{
		/// <summary>
		/// Initializes a new instance of the System.ObsoleteAttribute class with default
		/// properties.
		/// </summary>
		public ObsoleteAttribute()
		{
		}
	}

	/// <summary>
	/// Indicates that a method will allow a variable number of arguments in its invocation.
	/// This class cannot be inherited.
	/// </summary>
	public sealed class ParamArrayAttribute : Attribute
	{
		/// <summary>
		/// Initializes a new instance of the System.ParamArrayAttribute class with default
		/// properties.
		/// </summary>
		public ParamArrayAttribute()
		{
		}
	}

	/// <summary>
	/// Represents a type using an internal metadata token.
	/// </summary>
	public struct RuntimeTypeHandle { }

	/// <summary>
	/// Represents text as a sequence of UTF-16 code units.To browse the .NET Framework
	/// source code for this type, see the Reference Source.
	/// </summary>
	public sealed class String
	{
		/// <summary>
		/// Represents the empty string. This field is read-only.
		/// </summary>
		public const String Empty = "";

		/// <summary>Creates the string representation of a specified object.</summary>
		/// <param name="arg0">The object to represent, or null.</param>
		/// <returns>The string representation of the value of arg0, or System.String.Empty if arg0 is null.</returns>
		[Pure] public extern static String Concat(object arg0);

		/// <summary>Concatenates the string representations of two specified objects.</summary>
		/// <param name="arg0">The first object to concatenate.</param>
		/// <param name="arg1">The second object to concatenate.</param>
		/// <returns>The concatenated string representations of the values of arg0, and arg2.</returns>
		[Pure] public extern static String Concat(object arg0, object arg1);

		/// <summary>Concatenates the string representations of three specified objects.</summary>
		/// <param name="arg0">The first object to concatenate.</param>
		/// <param name="arg1">The second object to concatenate.</param>
		/// <param name="arg2">The third object to concatenate.</param>
		/// <returns>The concatenated string representations of the values of arg0, arg1, and arg2.</returns>
		[Pure] public extern static String Concat(object arg0, object arg1, object arg2);

		/// <summary>
		/// Concatenates the elements of a specified System.String array.
		/// </summary>
		/// <param name="args">An object array that contains the elements to concatenate.</param>
		/// <returns>
		/// The concatenated elements of values.
		/// </returns>
		[Pure] public extern static String Concat(params object[] args);

		/// <summary>
		/// Concatenates two specified instances of System.String.
		/// </summary>
		/// <param name="str0">The first string to concatenate.</param>
		/// <param name="str1">The second string to concatenate.</param>
		/// <returns>The concatenation of str0 and str1.</returns>
		[Pure] public extern static String Concat(String str0, String str1);

		/// <summary>
		/// Concatenates three specified instances of System.String.
		/// </summary>
		/// <param name="str0">The first string to concatenate.</param>
		/// <param name="str1">The second string to concatenate.</param>
		/// <param name="str2">The third string to concatenate.</param>
		/// <returns>The concatenation of str0, str1, and str2.</returns>
		[Pure] public extern static String Concat(String str0, String str1, String str2);

		/// <summary>
		/// Concatenates four specified instances of System.String.
		/// </summary>
		/// <param name="str0">The first string to concatenate.</param>
		/// <param name="str1">The second string to concatenate.</param>
		/// <param name="str2">The third string to concatenate.</param>
		/// <param name="str3">The fourth string to concatenate.</param>
		/// <returns>The concatenation of str0, str1, str2, and str3.</returns>
		[Pure] public extern static String Concat(String str0, String str1, String str2, String str3);

		/// <summary>
		/// Concatenates the elements of a specified System.String array.
		/// </summary>
		/// <param name="values">An array of string instances.</param>
		/// <returns>The concatenated elements of values.</returns>
		[Pure] public extern static String Concat(params String[] values);

		/// <summary>
		/// Replaces the format item in a specified string with the string representation
		/// of a corresponding object in a specified array.
		/// </summary>
		/// <param name="args">An object array that contains zero or more objects to format.</param>
		/// <param name="format">A composite format string.</param>
		/// <returns>
		/// A copy of format in which the format items have been replaced by the string representation
		/// of the corresponding objects in args.
		/// </returns>
		[Pure] public extern static String Format(String format, params Object[] args);

		/// <summary>
		/// Indicates whether the specified string is null or an System.String.Empty string.
		/// </summary>
		/// <param name="value">The string to test.</param>
		/// <returns>
		/// true if the value parameter is null or an empty string (""); otherwise, false.
		/// </returns>
		[Pure] public extern static bool IsNullOrEmpty(String value);

		/// <summary>
		/// Determines whether this instance and a specified object, which must also be a
		/// System.String object, have the same value.
		/// </summary>
		/// <param name="o">The string to compare to this instance.</param>
		/// <returns>
		/// true if obj is a System.String and its value is the same as this instance; otherwise,
		/// false. If obj is null, the method returns false.
		/// </returns>
		[Pure] public extern override bool Equals(object o);

		/// <summary>
		/// Returns the hash code for this string.
		/// </summary>
		/// <returns>A 32-bit signed integer hash code.</returns>
		[Pure] public extern override int GetHashCode();

		/// <summary>
		/// Determines whether two specified strings have the same value.
		/// </summary>
		/// <param name="a">The first string to compare, or null.</param>
		/// <param name="b">The second string to compare, or null.</param>
		/// <returns>true if the value of a is the same as the value of b; otherwise, false.</returns>
		[Pure] public extern static bool operator ==(String a, String b);

		/// <summary>
		///  Determines whether two specified strings have different values.
		/// </summary>
		/// <param name="a">The first string to compare, or null.</param>
		/// <param name="b">The second string to compare, or null.</param>
		/// <returns>true if the value of a is different from the value of b; otherwise, false.</returns>
		[Pure] public extern static bool operator !=(String a, String b);
	}

	namespace Reflection
	{
		/// <summary>
		/// Obtains information about the attributes of a member and provides access to member
		/// metadata.
		/// </summary>
		public abstract class MemberInfo
		{
			/// <summary>
			/// Gets the name of the current member.
			/// </summary>
			/// <returns>
			/// A System.String containing the name of this member.
			/// </returns>
			public extern string Name { get; }
		}
	}

	/// <summary>
	/// Represents an 8-bit signed integer.
	/// </summary>
	public struct SByte :IComparable<SByte>
	{
		/// <summary>
		/// Represents the largest possible value of System.SByte. This field is constant.
		/// </summary>
		public const SByte MaxValue = 127;

		/// <summary>
		/// Represents the smallest possible value of System.SByte. This field is constant.
		/// </summary>
		public const SByte MinValue = -128;

		/// <summary>
		/// Compares this instance to a specified 8-bit signed integer and returns an indication
		/// of their relative values.
		/// </summary>
		/// <param name="value">An 8-bit signed integer to compare.</param>
		/// <returns>
		/// A signed integer that indicates the relative order of this instance and value.Return
		/// Value Description Less than zero This instance is less than value. Zero This
		/// instance is equal to value. Greater than zero This instance is greater than value.
		/// </returns>
		public extern int CompareTo(SByte value);
	}

	/// <summary>
	/// Represents a single-precision floating-point number.
	/// </summary>
	public struct Single : IComparable<Single>
	{
		/// <summary>
		/// Represents the smallest possible value of System.Single. This field is constant.
		/// </summary>
		public const Single MinValue = -3.40282347E+38F;
		/// <summary>
		/// Represents the smallest positive System.Single value that is greater than zero.
		/// This field is constant.
		/// </summary>
		public const Single Epsilon = 1.401298E-45F;
		/// <summary>
		/// Represents the largest possible value of System.Single. This field is constant.
		/// </summary>
		public const Single MaxValue = 3.40282347E+38F;
		/// <summary>
		/// Represents positive infinity. This field is constant.
		/// </summary>
		public const Single PositiveInfinity = 1F / 0F;
		/// <summary>
		/// Represents negative infinity. This field is constant.
		/// </summary>
		public const Single NegativeInfinity = -1F / 0F;
		/// <summary>
		/// Represents not a number (NaN). This field is constant.
		/// </summary>
		public const Single NaN = 0F / 0F;

		/// <summary>
		/// Compares this instance to a specified single-precision floating-point number
		/// and returns an integer that indicates whether the value of this instance is less
		/// than, equal to, or greater than the value of the specified single-precision floating-point
		/// number.
		/// </summary>
		/// <param name="other">A single-precision floating-point number to compare.</param>
		/// <returns>
		/// A signed number indicating the relative values of this instance and value.Return
		/// Value Description Less than zero This instance is less than value.-or- This instance
		/// is not a number (System.Single.NaN) and value is a number. Zero This instance
		/// is equal to value.-or- Both this instance and value are not a number (System.Single.NaN),
		/// System.Single.PositiveInfinity, or System.Single.NegativeInfinity. Greater than
		/// zero This instance is greater than value.-or- This instance is a number and value
		/// is not a number (System.Single.NaN).
		/// </returns>
		public extern int CompareTo(Single other);
	}

	/// <summary>
	/// Represents type declarations: class types, interface types, array types, value
	/// types, enumeration types, type parameters, generic type definitions, and open
	/// or closed constructed generic types.To browse the .NET Framework source code
	/// for this type, see the Reference Source.
	/// </summary>
	public abstract class Type : Reflection.MemberInfo
	{
		/// <summary>
		/// Gets the type referenced by the specified type handle.
		/// </summary>
		/// <param name="handle">The object that refers to the type.</param>
		/// <returns>
		/// The type referenced by the specified System.RuntimeTypeHandle, or null if the
		/// System.RuntimeTypeHandle.Value property of handle is null.
		/// </returns>
		public static extern Type GetTypeFromHandle(RuntimeTypeHandle handle);
	}

	/// <summary>
	/// Represents a 16-bit unsigned integer.
	/// </summary>
	public struct UInt16 : IComparable<UInt16>
	{
		/// <summary>
		/// Represents the largest possible value of System.UInt16. This field is constant.
		/// </summary>
		public const UInt16 MaxValue = 65535;
		/// <summary>
		/// Represents the smallest possible value of System.UInt16. This field is constant.
		/// </summary>
		public const UInt16 MinValue = 0;

		/// <summary>
		/// Compares this instance to a specified 16-bit unsigned integer and returns an
		/// indication of their relative values.
		/// </summary>
		/// <param name="other">An unsigned integer to compare.</param>
		/// <returns>
		/// A signed number indicating the relative values of this instance and value.Return
		/// Value Description Less than zero This instance is less than value. Zero This
		/// instance is equal to value. Greater than zero This instance is greater than value.
		/// </returns>
		public extern int CompareTo(UInt16 other);
	}

	/// <summary>
	/// Represents a 32-bit unsigned integer.
	/// </summary>
	public struct UInt32 : IComparable<UInt32>
	{
		/// <summary>
		/// Represents the largest possible value of System.UInt32. This field is constant.
		/// </summary>
		public const UInt32 MaxValue = 4294967295;
		/// <summary>
		/// Represents the smallest possible value of System.UInt32. This field is constant.
		/// </summary>
		public const UInt32 MinValue = 0;

		/// <summary>
		/// Compares this instance to a specified 32-bit unsigned integer and returns an
		/// indication of their relative values.
		/// </summary>
		/// <param name="other">An unsigned integer to compare.</param>
		/// <returns>
		/// A signed number indicating the relative values of this instance and value.Return
		/// value Description Less than zero This instance is less than value. Zero This
		/// instance is equal to value. Greater than zero This instance is greater than value.
		/// </returns>
		public extern int CompareTo(UInt32 other);
	}

	/// <summary>
	/// Represents a 64-bit unsigned integer.
	/// </summary>
	public struct UInt64 : IComparable<UInt64>
	{
		/// <summary>
		/// Represents the largest possible value of System.UInt64. This field is constant.
		/// </summary>
		public const UInt64 MaxValue = 18446744073709551615;
		/// <summary>
		/// Represents the smallest possible value of System.UInt64. This field is constant.
		/// </summary>
		public const UInt64 MinValue = 0;

		/// <summary>
		/// Compares this instance to a specified 64-bit unsigned integer and returns an
		/// indication of their relative values.
		/// </summary>
		/// <param name="other">An unsigned integer to compare.</param>
		/// <returns>
		/// A signed number indicating the relative values of this instance and value.Return
		/// Value Description Less than zero This instance is less than value. Zero This
		/// instance is equal to value. Greater than zero This instance is greater than value.
		/// </returns>
		public extern int CompareTo(UInt64 other);
	}

	/// <summary>
	/// A platform-specific type that is used to represent a pointer or a handle.
	/// </summary>
	public struct UIntPtr { }

	/// <summary>
	/// Provides the base class for value types.
	/// </summary>
	public abstract class ValueType { }

	/// <summary>
	/// Specifies a return value type for a method that does not return a value.
	/// </summary>
	public struct Void { }
}
