// Response command, sent from server to client, triggered when a SetFileModifiedTime RPC
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
	public sealed class SetFileModifiedTimeResponse : BaseMoriartyResponse
	{
		#region Private members
		bool m_bSuccess = false;
		#endregion

		public SetFileModifiedTimeResponse(UInt32 uResponseToken, string sAbsoluteFilename, UInt64 uUnixTimestamp)
			: base(MoriartyConnection.ERPC.SetFileModifiedTime, uResponseToken)
		{
			m_bSuccess = CoreLib.Utilities.SetModifiedTime(sAbsoluteFilename, uUnixTimestamp);
		}

		/// <summary>
		/// The SetFileModifiedTime response only includes a success or failure code.
		/// </summary>
		protected override void WriteResponse(BinaryWriter writer)
		{
			writer.NetWriteByte((byte)(m_bSuccess
				? MoriartyConnection.EResult.ResultSuccess
				: MoriartyConnection.EResult.ResultFileNotFound));
		}
	}
} // namespace Moriarty.Commands
