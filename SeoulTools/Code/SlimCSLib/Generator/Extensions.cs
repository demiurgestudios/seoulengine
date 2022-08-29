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
using System.Collections.Immutable;
using static SlimCSLib.Generator.Constants;

namespace SlimCSLib.Generator
{
	/// <summary>
	/// Miscellaneous extension methods used by Visitor.
	/// </summary>
	public static class Extensions
	{
		public static T Back<T>(this List<T> list)
		{
			return list[list.Count - 1];
		}

		public static bool CanCollapseFunctionPreDeclare(
			this LocalDeclarationStatementSyntax nodeA,
			AssignmentExpressionSyntax nodeB,
			Model model)
		{
			var vars = nodeA.Declaration.Variables;
			if (vars.Count != 1)
			{
				return false;
			}

			if (null == vars[0].Initializer)
			{
				return false;
			}

			var constant = model.GetConstantValue(vars[0].Initializer.Value);
			if (!constant.HasValue || !(constant.Value is null))
			{
				return false;
			}

			var symA = model.GetDeclaredSymbol(vars[0]);
			var symB = model.GetSymbolInfo(nodeB.Left).Symbol;
			if (symA != symB)
			{
				return false;
			}

			var lambda = nodeB.Right.TryResolveLambda();
			if (null == lambda)
			{
				return false;
			}

			return true;
		}

		public static bool CanConvertToIdent(this ImplicitElementAccessSyntax node)
		{
			// To convert the access to an ident, we must
			// be a key in an assignment within an object creation expression
			// (which will be converted to a table in Lua).
			//
			// e.g.
			// {
			//   ["foo"] = "bar",
			// }
			var parent = node.Parent;
			var assign = parent as AssignmentExpressionSyntax;
			if (null == parent)
			{
				return false;
			}

			// We must be on the left side of the assignment.
			if (assign.Left != node)
			{
				return false;
			}

			// Assignment parent must be an object creation expression.
			if (assign.Parent.Kind() != SyntaxKind.ObjectInitializerExpression)
			{
				return false;
			}

			// Need to have a single lookup in our expression (e.g. ["foo"]).
			if (node.ArgumentList == null || node.ArgumentList.Arguments.Count != 1)
			{
				return false;
			}

			// Expression must be a string and that string must be a valid lua ident.
			var expr = node.ArgumentList.Arguments[0].Expression;
			if (expr.Kind() != SyntaxKind.StringLiteralExpression)
			{
				return false;
			}

			// Check the string for ident.
			var s = ((LiteralExpressionSyntax)expr).Token.ValueText;
			if (!s.IsValidLuaIdent())
			{
				return false;
			}

			// Success.
			return true;
		}

		public static bool CanAlwaysRemoveParens(this ExpressionSyntax node)
		{
			var eKind = node.Kind();
			switch (eKind)
			{
				// Binary expressions that will be converted to bit ops can have parens removed.
				case SyntaxKind.BitwiseAndExpression:
				case SyntaxKind.BitwiseOrExpression:
				case SyntaxKind.LeftShiftExpression:
				case SyntaxKind.RightShiftExpression:
					return true;

				case SyntaxKind.CastExpression: return true;
				case SyntaxKind.IdentifierName: return true;
				case SyntaxKind.IdentifierToken: return true;
				case SyntaxKind.InvocationExpression: return true;
				case SyntaxKind.ObjectCreationExpression: return true;
				case SyntaxKind.ParenthesizedLambdaExpression: return true;
				case SyntaxKind.SimpleMemberAccessExpression: return true;
				default:
					if (node is LiteralExpressionSyntax)
					{
						return true;
					}
					else
					{
						return false;
					}
			}
		}

		/// <summary>
		/// Returns true if the declaration statement for this field can be omitted
		/// from Lua.
		/// </summary>
		/// <param name="node">Node to check.</param>
		/// <param name="model">Model to use for semantic knowledge.</param>
		/// <returns>true if it can be omitted, false otherwise.</returns>
		/// <remarks>
		/// Because all nodes are nil by default in Lua (there is no "undefined"),
		/// we don't need to emit declaration statements for instance members
		/// if they are reference types with a default value of null.
		/// </remarks>
		public static bool CanOmitDeclarationStatement(this FieldDeclarationSyntax node, Model model)
		{
			// Ref type instance members or nullable values with a default
			// value of null can just be omitted - they are implicitly nil in Lua by default.
			var decl = node.Declaration;
			var typeInfo = model.GetTypeInfo(decl.Type);
			var type = typeInfo.ConvertedType;
			if (!type.IsReferenceType && type.OriginalDefinition.SpecialType != SpecialType.System_Nullable_T)
			{
				return false;
			}

			// Now verify that all variables either have no initializer
			// or the initializer is a constant that evaluates to null.
			var bAllNull = true;
			var vars = decl.Variables;
			foreach (var v in vars)
			{
				// Easy case, no initializer.
				if (null == v.Initializer)
				{
					continue;
				}

				// Constant eval - if either not constant or not a constant
				// null, we're done.
				var constant = model.GetConstantValue(v.Initializer.Value);
				if (!constant.HasValue || !(constant.Value is null))
				{
					bAllNull = false;
					break;
				}
			}

			// All variables initialize null, so we can omit this declaration.
			if (bAllNull)
			{
				return true;
			}

			return false;
		}

