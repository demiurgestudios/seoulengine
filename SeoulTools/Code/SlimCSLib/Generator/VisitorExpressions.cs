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
	public sealed partial class Visitor : CSharpSyntaxVisitor<SyntaxNode>, IDisposable
	{
		#region Private members
		bool AlwaysEvalutesTrueInLua(SpecialType eType)
		{
			// Certain types are always "true".
			switch (eType)
			{
				case SpecialType.System_Double:
				case SpecialType.System_Int32:
				case SpecialType.System_Int64:
				case SpecialType.System_String:
				case SpecialType.System_UInt32:
				case SpecialType.System_UInt64:
					return true;
				default:
					return false;
			}
		}

		public bool AlwaysEvalutesTrueInLua(ExpressionSyntax node)
		{
			var eKind = node.Kind();
			switch (eKind)
			{
				// TODO: Assuming math functions
				// always evaluate to true is not always a valid
				// assumption.
				case SyntaxKind.AddExpression:
				case SyntaxKind.DivideExpression:
				case SyntaxKind.ModuloExpression:
				case SyntaxKind.MultiplyExpression:
				case SyntaxKind.SubtractExpression:
					return true;

				case SyntaxKind.NumericLiteralExpression: // Numbers are always true in Lua.
				case SyntaxKind.ObjectCreationExpression: // A creation is always "true" in Lua.
				case SyntaxKind.StringLiteralExpression: // A literal string is always "true" in Lua.
				case SyntaxKind.TrueLiteralExpression:
					return true;

				case SyntaxKind.ParenthesizedExpression: // Handle inner.
					return AlwaysEvalutesTrueInLua(((ParenthesizedExpressionSyntax)node).Expression);
			}

			// Invocations, check the return type.
			if (node is InvocationExpressionSyntax invoke)
			{
				var sym = m_Model.GetSymbolInfo(invoke.Expression).Symbol as IMethodSymbol;
				if (null != sym)
				{
					if (AlwaysEvalutesTrueInLua(sym.ReturnType.SpecialType))
					{
						return true;
					}
				}
			}

			// Other nodes, we need deeper analysis.
			var type = m_Model.GetTypeInfo(node).Type;
			if (null != type)
			{
				if (AlwaysEvalutesTrueInLua(type.SpecialType))
				{
					return true;
				}
			}

			// Finally, check for constant evaluation.
			var constant = m_Model.GetConstantValue(node);
			if (constant.HasValue)
			{
				// Any constant value other than null and false is "true" in Lua.
				if (null == constant.Value ||
					(constant.Value is bool && ((bool)constant.Value) == false))
				{
					return false;
				}

				return true;
			}

			// Ran out of ideas, all the rest are not known to be true.
			return false;
		}

		static SyntaxToken Literal(object o)
		{
			if (o == null)
			{
				return SyntaxFactory.Token(SyntaxKind.NullKeyword);
			}

			var eTypeCode = Type.GetTypeCode(o.GetType());
			switch (eTypeCode)
			{
				case TypeCode.Boolean:
					return SyntaxFactory.Token(((bool)o) ? SyntaxKind.TrueKeyword : SyntaxKind.FalseKeyword);
				case TypeCode.Double: return SyntaxFactory.Literal((double)o);
				case TypeCode.Int32: return SyntaxFactory.Literal((int)o);
				case TypeCode.String: return SyntaxFactory.Literal((string)o);
				default:
					throw new SlimCSCompilationException($"'{eTypeCode}' is not a supported literal type value.");
			}
		}

		bool MayOverflowInt32(
			ExpressionSyntax left,
			TypeInfo leftTypeInfo,
			SyntaxToken op,
			ExpressionSyntax right,
			TypeInfo rightTypeInfo)
		{
			// Check for overflowing operators.
			var eKind = op.Kind();
			switch (eKind)
			{
				case SyntaxKind.AsteriskToken:
				case SyntaxKind.MinusToken:
				case SyntaxKind.PercentToken:
				case SyntaxKind.PlusToken:
				case SyntaxKind.SlashToken:
					{
						// Overflows if left and right are both of type Int32 or UInt32.
						var leftType = leftTypeInfo.Type;
						var rightType = rightTypeInfo.Type;

						// Have type info, check.
						if (null != leftType && null != rightType)
						{
							// Both are 32-bit integers, so they can overflow.
							var eLeftSpecial = leftType.SpecialType;
							var eRightSpecial = rightType.SpecialType;
							if ((eLeftSpecial == SpecialType.System_Int32 || eLeftSpecial == SpecialType.System_UInt32) &&
								(eRightSpecial == SpecialType.System_Int32 || eRightSpecial == SpecialType.System_UInt32))
							{
								// TODO: May not always be what we want, but in short,
								// if we're about to report overflow but both operands
								// are constant, don't report as overflowing. I eventually
								// want to just collapse fully constant expressions like this
								// at compile time, but we're not doing that yet.
								if (m_Model.GetConstantValue(left).HasValue &&
									m_Model.GetConstantValue(right).HasValue)
								{
									return false;
								}

								return true;
							}
						}
					}
					break;
			}

			// Will not overflow.
			return false;
		}

		IReadOnlyList<ArgumentSyntax> ResolveArguments(
			SyntaxNode context,
			ImmutableArray<IParameterSymbol> prms,
			SeparatedSyntaxList<ArgumentSyntax> args)
		{
			// We need to generate a new args array if:
			// - input arguments have named arguments.
			// - input arguments length < parameters length and parameters does not end in an IsParam argument.
			// - input arguments length < parameters length - 1 and the last param is an IsParam argument.
			var bParams = prms.HasVarargs();
			var bNeedFixup =
				(bParams && args.Count < prms.Length - 1) ||
				(!bParams && args.Count < prms.Length);
			if (!bNeedFixup)
			{
				foreach (var arg in args)
				{
					if (arg.NameColon != null)
					{
						bNeedFixup = true;
						break;
					}
				}
			}

			// Done if no named arguments.
			if (!bNeedFixup)
			{
				return args;
			}

			// Compute target length - max of input arguments
			// and prms count or prms count - 1 if bParams it rue.
			var iTarget = Utilities.Max(args.Count,
				bParams ? prms.Length - 1 : prms.Length);

			// Adjust as needed for named arguments.
			var ret = new ArgumentSyntax[iTarget];
			for (int i = 0; i < args.Count; ++i)
			{
				var arg = args[i];

				// Named arguments, find their positional location
				// and place them there.
				if (arg.NameColon != null)
				{
					var iParamIndex = prms.IndexOf(arg.NameColon);
					if (iParamIndex >= ret.Length)
					{
						Array.Resize(ref ret, (iParamIndex + 1));
					}
					ret[iParamIndex] = args[i];
				}
				// Otherwise, place at the current offset.
				else
				{
					ret[i] = args[i];
				}
			}

			// Now fill in any arguments that have not yet
			// been filled in.
			int iNextValid = -1;
			for (int i = 0; i < ret.Length; ++i)
			{
				var arg = ret[i];
				if (null != arg)
				{
					continue;
				}

				// Update next valid if not set or too old.
				if (iNextValid < i)
				{
					for (int j = i + 1; j < ret.Length; ++j)
					{
						if (null != ret[j])
						{
							iNextValid = j;
							break;
						}
					}
				}

				var prm = prms[i];
				if (!prm.IsOptional || !prm.HasExplicitDefaultValue)
				{
					throw new SlimCSCompilationException(args[0], $"argument {i} is not optional but was not specified.");
				}

				var lit = Literal(prm.ExplicitDefaultValue);
				arg = SyntaxFactory.Argument(
					SyntaxFactory.LiteralExpression(lit.ExpressionKind(context), lit).WithAdditionalAnnotations(kLineMismatchAllowed)).WithAdditionalAnnotations(kLineMismatchAllowed);

				ret[i] = arg;
			}

			// Final step, remove any trailing explicit null values, since they are
			// implicit in Lua.
			iTarget = ret.Length;
			for (int i = ret.Length - 1; i >= 0; --i)
			{
				if (ret[i].Expression is LiteralExpressionSyntax lit)
				{
					if (null == lit.Token.Value)
					{
						--iTarget;
						continue;
					}
				}

				// If we get here in all cases, we've hit
				// a non-null value, so break.
				break;
			}

			// Resize if iTarget is no longer equal to ret.Length.
			if (iTarget != ret.Length)
			{
				Array.Resize(ref ret, iTarget);
			}

			return ret;
		}

		SyntaxNode VisitAssignmentExpression(
			AssignmentExpressionSyntax node,
			ref int riImplicitAssignmentIndex,
			bool bRefArrayInitializer)
		{
			m_Model.Enforce(node);

			// In strict C# subset requires assignments to be statements.
			if (!node.IsValidAssignmentContext())
			{
				throw new UnsupportedNodeException(node, "assignments can only be statements, not expressions");
			}

			// Cache the op - may be tweaked for some cases, see events below.
			var eKind = node.OperatorToken.Kind();

			// Special case handling for operator overloads. In particular,
			// EventAdd and EventRemove must be collapsed (they will be of the
			// format a += b and must be treated as a = b).
			IMethodSymbol lhs = null;

			// TODO: Support this case - apparently the assignment
			// expression in an object creation context invokes the indexer.
			if (node.Parent.Kind() != SyntaxKind.ObjectInitializerExpression)
			{
				var sym = m_Model.GetSymbolInfo(node.Left).Symbol;
				if (null != sym)
				{
					switch (sym.Kind)
					{
						case SymbolKind.Event:
							{
								var eventSym = (IEventSymbol)sym;
								if (node.OperatorToken.Kind() == SyntaxKind.PlusEqualsToken)
								{
									lhs = eventSym.ResolveAddMethod();
								}
								else
								{
									lhs = eventSym.ResolveRemoveMethod();
								}

								// Need to pretend to be a regular assignment
								// in this case.
								eKind = SyntaxKind.EqualsToken;
							}
							break;
						case SymbolKind.Property:
							{
								var propSym = (IPropertySymbol)sym;
								if (!propSym.IsAutoProperty())
								{
									lhs = propSym.ResolveSetMethod();
								}
							}
							break;
					}
				}
			}

			// Filtering - if the containing type of lhs is not
			// normal (it is a Lua special, like Table), then
			// it won't have any special behavior.
			if (null != lhs && lhs.ContainingType.GetLuaKind() != LuaKind.NotSpecial)
			{
				lhs = null;
			}

			// Handle for (e.g.) |=, &=, etc.
			if (eKind != SyntaxKind.EqualsToken)
			{
				// All of these are left = left op right
				var left = node.Left;
				var right = node.Right;

				// Commit lhs.
				LHS = lhs;
				Visit(left);
				LHS = null;

				// If there's a LHS method, skip the assignment token.
				if (null == lhs) { Write(" ="); }

				// Now emit the RHS value to assign.
				SeparateForFirst(left);
				// Need to wrap left and right in parens to maintain precedence
				// (e.g. a += b, += is lower precedence than everything except
				// the comma operator, but the expansion, a = a + b, '+' is
				// higher precedence than many operators).
				VisitBinaryExpression(left, node.OperatorToken, right, true);

				// Finish the function call if there's one.
				if (null != lhs) { Write(')'); }

				return node;
			}

			// Handle reduction of function assignment to a function
			// declaration.
			// e.g. A.B = function(self) to function A:B()
			var eConvert = CanConvertToFunctionDeclaration(node);
			if (null == lhs &&
				eConvert != CanConvertToFuncResult.CannotConvert)
			{
				var bSend = (eConvert == CanConvertToFuncResult.ConvertToSend);
				var eEmitType = (bSend ? MemberAccessEmitType.Send : MemberAccessEmitType.Normal);

				var left = (MemberAccessExpressionSyntax)node.Left;
				var right = node.Right.TryResolveLambda();

				Write(kLuaFunction);
				Write(' ');
				VisitMemberAccessExpression(left, eEmitType);
				VisitParenthesizedLambdaExpression(right, true, bSend);
			}
			else
			{
				// Check for collapsing (we want to allow implicit
				// member access to just be implicit in Lua, instead of
				// explicit.
				//
				// Complicated looking, but in short, we're looking
				// for an implicit assignment of the form [1] = "foo"
				// where the integer value in brackets is equal
				// to the current value of riImplicitAssignmentIndex.
				if (node.Parent.Kind() == SyntaxKind.ObjectInitializerExpression)
				{
					var left = node.Left;
					if (left.Kind() == SyntaxKind.ImplicitElementAccess)
					{
						var impl = (ImplicitElementAccessSyntax)left;
						if (impl.ArgumentList != null &&
							impl.ArgumentList.Arguments.Count == 1)
						{
							var lookup = impl.ArgumentList.Arguments[0].Expression;
							if (lookup.Kind() == SyntaxKind.NumericLiteralExpression)
							{
								var literal = (LiteralExpressionSyntax)lookup;
								int iLiteralValue = (int)literal.Token.Value;
								if (iLiteralValue == riImplicitAssignmentIndex)
								{
									// The next implicit will have the next index.
									++riImplicitAssignmentIndex;
									if (bRefArrayInitializer) { Write('('); }
									Visit(node.Right);
									if (bRefArrayInitializer) { Write(" or false)"); }
									return node;
								}
							}
						}
					}
				}

				// Check for the need for array wrapping.
				var bNeedsArrayPlaceholder = bRefArrayInitializer;
				if (!bNeedsArrayPlaceholder && node.Left is ElementAccessExpressionSyntax)
				{
					// Determine array properties.
					var element = (ElementAccessExpressionSyntax)node.Left;
					(_, bNeedsArrayPlaceholder) = m_Model.GetArrayFeatures(element.Expression);
				}

				// Special case - if node.Left is all discarded (either
				// a single variable discard declaration or a tuple with
				// all discards), then we just omit the entire assignment,
				// as long as lhs is null).
				var bAllDiscarded = false;
				if (null == lhs)
				{
					bAllDiscarded = node.Left.AllDiscarded(m_Model);

					// Sanity check - if all the LHS is discarded,
					// we can completely omit the assignment if the RHS is a function
					// call.
					bAllDiscarded = bAllDiscarded && node.Right.Kind() == SyntaxKind.InvocationExpression;
				}

				// Skip if discarded.
				if (!bAllDiscarded)
				{
					// Commit lhs.
					LHS = lhs;
					Visit(node.Left);
					LHS = null;

					// If there's a LHS method, skip the assignment token.
					if (null == lhs) { Write(" ="); }
				}

				// Wrap the write if a ref element array type.
				if (bNeedsArrayPlaceholder) { Write(" ("); }

				// Emit the RHS (value to set).
				Visit(node.Right);

				// Terminate write rwapping.
				if (bNeedsArrayPlaceholder) { Write(" or false)"); }

				// Finish the function call if there's one.
				if (null != lhs) { Write(')'); }
			}

			return node;
		}

		void VisitAsInvoke(ExpressionSyntax node, MemberAccessEmitType eType)
		{
			var eKind = node.Kind();
			switch (eKind)
			{
				case SyntaxKind.AnonymousMethodExpression:
				case SyntaxKind.InvocationExpression:
					Visit(node);
					break;
				case SyntaxKind.IdentifierName:
					VisitIdentifierNameAsInvoke((IdentifierNameSyntax)node);
					break;
				case SyntaxKind.SimpleLambdaExpression:
					Visit(node);
					break;
				case SyntaxKind.SimpleMemberAccessExpression:
					VisitMemberAccessExpression((MemberAccessExpressionSyntax)node, eType);
					break;
				default:
					throw new SlimCSCompilationException(node, $"unsupported invoke type '{eKind}'.");
			}
		}

		// Utility and enum used to track a node's nullable kind
		// for comparison op (<, <=, >, >=) handling.
		enum NullableKind
		{
			NotNullable,

			/// <summary>
			/// Simple nullables do not have side effects when evaluated.
			/// </summary>
			NullableSimple,

			/// <summary>
			/// Nullable complex potentially have side effects when evaluated (e.g. a function call).
			/// </summary>
			NullableComplex,
		}
		NullableKind GetNullableKind(ExpressionSyntax node)
		{
			var constEval = m_Model.GetConstantValue(node);

			// TODO: If the value is constant and null,
			// then we can just immediately evaluate the expression
			// as "false".
			//
			// Not nullable.
			if (constEval.HasValue && null != constEval.Value)
			{
				return NullableKind.NotNullable;
			}

			// Not nullable unless the original definition is nullable.
			var origDef = m_Model.GetTypeInfo(node).Type.OriginalDefinition;
			if (origDef?.SpecialType != SpecialType.System_Nullable_T)
			{
				return NullableKind.NotNullable;
			}

			// Determine nullable type for handling.
			var sym = m_Model.GetSymbolInfo(node).Symbol;
			var eSymKind = sym?.Kind;

			// Different handling based on symbol kind.
			NullableKind eReturn = NullableKind.NotNullable;
			switch (eSymKind)
			{
				case SymbolKind.Event:
					// Always complex.
					eReturn = NullableKind.NullableComplex;
					break;
				case SymbolKind.Method:
					// If we're invoking a method but it resolves to a non-nullable return value,
					// the result for our concerns is not nullable.
					{
						var methodSym = (IMethodSymbol)sym;
						if (methodSym.ReturnType.OriginalDefinition?.SpecialType == SpecialType.System_Nullable_T)
						{
							eReturn = NullableKind.NullableComplex;
						}
					}
					break;
				case SymbolKind.Property:
					// Complex if not contained in the Lua table type.
					eReturn = sym.ContainingType.IsLuaTable() ? NullableKind.NullableSimple : NullableKind.NullableComplex;
					break;
				default:
					// All other types become simply nullable.
					eReturn = NullableKind.NullableSimple;
					break;
			}

			return eReturn;
		}

		void VisitBinaryExpression(
			ExpressionSyntax left,
			SyntaxToken op,
			ExpressionSyntax right,
			bool bAddParens = false)
		{
			// Cache, used in several branches.
			var leftTypeInfo = m_Model.GetTypeInfo(left);
			var rightTypeInfo = m_Model.GetTypeInfo(right);

			// Delegate handling - don't support '+' or '-' operators yet.
			{
				var eOpKind = op.Kind();
				if (eOpKind == SyntaxKind.PlusEqualsToken ||
					eOpKind == SyntaxKind.PlusPlusToken ||
					eOpKind == SyntaxKind.PlusToken ||
					eOpKind == SyntaxKind.MinusEqualsToken ||
					eOpKind == SyntaxKind.MinusMinusToken ||
					eOpKind == SyntaxKind.MinusToken)
				{
					if (leftTypeInfo.ConvertedType?.TypeKind == TypeKind.Delegate)
					{
						throw new UnsupportedNodeException(left, $"operator {op} is not supported on delegates.");
					}
					else if (rightTypeInfo.ConvertedType?.TypeKind == TypeKind.Delegate)
					{
						throw new UnsupportedNodeException(right, $"operator {op} is not supported on delegates.");
					}
				}
			}

			// Prefiltering of some types - assign support (e.g. *=).
			switch (op.Kind())
			{
				case SyntaxKind.AsteriskEqualsToken:
					op = SyntaxFactory.Token(SyntaxKind.AsteriskToken);
					break;
				case SyntaxKind.MinusEqualsToken:
					op = SyntaxFactory.Token(SyntaxKind.MinusToken);
					break;
				case SyntaxKind.PercentEqualsToken:
					op = SyntaxFactory.Token(SyntaxKind.PercentToken);
					break;
				case SyntaxKind.PlusEqualsToken:
					op = SyntaxFactory.Token(SyntaxKind.PlusToken);
					break;
				case SyntaxKind.SlashEqualsToken:
					op = SyntaxFactory.Token(SyntaxKind.SlashToken);
					break;
			}

			// Check for the possibility of 32-bit integer
			// ops. To implement these, we implement the following
			// global extensions and hooks:
			// __i32narrow__ = bit.tobit - narrows a number to 32-bit
			// integer - does *not* correctly handle truncation (a
			// narrow is only valid for numbers that are already integers)
			// __i32truncate__ = math.i32truncate - i32truncate is a custom
			// extension, implemented in a modified lua vm.
			// Equivalent to an (int) cast - both truncates and
			// narrows the input value.
			// __i32mod__ = math.i32mod - i32mod is a custom extension,
			// implemented as a standard c function extension to math.
			// Implements modulo of a % b, where a and b are narrowed
			// to 32-bit integers.
			// __i32mul__ = math.i32mul - i32mul is a custom extension,
			// implemented in a modified lua vm.
			// Equivalent to a * b, where a and b
			// are first cast to 32-bit integers.
			//
			// With these extensions, integer operations are implemented
			// as follows:
			// - a + b -> __i32narrow__(a + b)
			// - a / b -> __i32truncate__(a / b)
			// - a % b -> __i32mod__(a, b)
			// - a * b -> __i32mul__(a, b)
			// - a - b -> __i32narrow__(a - b)
			var bIntegerOp = MayOverflowInt32(left, leftTypeInfo, op, right, rightTypeInfo);

			// Special handling for certain types.
			var eKind = op.Kind();
			switch (eKind)
			{
				// These three operators require special handling if
				// the operands are boolean.
				case SyntaxKind.AmpersandEqualsToken: // assign support.
				case SyntaxKind.AmpersandToken:
					{
						var typeInfo = m_Model.GetTypeInfo(left).ConvertedType;
						var eSpecialType = typeInfo.SpecialType;

						Write(eSpecialType == SpecialType.System_Boolean ? kLuaBband : kLuaBitAnd);
						goto as_function;
					}

				case SyntaxKind.BarEqualsToken: // assign support.
				case SyntaxKind.BarToken:
					{
						var typeInfo = m_Model.GetTypeInfo(left).ConvertedType;
						var eSpecialType = typeInfo.SpecialType;

						Write(eSpecialType == SpecialType.System_Boolean ? kLuaBbor : kLuaBitOr);
						goto as_function;
					}

				case SyntaxKind.CaretEqualsToken: // assign support.
				case SyntaxKind.CaretToken:
					{
						var typeInfo = m_Model.GetTypeInfo(left).ConvertedType;
						var eSpecialType = typeInfo.SpecialType;

						Write(eSpecialType == SpecialType.System_Boolean ? kLuaBbxor : kLuaBitXor);
						goto as_function;
					}

				case SyntaxKind.LessThanLessThanEqualsToken: // assign support.
				case SyntaxKind.LessThanLessThanToken:
					Write(kLuaBitLshift);
					goto as_function;

				case SyntaxKind.GreaterThanGreaterThanEqualsToken: // assign support.
				case SyntaxKind.GreaterThanGreaterThanToken:
					// Whether we use a right shift or an arithmetic right shift
					// depends on the type of the left argument.
					{
						var typeInfo = m_Model.GetTypeInfo(left).ConvertedType;
						var eSpecialType = typeInfo.SpecialType;
						switch (eSpecialType)
						{
							case SpecialType.System_Int32:
								Write(kLuaBitArshift);
								break;
							case SpecialType.System_UInt32:
								Write(kLuaBitRshift);
								break;
							default:
								throw new UnsupportedNodeException(left, $"unsupported type '{eSpecialType}' for right shift.");
						}

						goto as_function;
					}

					// For both 'as' and 'is', the right
					// argument needs to be checked. If it is
					// an interface, we emit it as a string literal
					// insde, since interfaces do not exist in the Lua
					// backend as global tables like classes do.
				case SyntaxKind.AsKeyword:
					Write(kLuaAs);
					goto as_is_operator;
				case SyntaxKind.IsKeyword:
					Write(kLuaIs);
					goto as_is_operator;

				as_is_operator:
					{
						var rightType = m_Model.GetTypeInfo(right).ConvertedType;
						if (null != rightType && rightType.TypeKind == TypeKind.Interface)
						{
							Write('(');
							Visit(left);
							Write(", ");
							WriteStringLiteral(m_Model.LookupOutputId(rightType));
							Write(')');
							break;
						}
						else if (null != rightType && rightType.TypeKind == TypeKind.Delegate)
						{
							Write('(');
							Visit(left);
							Write(", ");
							Write(kLuaDelegate);
							Write(')');
							break;
						}
						else
						{
							goto as_function;
						}
					}

				case SyntaxKind.PercentToken:
					if (bIntegerOp)
					{
						// Call into a hook that we've
						// added to the LuaJIT math library.
						Write(kLuaI32Mod);
					}
					else
					{
						Write(kLuaMathFmod);
					}
					goto as_function;

				case SyntaxKind.AsteriskToken:
					// Call into a hook that we've
					// added to the LuaJIT math library.
					if (bIntegerOp)
					{
						Write(kLuaI32Mul);
						goto as_function;
					}
					else
					{
						goto default_handling;
					}

				as_function:
					Write('(');
					Visit(left);
					Write(',');
					Visit(right);
					Write(')');
					break;

				case SyntaxKind.AmpersandAmpersandToken:
				case SyntaxKind.BarBarToken:
				case SyntaxKind.EqualsEqualsToken:
				case SyntaxKind.ExclamationEqualsToken:
				case SyntaxKind.GreaterThanEqualsToken:
				case SyntaxKind.GreaterThanToken:
				case SyntaxKind.LessThanToken:
				case SyntaxKind.LessThanEqualsToken:
				case SyntaxKind.MinusToken:
				case SyntaxKind.PlusToken:
				case SyntaxKind.QuestionQuestionToken:
				case SyntaxKind.SlashToken:
					default_handling:
					{
						// Special handling for identifying a need for the Lua '..' operator.
						if (eKind == SyntaxKind.PlusToken)
						{
							if (leftTypeInfo.ConvertedType?.SpecialType == SpecialType.System_String ||
								rightTypeInfo.ConvertedType?.SpecialType == SpecialType.System_String)
							{
								// Must wrap in tostring - C# will coerce, Lua will only
								// coerce if the value is a number.
								var bLeftToString = NeedsLuaToString(left);
								var bRightToString = NeedsLuaToString(right);

								if (bLeftToString) { Write(kLuaTostring); Write('('); }
								Visit(left);
								if (bLeftToString) { Write(')'); }

								Write(' ');
								Write(kLuaConcat);

								if (bRightToString) { Write(kLuaTostring); Write('('); }
								Visit(right);
								if (bRightToString) { Write(')'); }

								return;
							}
						}

						// Special handling for ?? - if the entire expression is
						// of a type that is nullable, we need to convert the expression
						// into a ternary of the format (a == null ? b : a), since
						// a ?? b where a == false is expected to evaluate to false, not b,
						// but the 'or' operator in Lua will return 'b'.
						if (eKind == SyntaxKind.QuestionQuestionToken)
						{
							var expressionType = m_Model.GetTypeInfo(left.Parent).Type;
							bool bIsPotentiallyNullBoolean = (
								SpecialType.System_Boolean == expressionType.SpecialType ||
								SpecialType.System_Object == expressionType.SpecialType ||
								expressionType.IsLuaDval());

							if (bIsPotentiallyNullBoolean)
							{
								InternalVisitConditionalExpression(left, right, left, true);
								return;
							}
						}

						// Nullable int/double handling - two possibilities.
						// - if an arg is nullable but simple (just a variable
						//   lookup, no side effects), then we emit
						//   a conditional check prior to the <, <=, >, or >=
						//   comparisons (e.g. a and b and a <= b)
						// - if the element has side effects (e.g. a = f()),
						//   then we instead or the result with NaN (0/0),
						//   so the comparison will always evaluate to false.
						//   (e.g. (a or (0/0)) <= (b or (0/0))
						//
						// This is done becuase the first conversion is cheaper
						// at runtime.
						NullableKind eLeftNullableKind = NullableKind.NotNullable;
						NullableKind eRightNullableKind = NullableKind.NotNullable;
						switch (eKind)
						{
							// Only applicable to comparisons.
							case SyntaxKind.GreaterThanEqualsToken:
							case SyntaxKind.GreaterThanToken:
							case SyntaxKind.LessThanEqualsToken:
							case SyntaxKind.LessThanToken:
								eLeftNullableKind = GetNullableKind(left);
								eRightNullableKind = GetNullableKind(right);
								break;
						}

						// Overflow and truncation handling.
						var bOverflowIntegerOp = false;
						if (bIntegerOp)
						{
							switch (eKind)
							{
								case SyntaxKind.SlashToken:
									// Divide is always faster as the floating point divide followed
									// by a truncate.
									bOverflowIntegerOp = true;
									Write(kLuaI32Truncate);
									break;

								// Wrap the operation in a call to __i32narrow__
								// which truncates to 32-bits.
								case SyntaxKind.MinusToken:
								case SyntaxKind.PlusToken:
									bOverflowIntegerOp = true;
									Write(kLuaI32Narrow);
									break;

								default:
									throw new SlimCSCompilationException(
										left,
										$"Invalid integer op {op} in fallback branch, this is a compiler bug.");
							}
						}

						// If nullable numbers, we need to emit
						// a non-null check prior to the comparison.
						if (eLeftNullableKind == NullableKind.NullableSimple || eRightNullableKind == NullableKind.NullableSimple)
						{
							if (NeedsWhitespaceSeparation) { Write(' '); }
							Write('(');
							if (eLeftNullableKind == NullableKind.NullableSimple)
							{
								Visit(left);
								Write(" ~= nil and");
							}
							if (eRightNullableKind == NullableKind.NullableSimple)
							{
								Visit(right);
								Write(" ~= nil and");
							}
						}

						// Wrap the overflow integer operation.
						if (bOverflowIntegerOp) { Write('('); }

						// Wrap the nullable number. This is needed if
						// the left operand is "complex" (a function call or
						// other element that might have unintended side effects
						// if we visited it twice).
						if (eLeftNullableKind == NullableKind.NullableComplex)
						{
							if (NeedsWhitespaceSeparation) { Write(' '); }
							Write("((");
						}

						// Left operand.
						if (bAddParens) { Write('('); }
						Visit(left);
						if (bAddParens) { Write(')'); }

						// For >=, >, <, <= comparisons, if the left value is a nullable
						// number, we need to emit (left or (0/0)) instead of just the
						// value itself, relying on NaN evaluating to false always (which
						// is exactly equivalent to the treatment of null in this nullable
						// case).
						if (eLeftNullableKind == NullableKind.NullableComplex) { Write(") or (0/0))"); }

						// Interior comments.
						WriteInteriorComment(
							left.GetStartColumn(),
							left.GetStartLine(),
							right.GetStartColumn());

						// Operator.
						Write(' ');
						Write(op);

						// We pre-emptively add separation here if name is the minus character,
						// since that can both be a unary and a binary operator and has difference
						// white spacing behavior in each case.
						if (eKind == SyntaxKind.MinusToken) { SeparateForFirst(right, true); }

						// Wrap the nullable number. This is needed if
						// the left operand is "complex" (a function call or
						// other element that might have unintended side effects
						// if we visited it twice).
						if (eRightNullableKind == NullableKind.NullableComplex)
						{
							if (NeedsWhitespaceSeparation) { Write(' '); }
							Write("((");
						}

						// Right operand.
						if (bAddParens) { Write('('); }
						Visit(right);
						if (bAddParens) { Write(')'); }

						// For >=, >, <, <= comparisons, if the right value is a nullable
						// number, we need to emit (right or (0/0)) instead of just the
						// value itself, relying on NaN evaluating to false always (which
						// is exactly equivalent to the treatment of null in this nullable
						// case).
						if (eRightNullableKind == NullableKind.NullableComplex) { Write(") or (0/0))"); }

						// Finish any overflow or truncation handling.
						if (bOverflowIntegerOp) { Write(')'); }

						// Terminate simple handling.
						if (eLeftNullableKind == NullableKind.NullableSimple || eRightNullableKind == NullableKind.NullableSimple)
						{
							Write(')');
						}
					}
					break;

				default:
					throw new SlimCSCompilationException(left.Parent, $"unsupported binary op '{eKind}'");
			}
		}

		HashSet<SyntaxNode> m_DelegateBindSet = new HashSet<SyntaxNode>();

		(INamedTypeSymbol, bool) NeedsDelegateBind(SyntaxNode node)
		{
			// Don't re-enter.
			if (m_DelegateBindSet.Contains(node)) { return (null, false); }

			// Identifier not invalid in member access.
			if (node is NameSyntax && node.Parent is MemberAccessExpressionSyntax) { return (null, false); }

			// Gather type information to determine need.
			var si = m_Model.GetSymbolInfo(node);
			if (si.Symbol?.Kind == SymbolKind.Method)
			{
				var ti = m_Model.GetTypeInfo(node);
				if (ti.ConvertedType?.TypeKind == TypeKind.Delegate)
				{
					return ((INamedTypeSymbol)ti.ConvertedType, true);
				}
				else
				{
					var last = node;
					var parent = node.Parent;
					while (parent is ParenthesizedExpressionSyntax paren)
					{
						last = parent;
						parent = paren.Parent;
					}

					if (parent is CastExpressionSyntax cast && cast.Expression == last)
					{
						ti = m_Model.GetTypeInfo(cast);
						if (ti.Type?.TypeKind == TypeKind.Delegate)
						{
							return ((INamedTypeSymbol)ti.Type, true);
						}
						else if (ti.ConvertedType?.TypeKind == TypeKind.Delegate)
						{
							return ((INamedTypeSymbol)ti.ConvertedType, true);
						}
					}
				}
			}

			// Not a delegate binding type.
			return (null, false);
		}

		void BindDelegate(INamedTypeSymbol namedSym, ExpressionSyntax methodExpr)
		{
			// Re-entrancy handling.
			if (m_DelegateBindSet.Contains(methodExpr)) { return; }
			m_DelegateBindSet.Add(methodExpr);

			try
			{
				// Resolve.
				while (true)
				{
					if (methodExpr is ParenthesizedExpressionSyntax paren)
					{
						methodExpr = paren.Expression;
						continue;
					}

					if (methodExpr is CastExpressionSyntax cast)
					{
						methodExpr = cast.Expression;
						continue;
					}

					break;
				}

				var targetSym = m_Model.GetSymbolInfo(methodExpr).Symbol;
				var targetMethod = targetSym as IMethodSymbol;
				if (null == targetMethod ||
					targetMethod.MethodKind == MethodKind.AnonymousFunction)
				{
					// Elide the delegate if we don't need the closure.
					Visit(methodExpr);
					return;
				}

				// Resolve the invocation method of the delegate.
				var method = namedSym.DelegateInvokeMethod;

				// If the invocable method has optional params, we need
				// to generate a closure function.
				var bNeedClosure = false;
				foreach (var p in method.Parameters)
				{
					if (p.IsOptional)
					{
						bNeedClosure = true;
						break;
					}
				}

				// Instance methods that don't need a complex closure
				// use the __bind_delegate__ utility, which caches
				// the bound delegate so that equality works as expected.
				if (!bNeedClosure && !targetMethod.IsStatic)
				{
					// Get member access type.
					var eType = GetMemberAccessEmitType(methodExpr, true);

					// Now emit the call.
					Write(kLuaBindDelegate);
					Write('(');
					if (methodExpr is MemberAccessExpressionSyntax mem)
					{
						Visit(mem.Expression);
					}
					else
					{
						Write(kLuaSelf);
					}
					Write(", ");
					Visit(methodExpr);
					Write(')');
				}
				else if (bNeedClosure)
				{
					// Generate a closure with necessary behavior
					// (including runtime resolution of optional parameters).

					// Function and parameters - need to dedup these names
					// in case they collide with the surrounding context.
					var currentSite = m_BlockScope.Back().m_Site;
					using (var scope = new Scope(this, ScopeKind.Lambda, currentSite))
					{
						Write(kLuaFunction);
						Write('(');
						var deduped = new string[targetMethod.Parameters.Length];
						for (int i = 0; i < targetMethod.Parameters.Length; ++i)
						{
							var prm = targetMethod.Parameters[i];
							if (0 != i)
							{
								Write(", ");
							}
							deduped[i] = m_BlockScope.GenDedupedId(prm.Name, true);
							Write(deduped[i]);
						}
						Write(')');

						// Emit default argument handling.
						for (int i = 0; i < method.Parameters.Length; ++i)
						{
							// Emit a default filtering statement unless
							// the default is null.
							var prm = method.Parameters[i];
							if (prm.IsOptional && prm.ExplicitDefaultValue != null)
							{
								if (NeedsWhitespaceSeparation) { Write(' '); }
								Write(kLuaIf);
								Write(' ');
								Write(deduped[i]);
								Write(" == ");
								Write(kLuaNil);
								Write(' ');
								Write(kLuaThen);
								Write(' ');
								Write(deduped[i]);
								Write(" = ");
								WriteConstant(prm.ExplicitDefaultValue);
								Write(' ');
								Write(kLuaEnd);
							}
						}

						// Return a value if there is one.
						if (NeedsWhitespaceSeparation) { Write(' '); }
						Write(kLuaReturn);
						Write(' ');

						// Now write the invocation.
						var eType = GetMemberAccessEmitType(methodExpr, true);
						VisitAsInvoke(methodExpr, eType);

						// Track whether we need an extra self parameter.
						var bNeedExplicitSelf = CanEmitAsLocal(targetMethod) && !targetMethod.IsStatic;

						// Emit args with optional param handling.
						Write('(');

						// Emit the extra self parameter if needed.
						if (bNeedExplicitSelf)
						{
							Write(kLuaSelf);
						}

						for (int i = 0; i < targetMethod.Parameters.Length; ++i)
						{
							var prm = targetMethod.Parameters[i];
							if ('(' != m_LastChar) { Write(", "); }
							Write(deduped[i]);
						}
						Write(") ");
						Write(kLuaEnd);
					}
				}
				else
				{
					// Elide the delegate if we don't need the closure.
					Visit(methodExpr);
				}
			}
			finally
			{
				m_DelegateBindSet.Remove(methodExpr);
			}
		}

		void VisitInitializerExpression(InitializerExpressionSyntax node, bool bRefArrayInitializer)
		{
			var exprs = node.Expressions;
			if (0 == exprs.Count)
			{
				Write("{}");
			}
			else
			{
				Write('{');
				Indent();
				int iImplicitAssignmentIndex = 1;
				for (int i = 0; i < exprs.Count; ++i)
				{
					var expr = exprs[i];
					SeparateForFirst(expr);
					var assign = expr as AssignmentExpressionSyntax;
					if (null != assign)
					{
						VisitAssignmentExpression(
							assign,
							ref iImplicitAssignmentIndex,
							bRefArrayInitializer);
					}
					else
					{
						if (bRefArrayInitializer) { Write('('); }
						Visit(expr);
						if (bRefArrayInitializer) { Write(" or false)"); }
					}
					if (i + 1 < exprs.Count)
					{
						Write(',');
					}
				}
				SeparateForLast(node);
				Outdent();
				Write('}');
			}
		}
		#endregion

		public override SyntaxNode VisitArrayCreationExpression(ArrayCreationExpressionSyntax node)
		{
			m_Model.Enforce(node);

			// TODO: Add support for arrays without an initializer.
			if (null == node.Initializer)
			{
				Write(kLuaArrayCreate);
				Write('(');

				// Enforce ensures only single dimensional arrays, so we can just emit the first rank
				// specifier.
				Visit(node.Type.RankSpecifiers.First().Sizes.First());
				Write(", ");

				// Default value.
				WriteDefaultValue(node.Type.ElementType);
				Write(')');
			}
			else
			{
				var type = m_Model.GetTypeInfo(node).Type as IArrayTypeSymbol;
				VisitInitializerExpression(node.Initializer, type.ElementType.NeedsArrayPlaceholder());
			}
			return node;
		}

		public override SyntaxNode VisitAssignmentExpression(AssignmentExpressionSyntax node)
		{
			int iUnused = 1;
			return VisitAssignmentExpression(node, ref iUnused, false);
		}

		public override SyntaxNode VisitBaseExpression(BaseExpressionSyntax node)
		{
			Write(node.Token);
			return node;
		}

		public override SyntaxNode VisitBinaryExpression(BinaryExpressionSyntax node)
		{
			VisitBinaryExpression(node.Left, node.OperatorToken, node.Right);
			return node;
		}

		public override SyntaxNode VisitCastExpression(CastExpressionSyntax node)
		{
			m_Model.Enforce(node);

			var toInfo = m_Model.GetTypeInfo(node.Type);
			var to = toInfo.Type;
			var fromInfo = m_Model.GetTypeInfo(node.Expression);
			var from = fromInfo.Type;

			// Treat enum as int32.
			SpecialType eTo;
			if (null == to) { eTo = SpecialType.None; }
			else if (to.TypeKind == TypeKind.Enum) { eTo = SpecialType.System_Int32; }
			else { eTo = to.SpecialType; }

			SpecialType eFrom;
			if (null == from) { eFrom = SpecialType.None; }
			else if (from.TypeKind == TypeKind.Enum) { eFrom = SpecialType.System_Int32; }
			else { eFrom = from.SpecialType; }

			// Removing cast for the right shift case.
			if (eTo == SpecialType.System_UInt32 && eFrom != SpecialType.System_UInt32)
			{
				// Special handling - if this cast to an int32 is
				// in a bit operation context, we don't need the cast - it
				// is implicit with the bit operation.
				switch (node.Parent.Kind())
				{
					case SyntaxKind.BitwiseAndExpression:
					case SyntaxKind.BitwiseOrExpression:
					case SyntaxKind.LeftShiftExpression:
					case SyntaxKind.RightShiftExpression:
						Visit(node.Expression.RemoveParens());
						return node;
				}
			}

			// Special case for cast to integer types.
			if (eTo.IsIntegralType() && eTo != eFrom)
			{
				// Special handling - if this cast to an int32 is
				// in a bit operation context, we don't need the cast - it
				// is implicit with the bit operation.
				if (eTo == SpecialType.System_Int32)
				{
					switch (node.Parent.Kind())
					{
						case SyntaxKind.BitwiseAndExpression:
						case SyntaxKind.BitwiseOrExpression:
						case SyntaxKind.LeftShiftExpression:
						case SyntaxKind.RightShiftExpression:
							Visit(node.Expression.RemoveParens());
							return node;
					}
				}

				// Integral to integral is supported unless narrowing
				// is involved.
				if (eFrom.IsIntegralType() &&
					eTo.IntegralSizeInBytes() >= eFrom.IntegralSizeInBytes())
				{
					Visit(node.Expression.RemoveParens());
					return node;
				}

				// Only valid if we get here and target is an int32.
				if (eTo != SpecialType.System_Int32)
				{
					throw new SlimCSCompilationException(node, $"general cast to {eTo} is not supported.");
				}

				// Default handling, invoke the int cast function.
				Write(kLuaCastInt);
				Write('(');
				Visit(node.Expression);
				Write(')');
			}
			// Handling for delegate casts.
			else if (to?.TypeKind == TypeKind.Delegate && to != from)
			{
				BindDelegate((INamedTypeSymbol)to, node.Expression);
			}
			// TODO: Ignoring cast to Table here is temporary
			// and not correct in general.
			// TODO: Ignore cast to Tuple type is temporary
			// and not correct in general.

			// Cast from to the same type are nop.
			// Casts to object are nop.
			// Cast from null are always a nop also.
			else if (
				to == from ||
				eTo == eFrom ||
				to.IsLuaTable() ||
				to.IsLuaTuple() ||
				eTo == SpecialType.System_Object ||
				null == from ||
				(eTo == SpecialType.System_Double && eFrom == SpecialType.System_Int32) ||
				(eTo == SpecialType.System_Double && eFrom == SpecialType.System_UInt32))
			{
				if (node.Expression.CanAlwaysRemoveParens())
				{
					Visit(node.Expression.RemoveParens());
				}
				else
				{
					Visit(node.Expression);
				}
			}
			// Other cases, used the cast global utility.
			else
			{
				Write(kLuaCast);
				Write('(');
				Visit(node.Expression);
				Write(',');

				// If the cast target is an interface, we need
				// to emit it as a string literal,
				// since interfaces do not exist as real objects
				// at runtime like classes do.
				if (to?.TypeKind == TypeKind.Interface)
				{
					WriteStringLiteral(m_Model.LookupOutputId(to));
				}
				else
				{
					Visit(node.Type);
				}
				Write(')');
			}

			return node;
		}

		public override SyntaxNode VisitCheckedExpression(CheckedExpressionSyntax node)
		{
			// Disallow checked, we only support 'unchecked'.
			if (node.Keyword.Kind() == SyntaxKind.CheckedKeyword)
			{
				throw new SlimCSCompilationException(node, "'checked' is not supported.");
			}

			// Otherwise, just vist the expression.
			Visit(node.Expression);
			return node;
		}

		public override SyntaxNode VisitConditionalAccessExpression(ConditionalAccessExpressionSyntax node)
		{
			var eOpKind = node.OperatorToken.Kind();
			switch (eOpKind)
			{
				// This is the ?. and ?-> operators.
				case SyntaxKind.QuestionToken:
					{
						// Track whether we need parens or not.
						bool bNeedParen = false;

						// If our parent is a statement context, we
						// must use an if statement for this. Otherwise,
						// we must use 'and'.
						var bStatement = node.Parent is ExpressionStatementSyntax;
						if (bStatement)
						{
							Write(kLuaIf);
							Visit(node.Expression);
							Write(' ');
							Write(kLuaThen);
						}
						else
						{
							// TODO: Find all cases where we can safely omit the parentheses.
							switch (node.Parent.Kind())
							{
								case SyntaxKind.Argument:
								case SyntaxKind.CoalesceExpression:
								case SyntaxKind.EqualsValueClause:
								case SyntaxKind.LogicalOrExpression:
								case SyntaxKind.ReturnStatement:
									break;

								case SyntaxKind.IfStatement:
									if (((IfStatementSyntax)node.Parent).Condition == node)
									{
										break;
									}
									else
									{
										goto default;
									}

								default:
									bNeedParen = true;
									break;
							}

							if (bNeedParen) { Write('('); }
							Visit(node.Expression);
							Write(' ');
							Write(kLuaAnd);
						}

						// Common handling.
						m_BindingTargets.Add(node.Expression);
						Visit(node.WhenNotNull);
						m_BindingTargets.PopBack();

						// Terminate the if, when we've emitted a statement.
						if (bStatement)
						{
							if (NeedsWhitespaceSeparation) { Write(' '); }
							Write(kLuaEnd);
						}
						else if (bNeedParen)
						{
							Write(')');
						}
					}
					break;

				default:
					throw new SlimCSCompilationException(node, $"unsupported conditional access expression op '{eOpKind}'.");
			}

			return node;
		}

		/// <summary>
		/// Visits a conditional expression. If <paramref name="bCompareConditionToNull"/> is true, the conditional expression
		/// checks if <paramref name="condition"/> == null instead.
		/// </summary>
		private void InternalVisitConditionalExpression(ExpressionSyntax condition, ExpressionSyntax whenTrue, ExpressionSyntax whenFalse, bool bCompareConditionToNull)
		{
			// Need to determine if we can represent the conditional as the Lua form:
			// a and b or c
			//
			// It works as long as b is known to never evaluate to any of the Lua
			// false values (false or nil).
			//
			// Otherwise, we need to use a different form. We wrap a and b in tables,
			// than access element 1 of the table after the expression finishes its
			// evaluation.
			if (AlwaysEvalutesTrueInLua(whenTrue))
			{
				Write('(');
				Visit(condition);
				if (bCompareConditionToNull)
				{
					Write(' ');
					Write(kLuaEqual);
					Write(' ');
					Write(kLuaNil);
				}
				Write(')');
				Write(' ');
				Write(kLuaAnd);
				Write('(');
				Visit(whenTrue);
				Write(')');
				Write(' ');
				Write(kLuaOr);
				Write('(');
				Visit(whenFalse);
				Write(')');
			}
			else
			{
				// TODO: I think this will be faster than wrapping in an anonymous
				// function. The fastest version of this would be to expand the expression stack
				// up to the statement, and replace this with an if (cond) { tmp = whentrue; } else { tmp = whenfalse; }
				//
				// e.g.
				//
				// var a = (a + (b ? c : d));
				//
				// becomes:
				// var tmp;
				// if (b) { tmp = c; } else { tmp = d; }
				// var a = (a + tmp);
				//
				// Which is roughly equivalent to evaluating the statement's
				// expression tree and emitting bytecode.

				// We wrap the entire expression in an element [1] lookup after completion.
				Write('(');
				Visit(condition);
				if (bCompareConditionToNull)
				{
					Write(' ');
					Write(kLuaEqual);
					Write(' ');
					Write(kLuaNil);
				}
				Write(' ');
				Write(kLuaAnd);
				Write(" {"); // Wrap in a table constructor.
				Visit(whenTrue);
				Write(" } "); // Wrap in a table constructor.
				Write(kLuaOr);
				Write(" {"); // Wrap in a table constructor.
				Visit(whenFalse);
				Write(" }"); // Wrap in a table constructor.
				Write(")[1]"); // Complete element 1 lookup.
			}
		}

		public override SyntaxNode VisitConditionalExpression(ConditionalExpressionSyntax node)
		{
			InternalVisitConditionalExpression(node.Condition, node.WhenTrue, node.WhenFalse, false);

			return node;
		}

		public override SyntaxNode VisitDefaultExpression(DefaultExpressionSyntax node)
		{
			WriteDefaultValue(node.Type);
			return node;
		}

		public override SyntaxNode VisitImplicitArrayCreationExpression(ImplicitArrayCreationExpressionSyntax node)
		{
			m_Model.Enforce(node);
			var type = m_Model.GetTypeInfo(node).Type as IArrayTypeSymbol;
			VisitInitializerExpression(node.Initializer, type.ElementType.NeedsArrayPlaceholder());
			return node;
		}

		public override SyntaxNode VisitInitializerExpression(InitializerExpressionSyntax node)
		{
			VisitInitializerExpression(node, false);
			return node;
		}

		public override SyntaxNode VisitInvocationExpression(InvocationExpressionSyntax node)
		{
			// Get method info.
			var symInfo = m_Model.GetSymbolInfo(node);

			// Early out.
			if (null == symInfo.Symbol &&
				symInfo.CandidateReason != CandidateReason.None)
			{
				throw new SlimCSCompilationException(node, symInfo.CandidateReason.ToString());
			}

			var sym = symInfo.Symbol;
			var method = (sym as IMethodSymbol);

			// If we failed to get the method directly,
			// resolve the method info in additional cases.
			if (null == method && null != sym)
			{
				INamedTypeSymbol type = null;
				switch (sym.Kind)
				{
					case SymbolKind.Event: type = ((IEventSymbol)sym).Type as INamedTypeSymbol; break;
					case SymbolKind.Field: type = ((IFieldSymbol)sym).Type as INamedTypeSymbol; break;
					case SymbolKind.Local: type = ((ILocalSymbol)sym).Type as INamedTypeSymbol; break;
					case SymbolKind.NamedType: type = ((INamedTypeSymbol)sym); break;
					case SymbolKind.Parameter: type = ((IParameterSymbol)sym).Type as INamedTypeSymbol; break;
					case SymbolKind.Property: type = ((IPropertySymbol)sym).Type as INamedTypeSymbol; break;
					default:
						throw new SlimCSCompilationException(node, $"unsupported symbol kind '{sym.Kind}' for invocation.");
				}

				if (null != type)
				{
					method = type.DelegateInvokeMethod;
				}
			}

			// Resolve arguments.
			var args = ResolveArguments(node, (null != method ? method.Parameters : ImmutableArray<IParameterSymbol>.Empty), node.ArgumentList.Arguments);

			// Enforce SlimCS constraints.
			m_Model.Enforce(
				node,
				method,
				args,
				args.Count > 0 &&
				args[args.Count - 1].Expression is SimpleNameSyntax &&
				IsVararg(((SimpleNameSyntax)args[args.Count - 1].Expression)));

			// Check for special lua type.
			var eLuaKind = (null != sym ? sym.GetLuaKind() : LuaKind.NotSpecial);

			// Accumulate an additional progress calls.
			if (eLuaKind == LuaKind.OnInitProgress)
			{
				m_Model.SharedModel.OnInitProgress();
			}

			// Check how we must emit the invocation.
			var eEmitType = GetMemberAccessEmitType(node.Expression);

			if (node.Expression.IsLuaAsTable())
			{
				if (args.Count != 1)
				{
					throw new SlimCSCompilationException(node, "AsTable supports only 1 argument.");
				}

				var arg = args[0].Expression as TypeOfExpressionSyntax;
				if (null == arg)
				{
					throw new SlimCSCompilationException(node, "argument to astable must be a typeof expression.");
				}

				Visit(arg.Type);
			}
			else if (eLuaKind == LuaKind.Ipairs || eLuaKind == LuaKind.Pairs)
			{
				if (args.Count != 1)
				{
					throw new SlimCSCompilationException(node, "ipairs and pairs support only 1 argument.");
				}

				// Just write out the base name, no generic specialization.
				SeparateForFirst(node.Expression);
				Write(eLuaKind == LuaKind.Ipairs ? kLuaIpairs : kLuaPairs);
				Write('(');
				Visit(args[0].Expression);
				Write(')');
			}
			else if (eLuaKind == LuaKind.Dyncast)
			{
				if (args.Count != 1)
				{
					throw new SlimCSCompilationException(node, "dyncasts support only 1 argument.");
				}

				Visit(args[0].Expression);
			}
			else if (node.Expression.IsLuaConcat())
			{
				if (args.Count < 2)
				{
					throw new SlimCSCompilationException(node, "concat function supports >= 2 arguments");
				}

				VisitInterpolation(null, null, args[0].Expression);

				for (int i = 1; i < args.Count; ++i)
				{
					Write(' ');
					Write(kLuaConcat);

					VisitInterpolation(null, null, args[i].Expression);
				}
			}
			else if (node.Expression.IsLuaLength())
			{
				Write('#');
				Visit(args[0].Expression);
			}
			else if (node.Expression.IsLuaNot())
			{
				Write(kLuaNot);
				var bParens = !args[0].Expression.CanAlwaysRemoveParens();
				if (bParens) { Write('('); }
				else { Write(' '); }
				Visit(args[0].Expression);
				if (bParens) { Write(')'); }
			}
			else if (node.Expression.IsLuaFunctionPseudo())
			{
				if (args.Count != 1)
				{
					throw new SlimCSCompilationException(node, "function pseudo supports only 1 argument");
				}
				Visit(args[0].Expression);
			}
			// Although we can collapse in all cases,
			// for style reasons, we only collapse
			// if this is a normal invocation (not
			// a send or converted send or any
			// of the special possibilities). This
			// is a single human's choice
			// and you, future human, can choose
			// otherwise in the future. Note that
			// if you do, you also need
			// to identify and collapse the
			// SendConverted case (which will have
			// 2 arguments, and the second argument
			// will be the string literal).
			else if (
				eEmitType == MemberAccessEmitType.Normal &&
				(null == method || (!method.HasInstanceInvoke() && method.TypeArguments.Length == 0)) &&
				args.Count == 1 &&
				args[0].Expression.Kind() == SyntaxKind.StringLiteralExpression)
			{
				Visit(node.Expression);
				Write(' ');
				Visit(args[0].Expression);
			}
			else
			{
				// TODO: I haven't found another way to handle
				// cases like nameof() reliably - doesn't seem like
				// the semantic model will tell us that 'nameof' is
				// the nameof keyword vs. a function invocation?
				var constant = m_Model.GetConstantValue(node);
				if (constant.HasValue)
				{
					WriteConstant(constant.Value);
					return node;
				}

				// Special handling around calls to object.GetType()
				if (method?.IsGetType() == true)
				{
					// Special handling - we *must* collapse cases of a.GetType()
					// into (instead) invocations of typeof() for targets known
					// to be some of the builtin types for 'int', since it is not
					// dynamically discoverable as different from 'double'.
					if (node.Expression.Kind() == SyntaxKind.SimpleMemberAccessExpression)
					{
						var member = (MemberAccessExpressionSyntax)node.Expression;
						var type = m_Model.GetTypeInfo(member.Expression).Type;
						switch (type.SpecialType)
						{
							case SpecialType.System_Boolean: Write("TypeOf(bool)"); return node;
							case SpecialType.System_Double: Write("TypeOf(double)"); return node;
							case SpecialType.System_Int32: Write("TypeOf(int)"); return node;
							case SpecialType.System_String: Write("TypeOf(String)"); return node;
						}
					}
					// Special case for GetType() in an implicit 'this' context.
					else if (
						eEmitType == MemberAccessEmitType.Extension &&
						node.Expression.Kind() == SyntaxKind.IdentifierName)
					{
						Write("GetType(self)");
						return node;
					}
				}

				// Check for the Conditional compilation attribute - if not set,
				// elide this method call (but sanity check that the call is
				// in a statement context before doing so).
				var bCommentedOut = constant.HasValue;
				if (!bCommentedOut && null != method)
				{
					if (IsCompiledOut(method))
					{
						// Sanity check that our parent is an expression statement.
						if (node.Parent.Kind() != SyntaxKind.ExpressionStatement)
						{
							throw new SlimCSCompilationException(node, "only invocations as statements can be compiled out with the Conditional attribute.");
						}

						bCommentedOut = true;
					}
				}

				try
				{
					// If commented out, comment out what we're about to emit.
					if (bCommentedOut)
					{
						m_iCommentWritingSuppressed++;
						if (1 == m_iCommentWritingSuppressed) { Write("--[["); }
					}

					// Emit the invocation expression (part before the parens).
					int iStart = 0;
					ExpressionSyntax extraArgument = null;
					switch (node.Expression.Kind())
					{
						case SyntaxKind.MemberBindingExpression:
							{
								var member = (MemberBindingExpressionSyntax)node.Expression;
								VisitMemberBindingExpression(member, eEmitType);
								if (eEmitType.IsBase()) { extraArgument = kThisExpression; }
								else if (eEmitType == MemberAccessEmitType.Extension) { extraArgument = BindingTarget; }
								else if (eEmitType == MemberAccessEmitType.SendConverted) { iStart = 1; }
							}
							break;

						case SyntaxKind.SimpleMemberAccessExpression:
							{
								var member = (MemberAccessExpressionSyntax)node.Expression;
								VisitMemberAccessExpression(member, eEmitType);
								if (eEmitType.IsBase()) { extraArgument = kThisExpression; }
								else if (eEmitType == MemberAccessEmitType.Extension) { extraArgument = member.Expression; }
								else if (eEmitType == MemberAccessEmitType.SendConverted) { iStart = 1; }
							}
							break;

						default:
							// If we get here, we can only have the normal emit type
							// or the syntax is invalid.
							if (eEmitType != MemberAccessEmitType.Normal)
							{
								throw new SlimCSCompilationException(node, $"invalid invocation emit type '{eEmitType}'.");
							}

							Visit(node.Expression);

							// Check if we're calling an instance method that's been made local. If so,
							// add kThisExpression as an argument.
							if (CanEmitAsLocal(sym) && !sym.IsStatic)
							{
								extraArgument = kThisExpression;
							}
							break;
					}

					Write('(');

					// Emit type arguments.
					if (null != method && method.IsGenericMethod)
					{
						for (int i = 0; i < method.TypeArguments.Length; ++i)
						{
							if (0 != i)
							{
								Write(',');
							}

							var arg = method.TypeArguments[i];
							var sId = m_Model.LookupOutputId(arg);

							// If arg is itself a type symbol, we may need to resolve it appropriately.
							if (arg.Kind == SymbolKind.TypeParameter &&
								arg.ContainingSymbol.Kind != SymbolKind.Method)
							{
								Write(kLuaSelf);
								Write('.');
							}
							Write(sId);
						}
					}

					// Emit an additional argument if specified.
					if (null != extraArgument)
					{
						if (m_LastChar != '(') { Write(','); }

						// Special handling, need to pop the binding target for the scope of this call.
						if (BindingTarget == extraArgument)
						{
							var topTarget = m_BindingTargets.Back();
							m_BindingTargets.PopBack();
							Visit(topTarget);
							m_BindingTargets.Add(topTarget);
						}
						else
						{
							Visit(extraArgument);
						}
					}

					// Handle various additional arguments above.
					if (args.Count > 0 && m_LastChar != '(') { Write(','); }

					// Emit arguments to the call.
					for (int i = iStart; i < args.Count; ++i)
					{
						if (iStart != i)
						{
							Write(',');
						}

						// Emit delegate conversion handling if
						// the target is a delegate.
						if (null != method)
						{
							// Check if we need to unpack an input array to a params argument.
							if (i + 1 == args.Count &&
								i + 1 == method.Parameters.Length &&
								method.Parameters[i].IsParams)
							{
								var type = m_Model.GetTypeInfo(args[i].Expression).Type;
								if (null != type &&
									type.TypeKind == TypeKind.Array)
								{
									var ident = args[i].Expression as IdentifierNameSyntax;
									if (null == ident || !IsVararg(ident))
									{
										Write(kLuaTableUnpack);
										Write('(');
										Visit(args[i].Expression);
										Write(')');
										continue;
									}
								}
							}
						}

						// Default handling.
						Visit(args[i].Expression);
					}

					Write(')');

					// If commented out, terminate the comment.
					if (bCommentedOut && 1 == m_iCommentWritingSuppressed) { Write("]]"); }
				}
				finally
				{
					if (bCommentedOut)
					{
						--m_iCommentWritingSuppressed;
					}
				}
			}

			return node;
		}

		public override SyntaxNode VisitLiteralExpression(LiteralExpressionSyntax node)
		{
			// Enforce SlimCS constraints.
			m_Model.Enforce(node);

			// In contexts whefre the input context is an integer but the literal
			// is outside the range of an int32, we must pre-emtpively convert the value
			// to its int32 value, or the later double -> int32 conversion will
			// be incorrect (double -> int conversion behaves differently
			// than uint -> int conversion for values outside the range of an int).
			if (node.Kind() == SyntaxKind.NumericLiteralExpression)
			{
				var typeInfo = m_Model.GetTypeInfo(node);
				var type = typeInfo.ConvertedType;
				if (null != type)
				{
					var eSpecialType = type.SpecialType;
					if (eSpecialType.IsIntegralType())
					{
						// Convert to an int an write that value - this
						// forces (e.g.) uint -> int conversion behavior.
						int iValue = node.Token.Value.ToInt32WithInt32Overflow();
						Write(iValue.ToString());
						return node;
					}
				}
			}

			// Fall-through behavior.
			Write(node.Token);
			return node;
		}

		public override SyntaxNode VisitObjectCreationExpression(ObjectCreationExpressionSyntax node)
		{
			var sym = m_Model.GetSymbolInfo(node.Type).Symbol;
			var methodSym = (m_Model.GetSymbolInfo(node).Symbol as IMethodSymbol);
			var args = (null != node.ArgumentList
				? ResolveArguments(node, (null != methodSym ? methodSym.Parameters : ImmutableArray<IParameterSymbol>.Empty), node.ArgumentList.Arguments)
				: new ArgumentSyntax[0]);

			// Semantic error.
			if (null == sym)
			{
				throw new SlimCSCompilationException(node, $"cannot create an instance of type '{node.Type.ToString()}', not newable.");
			}

			// Special classification for various special types
			// handled specifically as Lua hooks.
			var eLuaKind = sym.GetLuaKind();
			switch (eLuaKind)
			{
				// Table constructor.
				case LuaKind.Table:
					{
						// This was a pseudo creation of a Lua style table.
						if (null != node.ArgumentList && node.ArgumentList.Arguments.Count != 0)
						{
							throw new UnsupportedNodeException(node, "creation expected to have 0 arguments.");
						}

						// Check for initializer list.
						if (null != node.Initializer)
						{
							Visit(node.Initializer);
						}
						// Empty constructor.
						else if (null != node.ArgumentList)
						{
							Write("{}");
						}
						else
						{
							throw new UnsupportedNodeException(node, "invalid table creation");
						}
					}
					break;

				// Tuple conversion.
				case LuaKind.Tuple:
					{
						// TODO: Verify conditions under which we can ellide the parentheses.

						for (int i = 0; i < args.Count; ++i)
						{
							if (0 != i)
							{
								Write(", ");
							}
							Visit(args[i].Expression);
						}
					}
					break;

				default:
					{
						// sym must be a type symbol for an object creation expression.
						if (!(sym is ITypeSymbol))
						{
							throw new SlimCSCompilationException(node, $"kind '{sym.Kind}' is invalid target for new expression.");
						}

						// Emit an initializer call
						var bInitializer = (null != node.Initializer);
						var bArrayInit = false;
						if (bInitializer)
						{
							bArrayInit = true;
							foreach (var expr in node.Initializer.Expressions)
							{
								if (expr.Kind() == SyntaxKind.SimpleAssignmentExpression)
								{
									bArrayInit = false;
									break;
								}
							}

							if (bArrayInit)
							{
								Write(kLuaInitArr);
							}
							else
							{
								Write(kLuaInitList);
							}
							Write('(');
						}

						var typeSym = (ITypeSymbol)sym;
						var eKind = typeSym.TypeKind;
						switch (eKind)
						{
							case TypeKind.Class:
								Visit(node.Type);
								goto type_create;

							case TypeKind.Delegate:
								{
									// Sanity check argument count.
									if (args.Count != 1)
									{
										throw new SlimCSCompilationException(node.Type, "expected 1 argument.");
									}

									BindDelegate((INamedTypeSymbol)typeSym, args[0].Expression);
								}
								break;

							case TypeKind.TypeParameter:
								Visit(node.Type);
								goto type_create;

							default:
								throw new SlimCSCompilationException(node.Type, $"unsupported object creation kind '{eKind}'.");

								type_create:
								{
									var sId = (null == methodSym
										? kLuaBaseConstructorName
										: m_Model.LookupOutputId(methodSym));

									Write(':');

									// Can use default new if the constructor name matches the
									// default name - otherwise we must use override new
									// and pass the name as an argument.
									if (sId == kLuaBaseConstructorName)
									{
										Write(kLuaNewDefaultMethod);
										Write('(');
									}
									// Override new.
									else
									{
										Write(kLuaNewOverrideMethod);
										Write('(');
										WriteStringLiteral(sId);
										if (args.Count > 0) { Write(','); }
									}

									for (int i = 0; i < args.Count; ++i)
									{
										if (0 != i)
										{
											Write(',');
										}
										Visit(args[i].Expression);
									}
									Write(')');
								}
								break;
						}

						if (bInitializer)
						{
							int iOutIndex = 1;
							foreach (var expr in node.Initializer.Expressions)
							{
								switch (expr.Kind())
								{
									case SyntaxKind.SimpleAssignmentExpression:
										{
											var assign = (AssignmentExpressionSyntax)expr;
											var left = assign.Left;
											var right = assign.Right;
											var leftSym = m_Model.GetSymbolInfo(left).Symbol;

											// Sanity check.
											if (null == leftSym)
											{
												throw new SlimCSCompilationException(expr, $"invalid assignment target {left} in initializer list.");
											}

											// Special handling for real properties
											// that require invocation of the stter.
											if (leftSym.Kind == SymbolKind.Property)
											{
												var propSym = (IPropertySymbol)leftSym;
												if (!propSym.IsAutoProperty())
												{
													Write(',');
													SeparateForFirst(right);
													Write(kLuaTrue); // The value is a property setter.
													Write(", "); WriteStringLiteral(m_Model.LookupOutputId(propSym.ResolveSetMethod()));
													Write(','); Visit(right);
													continue;
												}
											}

											// Standard field or auto property.
											Write(',');
											SeparateForFirst(right);
											Write(kLuaFalse); // The value is a field or auto property.
											Write(", "); WriteStringLiteral(m_Model.LookupOutputId(leftSym));
											Write(','); Visit(right);
										}
										break;
									default:
										if (bArrayInit) // Value only in this case.
										{
											Write(", ");
											Visit(expr);
										}
										else
										{
											Write(", ");
											Write(kLuaFalse); // The value is a field or auto property (effectively, this is an array).
											Write(", ");
											Write(iOutIndex.ToString()); // Array index, 1-based for Lua.
											Write(", ");
											Visit(expr);
										}

										// Advance.
										iOutIndex++;
										break;
								}
							}

							Write(')');
						}
					}
					break;
			}

			return node;
		}

		public override SyntaxNode VisitParenthesizedExpression(ParenthesizedExpressionSyntax node)
		{
			bool bCanRemoveParens = node.Expression.CanAlwaysRemoveParens();

			if (!bCanRemoveParens) { Write('('); }
			Visit(node.Expression);
			if (!bCanRemoveParens) { Write(')'); }

			return node;
		}

		public override SyntaxNode VisitPostfixUnaryExpression(PostfixUnaryExpressionSyntax node)
		{
			// Handling for special cases - prefix increment and prefix decrement are
			// expanded and are only allowed to be used in a statement context.
			var eKind = node.Kind();
			switch (eKind)
			{
				case SyntaxKind.PostDecrementExpression:
				case SyntaxKind.PostIncrementExpression:
					{
						// In strict C# subset requires assignments to be statements.
						if (!node.IsValidAssignmentContext())
						{
							throw new UnsupportedNodeException(node, "increment and decrement can only be statements, not expressions.");
						}

						// Expand to (operand) = (operand) +- 1

						// Wrap the LHS to determine how we terminate it.
						var propSym = m_Model.GetSymbolInfo(node.Operand).Symbol as IPropertySymbol;
						var lhs = (propSym != null && !propSym.IsAutoProperty()) ? propSym.ResolveSetMethod() : null;

						// Filtering - if the containing type of lhs is not
						// normal (it is a Lua special, like Table), then
						// it won't have any special behavior.
						if (null != lhs && lhs.ContainingType.GetLuaKind() != LuaKind.NotSpecial)
						{
							lhs = null;
						}

						LHS = lhs;
						Visit(node.Operand);
						LHS = null;

						// If there's a LHS method, skip the assignment token.
						if (null == lhs) { Write(" ="); }

						// Write the LHS value.
						Visit(node.Operand);
						Write(eKind == SyntaxKind.PostIncrementExpression ? " + 1" : " - 1");

						// Finish the function call if there's one.
						if (null != lhs) { Write(')'); }
					}
					break;

				default:
					throw new UnsupportedNodeException(node, $"unsupported postfix unary expression '{eKind}'.");
			}

			return node;
		}

		public override SyntaxNode VisitPrefixUnaryExpression(PrefixUnaryExpressionSyntax node)
		{
			// Handling for special cases - prefix increment and prefix decrement are
			// expanded and are only allowed to be used in a statement context.
			var eKind = node.Kind();
			switch (eKind)
			{
				case SyntaxKind.PreDecrementExpression:
				case SyntaxKind.PreIncrementExpression:
					{
						// In strict C# subset requires assignments to be statements.
						if (!node.IsValidAssignmentContext())
						{
							throw new UnsupportedNodeException(node, "increment and decrement can only be statements, not expressions.");
						}

						// Expand to (operand) = (operand) +- 1

						// Wrap the LHS to determine how we terminate it.
						var propSym = m_Model.GetSymbolInfo(node.Operand).Symbol as IPropertySymbol;
						var lhs = (propSym != null && !propSym.IsAutoProperty()) ? propSym.ResolveSetMethod() : null;

						// Filtering - if the containing type of lhs is not
						// normal (it is a Lua special, like Table), then
						// it won't have any special behavior.
						if (null != lhs && lhs.ContainingType.GetLuaKind() != LuaKind.NotSpecial)
						{
							lhs = null;
						}

						LHS = lhs;
						Visit(node.Operand);
						LHS = null;

						// If there's a LHS method, skip the assignment token.
						if (null == lhs) { Write(" ="); }

						// Write the LHS value.
						Visit(node.Operand);
						Write(eKind == SyntaxKind.PreIncrementExpression ? " + 1" : " - 1");

						// Finish the function call if there's one.
						if (null != lhs) { Write(')'); }
					}
					break;

				case SyntaxKind.BitwiseNotExpression:
					Write(kLuaBitNot);
					goto as_function;

					as_function:
					Write('(');
					Visit(node.Operand);
					Write(')');
					break;

				case SyntaxKind.LogicalNotExpression:
					Write(node.OperatorToken);
					Visit(node.Operand);
					break;

				case SyntaxKind.UnaryMinusExpression:
					// If the operand type is an integer type, we need
					// to wrap the negation to make sure -0 doesn't propagate.
					{
						var type = m_Model.GetTypeInfo(node.Operand).Type;

						// If an integral type, we either wrap the value in
						// a call to __i32narrow__ to eliminate the possibility
						// of -0 or we remove that case directly if the value
						// is a constant.
						if (true == type?.SpecialType.IsIntegralType())
						{
							var constant = m_Model.GetConstantValue(node.Operand);

							// If we're negating a constant, just collapse it.
							if (constant.HasValue)
							{
								// TODO: We may eventually support other integral constants.
								var iValue = (int)constant.Value;
								if (iValue != 0)
								{
									iValue = -iValue;
								}
								WriteConstant(iValue);
							}
							// Otherwise, wrap it with a call to __i32narrow__.
							else
							{
								Write(kLuaI32Narrow);
								Write('(');
								Write(node.OperatorToken);
								Visit(node.Operand);
								Write(')');
							}
						}
						// Default handling.
						else
						{
							Write(node.OperatorToken);
							Visit(node.Operand);
						}
					}
					break;

				case SyntaxKind.UnaryPlusExpression:
					// TODO: This applies some conversions to the value - for our purposes,
					// it should be roughly considered and treated like a cast.
					Visit(node.Operand);
					break;

				default:
					throw new UnsupportedNodeException(node, $"unsupported prefix unary expression '{eKind}'.");
			}

			return node;
		}

		public override SyntaxNode VisitThisExpression(ThisExpressionSyntax node)
		{
			Write(node.Token);
			return node;
		}

		public override SyntaxNode VisitThrowExpression(ThrowExpressionSyntax node)
		{
			// TODO: Straight conversion into an error call is not quite right.
			Write(kLuaError);
			Write('(');

			if (null != node.Expression)
			{
				// TODO: Unclear why this is, but throw null always results in
				// throw NullReferenceException().
				var constant = m_Model.GetConstantValue(node.Expression);
				if (constant.HasValue && constant.Value == null)
				{
					Write(kCsharpNullReferenceException);
					Write(':');
					Write(kLuaNewDefaultMethod);
					Write("()");
				}
				else
				{
					Visit(node.Expression);
				}
			}

			Write(')');

			return node;
		}

		public override SyntaxNode VisitTupleExpression(TupleExpressionSyntax node)
		{
			var eParentKind = node.Parent.Kind();
			var args = node.Arguments;

			// Track declaration.
			var bDeclaration = args.First().Expression.Kind() == SyntaxKind.DeclarationExpression;

			// Track whether we're the variable portion of a foreach.
			var bForEachVariable = (
				eParentKind == SyntaxKind.ForEachVariableStatement &&
				((ForEachVariableStatementSyntax)node.Parent).Variable == node);

			// Tracking for iteration count.
			var iCount = 0;

			// Foreach always emit every field, with no further processing necessary.
			if (bForEachVariable)
			{
				iCount = args.Count;
			}
			else
			{
				// If a tuple assignment or declaration contains discards, we handle
				// it as follows:
				// - if all the discards are at the end of the tuple, we just
				//   truncate the count of args. In Lua, we can just omit discards,
				//   we don't need placeholders, unless they serve to set the place
				//   of actual assignments. This is a "true" discard.
				// - if we need to emit an actual discard, which happens
				//   if there are real variables after the discard or if
				//   *all* variables are discarded but we've still arrived
				//   a the tuple node (in which case we're the LHS of an assignment
				//   and the RHS cannot be emitted outside of an assignment statement),
				//   we need to prefix the assignment with a "local _; " statement,
				//   to guarantee that the "fake" discarded variables do not
				//   escape into the global table.
				//
				// Count the number of args we must emit plus whether we
				// will need to emit any discard names explicitly.
				var bHasDiscard = false;
				var bNeedDiscardEmit = false;
				for (int i = 0; i < args.Count; ++i)
				{
					var arg = args[i];
					if (arg.IsDiscard(m_Model))
					{
						// bHasDiscard is now true.
						bHasDiscard = true;
					}
					// Otherwise, this variable must be emitted.
					else
					{
						// Update count.
						iCount = i + 1;

						// Must also set bNeedDiscardEmit if
						// there was a discard prior to this.
						if (bHasDiscard)
						{
							bNeedDiscardEmit = true;
						}
					}
				}

				// If we get here, it means the assignment expression
				// has decided we need to be emitted (e.g. because
				// the RHS is a tuple). So clamp the count to at least
				// 1, unless args.Count is 0, which is an error.
				if (0 == iCount)
				{
					if (args.Count == 0)
					{
						throw new SlimCSCompilationException(node, "tuple count of 0 is unsupported.");
					}
					else
					{
						// Need to emit at least 1.
						iCount = 1;

						// Also need to emit a discard in this case.
						bNeedDiscardEmit = true;
					}
				}

				// Tuples of the format (var a, var b) are collapsed
				// to local a, b in lua. Note that we exclude the local
				// keyword if we're in the initializer block of a foreach
				// loop, since the equivalent lua for k,v in does not use it
				// in that context.
				if (bDeclaration)
				{
					if (NeedsWhitespaceSeparation) { Write(' '); }
					Write(kLuaLocal);
					Write(' ');
				}
				// In this case, we need to prefix the rest of the statement
				// with a "local _; " if bHasVarsAfterDiscard is true.
				//
				// NOTE: This only works because we only allow assignment in
				// statements and because we can shadow variable declarations
				// (we're not checking for this happening multiple times in
				// a single statement block).
				//
				// NOTE: We don't emit this prefix if we're directly within
				// a case block of a switch...case, because that can
				// generate a "jumps into scope of local' error. We rely
				// on the fact that a switch...case block defines its
				// own '_' local variable to store the switch variable,
				// and reuse it here, since we know that once we're
				// in a context that can contain a tuple expression,
				// that temporary is no longer required.
				//
				// TODO: Checking that we're immediately within
				// a switch block is brittle. We're re-using the
				// discard variable that we know will be defined for the
				// block itself, which breaks if that name ever changes
				// or the implementation of switch...case changes such that
				// resuing the variable is no longer valid.
				else if (bNeedDiscardEmit && m_BlockScope.Back().m_eKind != ScopeKind.Switch)
				{
					if (NeedsWhitespaceSeparation) { Write(' '); }
					Write(kLuaLocal);
					Write(" _; ");
				}
			}

			// TODO: Just removing the parens is not always correct,
			// sometimes need to collapse into a table if the assignment is to a single
			// variable.
			for (int i = 0; i < iCount; ++i)
			{
				if (0 != i)
				{
					Write(", ");
				}

				var arg = args[i];
				if (bDeclaration && arg.Expression.Kind() == SyntaxKind.DeclarationExpression)
				{
					var desig = ((DeclarationExpressionSyntax)arg.Expression).Designation;
					var eDesigKind = desig.Kind();
					switch (eDesigKind)
					{
						case SyntaxKind.DiscardDesignation:
							var discard = (DiscardDesignationSyntax)desig;
							Write(discard.UnderscoreToken);
							break;
						case SyntaxKind.SingleVariableDesignation:
							var single = (SingleVariableDesignationSyntax)desig;
							Write(single.Identifier);
							break;
						default:
							throw new UnsupportedNodeException(desig);
					}
				}
				else
				{
					Visit(arg.Expression);
				}
			}

			return node;
		}

		public override SyntaxNode VisitTypeOfExpression(TypeOfExpressionSyntax node)
		{
			// TODO: This works as long as type is not one of the builtin
			// types.
			Write(kLuaTypeOf);
			Write('(');
			Visit(node.Type);
			Write(')');
			return node;
		}
	}
}
