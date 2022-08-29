// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading;
using Microsoft.VisualStudio;
using Microsoft.VisualStudio.Debugger.Interop;

namespace SlimCSVS.Package.debug
{
	[Guid(Constants.ksEngineGuid)]
	public class Engine : IDebugEngine2, IDebugEngineLaunch2, IDebugProgram3
	{
		/// <summary>
		/// Fixed port used by the debugger connection.
		/// </summary>
		public const int kiPort = 25762;

		#region Private members
		class EngineDummyProcess : IDebugProcess3
		{
			#region Private members
			readonly IDebugPort2 m_pPort;
			readonly Guid m_ProcessGuid = Guid.NewGuid();
			#endregion

			public EngineDummyProcess(IDebugPort2 pPort)
			{
				m_pPort = pPort;
			}

			public int Attach(IDebugEventCallback2 pCallback, Guid[] rgguidSpecificEngines, uint celtSpecificEngines, int[] rghrEngineAttach) { return VSConstants.S_OK; }
			public int CanDetach() { return VSConstants.S_OK; }
			public int CauseBreak() { return VSConstants.S_OK; }
			public int Continue(IDebugThread2 pThread) { return VSConstants.S_OK; }
			public int Detach() { return VSConstants.S_OK; }
			public int DisableENC(EncUnavailableReason reason) { throw new NotImplementedException(); }
			public int EnumPrograms(out IEnumDebugPrograms2 ppEnum) { ppEnum = null; return VSConstants.S_OK; }
			public int EnumThreads(out IEnumDebugThreads2 ppEnum) { ppEnum = null; return VSConstants.S_OK; }
			public int Execute(IDebugThread2 pThread) { return VSConstants.S_OK; }
			public int GetAttachedSessionName(out string pbstrSessionName) { pbstrSessionName = null; return VSConstants.S_OK; }
			public int GetDebugReason(enum_DEBUG_REASON[] pReason) { throw new NotImplementedException(); }
			public int GetENCAvailableState(EncUnavailableReason[] pReason) { throw new NotImplementedException(); }
			public int GetEngineFilter(GUID_ARRAY[] pEngineArray) { throw new NotImplementedException(); }
			public int GetHostingProcessLanguage(out Guid pguidLang) { throw new NotImplementedException(); }
			public int GetInfo(enum_PROCESS_INFO_FIELDS Fields, PROCESS_INFO[] pProcessInfo) { return VSConstants.S_OK; }
			public int GetName(enum_GETNAME_TYPE gnType, out string pbstrName) { pbstrName = null; return VSConstants.S_OK; }
			public int GetPort(out IDebugPort2 ppPort) { ppPort = m_pPort; return VSConstants.S_OK; }
			public int GetPhysicalProcessId(AD_PROCESS_ID[] pProcessId)
			{
				pProcessId[0].ProcessIdType = (uint)enum_AD_PROCESS_ID.AD_PROCESS_ID_GUID;
				pProcessId[0].guidProcessId = m_ProcessGuid;
				return VSConstants.S_OK;
			}
			public int GetProcessId(out Guid pguidProcessId) { pguidProcessId = m_ProcessGuid; return VSConstants.S_OK; }
			public int GetServer(out IDebugCoreServer2 ppServer) { ppServer = null; return VSConstants.S_OK; }
			public int SetHostingProcessLanguage(ref Guid guidLang) { throw new NotImplementedException(); }
			public int Step(IDebugThread2 pThread, enum_STEPKIND sk, enum_STEPUNIT Step) { return VSConstants.S_OK; }
			public int Terminate() { return VSConstants.S_OK; }
		}

		readonly BreakpointManager m_BreakpointManager;
		readonly ConnectionManager m_ConnectionManager;
		IDebugEventCallback2 m_EventCallback;
		Guid m_ProgramGuid;

		~Engine()
		{
		}

		void OnConnect(Connection connection)
		{
			m_BreakpointManager.SendAllBound(connection);
		}

		internal bool SendEventCallback(IDebugEvent2 evt, Guid guid)
		{
			var callback = m_EventCallback;
			if (null == callback)
			{
				return false;
			}

			uint uAttributes = 0;
			if (VSConstants.S_OK != evt.GetAttributes(out uAttributes))
			{
				return false;
			}

			return (VSConstants.S_OK == callback.Event(
				this,
				null,
				this,
				null,
				evt,
				ref guid,
				uAttributes));
		}
		#endregion

