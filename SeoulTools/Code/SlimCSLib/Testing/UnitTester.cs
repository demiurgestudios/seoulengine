// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using Microsoft.CodeAnalysis;
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Threading;
using System.Text;

namespace SlimCSLib.Testing
{
	public static class UnitTester
	{
		public const int kiTestLuaRunTimeInMilliseconds = 5000;
		public const string ksTestHarnessName = "TestHarness.cs";

		#region Private members
		/// <summary>
		/// Utility structure used for unit test executable runs.
		/// </summary>
		struct ProcessRunResult
		{
			public int m_iExitCode;
			public List<string> m_Stdout;
			public List<string> m_Stderr;
		}

		/// <summary>
		/// Generate the arguments to pass to a luac runtime
		/// to execute unit tests.
		/// </summary>
		/// <param name="sFileName">The main file to execute.</param>
		/// <returns>Arguments for the lua runtime.</returns>
		static string MakeTestArguments(string sFileName)
		{
			return Utilities.FormatArgumentsForCommandLine(sFileName);
		}

		/// <summary>
		/// Generic utility for executing a command-line process and returning results.
		/// </summary>
		/// <param name="sFileName">The executable to run.</param>
		/// <param name="sArguments">Gathered argument to pass to the command.</param>
		/// <param name="sWorkingDirectory">Current directory of the process.</param>
		/// <returns>Results of the process run.</returns>
		static ProcessRunResult RunProcess(string sFileName, string sArguments, string sWorkingDirectory)
		{
			// Perform the run.
			var ret = new ProcessRunResult()
			{
				m_iExitCode = 1,
				m_Stderr = new List<string>(),
				m_Stdout = new List<string>(),
			};
			using (var process = System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo()
			{
				Arguments = sArguments,
				CreateNoWindow = true,
				ErrorDialog = false,
				FileName = sFileName,
				RedirectStandardError = true,
				RedirectStandardInput = false,
				RedirectStandardOutput = true,
				StandardErrorEncoding = Encoding.UTF8,
				StandardOutputEncoding = Encoding.UTF8,
				UseShellExecute = false,
				WorkingDirectory = sWorkingDirectory,
			}))
			{
				var waitOutput = new AutoResetEvent(false);
				var waitError = new AutoResetEvent(false);
				process.ErrorDataReceived += (sender, e) =>
				{
					lock (ret.m_Stderr)
					{
						if (!string.IsNullOrEmpty(e.Data))
						{
							ret.m_Stderr.Add(e.Data);
						}
						else if (null == e.Data)
						{
							// null indicates stream termination.
							waitError.Set();
						}
					}
				};
				process.OutputDataReceived += (sender, e) =>
				{
					lock (ret.m_Stdout)
					{
						if (!string.IsNullOrEmpty(e.Data))
						{
							ret.m_Stdout.Add(e.Data);
						}
						else if (null == e.Data)
						{
							// null indicates stream termination.
							waitOutput.Set();
						}
					}
				};

				process.BeginErrorReadLine();
				process.BeginOutputReadLine();

				// Run - on successful completion, set the exit code.
				if (process.WaitForExit(kiTestLuaRunTimeInMilliseconds))
				{
					ret.m_iExitCode = process.ExitCode;
				}
				// Otherwise, kill the process.
				else
				{
					// Kill.
					process.Kill();

					// Always an error.
					ret.m_iExitCode = 1;
				}

				WaitHandle.WaitAll(new WaitHandle[] { waitOutput, waitError }, kiTestLuaRunTimeInMilliseconds);
			}

			return ret;
		}

		static string AssembleExceptionLuaBody(List<string> testFiles)
		{
			var ret = new StringBuilder();
			foreach (var sFileCS in testFiles)
			{
				string sBody = null;
				try
				{
					sBody = File.ReadAllText(Path.ChangeExtension(sFileCS, ".lua"));
				}
				catch (IOException)
				{
					sBody = "<error reading file>";
				}

				ret.AppendLine($"File: {Path.GetFileNameWithoutExtension(sFileCS)}");
				ret.AppendLine(sBody);
			}

			return ret.ToString();
		}

