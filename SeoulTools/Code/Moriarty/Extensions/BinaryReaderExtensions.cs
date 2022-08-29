// Extensions to the BinaryReader class, used for reading data from
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
	public static class BinaryReaderExtensions
	{
		/// <summary>
		/// Read a single byte from reader, identical behavior to BinaryReader.ReadByte()
		/// </summary>
		public static byte NetReadByte(this BinaryReader reader)
		{
			return reader.ReadByte();
		}

		/// <summary>
		/// Read a 32-bit signed integer from reader, endian swapped from network to host endianness
		/// </summary>
		public static Int32 NetReadInt32(this BinaryReader reader)
		{
			return IPAddress.NetworkToHostOrder(reader.ReadInt32());
		}

		/// <summary>
		/// Read a 64-bit signed integer from reader, endian swapped from network to host endianness
		/// </summary>
		public static Int64 NetReadInt64(this BinaryReader reader)
		{
			return IPAddress.NetworkToHostOrder(reader.ReadInt64());
		}

		/// <summary>
		/// Read a FilePath structure from reader
		/// </summary>
		public static FilePath NetReadFilePath(this BinaryReader reader)
		{
			FilePath ret = new FilePath();
			ret.m_eGameDirectory = (CoreLib.GameDirectory)reader.NetReadByte();
			ret.m_eFileType = (CoreLib.FileType)reader.NetReadByte();
			ret.m_sRelativePathWithoutExtension = reader.NetReadString();
			return ret;
		}

		/// <summary>
		/// Read an array of FilePath structures from reader
		/// </summary>
		public static FilePath[] NetReadFilePathArray(this BinaryReader reader)
		{
			UInt32 nFilePaths = reader.NetReadUInt32();
			FilePath[] aReturn = new FilePath[nFilePaths];
			for (UInt32 i = 0u; i < nFilePaths; ++i)
			{
				aReturn[i] = reader.NetReadFilePath();
			}

			return aReturn;
		}

		/// <summary>
		/// Read a UTF8 encoded string from reader, either from a Seoul::HString or Seoul::String
		/// </summary>
		public static string NetReadString(this BinaryReader reader)
		{
			Int32 zSizeInBytes = reader.NetReadInt32();

			// Special case for the empty string, since the NetworkStream can
			// block if we ask it to read 0 bytes
			if (zSizeInBytes == 0)
			{
				return "";
			}

			byte[] asString = reader.ReadBytes(zSizeInBytes);

			return System.Text.Encoding.UTF8.GetString(asString, 0, zSizeInBytes);
		}

		/// <summary>
		/// Read a 32-bit unsigned integer from reader, endian swapped from network to host endianness
		/// </summary>
		public static UInt32 NetReadUInt32(this BinaryReader reader)
		{
			return unchecked((UInt32)IPAddress.NetworkToHostOrder(reader.ReadInt32()));
		}

		/// <summary>
		/// Read a 64-bit unsigned integer from reader, endian swapped from network to host endianness
		/// </summary>
		public static UInt64 NetReadUInt64(this BinaryReader reader)
		{
			return unchecked((UInt64)IPAddress.NetworkToHostOrder(reader.ReadInt64()));
		}
	}
} // namespace Moriarty.Extensions
