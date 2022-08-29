// Base class of the subset of commands that can be sent from the Moriarty server to a Moriarty
// client in response to an RPC previously sent from the client to the server.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.Collections.Generic;
using System.Linq;
using System.IO;
using System.Text;

using Moriarty.Extensions;

namespace Moriarty.Commands
{
	public abstract class BaseMoriartyResponse : BaseMoriartyCommand
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
		/// BaseMoriartyResponse is always constructed with the RPC that we are responding
		/// to eRPC, and the token used to uniquely identify this response to the specific RPC
		/// that triggered the response.
		/// </summary>
		protected BaseMoriartyResponse(MoriartyConnection.ERPC eRPC, UInt32 uResponseToken)
			: base(eRPC | MoriartyConnection.ERPC.ResponseFlag)
		{
			m_uResponseToken = uResponseToken;
		}

		protected abstract void WriteResponse(BinaryWriter writer);
	}
} // namespace Moriarty.Commands
