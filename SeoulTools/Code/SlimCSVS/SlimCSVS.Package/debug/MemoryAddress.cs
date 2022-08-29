// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using Microsoft.VisualStudio;
using Microsoft.VisualStudio.Debugger.Interop;
using System;

namespace SlimCSVS.Package.debug
{
	sealed class MemoryAddress : IDebugCodeContext2, IDebugCodeContext100
	{
		#region Private members
		readonly Engine m_Engine;
		IDebugDocumentContext2 m_DocumentContext;
		#endregion

		#region Overrides
		#region IDebugCodeContext2
		public int Add(ulong dwCount, out IDebugMemoryContext2 ppMemCxt) { ppMemCxt = null; return VSConstants.E_NOTIMPL; }
		public int Compare(enum_CONTEXT_COMPARE Compare, IDebugMemoryContext2[] rgpMemoryContextSet, uint dwMemoryContextSetLen, out uint pdwMemoryContext) { pdwMemoryContext = 0; return VSConstants.E_NOTIMPL; }
		public int GetDocumentContext(out IDebugDocumentContext2 ppSrcCxt) { ppSrcCxt = m_DocumentContext; return VSConstants.S_OK; }
		public int GetInfo(enum_CONTEXT_INFO_FIELDS dwFields, CONTEXT_INFO[] pinfo) { return VSConstants.E_NOTIMPL; }
		public int GetName(out string pbstrName) { pbstrName = string.Empty; return VSConstants.E_NOTIMPL; }
		public int Subtract(ulong dwCount, out IDebugMemoryContext2 ppMemCxt) { ppMemCxt = null; return VSConstants.E_NOTIMPL; }

		public int GetLanguageInfo(ref string pbstrLanguage, ref Guid pguidLanguage)
		{
			pbstrLanguage = Constants.LanguageNames.CSharp;
			pguidLanguage = Constants.LanguageGuids.CSharp;
			return VSConstants.S_OK;
		}
		#endregion

		#region IDebugCodeContext100
		public int GetProgram(out IDebugProgram2 ppProgram) { ppProgram = null; return VSConstants.E_NOTIMPL; }
		#endregion
		#endregion

		public MemoryAddress(Engine engine)
		{
			m_Engine = engine;
		}

		public void SetDocumentContext(IDebugDocumentContext2 documentContext)
		{
			m_DocumentContext = documentContext;
		}
	}
}