		#region Overrides
		#region IDebugEngine2
		int IDebugEngine2.Attach(
			IDebugProgram2[] rgpPrograms,
			IDebugProgramNode2[] rgpProgramNodes,
			uint celtPrograms,
			IDebugEventCallback2 eventCallback,
			enum_ATTACH_REASON dwReason)
		{
			m_EventCallback = eventCallback;
			rgpPrograms[0].GetProgramId(out m_ProgramGuid);
			return VSConstants.S_OK;
		}

		int IDebugEngine2.CauseBreak() { return ((IDebugProgram2)this).CauseBreak(); }
		int IDebugEngine2.ContinueFromSynchronousEvent(IDebugEvent2 eventObject) { return VSConstants.S_OK; }

		public int CreatePendingBreakpoint(IDebugBreakpointRequest2 pBPRequest, out IDebugPendingBreakpoint2 ppPendingBP)
		{
			m_BreakpointManager.CreatePendingBreakpoint(pBPRequest, out ppPendingBP);
			return VSConstants.S_OK;
		}

		int IDebugEngine2.DestroyProgram(IDebugProgram2 pProgram)
		{
			return VSConstants.S_OK;
		}

		public int EnumPrograms(out IEnumDebugPrograms2 ppEnum) { ppEnum = null; return VSConstants.S_OK; }

		int IDebugEngine2.GetEngineId(out Guid rOut)
		{
			rOut = Constants.kEngineGuid;
			return VSConstants.S_OK;
		}

		int IDebugEngine2.RemoveAllSetExceptions(ref Guid guidType) { return VSConstants.S_OK; }
		int IDebugEngine2.RemoveSetException(EXCEPTION_INFO[] pException) { return VSConstants.S_OK; }
		int IDebugEngine2.SetException(EXCEPTION_INFO[] pException) { return VSConstants.S_OK; }
		int IDebugEngine2.SetLocale(ushort wLangID) { return VSConstants.S_OK; }
		int IDebugEngine2.SetMetric(string pszMetric, object varValue) { return VSConstants.S_OK; }
		int IDebugEngine2.SetRegistryRoot(string pszRegistryRoot) { return VSConstants.S_OK; }
		#endregion

		#region IDebugEngineLaunch2
		int IDebugEngineLaunch2.CanTerminateProcess(IDebugProcess2 process)
		{
			return VSConstants.S_OK;
		}

		int IDebugEngineLaunch2.LaunchSuspended(
			string pszServer,
			IDebugPort2 pPort,
			string sExe,
			string sArgs,
			string sDir,
			string sEnv,
			string sOptions,
			enum_LAUNCH_FLAGS launchFlags,
			uint hStdInput,
			uint hStdOutput,
			uint hStdError,
			IDebugEventCallback2 eventCallback,
			out IDebugProcess2 rpProcess)
		{
			rpProcess = new EngineDummyProcess(pPort);
			m_EventCallback = eventCallback;
			return VSConstants.S_OK;
		}

		int IDebugEngineLaunch2.ResumeProcess(IDebugProcess2 pProcess)
		{
			if (null == pProcess)
			{
				return VSConstants.E_FAIL;
			}

			IDebugPort2 pPort;
			int iResult = pProcess.GetPort(out pPort);
			if (VSConstants.S_OK != iResult) { return iResult; }

			Guid guid;
			iResult = pProcess.GetProcessId(out guid);
			if (VSConstants.S_OK != iResult) { return iResult; }

			var pDefaultPort = (IDebugDefaultPort2)pPort;

			IDebugPortNotify2 pNotify;
			iResult = pDefaultPort.GetPortNotify(out pNotify);
			if (VSConstants.S_OK != iResult) { return iResult; }

			return pNotify.AddProgramNode(new ProgramNode(guid));
		}

		int IDebugEngineLaunch2.TerminateProcess(IDebugProcess2 process)
		{
			if (null == process)
			{
				return VSConstants.E_FAIL;
			}

			return process.Terminate();
		}
		#endregion

		#region IDebugProgram3
		[Obsolete] public int Attach(IDebugEventCallback2 pCallback) { return VSConstants.E_NOTIMPL; }
		public int CanDetach() { return VSConstants.S_OK; }
		public int CauseBreak()
		{
			SendCommandToAll(new commands.Break());
			return VSConstants.S_OK;
		}

