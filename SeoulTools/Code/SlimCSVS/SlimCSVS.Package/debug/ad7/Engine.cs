// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using Microsoft.VisualStudio;
using Microsoft.VisualStudio.Debugger.Interop;
using SlimCSVS.Package.projects;
using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;

namespace SlimCSVS.Package.debug.ad7
{
	[Guid(Constants.ksEngineGuid)]
	public class Engine : IDebugEngine2, IDebugEngineLaunch2, IDebugProgram3, IDisposable
	{
		/// <summary>
		/// Fixed port used by the debugger connection.
		/// </summary>
		public const int kiPort = 25762;

		#region Private members
		readonly string m_sSolutionFile;
		FileStream m_LockFile;
		BreakpointManager m_BreakpointManager;
		ConnectionManager m_ConnectionManager;
		MessageManager m_MessageManager;
		AD_PROCESS_ID m_ProcessId;
		IDebugEventCallback2 m_EventCallback;
		IDebugProcess2 m_Process;
		Thread m_Thread;
		Guid m_ProgramGuid;
		bool m_bDisposed;

		System.Collections.Concurrent.ConcurrentQueue<(UInt32, string)> m_ChildRequestResult = new System.Collections.Concurrent.ConcurrentQueue<(UInt32, string)>();
		System.Threading.AutoResetEvent m_ChildRequestEvent = new System.Threading.AutoResetEvent(false);
		System.Collections.Concurrent.ConcurrentQueue<UInt32> m_FrameInfoRequestResult = new System.Collections.Concurrent.ConcurrentQueue<uint>();
		System.Threading.AutoResetEvent m_FrameInfoRequestEvent = new System.Threading.AutoResetEvent(false);
		System.Collections.Concurrent.ConcurrentQueue<(UInt32, string, bool)> m_SetVariableRequestResult = new System.Collections.Concurrent.ConcurrentQueue<(UInt32, string, bool)>();
		System.Threading.AutoResetEvent m_SetVariableRequestEvent = new System.Threading.AutoResetEvent(false);

		/// <summary>
		/// Attempts to write a lock file to disk.
		/// </summary>
		/// <returns>true if successful, false otherwise.</returns>
		/// <remarks>
		/// The lock file has two purposes. First, it prevents dueling debugger
		/// sessions. But, more importantly, it is used by the application client
		/// to detect when a debugger host is listening for connections.
		/// </remarks>
		bool AcquireLock()
		{
			lock (this)
			{
				if (null != m_LockFile)
				{
					return true;
				}

				var sLockFile = LockFilePath;
				if (string.IsNullOrEmpty(sLockFile))
				{
					return false;
				}

				try
				{
					m_LockFile = File.Open(sLockFile, FileMode.Create, FileAccess.Write, FileShare.None);
				}
				catch
				{
					m_LockFile = null;
					return false;
				}

				return true;
			}
		}

		/// <summary>
		/// Path to write the lock file. Tied to the current active solution.
		/// </summary>
		string LockFilePath
		{
			get
			{
				return Path.ChangeExtension(m_sSolutionFile, ".debugger_listener");
			}
		}

		/// <summary>
		/// Release any current lock. Deletes and cleans up the lock file.
		/// </summary>
		void ReleaseLock()
		{
			lock (this)
			{
				if (null == m_LockFile)
				{
					return;
				}

				var lockFile = m_LockFile;
				m_LockFile = null;

				try
				{
					lockFile.Close();
					lockFile.Dispose();
					lockFile = null;
				}
				catch
				{}
				finally
				{
					try
					{
						var sPath = LockFilePath;
						if (!string.IsNullOrEmpty(sPath))
						{
							File.Delete(sPath);
						}
					}
					catch
					{ }
				}
			}
		}

		void HandleDetach()
		{
			if (null != m_Process)
			{
				m_Process.Detach();
				m_Process = null;
			}
			ProgramDestroyEvent.Send(this, 0, false);
			m_Thread = null;
			m_ProgramGuid = default(Guid);
			m_EventCallback = null;
			m_ProcessId = default(AD_PROCESS_ID);
			ReleaseLock();
		}

		void OnDisconnectHandler(Connection connection)
		{
			ProgramDestroyEvent.Send(this, 0, true);
		}

		~Engine()
		{
			Dispose();
		}

