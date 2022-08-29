// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using Microsoft.VisualStudio;
using Microsoft.VisualStudio.OLE.Interop;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Shell.Interop;
using System;
using System.Runtime.InteropServices;

namespace SlimCSVS.Package.projects
{
	public sealed class ProgramFlavorCfg : IVsDebuggableProjectCfg2, IVsProjectFlavorCfg
	{
		#region Private members
		readonly ProgramProjectFlavor m_Project;
		#endregion

		#region Overrides
		#region IVsDebuggableProjectCfg2
		public int DebugLaunch(uint grfLaunch)
		{
			var info = new VsDebugTargetInfo();
			info.bstrExe = "SlimCS";
			info.cbSize = (uint)Marshal.SizeOf(info);
			info.clsidCustom = debug.Constants.kEngineGuid;
			info.dlo = DEBUG_LAUNCH_OPERATION.DLO_Custom;
			info.grfLaunch = (uint)(__VSDBGLAUNCHFLAGS.DBGLAUNCH_WaitForAttachComplete | __VSDBGLAUNCHFLAGS.DBGLAUNCH_DetachOnStop);

			VsShellUtilities.LaunchDebugger(
				m_Project.ServiceProvider,
				info);

			return VSConstants.S_OK;
		}

		[Obsolete] public int EnumOutputs(out IVsEnumOutputs ppIVsEnumOutputs) { ppIVsEnumOutputs = null; return VSConstants.S_FALSE; }
		public int get_BuildableProjectCfg(out IVsBuildableProjectCfg ppIVsBuildableProjectCfg) { throw new NotImplementedException(); }
		public int get_CanonicalName(out string pbstrCanonicalName) { throw new NotImplementedException(); }
		public int get_DisplayName(out string pbstrDisplayName) { throw new NotImplementedException(); }
		[Obsolete] public int get_IsDebugOnly(out int pfIsDebugOnly) { pfIsDebugOnly = 0; return VSConstants.S_FALSE; }
		[Obsolete] public int get_IsPackaged(out int pfIsPackaged) { pfIsPackaged = 0; return VSConstants.S_FALSE; }
		[Obsolete] public int get_IsReleaseOnly(out int pfIsReleaseOnly) { pfIsReleaseOnly = 0; return VSConstants.S_FALSE; }
		[Obsolete] public int get_IsSpecifyingOutputSupported(out int pfIsSpecifyingOutputSupported) { pfIsSpecifyingOutputSupported = 0; return VSConstants.S_FALSE; }
		[Obsolete] public int get_Platform(out Guid pguidPlatform) { pguidPlatform = default(Guid); return VSConstants.S_FALSE; }
		[Obsolete] public int get_ProjectCfgProvider(out IVsProjectCfgProvider ppIVsProjectCfgProvider) { ppIVsProjectCfgProvider = null; return VSConstants.S_FALSE; }
		public int get_RootURL(out string pbstrRootURL) { throw new NotImplementedException(); }
		[Obsolete] public int get_TargetCodePage(out uint puiTargetCodePage) { puiTargetCodePage = 0u; return VSConstants.S_FALSE; }
		[Obsolete] public int get_UpdateSequenceNumber(ULARGE_INTEGER[] puliUSN) { return VSConstants.S_FALSE; }
		int IVsDebuggableProjectCfg2.OnBeforeDebugLaunch(uint grfLaunch) { return VSConstants.S_OK; }
		[Obsolete] public int OpenOutput(string szOutputCanonicalName, out IVsOutput ppIVsOutput) { throw new NotImplementedException(); }

		public int QueryDebugLaunch(uint grfLaunch, out int pfCanLaunch)
		{
			pfCanLaunch = 1;
			return VSConstants.S_OK;
		}
		#endregion

		#region IVsProjectFlavorCfg
		public int Close()
		{
			return VSConstants.S_OK;
		}

		public int get_CfgType(ref Guid cfgGuid, out IntPtr pOutConfig)
		{
			if (cfgGuid == typeof(IVsDebuggableProjectCfg2).GUID)
			{
				pOutConfig = Marshal.GetComInterfaceForObject(this, typeof(IVsDebuggableProjectCfg2));
				return VSConstants.S_OK;
			}
			else if (cfgGuid == typeof(IVsDebuggableProjectCfg).GUID)
			{
				pOutConfig = Marshal.GetComInterfaceForObject(this, typeof(IVsDebuggableProjectCfg));
				return VSConstants.S_OK;
			}

			pOutConfig = IntPtr.Zero;
			return VSConstants.E_FAIL;
		}
		#endregion
		#endregion

		public ProgramFlavorCfg(ProgramProjectFlavor project)
		{
			// Cache the project references.
			m_Project = project;
		}
	}
}
