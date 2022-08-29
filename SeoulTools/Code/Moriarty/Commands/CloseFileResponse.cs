// Response command, sent from server to client, triggered when a CloseFile RPC
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
	public sealed class CloseFileResponse : BaseMoriartyResponse
	{
		#region Private members
		bool m_bSuccess = false;
		#endregion

		public CloseFileResponse(UInt32 uResponseToken, bool bSuccess)
			: base(MoriartyConnection.ERPC.CloseFile, uResponseToken)
		{
			m_bSuccess = bSuccess;
		}

		protected override void WriteResponse(BinaryWriter writer)
		{
			writer.NetWriteByte((byte)(m_bSuccess
				? MoriartyConnection.EResult.ResultSuccess
				: MoriartyConnection.EResult.ResultGenericFailure));
		}
	}
} // namespace Moriarty.Commands