		internal bool SendEventCallback(IDebugEvent2 evt, Guid guid)
		{
			var callback = m_EventCallback;
			if (null == callback)
			{
				return false;
			}

			uint uAttributes = 0;
			int iResult = evt.GetAttributes(out uAttributes);
			if (VSConstants.S_OK != iResult)
			{
				return false;
			}

			iResult = callback.Event(
				this,
				m_Process,
				this,
				m_Thread,
				evt,
				ref guid,
				uAttributes);
			var bReturn = (VSConstants.S_OK == iResult);
			return bReturn;
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
			if (!AcquireLock())
			{
				return VSConstants.E_FAIL;
			}

			m_EventCallback = eventCallback;
			rgpPrograms[0].GetProgramId(out m_ProgramGuid);

			EngineCreateEvent.Send(this);
			ProgramCreateEvent.Send(this);
			m_Thread = new Thread(this);
			ThreadCreateEvent.Send(this);
			LoadCompleteEvent.Send(this);

			return VSConstants.S_OK;
		}

		int IDebugEngine2.CauseBreak()
		{
			return ((IDebugProgram2)this).CauseBreak();
		}

		int IDebugEngine2.ContinueFromSynchronousEvent(IDebugEvent2 eventObject)
		{
			if (eventObject is ProgramDestroyEvent)
			{
				Dispose();
			}

			return VSConstants.S_OK;
		}

		public int CreatePendingBreakpoint(IDebugBreakpointRequest2 pBPRequest, out IDebugPendingBreakpoint2 ppPendingBP)
		{
			m_BreakpointManager.CreatePendingBreakpoint(pBPRequest, out ppPendingBP);
			return VSConstants.S_OK;
		}

		int IDebugEngine2.DestroyProgram(IDebugProgram2 pProgram)
		{
			return VSConstants.S_OK;
		}

		public int EnumPrograms(out IEnumDebugPrograms2 ppEnum)
		{
			ppEnum = null;
			return VSConstants.E_NOTIMPL;
		}

		int IDebugEngine2.GetEngineId(out Guid rOut)
		{
			rOut = Constants.kEngineGuid;
			return VSConstants.S_OK;
		}

		int IDebugEngine2.RemoveAllSetExceptions(ref Guid guidType)
		{
			return VSConstants.S_OK;
		}

		int IDebugEngine2.RemoveSetException(EXCEPTION_INFO[] pException)
		{
			return VSConstants.S_OK;
		}

		int IDebugEngine2.SetException(EXCEPTION_INFO[] pException)
		{
			return VSConstants.S_OK;
		}

		int IDebugEngine2.SetLocale(ushort wLangID)
		{
			return VSConstants.S_OK;
		}

		int IDebugEngine2.SetMetric(string pszMetric, object varValue)
		{
			return VSConstants.S_OK;
		}

		int IDebugEngine2.SetRegistryRoot(string pszRegistryRoot)
		{
			return VSConstants.S_OK;
		}
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
			m_ProcessId = new AD_PROCESS_ID
			{
				guidProcessId = Guid.NewGuid(),
				ProcessIdType = (uint)enum_AD_PROCESS_ID.AD_PROCESS_ID_GUID,
			};
			m_EventCallback = eventCallback;
			pPort.GetProcess(m_ProcessId, out m_Process);

			// Done.
			rpProcess = m_Process;
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

			var pDefaultPort = (IDebugDefaultPort2)pPort;

			IDebugPortNotify2 pNotify;
			iResult = pDefaultPort.GetPortNotify(out pNotify);
			if (VSConstants.S_OK != iResult) { return iResult; }

			iResult = pNotify.AddProgramNode(new ProgramNode(m_ProcessId));
			return iResult;
		}

		int IDebugEngineLaunch2.TerminateProcess(IDebugProcess2 process)
		{
			if (null == process)
			{
				return VSConstants.E_FAIL;
			}

			return m_Process.Terminate();
		}
		#endregion

		#region IDebugProgram3
		[Obsolete]
		public int Attach(IDebugEventCallback2 pCallback)
		{
			return VSConstants.E_NOTIMPL;
		}

		public int CanDetach()
		{
			return VSConstants.S_OK;
		}

		public int CauseBreak()
		{
			m_MessageManager.SendBreak();
			return VSConstants.S_OK;
		}

		public int Continue(IDebugThread2 pThread)
		{
			m_Thread.OnResume();
			m_MessageManager.SendContinue();
			return VSConstants.S_OK;
		}

		public int Detach()
		{
			HandleDetach();
			return VSConstants.S_OK;
		}

