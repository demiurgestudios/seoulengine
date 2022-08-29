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
	sealed class ProgramNode : IDebugProgramNode2
	{
		#region Private members
		readonly AD_PROCESS_ID m_ProcessId;
		#endregion

		#region Overrides
		public int Attach_V7(IDebugProgram2 pMDMProgram, IDebugEventCallback2 pCallback, uint dwReason)
		{
			return VSConstants.E_NOTIMPL;
		}

		public int DetachDebugger_V7()
		{
			return VSConstants.E_NOTIMPL;
		}

		public int GetHostMachineName_V7(out string pbstrHostMachineName)
		{
			pbstrHostMachineName = null;
			return VSConstants.E_NOTIMPL;
		}

		public int GetEngineInfo(out string pbstrEngine, out Guid pguidEngine)
		{
			pbstrEngine = Constants.ksEngineName;
			pguidEngine = Constants.kEngineGuid;
			return VSConstants.S_OK;
		}

		public int GetHostName(enum_GETHOSTNAME_TYPE dwHostNameType, out string pbstrHostName)
		{
			pbstrHostName = null;
			return VSConstants.E_NOTIMPL;
		}

		public int GetHostPid(AD_PROCESS_ID[] pHostProcessId)
		{
			pHostProcessId[0] = m_ProcessId;
			return VSConstants.S_OK;
		}

		public int GetProgramName(out string pbstrProgramName)
		{
			pbstrProgramName = null;
			return VSConstants.E_NOTIMPL;
		}
		#endregion

		public ProgramNode(AD_PROCESS_ID processId)
		{
			m_ProcessId = processId;
		}
	}
}
