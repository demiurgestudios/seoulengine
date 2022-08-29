// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using System.Collections.Generic;
using System.Collections.Immutable;

namespace SlimCSLib.Generator
{
	/// <summary>
	/// Generalized extensions to the C# SemanticModel that apply
	/// various additional constraints or limitations of SlimCS.
	/// </summary>
	public static class Constraints
	{
		/// <summary>
		/// Enforce general SlimCS limitations on type access.
		/// We support a minimum number of System types required
		/// by the C# frontend.
		/// </summary>
		static void Enforce(this Model model, SyntaxNode context, ITypeSymbol type)
		{
			if (!type.IsValidSlimCSType())
			{
				throw new SlimCSCompilationException(context, $"SlimCS does not support '{type}'.");
			}
		}

		/// <summary>
		/// Enforces the System.Diagnostics.Contracts.PureAttribute.
		/// </summary>
		/// <param name="model">Model to use for processing.</param>
		/// <param name="node">Node to traverse and enforce pure.</param>
		static void EnforcePure(
			this Model model,
			CSharpSyntaxNode body,
			ITypeSymbol bodyContainingType)
		{
			// Traverse.
			foreach (var child in body.DescendantNodes())
			{
				var eKind = child.Kind();
				switch (eKind)
				{
					// Identifiers may be properties, which also need to be checked for purity.
					case SyntaxKind.IdentifierName:
						{
							var symInfo = model.GetSymbolInfo(child);
							if (symInfo.Symbol is IPropertySymbol propSym)
							{
								// Check that the getter is pure.
								var getter = propSym.GetMethod;
								if (!getter.HasPureAttribute())
								{
									throw new SlimCSCompilationException(child, $"impure access {child} inside pure method - all properties invoked must also be pure.");
								}
							}
						}
						break;

					// Invocations must be pure.
					case SyntaxKind.InvocationExpression:
						{
							var invoke = (InvocationExpressionSyntax)child;
							var sym = model.GetSymbolInfo(invoke.Expression).Symbol;
							if (null != sym && !sym.HasPureAttribute())
							{
								throw new SlimCSCompilationException(child, $"impure invocation {child} inside pure method - all methods invoked must also be pure.");
							}
						}
						break;

					// Must be verified.
					case SyntaxKind.SimpleAssignmentExpression:
						{
							var assign = (AssignmentExpressionSyntax)child;

							// Handling for assignment tuples.
							if (assign.Left.Kind() == SyntaxKind.TupleExpression)
							{
								var tuple = (TupleExpressionSyntax)assign.Left;
								foreach (var arg in tuple.Arguments)
								{
									// Assignments are only allowed to local variables.
									var sym = model.GetSymbolInfo(arg.Expression).Symbol;
									if (null != sym && sym.Kind != SymbolKind.Local)
									{
										throw new SlimCSCompilationException(arg.Expression, $"impure mutation {child} inside pure method.");
									}
								}
							}
							// Standard assignment.
							else
							{
								// Assignments are only allowed to local variables.
								var sym = model.GetSymbolInfo(assign.Left).Symbol;
								if (null != sym && sym.Kind != SymbolKind.Local)
								{
									throw new SlimCSCompilationException(child, $"impure mutation {child} inside pure method.");
								}
							}
						}
						break;

						// All others are pure or an inner node.
					default:
						break;
				}
			}
		}

		public static void Enforce(this Model model, AccessorDeclarationSyntax node)
		{
		}

		public static void Enforce(this Model model, AnonymousMethodExpressionSyntax node)
		{
			// Also validate parameters.
			if (null != node.ParameterList)
			{
				model.Enforce(node.ParameterList);
			}
		}

		public static void Enforce(this Model model, ArgumentListSyntax node)
		{
			// constraints:
			// - no ref and not out.
			// - no tuples (tuples are expanded to multiple values, so they can't be passed as single arguments.
			foreach (var arg in node.Arguments)
			{
				var type = model.GetTypeInfo(arg.Expression).ConvertedType;
				if (null != type && type.IsTupleType)
				{
					throw new UnsupportedNodeException(node, "SlimCS does not support tuples as single arguments.");
				}

				if (arg.RefOrOutKeyword.Kind() == SyntaxKind.RefKeyword ||
					arg.RefOrOutKeyword.Kind() == SyntaxKind.OutKeyword)
				{
					throw new UnsupportedNodeException(node, "'ref' and 'out' parameters are not supported.");
				}
			}
		}