		/// <summary>
		/// Returns true if the declaration statement for this property can be omitted
		/// from Lua.
		/// </summary>
		/// <param name="node">Node to check.</param>
		/// <param name="model">Model to use for semantic knowledge.</param>
		/// <returns>true if it can be omitted, false otherwise.</returns>
		/// <remarks>
		/// Because all nodes are nil by default in Lua (there is no "undefined"),
		/// we don't need to emit declaration statements for instance members
		/// if they are reference types with a default value of null.
		/// </remarks>
		public static bool CanOmitDeclarationStatement(this PropertyDeclarationSyntax node, Model model)
		{
			// Ref type properties with a default value of null can
			// just be omitted - they are implicitly nil in Lua by default.
			var typeInfo = model.GetTypeInfo(node.Type);
			var type = typeInfo.ConvertedType;
			if (!type.IsReferenceType && type.OriginalDefinition.SpecialType != SpecialType.System_Nullable_T)
			{
				return false;
			}

			// Easy case, no initializer.
			if (null == node.Initializer)
			{
				return true;
			}
			else
			{
				// If we have a constant initializer that's equal
				// to null, we can omit the declaration.
				var constant = model.GetConstantValue(node.Initializer.Value);
				if (constant.HasValue && constant.Value is null)
				{
					return true;
				}
			}

			return false;
		}

		/// <summary>
		/// Returns true if the declaration statement for this member can be omitted
		/// from Lua.
		/// </summary>
		public static bool CanOmitDeclarationStatement(this MemberDeclarationSyntax node, Model model)
		{
			switch (node.Kind())
			{
				case SyntaxKind.FieldDeclaration: return ((FieldDeclarationSyntax)node).CanOmitDeclarationStatement(model);
				case SyntaxKind.PropertyDeclaration: return ((PropertyDeclarationSyntax)node).CanOmitDeclarationStatement(model);
				default:
					return false;
			}
		}

		public static SyntaxKind ExpressionKind(this SyntaxToken tok, SyntaxNode context)
		{
			switch (tok.Kind())
			{
				case SyntaxKind.FalseKeyword: return SyntaxKind.FalseLiteralExpression;
				case SyntaxKind.NullKeyword: return SyntaxKind.NullLiteralExpression;
				case SyntaxKind.NumericLiteralToken: return SyntaxKind.NumericLiteralExpression;
				case SyntaxKind.StringLiteralToken: return SyntaxKind.StringLiteralExpression;
				case SyntaxKind.TrueKeyword: return SyntaxKind.TrueLiteralExpression;
				default:
					if (null != context)
					{
						throw new SlimCSCompilationException(context, $"unsupported lit '{tok.Kind()}' to expression conversion.");
					}
					else
					{
						throw new SlimCSCompilationException($"unsupported lit '{tok.Kind()}' to expression conversion.");
					}
			}
		}

		public static ISymbol First(this SymbolInfo info)
		{
			return info.Symbol ?? (info.CandidateSymbols.IsDefaultOrEmpty ? null : info.CandidateSymbols[0]);
		}

		public static string FullNamespace(this INamespaceSymbol sym)
		{
			string sReturn = string.Empty;
			if (null != sym.ContainingNamespace &&
				!sym.ContainingNamespace.IsGlobalNamespace)
			{
				sReturn = FullNamespace(sym.ContainingNamespace) + ".";
			}

			if (!sym.IsGlobalNamespace)
			{
				sReturn += sym.Name;
			}

			return sReturn;
		}

		public static BaseTypeDeclarationSyntax GetContainingTypeSyntax(this SyntaxNode node)
		{
			var ret = node.Parent;
			while (ret != null && !(ret is BaseTypeDeclarationSyntax))
			{
				ret = ret.Parent;
			}

			return (BaseTypeDeclarationSyntax)ret;
		}

		public static (SeparatedSyntaxList<ParameterSyntax>, bool) GetParameterList(this AccessorDeclarationSyntax node)
		{
			var parent = node.Parent.Parent;
			switch (parent.Kind())
			{
				case SyntaxKind.EventDeclaration:
					return (kEmptyParameters, true);
				case SyntaxKind.IndexerDeclaration:
					return (((IndexerDeclarationSyntax)parent).ParameterList.Parameters, (node.Kind() == SyntaxKind.SetAccessorDeclaration));
				case SyntaxKind.PropertyDeclaration:
					return (kEmptyParameters, (node.Kind() == SyntaxKind.SetAccessorDeclaration));
				default:
					throw new SlimCSCompilationException(node, $"unsupported accessor parent node kind '{parent.Kind()}'.");
			}
		}

		public static int GetEndLine(this SyntaxNode node)
		{
			var syntaxTree = node.SyntaxTree;
			var textSpan = node.Span;
			var pos = syntaxTree.GetLineSpan(node.Span);

			return pos.EndLinePosition.Line;
		}

		// Tricky - Roslyn does not provide a straightforward way to map an argument to a parameter
		// (to get the parameter type).
		public static IParameterSymbol GetParameter(this SemanticModel model, ArgumentSyntax arg)
		{
			var sym = model.GetSymbolInfo(arg.Parent.Parent).First() as IMethodSymbol;
			return sym?.GetParameter(arg);
		}

		public static IParameterSymbol GetParameter(this IMethodSymbol sym, ArgumentSyntax arg)
		{
			string sName = null;
			if (arg.NameColon?.Name != null)
			{
				sName = arg.NameColon.Name.Identifier.ValueText;
				foreach (var prm in sym.Parameters)
				{
					if (prm.Name == sName) { return prm; }
				}
			}
			else
			{
				var pp = arg.Parent.Parent;
				var iIndex = -1;
				if (pp is InvocationExpressionSyntax invoke)
				{
					var args = invoke.ArgumentList.Arguments;
					for (int i = 0; i < args.Count; ++i)
					{
						if (args[i] == arg)
						{
							iIndex = i;
							break;
						}
					}
				}
				else if (pp is ObjectCreationExpressionSyntax create)
				{
					var args = create.ArgumentList.Arguments;
					for (int i = 0; i < args.Count; ++i)
					{
						if (args[i] == arg)
						{
							iIndex = i;
							break;
						}
					}
				}
				else
				{
					// TODO:
					return null;
				}

				if (iIndex >= 0)
				{
					var prms = sym.Parameters;
					if (iIndex >= prms.Length)
					{
						if (!prms.IsEmpty && prms[prms.Length - 1].IsParams)
						{
							return prms[prms.Length - 1];
						}
						else
						{
							// TODO:
							return null;
						}
					}
					else
					{
						return prms[iIndex];
					}
				}
			}

			return null;
		}

