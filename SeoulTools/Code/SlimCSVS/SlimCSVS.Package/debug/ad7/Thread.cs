// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using Microsoft.VisualStudio;
using Microsoft.VisualStudio.Debugger.Interop;
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;

namespace SlimCSVS.Package.debug.ad7
{
	sealed class StackFrame : IDebugStackFrame2, IDebugExpressionContext2
	{
		#region Private members
		readonly UInt32 m_uStackDepth;
		readonly Engine m_Engine;
		readonly Thread m_Thread;
		readonly debug.StackFrame m_Frame;
		readonly List<Variable> m_Locals;

		static readonly Guid kAllLocals = new Guid("196db21f-5f22-45a9-b5a3-32cddb30db06");
		static readonly Guid kAllLocalsPlusArgs = new Guid("939729a8-4cb0-4647-9831-7ff465240d5f");
		static readonly Guid kArgs = new Guid("804bccea-0475-4ae7-8a46-1862688ab863");
		static readonly Guid kFilterLocals = new Guid("b200f725-e725-4c53-b36a-1ec27aef12ef");
		static readonly Guid kLocalsPlusArgs = new Guid("e74721bb-10c0-40f5-807f-920d37f95419");

		void GatherLocals(
			enum_DEBUGPROP_INFO_FLAGS dwFields,
			out uint puOut,
			out IEnumDebugPropertyInfo2 ppEnumObject)
		{
			var a = Gather(
				dwFields,
				null,
				m_Engine,
				m_Thread,
				this,
				string.Empty,
				m_Locals);

			puOut = (uint)a.Length;
			ppEnumObject = new PropertyInfoEnum(a);
		}
		#endregion

		#region IDebugStackFrame2
		int IDebugStackFrame2.EnumProperties(
			enum_DEBUGPROP_INFO_FLAGS dwFields,
			uint nRadix,
			ref Guid guidFilter,
			uint dwTimeout,
			out uint pcelt,
			out IEnumDebugPropertyInfo2 ppEnum)
		{
			pcelt = 0;
			ppEnum = null;

			if (guidFilter == kLocalsPlusArgs ||
				guidFilter == kAllLocalsPlusArgs ||
				guidFilter == kAllLocals)
			{
				GatherLocals(dwFields, out pcelt, out ppEnum);
				return VSConstants.S_OK;
			}
			else
			{
				return VSConstants.E_NOTIMPL;
			}
		}

		int IDebugStackFrame2.GetCodeContext(out IDebugCodeContext2 ppCodeCxt)
		{
			ppCodeCxt = null;
			return VSConstants.E_NOTIMPL;
		}

		int IDebugStackFrame2.GetDebugProperty(out IDebugProperty2 ppProperty)
		{
			ppProperty = new StackFrameProperty(this);
			return VSConstants.S_OK;
		}

		int IDebugStackFrame2.GetDocumentContext(out IDebugDocumentContext2 ppCxt)
		{
			var sFileName = string.Empty;
			int iLine = 1;
			m_Engine.ResolveFileAndLine(m_Frame.m_uRawBreakpoint, out sFileName, out iLine);

			var begin = new TEXT_POSITION
			{
				dwColumn = 0,
				dwLine = (uint)(iLine - 1),
			};
			var end = new TEXT_POSITION
			{
				dwColumn = 0,
				dwLine = (uint)(iLine - 1),
			};

			ppCxt = new DocumentContext(sFileName, begin, end, new MemoryAddress(m_Engine));
			return VSConstants.S_OK;
		}

		int IDebugStackFrame2.GetExpressionContext(out IDebugExpressionContext2 ppExprCxt)
		{
			ppExprCxt = this;
			return VSConstants.S_OK;
		}

		int IDebugStackFrame2.GetInfo(
			enum_FRAMEINFO_FLAGS dwFieldSpec,
			uint nRadix,
			FRAMEINFO[] pFrameInfo)
		{
			pFrameInfo[0] = ToFrameInfo(dwFieldSpec);
			return VSConstants.S_OK;
		}

		int IDebugStackFrame2.GetLanguageInfo(ref string pbstrLanguage, ref Guid pguidLanguage)
		{
			pbstrLanguage = Constants.LanguageNames.CSharp;
			pguidLanguage = Constants.LanguageGuids.CSharp;
			return VSConstants.S_OK;
		}

