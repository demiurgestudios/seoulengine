// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using System;
using System.Collections.Generic;
using System.IO;
using static SlimCSLib.Generator.Constants;

namespace SlimCSLib.Generator
{
	public sealed partial class Visitor : CSharpSyntaxVisitor<SyntaxNode>, IDisposable
	{
		#region Private members
		bool IsStringAlwaysNotNull(ExpressionSyntax node)
		{
			// TODO: Replace this special handling will general purpose
			// code to detect if a value can be null in context.

			// We need to wrap Strings if they can possibly be null, since
			// null is not handled the same as a string in most contexts
			// in Lua (e.g. a .. b will raise an error if either is null).
			//
			// Only constant strings, or existing calls to tostring(),
			// are guaranteed not to be null.
			{
				var sym = m_Model.GetSymbolInfo(node).Symbol;
				if (sym is IMethodSymbol methodSym)
				{
					// TODO: Not always a safe assumption.
					//
					// Assume if sym is an operator that it will always return a non-null string.
					if (methodSym.MethodKind == MethodKind.BuiltinOperator ||
						methodSym.MethodKind == MethodKind.UserDefinedOperator)
					{
						return true;
					}
					// Check for various tostring() cases that we know do not return null.
					else if (methodSym.IsCsharpToString() || methodSym.IsLuaToString())
					{
						return true;
					}
				}
			}

			// Constant value that evaluates to non-null.
			{
				var constantValue = m_Model.GetConstantValue(node);
				if (constantValue.HasValue && null != constantValue.Value)
				{
					return true;
				}
			}

			// Look for the null coalescing operator and check its second argument.
			if (node.Kind() == SyntaxKind.CoalesceExpression)
			{
				var binaryOp = (BinaryExpressionSyntax)node;
				return IsStringAlwaysNotNull(binaryOp.Right);
			}

			// Fallback, can be null.
			return false;
		}

		/// <summary>
		/// Check if an expression needs to be wrapped in a Lua tostring() call.
		/// </summary>
		/// <param name="node">The expression to check.</param>
		/// <returns>true if a tostring() call is needed, false otherwise.</returns>
		/// <remarks>
		/// The context of this check is an expression in a concatenation expression
		/// '..' or other contexts where a string or number will be implicitly coerced
		/// to a string.
		/// </remarks>
		bool NeedsLuaToString(ExpressionSyntax node)
		{
			var type = m_Model.GetTypeInfo(node).Type;
			bool bToString = true;

			// Early out, no type info.
			if (type == null)
			{
				return bToString;
			}

			// String and numbers will be implicitly coerced to string
			// in approprigate contexts.
			switch (type.SpecialType)
			{
				case SpecialType.System_Double:
				case SpecialType.System_Int32:
				case SpecialType.System_Int64:
				case SpecialType.System_Single:
				case SpecialType.System_UInt32:
				case SpecialType.System_UInt64:
					bToString = false;
					break;

				case SpecialType.System_String:
					bToString = !IsStringAlwaysNotNull(node);
					break;
			}

			return bToString;
		}
		#endregion

		void VisitInterpolation(
			InterpolationFormatClauseSyntax format,
			InterpolationAlignmentClauseSyntax align,
			ExpressionSyntax expr)
		{
			// TODO: Format clause support.
			if (null != format)
			{
				throw new UnsupportedNodeException(format, "format clause in interpolated string is not supported.");
			}

			// Wrap the interpolation in a call to string.align() (this is
			// our function, not a builtin one) to replicate the desired
			// behavior.
			var bHasAlignment = (null != align);
			if (bHasAlignment)
			{
				Write(kLuaStringAlign);
				Write('(');
			}

			// Check if we need to add a tostring() call. Redundant
			// if we're already wrapping the expression in an
			// align call (align will tostring() its string argument).
			var bToString = !bHasAlignment && NeedsLuaToString(expr);

			// Finally, check if we need at least parens.
			var bParens = !bToString && expr.IsLowerPrecedenceThanLuaConcat();

			// Add tostring() to the expression if necessary.
			if (bToString)
			{
				if (NeedsWhitespaceSeparation) { Write(' '); }
				Write(kLuaTostring); Write('(');
			}
			else if (bParens) { Write('('); }
			Visit(expr);
			if (bToString || bParens) { Write(')'); }

			// Finish the call to string.align().
			if (bHasAlignment)
			{
				Write(',');

				var value = align.Value;
				Visit(value);

				Write(')');
			}
		}

		public override SyntaxNode VisitInterpolation(InterpolationSyntax node)
		{
			VisitInterpolation(node.FormatClause, node.AlignmentClause, node.Expression);
			return node;
		}

		public override SyntaxNode VisitInterpolatedStringExpression(InterpolatedStringExpressionSyntax node)
		{
			int iCount = node.Contents.Count;
			for (int i = 0; i < iCount; ++i)
			{
				// Concatenation of the string parts.
				if (0 != i)
				{
					Write(' ');
					Write(kLuaConcat);
					Write(' ');
				}

				// Emit the expression itself. The handler for the individual
				// expression parts will add tostring() calls as
				// needed.
				var contents = node.Contents[i];
				Visit(contents);
			}

			// Special handling if count is 0, must write out an empty string.
			if (0 == iCount)
			{
				Write("''");
			}

			return node;
		}

		public override SyntaxNode VisitInterpolatedStringText(InterpolatedStringTextSyntax node)
		{
			Write(node.TextToken);
			return node;
		}
	}
}