		public static int GetStartColumn(this SyntaxNode node)
		{
			var syntaxTree = node.SyntaxTree;
			var textSpan = node.Span;
			var pos = syntaxTree.GetLineSpan(node.Span);

			return pos.StartLinePosition.Character;
		}

		public static int GetStartLine(this SyntaxNode node)
		{
			var syntaxTree = node.SyntaxTree;
			var textSpan = node.Span;
			var pos = syntaxTree.GetLineSpan(node.Span);

			return pos.StartLinePosition.Line;
		}

		public static int GetStartLine(this SyntaxToken token)
		{
			var syntaxTree = token.SyntaxTree;
			var textSpan = token.Span;
			var pos = syntaxTree.GetLineSpan(token.Span);

			return pos.StartLinePosition.Line;
		}

		public static int GetStartColumn(this SyntaxToken token)
		{
			var syntaxTree = token.SyntaxTree;
			var textSpan = token.Span;
			var pos = syntaxTree.GetLineSpan(token.Span);

			return pos.StartLinePosition.Character;
		}

		public static int GetStartColumn(this SyntaxTrivia trivia)
		{
			var syntaxTree = trivia.SyntaxTree;
			var textSpan = trivia.Span;
			var pos = syntaxTree.GetLineSpan(textSpan);

			return pos.StartLinePosition.Character;
		}

		public static int GetStartLine(this SyntaxTrivia trivia)
		{
			var syntaxTree = trivia.SyntaxTree;
			var textSpan = trivia.Span;
			var pos = syntaxTree.GetLineSpan(textSpan);

			return pos.StartLinePosition.Line;
		}

		public static bool HashInstanceThisContext(this Model model, SyntaxNode node)
		{
			var orig = node;
			while (null != node)
			{
				if (node is BaseMethodDeclarationSyntax method)
				{
					var sym = model.GetDeclaredSymbol(method);
					if (sym.IsStatic) { return false; }

					var body = (SyntaxNode)method.Body ?? method.ExpressionBody;
					return body.Contains(orig);
				}

				node = node.Parent;
			}

			return false;
		}

		public static bool HasVarargs(this ImmutableArray<IParameterSymbol> prms)
		{
			if (prms.Length == 0)
			{
				return false;
			}

			return prms[prms.Length - 1].IsParams;
		}

		public static bool HasToken(this SyntaxTokenList list, SyntaxKind eKind)
		{
			foreach (var v in list)
			{
				if (v.Kind() == eKind)
				{
					return true;
				}
			}

			return false;
		}

		public static int IndexOf(this ImmutableArray<IParameterSymbol> prms, NameColonSyntax syntax)
		{
			for (int i = 0; i < prms.Length; ++i)
			{
				if (prms[i].Name == syntax.Name.Identifier.ValueText)
				{
					return i;
				}
			}

			return -1;
		}

		public static bool AllDiscarded(this ExpressionSyntax expr, Model model)
		{
			if (expr is TupleExpressionSyntax tuple)
			{
				foreach (var arg in tuple.Arguments)
				{
					if (!arg.IsDiscard(model))
					{
						return false;
					}
				}

				return true;
			}
			else
			{
				return expr.IsDiscard(model);
			}
		}

		public static bool IsDiscard(this ExpressionSyntax expr, Model model)
		{
			if (expr is IdentifierNameSyntax ident)
			{
				// TODO: Better way to filter for this?
				if (ident.Identifier.ValueText == "_")
				{
					var sym = model.GetSymbolInfo(ident).Symbol;
					if (sym?.Kind == SymbolKind.Discard)
					{
						return true;
					}
				}
			}
			else if (expr is DeclarationExpressionSyntax decl)
			{
				if (decl.Designation is DiscardDesignationSyntax)
				{
					return true;
				}
			}

			return false;
		}

		public static bool IsDiscard(this ArgumentSyntax arg, Model model)
		{
			return arg.Expression.IsDiscard(model);
		}

		public static (bool, bool) GetArrayFeatures(this Model model, ExpressionSyntax node)
		{
			var typeInfo = model.GetTypeInfo(node);

			// Early out, no type data.
			var type = typeInfo.ConvertedType;
			if (null == type)
			{
				return (false, false);
			}

			// Derive - 0 based if a builtin array, ref type
			// based on element type.
			var b0Based = (type.Kind == SymbolKind.ArrayType);
			var bNeedsArrayPlaceholder = false;
			if (b0Based)
			{
				var arrayType = (IArrayTypeSymbol)type;
				bNeedsArrayPlaceholder = arrayType.ElementType.NeedsArrayPlaceholder();
			}
			return (b0Based, bNeedsArrayPlaceholder);
		}

		public static bool HasInstanceInvoke(this IMethodSymbol sym)
		{
			switch (sym.MethodKind)
			{
				case MethodKind.AnonymousFunction:
				case MethodKind.BuiltinOperator:
				case MethodKind.DelegateInvoke:
				case MethodKind.LocalFunction:
				case MethodKind.ReducedExtension:
				case MethodKind.StaticConstructor:
					return false;

				case MethodKind.Constructor:
				case MethodKind.Destructor:
				case MethodKind.EventAdd:
				case MethodKind.EventRaise:
				case MethodKind.EventRemove:
				case MethodKind.ExplicitInterfaceImplementation:
				case MethodKind.Ordinary:
				case MethodKind.PropertyGet:
				case MethodKind.PropertySet:
				case MethodKind.UserDefinedOperator:
					return !sym.IsStatic;

				default:
					throw new SlimCSCompilationException($"unsupported method kind '{sym.MethodKind}'.");
			}
		}

