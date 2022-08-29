// Utilities class for interfacing with an Android device.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

namespace Moriarty.Utilities
{
	public static class AndroidUtilities
	{
		public const string kAdbDevicesCommandlineArgument = "devices";
		public const int kiLaunchTimeoutInMilliseconds = 3000;

		#region Private members
		static string EncodeArguments(string sPackageName, List<string> arguments)
		{
			StringBuilder builder = new StringBuilder();
			builder.Append('[');
			for (int i = 0; i < arguments.Count; ++i)
			{
				if (i != 0)
				{
					builder.Append(',');
					builder.Append(' ');
				}

				builder.Append('"');
				builder.Append(arguments[i]);
				builder.Append('"');
			}
			builder.Append(']');

			return "commandline " + Uri.EscapeDataString(builder.ToString());
		}

        static string GetStopArguments(string sPackageName)
        {
            return "shell am force-stop " + sPackageName;
        }

		static string GetLaunchArguments(
			string sPackageName,
			string sActivityName,
			List<string> arguments,
			bool bWaitForDebugger)
		{
			return "shell am start " +
				(bWaitForDebugger ? "-W -D " : "-W ") +
				(arguments.Count > 0 ? ("-a android.intent.action.MAIN -e " + EncodeArguments(sPackageName, arguments) + " ") : "") +
				sPackageName + "/" + sActivityName;
		}
		#endregion

		public static void LaunchAPK(
			string sPackageName,
			string sActivityName,
			List<string> arguments,
			bool bWaitForDebugger)
		{
			StringBuilder messages = new StringBuilder();

			// Must start the server, otherwise the install or launch calls can hang.
			StartAdbServer();
	
            // Stop the process if it's already running - ignore the return in case it's not.
            CoreLib.Utilities.RunCommandLineProcess(
                AdbPath,
                GetStopArguments(sPackageName),
                null,
                null);

			// Launch the process.
			if (!CoreLib.Utilities.RunCommandLineProcess(
				AdbPath,
				GetLaunchArguments(sPackageName, sActivityName, arguments, bWaitForDebugger),
				delegate(string s)
				{
					lock (messages)
					{
						if (!string.IsNullOrWhiteSpace(s))
						{
							messages.Append(s);
							messages.Append('\n');
						}
					}
				},
				delegate(string s)
				{
					lock (messages)
					{
						if (!string.IsNullOrWhiteSpace(s))
						{
							messages.Append(s);
							messages.Append('\n');
						}
					}
				},
				null,
				null,
				kiLaunchTimeoutInMilliseconds))
			{
				throw new Exception(messages.ToString());
			}
		}

		public static void StartAdbServer()
		{
			Process process = Process.Start(new ProcessStartInfo()
				{
					Arguments = "start-server",
					CreateNoWindow = true,
					FileName = AdbPath,
					UseShellExecute = true,
					WindowStyle = ProcessWindowStyle.Hidden,
				});
			process.WaitForExit();
			process.Close();
		}

		public static string AdbPath
		{
			get
			{
				string sReturn = "adb.exe";
				try
				{
                    sReturn = Path.Combine(
                        AppDomain.CurrentDomain.BaseDirectory,
                        "Android\\adb.exe");
				}
				catch (Exception)
				{
				}

				return sReturn;
			}
		}
	}
}