		public static void Enforce(this Model model, ArrayCreationExpressionSyntax node)
		{
			// Current constraints:
			// - only 1 rank allowed.
			// - type cannot be dynamic or object - we must know at compile time that
			//   all the values in a particular array either can or cannot be nil.
			{
				var arrType = model.GetTypeInfo(node.Type).ConvertedType as IArrayTypeSymbol;
				if (null != arrType)
				{
					var elemType = arrType.ElementType;
					if (elemType.SpecialType == SpecialType.System_Object ||
						elemType.Kind == SymbolKind.DynamicType)
					{
						throw new SlimCSCompilationException(node, "SlimCS does not support arrays of type object or type dynamic.");
					}
				}
			}
			if (node.Type.RankSpecifiers.Count != 1)
			{
				throw new SlimCSCompilationException(node, "multi-dimensional arrays are not supported.");
			}
			if (node.Type.RankSpecifiers.First().Rank != 1)
			{
				throw new SlimCSCompilationException(node, "multi-dimensional arrays are not supported.");
			}
		}

		public static void Enforce(this Model model, AssignmentExpressionSyntax node)
		{
		}

		public static void Enforce(this Model model, CastExpressionSyntax node)
		{
			// Not allowed to cast to type dynamic.
			var type = model.GetTypeInfo(node.Type);
			if ((type.Type != null && type.Type.TypeKind == TypeKind.Dynamic) ||
				(type.ConvertedType != null && type.ConvertedType.TypeKind == TypeKind.Dynamic))
			{
				throw new SlimCSCompilationException(node, "SlimCS does not support cast to type dynamic.");
			}
		}

		public static void Enforce(this Model model, ClassDeclarationSyntax node)
		{
		}

		public static void Enforce(this Model model, ConstructorDeclarationSyntax node)
		{
			// Current constraints:
			// - async modifier is not supported.
			if (node.Modifiers.HasToken(SyntaxKind.AsyncKeyword))
			{
				throw new UnsupportedNodeException(node, "the 'async' keyword is not supported.");
			}

			// Also validate parameters.
			model.Enforce(node.ParameterList);
		}

		public static void Enforce(this Model model, DelegateDeclarationSyntax node)
		{
			// Current constraints:
			// - async modifier is not supported.
			if (node.Modifiers.HasToken(SyntaxKind.AsyncKeyword))
			{
				throw new UnsupportedNodeException(node, "the 'async' keyword is not supported.");
			}

			// Also validate parameters.
			model.Enforce(node.ParameterList);
		}

		public static void Enforce(this Model model, EnumDeclarationSyntax node)
		{
		}

		public static void Enforce(this Model model, EnumMemberDeclarationSyntax node)
		{
		}

		public static void Enforce(this Model model, EventDeclarationSyntax node)
		{
		}

		public static void Enforce(this Model model, EventFieldDeclarationSyntax node)
		{
		}

		public static void Enforce(this Model model, FieldDeclarationSyntax node)
		{
			// Also apply declaration constraints.
			model.Enforce(node.Declaration);
		}

		public static void Enforce(this Model model, ImplicitArrayCreationExpressionSyntax node)
		{
		}

		public static void Enforce(this Model model, IndexerDeclarationSyntax node)
		{
			if (node.ExpressionBody != null)
			{
				throw new SlimCSCompilationException(node, "expression body property not yet supported.");
			}

			// Pure enforcement.
			foreach (var accessor in node.AccessorList.Accessors)
			{
				var accessSym = model.GetDeclaredSymbol(accessor);
				if (accessSym.HasPureAttribute())
				{
					// Find the body to traverse.
					var body = (accessor.Body != null ? (CSharpSyntaxNode)accessor.Body : accessor.ExpressionBody?.Expression);
					if (null != body && accessSym.ContainingType != null)
					{
						model.EnforcePure(body, accessSym.ContainingType);
					}
				}
			}
		}

		public static void Enforce(this Model model, InterfaceDeclarationSyntax node)
		{
		}

		public static void Enforce(this Model model, ITypeSymbol to, EqualsValueClauseSyntax initializer)
		{
		}

		public static void Enforce(
			this Model model,
			InvocationExpressionSyntax node,
			IMethodSymbol method,
			IReadOnlyList<ArgumentSyntax> args,
			bool bParams)
		{
			// Also validate parameters.
			model.Enforce(node.ArgumentList);
		}

