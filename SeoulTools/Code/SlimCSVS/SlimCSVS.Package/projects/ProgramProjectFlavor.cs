// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using Microsoft.VisualStudio;
using Microsoft.VisualStudio.Shell.Flavor;
using Microsoft.VisualStudio.Shell.Interop;

namespace SlimCSVS.Package.projects
{
	public sealed class ProgramProjectFlavor : FlavoredProjectBase, IVsProjectFlavorCfgProvider
	{
		#region Private members
		static IServiceProvider s_GlobalServiceProvider;
		#endregion

		#region Overrides
		public int CreateProjectFlavorCfg(IVsCfg pBaseProjectCfg, out IVsProjectFlavorCfg ppFlavorCfg)
		{
			ppFlavorCfg = new ProgramFlavorCfg(this);
			return VSConstants.S_OK;
		}
		#endregion

		public ProgramProjectFlavor(Package package)
		{
			Package = package;
			serviceProvider = package; // Must be set or project is invalid.
			s_GlobalServiceProvider = serviceProvider;
		}

		public static IServiceProvider GlobalServiceProvider { get { return s_GlobalServiceProvider; } }

		public Package Package { get; }
		internal IServiceProvider ServiceProvider { get { return serviceProvider; } }
	}
}