		public int Continue(IDebugThread2 pThread)
		{
			SendCommandToAll(new commands.Continue());
			return VSConstants.S_OK;
		}

		public int Detach() { return VSConstants.S_OK; }
		public int EnumCodeContexts(IDebugDocumentPosition2 pDocPos, out IEnumDebugCodeContexts2 ppEnum) { ppEnum = null; return VSConstants.E_NOTIMPL; }
		public int EnumCodePaths(string hint, IDebugCodeContext2 start, IDebugStackFrame2 frame, int fSource, out IEnumCodePaths2 pathEnum, out IDebugCodeContext2 safetyContext) { pathEnum = null; safetyContext = null; return VSConstants.E_NOTIMPL; }
		public int EnumModules(out IEnumDebugModules2 ppEnum) { ppEnum = null; return VSConstants.E_NOTIMPL; }
		[Obsolete] int IDebugEngine2.EnumPrograms(out IEnumDebugPrograms2 programs) { programs = null; return VSConstants.E_NOTIMPL; }
		public int EnumThreads(out IEnumDebugThreads2 ppEnum) { ppEnum = null; return VSConstants.E_NOTIMPL; }
		[Obsolete] public int Execute() { return VSConstants.E_NOTIMPL; }
		public int ExecuteOnThread(IDebugThread2 pThread)
		{
			SendCommandToAll(new commands.Continue());
			return VSConstants.S_OK;
		}
		public int GetDebugProperty(out IDebugProperty2 ppProperty) { throw new NotImplementedException(); }
		public int GetDisassemblyStream(enum_DISASSEMBLY_STREAM_SCOPE dwScope, IDebugCodeContext2 codeContext, out IDebugDisassemblyStream2 disassemblyStream) { disassemblyStream = null; return VSConstants.E_NOTIMPL; }
		public int GetENCUpdate(out object update) { update = null; return VSConstants.E_NOTIMPL; }

		public int GetEngineInfo(out string rsEngineName, out Guid rEngineGuid)
		{
			rsEngineName = Constants.ksEngineName;
			rEngineGuid = Constants.kEngineGuid;
			return VSConstants.S_OK;
		}

		public int GetMemoryBytes(out IDebugMemoryBytes2 ppMemoryBytes) { ppMemoryBytes = null; return VSConstants.E_NOTIMPL; }
		public int GetName(out string programName) { programName = null; return VSConstants.E_NOTIMPL; }
		[Obsolete] public int GetProcess(out IDebugProcess2 process) { process = null; return VSConstants.E_NOTIMPL; }

		public int GetProgramId(out Guid pguidProgramId)
		{
			pguidProgramId = m_ProgramGuid;
			return VSConstants.S_OK;
		}

		public int Step(IDebugThread2 pThread, enum_STEPKIND eStepKind, enum_STEPUNIT eStepUnit)
		{
			SendCommandToAll(new commands.Step(eStepKind, eStepUnit));
			return VSConstants.S_OK;
		}

		public int Terminate() { return VSConstants.S_OK; }
		public int WriteDump(enum_DUMPTYPE DUMPTYPE, string pszDumpUrl) { throw new NotImplementedException(); }
		#endregion
		#endregion

		#region Internal handlers for client-to-server messages.
		internal void OnBreakpointHit(Connection.ERPC eRPC, UInt32 uResponseToken, BinaryReader reader)
		{
			var hit = new events.HitBreakpoint(reader);
			var work = new WaitCallback((unused) =>
			{
				hit.Send(this);
			});
		}
		#endregion

		public Engine()
		{
			m_BreakpointManager = new BreakpointManager(this);
			m_ConnectionManager = new ConnectionManager(this, kiPort);

			// TODO: Kind of a mess right now - need to either
			// use callbacks always, or make this an explicitly
			// registered handler in Connection.
			m_ConnectionManager.OnConnect += OnConnect;
		}

		/// <summary>
		/// BreakpointManager for the entire engine.
		/// </summary>
		internal BreakpointManager BreakpointManager { get { return m_BreakpointManager; } }

		/// <summary>
		/// Enqueue a command to be sent to all connected clients.
		/// </summary>
		public void SendCommandToAll(commands.BaseCommand command)
		{
			m_ConnectionManager.SendCommandToAll(command);
		}
	}
}