		int IDebugStackFrame2.GetName(out string pbstrName)
		{
			pbstrName = m_Frame.m_sFunc;
			return VSConstants.S_OK;
		}

		int IDebugStackFrame2.GetPhysicalStackRange(out ulong paddrMin, out ulong paddrMax)
		{
			paddrMin = 0;
			paddrMax = 0;
			return VSConstants.S_OK;
		}

		int IDebugStackFrame2.GetThread(out IDebugThread2 ppThread)
		{
			ppThread = m_Thread;
			return VSConstants.S_OK;
		}
		#endregion

		#region IDebugExpressionContext2
		int IDebugExpressionContext2.GetName(out string pbstrName)
		{
			pbstrName = null;
			return VSConstants.E_NOTIMPL;
		}

		int IDebugExpressionContext2.ParseText(
			string pszCode,
			enum_PARSEFLAGS dwFlags,
			uint nRadix,
			out IDebugExpression2 ppExpr,
			out string pbstrError,
			out uint pichError)
		{
			ppExpr = null;
			pbstrError = null;
			pichError = 0;

			(var sPath, var b) = ResolveExpressionPath(pszCode);
			if (!b)
			{
				return VSConstants.S_FALSE;
			}

			ppExpr = new Expression(m_Engine, m_Thread, this, sPath);
			pbstrError = null;
			pichError = 0;

			return VSConstants.S_OK;
		}

		(string, bool) ResolveExpressionPath(string sCode)
		{
			int i = sCode.IndexOf('.');

			string sRoot = null;
			if (i < 0)
			{
				sRoot = sCode;
			}
			else
			{
				sRoot = sCode.Substring(0, i);
			}

			bool bThis = false;
			foreach (var v in m_Locals)
			{
				if (v.m_sName == "this") { bThis = true; }
				if (v.m_sName == sRoot)
				{
					return (sCode, true);
				}
			}

			if (bThis)
			{
				return ("this." + sCode, true);
			}
			else
			{
				return (string.Empty, false);
			}
		}
		#endregion

		public StackFrame(
			UInt32 uStackDepth,
			Engine engine,
			Thread thread,
			debug.StackFrame frame,
			List<Variable> locals)
		{
			m_uStackDepth = uStackDepth;
			m_Engine = engine;
			m_Thread = thread;
			m_Frame = frame;
			m_Locals = locals;
		}

		public static DEBUG_PROPERTY_INFO[] Gather(
			enum_DEBUGPROP_INFO_FLAGS dwFields,
			Property parent,
			Engine engine,
			Thread thread,
			StackFrame frame,
			string sPath,
			List<Variable> vars)
		{
			var sPrefix = string.IsNullOrEmpty(sPath) ? string.Empty : (sPath + ".");
			var iCount = (null != vars ? vars.Count : 0);

			var a = new DEBUG_PROPERTY_INFO[iCount];
			for (int i = 0; i < iCount; ++i)
			{
				var v = vars[i];
				a[i] = new Property(parent, engine, thread, frame, sPrefix + v.m_sName, vars[i].m_sName, vars[i]).ToDebugPropertyInfo(dwFields);
			}

			return a;
		}

		public Engine Engine
		{
			get
			{
				return m_Engine;
			}
		}

		public List<Variable> Locals
		{
			get
			{
				return m_Locals;
			}
		}

		public UInt32 StackDepth
		{
			get
			{
				return m_uStackDepth;
			}
		}

		public Thread Thread
		{
			get
			{
				return m_Thread;
			}
		}

