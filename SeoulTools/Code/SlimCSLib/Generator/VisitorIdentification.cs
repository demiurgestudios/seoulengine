// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using System;
using System.Collections.Immutable;
using static SlimCSLib.Generator.Constants;

namespace SlimCSLib.Generator
{
	public sealed partial class Visitor : CSharpSyntaxVisitor<SyntaxNode>, IDisposable
	{
		#region Private members
		MemberAccessEmitType GetMemberAccessEmitType(ExpressionSyntax node, bool bTreatAsInvoke = false)
		{
			var invoke = (node.Parent as InvocationExpressionSyntax);
			var eKind = node.Kind();
			ISymbol sym = null;
			ISymbol typeA = null;
			ExpressionSyntax a = null;
			bool bBase = false;
			SimpleNameSyntax name = null;
			switch (eKind)
			{
				// Special case for a few system builtins that should be converted
				// into extension method invocation styles.
				case SyntaxKind.IdentifierName:
					if (invoke != null)
					{
						var methodSym = m_Model.GetSymbolInfo(invoke).Symbol as IMethodSymbol;
						if (methodSym?.IsGetType() == true)
						{
							return MemberAccessEmitType.Extension;
						}
					}

					// All other case, just a normal lookup.
					return MemberAccessEmitType.Normal;

				// Gather data from member bindings or simple member access
				// to determine specification kind.
				case SyntaxKind.MemberBindingExpression:
					{
						var member = (node as MemberBindingExpressionSyntax);
						name = member.Name;
						sym = m_Model.GetSymbolInfo(name).Symbol;
						a = m_Model.ReduceNops(m_BindingTargets.Back());
						typeA = m_Model.GetTypeInfo(m_BindingTargets.Back()).Type; // Intentionally not reduced to maintain correct typing.
						bBase = (a is BaseExpressionSyntax);
					}
					break;
				case SyntaxKind.SimpleMemberAccessExpression:
					{
						var member = (node as MemberAccessExpressionSyntax);
						name = member.Name;
						sym = m_Model.GetSymbolInfo(name).Symbol;
						a = m_Model.ReduceNops(member.Expression);
						typeA = m_Model.GetTypeInfo(member.Expression).Type; // Intentionally not reduced to maintain correct typing.
						bBase = (a is BaseExpressionSyntax);
					}
					break;

				// Other root expression, we can only identify
				// the invocation as normal, no special modifications.
				default:
					return MemberAccessEmitType.Normal;
			}

			// Several checks that require valid symbol info against the lookup.
			// Early out if we have no symbol info.
			if (null != sym)
			{
				// Check for a few guaranteed cases.
				if (sym is IMethodSymbol)
				{
					var methodSym = (IMethodSymbol)sym;
					if (methodSym.IsExtensionMethod)
					{
						return MemberAccessEmitType.Extension;
					}

					// Methods that have been converted to
					// top-level locals are also treated as extension methods.
					if (CanEmitAsLocal(methodSym) && !methodSym.IsStatic)
					{
						return MemberAccessEmitType.Extension;
					}

					// Some additional conversions.
					// Conversion of a few builtins to global function calls.
					if (methodSym.IsGetType() ||
						methodSym.IsCsharpToString())
					{
						return MemberAccessEmitType.Extension;
					}
				}

				// Instance member - only converted to a send
				// if we're in an invocation context.
				//
				// See Deviations section of the SlimCS documentation.
				if ((bTreatAsInvoke || invoke != null) &&
					(sym.Kind == SymbolKind.Method) &&
					!sym.IsStatic)
				{
					// Check for base access.
					if (bBase)
					{
						return MemberAccessEmitType.BaseMethod;
					}
					else
					{
						return MemberAccessEmitType.Send;
					}
				}

				// Property or event - get or set determined by
				// whether we can find an invocation of an appropriate
				// method in our parent context.
				if (sym.Kind == SymbolKind.Event)
				{
					var evt = (IEventSymbol)sym;
					if (LHS != null && LHS.Equals(evt.ResolveAddMethod())) { return (bBase ? MemberAccessEmitType.BaseEventAdd : MemberAccessEmitType.EventAdd); }
					else if (LHS != null && LHS.Equals(evt.ResolveRemoveMethod())) { return (bBase ? MemberAccessEmitType.BaseEventRemove : MemberAccessEmitType.EventRemove); }
					else if (node.Parent is InvocationExpressionSyntax) { return (bBase ? MemberAccessEmitType.BaseEventRaise : MemberAccessEmitType.EventRaise); }
				}
				else if (sym.Kind == SymbolKind.Property)
				{
					var prop = (IPropertySymbol)sym;
					if (!prop.IsAutoProperty())
					{
						if (LHS != null && LHS.Equals(prop.ResolveSetMethod()))
						{
							return (bBase ? MemberAccessEmitType.BaseSetter : MemberAccessEmitType.Setter);
						}
						else
						{
							return (bBase ? MemberAccessEmitType.BaseGetter : MemberAccessEmitType.Getter);
						}
					}
				}
			}

			// Finally - for all other cases, check if
			// the parent is an invocation expression and
			// the first argument to the invocation
			// is the same as the target of the member expression.
			if (invoke != null)
			{
				var args = invoke.ArgumentList.Arguments;
				if (args.Count > 0 && a.IsEquivalentTo(m_Model.ReduceNops(args[0].Expression), true))
				{
					return MemberAccessEmitType.SendConverted;
				}
			}

			// Final fallback, normal or possibly a base access.
			if (bBase)
			{
				return MemberAccessEmitType.BaseAccess;
			}
			else
			{
				return MemberAccessEmitType.Normal;
			}
		}

