// Enum of Platform targets supported by SeoulEngine, and
// some related utilities.
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
	/// Enum value used to identify different platforms.
	/// </summary>
	public enum Platform
	{
		// NOTE: Take care when changing these - these values must be kept in-sync with
		// equivalent values in SeoulEngine (Core.h).
		PC = 0,
		IOS = 1,
		Android = 2,
        Linux = 3,

		Unknown = int.MaxValue,
	}; // enum Platform

	/// <summary>
	/// Utility functions for converting to/from string representations and their
	/// corresponding Platform enums.
	/// </summary>
	public static class PlatformUtility
	{
		#region Non-public members
		const string ksPlatformArgument = "platform";
		#endregion

		/// <returns>
		/// True if ePlatform is a little endian platform, false otherwise.
		/// </returns>
		public static bool IsLittleEndian(Platform ePlatform)
		{
			switch (ePlatform)
			{
			case Platform.Unknown: return false;
			case Platform.PC: return true;
			case Platform.IOS: return true;
			case Platform.Android: return true;
			default:
				throw new Exception("Invalid enum value: " + Enum.GetName(typeof(Platform), ePlatform));
			};
		}

		/// <returns>
		/// True if ePlatform is a big endian platform, false otherwise.
		/// </returns>
		public static bool IsBigEndian(Platform ePlatform)
		{
			return !IsLittleEndian(ePlatform);
		}


		/// <returns>
		/// The string sPlatformString as a Platform enum, or
		/// the Unknown platform if sPlatformString does not
		/// match an expected platform string.
		/// </returns>
		public static Platform ToPlatform(string sPlatformString)
		{
			string[] asNames = Enum.GetNames(typeof(Platform));
			Platform[] aeValues = (Platform[])Enum.GetValues(typeof(Platform));
			int iNames = asNames.Length;
			for (int i = 0; i < iNames; ++i)
			{
				if (0 == String.Compare(sPlatformString, asNames[i], true))
				{
					return aeValues[i];
				}
			}

			return Platform.Unknown;
		}

		/// <returns>
		/// A Platform as specified in the command-line arguments
		/// in commandLine, or Unknown if no platform is explicitly
		/// specified.
		/// </returns>
		public static Platform ToPlatform(CommandLine commandLine)
		{
			string sPlatformString = commandLine[ksPlatformArgument];
			if (!string.IsNullOrEmpty(sPlatformString))
			{
				return ToPlatform(sPlatformString);
			}
			else
			{
				return Platform.Unknown;
			}
		}

		/// <summary>
		/// Canonical conversion of ePlatform to a string representation
		/// </summary>
		public static string ToString(Platform ePlatform)
		{
			return Enum.GetName(typeof(Platform), ePlatform);
		}
	} // class PlatformUtility
}