		public FRAMEINFO ToFrameInfo(enum_FRAMEINFO_FLAGS dwFieldSpec)
		{
			var frameInfo = new FRAMEINFO();

			if ((dwFieldSpec & enum_FRAMEINFO_FLAGS.FIF_FUNCNAME) != 0)
			{
				frameInfo.m_bstrFuncName = m_Frame.ToDisplayName(m_Engine, dwFieldSpec);
				frameInfo.m_dwValidFields |= enum_FRAMEINFO_FLAGS.FIF_FUNCNAME;
			}

			if ((dwFieldSpec & enum_FRAMEINFO_FLAGS.FIF_LANGUAGE) != 0)
			{
				frameInfo.m_bstrLanguage = Constants.LanguageNames.CSharp;
				frameInfo.m_dwValidFields |= enum_FRAMEINFO_FLAGS.FIF_LANGUAGE;
			}

			if ((dwFieldSpec & enum_FRAMEINFO_FLAGS.FIF_FRAME) != 0)
			{
				frameInfo.m_pFrame = this;
				frameInfo.m_dwValidFields |= enum_FRAMEINFO_FLAGS.FIF_FRAME;
			}

			if ((dwFieldSpec & enum_FRAMEINFO_FLAGS.FIF_DEBUGINFO) != 0)
			{
				frameInfo.m_fHasDebugInfo = 1;
				frameInfo.m_dwValidFields |= enum_FRAMEINFO_FLAGS.FIF_DEBUGINFO;
			}

			if ((dwFieldSpec & enum_FRAMEINFO_FLAGS.FIF_STALECODE) != 0)
			{
				frameInfo.m_fStaleCode = 0;
				frameInfo.m_dwValidFields |= enum_FRAMEINFO_FLAGS.FIF_STALECODE;
			}

			return frameInfo;
		}
	}

	sealed class Thread : IDebugThread2, IDebugThread100
	{
		public const string ksThreadName = "SlimCS Main Thread";
		public const uint kuThreadId = 0xd1a19eae;

		#region Private members
		readonly Engine m_Engine;
		readonly ConcurrentDictionary<(UInt32, string), List<Variable>> m_Children = new ConcurrentDictionary<(UInt32, string), List<Variable>>();
		readonly ConcurrentDictionary<UInt32, List<Variable>> m_Locals = new ConcurrentDictionary<uint, List<Variable>>();
		volatile events.HitBreakpoint m_HitBreakpoint;
		#endregion

		#region IDebugThread2
		int IDebugThread2.CanSetNextStatement(IDebugStackFrame2 stackFrame, IDebugCodeContext2 codeContext) { return VSConstants.S_FALSE; }
		int IDebugThread2.EnumFrameInfo(
			enum_FRAMEINFO_FLAGS dwFieldSpec,
			uint nRadix,
			out IEnumDebugFrameInfo2 enumObject)
		{
			enumObject = null;

			if (null == HitBreakpoint)
			{
				return VSConstants.E_FAIL;
			}

			var bp = HitBreakpoint;
			var aData = new FRAMEINFO[bp.Frames.Count];
			for (int i = 0; i < aData.Length; ++i)
			{
				List<Variable> vars = null;
				if (!m_Locals.TryGetValue((UInt32)i, out vars))
				{
					if (!m_Engine.RequestFrameInfo((UInt32)i, Constants.kQueryTimeout))
					{
						return VSConstants.E_FAIL;
					}

					m_Locals.TryGetValue((UInt32)i, out vars);
				}

				if (null == vars)
				{
					vars = new List<Variable>();
				}

				aData[i] = new StackFrame((UInt32)i, m_Engine, this, bp.Frames[i], vars).ToFrameInfo(dwFieldSpec);
			}

			enumObject = new FrameInfoEnum(aData);
			return VSConstants.S_OK;
		}

		int IDebugThread2.GetLogicalThread(
			IDebugStackFrame2 stackFrame,
			out IDebugLogicalThread2 logicalThread)
		{
			logicalThread = null;
			return VSConstants.E_NOTIMPL;
		}

		int IDebugThread2.GetName(out string rsThreadName)
		{
			rsThreadName = ksThreadName;
			return VSConstants.S_OK;
		}

		int IDebugThread2.GetProgram(out IDebugProgram2 rpProgram)
		{
			rpProgram = m_Engine;
			return VSConstants.S_OK;
		}

		int IDebugThread2.GetThreadId(out uint ruThreadId)
		{
			ruThreadId = kuThreadId;
			return VSConstants.S_OK;
		}

