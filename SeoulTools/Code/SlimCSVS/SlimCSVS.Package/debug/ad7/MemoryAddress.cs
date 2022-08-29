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
	sealed class MemoryAddress : IDebugCodeContext2, IDebugCodeContext100
	{
		#region Private members
		readonly Engine m_Engine;
		IDebugDocumentContext2 m_DocumentContext;
		#endregion

		#region Overrides
		#region IDebugCodeContext2
		public int Add(ulong dwCount, out IDebugMemoryContext2 ppMemCxt)
		{
			ppMemCxt = new MemoryAddress(m_Engine);
			return VSConstants.S_OK;
		}
		public int Compare(enum_CONTEXT_COMPARE uContextCompare, IDebugMemoryContext2[] aCompareTo, uint uCompareToLength, out uint pdwMemoryContext)
		{
			pdwMemoryContext = uint.MaxValue;

			var eContextCompare = (enum_CONTEXT_COMPARE)uContextCompare;

			for (uint u = 0; u < uCompareToLength; u++)
			{
				var compareTo = aCompareTo[u] as MemoryAddress;
				if (compareTo == null)
				{
					continue;
				}

				if (!Engine.ReferenceEquals(this.m_Engine, compareTo.m_Engine))
				{
					continue;
				}

				var bResult = false;
				switch (eContextCompare)
				{
					case enum_CONTEXT_COMPARE.CONTEXT_LESS_THAN:
					case enum_CONTEXT_COMPARE.CONTEXT_GREATER_THAN:
						bResult = false;
						break;

					case enum_CONTEXT_COMPARE.CONTEXT_EQUAL:
					case enum_CONTEXT_COMPARE.CONTEXT_LESS_THAN_OR_EQUAL:
					case enum_CONTEXT_COMPARE.CONTEXT_GREATER_THAN_OR_EQUAL:
					case enum_CONTEXT_COMPARE.CONTEXT_SAME_SCOPE:
					case enum_CONTEXT_COMPARE.CONTEXT_SAME_FUNCTION:
					case enum_CONTEXT_COMPARE.CONTEXT_SAME_MODULE:
					case enum_CONTEXT_COMPARE.CONTEXT_SAME_PROCESS:
						bResult = true;
						break;

					default:
						return VSConstants.E_NOTIMPL;
				}

				if (bResult)
				{
					pdwMemoryContext = u;
					return VSConstants.S_OK;
				}
			}

			return VSConstants.S_FALSE;
		}

		public int GetDocumentContext(out IDebugDocumentContext2 ppSrcCxt)
		{
			ppSrcCxt = m_DocumentContext;
			return VSConstants.S_OK;
		}

		public int GetInfo(enum_CONTEXT_INFO_FIELDS dwFields, CONTEXT_INFO[] pinfo)
		{
			try
			{
				pinfo[0].dwFields = 0;

				if ((dwFields & enum_CONTEXT_INFO_FIELDS.CIF_ADDRESS) != 0)
				{
					pinfo[0].bstrAddress = "0";
					pinfo[0].dwFields |= enum_CONTEXT_INFO_FIELDS.CIF_ADDRESS;
				}

				// Fields not supported by the sample
				if ((dwFields & enum_CONTEXT_INFO_FIELDS.CIF_ADDRESSOFFSET) != 0){}
				if ((dwFields & enum_CONTEXT_INFO_FIELDS.CIF_ADDRESSABSOLUTE) != 0){}
				if ((dwFields & enum_CONTEXT_INFO_FIELDS.CIF_MODULEURL) != 0){}
				if ((dwFields & enum_CONTEXT_INFO_FIELDS.CIF_FUNCTION) != 0) {}
				if ((dwFields & enum_CONTEXT_INFO_FIELDS.CIF_FUNCTIONOFFSET) != 0) {}

				return VSConstants.S_OK;
			}
			catch (Exception)
			{
				return VSConstants.RPC_E_SERVERFAULT;
			}
		}

		public int GetName(out string pbstrName)
		{
			pbstrName = null;
			return VSConstants.E_NOTIMPL;
		}

		public int Subtract(ulong dwCount, out IDebugMemoryContext2 ppMemCxt)
		{
			ppMemCxt = new MemoryAddress(m_Engine);
			return VSConstants.S_OK;
		}

		public int GetLanguageInfo(ref string pbstrLanguage, ref Guid pguidLanguage)
		{
			pbstrLanguage = Constants.LanguageNames.CSharp;
			pguidLanguage = Constants.LanguageGuids.CSharp;
			return VSConstants.S_OK;
		}
		#endregion

		#region IDebugCodeContext100
		public int GetProgram(out IDebugProgram2 ppProgram)
		{
			ppProgram = m_Engine;
			return VSConstants.S_OK;
		}
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