		public static void Enforce(this Model model, LiteralExpressionSyntax node)
		{
			var type = model.GetTypeInfo(node).Type;
			if (null != type)
			{
				if (!type.IsValidSlimCSType())
				{
					// We support integral types if we're within an expression
					// that explicitly casts to double or int32, or if
					// it is an integral type that is unambiguous when converted
					// to either an int32 or a double (so, char, byte, sbyte, int16, or uint16).
					switch (type.SpecialType)
					{
						case SpecialType.System_Byte:
						case SpecialType.System_Char:
						case SpecialType.System_Int16:
						case SpecialType.System_SByte:
						case SpecialType.System_Single:
						case SpecialType.System_UInt16:
							return;

						default:
							if (type.SpecialType.IsIntegralType() &&
								node.ResolveEnclosingCastExpression() is CastExpressionSyntax castExpr)
							{
								var convertedTo = model.GetTypeInfo(castExpr.Type).ConvertedType;
								if (convertedTo?.SpecialType == SpecialType.System_Int32 ||
									convertedTo?.SpecialType == SpecialType.System_Double)
								{
									return;
								}
							}
							break;
					}

					// Not a valid literal.
					throw new SlimCSCompilationException(node, $"SlimCS does not support literals of type '{type}' outside of an explicit cast to int or double.");
				}
			}
		}

		public static void Enforce(this Model model, LocalDeclarationStatementSyntax node)
		{
			model.Enforce(node.Declaration);
		}

		public static void Enforce(this Model model, LocalFunctionStatementSyntax node)
		{
			// Current constraints:
			// - async modifier is not supported.
			if (node.Modifiers.HasToken(SyntaxKind.AsyncKeyword))
			{
				throw new UnsupportedNodeException(node, "the 'async' keyword is not supported.");
			}

			// Also validate parameters.
			model.Enforce(node.ParameterList);
		}

		public static void Enforce(this Model model, MemberAccessExpressionSyntax node)
		{
			// Current constraints:
			// - Not allowed to access any .NET system types except for a limited subset.
			// - <literal>.<something> is not supported unless <something> is a constant,
			//   the entire expression evaluates to an extension method, or it is one
			//   of our (limited) supported subset (ToString).
			// - the entire expression must evaluate to a user defined value,
			//   a member of the SlimcCS library static class, or one of a limited number
			//   of supported builtins (e.g. ToString(), GetType(), .Length), or a member
			//   of an interface.

			// Anything that is fully constant is allowed.
			var constant = model.GetConstantValue(node);
			if (constant.HasValue)
			{
				return;
			}

			var type = model.GetTypeInfo(node).Type;
			if (null != type)
			{
				model.Enforce(node, type);
			}

			var sym = model.GetSymbolInfo(node).Symbol;
			if (node.Expression is LiteralExpressionSyntax)
			{
				var methodSym = sym as IMethodSymbol;
				if (null == methodSym ||
					(!methodSym.IsExtensionMethod && !methodSym.IsCsharpToString()))
				{
					throw new SlimCSCompilationException(node, "SlimCS does not support access of system properties via literal except for builtin constants.");
				}
			}

			if (null != sym)
			{
				if (!sym.DeclaringSyntaxReferences.IsDefaultOrEmpty ||
					sym.ContainingType.IsSlimCSSystemLib() ||
					sym.ContainingType?.TypeKind == TypeKind.Interface)
				{
					goto done;
				}

				var methodSym = sym as IMethodSymbol;
				if (null != methodSym)
				{
					if (methodSym.IsCsharpToString() ||
						methodSym.IsGetType() ||
						methodSym.IsStringNullOrEmpty() ||
						methodSym.IsSystemActivatorCreateInstance())
					{
						goto done;
					}
				}

				var propSym = sym as IPropertySymbol;
				if (null != propSym)
				{
					if (propSym.IsArrayLength() ||
						propSym.IsStringLength() ||
						propSym.IsExceptionMessage() ||
						propSym.IsMemberInfoName())
					{
						goto done;
					}
				}

				var typeSym = sym as ITypeSymbol;
				if (null != typeSym)
				{
					if (typeSym.IsValidSlimCSType())
					{
						goto done;
					}
				}

				throw new SlimCSCompilationException(node, $"SlimCS does not support '{sym}'.");
			}
			done:
			return;
		}

		public static void Enforce(this Model model, MethodDeclarationSyntax node)
		{
			// Current constraints:
			// - async modifier is not supported.
			// - pure methods must be pure (no side effects).
			if (node.Modifiers.HasToken(SyntaxKind.AsyncKeyword))
			{
				throw new UnsupportedNodeException(node, "the 'async' keyword is not supported.");
			}

			// Pure enforcement.
			var methodSym = model.GetDeclaredSymbol(node);
			if (methodSym.HasPureAttribute())
			{
				// Find the body to traverse.
				var body = (node.Body != null ? (CSharpSyntaxNode)node.Body : node.ExpressionBody?.Expression);
				if (null != body && methodSym.ContainingType != null)
				{
					model.EnforcePure(body, methodSym.ContainingType);
				}
			}

			// Also validate parameters.
			model.Enforce(node.ParameterList);
		}

