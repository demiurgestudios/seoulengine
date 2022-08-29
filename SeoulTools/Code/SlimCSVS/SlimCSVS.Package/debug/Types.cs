// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using Microsoft.VisualStudio.Debugger.Interop;
using System;
using System.IO;

namespace SlimCSVS.Package.debug
{
	public struct StackFrame
	{
		public string m_sFunc;
		public UInt32 m_uRawBreakpoint;

		public string ToDisplayName(ad7.Engine engine, enum_FRAMEINFO_FLAGS dwFieldSpec)
		{
			var sFileName = string.Empty;
			int iLine = 1;
			engine.ResolveFileAndLine(m_uRawBreakpoint, out sFileName, out iLine);

			var sReturn = string.Empty;
			if ((dwFieldSpec & enum_FRAMEINFO_FLAGS.FIF_FUNCNAME_MODULE) != 0)
			{
				sReturn += $"{Path.GetFileName(sFileName)}!";
			}
			sReturn += m_sFunc;
			if ((dwFieldSpec & enum_FRAMEINFO_FLAGS.FIF_FUNCNAME_LINES) != 0)
			{
				sReturn += $" Line {iLine}";
			}

			return sReturn;
		}
	}

	/// <summary>
	/// Tags for messages that are sent from the client (e.g. SeoulEngine)
	/// to the debugger server (e.g. Visual Studio).
	/// </summary>
	public enum ClientTag
	{
		Unknown = -1,
		AskBreakpoints,
		BreakAt,
		Frame,
		GetChildren,
		Heartbeat,
		SetVariable,
		Sync,
		Version,
		Watch,
	}

	/// <summary>
	/// Used to record the current execution state of the debugger client.
	/// </summary>
	public enum ExecuteState
	{
		/// <summary>
		/// Regular execution, run until stop, user-defined breakpoint, falt, etc.
		/// </summary>
		Running,

		/// <summary>
		/// Running until a step into - break will occur at the next instruction when the stack index is greater-equal than the stack index at start of the step into. */
		/// </summary>
		StepInto,

		/// <summary>
		/// Running until a step out - break will occur at the next instruction when the stack index is less than the stack index at the start of the step out. */
		/// </summary>
		StepOut,

		/// <summary>
		/// Running until a step over - break will occur at the next instruction when the stack index is equal to the stack index at the step over. */
		/// </summary>
		StepOver,
	}

	/// <summary>
	/// Tags for messages that are sent from the debugger server to the client.
	/// </summary>
	public enum ServerTag
	{
		Unknown = -1,
		AddWatch,
		Break,
		Continue,
		GetFrame,
		GetChildren,
		RemoveWatch,
		SetBreakpoints,
		SetVariable,
		StepInto,
		StepOut,
		StepOver,
	}

	/// <summary>
	/// When the client decides to break execution, the server asks
	/// for a SuspendReason, which is described by one of these constants.
	/// </summary>
	public enum SuspendReason
	{
		Unknown = 0,

		/// <summary>
		/// Client hit a user defined breakpoint.
		/// </summary>
		Breakpoint = 1,

		/// <summary>
		/// Break for triggered watchpoint.
		/// </summary>
		Watch = 2,

		/// <summary>
		/// Client experienced an unrecoverable error.
		/// </summary>
		Fault = 3,

		/// <summary>
		/// Client is asking the server to allow a stop - in our usage model, we always
		/// just stop if we need to stop.
		/// </summary>
		StopRequest = 4,

		/// <summary>
		/// Once the client has stopped for a user defined breakpoint, the debugger can
		/// request various step actions passed the breakpoint. Once the client stops again
		/// after completion of the step action, its SuspendReason is kStep.
		/// </summary>
		Step = 5,

		/// <summary>
		/// Debugger equivalent to the native assembly 'trap' or 'int' instruction.
		/// </summary>
		HaltOpcode = 6,

		/// <summary>
		/// When a VM is first encountered, the client transmits a script lookup table
		/// (to allow script file identifiers to be 16-bits) and it breaks to give the
		/// debugger a chance to attach.
		/// </summary>
		ScriptLoaded = 7,
	}

	/// <summary>
	/// Used in server-to-client and client-to-server messages when a data
	/// type is required (matches the Lua value types).
	/// </summary>
	public enum VariableType
	{
		Nil,
		Boolean,
		LightUserData,
		Number,
		String,
		Table,
		Function,
		UserData,
		Thread,

		// Special value - used to indicate a table
		// that has no children, to avoid displaying a +
		// for table that will expand to nothing.
		EmptyTable,
	}

	public static class VariableTypeExtension
	{
		public static bool IsReadOnly(this VariableType eType)
		{
			switch (eType)
			{
				case VariableType.Boolean: return false;
				case VariableType.Number: return false;
				case VariableType.String: return false;
				default:
					return true;
			}
		}

		public static string ToDisplayName(this VariableType eType)
		{
			switch (eType)
			{
				case VariableType.Nil: return "null";
				case VariableType.Boolean: return "bool";
				case VariableType.LightUserData: return "<light-userdata>";
				case VariableType.Number: return "double";
				case VariableType.String: return "string";
				case VariableType.Table: return "Table";
				case VariableType.Function: return "<function>";
				case VariableType.UserData: return "<userdata>";
				case VariableType.Thread: return "<thread>";
				case VariableType.EmptyTable: return "Table";
				default: return "<error>";
			}
		}
	}

	public struct Variable
	{
		public string m_sName;
		public VariableType m_eType;
		public string m_sExtendedType;
		public string m_sValue;

		public static Variable Create(Message msg)
		{
			Variable ret;
			ret.m_sName = msg.ReadString();
			ret.m_eType = (VariableType)msg.ReadInt32();
			ret.m_sExtendedType = msg.ReadString();
			ret.m_sValue = msg.ReadString();
			return ret;
		}

		public bool HasChildren
		{
			get
			{
				return (m_eType == VariableType.Table);
			}
		}
	}
}
