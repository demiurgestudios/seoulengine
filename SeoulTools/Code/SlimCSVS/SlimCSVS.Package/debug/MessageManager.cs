// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using Microsoft.VisualStudio.Debugger.Interop;
using System;
using System.Collections.Generic;
using System.Threading;

namespace SlimCSVS.Package.debug
{
	sealed class MessageManager
	{
		#region Private members
		readonly ad7.Engine m_Engine;
		readonly ad7.BreakpointManager m_Breaks;
		readonly ConnectionManager m_Conn;
		volatile Thread m_Worker = null;

		void OnReceive(Message msg)
		{
			var clientTag = (ClientTag)msg.MessageTag;
			switch (clientTag)
			{
				// Send a response message with all breakpoints to the client.
				case ClientTag.AskBreakpoints:
					{
						var bpMsg = m_Breaks.SetBreakpointsMessage;
						m_Conn.SendToAll(bpMsg);
					}
					break;

				case ClientTag.BreakAt:
					{
						bool bResyncFileIds = false;

						// Read the suspend reason.
						var eSuspendReason = (SuspendReason)msg.ReadInt32();

						// Read callstack.
						var frames = new List<StackFrame>();
						while (msg.HasData)
						{
							var sFunc = msg.ReadString();
							var uRawBreakpoint = msg.ReadUInt32();

							// Check m_uRawBackpoint - if the file ID portion
							// is 0, it means the stack frame landed in a file
							// association that was not made prior to the client entering
							// the file, so we need to send fixups at the end.
							UInt16 uFileId = (UInt16)(uRawBreakpoint & 0x0000FFFF);
							if (0u == uFileId)
							{
								var sFileName = msg.ReadString();
								var iLine = (Int32)((uRawBreakpoint >> 16) & 0x0000FFFF);
								uRawBreakpoint = m_Engine.ResolveRawBreakpoint(sFileName, iLine);
								bResyncFileIds = true;
							}

							frames.Add(new StackFrame()
							{
								m_sFunc = sFunc,
								m_uRawBreakpoint = uRawBreakpoint
							});
						}

						var hit = new events.HitBreakpoint(frames);
						m_Engine.OnHitBreakpoint(hit, eSuspendReason);

						// If we need to resync, do so now.
						if (bResyncFileIds)
						{
							var bpMsg = m_Breaks.FileIdMessage;
							m_Conn.SendToAll(bpMsg);
						}
					}
					break;

				case ClientTag.Frame:
					{
						var uDepth = msg.ReadUInt32();
						var vars = new List<Variable>();
						while (msg.HasData)
						{
							vars.Add(Variable.Create(msg));
						}
						m_Engine.OnReceiveFrameData(uDepth, vars);
					}
					break;

				case ClientTag.GetChildren:
					{
						var uStackDepth = msg.ReadUInt32();
						var sPath = msg.ReadString();
						var vars = new List<Variable>();
						while (msg.HasData)
						{
							vars.Add(Variable.Create(msg));
						}

						// Sort children lexographically - we leave
						// frame variables alone, as they will be ordered
						// in initializaiton order.
						vars.Sort((a, b) =>
						{
							return util.Utilities.StrCmpLogicalW(a.m_sName, b.m_sName);
						});

						m_Engine.OnReceiveVariableChildren(uStackDepth, sPath, vars);
					}
					break;

				case ClientTag.Heartbeat:
					// No action from the server.
					break;

				case ClientTag.SetVariable:
					{
						var uStackDepth = msg.ReadUInt32();
						var sPath = msg.ReadString();
						var bSuccess = msg.ReadBool();
						m_Engine.OnSetVariable(uStackDepth, sPath, bSuccess);
					}
					break;

				case ClientTag.Sync:
					{
						// A sync message is just the client's way of waiting
						// for the server to perform some processing. We
						// need to immediately send a continue on receive
						// of this message.
						m_Conn.SendToAll(Message.Create(ServerTag.Continue));
					}
					break;

				default:
					throw new System.IO.IOException("Unexpected message tag: " + clientTag.ToString());
			}
		}

		void WorkerBody(object obj)
		{
			while (true)
			{
				var msg = m_Conn.ReceiveOne();
				OnReceive(msg);
			}
		}
		#endregion

		public MessageManager(
			ad7.Engine engine,
			ad7.BreakpointManager breaks,
			ConnectionManager conn)
		{
			m_Engine = engine;
			m_Breaks = breaks;
			m_Conn = conn;
			m_Worker = new Thread(WorkerBody);
			m_Worker.Name = "Message Manager Worker";
			m_Worker.Start();
		}

		public void SendBreak()
		{
			m_Conn.SendToAll(Message.Create(ServerTag.Break));
		}

		public void SendBreakpointChange(UInt32 uBreakpoint, bool bEnable)
		{
			// FileId message plus a single breakpoint change.
			var msg = m_Breaks.FileIdMessage;
			msg.Write(uBreakpoint);
			msg.Write(bEnable);
			m_Conn.SendToAll(msg);
		}

		public void SendContinue()
		{
			m_Conn.SendToAll(Message.Create(ServerTag.Continue));
		}

		public void SendChildrenRequest(UInt32 uStackDepth, string sPath)
		{
			var msg = Message.Create(ServerTag.GetChildren);
			msg.Write(uStackDepth);
			msg.Write(sPath);
			m_Conn.SendToAll(msg);
		}

		public void SendFrameInfoRequest(UInt32 uDepth)
		{
			var msg = Message.Create(ServerTag.GetFrame);
			msg.Write(uDepth);
			m_Conn.SendToAll(msg);
		}

		public void SendVariableSet(UInt32 uStackDepth, string sPath, VariableType eType, string sValue)
		{
			var msg = Message.Create(ServerTag.SetVariable);
			msg.Write(uStackDepth);
			msg.Write(sPath);
			msg.Write((Int32)eType);
			msg.Write(sValue);
			m_Conn.SendToAll(msg);
		}

		public void SendStep(enum_STEPKIND eStepKind, enum_STEPUNIT eStepUnit)
		{
			switch (eStepKind)
			{
				case enum_STEPKIND.STEP_BACKWARDS: // Not supported.
					break;
				case enum_STEPKIND.STEP_INTO:
					m_Conn.SendToAll(Message.Create(ServerTag.StepInto));
					break;
				case enum_STEPKIND.STEP_OUT:
					m_Conn.SendToAll(Message.Create(ServerTag.StepOut));
					break;
				case enum_STEPKIND.STEP_OVER:
					m_Conn.SendToAll(Message.Create(ServerTag.StepOver));
					break;
			}
		}
	}
}