		bool ImplicitLookup(SyntaxNode node, ExpressionSyntax bindingTarget)
		{
			// Check the parent of the expression. If
			// the symbol is not a root type (at global scope),
			// then it is implicit if is the expression
			// part of the member expression.
			var member = (node.Parent as MemberAccessExpressionSyntax);

			// Must be an implicit lookup for non-root types that
			// also are contained within a member expression.
			if (null == member)
			{
				// Check for our place in binding expression.
				var binding = (node.Parent as MemberBindingExpressionSyntax);
				if (null == binding) { return true; }
				else if (bindingTarget == node) { return true; }
				else { return false; }
			}
			// Implicit if we're the expression part (first element), otherwise
			// it's an explicit lookup.
			else if (member.Expression == node)
			{
				return true;
			}
			else
			{
				return false;
			}
		}

		/// <summary>
		/// Returns true if the current lookup context of the identifier
		/// needs to be resolved for Lua.
		/// </summary>
		/// <param name="node">Node to check.</param>
		/// <param name="sym">Symbol information for the node.</param>
		/// <returns>true if additional resolve is needed, false otherwise.</returns>
		/// <remarks>
		/// Cases where this method return true include:
		/// - references to extension methods.
		/// - implicit member lookups (e.g. a = 2 instead of this.a = 2)
		/// </remarks>
		bool NeedsLookupFixup(SimpleNameSyntax node, ISymbol sym, ExpressionSyntax bindingTarget)
		{
			// Quick early out - always return true for extension method
			// lookups.
			if (sym is IMethodSymbol && ((IMethodSymbol)sym).IsExtensionMethod)
			{
				return true;
			}

			// False if the symbol does not have a containing type.
			if (null == sym.ContainingType)
			{
				return false;
			}

			// Never implicit for certain symbol types.
			switch (sym.Kind)
			{
				// Named types are always referenced by their deduped global names.
				case SymbolKind.NamedType:
					return false;

					// True if not a method generic parameter.
				case SymbolKind.TypeParameter:
					{
						if (sym.ContainingSymbol.Kind != SymbolKind.Method)
						{
							// Must be lookup inside a method context with implicit this.
							return m_Model.HashInstanceThisContext(node);
						}
					}
					return false;

				case SymbolKind.Event:
				case SymbolKind.Field:
				case SymbolKind.Property:
					break;

				case SymbolKind.Method:
					// Need to identify local methods in this case.
					{
						var methodSym = (IMethodSymbol)sym;
						switch (methodSym.MethodKind)
						{
							case MethodKind.DelegateInvoke:
							case MethodKind.LocalFunction:
								return false;

							case MethodKind.Constructor:
							case MethodKind.Destructor:
							case MethodKind.EventAdd:
							case MethodKind.EventRaise:
							case MethodKind.EventRemove:
							case MethodKind.Ordinary:
							case MethodKind.PropertyGet:
							case MethodKind.PropertySet:
								break;

							case MethodKind.ReducedExtension:
								break;

							default:
								throw new UnsupportedNodeException(node, $"implicit lookup not implemented for method kind '{methodSym.MethodKind}'");
						}
					}
					break;

				case SymbolKind.Label:
				case SymbolKind.Local:
				case SymbolKind.Parameter:
					return false;

				default:
					throw new UnsupportedNodeException(node, $"implicit lookup not implemented for symbol of kind '{sym.Kind}'");
			}

			// If the symbol is a member of the SlimCS system library
			// then it's fine for it to be implicit.
			if (sym.ContainingType.IsSlimCSSystemLib())
			{
				return false;
			}

			// Also fine (and required) that lookups to
			// private static/const members of the containing
			// type are "implicit" (since they were emitted
			// as top-level local variables).
			if (CanEmitAsLocal(sym))
			{
				return false;
			}

			// Final check, are we implicit or not.
			return ImplicitLookup(node, bindingTarget);
		}