		public static bool IsArrayLength(this ISymbol sym)
		{
			var propSym = sym as IPropertySymbol;
			if (null != propSym &&
				propSym.Name == "Length" &&
				propSym.ContainingType.SpecialType == SpecialType.System_Array)
			{
				return true;
			}

			return false;
		}

		public static bool IsStringLength(this ISymbol sym)
		{
			var propSym = sym as IPropertySymbol;
			if (null != propSym &&
				propSym.Name == "Length" &&
				propSym.ContainingType.SpecialType == SpecialType.System_String)
			{
				return true;
			}

			return false;
		}

		public static bool IsExceptionMessage(this ISymbol sym)
		{
			var propSym = sym as IPropertySymbol;
			if (null != propSym &&
				propSym.Name == "Message" &&
				propSym.ContainingType.IsSystemException())
			{
				return true;
			}

			return false;
		}

		public static bool IsMemberInfoName(this ISymbol sym)
		{
			var propSym = sym as IPropertySymbol;
			if (null != propSym &&
				propSym.Name == "Name" &&
				propSym.ContainingType.IsSystemReflectionMemberInfo())
			{
				return true;
			}

			return false;
		}

		public static bool IsSystemDelegate(this ITypeSymbol sym)
		{
			if (null != sym.ContainingNamespace &&
				sym.ContainingNamespace.Name == "System" &&
				(sym.Name == "Delegate" || sym.BaseType.Name == "Delegate"))
			{
				return true;
			}

			return false;
		}

		public static bool IsSystemException(this ITypeSymbol sym)
		{
			if (null != sym.ContainingNamespace &&
				sym.ContainingNamespace.Name == "System" &&
				(sym.Name == "Exception" || sym.Name == "NullArgumentException"))
			{
				return true;
			}

			return false;
		}

		public static bool IsSystemReflectionMemberInfo(this ITypeSymbol sym)
		{
			if (sym.Name == "MemberInfo" &&
				sym?.ContainingNamespace?.Name == "Reflection" &&
				sym?.ContainingNamespace?.ContainingNamespace?.Name == "System")
			{
				return true;
			}

			return false;
		}

		public static bool IsSystemType(this ITypeSymbol sym)
		{
			if (null != sym.ContainingNamespace &&
				sym.ContainingNamespace.Name == "System" &&
				sym.Name == "Type")
			{
				return true;
			}

			return false;
		}

		public static bool IsAscii(this char c)
		{
			return ((int)c < 128);
		}

		public static bool IsAsciiDigit(this char c)
		{
			return (c >= '0' && c <= '9');
		}

		public static bool IsAsciiLetter(this char c)
		{
			return
				(c >= 'A' && c <= 'Z') ||
				(c >= 'a' && c <= 'z');
		}

		public static bool IsAutomatic(this PropertyDeclarationSyntax node)
		{
			// Null accessor list is also automatic.
			if (null == node.AccessorList)
			{
				return true;
			}

			// Abstract/extern property is not automatic.
			if (node.Modifiers.IsAbstractOrExtern())
			{
				return false;
			}

			var accessors = node.AccessorList;
			foreach (var access in accessors.Accessors)
			{
				if (access.Body != null ||
					access.ExpressionBody != null)
				{
					return false;
				}
			}

			return true;
		}

		public static bool IsAutoProperty(this IPropertySymbol sym)
		{
			// Resolve.
			sym = sym.OriginalDefinition;

			// Punch through using Reflection, not a public property.
			var type = sym.GetType();
			var prop = type.GetProperty(
				"IsAutoProperty",
				System.Reflection.BindingFlags.NonPublic |
				System.Reflection.BindingFlags.Instance);
			if (null == prop)
			{
				return false;
			}

			var bAuto = (bool)prop.GetValue(sym);
			return bAuto;
		}

		public static bool ShouldSynthesizeAccessors(this Model model, PropertyDeclarationSyntax node)
		{
			var sym = model.GetDeclaredSymbol(node);

			// Only applicable to an auto property.
			if (!sym.IsAutoProperty())
			{
				return false;
			}

			// If an auto property that overrides another property, must synthesize accessors.
			if (null != sym.OverriddenProperty)
			{
				return true;
			}

			// If an auto property that has explicit interface implementations, must synthesize accessors.
			if (!sym.ExplicitInterfaceImplementations.IsEmpty)
			{
				return true;
			}

			// Iterate all interfaces of the containing type, and check if
			// sym is the implementation for one of their interface members.
			var containingType = sym.ContainingType;
			var interfs = containingType.AllInterfaces;
			foreach (var interf in interfs)
			{
				var mems = interf.GetMembers(sym.Name);
				foreach (var mem in mems)
				{
					if (mem is IPropertySymbol other)
					{
						// Implements an interface member.
						if (containingType.FindImplementationForInterfaceMember(other) == sym)
						{
							return true;
						}
					}
				}
			}

			// Does not require synthesized accessors.
			return false;
		}

		public static bool IsBase(this MemberAccessEmitType eType)
		{
			switch (eType)
			{
				case MemberAccessEmitType.BaseAccess:
				case MemberAccessEmitType.BaseEventAdd:
				case MemberAccessEmitType.BaseEventRaise:
				case MemberAccessEmitType.BaseEventRemove:
				case MemberAccessEmitType.BaseGetter:
				case MemberAccessEmitType.BaseMethod:
				case MemberAccessEmitType.BaseSetter:
					return true;
				default:
					return false;
			}
		}

		public static bool IsComment(this SyntaxKind kind)
		{
			switch (kind)
			{
				case SyntaxKind.MultiLineCommentTrivia:
				case SyntaxKind.MultiLineDocumentationCommentTrivia:
				case SyntaxKind.SingleLineCommentTrivia:
				case SyntaxKind.SingleLineDocumentationCommentTrivia:
					return true;
				default:
					return false;
			}
		}

