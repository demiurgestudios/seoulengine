// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using Microsoft.VisualStudio;
using Microsoft.VisualStudio.Debugger.Interop;
using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;

namespace SlimCSVS.Package.debug.ad7
{
	sealed class PendingBreakpoint : IDebugPendingBreakpoint2
	{
		#region Private members
		readonly Engine m_Engine;
		readonly BreakpointManager m_Manager;
		readonly IDebugBreakpointRequest2 m_pRequest;
		readonly BP_REQUEST_INFO m_RequestInfo;
		readonly List<BoundBreakpoint> m_BoundBreakpoints = new List<BoundBreakpoint>();
		bool m_bDeleted = false;
		bool m_bEnabled = true;

		bool CanBind
		{
			get
			{
				if (m_bDeleted ||
					m_RequestInfo.bpLocation.bpLocationType != (uint)enum_BP_LOCATION_TYPE.BPLT_CODE_FILE_LINE)
				{
					return false;
				}

				return true;
			}
		}
		#endregion

		#region Overrides
		public int Bind()
		{
			// Early out if binding is not possible.
			if (!CanBind)
			{
				return VSConstants.S_FALSE;
			}

			var docPosition = (IDebugDocumentPosition2)(Marshal.GetObjectForIUnknown(m_RequestInfo.bpLocation.unionmember2));

			string sFileName;
			docPosition.GetFileName(out sFileName);

			var startPosition = new TEXT_POSITION[1];
			var endPosition = new TEXT_POSITION[1];
			if (0 != docPosition.GetRange(startPosition, endPosition))
			{
				return VSConstants.E_FAIL;
			}

			var sFile = sFileName;
			int iLine = ((int)startPosition[0].dwLine + 1);
			UInt32 uRawBreakpoint = m_Engine.ResolveRawBreakpoint(sFile, iLine);
			m_Engine.OnBreakpointStateChange(uRawBreakpoint, true);

			var docContext = new DocumentContext(sFile, startPosition[0], endPosition[0], new MemoryAddress(m_Engine));

			var breakpointResolution = new BreakpointResolution(m_Engine, uRawBreakpoint, docContext);
			var boundBreakpoint = new BoundBreakpoint(m_Engine, uRawBreakpoint, this, breakpointResolution);

			// Disable the bound breakpoint prior to commiting.
			if (!m_bEnabled)
			{
				((IDebugBoundBreakpoint2)boundBreakpoint).Enable(0);
			}

			// Add to our cache.
			lock (m_BoundBreakpoints)
			{
				m_BoundBreakpoints.Add(boundBreakpoint);
			}

			m_Manager.StoreBoundBreakpoint(boundBreakpoint);

			return VSConstants.S_OK;
		}

		int IDebugPendingBreakpoint2.CanBind(out IEnumDebugErrorBreakpoints2 ppErrorEnum)
		{
			ppErrorEnum = null;

			if (!CanBind)
			{
				ppErrorEnum = null;
				return VSConstants.S_FALSE;
			}

			return VSConstants.S_OK;
		}

		int IDebugPendingBreakpoint2.Delete()
		{
			lock (m_BoundBreakpoints)
			{
				for (var i = m_BoundBreakpoints.Count - 1; i >= 0; i--)
				{
					var p = ((IDebugBoundBreakpoint2)m_BoundBreakpoints[i]);
					p.Delete();
				}
			}

			return VSConstants.S_OK;
		}

		int IDebugPendingBreakpoint2.Enable(int iEnable)
		{
			lock (m_BoundBreakpoints)
			{
				m_bEnabled = (iEnable != 0);
				foreach (var bp in m_BoundBreakpoints)
				{
					var p = ((IDebugBoundBreakpoint2)bp);
					p.Enable(iEnable);
				}
			}

			return VSConstants.S_OK;
		}

		int IDebugPendingBreakpoint2.EnumBoundBreakpoints(out IEnumDebugBoundBreakpoints2 ppEnum)
		{
			lock (m_BoundBreakpoints)
			{
				var aBoundBreakpoints = m_BoundBreakpoints.ToArray();
				ppEnum = new BoundBreakpointsEnum(aBoundBreakpoints);
			}
			return VSConstants.S_OK;
		}

		int IDebugPendingBreakpoint2.EnumErrorBreakpoints(enum_BP_ERROR_TYPE bpErrorType, out IEnumDebugErrorBreakpoints2 ppEnum)
		{
			ppEnum = null;
			return VSConstants.E_NOTIMPL;
		}
		int IDebugPendingBreakpoint2.GetBreakpointRequest(out IDebugBreakpointRequest2 ppBPRequest)
		{
			ppBPRequest = m_pRequest;
			return VSConstants.S_OK;
		}
		int IDebugPendingBreakpoint2.GetState(PENDING_BP_STATE_INFO[] pState)
		{
			if (m_bDeleted) { pState[0].state = (enum_PENDING_BP_STATE)enum_BP_STATE.BPS_DELETED; }
			else if (m_bEnabled) { pState[0].state = (enum_PENDING_BP_STATE)enum_BP_STATE.BPS_ENABLED; }
			else if (!m_bEnabled) { pState[0].state = (enum_PENDING_BP_STATE)enum_BP_STATE.BPS_DISABLED; }

			return VSConstants.S_OK;
		}

		public int SetCondition(BP_CONDITION bpCondition)
		{
			return VSConstants.E_NOTIMPL;
		}

		public int SetPassCount(BP_PASSCOUNT bpPassCount)
		{
			return VSConstants.E_NOTIMPL;
		}

		public int Virtualize(int fVirtualize)
		{
			return VSConstants.S_OK;
		}
		#endregion

		public PendingBreakpoint(
			Engine engine,
			BreakpointManager manager,
			IDebugBreakpointRequest2 pRequest)
		{
			m_Engine = engine;
			m_Manager = manager;
			m_pRequest = pRequest;

			// Retrieve request info.
			BP_REQUEST_INFO[] requestInfo = new BP_REQUEST_INFO[1];
			if (VSConstants.S_OK == m_pRequest.GetRequestInfo(enum_BPREQI_FIELDS.BPREQI_BPLOCATION, requestInfo))
			{
				m_RequestInfo = requestInfo[0];
			}
		}

		public void ClearBoundBreakpoints()
		{
			lock (m_BoundBreakpoints)
			{
				for (var i = m_BoundBreakpoints.Count - 1; i >= 0; i--)
				{
					var p = ((IDebugBoundBreakpoint2)m_BoundBreakpoints[i]);
					p.Delete();
				}
			}
		}

		public void OnBoundBreakpointDeleted(BoundBreakpoint boundBreakpoint)
		{
			lock (m_BoundBreakpoints)
			{
				m_BoundBreakpoints.Remove(boundBreakpoint);
			}
		}
	}
}
