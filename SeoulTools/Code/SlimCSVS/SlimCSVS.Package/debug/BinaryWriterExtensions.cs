// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.IO;

namespace SlimCSVS.Package.debug
{
	public static class BinaryWriterExtensions
	{
		/// <summary>
		/// Write a single byte to writer, identical behavior to BinaryReader.WriteByte().
		/// </summary>
		public static void LittleEndianWrite(this BinaryWriter writer, byte value)
		{
			writer.Write(value);
		}

		/// <summary>
		/// Write a 16-bit signed integer to writer, endian swapped from host to network endianness.
		/// </summary>
		public static void LittleEndianWrite(this BinaryWriter writer, Int16 value)
		{
			if (!BitConverter.IsLittleEndian)
			{
				value = unchecked((Int16)util.Utilities.EndianSwap((UInt16)value));
			}

			writer.Write(value);
		}

		/// <summary>
		/// Write a 32-bit signed integer to writer, endian swapped from host to network endianness.
		/// </summary>
		public static void LittleEndianWrite(this BinaryWriter writer, Int32 value)
		{
			if (!BitConverter.IsLittleEndian)
			{
				value = unchecked((Int32)util.Utilities.EndianSwap((UInt32)value));
			}

			writer.Write(value);
		}

		/// <summary>
		/// Write a 64-bit signed integer to writer, endian swapped from host to network endianness.
		/// </summary>
		public static void LittleEndianWrite(this BinaryWriter writer, Int64 value)
		{
			if (!BitConverter.IsLittleEndian)
			{
				value = unchecked((Int64)util.Utilities.EndianSwap((UInt64)value));
			}

			writer.Write(value);
		}

		/// <summary>
		/// Write a UTF8 encoded string to writer, to be consumed by either Seoul::String or Seoul::HString.
		/// </summary>
		public static void LittleEndianWrite(this BinaryWriter writer, string sValue)
		{
			byte[] asString = System.Text.Encoding.UTF8.GetBytes(sValue);
			writer.LittleEndianWrite((UInt32)asString.Length);
			writer.Write(asString);
		}

		/// <summary>
		/// Write a 16-bit unsigned integer to writer, endian swapped from host to network endianness.
		/// </summary>
		public static void LittleEndianWrite(this BinaryWriter writer, UInt16 value)
		{
			LittleEndianWrite(writer, unchecked((Int16)value));
		}

		/// <summary>
		/// Write a 32-bit unsigned integer to writer, endian swapped from host to network endianness.
		/// </summary>
		public static void LittleEndianWrite(this BinaryWriter writer, UInt32 value)
		{
			LittleEndianWrite(writer, unchecked((Int32)value));
		}

		/// <summary>
		/// Write a 64-bit unsigned integer to writer, endian swapped from host to network endianness.
		/// </summary>
		public static void LittleEndianWrite(this BinaryWriter writer, UInt64 value)
		{
			LittleEndianWrite(writer, unchecked((Int64)value));
		}

	}
}