		public static bool IsCsharpToString(this IMethodSymbol sym)
		{
			while (sym.OverriddenMethod != null)
			{
				sym = sym.OverriddenMethod;
			}

			if (sym.Name == "ToString" &&
				sym.ContainingType.SpecialType == SpecialType.System_Object)
			{
				return true;
			}

			return false;
		}

		public static bool IsLuaToString(this IMethodSymbol sym)
		{
			if (sym.ContainingType.IsSlimCSSystemLib() &&
				sym.Name == "tostring")
			{
				return true;
			}

			return false;
		}

		public static bool IsDeclarationSafeMemberAccess(this MemberAccessExpressionSyntax node)
		{
			if (node.Expression is MemberAccessExpressionSyntax)
			{
				if (!((MemberAccessExpressionSyntax)node.Expression).IsDeclarationSafeMemberAccess())
				{
					return false;
				}
			}
			else if (!(node.Expression is IdentifierNameSyntax))
			{
				return false;
			}

			return true;
		}

		public static bool IsDefaultCase(this SyntaxList<SwitchLabelSyntax> modifiers)
		{
			foreach (var e in modifiers)
			{
				if (e.Kind() == SyntaxKind.DefaultSwitchLabel)
				{
					return true;
				}
			}

			return false;
		}

		public static bool IsGetType(this IMethodSymbol sym)
		{
			while (sym.OverriddenMethod != null)
			{
				sym = sym.OverriddenMethod;
			}

			if (sym.Name == "GetType" &&
				sym.ContainingType.SpecialType == SpecialType.System_Object)
			{
				return true;
			}

			return false;
		}

		public static bool IsStringNullOrEmpty(this IMethodSymbol sym)
		{
			if (sym.Name == "IsNullOrEmpty" &&
				sym.ContainingType.SpecialType == SpecialType.System_String)
			{
				return true;
			}

			return false;
		}

		public static bool IsSystemActivatorType(this ITypeSymbol sym)
		{
			if (null != sym.ContainingNamespace &&
				sym.ContainingNamespace.Name == "System" &&
				sym.Name == "Activator")
			{
				return true;
			}

			return false;
		}

		public static bool IsSystemActivatorCreateInstance(this IMethodSymbol sym)
		{
			if (sym.Name == "CreateInstance" &&
				sym.ContainingType?.IsSystemActivatorType() == true)
			{
				return true;
			}

			return false;
		}

		public static bool IsInstanceMember(this SyntaxTokenList list)
		{
			foreach (var v in list)
			{
				var eKind = v.Kind();
				if (eKind == SyntaxKind.ConstKeyword ||
					eKind == SyntaxKind.StaticKeyword)
				{
					return false;
				}
			}

			return true;
		}

		public static int IntegralSizeInBytes(this SpecialType eType)
		{
			switch (eType)
			{
				case SpecialType.System_Byte: return 1;
				case SpecialType.System_Int16: return 2;
				case SpecialType.System_Int32: return 4;
				case SpecialType.System_Int64: return 8;
				case SpecialType.System_SByte: return 1;
				case SpecialType.System_UInt16: return 2;
				case SpecialType.System_UInt32: return 4;
				case SpecialType.System_UInt64: return 8;
				default:
					throw new SlimCSCompilationException($"{eType} is not integral.");
			}
		}

		public static bool IsIntegralType(this SpecialType eType)
		{
			switch (eType)
			{
				case SpecialType.System_Byte:
				case SpecialType.System_Int16:
				case SpecialType.System_Int32:
				case SpecialType.System_Int64:
				case SpecialType.System_SByte:
				case SpecialType.System_UInt16:
				case SpecialType.System_UInt32:
				case SpecialType.System_UInt64:
					return true;
				default:
					return false;
			}
		}

		public static bool IsLHS(this SyntaxNode node)
		{
			var parent = node.Parent;
			while (null != parent)
			{
				if (parent is AssignmentExpressionSyntax)
				{
					var assign = (AssignmentExpressionSyntax)parent;
					if (assign.Left == node)
					{
						return true;
					}
				}

				if (parent is StatementSyntax)
				{
					return false;
				}

				node = parent;
				parent = parent.Parent;
			}

			return false;
		}

		public static bool IsLuaDyncast(this ExpressionSyntax node)
		{
			var ident = (node as SimpleNameSyntax);
			if (null == ident)
			{
				return false;
			}

			return ident.Identifier.ValueText == "dyncast"; // TODO: Use semantic model.
		}

		public static bool IsLuaAsTable(this ExpressionSyntax node)
		{
			var ident = (node as SimpleNameSyntax);
			if (null == ident)
			{
				return false;
			}

			return ident.Identifier.ValueText == "astable"; // TODO: Use semantic model.
		}

		public static bool IsLuaClassInitPseudo(this MethodDeclarationSyntax node)
		{
			return node.Identifier.ValueText == "__cinit"; // TODO: Use semantic model.
		}

		public static bool IsLuaConcat(this ExpressionSyntax node)
		{
			var ident = (node as SimpleNameSyntax);
			if (null == ident)
			{
				return false;
			}

			return ident.Identifier.ValueText == "concat"; // TODO: Use semantic model.
		}

		public static bool IsLuaNot(this ExpressionSyntax node)
		{
			var ident = (node as SimpleNameSyntax);
			if (null == ident)
			{
				return false;
			}

			return ident.Identifier.ValueText == "not"; // TODO: Use semantic model.
		}

		public static bool IsLuaFunctionPseudo(this ExpressionSyntax node)
		{
			var ident = (node as SimpleNameSyntax);
			if (null == ident)
			{
				return false;
			}

			return ident.Identifier.ValueText == "function"; // TODO: Use semantic model.
		}

