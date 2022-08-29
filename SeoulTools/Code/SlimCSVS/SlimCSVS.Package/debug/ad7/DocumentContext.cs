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
	sealed class DocumentContext : IDebugDocumentContext2
	{
		#region Private members
		readonly string m_sFileName;
		readonly TEXT_POSITION m_BeginPosition;
		readonly TEXT_POSITION m_EndPosition;
		readonly MemoryAddress m_CodeContext;
		#endregion

		#region Overrides
		int IDebugDocumentContext2.Compare(enum_DOCCONTEXT_COMPARE Compare, IDebugDocumentContext2[] rgpDocContextSet, uint dwDocContextSetLen, out uint pdwDocContext)
		{
			dwDocContextSetLen = 0;
			pdwDocContext = 0;

			return VSConstants.E_NOTIMPL;
		}

		int IDebugDocumentContext2.EnumCodeContexts(out IEnumDebugCodeContexts2 ppEnumCodeContexts)
		{
			var aContexts = new MemoryAddress[1];
			aContexts[0] = m_CodeContext;
			ppEnumCodeContexts = new CodeContextEnum(aContexts);
			return VSConstants.S_OK;
		}

		int IDebugDocumentContext2.GetDocument(out IDebugDocument2 ppDocument)
		{
			ppDocument = null;
			return VSConstants.E_FAIL;
		}

		int IDebugDocumentContext2.GetLanguageInfo(ref string pbstrLanguage, ref Guid pguidLanguage)
		{
			pbstrLanguage = Constants.LanguageNames.CSharp;
			pguidLanguage = Constants.LanguageGuids.CSharp;
			return VSConstants.S_OK;
		}

		int IDebugDocumentContext2.GetName(enum_GETNAME_TYPE gnType, out string pbstrFileName)
		{
			pbstrFileName = m_sFileName;
			return VSConstants.S_OK;
		}

		int IDebugDocumentContext2.GetSourceRange(TEXT_POSITION[] pBegPosition, TEXT_POSITION[] pEndPosition)
		{
			return VSConstants.E_NOTIMPL;
		}

		int IDebugDocumentContext2.GetStatementRange(TEXT_POSITION[] pBegPosition, TEXT_POSITION[] pEndPosition)
		{
			pBegPosition[0].dwColumn = m_BeginPosition.dwColumn;
			pBegPosition[0].dwLine = m_BeginPosition.dwLine;

			pEndPosition[0].dwColumn = m_EndPosition.dwColumn;
			pEndPosition[0].dwLine = m_EndPosition.dwLine;

			return VSConstants.S_OK;
		}

		int IDebugDocumentContext2.Seek(int nCount, out IDebugDocumentContext2 ppDocContext)
		{
			ppDocContext = null;
			return VSConstants.E_NOTIMPL;
		}
		#endregion

		public DocumentContext(string sFileName, TEXT_POSITION begin, TEXT_POSITION end, MemoryAddress codeContext)
		{
			m_sFileName = sFileName;
			m_BeginPosition = begin;
			m_EndPosition = end;
			m_CodeContext = codeContext;
		}
	}
}
