// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System.IO;

namespace SlimCSVS.Package.debug.commands
{
	sealed class Breakpoint : BaseCommand
	{
		#region Private members
		readonly string m_sFileName;
		readonly int m_iLine;
		#endregion

		public Breakpoint(string sFileName, int iLine)
			: base(Connection.ERPC.BreakpointCommand)
		{
			m_sFileName = sFileName;
			m_iLine = iLine;
		}

		protected override void WriteCommand(BinaryWriter writer)
		{
			writer.NetWriteString(m_sFileName);
			writer.NetWriteInt32(m_iLine);
		}
	}
}
