// Global cooker constants and read-only values.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace CoreLib
{
	public static class Constants
	{
		#region Private members
		readonly static string[] s_kExtensions;
		readonly static HashSet<string> s_ktExtensions = new HashSet<string>();

		static Constants()
		{
			s_kExtensions = new string[]
			{
				".bank",
				".cs",
				".csp",
				".csproj",
				".csv",
				".fbx",
				".fcn",
				".fev",
				".fspro",
				".fx",
				".fxb",
				".fxc",
				".fxh",
				".fxh_marker",
				".html",
				".json",
				".lbc",
				".lua",
				".mtl",
				".pb",
				".pem",
				".png",
				".prefab",
				".proto",
				".saf",
				".sff",
				".sif0",
				".sif1",
				".sif2",
				".sif3",
				".sif4",
				".son",
				".spf",
				".ssa",
				".swf",
				".ttf",
				".txt",
				".wmv",
				".xfx",
			};

			foreach (var s in s_kExtensions)
			{
				s_ktExtensions.Add(s);
			}
		}
		#endregion

		/// <returns>
		/// An array listing all file extensions supported by
		/// SeoulEngine's runtime code.
		///
		/// All returned values are all lowercase and include the
		/// leading '.'.
		/// </returns>
		public static string[] GetSeoulEngineSupportedExtensions()
		{
			return s_kExtensions;
		}

		/// <returns>
		/// true if the given extension is a valid SeoulEngine extension
		/// </returns>
		public static bool IsValidSeoulEngineExtension(string s)
		{
			s = s.ToLower();
            if (string.IsNullOrEmpty(s) || s[0] != '.')
			{
				s = '.' + s;
			}

			return s_ktExtensions.Contains(s);
		}
	} // class Constants
} // namespace CoreLib