		void VisitBracketedArgumentList(
			ExpressionSyntax expression,
			BracketedArgumentListSyntax node)
		{
			// Evaluate for an indexer and use that for the emit if available.
			var sym = m_Model.GetSymbolInfo(expression.Parent).Symbol;
			var indexer = (sym as IPropertySymbol);
			(var b0Based, var bNeedsArrayPlaceholder) = m_Model.GetArrayFeatures(expression);
			var eLuaKind = (null != indexer ? indexer.ContainingType.GetLuaKind() : LuaKind.NotSpecial);

			// We wrangle Lua table semantics by using 'false' as a placeholder
			// in arrays that contain reference types (TODO: To fully
			// close this loop, we must disallow arrays of object[],
			// we must require arrays of value types or interfaces/concrete class
			// types).
			//
			// This requires that reads becomes (a[i] or nil) and writes become
			// a[i] = (v or false) for arrays of reference types. We also
			// must fully initialize an array prior to use.
			var bWrappedRead = (bNeedsArrayPlaceholder && !expression.Parent.IsLHS());
			if (bWrappedRead) { Write('('); }

			var bBase = (expression is BaseExpressionSyntax);
			if (!bBase)
			{
				// Special handling, need to pop the binding target for the scope of this call.
				if (BindingTarget == expression)
				{
					var topTarget = m_BindingTargets.Back();
					m_BindingTargets.PopBack();
					Visit(topTarget);
					m_BindingTargets.Add(topTarget);
				}
				else
				{
					Visit(expression);
				}
			}
			else
			{
				Write(m_Model.LookupOutputId(sym.ContainingType));
			}

			// Access is actually an indexer unless it's one of the
			// special Lua kinds, these have particular transformations
			// (e.g. Table is just a Lua table so we leave the element
			// access).
			if (null != indexer && eLuaKind == LuaKind.NotSpecial)
			{
				// If LHS, we're on the left side of an assignment. Otherwise, we're a getter.
				var bLHS = (LHS != null && LHS.Equals(indexer.ResolveSetMethod()));
				var method = (bLHS ? indexer.ResolveSetMethod() : indexer.ResolveGetMethod());
				var prms = (null != method ? method.Parameters : ImmutableArray<IParameterSymbol>.Empty);

				// Remove the last argument when resolving the arguments, since we leave
				// a trailing ',' and resolve it later.
				if (bLHS && !prms.IsDefaultOrEmpty) { prms = prms.RemoveAt(prms.Length - 1); }
				var args = ResolveArguments(node, prms, node.Arguments);
				var sId = m_Model.LookupOutputId(method);

				// Invocation - send, unless a base expression.
				Write(bBase ? '.' : ':');
				Write(sId);
				Write('(');

				// Add self if a base expression.
				if (bBase) { Write(kLuaSelf); }

				for (int i = 0; i < args.Count; ++i)
				{
					if (m_LastChar != '(') { Write(','); }
					Visit(args[i].Expression);
				}

				// If on the left hand side, we leave the call unterminated.
				if (bLHS) { Write(','); }
				else { Write(')'); }
			}
			else
			{
				foreach (var arg in node.Arguments)
				{
					// Emit with .identifier syntax if possible.
					if ((!b0Based || eLuaKind == LuaKind.Table || eLuaKind == LuaKind.Dval) &&
						arg.Expression is LiteralExpressionSyntax lit &&
						lit.Kind() == SyntaxKind.StringLiteralExpression &&
						lit.Token.ValueText.IsValidLuaIdent())
					{
						Write('.');
						Write(lit.Token.ValueText);
					}
					else
					{
						Write('[');

						// TODO: We want to add an optimization that eliminates the need
						// for this if the 0-based can be factored out at (for example) the loop
						// level.

						// If this is a 0-based lookup, we need to add +1.
						if (b0Based) { Write('('); }

						Visit(arg.Expression);

						// If this is a 0-based lookup, we need to add +1.
						if (b0Based) { Write(")+1"); }

						Write(']');
					}
				}
			}

			// Terminate the read wrapping.
			if (bWrappedRead)
			{
				Write(" or ");
				WriteArrayReadPlaceholder(expression);
				Write(')');
			}
		}

