// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.IO;

namespace SlimCSVS.Package.debug.commands
{
	public abstract class BaseResponse : BaseCommand
	{
		#region Private members
		UInt32 m_uResponseToken = 0u;
		#endregion

		/// <summary>
		/// A response always includes an RPC identifier, a response token,
		/// followed by response specific data defined by the WriteCommand() override.
		/// </summary>
		protected override sealed void WriteCommand(BinaryWriter writer)
		{
			writer.NetWriteUInt32(m_uResponseToken);
			WriteResponse(writer);
		}

		/// <summary>
		/// BaseResponse is always constructed with the RPC that we are responding
		/// </summary>
		protected BaseResponse(Connection.ERPC eRPC, UInt32 uResponseToken)
			: base(eRPC | Connection.ERPC.ResponseFlag)
		{
			m_uResponseToken = uResponseToken;
		}

		protected abstract void WriteResponse(BinaryWriter writer);
	}
}