		public static bool IsSlimCSSystemLib(this ISymbol sym)
		{
			if (null == sym) { return false; }

			if (sym.ContainingType == null &&
				sym.Name == "SlimCS" &&
				sym.ContainingNamespace.IsGlobalNamespace)
			{
				return true;
			}

			return false;
		}

		public static bool IsLowerPrecedenceThanLuaConcat(this ExpressionSyntax node)
		{
			if (node is BinaryExpressionSyntax binary)
			{
				// Comparisons, 'and', and 'or' are lower precedence.
				switch (binary.OperatorToken.Kind())
				{
						// Lower precedence.
					case SyntaxKind.AmpersandAmpersandToken:
					case SyntaxKind.BarBarToken:
					case SyntaxKind.QuestionQuestionToken:
					case SyntaxKind.EqualsEqualsToken:
					case SyntaxKind.ExclamationEqualsToken:
					case SyntaxKind.GreaterThanEqualsToken:
					case SyntaxKind.GreaterThanToken:
					case SyntaxKind.LessThanEqualsToken:
					case SyntaxKind.LessThanToken:
						return true;

						// Higher precedence - add, divide, multiply, subtract
					case SyntaxKind.AsteriskToken:
					case SyntaxKind.MinusToken:
					case SyntaxKind.PlusToken:
					case SyntaxKind.SlashToken:
						return false;

					default:
						throw new SlimCSCompilationException(node, $"unsupported binary op kind {binary.OperatorToken.Kind()} in lua concat.");
				}
			}

			return false;
		}

		public static bool IsLuaLength(this ExpressionSyntax node)
		{
			var ident = (node as SimpleNameSyntax);
			if (null == ident)
			{
				return false;
			}

			return ident.Identifier.ValueText == "lenop"; // TODO: Use semantic model.
		}

		public static bool IsLuaRange(this ExpressionSyntax node)
		{
			var ident = (node as SimpleNameSyntax);
			if (null == ident)
			{
				return false;
			}

			return ident.Identifier.ValueText == "range"; // TODO: Use semantic model.
		}

		public static bool IsLuaDval(this ITypeSymbol sym)
		{
			if (null == sym.ContainingType ||
				!sym.ContainingType.IsSlimCSSystemLib())
			{
				return false;
			}

			return (sym.Name == "dval");
		}

		public static bool IsLuaTable(this ITypeSymbol sym)
		{
			if (null == sym.ContainingType ||
				!sym.ContainingType.IsSlimCSSystemLib())
			{
				return false;
			}

			while (
				sym.BaseType != null &&
				sym.BaseType.ContainingType != null &&
				sym.BaseType.ContainingType.IsSlimCSSystemLib())
			{
				sym = sym.BaseType;
			}

			return (sym.Name == "Table");
		}

		public static bool IsLuaTuple(this ITypeSymbol sym)
		{
			if (null == sym.ContainingType ||
				!sym.ContainingType.IsSlimCSSystemLib())
			{
				return false;
			}

			return (sym.Name == "Tuple");
		}

		public static bool IsNonStaticMethodCall(this SimpleNameSyntax node, ISymbol sym, bool bInvoke)
		{
			if (!bInvoke)
			{
				var parent = node.Parent;
				if (!(parent is InvocationExpressionSyntax))
				{
					// Can't be a call if not an invocation.
					return false;
				}

				var invoke = (InvocationExpressionSyntax)parent;

				// node must be the expression.
				if (node != invoke.Expression)
				{
					return false;
				}
			}

			// Check if we're a method and not static.
			if (sym.IsStatic)
			{
				return false;
			}

			return sym.Kind == SymbolKind.Method;
		}

		/// <summary>
		/// Checks whether a node described by the given modifiers
		/// is static and/or private.
		/// </summary>
		/// <param name="list">Modifiers to check.</param>
		/// <returns>A tuple describing static (first) and private (second).</returns>
		public static (bool, bool) IsConstOrStaticAndPrivate(this SyntaxTokenList mods)
		{
			bool bPrivate = true;
			bool bConstOrStatic = false;
			foreach (var tok in mods)
			{
				var eKind = tok.Kind();
				switch (eKind)
				{
					case SyntaxKind.InternalKeyword:
					case SyntaxKind.ProtectedKeyword:
					case SyntaxKind.PublicKeyword:
						bPrivate = false;
						if (bConstOrStatic)
						{
							break;
						}
						break;

					case SyntaxKind.ConstKeyword:
					case SyntaxKind.StaticKeyword:
						bConstOrStatic = true;
						if (!bPrivate)
						{
							break;
						}
						break;
				}
			}

			return (bConstOrStatic, bPrivate);
		}

		public static bool IsAbstractOrExtern(this SyntaxTokenList mods)
		{
			return mods.HasToken(SyntaxKind.AbstractKeyword) || mods.HasToken(SyntaxKind.ExternKeyword);
		}

		public static bool IsValidAssignmentContext(this ExpressionSyntax node)
		{
			// Should never happen, handle gracefully.
			var parent = node.Parent;
			if (null == parent)
			{
				return false;
			}

			var eKind = parent.Kind();
			switch (eKind)
			{
				case SyntaxKind.ExpressionStatement:
					return true;

				// Ok in for statements if we're an incrementor or
				// an initializer.
				case SyntaxKind.ForStatement:
					{
						var forStmt = (ForStatementSyntax)parent;
						if (forStmt.Initializers != null)
						{
							if (forStmt.Initializers.Contains(node))
							{
								// We're an initializer, so the assignment
								// is ok in this case.
								return true;
							}
						}

						if (forStmt.Incrementors != null)
						{
							if (forStmt.Incrementors.Contains(node))
							{
								// We're an incrementor, so the assignment
								// is ok in this case.
								return true;
							}
						}
					}
					return false;

				case SyntaxKind.ObjectInitializerExpression:
					return true;
				default:
					return false;
			}
		}

