// Response command, sent from server to client, triggered when a Cook RPC
// is submitted to the server, set to cook a single file.
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
using Moriarty.Utilities;

namespace Moriarty.Commands
{
	public sealed class CookFileResponse : BaseMoriartyResponse
	{
		#region Private members
		UInt64 m_zFileSizeInBytes = 0u;
		UInt64 m_uUnixTimestamp = 0u;
		CookResult m_eResult = CookResult.ErrorMissingCookerSupport;
        FilePath m_FilePath = default(FilePath);
		#endregion

		public CookFileResponse(
            FilePath filePath,
            UInt64 zFileSizeInBytes,
            UInt64 uUnixTimestamp,
            UInt32 uResponseToken,
            CookResult eResult)
			: base(MoriartyConnection.ERPC.CookFile, uResponseToken)
		{
            m_zFileSizeInBytes = zFileSizeInBytes;
            m_uUnixTimestamp = uUnixTimestamp;
			m_eResult = eResult;
            m_FilePath = filePath;
		}

		/// <summary>
		/// The CookFile response is successful if the cook returned either
		/// CookResult.Success or CookResult.UpToDate. The response includes the general
		/// RPC result token and the specific CookResult token.
		/// </summary>
		protected override void WriteResponse(BinaryWriter writer)
		{
            writer.NetWriteFilePath(m_FilePath);
			writer.NetWriteUInt64(m_zFileSizeInBytes);
			writer.NetWriteUInt64(m_uUnixTimestamp);
			writer.NetWriteByte((byte)((CookResult.Success == m_eResult || CookResult.UpToDate == m_eResult)
				? MoriartyConnection.EResult.ResultSuccess
				: MoriartyConnection.EResult.ResultGenericFailure));
			writer.NetWriteUInt32((UInt32)m_eResult);
		}
	}
} // namespace Moriarty.Commands