		int IDebugThread2.GetThreadProperties(
			enum_THREADPROPERTY_FIELDS dwFields,
			THREADPROPERTIES[] propertiesArray)
		{
			try
			{
				THREADPROPERTIES props = new THREADPROPERTIES();

				if ((dwFields & enum_THREADPROPERTY_FIELDS.TPF_ID) != 0)
				{
					props.dwThreadId = kuThreadId;
					props.dwFields |= enum_THREADPROPERTY_FIELDS.TPF_ID;
				}
				if ((dwFields & enum_THREADPROPERTY_FIELDS.TPF_SUSPENDCOUNT) != 0)
				{
					props.dwFields |= enum_THREADPROPERTY_FIELDS.TPF_SUSPENDCOUNT;
				}
				if ((dwFields & enum_THREADPROPERTY_FIELDS.TPF_STATE) != 0)
				{
					props.dwThreadState = (uint)enum_THREADSTATE.THREADSTATE_RUNNING;
					props.dwFields |= enum_THREADPROPERTY_FIELDS.TPF_STATE;
				}
				if ((dwFields & enum_THREADPROPERTY_FIELDS.TPF_PRIORITY) != 0)
				{
					props.bstrPriority = "Normal";
					props.dwFields |= enum_THREADPROPERTY_FIELDS.TPF_PRIORITY;
				}
				if ((dwFields & enum_THREADPROPERTY_FIELDS.TPF_NAME) != 0)
				{
					props.bstrName = ksThreadName;
					props.dwFields |= enum_THREADPROPERTY_FIELDS.TPF_NAME;
				}
				if ((dwFields & enum_THREADPROPERTY_FIELDS.TPF_LOCATION) != 0)
				{
					props.bstrLocation = "<placeholder_location>";
					props.dwFields |= enum_THREADPROPERTY_FIELDS.TPF_LOCATION;
				}

				return VSConstants.S_OK;
			}
			catch (Exception)
			{
				return VSConstants.RPC_E_SERVERFAULT;
			}
		}

		int IDebugThread2.Resume(out uint suspendCount)
		{
			suspendCount = 0;
			return VSConstants.E_NOTIMPL;
		}

		int IDebugThread2.SetNextStatement(IDebugStackFrame2 stackFrame, IDebugCodeContext2 codeContext)
		{
			return VSConstants.E_NOTIMPL;
		}

		int IDebugThread2.SetThreadName(string name)
		{
			return VSConstants.E_NOTIMPL;
		}

		int IDebugThread2.Suspend(out uint suspendCount)
		{
			suspendCount = 0;
			return VSConstants.E_NOTIMPL;
		}
		#endregion

		public Thread(Engine engine)
		{
			m_Engine = engine;
		}

		public events.HitBreakpoint HitBreakpoint
		{
			get
			{
				return m_HitBreakpoint;
			}

			set
			{
				m_HitBreakpoint = value;
			}
		}

		public void OnResume()
		{
			m_Children.Clear();
			m_Locals.Clear();
		}

		public void SetFrameData(UInt32 uDepth, List<Variable> vars)
		{
			m_Locals[uDepth] = vars;
		}

		public List<Variable> GetVariableChildren(UInt32 uStackDepth, string sPath)
		{
			List<Variable> ret = null;
			m_Children.TryGetValue((uStackDepth, sPath), out ret);
			return ret;
		}

		/// <summary>
		/// Lookup a variable value cached in the thread variable table.
		/// Can return false if never requested.
		/// </summary>
		/// <param name="sPath">Path to query.</param>
		/// <returns>The variable value and true, or a default variable and false on failure.</returns>
		public (Variable v, bool b) GetVariableValue(StackFrame frame, string sPath)
		{
			int i = sPath.LastIndexOf('.');
			if (i >= 0)
			{
				// Get the children, then search those for the last part.
				var sParent = sPath.Substring(0, i);
				var sChild = sPath.Substring(i + 1);

				var vars = GetVariableChildren(frame.StackDepth, sParent);
				if (null == vars)
				{
					return (new Variable(), false);
				}

				foreach (var v in vars)
				{
					if (v.m_sName == sChild)
					{
						return (v, true);
					}
				}

				return (new Variable(), false);
			}
			// Must be a local variable.
			else
			{
				var locals = frame.Locals;
				if (null == locals)
				{
					return (new Variable(), false);
				}

				foreach (var local in locals)
				{
					if (local.m_sName == sPath)
					{
						return (local, true);
					}
				}

				return (new Variable(), false);
			}
		}

		public void SetVariableChildren(UInt32 uStackDepth, string sPath, List<Variable> vars)
		{
			var lookup = (uStackDepth, sPath);
			m_Children[lookup] = vars;
		}

