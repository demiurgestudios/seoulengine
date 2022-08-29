// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using System;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;

namespace SlimCSLib.Compilation
{
	/// <summary>
	/// Utilities for compiling input .cs files into syntax trees
	/// and (for testing) a runnable executable.
	/// </summary>
	public static class Compiler
	{
		#region Private members
		static readonly object s_Lock = new Object();
		static MetadataReference[] s_kaBuildAssemblies;
		static MetadataReference[] s_kaTestAssemblies;

		static MetadataReference ResolveMetadataReference(string sIn)
		{
			// Normalize the DLL reference.
			sIn = sIn.Trim();
			if (!sIn.EndsWith(".dll"))
			{
				sIn = sIn + ".dll";
			}

			// Check for the existence of the file locally - if it does not exist,
			// resolve to the runtime directory.
			var s = Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), sIn);
			if (!File.Exists(s))
			{
				s = Path.Combine(RuntimeEnvironment.GetRuntimeDirectory(), sIn);
			}

			// Resolve the metadata reference.
			return MetadataReference.CreateFromFile(s);
		}

		static MetadataReference[] BuildAssemblies
		{
			get
			{
				lock (s_Lock)
				{
					if (null == s_kaBuildAssemblies)
					{
						s_kaBuildAssemblies = new MetadataReference[]
						{
							ResolveMetadataReference("SlimCSCorlib"),
						};
					}
				}

				return s_kaBuildAssemblies;
			}
		}

		static MetadataReference[] TestAssemblies
		{
			get
			{
				lock (s_Lock)
				{
					if (null == s_kaTestAssemblies)
					{
						s_kaTestAssemblies = new MetadataReference[]
						{
							ResolveMetadataReference("mscorlib"),
							ResolveMetadataReference("System"),
							ResolveMetadataReference("System.Core")
						};
					}
				}

				return s_kaTestAssemblies;
			}
		}
		#endregion

		/// <summary>
		/// Common path, generates a CSharpCompilation object, used
		/// for semantic analysis and code validation.
		/// </summary>
		/// <param name="args">Configuration options.</param>
		/// <param name="aSyntaxTrees">Trees to use for generation.</param>
		/// <param name="bTesting">If true, use the full .NET for a buildable run. Otherwise, use SlimCSCorlib.</param>
		/// <returns>The compilation unit for semantic analysis and validation.</returns>
		public static CSharpCompilation CompileForCodeGen(
			CommandArgs args,
			SyntaxTree[] aSyntaxTrees,
			bool bTesting)
		{
			// Instantiation a compilation unit.
			var warningsSuppressed = new System.Collections.Generic.Dictionary<string, ReportDiagnostic>()
			{
				// warning CS0626: Method, operator, or accessor '<name>'
				// is marked external and has no attributes on it. Consider
				// adding a DllImport attribute to specify the external implementation.
				["CS0626"] = ReportDiagnostic.Suppress,

				// hidden CS8019: Unnecessary using directive.
				["CS8019"] = ReportDiagnostic.Suppress,
			};
			var compilation = CSharpCompilation.Create(
				"SlimCS__internal",
				aSyntaxTrees,
				(bTesting ? TestAssemblies : BuildAssemblies),
				new CSharpCompilationOptions(
					(args.m_bUnitTest ? OutputKind.ConsoleApplication : OutputKind.DynamicallyLinkedLibrary),
					specificDiagnosticOptions: warningsSuppressed));

			return compilation;
		}

		/// <summary>
		/// Converts the input ASTs into a runnable console .exe file
		/// in a temp location. Returns the path to that file.
		/// </summary>
		/// <param name="args">Configuration options.</param>
		/// <param name="aSyntaxTrees">Trees to use for generation.</param>
		/// <returns>The .exe to be executed.</returns>
		public static string CompileForExecution(SyntaxTree[] aSyntaxTrees)
		{
			// Get a temp path - we place this next to the input,
			// not in the temp directory, as anti-virus gets cranky
			// if we generate unknown .exe files in the temp directory.
			var sFile = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "SlimCSUnitTest__.exe");
			if (File.Exists(sFile))
			{
				File.Delete(sFile);
			}

			// Create the compiler.
			var ret = CSharpCompilation.Create(
				Path.GetFileNameWithoutExtension(sFile),
				aSyntaxTrees,
				TestAssemblies,
				new CSharpCompilationOptions(OutputKind.ConsoleApplication));

			// Emit.
			using (var stream = File.OpenWrite(sFile))
			{
				var res = ret.Emit(stream);
				if (!res.Success)
				{
					throw new CSharpCompilationException(string.Join(Environment.NewLine, res.Diagnostics));
				}
			}

			return sFile;
		}
	}
}
