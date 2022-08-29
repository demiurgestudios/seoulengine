// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using Microsoft.VisualStudio;
using Microsoft.VisualStudio.Debugger.Interop;

namespace SlimCSVS.Package.debug.ad7
{
	sealed class Expression : IDebugExpression2
	{
		#region Private members
		readonly Engine m_Engine;
		readonly Thread m_Thread;
		readonly StackFrame m_Frame;
		readonly string m_sPath;

		#region IDebugExpression2 overrides
		int IDebugExpression2.EvaluateAsync(
			enum_EVALFLAGS dwFlags,
			IDebugEventCallback2 pExprCallback)
		{
			return VSConstants.E_NOTIMPL;
		}

		int IDebugExpression2.Abort()
		{
			return VSConstants.S_FALSE;
		}

		int IDebugExpression2.EvaluateSync(
			enum_EVALFLAGS dwFlags,
			uint dwTimeout,
			IDebugEventCallback2 pExprCallback,
			out IDebugProperty2 ppResult)
		{
			ppResult = null;

			(var v, var b) = m_Thread.GetVariableValue(m_Frame, m_sPath);
			if (!b)
			{
				// If we failed to the value of a local, we're done.
				int i = m_sPath.LastIndexOf('.');
				if (i < 0)
				{
					return VSConstants.S_FALSE;
				}

				// Get the children, then search those for the last part.
				var sParent = m_sPath.Substring(0, i);
				if (!m_Engine.RequestChildren(m_Frame.StackDepth, sParent, new System.TimeSpan(0, 0, 0, 0, (int)dwTimeout)))
				{
					return VSConstants.S_FALSE;
				}

				// Try again.
				(v, b) = m_Thread.GetVariableValue(m_Frame, m_sPath);
				if (!b)
				{
					return VSConstants.S_FALSE;
				}
			}

			ppResult = new Property(null, m_Engine, m_Thread, m_Frame, m_sPath, m_sPath, v);
			return VSConstants.S_OK;
		}
		#endregion
		#endregion

		public Expression(
			Engine engine,
			Thread thread,
			StackFrame frame,
			string sPath)
		{
			m_Engine = engine;
			m_Thread = thread;
			m_Frame = frame;
			m_sPath = sPath;
		}
	}
}
