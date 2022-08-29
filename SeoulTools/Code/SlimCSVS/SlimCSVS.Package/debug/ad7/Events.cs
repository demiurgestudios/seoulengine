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
	class AsyncEvent : IDebugEvent2
	{
		public const uint kiAttributes = (uint)enum_EVENTATTRIBUTES.EVENT_ASYNCHRONOUS;

		int IDebugEvent2.GetAttributes(out uint eventAttributes)
		{
			eventAttributes = kiAttributes;
			return VSConstants.S_OK;
		}
	}

	sealed class AsyncBreakCompleteEvent : StoppingEvent, IDebugBreakEvent2
	{
		public static readonly Guid kGuid = new Guid("c7405d1d-e24b-44e0-b707-d8a5a4e1641b");
	}

	sealed class BreakpointEvent : StoppingEvent, IDebugBreakpointEvent2
	{
		public static readonly Guid kGuid = new Guid("501C1E21-C557-48B8-BA30-A1EAB0BC4A74");

		#region Private members
		readonly IEnumDebugBoundBreakpoints2 m_Breakpoints;

		BreakpointEvent(IDebugBoundBreakpoint2 bps)
		{
			m_Breakpoints = new BoundBreakpointsEnum(new IDebugBoundBreakpoint2[] { bps });
		}
		#endregion

		#region Overrides
		int IDebugBreakpointEvent2.EnumBreakpoints(out IEnumDebugBoundBreakpoints2 ppEnum)
		{
			ppEnum = m_Breakpoints;
			return VSConstants.S_OK;
		}
		#endregion

		internal static bool Send(Engine engine, IDebugBoundBreakpoint2 bpp)
		{
			return engine.SendEventCallback(new BreakpointEvent(bpp), kGuid);
		}
	}

	class StoppingEvent : IDebugEvent2
	{
		public const uint kiAttributes = (uint)enum_EVENTATTRIBUTES.EVENT_ASYNC_STOP;

		int IDebugEvent2.GetAttributes(out uint eventAttributes)
		{
			eventAttributes = kiAttributes;
			return VSConstants.S_OK;
		}
	}

	class SynchronousEvent : IDebugEvent2
	{
		public const uint kiAttributes = (uint)enum_EVENTATTRIBUTES.EVENT_SYNCHRONOUS;

		int IDebugEvent2.GetAttributes(out uint eventAttributes)
		{
			eventAttributes = kiAttributes;
			return VSConstants.S_OK;
		}
	}

	class SynchronousStoppingEvent : IDebugEvent2
	{
		public const uint kiAttributes = (uint)enum_EVENTATTRIBUTES.EVENT_STOPPING | (uint)enum_EVENTATTRIBUTES.EVENT_SYNCHRONOUS;

		int IDebugEvent2.GetAttributes(out uint eventAttributes)
		{
			eventAttributes = kiAttributes;
			return VSConstants.S_OK;
		}
	}

	sealed class EngineCreateEvent : AsyncEvent, IDebugEngineCreateEvent2
	{
		public static readonly Guid kGuid = new Guid("FE5B734C-759D-4E59-AB04-F103343BDD06");

		#region Private members
		readonly IDebugEngine2 m_Engine;

		EngineCreateEvent(Engine engine)
		{
			m_Engine = engine;
		}
		#endregion

		#region Overrides
		int IDebugEngineCreateEvent2.GetEngine(out IDebugEngine2 rEngine)
		{
			rEngine = m_Engine;
			return VSConstants.S_OK;
		}
		#endregion

		internal static void Send(Engine engine)
		{
			engine.SendEventCallback(new EngineCreateEvent(engine), kGuid);
		}
	}

	sealed class LoadCompleteEvent : StoppingEvent, IDebugLoadCompleteEvent2
	{
		public static readonly Guid kGuid = new Guid("B1844850-1349-45D4-9F12-495212F5EB0B");

		internal static void Send(Engine engine)
		{
			engine.SendEventCallback(new LoadCompleteEvent(), kGuid);
		}
	}

	sealed class OutputDebugStringEvent : AsyncEvent, IDebugOutputStringEvent2
	{
		public static readonly Guid kGuid = new Guid("569c4bb1-7b82-46fc-ae28-4536dd53e");

		#region Private members
		readonly string m_sMessage;
		#endregion

		#region Overrides
		int IDebugOutputStringEvent2.GetString(out string rs)
		{
			rs = m_sMessage;
			return VSConstants.S_OK;
		}
		#endregion

		public OutputDebugStringEvent(string s)
		{
			m_sMessage = s;
		}
	}

	sealed class ProgramCreateEvent : AsyncEvent, IDebugProgramCreateEvent2
	{
		public static readonly Guid kGuid = new Guid("96CD11EE-ECD4-4E89-957E-B5D496FC4139");

		#region Private members
		ProgramCreateEvent()
		{
		}
		#endregion

		internal static void Send(Engine engine)
		{
			engine.SendEventCallback(new ProgramCreateEvent(), kGuid);
		}
	}

	sealed class ProgramDestroyEvent : SynchronousEvent, IDebugProgramDestroyEvent2, IDebugProgramDestroyEventFlags2
	{
		public static readonly Guid kGuid = new Guid("E147E9E3-6440-4073-A7B7-A65592C714B5");

		#region Private members
		readonly uint m_uExitCode;
		readonly bool m_bContinueDebugging;

		ProgramDestroyEvent(uint uExitCode, bool bContinueDebugging)
		{
			m_uExitCode = uExitCode;
			m_bContinueDebugging = bContinueDebugging;
		}
		#endregion

		#region Overrides
		int IDebugProgramDestroyEvent2.GetExitCode(out uint ruExitCode)
		{
			ruExitCode = m_uExitCode;
			return VSConstants.S_OK;
		}

		int IDebugProgramDestroyEventFlags2.GetFlags(enum_PROGRAM_DESTROY_FLAGS[] pdwFlags)
		{
			pdwFlags[0] = (m_bContinueDebugging ? enum_PROGRAM_DESTROY_FLAGS.PROGRAM_DESTROY_CONTINUE_DEBUGGING : 0);
			return VSConstants.S_OK;
		}
		#endregion

		internal static void Send(Engine engine, uint uExitCode, bool bContinueDebugging)
		{
			engine.SendEventCallback(new ProgramDestroyEvent(uExitCode, bContinueDebugging), kGuid);
		}
	}

	sealed class StepCompleteEvent : StoppingEvent, IDebugStepCompleteEvent2
	{
		public static readonly Guid kGuid = new Guid("0F7F24C1-74D9-4EA6-A3EA-7EDB2D81441D");

		internal static void Send(Engine engine)
		{
			engine.SendEventCallback(new StepCompleteEvent(), kGuid);
		}
	}

	sealed class ThreadCreateEvent : AsyncEvent, IDebugThreadCreateEvent2
	{
		public static readonly Guid kGuid = new Guid("2090CCFC-70C5-491D-A5E8-BAD2DD9EE3EA");

		internal static void Send(Engine engine)
		{
			engine.SendEventCallback(new ThreadCreateEvent(), kGuid);
		}
	}

	sealed class ThreadDestroyEvent : AsyncEvent, IDebugThreadDestroyEvent2
	{
		public static readonly Guid kGuid = new Guid("2C3B7532-A36F-4A6E-9072-49BE649B8541");

		#region Private members
		readonly uint m_uExitCode;

		ThreadDestroyEvent(uint uExitCode)
		{
			m_uExitCode = uExitCode;
		}
		#endregion

		#region Overrides
		int IDebugThreadDestroyEvent2.GetExitCode(out uint ruExitCode)
		{
			ruExitCode = m_uExitCode;
			return VSConstants.S_OK;
		}
		#endregion

		internal static void Send(Engine engine, uint uExitCode)
		{
			engine.SendEventCallback(new ThreadDestroyEvent(uExitCode), kGuid);
		}
	}
}
