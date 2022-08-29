// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Threading.Tasks;

namespace SlimCSLib
{
	public class HelpArgException : Exception
	{
	}

	public class InvalidArgException : Exception
	{
		protected static string HelpHint
		{
			get
			{
				var assembly = Assembly.GetExecutingAssembly();
				var nameInfo = assembly.GetName();
				var sName = nameInfo.Name.ToString();
				return $"(\"{sName} -h\" for help)";
			}
		}

		public InvalidArgException(string sArg)
			: base($"Unknown argument {sArg} {HelpHint}")
		{
			Arg = sArg;
		}

		public InvalidArgException(string sArg, string sMessage)
			: base(sMessage)
		{
			Arg = sArg;
		}

		public string Arg { get; set; }
	}

	public sealed class MissingInDirException : InvalidArgException
	{
		public MissingInDirException()
			: base(string.Empty, $"Missing input directory {HelpHint}")
		{
		}
	}

	public sealed class MissingValueArgException : InvalidArgException
	{
		public MissingValueArgException(string sArg)
			: base(sArg, $"Value is required after {sArg} {HelpHint}")
		{
		}
	}

	public sealed class MissingPathArgException : InvalidArgException
	{
		public MissingPathArgException(string sArg, string sPath)
			: base(sArg, $"{sArg}: Path does not exist \"{sPath}\"")
		{
		}
	}

	public struct CommandArgs
	{
		static void PrintHelp()
		{
			var assembly = Assembly.GetEntryAssembly();
			var nameInfo = assembly.GetName();
			var sName = nameInfo.Name.ToString();

			ConsoleWrapper.WriteLine($"usage:  {sName} [-v] [-h] [-D<name>] [--luac <exe>]");
			ConsoleWrapper.WriteLine("         [-p] [--doff] [-t] [--tcul]");
			ConsoleWrapper.WriteLine("         <input-dir> [<output-dir>]");
			ConsoleWrapper.WriteLine();
			ConsoleWrapper.WriteLine( " -v     Print the compiler's version.");
			ConsoleWrapper.WriteLine( " -h     Print this help message.");
			ConsoleWrapper.WriteLine( " -D     Define the given conditional compilation symbol.");
			ConsoleWrapper.WriteLine( "--luac  The lua compiler to use for unit tests.");
			ConsoleWrapper.WriteLine( " -p     # of threads to create for compile.");
			ConsoleWrapper.WriteLine( "--doff  Disable usage of a compilation daemon.");
			ConsoleWrapper.WriteLine( " -t     Treat input as compiler unit tests.");
			ConsoleWrapper.WriteLine( "--tcul  Override the SlimCS culture for testing. Implicitly enables --doff.");
		}

		public CommandArgs(string[] asArgs)
		{
			// Cache.
			OriginalArguments = asArgs;

			// Defaults.
			m_ConditionalCompilationSymbols = new HashSet<string>();
			m_sLuaCommand = string.Empty;
			int iHardMaxParallelism = Utilities.Min(Generator.Constants.kMaxParallelism, Environment.ProcessorCount);
			m_ParallelOptions = new ParallelOptions()
			{
				MaxDegreeOfParallelism = iHardMaxParallelism
			};
			m_sOutputSuffix = string.Empty;
			m_bUnitTest = false;
			m_bNoDaemon = false;
			m_bValidate = true;
			m_sInDirectory = string.Empty;
			m_sOutDirectory = string.Empty;
			m_sDaemonMonitorDir = string.Empty;
			m_bKill = false;
			m_sTestCulture = string.Empty;

			// State.
			var assembly = Assembly.GetExecutingAssembly();
			var nameInfo = assembly.GetName();
			var sName = nameInfo.Name.ToString();
			var sVersion = nameInfo.Version.ToString();

			// Gather arguments.
			for (int i = 0; i < asArgs.Length; ++i)
			{
				var s = asArgs[i];
				if (s.StartsWith("-"))
				{
					switch (s)
					{
						case "-v":
							ConsoleWrapper.WriteLine($"{sName} v{sVersion}");
							throw new HelpArgException();

						case "-h":
							PrintHelp();
							throw new HelpArgException();

						default:
							if (s.StartsWith("-D"))
							{
								m_ConditionalCompilationSymbols.Add(s.Substring(2));
								break;
							}
							else
							{
								throw new InvalidArgException(s);
							}

						case "--luac":
							++i;
							if (i >= asArgs.Length)
							{
								throw new MissingValueArgException(s);
							}

							m_sLuaCommand = asArgs[i];
							if (!File.Exists(m_sLuaCommand))
							{
								throw new MissingPathArgException(s, m_sLuaCommand);
							}
							break;

						case "-p":
							++i;
							if (i >= asArgs.Length)
							{
								throw new MissingValueArgException(s);
							}

							int iParallel = iHardMaxParallelism;
							if (!int.TryParse(asArgs[i], out iParallel))
							{
								throw new InvalidArgException(s, $"{s}: Max parallelism integer expected.");
							}

							m_ParallelOptions = new ParallelOptions()
							{
								MaxDegreeOfParallelism = Utilities.Clamp(iParallel, 1, iHardMaxParallelism)
							};
							break;

						case "--doff":
							m_bNoDaemon = true;
							break;

						case "--daemon":
							++i;
							if (i >= asArgs.Length)
							{
								throw new MissingValueArgException(s);
							}

							if (!Directory.Exists(asArgs[i]))
							{
								throw new InvalidArgException(s, $"{s}: Valid directory of client SlimCS.exe expected.");
							}

							m_sDaemonMonitorDir = Path.GetFullPath(asArgs[i]);
							break;

						case "-t":
							m_bUnitTest = true;
							break;

						case "--kill":
							m_bKill = true;
							break;

						case "--tcul":
							++i;
							if (i >= asArgs.Length)
							{
								throw new MissingValueArgException(s);
							}

							m_sTestCulture = asArgs[i];
							m_bNoDaemon = true;
							break;
					}
				}
				else if (string.IsNullOrEmpty(m_sInDirectory))
				{
					m_sInDirectory = s;
				}
				else if (string.IsNullOrEmpty(m_sOutDirectory))
				{
					m_sOutDirectory = s;
				}
				else
				{
					throw new InvalidArgException(s);
				}
			}

			// Timing is true unless we're a unit test setup.
			m_bTiming = !(m_bUnitTest);

			// Early out if we're a daemon or if this is a kill command.
			if (!string.IsNullOrEmpty(m_sDaemonMonitorDir) ||
				m_bKill)
			{
				return;
			}

			// No input directory, can't run.
			if (string.IsNullOrEmpty(m_sInDirectory))
			{
				// If 0 args, just print help.
				if (asArgs.Length == 0)
				{
					PrintHelp();
					throw new HelpArgException();
				}
				// Otherwise, complain about incorrect usage.
				else
				{
					throw new MissingInDirException();
				}
			}

			// If we get here and have no output directory,
			// use the input directory.
			if (string.IsNullOrEmpty(m_sOutDirectory))
			{
				m_sOutDirectory = m_sInDirectory;
			}

			// Now clean and normalize all paths.
			var curDir = Directory.GetCurrentDirectory();
			m_sInDirectory = FileUtility.NormalizePath(Path.Combine(curDir, m_sInDirectory));
			m_sOutDirectory = FileUtility.NormalizePath(Path.Combine(curDir, m_sOutDirectory));
			if (!string.IsNullOrEmpty(m_sLuaCommand))
			{
				m_sLuaCommand = FileUtility.NormalizePath(Path.Combine(curDir, m_sLuaCommand));
			}
		}