		void VisitMemberAccessExpression(MemberAccessExpressionSyntax node, MemberAccessEmitType eType)
		{
			m_Model.Enforce(node);

			// Check operator kind - we don't support pointer dereference.
			var eOpKind = node.OperatorToken.Kind();
			switch (eOpKind)
			{
				case SyntaxKind.DotToken:
					// Handling for array replacement.
					{
						var sym = m_Model.GetSymbolInfo(node.Name).Symbol;
						if (null != sym && sym.IsArrayLength())
						{
							Write('#');
							Visit(node.Expression);
							return;
						}
					}

					// Resolve a few special cases:
					// - top level types always just resolve to their deduped name.
					// - handle for some remaps.
					// - some symbols resolve to a top-level local variable in
					//   Lua (private, static or const, in classes that are not partial
					//   and do not have any nested classes which are pratial).
					{
						var sym = m_Model.GetSymbolInfo(node).Symbol;

						// Named type handling - top levels always resolve to just
						// their deduped name.
						var namedTypeSym = sym as INamedTypeSymbol;
						if (null != namedTypeSym)
						{
							Write(m_Model.LookupOutputId(namedTypeSym));
							return;
						}

						if (sym is IMethodSymbol methodSym)
						{
							// TODO: Need to resolve any remaps for this
							// new lookup.

							if (methodSym.IsStringNullOrEmpty())
							{
								Write(kBuiltinStringImplementations);
								Write('.');
								Write(kStringIsNullOrEmpty);
								return;
							}
						}

						// Resolution handling.
						if (CanEmitAsLocal(sym))
						{
							Write(m_Model.LookupOutputId(sym));
							return;
						}
					}

					// Custom handling for extension invocations.
					if (eType == MemberAccessEmitType.Extension)
					{
						Visit(node.Name);
						return;
					}

					// Default handling.
					if (MemberAccessEmitType.EventAdd == eType ||
						MemberAccessEmitType.EventRaise == eType ||
						MemberAccessEmitType.EventRemove == eType ||
						MemberAccessEmitType.Getter == eType ||
						MemberAccessEmitType.Send == eType ||
						MemberAccessEmitType.SendConverted == eType ||
						MemberAccessEmitType.Setter == eType)
					{
						var sym = m_Model.GetSymbolInfo(node.Name).Symbol;
						Visit(node.Expression);
						Write(sym?.IsStatic == true ? '.' : ':');
					}
					else
					{
						var sym = m_Model.GetSymbolInfo(node.Expression).Symbol;

						// Base handling.
						if (sym != null && eType.IsBase())
						{
							// Base access becomes self.
							if (eType == MemberAccessEmitType.BaseAccess)
							{
								Write(kLuaSelf);
							}
							// Otherwise, we become BaseClass. and depend on the
							// context to resolve the self as an argument.
							else
							{
								var idSym = m_Model.GetSymbolInfo(node.Name).Symbol;
								Write(m_Model.LookupOutputId(idSym.ContainingType));
							}
							Write(node.OperatorToken);
						}
						// Skip the expression if it's the SlimCS system library static class.
						else if (sym == null || !sym.IsSlimCSSystemLib())
						{
							Visit(node.Expression);
							Write(node.OperatorToken);
						}
					}

					// Write setter or getter prefix if of the appropriate type.
					if (MemberAccessEmitType.BaseEventAdd == eType ||
						MemberAccessEmitType.EventAdd == eType)
					{
						var eventSym = m_Model.GetSymbolInfo(node.Name).Symbol as IEventSymbol;
						var addMethod = eventSym.ResolveAddMethod();
						Write(m_Model.LookupOutputId(addMethod));

						// If we're the left hand side of an assignment,
						// we add an open paren (unterminated setter)
						// to allow the assignment to add the value
						// to add.
						if (LHS != null && LHS.Equals(addMethod)) { Write('('); }
						if (MemberAccessEmitType.BaseEventAdd == eType) { Write(kLuaSelf); Write(','); }
					}
					else if (
						MemberAccessEmitType.BaseEventRaise == eType ||
						MemberAccessEmitType.EventRaise == eType)
					{
						var eventSym = m_Model.GetSymbolInfo(node.Name).Symbol as IEventSymbol;
						Write(m_Model.LookupOutputId(eventSym.ResolveRaiseMethod()));

						// TODO:
						if (MemberAccessEmitType.BaseEventRaise == eType)
						{
							throw new UnsupportedNodeException(node, "event raise via base is not yet supported.");
						}
					}
					else if (
						MemberAccessEmitType.BaseEventRemove == eType ||
						MemberAccessEmitType.EventRemove == eType)
					{
						var eventSym = m_Model.GetSymbolInfo(node.Name).Symbol as IEventSymbol;
						Write(m_Model.LookupOutputId(eventSym.ResolveRemoveMethod()));

						// If we're the left hand side of an assignment,
						// we add an open paren (unterminated setter)
						// to allow the assignment to add the value
						// to remove.
						if (LHS != null && LHS.Equals(eventSym.ResolveRemoveMethod())) { Write('('); }
						if (MemberAccessEmitType.BaseEventRemove == eType) { Write(kLuaSelf); Write(','); }
					}
					else if (
						MemberAccessEmitType.BaseGetter == eType ||
						MemberAccessEmitType.Getter == eType)
					{
						var propertySym = m_Model.GetSymbolInfo(node.Name).Symbol as IPropertySymbol;
						Write(m_Model.LookupOutputId(propertySym.ResolveGetMethod()));

						// Getters, also add the parens to convert to an invoke.
						if (MemberAccessEmitType.BaseGetter == eType) { Write('('); Write(kLuaSelf); Write(')'); }
						else { Write("()"); }
					}
					else if (
						MemberAccessEmitType.BaseSetter == eType ||
						MemberAccessEmitType.Setter == eType)
					{
						var propertySym = m_Model.GetSymbolInfo(node.Name).Symbol as IPropertySymbol;
						Write(m_Model.LookupOutputId(propertySym.ResolveSetMethod()));

						// If we're the left hand side of an assignment,
						// we add an open paren (unterminated setter)
						// to allow the assignment to add the value
						// to set.
						if (LHS != null && LHS.Equals(propertySym.ResolveSetMethod())) { Write('('); }
						if (MemberAccessEmitType.BaseSetter == eType) { Write(kLuaSelf); Write(','); }
					}
					else
					{
						Visit(node.Name);
					}
					break;
				default:
					throw new UnsupportedNodeException(node, $"unsupported member access operator '{eOpKind}'.");
			}
		}

