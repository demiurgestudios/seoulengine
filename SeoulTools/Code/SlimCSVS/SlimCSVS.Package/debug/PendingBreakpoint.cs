// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using Microsoft.VisualStudio;
using Microsoft.VisualStudio.Debugger.Interop;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;

namespace SlimCSVS.Package.debug
{
	sealed class PendingBreakpoint : IDebugPendingBreakpoint2
	{
		#region Private members
		readonly Engine m_Engine;
		readonly BreakpointManager m_Manager;
		readonly IDebugBreakpointRequest2 m_pRequest;
		readonly BP_REQUEST_INFO m_RequestInfo;
		readonly List<BoundBreakpoint> m_BoundBreakpoints = new List<BoundBreakpoint>();
		#endregion

		#region Overrides
		public int Bind()
		{
			var docPosition = (IDebugDocumentPosition2)(Marshal.GetObjectForIUnknown(m_RequestInfo.bpLocation.unionmember2));

			string sFileName;
			docPosition.GetFileName(out sFileName);

			var startPosition = new TEXT_POSITION[1];
			var endPosition = new TEXT_POSITION[1];
			if (0 != docPosition.GetRange(startPosition, endPosition))
			{
				return VSConstants.E_FAIL;
			}

			int iLine = ((int)startPosition[0].dwLine + 1);

			var aBP = new commands.Breakpoint[] { new commands.Breakpoint() { m_sFileName = sFileName, m_iLine = (int)startPosition[0].dwLine + 1 } };
			var cmd = new commands.Breakpoints(aBP);
			m_Engine.SendCommandToAll(cmd);

			var docContext = new DocumentContext(sFileName, startPosition[0], endPosition[0]);

			var breakpointResolution = new BreakpointResolution(m_Engine, docContext);
			var boundBreakpoint = new BoundBreakpoint(m_Engine, sFileName, iLine, this, breakpointResolution);

			string sFileAndLine = Path.GetFileName(sFileName) + iLine.ToString();
			m_Manager.StoreBoundBreakpoint(sFileAndLine, boundBreakpoint);

			return VSConstants.S_OK;
		}

		public int CanBind(out IEnumDebugErrorBreakpoints2 ppErrorEnum) { ppErrorEnum = null; return VSConstants.S_OK; }
		public int Delete() { return VSConstants.S_OK; }
		public int Enable(int fEnable) { return VSConstants.S_OK; }
		public int EnumBoundBreakpoints(out IEnumDebugBoundBreakpoints2 ppEnum) { ppEnum = null; return VSConstants.E_NOTIMPL; }
		public int EnumErrorBreakpoints(enum_BP_ERROR_TYPE bpErrorType, out IEnumDebugErrorBreakpoints2 ppEnum) { ppEnum = null; return VSConstants.E_NOTIMPL; }
		public int GetBreakpointRequest(out IDebugBreakpointRequest2 ppBPRequest) { ppBPRequest = null; return VSConstants.E_NOTIMPL; }
		public int GetState(PENDING_BP_STATE_INFO[] pState) { return VSConstants.E_NOTIMPL; }
		public int SetCondition(BP_CONDITION bpCondition) { return VSConstants.E_NOTIMPL; }
		public int SetPassCount(BP_PASSCOUNT bpPassCount) { return VSConstants.E_NOTIMPL; }
		public int Virtualize(int fVirtualize) { return VSConstants.S_OK; }
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
	}
}
