// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.Collections.Concurrent;
using System.IO;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using System.Threading;

namespace SlimCSVS.Package.debug
{
	public sealed class Message
	{
		/// <summary>
		/// Sanity check to catch bad messages and avoid crashes due to OOM allocation attempts.
		/// </summary>
		public const UInt32 kMaxMessageSize = (1 << 16);

		#region Private members
		UInt32 m_uTag = 0;
		MemoryStream m_Data;
		BinaryReader m_Reader;
		BinaryWriter m_Writer;

		BinaryReader Reader
		{
			get
			{
				if (null == m_Data)
				{
					throw new System.IO.IOException("no data");
				}
				else if (null == m_Reader)
				{
					m_Reader = new BinaryReader(m_Data);
				}

				return m_Reader;
			}
		}

		BinaryWriter Writer
		{
			get
			{
				if (null == m_Data)
				{
					m_Data = new MemoryStream();
					m_Writer = new BinaryWriter(m_Data);
				}

				return m_Writer;
			}
		}
		#endregion

		// These methods return "simple" messages, with the eTag type and no data (the message size field is 0).
		public static Message Create(ClientTag eTag)
		{
			var ret = new Message();
			ret.m_uTag = (UInt32)eTag;
			return ret;
		}

		public static Message Create(ServerTag eTag)
		{
			var ret = new Message();
			ret.m_uTag = (UInt32)eTag;
			return ret;
		}

		/// <summary>
		/// Used to generate a Message structure from incoming data.
		/// </summary>
		public static Message Create(BinaryReader reader)
		{
			var uMessageSize = reader.LittleEndianReadUInt32();
			var uTag = reader.LittleEndianReadUInt32();

			// Sanity check for bad data.
			if (uMessageSize > kMaxMessageSize)
			{
				return null;
			}

			// Read the data blob if the message has data.
			var aData = reader.ReadBytes((int)uMessageSize);

			// Instantiate a Message and populate with the read data.
			var ret = new Message();
			ret.m_Data = new MemoryStream(aData, false);
			ret.m_uTag = uTag;
			return ret;
		}

		/// <summary>
		/// Return the total size of data stored in this message, in bytes.
		/// </summary>
		public long DataLength { get { return (null == m_Data ? 0 : m_Data.Length); } }

		/// <summary>
		/// Return true if the read buffer of this Message has data remaining.
		/// </summary>
		public bool HasData
		{
			get
			{
				var reader = Reader;
				var stream = reader.BaseStream;
				return stream.Position < stream.Length;
			}
		}

		/// <summary>
		/// The identifying tag of this message.
		/// </summary>
		public uint MessageTag
		{
			get
			{
				return m_uTag;
			}
		}

		// BinaryReader equivalents.
		public bool ReadBool() { return Reader.LittleEndianReadByte() != 0; }
		public Int16 ReadInt16() { return Reader.LittleEndianReadInt16(); }
		public Int32 ReadInt32() { return Reader.LittleEndianReadInt32(); }
		public Int64 ReadInt64() { return Reader.LittleEndianReadInt64(); }
		public string ReadString() { return Reader.LittleEndianReadString(); }
		public UInt16 ReadUInt16() { return Reader.LittleEndianReadUInt16(); }
		public UInt32 ReadUInt32() { return Reader.LittleEndianReadUInt32(); }
		public UInt64 ReadUInt64() { return Reader.LittleEndianReadUInt64(); }

		// Write contents into an output network stream.
		public void Send(BinaryWriter writer)
		{
			lock (this)
			{
				// Flush data to writer.
				var iLength = DataLength;
				writer.Write((UInt32)iLength);
				writer.Write((UInt32)m_uTag);
				if (iLength > 0)
				{
					// Commit.
					m_Writer.Flush();
					writer.Write(m_Data.GetBuffer(), 0, (int)iLength);
				}
				writer.Flush();
			}
		}

		// BinaryWriter equivalents.
		public void Write(bool b) { Writer.LittleEndianWrite(b ? (byte)1 : (byte)0); }
		public void Write(Int16 v) { Writer.LittleEndianWrite(v); }
		public void Write(Int32 v) { Writer.LittleEndianWrite(v); }
		public void Write(Int64 v) { Writer.LittleEndianWrite(v); }
		public void Write(string s) { Writer.LittleEndianWrite(s); }
		public void Write(UInt16 v) { Writer.LittleEndianWrite(v); }
		public void Write(UInt32 v) { Writer.LittleEndianWrite(v); }
		public void Write(UInt64 v) { Writer.LittleEndianWrite(v); }
	}
}
