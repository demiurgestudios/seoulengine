// Response command, sent from server to client, triggered when a CreateDirPath RPC
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
	public sealed class DeleteDirectoryResponse : BaseMoriartyResponse
	{
		#region Private members
		bool m_bSuccess = false;
		#endregion

		public DeleteDirectoryResponse(UInt32 uResponseToken, string sAbsoluteFilename, bool bRecursive)
			: base(MoriartyConnection.ERPC.DeleteDirectory, uResponseToken)
		{
			try
			{
				Directory.Delete(sAbsoluteFilename, bRecursive);
                m_bSuccess = true;
			}
			catch (Exception)
			{
				m_bSuccess = false;
			}
		}

        /// <summary>
        /// The DeleteDirectory response only includes a success or failure code.
        /// </summary>
        protected override void WriteResponse(BinaryWriter writer)
		{
			writer.NetWriteByte((byte)(m_bSuccess
				? MoriartyConnection.EResult.ResultSuccess
				: MoriartyConnection.EResult.ResultFileNotFound));
		}
	}
} // namespace Moriarty.Commands