		public static bool IsValidLuaIdent(this string s)
		{
			// Cannot be one of the lua reserved keywords.
			if (kLuaReserved.Contains(s))
			{
				return false;
			}

			if (string.IsNullOrEmpty(s))
			{
				return false;
			}

			if (!s[0].IsValidLuaIdentStartChar())
			{
				return false;
			}

			for (int i = 1; i < s.Length; ++i)
			{
				if (!s[i].IsValidLuaIdentChar())
				{
					return false;
				}
			}

			return true;
		}

		public static bool IsValidLuaIdentStartChar(this char c)
		{
			return c.IsAsciiLetter() || '_' == c;
		}

		public static bool IsValidLuaIdentChar(this char c)
		{
			return c.IsValidLuaIdentStartChar() || c.IsAsciiDigit();
		}

		public static string ToValidLuaIdent(this string s)
		{
			System.Text.StringBuilder ret = null;
			if (string.IsNullOrEmpty(s))
			{
				return s;
			}

			if (!s[0].IsValidLuaIdentStartChar())
			{
				ret = new System.Text.StringBuilder();
				ret.Append('_');
			}

			for (int i = 1; i < s.Length; ++i)
			{
				var ch = s[i];
				if (!ch.IsValidLuaIdentChar())
				{
					if (null == ret)
					{
						ret = new System.Text.StringBuilder();
						ret.Append(s, 0, i);
					}
					ret.Append('_');
				}
				else if (null != ret)
				{
					ret.Append(ch);
				}
			}

			if (null != ret)
			{
				return ret.ToString();
			}
			else
			{
				return s;
			}
		}

		public static bool IsValidSlimCSType(this ITypeSymbol type)
		{
			switch (type.SpecialType)
			{
				case SpecialType.None:
					switch (type.TypeKind)
					{
						case TypeKind.Class:
							if (type.DeclaringSyntaxReferences.IsDefaultOrEmpty)
							{
								// We need to support System.Delegate, System.Exception,
								// and System.Type.
								if (!type.IsSystemDelegate() &&
									!type.IsSystemException() &&
									!type.IsSystemType() &&
									!type.IsSystemActivatorType())
								{
									goto default;
								}
							}
							return true;

						case TypeKind.Error:
							// Although we don't "support" an error, we don't
							// want to flag it as it just prevents the compilation
							// error checking from being most prominent in the error
							// output.
							return true;

						case TypeKind.Array:
						case TypeKind.Delegate:
						case TypeKind.Enum:
						case TypeKind.Interface:
						case TypeKind.TypeParameter:
							return true;

						case TypeKind.Struct:
							if (type.OriginalDefinition.SpecialType != SpecialType.System_Nullable_T)
							{
								goto default;
							}
							return true;

						default:
							return false;
					}

				case SpecialType.System_Boolean:
				case SpecialType.System_Delegate:
				case SpecialType.System_Double:
				case SpecialType.System_Int32:
				case SpecialType.System_Object:
				case SpecialType.System_String:
					return true;

				default:
					return false;
			}
		}

		public static LuaKind GetLuaKind(this ISymbol sym)
		{
			// Can't be a special kind if not contained in the SlimCS system library static class.
			if (sym.ContainingType == null || !sym.ContainingType.IsSlimCSSystemLib())
			{
				return LuaKind.NotSpecial;
			}

			// Key off name.
			var sName = sym.Name;
			switch (sName)
			{
				case kLuaDyncast: return LuaKind.Dyncast;
				case kLuaDval: return LuaKind.Dval;
				case kLuaIpairs: return LuaKind.Ipairs;
				case kLuaOnInitProgress: return LuaKind.OnInitProgress;
				case kLuaPairs: return LuaKind.Pairs;
				case kPseudoTuple: return LuaKind.Tuple;
				case kLuaRequire: return LuaKind.Require;
				case kPseudoLuaTableTypeName: return LuaKind.Table;
				case kPseudoLuaTableVTypeName: return LuaKind.Table;
				default:
					return LuaKind.NotSpecial;
			}
		}

		public static bool NeedsDelimiters(this BlockSyntax node)
		{
			var eKind = node.Parent.Kind();
			switch (eKind)
			{
				case SyntaxKind.Block:
				case SyntaxKind.SwitchSection:
					return true;
				default:
					return false;
			}
		}

		public static void PopBack<T>(this List<T> list)
		{
			list.RemoveAt(list.Count - 1);
		}

		public static ISymbol Reduce(this ISymbol sym)
		{
			// Reduce and resolve if an extension or generic specialization of
			// a generic method.
			var methodSym = (sym as IMethodSymbol);
			if (null != methodSym)
			{
				return methodSym.Reduce();
			}

			return sym;
		}

		public static IMethodSymbol Reduce(this IMethodSymbol sym)
		{
			// Reduce and resolve if an extension or generic specialization of
			// a generic method.
			sym = sym.OriginalDefinition;
			if (sym.ReducedFrom != null) { sym = sym.ReducedFrom; }
			while (null != sym.OverriddenMethod)
			{
				sym = sym.OverriddenMethod;
			}

			return sym;
		}

		public static SyntaxNode RemoveParens(this SyntaxNode node)
		{
			var parens = node as ParenthesizedExpressionSyntax;
			if (null != parens)
			{
				return parens.Expression.RemoveParens();
			}
			else
			{
				return node;
			}
		}

		public static ExpressionSyntax ReduceNops(this Model model, ExpressionSyntax node)
		{
			if (node is ParenthesizedExpressionSyntax)
			{
				var parens = (ParenthesizedExpressionSyntax)node;

				if (parens.Expression.CanAlwaysRemoveParens())
				{
					return model.ReduceNops(((ParenthesizedExpressionSyntax)node).Expression);
				}
			}
			else if (node is InvocationExpressionSyntax)
			{
				var invoke = (InvocationExpressionSyntax)node;
				if (invoke.Expression.IsLuaDyncast())
				{
					return model.ReduceNops(invoke.ArgumentList.Arguments[0].Expression);
				}
			}

			return node;
		}

