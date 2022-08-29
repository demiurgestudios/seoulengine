// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using Microsoft.VisualStudio;
using Microsoft.VisualStudio.Debugger.Interop;
using System;

namespace SlimCSVS.Package.util
{
	public sealed class ProgramProvider : IDebugProgramProvider2
	{
		#region Private members
		int m_iLocale;
		#endregion

		public int Locale { get { return m_iLocale; } }

		public int GetProviderProcessData(
			enum_PROVIDER_FLAGS flags,
			IDebugDefaultPort2 port,
			AD_PROCESS_ID processId,
			CONST_GUID_ARRAY engineFilter,
			PROVIDER_PROCESS_DATA[] process)
		{
			return VSConstants.S_FALSE;
		}

		public int GetProviderProgramNode(
			enum_PROVIDER_FLAGS flags,
			IDebugDefaultPort2 port,
			AD_PROCESS_ID processId,
			ref Guid guidEngine,
			ulong uProgramId,
			out IDebugProgramNode2 programNode)
		{
			programNode = null;
			return VSConstants.E_NOTIMPL;
		}

		public int WatchForProviderEvents(
			enum_PROVIDER_FLAGS flags,
			IDebugDefaultPort2 port,
			AD_PROCESS_ID processId,
			CONST_GUID_ARRAY engineFilter,
			ref Guid guidLaunchingEngine,
			IDebugPortNotify2 eventCallback)
		{
			return VSConstants.S_OK;
		}

		public int SetLocale(ushort iLocale)
		{
			m_iLocale = iLocale;
			return VSConstants.S_OK;
		}
	}
}
