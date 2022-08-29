// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using Microsoft.VisualStudio.Shell;
using System.Runtime.InteropServices;
using SlimCSVS.Package.debug;
using SlimCSVS.Package.projects;
using SlimCSVS.Package.util;

namespace SlimCSVS.Package
{
	[Guid(Constants.ksPackageGuid)]
	[InstalledProductRegistration("#110", "#111", "0.1.5", IconResourceID = 400)] // Info on this package for Help/About
	[PackageRegistration(UseManagedResourcesOnly = true)]
	[ProvideDebugEngine(typeof(ProgramProvider), typeof(debug.ad7.Engine), Constants.ksEngineName, Constants.ksEngineGuid)]
	[ProvideProjectFactory(typeof(ProgramProjectFactory), "CSharp", "C# SlimCS Project (*.csproj)", "csproj", "csproj", @".\NullPath", LanguageVsTemplate = "CSharp", ProjectSubTypeVsTemplate = "CSharp", DisableOnlineTemplates = true)]
	public sealed class Package : Microsoft.VisualStudio.Shell.Package
	{
		protected override void Initialize()
		{
			base.Initialize();
			RegisterProjectFactory(new ProgramProjectFactory(this));
		}
	}
}