		public static ParenthesizedLambdaExpressionSyntax TryResolveLambda(this ExpressionSyntax node)
		{
			var eKind = node.Kind();
			switch (eKind)
			{
				case SyntaxKind.CastExpression:
					var cast = (CastExpressionSyntax)node;
					return cast.Expression.TryResolveLambda();

				case SyntaxKind.InvocationExpression:
					var invoke = (InvocationExpressionSyntax)node;
					if (invoke.Expression.IsLuaFunctionPseudo())
					{
						if (invoke.ArgumentList.Arguments.Count != 1)
						{
							throw new SlimCSCompilationException(node, "function pseudo only accepts 1 argument.");
						}

						return invoke.ArgumentList.Arguments[0].Expression.TryResolveLambda();
					}
					return null;

				case SyntaxKind.ParenthesizedLambdaExpression:
					var lambda = (ParenthesizedLambdaExpressionSyntax)node;
					return lambda;

				case SyntaxKind.ParenthesizedExpression:
					var parens = (ParenthesizedExpressionSyntax)node;
					return parens.Expression.TryResolveLambda();

				default:
					return null;
			}
		}

		public static bool HasPureAttribute(this ISymbol sym)
		{
			var attrs = sym.GetAttributes();
			foreach (var attr in attrs)
			{
				var clz = attr.AttributeClass;
				if (clz.Name != "PureAttribute")
				{
					continue;
				}

				if (clz.ContainingNamespace?.Name != "Contracts" ||
					clz.ContainingNamespace?.ContainingNamespace?.Name != "Diagnostics" ||
					clz.ContainingNamespace?.ContainingNamespace?.ContainingNamespace?.Name != "System")
				{
					continue;
				}

				return true;
			}

			return false;
		}

		public static bool IsTopLevelChunkAttribute(this ITypeSymbol sym)
		{
			if (null == sym.ContainingType || !sym.ContainingType.IsSlimCSSystemLib())
			{
				return false;
			}

			return (sym.Name == "TopLevelChunkAttribute");
		}

		public static bool IsTopLevelChunkClass(this ITypeSymbol sym)
		{
			// Check for the "TopLevelChunk" attribute.
			var attrs = sym.GetAttributes();
			foreach (var attr in attrs)
			{
				if (attr.AttributeClass.IsTopLevelChunkAttribute())
				{
					return true;
				}
			}

			return false;
		}

		public static bool NeedsArrayPlaceholder(this ITypeSymbol sym)
		{
			return sym.IsReferenceType || sym.TypeKind == TypeKind.TypeParameter;
		}

		public static IMethodSymbol ResolveAddMethod(this IEventSymbol sym)
		{
			var ret = sym.AddMethod;
			while (null == ret)
			{
				sym = sym.OverriddenEvent;
				if (null == sym)
				{
					break;
				}
				ret = sym.AddMethod;
			}

			return ret;
		}

		public static CastExpressionSyntax ResolveEnclosingCastExpression(this SyntaxNode node)
		{
			var parent = node.Parent;
			while (!(parent is CastExpressionSyntax))
			{
				switch (parent?.Kind())
				{
					case SyntaxKind.ParenthesizedExpression:
						parent = parent.Parent;
						break;
					default:
						return null;
				}
			}

			return (CastExpressionSyntax)parent;
		}

		public static IMethodSymbol ResolveRaiseMethod(this IEventSymbol sym)
		{
			var ret = sym.RaiseMethod;
			while (null == ret)
			{
				sym = sym.OverriddenEvent;
				if (null == sym)
				{
					break;
				}
				ret = sym.RaiseMethod;
			}

			return ret;
		}

		public static IMethodSymbol ResolveRemoveMethod(this IEventSymbol sym)
		{
			var ret = sym.RemoveMethod;
			while (null == ret)
			{
				sym = sym.OverriddenEvent;
				if (null == sym)
				{
					break;
				}
				ret = sym.RemoveMethod;
			}

			return ret;
		}

		public static IMethodSymbol ResolveGetMethod(this IPropertySymbol sym)
		{
			var ret = sym.GetMethod;
			while (null == ret)
			{
				sym = sym.OverriddenProperty;
				if (null == sym)
				{
					break;
				}
				ret = sym.GetMethod;
			}

			return ret;
		}

		public static IMethodSymbol ResolveSetMethod(this IPropertySymbol sym)
		{
			var ret = sym.SetMethod;
			while (null == ret)
			{
				sym = sym.OverriddenProperty;
				if (null == sym)
				{
					break;
				}
				ret = sym.SetMethod;
			}

			return ret;
		}

		/// <summary>
		/// Given an input value expected to be an integral value,
		/// converts it to an int32 with int32 overflow behavior.
		/// </summary>
		/// <param name="o">Input value that is an integral type.</param>
		/// <returns>The cast int32 value.</returns>
		public static int ToInt32WithInt32Overflow(this object o)
		{
			switch (Type.GetTypeCode(o.GetType()))
			{
				// Simple cases.
				case TypeCode.Byte: return (byte)o;
				case TypeCode.Int16: return (Int16)o;
				case TypeCode.Int32: return (int)o;
				case TypeCode.Int64: return (int)((Int64)o);
				case TypeCode.SByte: return (sbyte)o;
				case TypeCode.UInt16: return (UInt16)o;
				case TypeCode.UInt32: return (int)((UInt32)o);
				case TypeCode.UInt64: return (int)((UInt64)o);
				default:
					throw new System.Exception($"Input value {o} is not an integral value.");
			}
		}
	}
}
