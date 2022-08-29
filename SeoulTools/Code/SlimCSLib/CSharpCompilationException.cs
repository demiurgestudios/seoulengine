// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using System;

namespace SlimCSLib
{
	public abstract class ExceptionCommon : Exception
	{
		#region Private members
		static string ComposeMessage(SyntaxNode node, string sMessage)
		{
			var syntaxTree = node.SyntaxTree;
			var textSpan = node.Span;
			var pos = syntaxTree.GetLineSpan(node.Span);

			return $"{pos.Path}({pos.StartLinePosition.Line + 1},{pos.StartLinePosition.Character + 1},{pos.EndLinePosition.Line + 1},{pos.EndLinePosition.Character + 1}): {sMessage}";
		}

		static string ComposeMessage(SyntaxNode node)
		{
			return ComposeMessage(node, "'" + node.Kind().ToString() + "' nodes are not supported.");
		}
		#endregion

		public ExceptionCommon(string sMessage)
			: base(sMessage)
		{
			OrigMessage = sMessage;
		}

		public ExceptionCommon(SyntaxNode node)
			: base(ComposeMessage(node))
		{
			Node = node;
			OrigMessage = "'" + node.Kind().ToString() + "' nodes are not supported.";
		}

		public ExceptionCommon(SyntaxNode node, string sMessage)
			: base(ComposeMessage(node, sMessage))
		{
			Node = node;
			OrigMessage = sMessage;
		}

		public SyntaxNode Node { get; private set; }
		public string OrigMessage { get; private set; }
	}

	public sealed class CSharpCompilationException : ExceptionCommon
	{
		public CSharpCompilationException(string sMessage)
			: base(sMessage)
		{
		}

		public CSharpCompilationException(SyntaxNode node, string sMessage)
			: base(node, sMessage)
		{
		}
	}

	public sealed class SlimCSCompilationException : ExceptionCommon
	{
		public SlimCSCompilationException(string sMessage)
			: base(sMessage)
		{
		}

		public SlimCSCompilationException(SyntaxNode node, string sMessage)
			: base(node, sMessage)
		{
		}
	}

	public sealed class UnsupportedNodeException : ExceptionCommon
	{
		public UnsupportedNodeException(SyntaxNode node)
			: base(node)
		{
		}

		public UnsupportedNodeException(SyntaxNode node, string sMessage)
			: base(node, sMessage)
		{
		}
	}
}
