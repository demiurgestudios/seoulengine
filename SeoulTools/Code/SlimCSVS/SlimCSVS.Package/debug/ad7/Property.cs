// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using Microsoft.VisualStudio;
using Microsoft.VisualStudio.Debugger.Interop;
using System.Collections.Generic;

namespace SlimCSVS.Package.debug.ad7
{
	sealed class Property : IDebugProperty2
	{
		#region Private members
		readonly Property m_Parent;
		readonly Engine m_Engine;
		readonly Thread m_Thread;
		readonly StackFrame m_Frame;
		readonly string m_sPath;
		readonly string m_sDisplayName;
		Variable m_Var;
		#endregion

		#region IDebugProperty2
		int IDebugProperty2.EnumChildren(
			enum_DEBUGPROP_INFO_FLAGS dwFields,
			uint dwRadix,
			ref System.Guid guidFilter,
			enum_DBG_ATTRIB_FLAGS dwAttribFilter,
			string pszNameFilter,
			uint dwTimeout,
			out IEnumDebugPropertyInfo2 ppEnum)
		{
			ppEnum = null;

			// Nothing to do if no children.
			if (!m_Var.HasChildren)
			{
				return VSConstants.S_FALSE;
			}

			// Refresh variable information.
			if (!m_Engine.RequestChildren(m_Frame.StackDepth, m_sPath, new System.TimeSpan(0, 0, 0, 0, (int)dwTimeout)))
			{
				return VSConstants.S_FALSE;
			}

			// Get variables for the given path.
			var vars = m_Thread.GetVariableChildren(m_Frame.StackDepth, m_sPath);

			// Sanitize.
			if (null == vars)
			{
				vars = new List<Variable>();
			}

			// Done, populate.
			var a = StackFrame.Gather(
				dwFields,
				this,
				m_Engine,
				m_Thread,
				m_Frame,
				m_sPath,
				vars);

			ppEnum = new PropertyEnum(a);

			return VSConstants.S_OK;
		}

		int IDebugProperty2.GetDerivedMostProperty(out IDebugProperty2 ppDerivedMost)
		{
			ppDerivedMost = null;
			return VSConstants.E_NOTIMPL;
		}

		int IDebugProperty2.GetExtendedInfo(ref System.Guid guidExtendedInfo, out object pExtendedInfo)
		{
			pExtendedInfo = null;
			return VSConstants.E_NOTIMPL;
		}

		int IDebugProperty2.GetMemoryBytes(out IDebugMemoryBytes2 ppMemoryBytes)
		{
			ppMemoryBytes = null;
			return VSConstants.E_NOTIMPL;
		}

		int IDebugProperty2.GetMemoryContext(out IDebugMemoryContext2 ppMemory)
		{
			ppMemory = null;
			return VSConstants.E_NOTIMPL;
		}

		int IDebugProperty2.GetParent(out IDebugProperty2 ppParent)
		{
			ppParent = m_Parent;
			return (null != ppParent ? VSConstants.S_OK : VSConstants.S_FALSE);
		}

		int IDebugProperty2.GetPropertyInfo(
			enum_DEBUGPROP_INFO_FLAGS dwFields,
			uint dwRadix,
			uint dwTimeout,
			IDebugReference2[] rgpArgs,
			uint dwArgCount,
			DEBUG_PROPERTY_INFO[] pPropertyInfo)
		{
			pPropertyInfo[0] = ToDebugPropertyInfo(dwFields);
			rgpArgs = null;
			return VSConstants.S_OK;
		}

		int IDebugProperty2.GetReference(out IDebugReference2 ppReference)
		{
			ppReference = null;
			return VSConstants.E_NOTIMPL;
		}

		int IDebugProperty2.GetSize(out uint pdwSize)
		{
			pdwSize = 0;
			return VSConstants.E_NOTIMPL;
		}

		int IDebugProperty2.SetValueAsReference(IDebugReference2[] rgpArgs, uint dwArgCount, IDebugReference2 pValue, uint dwTimeout)
		{
			return VSConstants.E_NOTIMPL;
		}

		int IDebugProperty2.SetValueAsString(string pszValue, uint dwRadix, uint dwTimeout)
		{
			var bResult = m_Engine.RequestVariableSet(m_Frame, m_sPath, m_Var.m_eType, pszValue, new System.TimeSpan(0, 0, 0, 0, (int)dwTimeout));
			if (bResult)
			{
				m_Var.m_sValue = pszValue;
			}
			return (bResult ? VSConstants.S_OK : VSConstants.S_FALSE);
		}
		#endregion

		public Property(
			Property parent,
			Engine engine,
			Thread thread,
			StackFrame frame,
			string sPath,
			string sDisplayName,
			Variable v)
		{
			m_Parent = parent;
			m_Engine = engine;
			m_Thread = thread;
			m_Frame = frame;
			m_sPath = sPath;
			m_sDisplayName = sDisplayName;
			m_Var = v;
		}

