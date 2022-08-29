// Base class of all commands that can be sent from the Moriarty server to a Moriarty
// client.
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
	public abstract class BaseMoriartyCommand
	{
		#region Private members
		MoriartyConnection.ERPC m_eRPC = MoriartyConnection.ERPC.Unknown;
		#endregion

		protected BaseMoriartyCommand(MoriartyConnection.ERPC eRPC)
		{
			m_eRPC = eRPC;
		}

		protected abstract void WriteCommand(BinaryWriter writer);

		/// <summary>
		/// A command always includes an RPC identifier, followed by
		/// command specific data defined by the WriteCommand() override.
		/// </summary>
		public void Write(BinaryWriter writer)
		{
			writer.NetWriteByte((byte)m_eRPC);
			WriteCommand(writer);
		}
	}
} // namespace Moriarty.Commands
