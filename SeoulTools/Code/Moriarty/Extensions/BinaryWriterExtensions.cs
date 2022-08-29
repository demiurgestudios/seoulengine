// Extensions to the BinaryWriter class, used for writing data to
// a NetworkStream in formats expected by MoriartyConnection objects.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Diagnostics;
using System.Text;
using System.Threading;

using Moriarty.Utilities;

namespace Moriarty.Extensions
{
	public static class BinaryWriterExtensions
	{
		public static void NetWriteByte(this BinaryWriter writer, byte value)
		{
			writer.Write(value);
		}

		/// <summary>
		/// Write a 32-bit signed integer to writer, endian swapped from host to network endianness
		/// </summary>
		public static void NetWriteInt32(this BinaryWriter writer, Int32 value)
		{
			writer.Write(IPAddress.HostToNetworkOrder(value));
		}

		/// <summary>
		/// Write a 64-bit signed integer to writer, endian swapped from host to network endianness
		/// </summary>
		public static void NetWriteInt64(this BinaryWriter writer, Int64 value)
		{
			writer.Write(IPAddress.HostToNetworkOrder(value));
		}

		/// <summary>
		/// Write a FilePath structure to writer
		/// </summary>
		public static void NetWriteFilePath(this BinaryWriter writer, FilePath filePath)
		{
			writer.NetWriteByte((byte)filePath.m_eGameDirectory);
			writer.NetWriteByte((byte)filePath.m_eFileType);
			writer.NetWriteString(filePath.m_sRelativePathWithoutExtension);
		}

		/// <summary>
		/// Write a UTF8 encoded string to writer, to be consumed by either Seoul::String or Seoul::HString
		/// </summary>
		public static void NetWriteString(this BinaryWriter writer, string sValue)
		{
			byte[] asString = System.Text.Encoding.UTF8.GetBytes(sValue);
			writer.NetWriteUInt32((UInt32)asString.Length);
			writer.Write(asString);
		}

		/// <summary>
		/// Write a 32-bit unsigned integer to writer, endian swapped from host to network endianness
		/// </summary>
		public static void NetWriteUInt32(this BinaryWriter writer, UInt32 value)
		{
			writer.Write(IPAddress.HostToNetworkOrder(unchecked((Int32)value)));
		}

		/// <summary>
		/// Write a 64-bit unsigned integer to writer, endian swapped from host to network endianness
		/// </summary>
		public static void NetWriteUInt64(this BinaryWriter writer, UInt64 value)
		{
			writer.Write(IPAddress.HostToNetworkOrder(unchecked((Int64)value)));
		}

		/// <summary>
		/// brief Write an array of UTF-8 strings to write
		/// </summary>
		public static void NetWriteStringArray(this BinaryWriter writer, string[] asValues)
		{
			writer.NetWriteUInt32((UInt32)asValues.Length);
			foreach (string sStr in asValues)
			{
				writer.NetWriteString(sStr);
			}
		}
	}
} // namespace Moriarty.Extensions