		void VisitMemberBindingExpression(MemberBindingExpressionSyntax node, MemberAccessEmitType eType)
		{
			// Sanity check that a bind target was set.
			if (m_BindingTargets.Count == 0)
			{
				throw new SlimCSCompilationException(node, "encountered member binding expression with null target, internal compiler error.");
			}

			// Custom handling for extension invocations.
			if (eType == MemberAccessEmitType.Extension)
			{
				Visit(node.Name);
				return;
			}

			// Default handling - when about to visit
			// a binding target, we need to temporarily remove
			// it, as nested binding expression (e.g. a?.b.c?.d)
			// may exist, in which case we want the
			// parent target, not the current.
			var topTarget = m_BindingTargets.Back();
			m_BindingTargets.PopBack();
			Visit(topTarget);
			m_BindingTargets.Add(topTarget);

			// Default handling.
			if (MemberAccessEmitType.EventAdd == eType ||
				MemberAccessEmitType.EventRaise == eType ||
				MemberAccessEmitType.EventRemove == eType ||
				MemberAccessEmitType.Getter == eType ||
				MemberAccessEmitType.Send == eType ||
				MemberAccessEmitType.SendConverted == eType ||
				MemberAccessEmitType.Setter == eType)
			{
				Write(':');
			}
			else
			{
				Write(node.OperatorToken);
			}
			Visit(node.Name);
		}

