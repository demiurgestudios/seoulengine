// Response command, sent from server to client, triggered when an OpenFile RPC
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
	public sealed class OpenFileResponse : BaseMoriartyResponse
	{
		#region Private members
		int m_iHandle = 0;
		UInt64 m_zFileSizeInBytes = 0u;
		UInt64 m_uUnixTimestamp = 0u;
		bool m_bDirectory = false;
		#endregion

		public OpenFileResponse(UInt32 uResponseToken, int iHandle, string sAbsoluteFilename)
			: base(MoriartyConnection.ERPC.OpenFile, uResponseToken)
		{
			m_iHandle = iHandle;

			// Get the values that must be returned for the stat, if we have a valid handle.
			if (0 != m_iHandle)
			{
				m_zFileSizeInBytes = CoreLib.Utilities.GetFileSize(sAbsoluteFilename);
				m_uUnixTimestamp = CoreLib.Utilities.GetModifiedTime(sAbsoluteFilename);
				m_bDirectory = Directory.Exists(sAbsoluteFilename);
			}
		}

		/// <summary>
		/// The OpenFile response is successful if we have a valid file timestamp,
		/// otherwise it is classified as failed due to a FileNotFound error.
		/// </summary>
		protected override void WriteResponse(BinaryWriter writer)
		{
			writer.NetWriteByte((byte)(0u == m_uUnixTimestamp
				? MoriartyConnection.EResult.ResultFileNotFound
				: MoriartyConnection.EResult.ResultSuccess));
			writer.NetWriteInt32(m_iHandle);
			writer.NetWriteByte((byte)(m_bDirectory ? MoriartyConnection.kFlag_StatFile_Directory : 0));
			writer.NetWriteUInt64(m_zFileSizeInBytes);
			writer.NetWriteUInt64(m_uUnixTimestamp);
		}
	}
} // namespace Moriarty.Commands