		public int EnumCodeContexts(IDebugDocumentPosition2 pDocPos, out IEnumDebugCodeContexts2 ppEnum)
		{
			ppEnum = null;
			return VSConstants.E_NOTIMPL;
		}

		public int EnumCodePaths(string hint, IDebugCodeContext2 start, IDebugStackFrame2 frame, int fSource, out IEnumCodePaths2 pathEnum, out IDebugCodeContext2 safetyContext)
		{
			pathEnum = null;
			safetyContext = null;
			return VSConstants.E_NOTIMPL;
		}

		public int EnumModules(out IEnumDebugModules2 ppEnum)
		{
			ppEnum = null;
			return VSConstants.E_NOTIMPL;
		}

		[Obsolete] int IDebugEngine2.EnumPrograms(out IEnumDebugPrograms2 programs)
		{
			programs = null;
			return VSConstants.E_NOTIMPL;
		}

		public int EnumThreads(out IEnumDebugThreads2 ppEnum)
		{
			var threads = new Thread[1];
			threads[0] = m_Thread;

			ppEnum = new ThreadEnum(threads);
			return VSConstants.S_OK;
		}

		[Obsolete] public int Execute()
		{
			return VSConstants.E_NOTIMPL;
		}

		public int ExecuteOnThread(IDebugThread2 pThread)
		{
			m_Thread.OnResume();
			m_MessageManager.SendContinue();
			return VSConstants.S_OK;
		}

		public int GetDebugProperty(out IDebugProperty2 ppProperty)
		{
			ppProperty = null;
			return VSConstants.E_NOTIMPL;
		}

		public int GetDisassemblyStream(enum_DISASSEMBLY_STREAM_SCOPE dwScope, IDebugCodeContext2 codeContext, out IDebugDisassemblyStream2 disassemblyStream)
		{
			disassemblyStream = null;
			return VSConstants.E_NOTIMPL;
		}

		public int GetENCUpdate(out object update)
		{
			update = null;
			return VSConstants.E_NOTIMPL;
		}

		public int GetEngineInfo(out string rsEngineName, out Guid rEngineGuid)
		{
			rsEngineName = Constants.ksEngineName;
			rEngineGuid = Constants.kEngineGuid;
			return VSConstants.S_OK;
		}

		public int GetMemoryBytes(out IDebugMemoryBytes2 ppMemoryBytes)
		{
			ppMemoryBytes = null;
			return VSConstants.E_NOTIMPL;
		}

		public int GetName(out string programName)
		{
			programName = Constants.ksProgramName;
			return VSConstants.S_OK;
		}

		[Obsolete] public int GetProcess(out IDebugProcess2 process)
		{
			process = m_Process;
			return VSConstants.S_OK;
		}

		public int GetProgramId(out Guid pguidProgramId)
		{
			pguidProgramId = m_ProgramGuid;
			return VSConstants.S_OK;
		}

		public int Step(IDebugThread2 pThread, enum_STEPKIND eStepKind, enum_STEPUNIT eStepUnit)
		{
			m_Thread.OnResume();
			m_MessageManager.SendStep(eStepKind, eStepUnit);
			return VSConstants.S_OK;
		}

		public int Terminate()
		{
			return Detach();
		}

		public int WriteDump(enum_DUMPTYPE DUMPTYPE, string pszDumpUrl)
		{
			return VSConstants.E_NOTIMPL;
		}
		#endregion
		#endregion

		#region Internal accessors
		internal void OnBreakpointStateChange(UInt32 uRawBreakpoint, bool bEnable)
		{
			m_MessageManager.SendBreakpointChange(uRawBreakpoint, bEnable);
		}

		internal void OnHitBreakpoint(events.HitBreakpoint bp, SuspendReason eSuspendReason)
		{
			m_Thread.HitBreakpoint = bp;
			bp.Send(eSuspendReason, this, m_BreakpointManager);
		}

		internal void OnReceiveFrameData(UInt32 uDepth, List<Variable> vars)
		{
			m_Thread.SetFrameData(uDepth, vars);
			m_FrameInfoRequestResult.Enqueue(uDepth);
			m_FrameInfoRequestEvent.Set();
		}

		internal void OnReceiveVariableChildren(UInt32 uStackDepth, string sPath, List<Variable> vars)
		{
			m_Thread.SetVariableChildren(uStackDepth, sPath, vars);
			m_ChildRequestResult.Enqueue((uStackDepth, sPath));
			m_ChildRequestEvent.Set();
		}

