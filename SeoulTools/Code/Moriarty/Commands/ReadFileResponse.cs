// Response command, sent from server to client, triggered when a ReadFile RPC
// is submitted to the server.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

using Moriarty.Extensions;

namespace Moriarty.Commands
{
	public sealed class ReadFileResponse : BaseMoriartyResponse
	{
		#region Private members
		byte[] m_aData = null;
		bool m_bCompressed = false;
		#endregion

		public ReadFileResponse(UInt32 uResponseToken, UInt64 zSizeInBytes, UInt64 uOffsetInBytes, RemoteFileEntry entry)
			: base(MoriartyConnection.ERPC.ReadFile, uResponseToken)
		{
			try
			{
				// Check if we have a stream - if not, fail.
				Stream stream = (null != entry ? entry.Stream : null);
				if (null == stream)
				{
					return;
				}

				// TODO: Support or enforce this on the client side.
				if (zSizeInBytes > int.MaxValue ||
					uOffsetInBytes > long.MaxValue)
				{
					return;
				}

				// Cache the size and offset.
				int iSizeInBytes = (int)zSizeInBytes;
				long iOffsetInBytes = (long)uOffsetInBytes;

				// Seek to the read position.
				if (iOffsetInBytes != stream.Seek(iOffsetInBytes, SeekOrigin.Begin))
				{
					return;
				}

				// Read the data.
				m_aData = new byte[iSizeInBytes];
				int iReadSizeInBytes = stream.Read(m_aData, 0, iSizeInBytes);

				// Resize the array to match the read size.
				Array.Resize(ref m_aData, iReadSizeInBytes);

				// If we can potentially compress the send, attempt that now.
				if (entry.AllowCompression)
				{
					byte[] aCompressedData = CoreLib.Utilities.LZ4Compress(m_aData);

					// If compression was successful and the compressed data is
					// smaller than the uncompressed data, send the data
					// compressed.
					if (aCompressedData != null &&
						aCompressedData.Length < m_aData.Length)
					{
						m_bCompressed = true;
						m_aData = aCompressedData;
					}
				}
			}
			catch (Exception)
			{
				m_aData = null;
			}
		}

		/// <summary>
		/// The ReadFile response is successful if we have a non-NULL data buffer,
		/// otherwise the read file failed. The response includes the read data as well
		/// as the result code.
		/// </summary>
		protected override void  WriteResponse(BinaryWriter writer)
		{
			MoriartyConnection.EResult eResult = ((null != m_aData)
				? MoriartyConnection.EResult.ResultSuccess
				: MoriartyConnection.EResult.ResultGenericFailure);

			// Write the result code and the size of the read data in bytes.
			writer.NetWriteByte(m_bCompressed ? (byte)1 : (byte)0);
			writer.NetWriteByte((byte)eResult);
			writer.NetWriteUInt64((UInt64)m_aData.Length);

			// If we have some data, write the buffer.
			if (m_aData.Length > 0)
			{
				writer.Write(m_aData);
			}
		}
	}
} // namespace Moriarty.Commands
