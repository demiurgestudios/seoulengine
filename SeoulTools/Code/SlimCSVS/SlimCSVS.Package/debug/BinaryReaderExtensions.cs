// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.IO;

namespace SlimCSVS.Package.debug
{
	public static class BinaryReaderExtensions
	{
		/// <summary>
		/// Read a single byte from reader, identical behavior to BinaryReader.ReadByte().
		/// </summary>
		public static byte LittleEndianReadByte(this BinaryReader reader)
		{
			return reader.ReadByte();
		}

		/// <summary>
		/// Read a 16-bit signed integer from reader, endian swapped from little endian to current.
		/// </summary>
		public static Int16 LittleEndianReadInt16(this BinaryReader reader)
		{
			var ret = reader.ReadInt16();
			if (!BitConverter.IsLittleEndian)
			{
				ret = unchecked((Int16)util.Utilities.EndianSwap((UInt16)ret));
			}
			return ret;
		}

		/// <summary>
		/// Read a 32-bit signed integer from reader, endian swapped from little endian to current.
		/// </summary>
		public static Int32 LittleEndianReadInt32(this BinaryReader reader)
		{
			var ret = reader.ReadInt32();
			if (!BitConverter.IsLittleEndian)
			{
				ret = unchecked((Int32)util.Utilities.EndianSwap((UInt32)ret));
			}
			return ret;
		}

		/// <summary>
		/// Read a 64-bit signed integer from reader, endian swapped from network to host endianness.
		/// </summary>
		public static Int64 LittleEndianReadInt64(this BinaryReader reader)
		{
			var ret = reader.ReadInt64();
			if (!BitConverter.IsLittleEndian)
			{
				ret = unchecked((Int64)util.Utilities.EndianSwap((UInt64)ret));
			}
			return ret;
		}

		/// <summary>
		/// Read a UTF8 encoded string from reader, either from a Seoul::HString or Seoul::String.
		/// </summary>
		public static string LittleEndianReadString(this BinaryReader reader)
		{
			Int32 iSizeInBytes = reader.LittleEndianReadInt32();

			// Special case for the empty string, since the NetworkStream can
			// block if we ask it to read 0 bytes
			if (iSizeInBytes == 0)
			{
				return "";
			}

			byte[] asString = reader.ReadBytes(iSizeInBytes);

			return System.Text.Encoding.UTF8.GetString(asString, 0, iSizeInBytes);
		}

		/// <summary>
		/// Read a 16-bit unsigned integer from reader, endian swapped from network to host endianness.
		/// </summary>
		public static UInt16 LittleEndianReadUInt16(this BinaryReader reader)
		{
			return unchecked((UInt16)reader.LittleEndianReadInt16());
		}

		/// <summary>
		/// Read a 32-bit unsigned integer from reader, endian swapped from network to host endianness.
		/// </summary>
		public static UInt32 LittleEndianReadUInt32(this BinaryReader reader)
		{
			return unchecked((UInt32)reader.LittleEndianReadInt32());
		}

		/// <summary>
		/// Read a 64-bit unsigned integer from reader, endian swapped from network to host endianness.
		/// </summary>
		public static UInt64 LittleEndianReadUInt64(this BinaryReader reader)
		{
			return unchecked((UInt64)reader.LittleEndianReadInt64());
		}
	}
}
