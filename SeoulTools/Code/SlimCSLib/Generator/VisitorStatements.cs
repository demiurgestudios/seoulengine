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
		string GetCaseStatementLabel(object key)
		{
			// Find the nearest switch.
			BlockScope scope = null;
			for (int i = m_BlockScope.Count - 1; i >= 0; --i)
			{
				scope = m_BlockScope[i];
				if (scope.m_eKind == ScopeKind.Switch)
				{
					break;
				}
			}

			// Now look for the corresponding label.
			return scope.SwitchLabels[key];
		}

		string ResolveContinueStatementLabel(SyntaxNode node)
		{
			// Find the innermost loop block.
			int iScopes = m_BlockScope.Count;
			for (int i = (iScopes - 1); i >= 0; --i)
			{
				var loopScope = m_BlockScope[i];
				if (loopScope.m_eKind == ScopeKind.Loop)
				{
					// If a label was allocated already, just return it.
					if (!string.IsNullOrEmpty(loopScope.m_sContinueLabel))
					{
						return loopScope.m_sContinueLabel;
					}

					// Get a set of already defined labels from the current context.
					(var set, var funcScope) = m_BlockScope.GetGotoLabelsInScope();

					// Initial value.
					var sContinueLabel = "continue";
					int iDedup = 0;
					// Dedup
					while (set.Contains(sContinueLabel))
					{
						sContinueLabel = "continue" + iDedup.ToString();
						++iDedup;
					}

					// Assign and return.
					loopScope.m_sContinueLabel = sContinueLabel;
					funcScope.UtilityGotoLabels.Add(sContinueLabel);
					return loopScope.m_sContinueLabel;
				}
			}

			// If we get here, we've encountered a continue statement
			// outside of a loop context.
			throw new SlimCSCompilationException(node, "continue outside loop body.");
		}

		BlockScope CheckTryCatchFinallyUsingContext(SyntaxKind eKind)
		{
			// Search blocks - depending on kind, terminate once
			// we find a certain block type and return false,
			// or return true once we hit a try-catch-finally or using
			// block.
			int iScopes = m_BlockScope.Count;
			for (int i = (iScopes - 1); i >= 0; --i)
			{
				var scope = m_BlockScope[i];
				if (scope.m_eKind == ScopeKind.TryCatchFinallyOrUsing)
				{
					return scope;
				}

				// Can terminate with false if we hit any of the
				// function contexts.
				if (scope.m_eKind == ScopeKind.Function ||
					scope.m_eKind == ScopeKind.Lambda ||
					scope.m_eKind == ScopeKind.TopLevelChunk ||
					scope.m_eKind == ScopeKind.Type)
				{
					return null;
				}

				// If continue, terminate if we hit a loop context.
				if (eKind == SyntaxKind.ContinueStatement &&
					scope.m_eKind == ScopeKind.Loop)
				{
					return null;
				}
				// If break, terminate if we hit a switch or loop context.
				else if (
					eKind == SyntaxKind.BreakStatement &&
					(scope.m_eKind == ScopeKind.Loop || scope.m_eKind == ScopeKind.Switch))
				{
					return null;
				}
			}

			// Nothing special, no need to emit special.
			return null;
		}
		#endregion

		public override SyntaxNode VisitBlock(BlockSyntax node)
		{
			bool bDelimit = node.NeedsDelimiters();

			if (bDelimit)
			{
				SeparateForFirst(node);
				Write(kLuaDo);
				Indent();
			}

			StatementSyntax lastStmt = null;
			for (int iStmt = 0; iStmt < node.Statements.Count; ++iStmt)
			{
				var stmt = node.Statements[iStmt];

				// TODO: Remove this once we don't need this collapse anymore.
				//
				// LuaToCS needs to emit recursively lambdas in the following form:
				// local a = nil; a = function() ... end
				//
				// Because bindings cannot self reference but in Lua, function bindings
				// specifically *can* self reference (a statement of the form local function() ... end)
				//
				// This is a special hook to collapse those forms back to their originals.
				var bPreDeclare = false;
				if (stmt is LocalDeclarationStatementSyntax &&
					iStmt + 1 < node.Statements.Count &&
					node.Statements[iStmt + 1] is ExpressionStatementSyntax &&
					((ExpressionStatementSyntax)node.Statements[iStmt + 1]).Expression is AssignmentExpressionSyntax)
				{
					var left = (LocalDeclarationStatementSyntax)stmt;
					var right = (AssignmentExpressionSyntax)((ExpressionStatementSyntax)node.Statements[iStmt + 1]).Expression;
					if (left.CanCollapseFunctionPreDeclare(right, m_Model))
					{
						// Emit a local keyword, then visit the next statement instead.
						++iStmt;
						stmt = node.Statements[iStmt];
						bPreDeclare = true;
					}
				}

				// Make sure we separate statements on the same line
				// with a semicolon - this is required in Lua,
				// but semicolons are otherwise optional.
				if (null != lastStmt &&
					lastStmt.GetEndLine() == stmt.GetStartLine())
				{
					Write(';');
				}

				// See above commment - handling the pre declare function case.
				if (bPreDeclare)
				{
					var assign = (AssignmentExpressionSyntax)((ExpressionStatementSyntax)stmt).Expression;
					var left = assign.Left;
					var right = assign.Right.TryResolveLambda();

					SeparateForFirst(stmt);
					Write(kLuaLocal);
					Write(' ');
					Write(kLuaFunction);
					Visit(left);
					VisitParenthesizedLambdaExpression(right, true, false);
				}
				else
				{
					Visit(stmt);
				}
				lastStmt = stmt;
			}

			if (bDelimit)
			{
				SeparateForLast(node);
				Outdent();
				Write(kLuaEnd);
			}

			return node;
		}

		public override SyntaxNode VisitBreakStatement(BreakStatementSyntax node)
		{
			// If we're in a try-catch using context, we need to
			// emit a different statement (a return with a special code).
			var specialScope = CheckTryCatchFinallyUsingContext(SyntaxKind.BreakStatement);
			if (null != specialScope)
			{
				// Add the option.
				specialScope.m_eControlOptions |= TryCatchFinallyUsingControlOption.Break;

				// Emit as a return.
				Write(kLuaReturn);
				Write(' ');
				Write(kTryCatchFinallyUsingBreakCode);
			}
			else
			{
				Write(kLuaBreak);
			}

			return node;
		}

		public override SyntaxNode VisitContinueStatement(ContinueStatementSyntax node)
		{
			// If we're in a try-catch using context, we need to
			// emit a different statement (a return with a special code).
			var specialScope = CheckTryCatchFinallyUsingContext(SyntaxKind.ContinueStatement);
			if (null != specialScope)
			{
				// Add the option.
				specialScope.m_eControlOptions |= TryCatchFinallyUsingControlOption.Continue;

				// Emit as a return.
				Write(kLuaReturn);
				Write(' ');
				Write(kTryCatchFinallyUsingContinueCode);
			}
			else
			{
				// A continue is converted into a Goto in Lua.
				var sId = ResolveContinueStatementLabel(node);

				// Now emit.
				Write(kLuaGoto);
				Write(' ');
				Write(sId);
			}

			return node;
		}

		public override SyntaxNode VisitEmptyStatement(EmptyStatementSyntax node)
		{
			// Nop.
			return node;
		}

		public override SyntaxNode VisitExpressionStatement(ExpressionStatementSyntax node)
		{
			m_Indent.Back().StatementStart();
			Visit(node.Expression);
			m_Indent.Back().StatementEnd();
			return node;
		}

		public override SyntaxNode VisitGotoStatement(GotoStatementSyntax node)
		{
			Write(kLuaGoto);

			var eKind = node.Kind();
			switch (eKind)
			{
				case SyntaxKind.GotoCaseStatement:
					{
						var constantValue = m_Model.GetConstantValue(node.Expression);
						if (!constantValue.HasValue)
						{
							throw new SlimCSCompilationException(node.Expression, "constant value expected.");
						}

						// Filter.
						var oValue = constantValue.Value;
						if (null == oValue)
						{
							oValue = kNullSentinel;
						}
						var sLabel = GetCaseStatementLabel(oValue);
						Write(' ');
						Write(sLabel);
					}
					break;

				case SyntaxKind.GotoDefaultStatement:
					{
						var sLabel = GetCaseStatementLabel(kDefaultSentinel);
						Write(' ');
						Write(sLabel);
					}
					break;

				case SyntaxKind.GotoStatement:
					Visit(node.Expression);
					break;

				default:
					throw new SlimCSCompilationException(node, $"unsupported goto target type '{eKind}'.");
			}

			return node;
		}

		void VisitIfStatement(IfStatementSyntax node, bool bElseIf)
		{
			// If handler.
			SeparateForFirst(node);
			Write(bElseIf ? kLuaElseif : kLuaIf);
			Write(' ');
			m_Indent.Back().m_iAdditional += 3; // Line up after "if "
			Visit(node.Condition);
			m_Indent.Back().m_iAdditional -= 3; // Line up after "if "
			Write(' ');
			Write(kLuaThen);
			Indent();
			Visit(node.Statement);
			SeparateForLast(node.Statement);
			Outdent();

			// Else handler.
			if (null != node.Else)
			{
				var stmt = node.Else.Statement;
				if (stmt.Kind() == SyntaxKind.IfStatement)
				{
					VisitIfStatement((IfStatementSyntax)stmt, true);
				}
				else
				{
					SeparateForFirst(node.Else);
					Write(kLuaElse);
					Indent();
					Visit(stmt);
					SeparateForLast(stmt);
					Outdent();
				}
			}
		}

		public override SyntaxNode VisitIfStatement(IfStatementSyntax node)
		{
			VisitIfStatement(node, false);
			SeparateForLast(node);
			Write(kLuaEnd);
			return node;
		}

		public override SyntaxNode VisitLabeledStatement(LabeledStatementSyntax node)
		{
			Write("::");
			WriteIdWithDedup(node);
			Write("::");
			Visit(node.Statement);

			return node;
		}

		public override SyntaxNode VisitLocalDeclarationStatement(LocalDeclarationStatementSyntax node)
		{
			m_Model.Enforce(node);

			m_Indent.Back().StatementStart();

			// If we're parented to a switch case statement, then we suppress the local
			// part of the declaration, as it must have been pre-declared at the top
			// of the switch.
			if (node.Parent.Kind() != SyntaxKind.SwitchSection)
			{
				Write(kLuaLocal);
				Write(' ');
			}

			// Handle the case of local a = function() to local function a
			if (CanConvertToFunctionDeclaration(node))
			{
				Write(kLuaFunction);
				Write(' ');

				var v = node.Declaration.Variables[0];
				var lambda = v.Initializer.Value.TryResolveLambda();
				Write(v.Identifier);
				VisitParenthesizedLambdaExpression(lambda, true, false);
			}
			else
			{
				Visit(node.Declaration);
			}

			m_Indent.Back().StatementEnd();

			return node;
		}

		public override SyntaxNode VisitReturnStatement(ReturnStatementSyntax node)
		{
			var returnType = m_BlockScope.ReturnType(m_Model);

			m_Model.Enforce(node, returnType);

			// Return must be wrapped in a do block in a few cases - lua does
			// not support a return statement that is not the last statement
			// of a block.
			var bNeedDo = (node.Parent is SwitchSectionSyntax);

			// If we're not the last statement of a block.
			if (!bNeedDo && node.Parent is BlockSyntax)
			{
				var block = (BlockSyntax)node.Parent;
				if (block.Statements.Last() != node)
				{
					bNeedDo = true;
				}
			}

			// Also, if we're in a loop context that will emit
			// a continue.
			if (!bNeedDo &&
				m_BlockScope.Back().m_eKind == ScopeKind.Loop &&
				m_BlockScope.Back().m_sContinueLabel != null)
			{
				bNeedDo = true;
			}

			// If at the top level of a switch context, we need
			// to wrap the return statement in a do...end block - it
			// is a syntax error for anything to appear after the return
			// otherwise.
			if (bNeedDo) { Write(kLuaDo); Write(' '); }

			// Return.
			Write(kLuaReturn);

			// If we're in a try-catch using context, we need to
			// prefix a result code
			var specialScope = CheckTryCatchFinallyUsingContext(SyntaxKind.ReturnStatement);
			if (null != specialScope)
			{
				// Add the option.
				specialScope.m_eControlOptions |= TryCatchFinallyUsingControlOption.Return;

				// Check and add multiple returns.
				if (node.Expression != null &&
					node.Expression.Kind() == SyntaxKind.TupleExpression)
				{
					specialScope.m_eControlOptions |= TryCatchFinallyUsingControlOption.ReturnMultiple;
				}

				// Emit the code.
				if (NeedsWhitespaceSeparation) { Write(' '); }
				Write(kTryCatchFinallyUsingReturnCode);

				// Emit a separation comma
				if (node.Expression != null) { Write(','); }
			}

			if (null != node.Expression)
			{
				m_Indent.Back().StatementStart();
				Visit(node.Expression);
				m_Indent.Back().StatementEnd();
			}

			// Terminate the do block on the switch if needed.
			if (bNeedDo)
			{
				if (NeedsWhitespaceSeparation) { Write(' '); }
				Write(kLuaEnd);
			}

			return node;
		}

		public override SyntaxNode VisitSwitchStatement(SwitchStatementSyntax node)
		{
			// We convert a switch-case statement into a repeat ... until true block,
			// so that break has the intended meaning.
			using (var scope = new Scope(this, ScopeKind.Switch, node))
			{
				// Cache - used throughout.
				var switchScope = m_BlockScope.Back();

				// First, generate a label for each case.
				{
					(var set, var funcScope) = m_BlockScope.GetGotoLabelsInScope();
					int iDedup = 0;
					foreach (var sec in node.Sections)
					{
						var sLabel = "case";
						while (set.Contains(sLabel))
						{
							sLabel = "case" + iDedup.ToString();
							++iDedup;
						}

						// Need to add the label to our local set and the function scope.
						set.Add(sLabel);
						funcScope.UtilityGotoLabels.Add(sLabel);

						// Finally, associate.
						switchScope.SwitchSections.Add(sec, sLabel);
						foreach (var lbl in sec.Labels)
						{
							object oKey;
							var eLabelKind = lbl.Kind();
							switch (eLabelKind)
							{
								case SyntaxKind.CaseSwitchLabel:
									{
										var val = ((CaseSwitchLabelSyntax)lbl).Value;
										var constantValue = m_Model.GetConstantValue(val);
										if (!constantValue.HasValue)
										{
											throw new SlimCSCompilationException(val, "constant value expected.");
										}

										// Filter null keys, can't be keys in Dictionary<>
										oKey = constantValue.Value;
										if (oKey == null)
										{
											oKey = kNullSentinel;
										}
									}
									break;
								case SyntaxKind.DefaultSwitchLabel:
									oKey = kDefaultSentinel;
									break;
								default:
									throw new SlimCSCompilationException(lbl, $"unsupported switch case label kind '{eLabelKind}'.");
							}
							switchScope.SwitchLabels.Add(oKey, sLabel);
						}
					}
				}

				// Now, iterate statements of all sections and look for
				// any "top" level local declarations. We need to pre declare
				// these before we emit the dispatch code, since goto
				// in Lua is not allowed to jump into the scope of a local
				// variable.
				List<SyntaxToken> vars = null;
				foreach (var sec in node.Sections)
				{
					foreach (var stmt in sec.Statements)
					{
						if (stmt is LocalDeclarationStatementSyntax)
						{
							var localStmt = (LocalDeclarationStatementSyntax)stmt;
							foreach (var decl in localStmt.Declaration.Variables)
							{
								if (null == vars) { vars = new List<SyntaxToken>(); }
								vars.Add(decl.Identifier);
							}
						}
					}
				}

				// Header.
				Write(kLuaRepeat);
				Indent();

				// Emit the condition assigned to a local variable.
				var sCond = "_"; // TODO:
				if (NeedsWhitespaceSeparation) { Write(' '); }
				Write(kLuaLocal);
				Write(' ');
				Write(sCond);
				Write(" =");
				Visit(node.Expression);

				// If we have local variables declaraitons, pre-declare them now.
				if (null != vars)
				{
					foreach (var v in vars)
					{
						Write(';');
						Write(' ');
						Write(kLuaLocal);
						Write(' ');
						Write(v);
					}
				}

				// The body is two parts - a dispatch
				// section which is a chain of if else if else
				// that performs a goto to the actual body, which
				// is delimited with goto target labels.

				int iCount = node.Sections.Count;

				// Dispatch section - fixed line emit.
				if (iCount > 0)
				{
					// Potentially generate a new list and move the default case
					// to the end of the eval list if there is one.
					IReadOnlyList<SwitchSectionSyntax> sections = node.Sections;
					int iDefault = -1;
					{
						for (int i = 0; i < sections.Count; ++i)
						{
							var sec = sections[i];
							if (sec.Labels.IsDefaultCase())
							{
								iDefault = i;
								break;
							}
						}

						if (iDefault >= 0)
						{
							var a = new SwitchSectionSyntax[sections.Count];
							var iOut = 0;
							for (int iIn = 0; iIn < sections.Count; ++iIn)
							{
								if (iIn == iDefault)
								{
									continue;
								}

								a[iOut++] = sections[iIn];
							}
							a[iOut++] = sections[iDefault];

							// Replace.
							sections = a;
						}
					}

					using (var fixedLine = new FixedLine(this, node.Expression))
					{
						// Track whether we need to terminate the if..elseif..else..end
						// block.
						var bTerminate = true;

						// Separate.
						Write("; ");
						for (int i = 0; i < iCount; ++i)
						{
							var sec = sections[i];
							SeparateForFirst(sec.Labels.First());

							// Emit an else.
							var bDefault = sec.Labels.IsDefaultCase();

							// Don't emit anything if we have only 1 section
							// and it is the default section.
							if (1 == iCount && bDefault)
							{
								bTerminate = false;
							}
							else
							{
								if (bDefault) { Write(kLuaElse); }
								else if (0 != i) { Write(kLuaElseif); }
								else { Write(kLuaIf); }
							}

							// Now emit comparisons on the label expressions.
							if (!bDefault)
							{
								for (int iLabel = 0; iLabel < sec.Labels.Count; ++iLabel)
								{
									var lbl = sec.Labels[iLabel];

									if (NeedsWhitespaceSeparation) { Write(' '); }
									if (0 != iLabel)
									{
										Write(kLuaOr);
										Write(' ');
									}
									Write(sCond);
									Write(' ');
									Write(kLuaEqual);

									// Emit the value.
									var csl = (CaseSwitchLabelSyntax)lbl;

									// The value must be constant, or this is an error.
									// We also *must* evaluate to the constant value
									// here or it could potentially produce erroneous code
									// if (for example) the case is on a local constant value
									// that is defined in the body of the switch.
									var constant = m_Model.GetConstantValue(csl.Value);
									if (!constant.HasValue)
									{
										throw new SlimCSCompilationException(csl, "constant value expected.");
									}

									Write(' ');
									WriteConstant(constant.Value);
								}

								if (NeedsWhitespaceSeparation) { Write(' '); }
								Write(kLuaThen);
							}

							// Get the label for the section.
							var sLabel = switchScope.SwitchSections[sec];

							// Now emit the goto based on the first label.
							if (NeedsWhitespaceSeparation) { Write(' '); }
							Write(kLuaGoto);
							Write(' ');
							Write(sLabel);
						}

						// Terminate.
						if (bTerminate)
						{
							if (NeedsWhitespaceSeparation) { Write(' '); }

							// If we have no default section but have non-default
							// sections, we need an else branch that just breaks.
							if (iDefault < 0 && sections.Count > 0)
							{
								Write(kLuaElse);
								Write(' ');
								Write(kLuaBreak);
								Write(' ');
							}

							Write(kLuaEnd);
						}
					}
				}

				// Body - this matches the layout of the switch case and uses
				// a single goto label for each case.
				for (int i = 0; i < iCount; ++i)
				{
					var sec = node.Sections[i];
					SeparateForFirst(sec.Labels.First());

					// Get the label and emit it.
					var sLabel = switchScope.SwitchSections[sec];
					Write("::");
					Write(sLabel);
					Write("::");

					// Now the block.
					Indent();
					foreach (var stmt in sec.Statements)
					{
						Visit(stmt);
					}
					Outdent();
				}

				// Footer.
				SeparateForLast(node);
				Outdent();
				Write(kLuaUntil);
				Write(' ');
				Write(kLuaTrue);
			}

			return node;
		}

		public override SyntaxNode VisitThrowStatement(ThrowStatementSyntax node)
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

		void VisitTryOrUsingProlog(SyntaxNode node, TryCatchFinallyUsingControlOption eFlags)
		{
			// If we have some options, we need
			// to wrap in a do block and also capture
			// return values.
			if (eFlags == TryCatchFinallyUsingControlOption.None)
			{
				return;
			}

			Write(kLuaDo);
			Write(' ');
			Write(kLuaLocal);
			Write(' ');
			Write(kTryCatchFinallyUsingResVar);
			Write(", ");
			Write(kTryCatchFinallyUsingRetVar);
			Write(" = ");
		}

		void VisitTryOrUsingEpilogue(SyntaxNode node, TryCatchFinallyUsingControlOption eFlags)
		{
			// If we have some options, terminate
			// the do block and perform handling
			// on the results based on options.
			if (eFlags == TryCatchFinallyUsingControlOption.None)
			{
				return;
			}

			var bIf = true;
			if ((eFlags & TryCatchFinallyUsingControlOption.Break) == TryCatchFinallyUsingControlOption.Break)
			{
				if (NeedsWhitespaceSeparation) { Write(' '); }
				Write(bIf ? kLuaIf : kLuaElseif);
				bIf = false;
				Write(' ');
				Write(kTryCatchFinallyUsingBreakCode);
				Write(" == ");
				Write(kTryCatchFinallyUsingResVar);
				Write(' ');
				Write(kLuaThen);
				Write(' ');

				// Handle nesting - if we ourselves are in an enclosing using or try...catch...finally block,
				// we also need to perform the nested behavior.
				var scope = CheckTryCatchFinallyUsingContext(SyntaxKind.BreakStatement);
				if (null != scope)
				{
					// Add the option.
					scope.m_eControlOptions |= TryCatchFinallyUsingControlOption.Break;

					Write(kLuaReturn);
					Write(' ');
					Write(kTryCatchFinallyUsingBreakCode);
				}
				else
				{
					Write(kLuaBreak);
				}
			}
			if ((eFlags & TryCatchFinallyUsingControlOption.Continue) == TryCatchFinallyUsingControlOption.Continue)
			{
				if (NeedsWhitespaceSeparation) { Write(' '); }
				Write(bIf ? kLuaIf : kLuaElseif);
				bIf = false;
				Write(' ');
				Write(kTryCatchFinallyUsingContinueCode);
				Write(" == ");
				Write(kTryCatchFinallyUsingResVar);
				Write(' ');
				Write(kLuaThen);
				Write(' ');

				// Handle nesting - if we ourselves are in an enclosing using or try...catch...finally block,
				// we also need to perform the nested behavior.
				var scope = CheckTryCatchFinallyUsingContext(SyntaxKind.ContinueStatement);
				if (null != scope)
				{
					// Add the option.
					scope.m_eControlOptions |= TryCatchFinallyUsingControlOption.Continue;

					Write(kLuaReturn);
					Write(' ');
					Write(kTryCatchFinallyUsingContinueCode);
				}
				else
				{
					Write(kLuaGoto);
					Write(' ');

					// A continue is converted into a Goto in Lua.
					var sId = ResolveContinueStatementLabel(node);
					Write(sId);
				}
			}
			if ((eFlags & TryCatchFinallyUsingControlOption.Return) == TryCatchFinallyUsingControlOption.Return)
			{
				if (NeedsWhitespaceSeparation) { Write(' '); }
				Write(bIf ? kLuaIf : kLuaElseif);
				bIf = false;
				Write(' ');
				Write(kTryCatchFinallyUsingReturnCode);
				Write(" == ");
				Write(kTryCatchFinallyUsingResVar);
				Write(' ');
				Write(kLuaThen);
				Write(' ');

				// Handle nesting - if we ourselves are in an enclosing using or try...catch...finally block,
				// we also need to perform the nested behavior.
				var scope = CheckTryCatchFinallyUsingContext(SyntaxKind.ReturnStatement);
				if (null != scope)
				{
					// Add the option.
					scope.m_eControlOptions |= TryCatchFinallyUsingControlOption.Return;

					Write(kLuaReturn);
					Write(' ');
					Write(kTryCatchFinallyUsingReturnCode);
					Write(", ");
					Write(kTryCatchFinallyUsingRetVar);
				}
				else
				{
					Write(kLuaReturn);
					Write(' ');
					Write(kTryCatchFinallyUsingRetVar);
				}
			}

			// Terminate the if and then the do block.
			if (NeedsWhitespaceSeparation) { Write(' '); }
			Write(kLuaEnd);
			Write(' ');
			Write(kLuaEnd);
		}

		public override SyntaxNode VisitTryStatement(TryStatementSyntax node)
		{
			// Several different possibilities:
			// - if there are no control flow escapes (see next bullet), then
			//   a try...catch...finally block becomes:
			//
			//   try(try, type0, catch0, type1, catch1, ...)
			//   tryfinally(try, type0, catch0, type1, catch1, ..., finally)
			//
			// - if there are control flow escapes (e.g. a break, continue, or return
			//   within any of the try, catch, or finally blocks):
			//
			//   do local res, ret = try(try, filter0, catch0, filter1, catch1, ...) if 0 == res then break elseif 1 == res then goto continue elseif 2 == res then return ret end end
			//   do local res, ret = tryfinally(try, filter0, catch0, filter1, catch1, ..., finally) if 0 == res then break elseif 1 == res then goto continue elseif 2 == res then return ret end end
			//
			// - finally, the third case occurs if a control flow escapes returns multiple values.

			// First, gather information about control flow.
			TryCatchFinallyUsingControlOption eFlags = TryCatchFinallyUsingControlOption.None;
			using (var lockScope = new OutputLock(this))
			{
				// Try block
				{
					using (var scope = new Scope(this, ScopeKind.TryCatchFinallyOrUsing, node.Block))
					{
						Visit(node.Block);
						eFlags |= m_BlockScope.Back().m_eControlOptions;
					}
				}

				// Catch block(s).
				foreach (var c in node.Catches)
				{
					using (var scope = new Scope(this, ScopeKind.TryCatchFinallyOrUsing, c.Block))
					{
						Visit(c.Block);
						eFlags |= m_BlockScope.Back().m_eControlOptions;
					}
				}

				// Finally block, if defined.
				if (null != node.Finally)
				{
					using (var scope = new Scope(this, ScopeKind.TryCatchFinallyOrUsing, node.Finally.Block))
					{
						Visit(node.Finally.Block);
						eFlags |= m_BlockScope.Back().m_eControlOptions;
					}
				}
			}

			// TODO: Multiple return support.
			if ((eFlags & TryCatchFinallyUsingControlOption.ReturnMultiple) == TryCatchFinallyUsingControlOption.ReturnMultiple)
			{
				throw new SlimCSCompilationException(node, "SlimCS does not yet support multiple return values from within a try-catch-finally or using context.");
			}

			// Common prolog to try(), tryfinally(), and using() invocations.
			VisitTryOrUsingProlog(node, eFlags);

			// Try block - different function based on whether we have a finally or not.
			Write(null == node.Finally ? kPseudoTry : kPseudoTryFinally);
			Write('(');
			Write(kLuaFunction);
			Write("()");
			using (var scope = new Scope(this, ScopeKind.TryCatchFinallyOrUsing, node.Block))
			{
				Indent();
				Visit(node.Block);
				Outdent();
			}
			SeparateForLast(node.Block);
			Write(kLuaEnd);

			// Catch block(s).
			foreach (var c in node.Catches)
			{
				// Use the fallback by default.
				var sVarId = kTryCatchFinallyUsingTestVar;
				var bHasVarId = false;

				// Resolve the actual var name if defined.
				if (null != c.Declaration && SyntaxKind.None != c.Declaration.Identifier.Kind())
				{
					sVarId = c.Declaration.Identifier.ValueText;
					bHasVarId = true;
				}

				// Query function to determine if
				// a particular catch body should
				// be executed.
				Write(',');

				// Catch all, filter function just returns true.
				SeparateForFirst(c);
				if (null == c.Declaration && null == c.Filter)
				{
					Write(kLuaFunction);
					Write("() return true end");
				}
				// Need to apply type selection and possibly filtering.
				else
				{
					Write(kLuaFunction);
					Write('(');
					Write(sVarId);
					Write(") ");

					// Declaration 'is' check.
					if (null != c.Declaration)
					{
						// Resolve type and emit and is.
						Write(kLuaIf);
						Write(' ');
						Write(kLuaIs);
						Write('(');
						Write(sVarId);
						Write(',');
						Visit(c.Declaration.Type);
						Write(") ");
						Write(kLuaThen);
						Write(" return true end");
					}

					// Formatting.
					if (NeedsWhitespaceSeparation) { Write(' '); }

					// Filter check.
					if (null != c.Filter)
					{
						Write(kLuaReturn);
						Write(' ');
						Visit(c.Filter.FilterExpression);
					}
					else
					{
						// Just return false.
						Write(kLuaReturn);
						Write(' ');
						Write(kLuaFalse);
					}

					// Termination.
					if (NeedsWhitespaceSeparation) { Write(' '); }
					Write(kLuaEnd);
				}

				// Now the actual catch block.
				Write(", ");
				Write(kLuaFunction);
				Write('(');
				if (bHasVarId) { Write(sVarId); }
				Write(')');
				using (var scope = new Scope(this, ScopeKind.TryCatchFinallyOrUsing, c.Block))
				{
					Indent();
					Visit(c.Block);
					Outdent();
				}
				SeparateForLast(c.Block);
				Write(kLuaEnd);
			}

			// Finally block, if defined.
			if (null != node.Finally)
			{
				Write(", ");
				Write(kLuaFunction);
				Write("()");
				using (var scope = new Scope(this, ScopeKind.TryCatchFinallyOrUsing, node.Finally.Block))
				{
					Indent();
					Visit(node.Finally.Block);
					Outdent();
				}
				SeparateForLast(node.Finally.Block);
				Write(kLuaEnd);
			}

			// Terminate the try function call.
			Write(')');

			// Common epilogue to try(), tryfinally(), and using() invocations.
			VisitTryOrUsingEpilogue(node, eFlags);

			return node;
		}

		public override SyntaxNode VisitUsingStatement(UsingStatementSyntax node)
		{
			TryCatchFinallyUsingControlOption eFlags = TryCatchFinallyUsingControlOption.None;
			using (var lockScope = new OutputLock(this))
			{
				using (var scope = new Scope(this, ScopeKind.TryCatchFinallyOrUsing, node.Statement))
				{
					Visit(node.Statement);
					eFlags |= m_BlockScope.Back().m_eControlOptions;
				}
			}

			// TODO: Multiple return support.
			if ((eFlags & TryCatchFinallyUsingControlOption.ReturnMultiple) == TryCatchFinallyUsingControlOption.ReturnMultiple)
			{
				throw new SlimCSCompilationException(node, "SlimCS does not yet support multiple return values from within a try-catch-finally or using context.");
			}

			// Common prolog to try(), tryfinally(), and using() invocations.
			VisitTryOrUsingProlog(node, eFlags);

			// Using blocks are converted into a call to the pseudo function using().
			Write(kPseudoUsing);
			Write('(');

			// Will either be a declaration (with a series of variables) or
			// a single expression.
			if (null != node.Declaration)
			{
				var vars = node.Declaration.Variables;
				int iCount = vars.Count;
				for (int i = 0; i < iCount; ++i)
				{
					if (0 != i)
					{
						Write(',');
					}

					var v = vars[i];
					if (null != v.Initializer)
					{
						Visit(v.Initializer.Value);
					}
					else
					{
						SeparateForFirst(v);
						Write(' ');
						Write(kLuaNil);
					}
				}

				// Separate from the body.
				if (iCount > 0)
				{
					Write(',');
				}
			}
			else if (null != node.Expression)
			{
				Visit(node.Expression);
				Write(',');
			}

			// The body becomes an anonymous function.
			if (NeedsWhitespaceSeparation) { Write(' '); }
			Write(kLuaFunction);
			Write('(');

			// Arguments are identifiers if a declaration, otherwise no
			// arguments.
			if (null != node.Declaration)
			{
				var vars = node.Declaration.Variables;
				int iCount = vars.Count;
				for (int i = 0; i < iCount; ++i)
				{
					if (0 != i)
					{
						Write(", ");
					}

					var v = vars[i];
					Write(v.Identifier);
				}
			}

			// Terminate parameters of the function and emit the body.
			Write(')');
			using (var scope = new Scope(this, ScopeKind.TryCatchFinallyOrUsing, node.Statement))
			{
				Indent();
				Visit(node.Statement);
				Outdent();
			}

			// Terminate the body of the function and the using call.
			SeparateForLast(node);
			Write("end)");

			// Common epilogue to try(), tryfinally(), and using() invocations.
			VisitTryOrUsingEpilogue(node, eFlags);

			return node;
		}
	}
}
