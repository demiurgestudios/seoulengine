// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using Microsoft.VisualStudio.Shell.Flavor;
using System;
using System.Runtime.InteropServices;

namespace SlimCSVS.Package.projects
{
	[Guid(debug.Constants.ksFactoryGuid)]
	public sealed class ProgramProjectFactory : FlavoredProjectFactoryBase
	{
		#region Private members
		readonly Package m_Package;
		#endregion

		#region Overrides
		protected override object PreCreateForOuter(IntPtr pOuterProjectIUnknown)
		{
			return new ProgramProjectFlavor(m_Package);
		}
		#endregion

		public ProgramProjectFactory(Package package)
		{
			m_Package = package;
		}
	}
}
