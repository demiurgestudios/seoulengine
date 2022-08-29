// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using Microsoft.VisualStudio;
using Microsoft.VisualStudio.Debugger.Interop;
using System.Runtime.InteropServices;

namespace SlimCSVS.Package.debug
{
	sealed class BreakpointResolution : IDebugBreakpointResolution2
	{
		#region Private members
		readonly Engine m_Engine;
		readonly DocumentContext m_DocumentContext;
		#endregion

		#region Overrides
		int IDebugBreakpointResolution2.GetBreakpointType(enum_BP_TYPE[] pBPType) { pBPType[0] = enum_BP_TYPE.BPT_CODE; return VSConstants.S_OK; }

		int IDebugBreakpointResolution2.GetResolutionInfo(
			enum_BPRESI_FIELDS dwFields,
			BP_RESOLUTION_INFO[] pBPResolutionInfo)
		{
			if ((dwFields & enum_BPRESI_FIELDS.BPRESI_BPRESLOCATION) != 0)
			{
				var location = new BP_RESOLUTION_LOCATION();
				location.bpType = (uint)enum_BP_TYPE.BPT_CODE;

				var codeContext = new MemoryAddress(m_Engine);
				codeContext.SetDocumentContext(m_DocumentContext);
				location.unionmember1 = Marshal.GetComInterfaceForObject(codeContext, typeof(IDebugCodeContext2));

				pBPResolutionInfo[0].bpResLocation = location;
				pBPResolutionInfo[0].dwFields |= enum_BPRESI_FIELDS.BPRESI_BPRESLOCATION;
			}

			if ((dwFields & enum_BPRESI_FIELDS.BPRESI_PROGRAM) != 0)
			{
				pBPResolutionInfo[0].pProgram = (IDebugProgram2)m_Engine;
				pBPResolutionInfo[0].dwFields |= enum_BPRESI_FIELDS.BPRESI_PROGRAM;
			}

			return VSConstants.S_OK;
		}
		#endregion

		public BreakpointResolution(Engine engine, DocumentContext documentContext)
		{
			m_Engine = engine;
			m_DocumentContext = documentContext;
		}
	}

	sealed class ErrorBreakpointResolution : IDebugErrorBreakpointResolution2
	{
		#region Private members
		string m_sMessage;
		enum_BP_ERROR_TYPE m_eErrorType;
		#endregion

		#region Overrides
		int IDebugErrorBreakpointResolution2.GetBreakpointType(enum_BP_TYPE[] pBPType) { pBPType[0] = enum_BP_TYPE.BPT_CODE; return VSConstants.S_OK; }

		int IDebugErrorBreakpointResolution2.GetResolutionInfo(
			enum_BPERESI_FIELDS dwFields,
			BP_ERROR_RESOLUTION_INFO[] info)
		{
			if ((dwFields & enum_BPERESI_FIELDS.BPERESI_MESSAGE) != 0)
			{
				info[0].dwFields |= enum_BPERESI_FIELDS.BPERESI_MESSAGE;
				info[0].bstrMessage = m_sMessage;
			}

			if ((dwFields & enum_BPERESI_FIELDS.BPERESI_TYPE) != 0)
			{
				info[0].dwFields |= enum_BPERESI_FIELDS.BPERESI_TYPE;
				info[0].dwType = m_eErrorType;
			}

			return VSConstants.S_OK;
		}
		#endregion

		public ErrorBreakpointResolution(
			string sMessage,
			enum_BP_ERROR_TYPE eErrorType = enum_BP_ERROR_TYPE.BPET_GENERAL_WARNING)
		{
			m_sMessage = sMessage;
			m_eErrorType = eErrorType;
		}
	}
}