		void WriteIdWithDedup(SimpleNameSyntax node, bool bInvoke = false)
		{
			var sym = m_Model.GetSymbolInfo(node).Symbol;
			if (null == sym)
			{
				Write(node.Identifier);
				return;
			}

			// TODO: Too much bolierplate around this, need to unify.
			bool bLHS = false;
			bool bParen = false;
			switch (sym.Kind)
			{
				case SymbolKind.Event:
					{
						var evt = (IEventSymbol)sym;
						bParen = true;
						if (LHS != null && LHS.Equals(evt.ResolveAddMethod())) { sym = evt.ResolveAddMethod(); bLHS = true; }
						else if (LHS != null && LHS.Equals(evt.ResolveRemoveMethod())) { sym = evt.ResolveRemoveMethod(); bLHS = true; }
						else if (node.Parent is InvocationExpressionSyntax && null != evt.ResolveRaiseMethod()) { sym = evt.ResolveRaiseMethod(); }
						else { bParen = false; }
					}
					break;
				case SymbolKind.Property:
					{
						var prop = (IPropertySymbol)sym;
						if (!prop.IsAutoProperty())
						{
							bParen = true;
							if (LHS != null && LHS.Equals(prop.ResolveSetMethod())) { sym = prop.ResolveSetMethod(); bLHS = true; }
							else if (null != prop.ResolveGetMethod()) { sym = prop.ResolveGetMethod(); }
							else { bParen = false; }
						}
					}
					break;
			}
			var methodSym = (sym as IMethodSymbol);

			// Prefix this (self) or type scope, if needed.
			if (NeedsLookupFixup(node, sym, BindingTarget))
			{
				// Type scope if sym is static/const/extension or is a type reference.
				if (sym.IsStatic ||
					(null != methodSym && methodSym.IsExtensionMethod) ||
					sym is INamedTypeSymbol ||
					m_Model.GetConstantValue(node).HasValue)
				{
					Write(m_Model.LookupOutputId(sym.ContainingType));
					Write(((bParen && !sym.IsStatic) || node.IsNonStaticMethodCall(sym, bInvoke)) ? ':' : '.');
				}
				// Self is sym is not static/const.
				else
				{
					Write(kLuaSelf);
					Write(((bParen && !sym.IsStatic) || node.IsNonStaticMethodCall(sym, bInvoke)) ? ':' : '.');
				}
			}

			// Handle deduping of various cases.
			string sId = string.Empty;
			if (m_BlockScope.Dedup(sym, ref sId))
			{
				Write(sId);
			}
			else if (IsVararg(node) || (sym.Kind == SymbolKind.Parameter && ((IParameterSymbol)sym).IsParams))
			{
				// TODO: Need to push this higher up the stack
				// so it can be optimized out.
				var parent = node.Parent;
				if (parent is InitializerExpressionSyntax &&
					((InitializerExpressionSyntax)parent).Expressions.Count == 1)
				{
					Write("...");
				}
				else if (parent.Parent is ArgumentListSyntax argList &&
					argList.Arguments.Count > 0 &&
					argList.Arguments.Last().Expression == node)
				{
					var arg = argList.Arguments.Last();
					var invokeSymbol = (IMethodSymbol)m_Model.GetSymbolInfo(argList.Parent).Symbol;
					var prm = invokeSymbol.GetParameter(arg);
					if (prm.IsParams)
					{
						Write("...");
					}
					else
					{
						Write("{...}");
					}
				}
				else if (parent is EqualsValueClauseSyntax)
				{
					Write("{...}");
				}
				else
				{
					Write("({...})");
				}
			}
			else
			{
				Write(m_Model.LookupOutputId(sym));
			}

			// If paren is specified, need to add at least one.
			if (bParen)
			{
				Write('(');
				// If LHS is specified, we leave an unterminated opening paren.
				// Otherwise, we terminate the getter.
				if (!bLHS) { Write(')'); }
			}
		}

