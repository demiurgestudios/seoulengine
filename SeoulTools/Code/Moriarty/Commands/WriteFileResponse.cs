// Response command, sent from server to client, triggered when a WriteFile RPC
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
	public sealed class WriteFileResponse : BaseMoriartyResponse
	{
		#region Private members
		UInt32 m_zSizeInBytes = 0u;
		bool m_bSuccess = false;
		#endregion

		public WriteFileResponse(UInt32 uResponseToken, UInt64 zSizeInBytes, UInt64 uOffsetInBytes, RemoteFileEntry entry, byte[] aData)
			: base(MoriartyConnection.ERPC.WriteFile, uResponseToken)
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

				// Cache the write sizes.
				int iSizeInBytes = (int)zSizeInBytes;
				long iOffsetInBytes = (long)uOffsetInBytes;

				// Seek to the write position.
				if (iOffsetInBytes != stream.Seek(iOffsetInBytes, SeekOrigin.Begin))
				{
					return;
				}

				// Write the data, cache the write size and mark success if we
				// don't encounter an exception.
				stream.Write(aData, 0, iSizeInBytes);
				m_zSizeInBytes = (UInt32)iSizeInBytes;
				m_bSuccess = true;
			}
			catch (Exception)
			{
				m_zSizeInBytes = 0u;
				m_bSuccess = false;
			}
		}

		/// <summary>
		/// The WriteFile response is successful if the data was written
		/// successfully, otherwise it is a generic failure. The response message
		/// includes the success code and the amount of data written.
		/// </summary>
		protected override void  WriteResponse(BinaryWriter writer)
		{
			MoriartyConnection.EResult eResult = (m_bSuccess
				? MoriartyConnection.EResult.ResultSuccess
				: MoriartyConnection.EResult.ResultGenericFailure);

			writer.NetWriteByte((byte)eResult);
			writer.NetWriteUInt64((UInt64)m_zSizeInBytes);
		}
	}
} // namespace Moriarty.Commands
