// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using Microsoft.VisualStudio;
using Microsoft.VisualStudio.Shell.Interop;
using System;
using System.IO;

namespace SlimCSVS.Package.projects
{
	public static class ProgramExtensions
	{
		public static IVsSolution GetSolution(this IServiceProvider self)
		{
			return (IVsSolution)self.GetService(typeof(SVsSolution));
		}

		public static bool GetSolutionDirectory(this IServiceProvider self, out string sDirectory)
		{
			var sln = self.GetSolution();
			return (VSConstants.S_OK == sln.GetSolutionInfo(out sDirectory, out _, out _));
		}

		public static bool GetSolutionFile(this IServiceProvider self, out string sFile)
		{
			var sln = self.GetSolution();
			return (VSConstants.S_OK == sln.GetSolutionInfo(out _, out sFile, out _));
		}

		public static bool GetSolutionName(this IServiceProvider self, out string sName)
		{
			string sFile;
			if (self.GetSolutionFile(out sFile))
			{
				sName = Path.GetFileNameWithoutExtension(sFile);
				return true;
			}

			sName = string.Empty;
			return false;
		}
	}
}