		internal void OnSetVariable(UInt32 uStackDepth, string sPath, bool bSuccess)
		{
			m_SetVariableRequestResult.Enqueue((uStackDepth, sPath, bSuccess));
			m_SetVariableRequestEvent.Set();
		}

		internal bool RequestChildren(UInt32 uStackDepth, string sPath, TimeSpan timeout)
		{
			m_MessageManager.SendChildrenRequest(uStackDepth, sPath);
			while (m_ChildRequestEvent.WaitOne(timeout))
			{
				var iCount = m_ChildRequestResult.Count;
				(UInt32 uResultStackDepth, string sResultPath) res = (0u, string.Empty);
				while (m_ChildRequestResult.TryDequeue(out res) && iCount > 0)
				{
					--iCount;
					if (res.uResultStackDepth == uStackDepth &&
						res.sResultPath == sPath)
					{
						return true;
					}
					else
					{
						m_ChildRequestResult.Enqueue(res);
					}
				}
			}

			return false;
		}

		internal bool RequestFrameInfo(UInt32 uDepth, TimeSpan timeout)
		{
			m_MessageManager.SendFrameInfoRequest(uDepth);
			while (m_FrameInfoRequestEvent.WaitOne(timeout))
			{
				var iCount = m_FrameInfoRequestResult.Count;
				UInt32 uResultDepth = 0;
				while (m_FrameInfoRequestResult.TryDequeue(out uResultDepth) && iCount > 0)
				{
					--iCount;
					if (uResultDepth == uDepth)
					{
						return true;
					}
					else
					{
						m_FrameInfoRequestResult.Enqueue(uResultDepth);
					}
				}
			}

			return false;
		}

		internal bool RequestVariableSet(StackFrame frame, string sPath, VariableType eType, string sValue, TimeSpan timeout)
		{
			var uStackDepth = frame.StackDepth;
			m_MessageManager.SendVariableSet(uStackDepth, sPath, eType, sValue);
			while (m_SetVariableRequestEvent.WaitOne(timeout))
			{
				var iCount = m_SetVariableRequestResult.Count;
				(UInt32 uResultStackDepth, string sResultPath, bool bSuccess) result = (0u, string.Empty, false);
				while (m_SetVariableRequestResult.TryDequeue(out result) && iCount > 0)
				{
					--iCount;
					if (result.uResultStackDepth == uStackDepth &&
						result.sResultPath == sPath)
					{
						// Update the value.
						if (result.bSuccess)
						{
							m_Thread.SetVariableValue(frame, sPath, sValue);
						}

						return result.bSuccess;
					}
					else
					{
						m_SetVariableRequestResult.Enqueue(result);
					}
				}
			}

			return false;
		}

		internal void ResolveFileAndLine(UInt32 uRawBreakpoint, out string sFileName, out Int32 iLine)
		{
			m_BreakpointManager.ResolveFileAndLine(uRawBreakpoint, out sFileName, out iLine);
		}

		internal UInt32 ResolveRawBreakpoint(string sFileName, Int32 iLine)
		{
			return m_BreakpointManager.ResolveRawBreakpoint(sFileName, iLine);
		}
		#endregion

		public Engine()
		{
			ProgramProjectFlavor.GlobalServiceProvider.GetSolutionFile(out m_sSolutionFile);
			if (string.IsNullOrEmpty(m_sSolutionFile)) { m_sSolutionFile = string.Empty; }
			m_BreakpointManager = new BreakpointManager(this);
			m_ConnectionManager = new ConnectionManager(this, kiPort);
			m_MessageManager = new MessageManager(this, m_BreakpointManager, m_ConnectionManager);

			m_ConnectionManager.OnDisconnect += OnDisconnectHandler;
		}

		/// <summary>
		/// Cleanup resources, shut down connections, place the engine in an unusable state.
		/// </summary>
		public void Dispose()
		{
			if (m_bDisposed)
			{
				return;
			}

			// Now considered disposed.
			m_bDisposed = true;

			// Finish dispose.
			HandleDetach();
			m_ConnectionManager.OnDisconnect -= OnDisconnectHandler;
			m_MessageManager = null;
			m_ConnectionManager.Dispose();
			m_ConnectionManager = null;
			m_BreakpointManager = null;
		}

		/// <summary>
		/// Enqueue a message to be sent to all connected clients.
		/// </summary>
		public void SendToAll(Message msg)
		{
			m_ConnectionManager.SendToAll(msg);
		}
	}
}
