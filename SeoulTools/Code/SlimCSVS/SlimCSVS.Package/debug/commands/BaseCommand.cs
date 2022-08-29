// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System.IO;

namespace SlimCSVS.Package.debug.commands
{
	public abstract class BaseCommand
	{
		#region Private members
		Connection.ERPC m_eRPC = Connection.ERPC.Unknown;
		#endregion

		protected BaseCommand(Connection.ERPC eRPC)
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
}