		static void TestRunLua(
			CommandArgs args,
			SharedModel sharedModel,
			string sFileName,
			List<string> expected,
			List<string> testFiles)
		{
			// Perform the run with Lua.
			var sWorkingDirectory = Path.GetDirectoryName(sFileName);
			var res = RunProcess(args.m_sLuaCommand, MakeTestArguments(sFileName), sWorkingDirectory);

			// Error.
			if (0 != res.m_iExitCode)
			{
				var sMessage =
					$"unexpected return value {res.m_iExitCode}" +
					Environment.NewLine +
					string.Join(Environment.NewLine, res.m_Stderr);
				throw new TestRunException(sMessage, AssembleExceptionLuaBody(testFiles));
			}

			// Check for equality tolerance - this is used in cases
			// where the formatting of Lua print() for floating point
			// numbers is expected to be different from C#.
			var bEqualityTolerance = false;
			{
				var attributes = sharedModel?.MainMethod?.ContainingType?.GetAttributes();
				if (null != attributes)
				{
					foreach (var attr in attributes.Value)
					{
						if (attr.AttributeClass.Name == "DoubleEqualityToleranceAttribute")
						{
							bEqualityTolerance = true;
							break;
						}
					}
				}
			}

			// Check expected output.
			if (expected.Count != res.m_Stdout.Count)
			{
				throw new TestRunException(
					$"expected output has '{expected.Count}' lines ({string.Join("|", expected)}) but run output has '{res.m_Stdout.Count}' lines ({string.Join("|", res.m_Stdout)}).",
					AssembleExceptionLuaBody(testFiles));
			}
			for (int i = 0; i < res.m_Stdout.Count; ++i)
			{
				if (expected[i] != res.m_Stdout[i])
				{
					// Allow some additional tolerance if requested.
					if (bEqualityTolerance)
					{
						double fA = 0.0;
						double fB = 0.0;
						if (double.TryParse(expected[i], NumberStyles.Any, CultureInfo.InvariantCulture, out fA) &&
							double.TryParse(res.m_Stdout[i], NumberStyles.Any, CultureInfo.InvariantCulture, out fB) &&
							Math.Abs(fA - fB) <= 1e-13)
						{
							continue;
						}
					}

					// Handle standard exception messages in other cultures (SlimCS is always
					// going to return an English string for now).
					switch (res.m_Stdout[i])
					{
						case "Exception of type 'System.Exception' was thrown.":
						{
							var testException = new Exception();
							if (expected[i] == testException.Message)
							{
								continue;
							}
						} break;

						case "Object reference not set to an instance of an object.":
						{
							var testException = new NullReferenceException();
							if (expected[i] == testException.Message)
							{
								continue;
							}
						} break;
					}

					// Failure.
					throw new TestRunException(
						$"{i}: C# output: '{expected[i]}' != Lua output: '{res.m_Stdout[i]}'",
						AssembleExceptionLuaBody(testFiles));
				}
			}
		}

		static void Test(
			CommandArgs args,
			SharedModel sharedModel,
			string sFileName,
			SyntaxTree[] aTrees,
			List<string> testFiles)
		{
			// No luac available, skip the test run.
			if (string.IsNullOrEmpty(args.m_sLuaCommand))
			{
				return;
			}

			// Run the assembly to get expected return values.
			var expected = TestExpected(aTrees);

			// Run in lua.
			TestRunLua(args, sharedModel, sFileName, expected, testFiles);
		}