		void WriteIdWithDedup(LabeledStatementSyntax node)
		{
			var sym = m_Model.GetDeclaredSymbol(node);
			if (null == sym)
			{
				Write(node.Identifier);
				return;
			}

			string sId = string.Empty;
			if (m_BlockScope.Dedup(sym, ref sId))
			{
				Write(sId);
				return;
			}
			else
			{
				Write(m_Model.LookupOutputId(sym));
				return;
			}
		}

		void WriteIdWithDedup(PropertyDeclarationSyntax node)
		{
			var sym = m_Model.GetDeclaredSymbol(node);
			var sId = m_Model.LookupOutputId(sym);
			Write(sId);
		}

		void WriteIdWithDedup(VariableDeclaratorSyntax node)
		{
			var sym = m_Model.GetDeclaredSymbol(node);
			if (null == sym)
			{
				Write(node.Identifier);
				return;
			}

			string sId = string.Empty;
			if (m_BlockScope.Dedup(sym, ref sId))
			{
				Write(sId);
				return;
			}
			else
			{
				Write(m_Model.LookupOutputId(sym));
				return;
			}
		}
		#endregion

		public override SyntaxNode VisitArrayType(ArrayTypeSyntax node)
		{
			// TODO: Need to constrain rank.
			// TODO: This is incorrect in most cases.
			Visit(node.ElementType);
			return node;
		}

		public override SyntaxNode VisitElementAccessExpression(ElementAccessExpressionSyntax node)
		{
			VisitBracketedArgumentList(node.Expression, node.ArgumentList);
			return node;
		}

		public override SyntaxNode VisitElementBindingExpression(ElementBindingExpressionSyntax node)
		{
			VisitBracketedArgumentList(BindingTarget, node.ArgumentList);
			return node;
		}

		public override SyntaxNode VisitGenericName(GenericNameSyntax node)
		{
			// Specialized generic names are converted into dynamic lookups.
			if (!node.IsUnboundGenericName)
			{
				var sym = m_Model.GetSymbolInfo(node).Symbol;
				if (sym != null && sym.Kind != SymbolKind.Method)
				{
					var sTypeId = m_Model.LookupOutputId(sym);
					var sQualifiedId = GetFullTypeString(sym, '+');

					// We generate a lookup name - started with <> to guarantee dedup.
					var stringBuilder = new System.Text.StringBuilder();
					stringBuilder.Append(sQualifiedId);
					stringBuilder.Append('`');
					stringBuilder.Append(node.TypeArgumentList.Arguments.Count.ToString());
					stringBuilder.Append('[');
					for (int i = 0; i < node.TypeArgumentList.Arguments.Count; ++i)
					{
						if (0 != i)
						{
							stringBuilder.Append(',');
						}

						var typeSyntax = node.TypeArgumentList.Arguments[i];
						var type = m_Model.GetTypeInfo(typeSyntax).ConvertedType;
						if (type.ContainingNamespace != null && !type.ContainingNamespace.IsGlobalNamespace)
						{
							stringBuilder.Append(type.ContainingNamespace.FullNamespace());
							stringBuilder.Append('.');
						}
						if (type.OriginalDefinition.SpecialType == SpecialType.System_Nullable_T)
						{
							stringBuilder.Append("Nullable`1[");
							{
								var argType = ((INamedTypeSymbol)type).TypeArguments[0];
								if (argType.ContainingNamespace != null && !argType.ContainingNamespace.IsGlobalNamespace)
								{
									stringBuilder.Append(argType.ContainingNamespace.FullNamespace());
									stringBuilder.Append('.');
								}
								stringBuilder.Append(m_Model.LookupOutputId(argType));
							}
							stringBuilder.Append(']');
						}
						else
						{
							stringBuilder.Append(m_Model.LookupOutputId(type));
						}
					}
					stringBuilder.Append(']');
					Write(kLuaGenericTypeLookup);
					Write('(');
					WriteStringLiteral(sTypeId);
					Write(", ");
					WriteStringLiteral(stringBuilder.ToString());

					var prms = (sym.Kind == SymbolKind.Method
						? ((IMethodSymbol)sym).TypeParameters
						: ((INamedTypeSymbol)sym).TypeParameters);
					for (int i = 0; i < node.TypeArgumentList.Arguments.Count; ++i)
					{
						var typePrm = prms[i];
						var typeArg = node.TypeArgumentList.Arguments[i];

						Write(", ");
						WriteStringLiteral(m_Model.LookupOutputId(typePrm));
						Write(", ");
						Visit(typeArg);
					}

					Write(')');

					return node;
				}
			}

			WriteIdWithDedup(node);
			return node;
		}