        public static void Enforce(this Model model, OperatorDeclarationSyntax node)
        {
			// Current constraints:
			// - async modifier is not supported.
			if (node.Modifiers.HasToken(SyntaxKind.AsyncKeyword))
            {
                throw new UnsupportedNodeException(node, "the 'async' keyword is not supported.");
            }

            // Also validate parameters.
            model.Enforce(node.ParameterList);
        }

        public static void Enforce(this Model model, ParameterListSyntax node)
		{
			foreach (var prm in node.Parameters)
			{
				if (null != prm.Type)
				{
					var type = model.GetTypeInfo(prm.Type).ConvertedType;
					if (null != type)
					{
						if (type.IsTupleType)
						{
							throw new UnsupportedNodeException(node, "SlimCS does not support tuples as single arguments.");
						}
						if (type.TypeKind == TypeKind.Dynamic)
						{
							throw new UnsupportedNodeException(node, "SlimCS does not support the dynamic type.");
						}

						// Final check.
						Enforce(model, prm, type);
					}
				}

				if (prm.Modifiers.HasToken(SyntaxKind.RefKeyword) ||
					prm.Modifiers.HasToken(SyntaxKind.OutKeyword))
				{
					throw new UnsupportedNodeException(node, "'ref' and 'out' parameters are not supported.");
				}
			}
		}

		public static void Enforce(this Model model, ParenthesizedLambdaExpressionSyntax node)
		{
			// Also validate parameters.
			model.Enforce(node.ParameterList);
		}

		public static void Enforce(this Model model, PredefinedTypeSyntax node)
		{
			switch (node.Keyword.Kind())
			{
				// Supported predefined types in SlimCS are: double, int,
				// object, string, and bool.
				case SyntaxKind.BoolKeyword:
				case SyntaxKind.DoubleKeyword:
				case SyntaxKind.IntKeyword:
				case SyntaxKind.ObjectKeyword:
				case SyntaxKind.StringKeyword:
					return;

				// We also allow predefined types of: float, sbyte, byte,
				// short, ushort, uint, long, and ulong in preparation for
				// SlimCS (v2).
				case SyntaxKind.FloatKeyword:
				case SyntaxKind.SByteKeyword:
				case SyntaxKind.ByteKeyword:
				case SyntaxKind.LongKeyword:
				case SyntaxKind.ShortKeyword:
				case SyntaxKind.UShortKeyword:
				case SyntaxKind.UIntKeyword:
				case SyntaxKind.ULongKeyword:
					return;

				default:
					throw new SlimCSCompilationException(node, $"predefined type '{node.Keyword}' is not supported by SlimCS.");
			}
		}

		public static void Enforce(this Model model, PropertyDeclarationSyntax node)
		{
			if (node.ExpressionBody != null)
			{
				throw new SlimCSCompilationException(node, "expression body property not yet supported.");
			}

			// Also apply declaration constraints.
			if (node.Initializer != null)
			{
				var to = model.GetTypeInfo(node.Type).ConvertedType;
				model.Enforce(to, node.Initializer);
			}

			// Pure enforcement.
			foreach (var accessor in node.AccessorList.Accessors)
			{
				var accessSym = model.GetDeclaredSymbol(accessor);
				if (accessSym.HasPureAttribute())
				{
					// Find the body to traverse.
					var body = (accessor.Body != null ? (CSharpSyntaxNode)accessor.Body : accessor.ExpressionBody?.Expression);
					if (null != body && accessSym.ContainingType != null)
					{
						model.EnforcePure(body, accessSym.ContainingType);
					}
				}
			}
		}

		public static void Enforce(
			this Model model,
			ReturnStatementSyntax node,
			ITypeSymbol contextReturnType)
		{
		}

		public static void Enforce(this Model model, VariableDeclarationSyntax node)
		{
			// Current constraints:
			// - locals, members, and other variable declarations cannot be tuple types.
			// - variables have a limited number of supported types in SlimCS.
			var to = model.GetTypeInfo(node.Type).ConvertedType;
			if (to != null && to.IsTupleType)
			{
				throw new UnsupportedNodeException(node, "SlimCS does not support variables of type tuple, only return values.");
			}

			// If a builtin special type, limited set.
			if (!to.IsValidSlimCSType())
			{
				throw new UnsupportedNodeException(node, $"SlimCS does not support variables of type '{to}'.");
			}

			foreach (var v in node.Variables)
			{
				model.Enforce(to, v.Initializer);
			}
		}
	}
}
