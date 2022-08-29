// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using Microsoft.VisualStudio;
using Microsoft.VisualStudio.Debugger.Interop;

namespace SlimCSVS.Package.debug
{
	sealed class BoundBreakpoint : IDebugBoundBreakpoint2
	{
		#region Private members
		readonly Engine m_Engine;
		readonly string m_sFileName;
		readonly int m_iLine;
		readonly BreakpointResolution m_BreakpointResolution;
		readonly PendingBreakpoint m_PendingBreakpoint;

		bool m_bDeleted = false;
		bool m_bEnabled = true;
		#endregion

		#region Overrides
		int IDebugBoundBreakpoint2.Delete() { return VSConstants.S_OK; }
		int IDebugBoundBreakpoint2.Enable(int fEnable) { return VSConstants.S_OK; }

		int IDebugBoundBreakpoint2.GetBreakpointResolution(out IDebugBreakpointResolution2 ppBPResolution)
		{
			ppBPResolution = m_BreakpointResolution;
			return VSConstants.S_OK;
		}

		int IDebugBoundBreakpoint2.GetHitCount(out uint pdwHitCount) { pdwHitCount = 0; return VSConstants.E_NOTIMPL; }

		int IDebugBoundBreakpoint2.GetPendingBreakpoint(out IDebugPendingBreakpoint2 ppPendingBreakpoint)
		{
			ppPendingBreakpoint = m_PendingBreakpoint;
			return VSConstants.S_OK;
		}

		int IDebugBoundBreakpoint2.GetState(enum_BP_STATE[] pState)
		{
			// Invalid initially.
			pState[0] = 0;

			// Setup based on state and priorities.
			if (m_bDeleted)       { pState[0] = enum_BP_STATE.BPS_DELETED; }
			else if (m_bEnabled)  { pState[0] = enum_BP_STATE.BPS_ENABLED; }
			else if (!m_bEnabled) { pState[0] = enum_BP_STATE.BPS_DISABLED; }

			return VSConstants.S_OK;
		}

		int IDebugBoundBreakpoint2.SetCondition(BP_CONDITION bpCondition) { return VSConstants.E_NOTIMPL; }
		int IDebugBoundBreakpoint2.SetHitCount(uint dwHitCount) { return VSConstants.E_NOTIMPL; }
		int IDebugBoundBreakpoint2.SetPassCount(BP_PASSCOUNT bpPassCount) { return VSConstants.E_NOTIMPL; }
		#endregion

		public BoundBreakpoint(
			Engine engine,
			string sFileName,
			int iLine,
			PendingBreakpoint pendingBreakpoint,
			BreakpointResolution breakpointResolution)
		{
			m_Engine = engine;
			m_sFileName = sFileName;
			m_iLine = iLine;
			m_BreakpointResolution = breakpointResolution;
			m_PendingBreakpoint = pendingBreakpoint;
		}

		public string FileName { get { return m_sFileName; } }
		public int Line { get { return m_iLine; } }
	}
}
