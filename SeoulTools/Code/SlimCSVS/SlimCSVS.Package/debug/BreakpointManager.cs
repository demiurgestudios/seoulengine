// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using Microsoft.VisualStudio.Debugger.Interop;
using System;
using System.Collections.Generic;

namespace SlimCSVS.Package.debug
{
	sealed class BreakpointManager
	{
		#region Private members
		readonly Engine m_Engine;
		readonly List<PendingBreakpoint> m_PendingBreakpoints = new List<PendingBreakpoint>();
		readonly Dictionary<string, BoundBreakpoint> m_BoundBreakpoints = new Dictionary<string, BoundBreakpoint>();
		#endregion

		public BreakpointManager(Engine engine)
		{
			m_Engine = engine;
		}

		public void CreatePendingBreakpoint(
			IDebugBreakpointRequest2 pBPRequest,
			out IDebugPendingBreakpoint2 ppPendingBP)
		{
			lock (this)
			{
				var pb = new PendingBreakpoint(m_Engine, this, pBPRequest);
				ppPendingBP = (IDebugPendingBreakpoint2)pb;
				m_PendingBreakpoints.Add(pb);
			}
		}

		public void SendAllBound(Connection connection)
		{
			BoundBreakpoint[] aBP = null;
			int iOut = 0;
			lock (this)
			{
				aBP = new BoundBreakpoint[m_BoundBreakpoints.Count];
				foreach (var t in m_BoundBreakpoints)
				{
					aBP[iOut++] = t.Value;
				}
			}

			var a = new commands.Breakpoint[iOut];
			for (int i = 0; i < iOut; ++i)
			{
				a[i] = new commands.Breakpoint() { m_sFileName = aBP[i].FileName, m_iLine = aBP[i].Line };
			}

			connection.SendCommand(new commands.Breakpoints(a));
		}

		public void StoreBoundBreakpoint(
			string sFileAndLine,
			BoundBreakpoint bp)
		{
			lock (this)
			{
				m_BoundBreakpoints.Add(sFileAndLine, bp);
			}
		}


		public bool TryGetBoundBreakpoint(string sFileAndLine, out BoundBreakpoint bp)
		{
			lock (this)
			{
				return m_BoundBreakpoints.TryGetValue(sFileAndLine, out bp);
			}
		}
	}
}