		void VisitIdentifierNameAsInvoke(IdentifierNameSyntax node)
		{
			WriteIdWithDedup(node, true);
		}

		public override SyntaxNode VisitIdentifierName(IdentifierNameSyntax node)
		{
			var (to, b) = NeedsDelegateBind(node);
			if (b)
			{
				BindDelegate(to, node);
			}
			else
			{
				WriteIdWithDedup(node);
			}

			return node;
		}

		public override SyntaxNode VisitImplicitElementAccess(ImplicitElementAccessSyntax node)
		{
			// We can just emit this as an identifier.
			if (node.CanConvertToIdent())
			{
				var literal = (LiteralExpressionSyntax)node.ArgumentList.Arguments[0].Expression;
				Write(literal.Token.ValueText);
			}
			else
			{
				foreach (var arg in node.ArgumentList.Arguments)
				{
					Write('[');
					Visit(arg.Expression);
					Write(']');
				}
			}

			return node;
		}

		public override SyntaxNode VisitMemberAccessExpression(MemberAccessExpressionSyntax node)
		{
			// Must suppress constant emit once we're within a member access expression,
			// since (e.g.) s.Length cannot be convert to 'Foo'.Length in Lua, even if
			// s is known to be 'Foo' at compile time.
			++m_iConstantEmitSuppressed;
			try
			{
				// Special handling for delegate references/binding.
				var (to, b) = NeedsDelegateBind(node);
				if (b)
				{
					BindDelegate(to, node);
				}
				else
				{
					VisitMemberAccessExpression(node, GetMemberAccessEmitType(node));
				}
				return node;
			}
			finally
			{
				--m_iConstantEmitSuppressed;
			}
		}

		public override SyntaxNode VisitMemberBindingExpression(MemberBindingExpressionSyntax node)
		{
			VisitMemberBindingExpression(node, GetMemberAccessEmitType(node));
			return node;
		}

		public override SyntaxNode VisitNullableType(NullableTypeSyntax node)
		{
			var sym = m_Model.GetTypeInfo(node).Type;
			Write(m_Model.LookupOutputId(sym));
			return node;
		}

		public override SyntaxNode VisitParameter(ParameterSyntax node)
		{
			var sym = m_Model.GetDeclaredSymbol(node);
			if (null == sym || !IsUsed(node.Identifier.ValueText))
			{
				Write(node.Identifier, true);
				return node;
			}

			// Handle deduping of various cases.
			string sId = string.Empty;
			if (m_BlockScope.Dedup(sym, ref sId))
			{
				Write(sId);
			}
			else
			{
				Write(node.Identifier, true);
			}

			return node;
		}

		public override SyntaxNode VisitPredefinedType(PredefinedTypeSyntax node)
		{
			m_Model.Enforce(node);

			Write(node.Keyword);
			return node;
		}

		public override SyntaxNode VisitQualifiedName(QualifiedNameSyntax node)
		{
			// Check if this is a top-level type that we will access through
			// its deduped name.
			var sym = m_Model.GetSymbolInfo(node).Symbol as INamedTypeSymbol;
			if (null != sym)
			{
				Write(m_Model.LookupOutputId(sym));
			}
			else
			{
				Visit(node.Left);
				Write('.');
				Visit(node.Right);
			}

			return node;
		}

		public override SyntaxNode VisitTypeParameter(TypeParameterSyntax node)
		{
			Write(node.Identifier);
			return node;
		}
	}
}