		/// <summary>
		/// Copy of the original command-line arguments.
		/// </summary>
		public string[] OriginalArguments { get; private set; }

		/// <summary>
		/// Conditional compilation symbols.
		/// </summary>
		public HashSet<string> m_ConditionalCompilationSymbols;

		/// <summary>
		/// Configuration for any paralell execution.
		/// </summary>
		public ParallelOptions m_ParallelOptions;

		/// <summary>
		/// Base directory of the input .cs files.
		/// </summary>
		public string m_sInDirectory;

		/// <summary>
		/// Base directory of the output .cs files.
		/// </summary>
		public string m_sOutDirectory;

		/// <summary>
		/// Used when m_bUnitTest is true. When defined, this is the luac
		/// executable that will be used to execute and verify the unit test
		/// file.
		/// </summary>
		public string m_sLuaCommand;

		/// <summary>
		/// An output suffix to apply to all files generates. Empty by default.
		/// </summary>
		public string m_sOutputSuffix;

		/// <summary>
		/// True if daemon mode is disabled.
		/// </summary>
		/// <remarks>
		/// By default, a single daemon compilation server is launched to
		/// keep it warm started (avoid JIT compilation of Roslyn in
		/// the compiler itself). This disables the daemon.
		/// </remarks>
		public bool m_bNoDaemon;

		/// <summary>
		/// True if this run is for executing unit tests.
		/// </summary>
		public bool m_bUnitTest;

		/// <summary>
		/// When true, the Roslyn emit functionality will be used to fully
		/// verify input. Otherwise, syntax or semantic errors may be allowed.
		/// </summary>
		public bool m_bValidate;

		/// <summary>
		/// When true, timing information is printed to the log.
		/// </summary>
		public bool m_bTiming;

		/// <summary>
		/// When not empty, indicates this executable is a daemon.
		/// </summary>
		/// <remarks>
		/// This directory is expected to point to the directory of the client
		/// SlimCS.exe process, which is expected to differ from the server SlimCS.exe.
		/// The server monitors this directory and will automatically exit when
		/// any files relevant to the SlimCS process change, to allow updates to
		/// SlimCS.exe.
		/// </remarks>
		public string m_sDaemonMonitorDir;

		/// <summary>
		/// Exclusive to client/daemon communication.
		/// </summary>
		/// <remarks>
		/// Used by the client to tell a daemon to exit.
		/// </remarks>
		public bool m_bKill;

		/// <summary>
		/// Testing functionality, override the global SlimCS process culture.
		/// </summary>
		/// <remarks>
		/// Can be used in testing scenarios to force SlimCS to behave as if the system culture
		/// is some other value. SlimCS is expected to generate identical Lua code independent
		/// of system culture.
		/// </remarks>
		public string m_sTestCulture;
	}
}