		/// <summary>
		/// Generates an executes a unit testing binary to generate
		/// the list of expected std output, for comparison against
		/// Lua generated output.
		/// </summary>
		/// <param name="aTrees">Full set of CSharp ASTs for the binary.</param>
		/// <returns>The output results.</returns>
		static List<string> TestExpected(SyntaxTree[] aTrees)
		{
			// Generate the testing EXE.
			var sExe = Compilation.Compiler.CompileForExecution(aTrees);
			try
			{
				// Execute and then return stdout results on success.
				var res = RunProcess(sExe, string.Empty, Directory.GetCurrentDirectory());
				if (0 != res.m_iExitCode)
				{
					throw new TestRunException(
						$"expected run (C#) failed with exide code: {res.m_iExitCode} and errors: {string.Join(Environment.NewLine, res.m_Stderr)}", null);
				}

				return res.m_Stdout;
			}
			finally
			{
				// TODO: Must clean this up after run completion,
				// but we don't want this to enter an infinite loop.
				while (File.Exists(sExe))
				{
					try
					{
						File.Delete(sExe);
					}
					catch
					{ }
				}
			}
		}
		#endregion

		class FileNameSorter : IComparer<string>
		{
			[System.Runtime.InteropServices.DllImport("shlwapi.dll", CharSet = System.Runtime.InteropServices.CharSet.Unicode, ExactSpelling = true)]
			static extern int StrCmpLogicalW(string x, string y);

			public int Compare(string x, string y)
			{
				return StrCmpLogicalW(x, y);
			}
		}

