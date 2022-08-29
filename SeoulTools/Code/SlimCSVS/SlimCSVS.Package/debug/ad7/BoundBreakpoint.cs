// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using Microsoft.VisualStudio;
using Microsoft.VisualStudio.Debugger.Interop;
using System;

namespace SlimCSVS.Package.debug.ad7
{
	sealed class BoundBreakpoint : IDebugBoundBreakpoint2
	{
		#region Private members
		readonly Engine m_Engine;
		readonly UInt32 m_uRawBreakpoint;
		readonly BreakpointResolution m_BreakpointResolution;
		readonly PendingBreakpoint m_PendingBreakpoint;

		bool m_bDeleted = false;
		bool m_bEnabled = true;
		#endregion

		#region Overrides
		int IDebugBoundBreakpoint2.Delete()
		{
			// Early out if already deleted.
			if (m_bDeleted)
			{
				return VSConstants.S_OK;
			}

			m_bDeleted = true;
			m_Engine.OnBreakpointStateChange(m_uRawBreakpoint, false);
			m_PendingBreakpoint.OnBoundBreakpointDeleted(this);
			return VSConstants.S_OK;
		}

		int IDebugBoundBreakpoint2.Enable(int iEnable)
		{
			var bEnable = (0 != iEnable);
			if (m_bEnabled != bEnable)
			{
				m_bEnabled = bEnable;
				m_Engine.OnBreakpointStateChange(m_uRawBreakpoint, bEnable);
			}
			return VSConstants.S_OK;
		}

		int IDebugBoundBreakpoint2.GetBreakpointResolution(out IDebugBreakpointResolution2 ppBPResolution)
		{
			ppBPResolution = m_BreakpointResolution;
			return VSConstants.S_OK;
		}

		int IDebugBoundBreakpoint2.GetHitCount(out uint pdwHitCount)
		{
			pdwHitCount = 0;
			return VSConstants.E_NOTIMPL;
		}

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

		int IDebugBoundBreakpoint2.SetCondition(BP_CONDITION bpCondition)
		{
			return VSConstants.E_NOTIMPL;
		}

		int IDebugBoundBreakpoint2.SetHitCount(uint dwHitCount)
		{
			return VSConstants.E_NOTIMPL;
		}

		int IDebugBoundBreakpoint2.SetPassCount(BP_PASSCOUNT bpPassCount)
		{
			return VSConstants.E_NOTIMPL;
		}
		#endregion

		public BoundBreakpoint(
			Engine engine,
			UInt32 uRawBreakpoint,
			PendingBreakpoint pendingBreakpoint,
			BreakpointResolution breakpointResolution)
		{
			m_Engine = engine;
			m_uRawBreakpoint = uRawBreakpoint;
			m_BreakpointResolution = breakpointResolution;
			m_PendingBreakpoint = pendingBreakpoint;
		}

		public UInt32 RawBreakpoint { get { return m_uRawBreakpoint; } }
		public bool Valid { get { return m_bEnabled && !m_bDeleted; } }
	}
}
