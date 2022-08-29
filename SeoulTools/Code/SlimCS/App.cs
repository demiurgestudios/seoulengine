// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.Globalization;
using System.Threading;
using SlimCSLib;
using SlimCSLib.Compilation;
using SlimCSLib.Testing;

namespace SlimCS
{
	/// <summary>
	/// Root class and entry point of SlimCS.
	/// </summary>
	///
	/// <remarks>
	/// SlimCS converts C# "lite" code to equivalent Lua code.
	///
	/// It is intended for usage as a game scripting language.
	///
	/// As a result, SlimCS deliberately implements a strict C# subset,
	/// to minimize "surprises" in the generated Lua code.
	/// </remarks>
	static class App
	{
		public static int Main(string[] asArgs)
		{
			CommandArgs args = default(CommandArgs);
			try
			{
				args = new CommandArgs(asArgs);
			}
			catch (HelpArgException)
			{
				return 0;
			}
			catch (InvalidArgException e)
			{
				ConsoleWrapper.Error.WriteLine(e.Message);
				return 1;
			}
			catch (Exception e)
			{
				ConsoleWrapper.Error.WriteLine("Unexpected exception, likely, a compiler bug.");
				ConsoleWrapper.Error.WriteLine(e.ToString());
				return 1;
			}

			try
			{
				// Apply the culture setting now if requested.
				if (!string.IsNullOrEmpty(args.m_sTestCulture))
				{
					var culture = new CultureInfo(args.m_sTestCulture);
					Thread.CurrentThread.CurrentCulture = culture;
					Thread.CurrentThread.CurrentUICulture = culture;
					CultureInfo.DefaultThreadCurrentCulture = culture;
					CultureInfo.DefaultThreadCurrentUICulture = culture;
				}

				// We're the server daemon, run.
				if (!string.IsNullOrEmpty(args.m_sDaemonMonitorDir))
				{
					Service.ServerDaemonMain(args);
				}
				else if (args.m_bUnitTest)
				{
					UnitTester.RunUnitTests(args);
				}
				else if (args.m_bNoDaemon)
				{
					Pipeline.ClBuild(
						ConsoleWrapper.Out.WriteLine,
						ConsoleWrapper.Error.WriteLine,
						args,
						false,
						false);
				}
				else
				{
					return Service.DaemonBuild(args) ? 0 : 1;
				}
			}
			catch (Exception e) when
				(e is SlimCSCompilationException ||
				e is CSharpCompilationException ||
				e is TestRunException ||
				e is UnsupportedNodeException)
			{
				ConsoleWrapper.Error.WriteLine(e.Message);
				return 1;
			}
			catch (Exception e)
			{
				ConsoleWrapper.Error.WriteLine("Unexpected exception, likely, a compiler bug.");
				ConsoleWrapper.Error.WriteLine(e.ToString());
				return 1;
			}

			return 0;
		}
	}
}
