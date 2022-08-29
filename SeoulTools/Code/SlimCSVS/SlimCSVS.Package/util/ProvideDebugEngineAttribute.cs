// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using Microsoft.VisualStudio.Shell;
using System;
using System.IO;

namespace SlimCSVS.Package.util
{
	public sealed class ProvideDebugEngineAttribute : RegistrationAttribute
	{
		#region Private members
		readonly string m_sId;
		readonly string m_sName;
		readonly Type m_DebugEngine;
		readonly Type m_ProgramProvider;
		#endregion

		public ProvideDebugEngineAttribute(
			Type programProvider,
			Type debugEngine,
			string sName,
			string sId)
		{
			m_sName = sName;
			m_ProgramProvider = programProvider;
			m_DebugEngine = debugEngine;
			m_sId = sId;
		}

		/// <summary>
		/// Register this debug engine with the VS registry.
		/// Most of this is boilerplate from the Microsoft
		/// debug engine sample and various other sources.
		/// </summary>
		/// <param name="context">Context provided by the VS engine.</param>
		public override void Register(RegistrationContext context)
		{
			{
				var root = context.CreateKey("AD7Metrics\\Engine\\{" + m_sId + "}");

				// Configuration.
				root.SetValue("AddressBP", 0); // Breakpoints are address based (vs. line number and file).
				root.SetValue("AlwaysLoadProgramProviderLocal", 1);
				root.SetValue("Attach", 0); // Supports attaching.
				root.SetValue("AutoSelectPriority", 4);
				root.SetValue("CallstackBP", 1); // Breakpoints should fill in a callstack.
				root.SetValue("CLSID", m_DebugEngine.GUID.ToString("B"));
				root.SetValue("EngineAssembly", m_DebugEngine.Assembly.FullName);
				root.SetValue("EngineClass", m_DebugEngine.FullName);
				root.SetValue("Exceptions", 0);
				root.SetValue("HitCountBP", 0);
				root.SetValue("LoadProgramProviderUnderWOW64", 1);
				root.SetValue("LoadUnderWOW64", 1);
				root.SetValue("Name", m_sName);
				root.SetValue("PortSupplier", "{708C1ECA-FF48-11D2-904F-00C04FA302A1}");
				root.SetValue("ProgramProvider", m_ProgramProvider.GUID.ToString("B"));
				root.SetValue("RemoteDebugging", 0); // Our support is entirely server base, so we don't expose this explicitly.
				root.SetValue("SetNextStatement", 0);

				using (var incompat = root.CreateSubkey("IncompatibleList"))
				{
					incompat.SetValue("guidCOMPlusNativeEng", "{92EF0900-2251-11D2-B72E-0000F87572EF}");
					incompat.SetValue("guidCOMPlusOnlyEng", "{449EC4CC-30D2-4032-9256-EE18EB41B62B}");
					incompat.SetValue("guidNativeOnlyEng", "{3B476D35-A401-11D2-AAD4-00C04F990171}");
					incompat.SetValue("guidScriptEng", "{F200A7E7-DEA5-11D0-B854-00A0244A1DE2}");
				}
			}

			// Class ID config.
			{
				var root = context.CreateKey("CLSID");

				// Boilerplate.
				using (var sub = root.CreateSubkey(m_DebugEngine.GUID.ToString("B")))
				{
					sub.SetValue("Assembly", m_DebugEngine.Assembly.FullName);
					sub.SetValue("Class", m_DebugEngine.FullName);
					sub.SetValue("CodeBase", Path.Combine(context.ComponentPath, m_DebugEngine.Module.Name));
					sub.SetValue("InprocServer32", context.InprocServerPath);
					sub.SetValue("ThreadingModel", "Free");
				}

				// Boilerplate
				using (var sub = root.CreateSubkey(m_ProgramProvider.GUID.ToString("B")))
				{
					sub.SetValue("Assembly", m_ProgramProvider.Assembly.FullName);
					sub.SetValue("Class", m_ProgramProvider.FullName);
					sub.SetValue("CodeBase", Path.Combine(context.ComponentPath, m_DebugEngine.Module.Name));
					sub.SetValue("InprocServer32", context.InprocServerPath);
					sub.SetValue("ThreadingModel", "Free");
				}
			}
		}

		public override void Unregister(RegistrationContext context)
		{
			// Nop.
		}
	}
}
