// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System.Collections.Generic;
using System.IO;

namespace SlimCSVS.Package.debug.commands
{
	struct Breakpoint
	{
		public string m_sFileName;
		public int m_iLine;
		public bool m_bEnabled;
	}

	sealed class Breakpoints : BaseCommand
	{
		#region Private members
		readonly Breakpoint[] m_aBreakpoints;
		#endregion

		public Breakpoints(Breakpoint[] aBreakpoints)
			: base(Connection.ERPC.BreakpointsCommand)
		{
			m_aBreakpoints = aBreakpoints;
		}

		protected override void WriteCommand(BinaryWriter writer)
		{
			writer.NetWriteInt32(m_aBreakpoints.Length);
			foreach (var bp in m_aBreakpoints)
			{
				writer.NetWriteString(bp.m_sFileName);
				writer.NetWriteInt32(bp.m_iLine);
				writer.NetWriteByte(bp.m_bEnabled ? (byte)1 : (byte)0);
			}
		}
	}
}