		public bool SetVariableValue(StackFrame frame, string sPath, string sValue)
		{
			int i = sPath.LastIndexOf('.');
			if (i >= 0)
			{
				// Get the children, then search those for the last part.
				var sParent = sPath.Substring(0, i);
				var sChild = sPath.Substring(i + 1);

				var vars = GetVariableChildren(frame.StackDepth, sParent);
				if (null == vars)
				{
					return false;
				}

				for (int iVar = 0; iVar < vars.Count; ++iVar)
				{
					var v = vars[iVar];
					if (v.m_sName == sChild)
					{
						v.m_sValue = sValue;
						vars[iVar] = v;
						return true;
					}
				}

				return false;
			}
			// Must be a local variable.
			else
			{
				var locals = frame.Locals;
				if (null == locals)
				{
					return false;
				}

				for (int iVar = 0; iVar < locals.Count; ++iVar)
				{
					var local = locals[iVar];
					if (local.m_sName == sPath)
					{
						local.m_sValue = sValue;
						locals[iVar] = local;
						return true;
					}
				}

				return false;
			}
		}

		#region IDebugThread100 Members
		int IDebugThread100.CanDoFuncEval()
		{
			return VSConstants.S_FALSE;
		}

		int IDebugThread100.GetFlags(out uint flags)
		{
			flags = 0;
			return VSConstants.E_NOTIMPL;
		}

		int IDebugThread100.GetThreadDisplayName(out string name)
		{
			name = ksThreadName;
			return VSConstants.S_OK;
		}

		int IDebugThread100.GetThreadProperties100(uint dwFields, THREADPROPERTIES100[] props)
		{
			int hRes = VSConstants.S_OK;

			THREADPROPERTIES[] props90 = new THREADPROPERTIES[1];
			enum_THREADPROPERTY_FIELDS dwFields90 = (enum_THREADPROPERTY_FIELDS)(dwFields & 0x3f);
			hRes = ((IDebugThread2)this).GetThreadProperties(dwFields90, props90);
			props[0].bstrLocation = props90[0].bstrLocation;
			props[0].bstrName = props90[0].bstrName;
			props[0].bstrPriority = props90[0].bstrPriority;
			props[0].dwFields = (uint)props90[0].dwFields;
			props[0].dwSuspendCount = props90[0].dwSuspendCount;
			props[0].dwThreadId = props90[0].dwThreadId;
			props[0].dwThreadState = props90[0].dwThreadState;

			if (hRes == VSConstants.S_OK && dwFields != (uint)dwFields90)
			{
				if ((dwFields & (uint)enum_THREADPROPERTY_FIELDS100.TPF100_DISPLAY_NAME) != 0)
				{
					props[0].bstrDisplayName = ksThreadName;
					props[0].dwFields |= (uint)enum_THREADPROPERTY_FIELDS100.TPF100_DISPLAY_NAME;
					props[0].DisplayNamePriority = 10;
					props[0].dwFields |= (uint)enum_THREADPROPERTY_FIELDS100.TPF100_DISPLAY_NAME_PRIORITY;
				}

				if ((dwFields & (uint)enum_THREADPROPERTY_FIELDS100.TPF100_CATEGORY) != 0)
				{
					props[0].dwThreadCategory = 0;
					props[0].dwFields |= (uint)enum_THREADPROPERTY_FIELDS100.TPF100_CATEGORY;
				}

				if ((dwFields & (uint)enum_THREADPROPERTY_FIELDS100.TPF100_AFFINITY) != 0)
				{
					props[0].AffinityMask = 0;
					props[0].dwFields |= (uint)enum_THREADPROPERTY_FIELDS100.TPF100_AFFINITY;
				}

				if ((dwFields & (uint)enum_THREADPROPERTY_FIELDS100.TPF100_PRIORITY_ID) != 0)
				{
					props[0].priorityId = 0;
					props[0].dwFields |= (uint)enum_THREADPROPERTY_FIELDS100.TPF100_PRIORITY_ID;
				}
			}

			return hRes;
		}

		int IDebugThread100.SetFlags(uint flags)
		{
			return VSConstants.E_NOTIMPL;
		}

		int IDebugThread100.SetThreadDisplayName(string name)
		{
			return VSConstants.E_NOTIMPL;
		}
		#endregion
	}
}
