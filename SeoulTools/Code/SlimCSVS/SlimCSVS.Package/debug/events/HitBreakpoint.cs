// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System.Collections.Generic;
using System.IO;

namespace SlimCSVS.Package.debug.events
{
	sealed class HitBreakpoint
	{
		#region Private members
		readonly List<StackFrame> m_Frames = new List<StackFrame>();
		#endregion

		public HitBreakpoint(List<StackFrame> frames)
		{
			m_Frames = frames;
		}

		public List<StackFrame> Frames { get { return m_Frames; } }

		public bool Send(
			SuspendReason eSuspendReason,
			ad7.Engine engine,
			ad7.BreakpointManager bpm)
		{
			if (m_Frames.Count == 0)
			{
				return false;
			}

			var top = m_Frames[0];

			// Handle based on the suspend type.
			if (eSuspendReason == SuspendReason.Step)
			{
				ad7.StepCompleteEvent.Send(engine);
				return true;
			}
			else
			{
				ad7.BoundBreakpoint bp = default(ad7.BoundBreakpoint);
				if (bpm.TryGetBoundBreakpoint(top.m_uRawBreakpoint, out bp))
				{
					return ad7.BreakpointEvent.Send(engine, bp);
				}
			}

			return false;
		}
	}
}
