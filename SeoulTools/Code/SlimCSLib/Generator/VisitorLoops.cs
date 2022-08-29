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
		bool IsSimpleFor(ForStatementSyntax node, out int riSimpleForStep, out int riConditionMod)
		{
			// Simple for is of the form:
			// for (var i = <init>; i <= <cond>; <incr_by_constant>)

			// No step by default.
			riSimpleForStep = 0;

			// No mod by default.
			riConditionMod = 0;

			// Must have a single variable with no initializers and a single incrementor.
			if (null == node.Declaration ||
				node.Declaration.Variables.Count != 1 ||
				(null != node.Initializers && node.Initializers.Count != 0) ||
				null == node.Incrementors ||
				node.Incrementors.Count != 1)
			{
				return false;
			}

			// Condition must be of the form:
			// <var_name> <= <cond>
			// <var_name> >= <cond>
			var cond = node.Condition;
			if (null == cond)
			{
				return false;
			}

			// TODO: I think these conditions coupled
			// with the conditions on the incrementor mean
			// we'll always produce an equivalent expression,
			// but this is tricky and should be more explicitly
			// verified.
			var eCondKind = cond.Kind();
			if (SyntaxKind.GreaterThanOrEqualExpression != eCondKind &&
				SyntaxKind.LessThanOrEqualExpression != eCondKind)
			{
				// Less than and greater than are supported, but require
				// a modification of the condition expression.
				switch (eCondKind)
				{
					case SyntaxKind.GreaterThanExpression:
						riConditionMod = 1;
						break;
					case SyntaxKind.LessThanExpression:
						riConditionMod = -1;
						break;
					default:
						return false;
				}
			}

			// Left of the condition must be the same symbol as the declared variable.
			var binExpr = (BinaryExpressionSyntax)cond;
			var leftSym = m_Model.GetSymbolInfo(binExpr.Left).Symbol;
			var varSym = m_Model.GetDeclaredSymbol(node.Declaration.Variables[0]);
			if (varSym != leftSym)
			{
				return false;
			}

			// The declared variable cannot be written to in the body of the for loop
			// (it must not be in the current write set of the loop's data flow
			// analysis).
			if (m_BlockScope.Back().m_Flow.WrittenInside.Contains(varSym))
			{
				return false;
			}

			// Finally, incrementor must target the varSym and must be:
			// ++i
			// --i
			// i++
			// i--
			// i += <constant>
			// i -= <constant>
			// i = i + <constant>
			// i = i - <constant>
			var incr = node.Incrementors[0];
			var eIncrKind = incr.Kind();
			switch (eIncrKind)
			{
				// Check target and in some cases, factor.

				// i++, i--
				case SyntaxKind.PostDecrementExpression:
				case SyntaxKind.PostIncrementExpression:
					{
						var postIncr = (PostfixUnaryExpressionSyntax)incr;
						var targetSym = m_Model.GetSymbolInfo(postIncr.Operand).Symbol;

						// Target must be the initializer variable.
						if (targetSym != varSym)
						{
							return false;
						}

						// Success, set the step appropriately.
						riSimpleForStep = (eIncrKind == SyntaxKind.PostDecrementExpression ? -1 : 1);
						return true;
					}

				// --i, ++i
				case SyntaxKind.PreDecrementExpression:
				case SyntaxKind.PreIncrementExpression:
					{
						var preIncr = (PrefixUnaryExpressionSyntax)incr;
						var targetSym = m_Model.GetSymbolInfo(preIncr.Operand).Symbol;

						// Target must be the initializer variable.
						if (targetSym != varSym)
						{
							return false;
						}

						// Success, set the step appropriately.
						riSimpleForStep = (eIncrKind == SyntaxKind.PreDecrementExpression ? -1 : 1);
						return true;
					}

				// i += <constant>, i -= <constant>
				case SyntaxKind.AddAssignmentExpression:
				case SyntaxKind.SubtractAssignmentExpression:
					{
						var assign = (AssignmentExpressionSyntax)incr;
						var targetSym = m_Model.GetSymbolInfo(assign.Left).Symbol;

						// Target must be the initializer variable.
						if (targetSym != varSym)
						{
							return false;
						}

						// Check that right is constant.
						var constant = m_Model.GetConstantValue(assign.Right);
						if (!constant.HasValue || !(constant.Value is int))
						{
							return false;
						}

						// Cast to an int and negate as necessary.
						riSimpleForStep = (int)constant.Value;
						if (SyntaxKind.SubtractAssignmentExpression == eIncrKind) { riSimpleForStep = -riSimpleForStep; }
						return true;
					}

				// i = i + 1, i = i - 1
				case SyntaxKind.SimpleAssignmentExpression:
					{
						var assign = (AssignmentExpressionSyntax)incr;
						var targetSym = m_Model.GetSymbolInfo(assign.Left).Symbol;

						// Target must be the initializer variable.
						if (targetSym != varSym)
						{
							return false;
						}

						// Right must be an add or subtract expression.
						var right = assign.Right;
						var eRightKind = right.Kind();
						switch (eRightKind)
						{
							case SyntaxKind.AddExpression:
							case SyntaxKind.SubtractExpression:
								{
									var binaryOp = (BinaryExpressionSyntax)assign.Right;

									// Left of the add or subtract must also be the varSym.
									var innerTargetSym = m_Model.GetSymbolInfo(binaryOp.Left).Symbol;

									// Target must be the initializer variable.
									if (innerTargetSym != varSym)
									{
										return false;
									}

									// Check that right is constant.
									var constant = m_Model.GetConstantValue(binaryOp.Right);
									if (!constant.HasValue || !(constant.Value is int))
									{
										return false;
									}

									// Cast to an int and negate as necessary.
									riSimpleForStep = (int)constant.Value;
									if (SyntaxKind.SubtractExpression == eRightKind) { riSimpleForStep = -riSimpleForStep; }
									return true;
								}
							default:
								return false;
						}
					}

				default:
					return false;
			}
		}
		#endregion

		public override SyntaxNode VisitDoStatement(DoStatementSyntax node)
		{
			using (var scope = new Scope(this, ScopeKind.Loop, node.Statement))
			{
				Write(kLuaRepeat);
				Indent();
				Visit(node.Statement);
				WriteContinueLabelIfDefined();
				SeparateForLast(node);
				Outdent();
				Write(kLuaUntil);
				VisitNegated(node.Condition);
			}

			return node;
		}

		public override SyntaxNode VisitForEachStatement(ForEachStatementSyntax node)
		{
			using (var scope = new Scope(this, ScopeKind.Loop, node.Statement))
			{
				// Special handling if the range method is being used
				// as the source of the enumeration.
				var call = (node.Expression as InvocationExpressionSyntax);
				var bWrapArrayRead = false;
				IArrayTypeSymbol arrayType = null;
				if (null != call && call.Expression.IsLuaRange())
				{
					var args = call.ArgumentList.Arguments;
					if (args.Count != 2 && args.Count != 3)
					{
						throw new SlimCSCompilationException(call, "expected 2 or 3 arguments to range.");
					}

					Write(kLuaFor);
					Write(' ');
					Write(node.Identifier, true);
					Write(" =");
					Visit(args[0].Expression);
					Write(',');
					Visit(args[1].Expression);
					if (args.Count == 3)
					{
						Write(',');
						Visit(args[2].Expression);
					}
				}
				else
				{
					var typeInfo = m_Model.GetTypeInfo(node.Expression);
					if (typeInfo.ConvertedType == null)
					{
						throw new SlimCSCompilationException(node, $"unsupported foreach target '{node.Expression}'.");
					}

					// Deliberately don't use ConvertedType here, as this will be IEnumerable
					// in foreach contexts.
					var eKind = typeInfo.Type.TypeKind;
					switch (eKind)
					{
						case TypeKind.Array:
							// Arrays are wrangled by using 'false' as a placeholder in
							// arrays of reference types. We must apply a conversion
							// expression (a[i] or nil) to all reads.
							{
								arrayType = (IArrayTypeSymbol)typeInfo.Type;
								bWrapArrayRead = arrayType.ElementType.NeedsArrayPlaceholder();
								Write(kLuaFor);
								Write(" _, ");
								Write(node.Identifier, true);
								Write(' ');
								Write(kLuaIn);
								Write(' ');
								Write(kLuaIpairs);
								Write('(');
								Visit(node.Expression);
								Write(')');
							}
							break;

						// TODO: Not always correct.
						case TypeKind.Interface:
							Write(kLuaFor);
							Write(' ');
							Write(node.Identifier, true);
							Write(' ');
							Write(kLuaIn);
							Write(' ');
							Visit(node.Expression);
							break;

						default:
							throw new SlimCSCompilationException(node, $"unsupported foreach type kind '{eKind}'.");
					}
				}

				// Terminate loop header.
				Write(' ');
				Write(kLuaDo);

				// Insert read conversion if needed.
				if (bWrapArrayRead && IsUsed(node.Identifier.ValueText))
				{
					Write(' ');
					Write(node.Identifier);
					Write(" = ");
					Write(node.Identifier);
					Write(" or ");
					WriteArrayReadPlaceholder(arrayType);
					Write(';'); // TODO: Conditionally add the semicolon.
				}

				// Main body of the loop.
				Indent();
				Visit(node.Statement);
				WriteContinueLabelIfDefined();
				SeparateForLast(node);
				Outdent();
				Write(kLuaEnd);
			}

			return node;
		}

		public override SyntaxNode VisitForEachVariableStatement(ForEachVariableStatementSyntax node)
		{
			using (var scope = new Scope(this, ScopeKind.Loop, node.Statement))
			{
				Write(kLuaFor);
				Write(' ');
				Visit(node.Variable);
				Write(' ');
				Write(kLuaIn);
				Write(' ');
				Visit(node.Expression);
				Write(' ');
				Write(kLuaDo);
				Indent();
				Visit(node.Statement);
				WriteContinueLabelIfDefined();
				SeparateForLast(node);
				Outdent();
				Write(kLuaEnd);
			}

			return node;
		}

		public override SyntaxNode VisitForStatement(ForStatementSyntax node)
		{
			using (var scope = new Scope(this, ScopeKind.Loop, node.Statement))
			{
				// Simple for is of the form:
				// for (var i = <init>; i <= <cond>; <incr_by_constant>)
				int iSimpleForStep = 0;
				int iSimpleForConditionMod = 0;
				var bSimpleFor = IsSimpleFor(node, out iSimpleForStep, out iSimpleForConditionMod);
				if (bSimpleFor)
				{
					var v = node.Declaration.Variables[0];

					Write(kLuaFor);
					Write(' ');

					// Variable identifier.
					WriteIdWithDedup(v);
					Write(" =");

					// Initializer.
					Visit(v.Initializer.Value);
					Write(",");

					// Special handling - if the condition is constant,
					// and we have a non-zero mod to the condition,
					// apply it now.
					var binExpr = (BinaryExpressionSyntax)node.Condition;
					bool bDone = false;
					if (0 != iSimpleForConditionMod)
					{
						var constant = m_Model.GetConstantValue(binExpr.Right);
						if (constant.HasValue && constant.Value is int)
						{
							var iCondition = (int)constant.Value;
							iCondition += iSimpleForConditionMod;
							Write(' ');
							Write(iCondition.ToString());
							bDone = true;
						}
					}

					// Standard condition handling.
					if (!bDone)
					{
						if (0 != iSimpleForConditionMod) { Write(" ("); }
						Visit(binExpr.Right);
						if (0 != iSimpleForConditionMod)
						{
							Write(") ");
							if (iSimpleForConditionMod < 0) { Write("-"); Write((-iSimpleForConditionMod).ToString()); }
							else { Write("+"); Write(iSimpleForConditionMod.ToString()); }
						}
					}

					// Only need the step if iSimpleForStep != 1
					if (1 != iSimpleForStep)
					{
						Write(", ");
						Write(iSimpleForStep.ToString());
					}

					// For loop body.
					Write(' ');
					Write(kLuaDo);
					Indent();
					Visit(node.Statement);
					WriteContinueLabelIfDefined();
					SeparateForLast(node);
					Outdent();
					Write(kLuaEnd);
				}
				// In the complete case, the for loop becomes a do block wrapping a while statement.
				else
				{
					Write(kLuaDo);
					Write(' ');

					// Declaration.
					if (null != node.Declaration)
					{
						Write(kLuaLocal);
						Visit(node.Declaration);
					}

					// Initializers.
					if (null != node.Initializers)
					{
						foreach (var v in node.Initializers)
						{
							Write(';');
							Visit(v);
						}
					}

					// Start of the while.
					if (NeedsWhitespaceSeparation) { Write(' '); }
					Write(kLuaWhile);
					Write(' ');
					if (null == node.Condition) { Write(kLuaTrue); }
					else { Visit(node.Condition); }
					Write(' ');
					Write(kLuaDo);
					Indent();
					Visit(node.Statement);
					WriteContinueLabelIfDefined();

					// Incrementors - need to fix the line in this case,
					// as the incrementors input line is different from the
					// output line, probably.
					if (null != node.Incrementors && node.Incrementors.Count > 0)
					{
						using (var fixedLine = new FixedLine(this, node.Incrementors[node.Incrementors.Count - 1]))
						{
							Write(' ');
							for (int i = 0; i < node.Incrementors.Count; ++i)
							{
								if (0 != i)
								{
									Write(';');
								}
								Visit(node.Incrementors[i]);
							}
						}
					}

					// Terminate the while loop.
					SeparateForLast(node);
					Outdent();
					Write(kLuaEnd);

					// Terminate the outer do.
					Write(' ');
					Write(kLuaEnd);
				}
			}

			return node;
		}

		public override SyntaxNode VisitWhileStatement(WhileStatementSyntax node)
		{
			using (var scope = new Scope(this, ScopeKind.Loop, node.Statement))
			{
				Write(kLuaWhile);
				Write(' ');
				Visit(node.Condition);
				Write(' ');
				Write(kLuaDo);
				Indent();
				Visit(node.Statement);
				WriteContinueLabelIfDefined();
				SeparateForLast(node);
				Outdent();
				Write(kLuaEnd);
			}

			return node;
		}
	}
}
