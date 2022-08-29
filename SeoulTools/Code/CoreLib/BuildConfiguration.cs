// Equivalent to SEOUL_DEBUG, SEOUL_DEVELOPER, and SEOUL_SHIP
// in the SeoulEngine codebase.
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
	/// <summary>
	/// Enum identifying different build configurations - used
	/// when cooking some types of content that can be build specific.
	/// </summary>
	public enum BuildConfiguration
	{
		// NOTE: Take care when changing these - these values must be kept in-sync with
		// equivalent values in SeoulEngine.
		Debug = 0,
		Developer = 1,
		Ship = 2,

		Unknown = int.MaxValue,
	}; // enum BuildConfiguration

	/// <summary>
	/// Utility functions for converting to/from string representations
	/// and their corresponding build config enums.
	/// </summary>
	public static class BuildConfigurationUtility
	{
		#region Non-public members
		const string ksBuildConfigurationArgument = "build";
		#endregion

		/// <returns>
		/// The string sBuildConfigurationString as a BuildConfiguration enum, or
		/// the Unknown build configuration if sBuildConfigurationString does not
		/// match an expected build configuration string.
		/// </returns>
		public static BuildConfiguration ToBuildConfiguration(string sBuildConfigurationString)
		{
			string[] asNames = Enum.GetNames(typeof(BuildConfiguration));
			BuildConfiguration[] aeValues = (BuildConfiguration[])Enum.GetValues(typeof(BuildConfiguration));
			int iNames = asNames.Length;
			for (int i = 0; i < iNames; ++i)
			{
				if (0 == String.Compare(sBuildConfigurationString, asNames[i], true))
				{
					return aeValues[i];
				}
			}

			return BuildConfiguration.Unknown;
		}

		/// <returns>
		/// A build configuration as specified in the command-line arguments
		/// in commandLine, or Unknown if no build configuration is explicitly
		/// specified.
		/// </returns>
		public static BuildConfiguration ToBuildConfiguration(CommandLine commandLine)
		{
			string sBuildConfigurationString = commandLine[ksBuildConfigurationArgument];
			if (!String.IsNullOrEmpty(sBuildConfigurationString))
			{
				return ToBuildConfiguration(sBuildConfigurationString);
			}
			else
			{
				return BuildConfiguration.Unknown;
			}
		}

		/// <returns>
		/// A string representation of the BuildConfiguration enum eBuildConfiguration.
		/// </returns>
		public static string ToString(BuildConfiguration eBuildConfiguration)
		{
			return Enum.GetName(typeof(BuildConfiguration), eBuildConfiguration);
		}
	}; // class BuildConfigurationUtility
} // namespace CoreLib