		public DEBUG_PROPERTY_INFO ToDebugPropertyInfo(enum_DEBUGPROP_INFO_FLAGS dwFields)
		{
			var ret = new DEBUG_PROPERTY_INFO();
			if ((dwFields & enum_DEBUGPROP_INFO_FLAGS.DEBUGPROP_INFO_FULLNAME) != 0)
			{
				ret.bstrFullName = m_sPath;
				ret.dwFields |= enum_DEBUGPROP_INFO_FLAGS.DEBUGPROP_INFO_FULLNAME;
			}

			if ((dwFields & enum_DEBUGPROP_INFO_FLAGS.DEBUGPROP_INFO_NAME) != 0)
			{
				ret.bstrName = m_sDisplayName;
				ret.dwFields |= enum_DEBUGPROP_INFO_FLAGS.DEBUGPROP_INFO_NAME;
			}

			if ((dwFields & enum_DEBUGPROP_INFO_FLAGS.DEBUGPROP_INFO_TYPE) != 0)
			{
				ret.bstrType = (string.IsNullOrEmpty(m_Var.m_sExtendedType) ? m_Var.m_eType.ToDisplayName() : m_Var.m_sExtendedType);
				ret.dwFields |= enum_DEBUGPROP_INFO_FLAGS.DEBUGPROP_INFO_TYPE;
			}

			if ((dwFields & enum_DEBUGPROP_INFO_FLAGS.DEBUGPROP_INFO_VALUE) != 0)
			{
				ret.bstrValue = m_Var.m_sValue;
				ret.dwFields |= enum_DEBUGPROP_INFO_FLAGS.DEBUGPROP_INFO_VALUE;
			}

			if ((dwFields & enum_DEBUGPROP_INFO_FLAGS.DEBUGPROP_INFO_ATTRIB) != 0)
			{
				if (m_Var.HasChildren)
				{
					ret.dwAttrib |= enum_DBG_ATTRIB_FLAGS.DBG_ATTRIB_OBJ_IS_EXPANDABLE;
				}
				if (m_Var.m_eType == VariableType.Boolean)
				{
					ret.dwAttrib |= enum_DBG_ATTRIB_FLAGS.DBG_ATTRIB_VALUE_BOOLEAN;
				}
				if (m_Var.m_eType == VariableType.String)
				{
					ret.dwAttrib |= enum_DBG_ATTRIB_FLAGS.DBG_ATTRIB_VALUE_RAW_STRING;
				}
				if (m_Var.m_eType.IsReadOnly())
				{
					ret.dwAttrib |= enum_DBG_ATTRIB_FLAGS.DBG_ATTRIB_VALUE_READONLY;
				}
			}

			if (((dwFields & enum_DEBUGPROP_INFO_FLAGS.DEBUGPROP_INFO_PROP) != 0) || m_Var.HasChildren)
			{
				ret.pProperty = this;
				ret.dwFields |= enum_DEBUGPROP_INFO_FLAGS.DEBUGPROP_INFO_PROP;
			}

			return ret;
		}
	}

	sealed class StackFrameProperty : IDebugProperty2
	{
		#region Private members
		readonly StackFrame m_Frame;
		#endregion

		#region IDebugProperty2
		int IDebugProperty2.EnumChildren(
			enum_DEBUGPROP_INFO_FLAGS dwFields,
			uint dwRadix,
			ref System.Guid guidFilter,
			enum_DBG_ATTRIB_FLAGS dwAttribFilter,
			string pszNameFilter,
			uint dwTimeout,
			out IEnumDebugPropertyInfo2 ppEnum)
		{
			return ((IDebugStackFrame2)m_Frame).EnumProperties(
				dwFields,
				dwRadix,
				ref guidFilter,
				dwTimeout,
				out _,
				out ppEnum);
		}

		int IDebugProperty2.GetDerivedMostProperty(out IDebugProperty2 ppDerivedMost)
		{
			ppDerivedMost = null;
			return VSConstants.E_NOTIMPL;
		}

		int IDebugProperty2.GetExtendedInfo(ref System.Guid guidExtendedInfo, out object pExtendedInfo)
		{
			pExtendedInfo = null;
			return VSConstants.E_NOTIMPL;
		}

		int IDebugProperty2.GetMemoryBytes(out IDebugMemoryBytes2 ppMemoryBytes)
		{
			ppMemoryBytes = null;
			return VSConstants.E_NOTIMPL;
		}

		int IDebugProperty2.GetMemoryContext(out IDebugMemoryContext2 ppMemory)
		{
			ppMemory = null;
			return VSConstants.E_NOTIMPL;
		}

		int IDebugProperty2.GetParent(out IDebugProperty2 ppParent)
		{
			ppParent = null;
			return VSConstants.S_OK;
		}

		int IDebugProperty2.GetPropertyInfo(
			enum_DEBUGPROP_INFO_FLAGS dwFields,
			uint dwRadix,
			uint dwTimeout,
			IDebugReference2[] rgpArgs,
			uint dwArgCount,
			DEBUG_PROPERTY_INFO[] pPropertyInfo)
		{
			pPropertyInfo[0] = new DEBUG_PROPERTY_INFO();
			rgpArgs = null;
			return VSConstants.E_NOTIMPL;
		}

		int IDebugProperty2.GetReference(out IDebugReference2 ppReference)
		{
			ppReference = null;
			return VSConstants.E_NOTIMPL;
		}

		int IDebugProperty2.GetSize(out uint pdwSize)
		{
			pdwSize = 0;
			return VSConstants.E_NOTIMPL;
		}

		int IDebugProperty2.SetValueAsReference(IDebugReference2[] rgpArgs, uint dwArgCount, IDebugReference2 pValue, uint dwTimeout)
		{
			return VSConstants.E_NOTIMPL;
		}

		int IDebugProperty2.SetValueAsString(string pszValue, uint dwRadix, uint dwTimeout)
		{
			return VSConstants.E_NOTIMPL;
		}
		#endregion

		public StackFrameProperty(StackFrame frame)
		{
			m_Frame = frame;
		}
	}
}
