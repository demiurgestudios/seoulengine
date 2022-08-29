// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using Microsoft.VisualStudio;
using Microsoft.VisualStudio.Debugger.Interop;

namespace SlimCSVS.Package.debug.events
{
	class Enum<T, I> where I : class
	{
		#region Private members
		readonly T[] m_Data;
		uint m_uPosition = 0;
		#endregion

		public Enum(T[] data)
		{
			m_Data = data;
		}

		public int Clone(out I ppEnum)
		{
			ppEnum = null;
			return VSConstants.E_NOTIMPL;
		}

		public int GetCount(out uint pcelt)
		{
			pcelt = (uint)m_Data.Length;
			return VSConstants.S_OK;
		}

		public int Next(uint celt, T[] rgelt, out uint celtFetched)
		{
			return Move(celt, rgelt, out celtFetched);
		}

		public int Reset()
		{
			lock (this)
			{
				m_uPosition = 0;

				return VSConstants.S_OK;
			}
		}

		public int Skip(uint celt)
		{
			uint celtFetched;
			return Move(celt, null, out celtFetched);
		}

		private int Move(uint celt, T[] rgelt, out uint celtFetched)
		{
			lock (this)
			{
				int hr = VSConstants.S_OK;
				celtFetched = (uint)m_Data.Length - m_uPosition;

				if (celt > celtFetched)
				{
					hr = VSConstants.S_FALSE;
				}
				else if (celt < celtFetched)
				{
					celtFetched = celt;
				}

				if (rgelt != null)
				{
					for (int c = 0; c < celtFetched; c++)
					{
						rgelt[c] = m_Data[m_uPosition + c];
					}
				}

				m_uPosition += celtFetched;

				return hr;
			}
		}
	}

	sealed class BoundBreakpointsEnum : Enum<IDebugBoundBreakpoint2, IEnumDebugBoundBreakpoints2>, IEnumDebugBoundBreakpoints2
	{
		public BoundBreakpointsEnum(IDebugBoundBreakpoint2[] breakpoints)
			: base(breakpoints)
		{

		}

		public int Next(uint celt, IDebugBoundBreakpoint2[] rgelt, ref uint celtFetched)
		{
			return Next(celt, rgelt, out celtFetched);
		}
	}

	sealed class CodeContextEnum : Enum<IDebugCodeContext2, IEnumDebugCodeContexts2>, IEnumDebugCodeContexts2
	{
		public CodeContextEnum(IDebugCodeContext2[] codeContexts)
			: base(codeContexts)
		{

		}

		public int Next(uint celt, IDebugCodeContext2[] rgelt, ref uint celtFetched)
		{
			return Next(celt, rgelt, out celtFetched);
		}
	}

	sealed class ErrorBreakpointsEnum : Enum<IDebugErrorBreakpoint2, IEnumDebugErrorBreakpoints2>, IEnumDebugErrorBreakpoints2
	{
		public ErrorBreakpointsEnum(IDebugErrorBreakpoint2[] breakpoints)
			: base(breakpoints)
		{

		}

		public int Next(uint celt, IDebugErrorBreakpoint2[] rgelt, ref uint celtFetched)
		{
			return Next(celt, rgelt, out celtFetched);
		}
	}

	sealed class FrameInfoEnum : Enum<FRAMEINFO, IEnumDebugFrameInfo2>, IEnumDebugFrameInfo2
	{
		public FrameInfoEnum(FRAMEINFO[] data)
			: base(data)
		{
		}

		public int Next(uint celt, FRAMEINFO[] rgelt, ref uint celtFetched)
		{
			return Next(celt, rgelt, out celtFetched);
		}
	}

	sealed class ModuleEnum : Enum<IDebugModule2, IEnumDebugModules2>, IEnumDebugModules2
	{
		public ModuleEnum(IDebugModule2[] modules)
			: base(modules)
		{

		}

		public int Next(uint celt, IDebugModule2[] rgelt, ref uint celtFetched)
		{
			return Next(celt, rgelt, out celtFetched);
		}
	}

	sealed class ProgramEnum : Enum<IDebugProgram2, IEnumDebugPrograms2>, IEnumDebugPrograms2
	{
		public ProgramEnum(IDebugProgram2[] data) : base(data)
		{
		}

		public int Next(uint celt, IDebugProgram2[] rgelt, ref uint celtFetched)
		{
			return Next(celt, rgelt, out celtFetched);
		}
	}

	sealed class PropertyEnum : Enum<DEBUG_PROPERTY_INFO, IEnumDebugPropertyInfo2>, IEnumDebugPropertyInfo2
	{
		public PropertyEnum(DEBUG_PROPERTY_INFO[] properties)
			: base(properties)
		{

		}
	}

	sealed class PropertyInfoEnum : Enum<DEBUG_PROPERTY_INFO, IEnumDebugPropertyInfo2>, IEnumDebugPropertyInfo2
	{
		public PropertyInfoEnum(DEBUG_PROPERTY_INFO[] data)
			: base(data)
		{
		}
	}

	sealed class ThreadEnum : Enum<IDebugThread2, IEnumDebugThreads2>, IEnumDebugThreads2
	{
		public ThreadEnum(IDebugThread2[] threads)
			: base(threads)
		{

		}

		public int Next(uint celt, IDebugThread2[] rgelt, ref uint celtFetched)
		{
			return Next(celt, rgelt, out celtFetched);
		}
	}
}
