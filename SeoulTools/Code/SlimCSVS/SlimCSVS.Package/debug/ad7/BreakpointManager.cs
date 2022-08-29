// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using Microsoft.VisualStudio.Debugger.Interop;
using SlimCSVS.Package.projects;
using System;
using System.Collections.Generic;
using System.IO;

namespace SlimCSVS.Package.debug.ad7
{
	sealed class BreakpointManager
	{
		#region Private members
		readonly Engine m_Engine;
		readonly string m_sDirectory;
		readonly List<PendingBreakpoint> m_PendingBreakpoints = new List<PendingBreakpoint>();
		readonly Dictionary<UInt32, BoundBreakpoint> m_BoundBreakpoints = new Dictionary<UInt32, BoundBreakpoint>();
		readonly Dictionary<string, UInt16> m_tFileLookup = new Dictionary<string, ushort>();
		readonly Dictionary<UInt16, string> m_tReverseFileLookup = new Dictionary<ushort, string>();
		UInt16 m_uNextFileId = 0;
		#endregion

		public BreakpointManager(Engine engine)
		{
			m_Engine = engine;
			ProgramProjectFlavor.GlobalServiceProvider.GetSolutionDirectory(out m_sDirectory);
		}

		public void CreatePendingBreakpoint(
			IDebugBreakpointRequest2 pBPRequest,
			out IDebugPendingBreakpoint2 ppPendingBP)
		{
			lock (this)
			{
				var pb = new PendingBreakpoint(m_Engine, this, pBPRequest);
				ppPendingBP = (IDebugPendingBreakpoint2)pb;
				m_PendingBreakpoints.Add(pb);
			}
		}

		public void ResolveFileAndLine(UInt32 uRawBreakpoint, out string sFileName, out Int32 iLine)
		{
			lock (this)
			{
				iLine = (Int32)((uRawBreakpoint >> 16) & 0x0000FFFF);
				UInt16 uFileId = (UInt16)(uRawBreakpoint & 0x0000FFFF);
				m_tReverseFileLookup.TryGetValue(uFileId, out sFileName);
			}
		}

		public UInt32 ResolveRawBreakpoint(string sFileName, Int32 iLine)
		{
			// Normalize the filename - make relative to project
			// folder, remove the extension, and convert to lowercase.
			if (Path.IsPathRooted(sFileName))
			{
				sFileName = util.Utilities.PathRelativeTo(m_sDirectory, sFileName);
			}

			// Make sure we have a proper extension.
			sFileName = Path.ChangeExtension(sFileName, ".cs");

			// Normalize the filename - remove extension and ToLower().
			var s = Path.ChangeExtension(sFileName, null);

			UInt16 uFileId = 0;
			lock (this)
			{
				if (!m_tFileLookup.TryGetValue(s, out uFileId))
				{
					uFileId = ++m_uNextFileId;
					m_tFileLookup.Add(s, uFileId);
					m_tReverseFileLookup.Add(uFileId, sFileName);
				}
			}

			var uBreakpoint = (UInt32)((((UInt32)iLine << 16) & 0xFFFF0000) | ((UInt32)uFileId & 0x0000FFFF));
			return uBreakpoint;
		}

		public Message FileIdMessage
		{
			get
			{
				var msg = Message.Create(ServerTag.SetBreakpoints);
				lock (this)
				{
					msg.Write(m_tFileLookup.Count);
					foreach (var t in m_tFileLookup)
					{
						msg.Write(t.Key);
						msg.Write(t.Value);
					}
				}
				return msg;
			}
		}

		public Message SetBreakpointsMessage
		{
			get
			{
				var msg = Message.Create(ServerTag.SetBreakpoints);
				lock (this)
				{
					msg.Write(m_tFileLookup.Count);
					foreach (var t in m_tFileLookup)
					{
						msg.Write(t.Key);
						msg.Write(t.Value);
					}
					foreach (var t in m_BoundBreakpoints)
					{
						var bp = t.Value;
						msg.Write((UInt32)bp.RawBreakpoint);
						msg.Write(bp.Valid);
					}
				}

				return msg;
			}
		}

		public void StoreBoundBreakpoint(BoundBreakpoint bp)
		{
			lock (this)
			{
				m_BoundBreakpoints[bp.RawBreakpoint] = bp;
			}
		}


		public bool TryGetBoundBreakpoint(UInt32 uRawBreakpoint, out BoundBreakpoint bp)
		{
			lock (this)
			{
				return m_BoundBreakpoints.TryGetValue(uRawBreakpoint, out bp);
			}
		}
	}
}
