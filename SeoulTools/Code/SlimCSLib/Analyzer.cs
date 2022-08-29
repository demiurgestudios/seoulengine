// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.Diagnostics;
using System.Collections.Immutable;
using System.IO;

namespace SlimCSLib
{
	/// <summary>
	/// Analyzer that wraps SlimCS for integration of SlimCS specific
	/// error handling into Visual Studio.
	/// </summary>
	[DiagnosticAnalyzer("C#")]
	public sealed class SlimCSAnalyzer : DiagnosticAnalyzer
	{
		#region Private members
		void Analyze(CompilationAnalysisContext context)
		{
			var csharpCompilation = context.Compilation as CSharpCompilation;
			if (null == csharpCompilation)
			{
				return;
			}

			// TODO: Brittle, but we know that
			// SlimCS requires at least one file in the root. So we just
			// use the directory of the shortest path.
			var dir = string.Empty;
			{
				var shortest = string.Empty;
				foreach (var tree in csharpCompilation.SyntaxTrees)
				{
					var f = tree.FilePath;
					if (string.IsNullOrEmpty(shortest) ||
						f.Length < shortest.Length)
					{
						shortest = f;
					}
				}

				dir = Path.GetDirectoryName(shortest);
			}

			var args = new CommandArgs(new string[] { dir });
			var shared = new SharedModel(args, csharpCompilation);
			var syntaxTrees = csharpCompilation.SyntaxTrees;

			// Now emit the output as .lua files.
			var es = Compilation.Pipeline.Analyze(args, shared, syntaxTrees);
			foreach (var e in es)
			{
				var msg = string.Empty;
				var location = Location.None;
				if (e is SlimCSLib.ExceptionCommon com)
				{
					if (null != com.Node)
					{
						location = Location.Create(
							com.Node.SyntaxTree,
							com.Node.Span);
					}
					msg = com.OrigMessage;
				}
				else
				{
					msg = e.Message;
				}

				context.ReportDiagnostic(Diagnostic.Create(
					Rule,
					location,
					msg));
			}
		}

		readonly static DiagnosticDescriptor Rule = new DiagnosticDescriptor(
			"SlimCS",
			"SlimCS Analyzer",
			"{0}",
			"",
			DiagnosticSeverity.Error,
			true);
		#endregion

		public override void Initialize(AnalysisContext context)
		{
			context.ConfigureGeneratedCodeAnalysis(GeneratedCodeAnalysisFlags.None);
			context.EnableConcurrentExecution();
			context.RegisterCompilationAction(Analyze);
		}

		public override ImmutableArray<DiagnosticDescriptor> SupportedDiagnostics => ImmutableArray.Create(Rule);
	}
}
