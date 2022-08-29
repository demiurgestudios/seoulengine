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
		sealed class Scope : IDisposable
		{
			readonly Visitor m_Owner;
			readonly CSharpSyntaxNode m_Node;
			readonly ScopeKind m_eKind;

			public Scope(Visitor owner, ScopeKind eKind, CSharpSyntaxNode node)
			{
				m_Owner = owner;
				m_Node = node;
				m_eKind = eKind;

				if (null == m_Node)
				{
					return;
				}

				switch (m_eKind)
				{
					case ScopeKind.Function:
					case ScopeKind.Loop:
					case ScopeKind.Switch:
						m_Owner.m_Indent.Back().StatementStart();
						m_Owner.BeginBlockScope(m_eKind, m_Node);
						break;

					case ScopeKind.Lambda:
					case ScopeKind.TryCatchFinallyOrUsing:
						m_Owner.BeginBlockScope(m_eKind, m_Node);
						break;

					case ScopeKind.TopLevelChunk:
						m_Owner.BeginTypeScope(node.GetContainingTypeSyntax());
						m_Owner.BeginBlockScope(m_eKind, m_Node);
						break;

					case ScopeKind.Type:
						m_Owner.BeginTypeScope((BaseTypeDeclarationSyntax)m_Node);
						break;
				}
			}

			public void Dispose()
			{
				if (null != m_Node)
				{
					switch (m_eKind)
					{
						case ScopeKind.Function:
						case ScopeKind.Loop:
						case ScopeKind.Switch:
							m_Owner.EndBlockScope();
							m_Owner.m_Indent.Back().StatementEnd();
							break;

						case ScopeKind.Lambda:
						case ScopeKind.TryCatchFinallyOrUsing:
							m_Owner.EndBlockScope();
							break;

						case ScopeKind.TopLevelChunk:
							m_Owner.EndBlockScope();
							m_Owner.EndTypeScope();
							break;

						case ScopeKind.Type:
							m_Owner.EndTypeScope();
							break;
					}
				}
			}
		}

		readonly List<BlockScope> m_BlockScope = new List<BlockScope>();

		struct TypeScope
		{
			public HashSet<string> m_Globals;
			public Dictionary<SyntaxNode, bool> m_tDeclarationsBoundAsTopLevelLocals;
			public HashSet<ISymbol> m_MembersBoundAsTopLevelLocals;
			public INamedTypeSymbol m_Symbol;
		}

		readonly List<TypeScope> m_TypeScope = new List<TypeScope>();
		readonly CommandArgs m_Args;
		readonly Model m_Model;
		readonly List<ExpressionSyntax> m_BindingTargets = new List<ExpressionSyntax>();
		readonly HashSet<string> m_ConditionalCompilationSymbols;

		ExpressionSyntax BindingTarget { get { return m_BindingTargets.Count == 0 ? null : m_BindingTargets.Back(); } }

		// "Fallback" when the block scope is empty - can happen in
		// block initializers at top-level class scope.
		IMethodSymbol m_LHS = null;

		void BeginBlockScope(ScopeKind eKind, CSharpSyntaxNode node)
		{
			// Retrieve a lookup table of global symbols accessible
			// from this type context.
			var globals = LookupGlobals(node);

			// The 'base' or 'this' part of a constructor cannot be analyzed
			// for data flow, so we need to add any of its reads as extra
			// read params.
			HashSet<String> extraRead = null;
			var constructor = node.Parent as ConstructorDeclarationSyntax;
			if (null != constructor)
			{
				var init = constructor.Initializer;
				if (null != init)
				{
					foreach (var arg in init.ArgumentList.Arguments)
					{
						var argDataFlow = m_Model.AnalyzeDataFlow(arg.Expression);
						foreach (var sym in argDataFlow.ReadInside)
						{
							if (null == extraRead) { extraRead = new HashSet<string>(); }
							extraRead.Add(m_Model.LookupOutputId(sym));
						}
					}
				}
			}

			// We also track any variable assignments in the constructor scope
			// to help us eliminate redundant member initializations in the output
			// Lua code.
			HashSet<ISymbol> extraWrite = null;
			if (null != constructor && null != constructor.Body)
			{
				var body = constructor.Body;

				// TODO: We can check parent constructor bodies for virtual
				// invocations and then skip this stuff if that does not happen - it's
				// complex, since we need to check invocations of non-virtuals that
				// may invoke virtuals, but possibly worth it?

				// Omit extra writes if the construct will invoke a parent - due to virtual
				// methods, we must always emit inline initializers in this case, since
				// the virtual invocations may see the variables before the constructor
				// has a chance to run.
				var type = m_Model.GetDeclaredSymbol(constructor).ContainingType;
				if (null == constructor.Initializer && null == ResolveBaseConstructor(type))
				{
					// TODO: More correct would involve control flow analysis,
					// but not worth the trouble right now.
					//
					// Only traverse immediate children - we don't want to consider assignments
					// in blocks scopes in case they are conditional.
					foreach (var stmt in body.Statements)
					{
						ExpressionSyntax child = null;
						if (stmt is ExpressionStatementSyntax exprStmt)
						{
							child = exprStmt.Expression;
						}
						else
						{
							continue;
						}

						// Not an assignment, can skip this node.
						if (!(child is AssignmentExpressionSyntax assign))
						{
							continue;
						}

						if (assign.Left is TupleExpressionSyntax tuple)
						{
							foreach (var arg in tuple.Arguments)
							{
								var sym = m_Model.GetSymbolInfo(arg.Expression).Symbol;
								if (null != sym)
								{
									if (null == extraWrite) { extraWrite = new HashSet<ISymbol>(); }
									extraWrite.Add(sym);
								}
							}
						}
						else
						{
							var sym = m_Model.GetSymbolInfo(assign.Left).Symbol;
							if (null != sym)
							{
								if (null == extraWrite) { extraWrite = new HashSet<ISymbol>(); }
								extraWrite.Add(sym);
							}
						}
					}
				}
			}

			// Perform data flow analysis on the given node.
			var dataFlow = m_Model.AnalyzeDataFlow(node);
			if (!dataFlow.Succeeded)
			{
				throw new SlimCSCompilationException(node, "data flow analysis failed.");
			}

			// Track.
			var data = new BlockScope(node, eKind, dataFlow, globals, extraRead, extraWrite);
			m_BlockScope.Add(data);

			// Generate remaps at stack top.
			m_BlockScope.DedupTop();

			// Final step, find any labelled statements in the block and dedup
			// those as well.
			if (data.m_eKind == ScopeKind.Function ||
				data.m_eKind == ScopeKind.Lambda ||
				data.m_eKind == ScopeKind.TopLevelChunk)
			{
				// Find all label statements defined in the body of the function.
				foreach (var child in data.m_Site.DescendantNodes())
				{
					if (!(child is LabeledStatementSyntax))
					{
						continue;
					}

					// Get the labeled data.
					var labeled = (LabeledStatementSyntax)child;
					var labeledSym = m_Model.GetDeclaredSymbol(labeled);

					// Attempt to dedup the label name.
					var sDedup = m_BlockScope.GenDedupedId(labeledSym.Name, false);

					// Need to insert the remap.
					if (sDedup != labeledSym.Name)
					{
						if (null == data.m_DedupSym)
						{
							data.m_DedupSym = new Dictionary<ISymbol, string>();
						}
						data.m_DedupSym.Add(labeledSym, sDedup);
					}
				}
			}
		}

		/// <summary>
		/// Add one level to the type scope, used
		/// to add specifiers to assignments of types defined
		/// in a .lua file.
		/// </summary>
		/// <param name="type">The type to add to the scope.</param>
		void BeginTypeScope(BaseTypeDeclarationSyntax type)
		{
			var scope = new TypeScope();
			scope.m_Globals = LookupGlobals(type);
			scope.m_Symbol = m_Model.GetDeclaredSymbol(type);

			// Fill in m_DeclarationsBoundAsTopLevelLocals and
			// m_MembersBoundAsTopLevelLocals. To qualify to
			// be emitted as a top-level local (at Lua file scope),
			// a member:
			// - must be private
			// - must be a field or method.
			// - must be static or const if a field.
			// - the containing type must be top-level (not nested).
			// - the containing type cannot be partial.
			// - (special case) cannot be equal to the main method, if one
			//   has been found and defined.
			HashSet<ISymbol> mems = null;
			Dictionary<SyntaxNode, bool> decls = null;
			if (m_TypeScope.Count == 0 &&
				type is ClassDeclarationSyntax &&
				!type.Modifiers.HasToken(SyntaxKind.PartialKeyword))
			{
				var classDecl = (ClassDeclarationSyntax)type;
				foreach (var mem in classDecl.Members)
				{
					var eKind = mem.Kind();
					switch (eKind)
					{
							// Delegates are just placeholders, effectively,
							// so we can just ignore these (do not
							// use but do not terminate).
						case SyntaxKind.DelegateDeclaration:
							break;

							// TODO: May have a problem here
							// with nested members and member collisions.
							//
							// Nested types do not affect local compatibility
							// of top level types.
						case SyntaxKind.ClassDeclaration:
						case SyntaxKind.EnumDeclaration:
						case SyntaxKind.StructDeclaration:
							break;

						// Interfaces are just placeholders, effectively,
						// so we can also just ignore these.
						case SyntaxKind.InterfaceDeclaration:
							break;

						// Potentially a member.
						case SyntaxKind.FieldDeclaration:
						case SyntaxKind.MethodDeclaration:
							// TODO: Dependencies between members
							// must be determined. In particular, methods
							// may reference methods that come later (textually)
							// in the file, which would mean we need to
							// pre declare the local variable.

							// Field can be emitted as a local if it is private
							// and static or const.
							if (mem is FieldDeclarationSyntax)
							{
								var field = (FieldDeclarationSyntax)mem;
								(var bStaticOrConst, var bPrivate) = field.Modifiers.IsConstOrStaticAndPrivate();
								if (bStaticOrConst && bPrivate)
								{
									// First, check declaration symbols for collisions.
									foreach (var v in field.Declaration.Variables)
									{
										var sym = m_Model.GetDeclaredSymbol(v);

										// Skip if there is a collision with a global symbol.
										if (scope.m_Globals.Contains(m_Model.LookupOutputId(sym)))
										{
											goto skip;
										}
									}

									// Add the field.
									if (null == decls) { decls = new Dictionary<SyntaxNode, bool>(); }
									decls.Add(field, false);

									// Add each declaration.
									foreach (var v in field.Declaration.Variables)
									{
										var sym = m_Model.GetDeclaredSymbol(v);
										if (null == mems) { mems = new HashSet<ISymbol>(); }
										mems.Add(sym);
									}

								skip:
									continue;
								}
							}
							// Method can be emitted as a local if it is private.
							else if (mem is MethodDeclarationSyntax)
							{
								var method = (MethodDeclarationSyntax)mem;
								(var _, var bPrivate) = method.Modifiers.IsConstOrStaticAndPrivate();
								if (bPrivate)
								{
									// Resolve the method symbol.
									var sym = m_Model.GetDeclaredSymbol(method);

									// Skip if the main method.
									if (sym == m_Model.SharedModel.MainMethod)
									{
										continue;
									}

									// Skip if there is a collision with a global symbol.
									if (scope.m_Globals.Contains(m_Model.LookupOutputId(sym)))
									{
										continue;
									}

									// Skip if the Lua class "initializer" pseudo function -
									// must be part of the class to be available to the class
									// setup harness.
									if (method.IsLuaClassInitPseudo())
									{
										continue;
									}

									// Add the method.
									if (null == decls) { decls = new Dictionary<SyntaxNode, bool>(); }
									decls.Add(method, false);

									// Reduce and resolve if an extension or generic specialization of
									// a generic method.
									sym = sym.Reduce();

									// Add.
									if (null == mems) { mems = new HashSet<ISymbol>(); }
									mems.Add(sym);
									continue;
								}
							}
							break;

						case SyntaxKind.ConstructorDeclaration:
						case SyntaxKind.DestructorDeclaration:
						case SyntaxKind.EventDeclaration:
						case SyntaxKind.EventFieldDeclaration:
						case SyntaxKind.IndexerDeclaration:
                        case SyntaxKind.OperatorDeclaration:
						case SyntaxKind.PropertyDeclaration:
							break;

						default:
							throw new SlimCSCompilationException(mem, $"class members of kind '{eKind}' not yet supported.");
					}
				}
			}
			scope.m_tDeclarationsBoundAsTopLevelLocals = decls;
			scope.m_MembersBoundAsTopLevelLocals = mems;

			m_TypeScope.Add(scope);
		}

		bool CanEmitAsLocal(SyntaxNode node, out bool bDeclared)
		{
			// Initially false.
			bDeclared = false;

			if (null == node) { return false; }

			if (m_TypeScope.Count == 0)
			{
				return false;
			}

			if (m_TypeScope.Back().m_tDeclarationsBoundAsTopLevelLocals == null)
			{
				return false;
			}

			return m_TypeScope.Back().m_tDeclarationsBoundAsTopLevelLocals.TryGetValue(node, out bDeclared);
		}

		bool CanEmitAsLocal(ISymbol sym)
		{
			if (null == sym) { return false; }

			if (m_TypeScope.Count == 0)
			{
				return false;
			}

			var scope = m_TypeScope[0];
			if (scope.m_MembersBoundAsTopLevelLocals == null)
			{
				return false;
			}

			// Reduce and resolve if an extension or generic specialization of
			// a generic method.
			sym = sym.Reduce();

			// Check the root type scope for any local bindings.
			return scope.m_MembersBoundAsTopLevelLocals.Contains(sym);
		}

		void OnDeclare(SyntaxNode node)
		{
			if (m_TypeScope.Count > 0 &&
				m_TypeScope.Back().m_tDeclarationsBoundAsTopLevelLocals != null &&
				m_TypeScope.Back().m_tDeclarationsBoundAsTopLevelLocals.ContainsKey(node))
			{
				m_TypeScope.Back().m_tDeclarationsBoundAsTopLevelLocals[node] = true;
			}
		}

		void EndBlockScope()
		{
			// Pop the top level data flow analysis.
			m_BlockScope.PopBack();
		}

		/// <summary>
		/// Terminates a type scope. Must be called in-sync
		/// with BeginTypeScope().
		/// </summary>
		void EndTypeScope()
		{
			m_TypeScope.PopBack();
		}

		bool IsCompiledOut(IMethodSymbol sym)
		{
			var attrs = sym.GetAttributes();
			bool bAtLeastOneCondition = false;
			foreach (var attr in attrs)
			{
				if (attr.AttributeClass.Name == "ConditionalAttribute")
				{
					var sArg = attr.ConstructorArguments[0].Value.ToString();
					if (m_ConditionalCompilationSymbols.Contains(sArg))
					{
						return false;
					}
					bAtLeastOneCondition = true;
				}
			}

			// If we get here and have at least one condition attribute,
			// the method is compiled out.
			return bAtLeastOneCondition;
		}

		/// <summary>
		/// A non-null method symbol if the left-hand side of the current assignment
		/// statement is an event, property, or indexer set.
		/// </summary>
		IMethodSymbol LHS
		{
			get
			{
				return (m_BlockScope.Count == 0 ? m_LHS : m_BlockScope.Back().m_LHS);
			}

			set
			{
				if (m_BlockScope.Count == 0) { m_LHS = value; }
				else { m_BlockScope.Back().m_LHS = value; }
			}
		}

		HashSet<string> LookupGlobals(SyntaxNode node)
		{
			var globals = new HashSet<string>();
			{
				// Enumerate types.
				var symbols = m_Model.LookupNamespacesAndTypes(node.SpanStart);
				foreach (var sym in symbols)
				{
					// Skip namespaces and alises (e.g. using A = A.B.C;),
					// we do not use namespaces in the output Lua.
					if (sym.Kind == SymbolKind.Alias ||
						sym.Kind == SymbolKind.Namespace)
					{
						continue;
					}

					globals.Add(m_Model.LookupOutputId(sym));
				}

				// Add any local symbols.
				if (m_TypeScope.Count > 0)
				{
					var typeScope = m_TypeScope.Back();
					if (null != typeScope.m_MembersBoundAsTopLevelLocals)
					{
						foreach (var sym in typeScope.m_MembersBoundAsTopLevelLocals)
						{
							globals.Add(m_Model.LookupOutputId(sym));
						}
					}
				}
			}

			return globals;
		}

		SyntaxNode VisitNegated(ExpressionSyntax node)
		{
			var eKind = node.Kind();
			switch (eKind)
			{
				// Boolean.
				case SyntaxKind.FalseLiteralExpression:
					Visit(kTrueExpression);
					break;
				case SyntaxKind.TrueLiteralExpression:
					Visit(kFalseExpression);
					break;

				// If already a not, drop the not.
				case SyntaxKind.LogicalNotExpression:
					var logicalNot = (PrefixUnaryExpressionSyntax)node;
					Visit(logicalNot.Operand);
					break;

				// These variations don't require parens.
				case SyntaxKind.IdentifierName:
				case SyntaxKind.ParenthesizedExpression:
					Write(' ');
					Write(kLuaNot);
					Visit(node);
					break;

				default:
					Write(' ');
					Write(kLuaNot);
					Write('(');
					Visit(node);
					Write(')');
					break;
			}

			return node;
		}
		#endregion

		public Visitor(
			CommandArgs args,
			Model model,
			TextWriter writer)
		{
			m_Args = args;
			m_Model = model;
			m_Writer = writer;
			m_ConditionalCompilationSymbols = new HashSet<string>(m_Args.m_ConditionalCompilationSymbols);

			// Always one indent level.
			m_Indent.Add(new IndentData(0, 0));
			var root = m_Model.SyntaxTree.GetRoot();
			GatherComments(root);
			Visit(root);
			m_Indent.PopBack();
		}

		public void Dispose()
		{
			// Nop
		}

		/// <summary>
		/// Fallback handler. Used to report C# nodes that SlimCS does not (deliberately) support.
		/// </summary>
		/// <param name="node">Node to process.</param>
		public override SyntaxNode DefaultVisit(SyntaxNode node)
		{
			throw new UnsupportedNodeException(node);
		}

		static bool SkipConstantEmit(SyntaxNode node)
		{
			// No need if a literal.
			if (node is LiteralExpressionSyntax) { return true; }

			// Also no need if a unary minus or plus on a literal.
			if (node.Kind() == SyntaxKind.UnaryMinusExpression || node.Kind() == SyntaxKind.UnaryPlusExpression)
			{
				if (((PrefixUnaryExpressionSyntax)node).Operand is LiteralExpressionSyntax) { return true; }
			}

			return false;
		}

		public override SyntaxNode Visit(SyntaxNode node)
		{
			if (null == node) { return node; }

			SeparateForFirst(node);

			// Sanity check - we must be achieving this
			// as debugger support depends on it. Disable
			// this check if output is disabled.
			if (!OutputLocked)
			{
				// Only applies to statement nodes (finest granularity
				// of debugging is at the statement level and their
				// are general cases that we need to support, e.g.,
				// chained method calls reversing into nested extension
				// calls).
				if (node is StatementSyntax && node.GetStartLine() != m_iOutputLine)
				{
					// If we're fixing the current line, then
					// line mismatches are expected.
					if (!FixedAtCurrentLine && (
						// The only typical situation in which this is expected and allowed is
						// when node is a block that's already been delimited by its
						// enclosing context.
						!(node is BlockSyntax) || ((BlockSyntax)node).NeedsDelimiters()) &&
						// Another situation, arguments that have been re-ordered due
						// to (e.g.) named arguments.
						!(node?.Parent is ArgumentSyntax))
					{
						// Last check - if the node has the kLineMismatchAllowed,
						// we expect the start line to be 0 and invalid.
						if (!node.HasAnnotation(kLineMismatchAllowed) ||
							node.GetStartLine() != 0)
						{
							throw new SlimCSCompilationException(
								node,
								$"target line number is {node.GetStartLine()} but output is at {m_iOutputLine}, this is a compiler bug.");
						}
					}
				}
			}

			var bCommented = false;
			if (0 == m_iCommentWritingSuppressed &&
				0 == m_iConstantEmitSuppressed &&
				!SkipConstantEmit(node))
			{
				var constant = m_Model.GetConstantValue(node);
				if (constant.HasValue)
				{
					WriteConstant(constant.Value);
					bCommented = true;
					++m_iCommentWritingSuppressed;
					if (1 == m_iCommentWritingSuppressed) { Write("--[["); }
				}
			}

			try
			{ 
				return base.Visit(node);
			}
			finally
			{
				if (bCommented)
				{
					if (1 == m_iCommentWritingSuppressed) { Write("]]");
	}
					--m_iCommentWritingSuppressed;
				}
			}
		}

		public override SyntaxNode VisitCompilationUnit(CompilationUnitSyntax node)
		{
			// Add any conditional define symbols for this unit.
			for (var directive = node.GetFirstDirective(); null != directive; directive = directive.GetNextDirective())
			{
				if (directive.Kind() == SyntaxKind.DefineDirectiveTrivia)
				{
					var define = (DefineDirectiveTriviaSyntax)directive;
					m_ConditionalCompilationSymbols.Add(define.Name.ValueText);
				}
			}

			// Now emit the members.
			var bPlaceholder = false;
			foreach (var m in node.Members)
			{
				// Handling for our special root wrapper classes
				// generated by LuaToCS.
				var type = m_Model.GetDeclaredSymbol(m) as ITypeSymbol;
				BlockSyntax topLevelChunk = null;
				if (null != type && type.IsTopLevelChunkClass())
				{
					var classDecl = (ClassDeclarationSyntax)m;
					foreach (var mem in classDecl.Members)
					{
						var method = mem as MethodDeclarationSyntax;
						if (null != method)
						{
							topLevelChunk = method.Body;
							break;
						}
					}
				}

				if (null != topLevelChunk)
				{
					bPlaceholder = true;
					using (var scope = new Scope(this, ScopeKind.TopLevelChunk, topLevelChunk))
					{
						Visit(topLevelChunk);
					}
				}
				else
				{
					Visit(m);
				}
			}

			// If this was a genuine class emit, return the last class that was emitted,
			// if there is one.
			if (!bPlaceholder)
			{
				EmitReturnForLastClass(node.Members);
			}

			WriteNewlineToTarget(node.GetEndLine()); // Necessary to write any trailing comments.

			return node;
		}

		bool EmitReturnForLastClass(SyntaxList<MemberDeclarationSyntax> members)
		{
			for (int i = members.Count - 1; i >= 0; --i)
			{
				var m = members[i];
				if (m is ClassDeclarationSyntax classDecl)
				{
					// Get the id.
					var sId = m_Model.LookupOutputId(classDecl);

					// Emite the return.
					SeparateForLast(classDecl);
					Write(kLuaReturn);
					Write(' ');
					Write(sId);
					return true;
				}
				else if (m is NamespaceDeclarationSyntax namespaceDecl)
				{
					if (EmitReturnForLastClass(namespaceDecl.Members))
					{
						return true;
					}
				}
			}

			return false;
		}

		public override SyntaxNode VisitIncompleteMember(IncompleteMemberSyntax node)
		{
			throw new SlimCSCompilationException(node, "incomplete member, syntax error.");
		}
	}
}