		/// <summary>
		/// When provided, the input directory is treated as a unit testing source.
		/// Each .cs file is compiled alone.
		/// </summary>
		/// <param name="settings">Configure settings for the unit tests.</param>
		public static void RunUnitTests(CommandArgs args)
		{
			// Get our temp path and clean it up. Then recreate it for the run.
			var sTempDir = Path.Combine(Path.GetTempPath(), "SlimCSUnitTests");

			FileUtility.ForceDeleteDirectory(sTempDir);

			// Create new settings for the individual compilations.
			var testArgs = args;
			testArgs.m_sInDirectory = sTempDir;
			testArgs.m_sOutDirectory = sTempDir;
			testArgs.m_bValidate = false; // Always disabling validate in test mode, handle with test run.
			testArgs.m_ConditionalCompilationSymbols = new HashSet<string>();
			testArgs.m_ConditionalCompilationSymbols.Add("DEBUG"); // Always define debug in test mode.

			// Test tracking.
			int iLuaFailed = 0;
			int iTestsRun = 0;
			int iTestsPassed = 0;
			int iTestsCSharpCompilationFailed = 0;

			// Include the test harness in all tests, if it exists.
			var sHarness = Path.Combine(args.m_sInDirectory, ksTestHarnessName);
			var bHarness = File.Exists(sHarness);
			var sTempHarness = FileUtility.NormalizePath(Path.Combine(sTempDir, Path.GetFileName(sHarness)));

			// Now list and enumerate files.
			var asFiles = Directory.GetFiles(args.m_sInDirectory, "*.cs");
			Array.Sort(asFiles, new FileNameSorter());
			int iFiles = asFiles.Length;
			for (int i = 0; i < asFiles.Length; ++i)
			{
				// Cache the name base name.
				var sInputFile = asFiles[i];

				// Skip the harness.
				var sFileName = Path.GetFileName(sInputFile);
				if (sFileName == ksTestHarnessName)
				{
					continue;
				}

				// Tests that start with Fail must fail to compile. These
				// are tests of SlimCS disabling specific functionality
				// deliberately.
				var bExpectFail = (sFileName.StartsWith("Fail"));
				var bFailed = false;

				var watch = System.Diagnostics.Stopwatch.StartNew();

				try
				{
					// Create a temp entry for the file.
					var sBaseFile = Path.GetFileName(sInputFile);
					var sTempMain = FileUtility.NormalizePath(Path.Combine(sTempDir, "SlimCSTestMain.lua"));
					var sOutputFile = FileUtility.NormalizePath(Path.Combine(sTempDir, sBaseFile));

					// Tracking.
					var testFiles = new List<string>();
					testFiles.Add(sOutputFile);

					// Start reporting.
					ConsoleWrapper.Write($".. Running test {sBaseFile}: ");

					// Make sure the temporary directory exists.
					FileUtility.CreateDirectoryAll(sTempDir);

					// Copy the harness if present.
					if (bHarness) { File.Copy(sHarness, sTempHarness); }

					// Copy the file(s) we're about to compile to the temp directory.
					do
					{
						File.Copy(sInputFile, sOutputFile);

						// If the next file is a lib file, include it with
						// this test.
						if (i + 1 < asFiles.Length &&
							Path.GetFileNameWithoutExtension(asFiles[i+1]).EndsWith("-lib"))
						{
							++i;
							sInputFile = asFiles[i];
							sOutputFile = FileUtility.NormalizePath(Path.Combine(sTempDir, Path.GetFileName(sInputFile)));
							testFiles.Add(sOutputFile);
							continue;
						}

						break;
					} while (true);

					try
					{
						// About to run the test.
						++iTestsRun;

						// Perform the compile.
						(var aTrees, var sharedModel) = Compilation.Pipeline.Build(
							ConsoleWrapper.Out.WriteLine,
							ConsoleWrapper.Error.WriteLine,
							testArgs,
							bExpectFail,
							true);

						// No main, can't run.
						if (null == sharedModel.MainMethod)
						{
							throw new TestRunException("no main method found.", null);
						}

						// Generate the main lua test harness.
						using (var file = File.Open(sTempMain, FileMode.Create, FileAccess.Write, FileShare.None))
						{
							using (var stream = new StreamWriter(file, Encoding.ASCII))
							{
								// Require the SlimCS main.
								stream.Write($"require '{Path.GetFileNameWithoutExtension(Compilation.Constants.ksMainLibNameLua)}'");
								stream.Write(Environment.NewLine);

								// Require the SlimCS file list.
								stream.Write($"require '{Path.GetFileNameWithoutExtension(Compilation.Constants.ksFileLibName)}'");
								stream.Write(Environment.NewLine);

								// Test entry point.
								var mainMethod = sharedModel.MainMethod;
								if (!mainMethod.ReturnsVoid)
								{
									stream.Write("os.exit(");
								}
								stream.Write(sharedModel.LookupOutputId(mainMethod.ContainingType));
								stream.Write(".");
								stream.Write(sharedModel.LookupOutputId(mainMethod));
								stream.Write("()");
								if (!mainMethod.ReturnsVoid)
								{
									stream.Write(")");
								}
							}
						}

						// Run the test.
						Test(testArgs, sharedModel, sTempMain, aTrees, testFiles);
					}
					finally
					{
						// Must always cleanup the temp files.
						FileUtility.ForceDeleteDirectory(sTempDir);
					}
				}
				catch (TestRunException e)
				{
					++iLuaFailed;
					ConsoleWrapper.WriteLine($"FAIL ({e.Message})");
					if (null != e.BodyLua)
					{
						ConsoleWrapper.WriteLine("Lua Body:");
						ConsoleWrapper.WriteLine(e.BodyLua);
					}
					continue;
				}
				catch (Exception e) when (e is SlimCSCompilationException || e is UnsupportedNodeException)
				{
					// Failure test, fall-through.
					if (bExpectFail)
					{
						bFailed = true;
					}
					else
					{
						++iTestsCSharpCompilationFailed;
						ConsoleWrapper.WriteLine($"FAIL ({e.Message})");
						continue;
					}
				}
				catch (Exception e)
				{
					++iTestsCSharpCompilationFailed;
					ConsoleWrapper.WriteLine($"FAIL ({e.Message})");
					continue;
				}

				// Handle a test that is supposed to fail but didn't.
				if (bExpectFail && !bFailed)
				{
					++iTestsCSharpCompilationFailed;
					ConsoleWrapper.WriteLine($"FAIL (compilation succeeded, expected to fail)");
					continue;
				}

				watch.Stop();

				// Test pass.
				++iTestsPassed;
				ConsoleWrapper.WriteLine($"PASS ({watch.Elapsed.TotalSeconds}) secs");
			}

			if (iLuaFailed > 0 || iTestsCSharpCompilationFailed > 0)
			{
				throw new TestRunException($"FAILED: {iLuaFailed}, {iTestsCSharpCompilationFailed}", null);
			}
		}
	}
}
