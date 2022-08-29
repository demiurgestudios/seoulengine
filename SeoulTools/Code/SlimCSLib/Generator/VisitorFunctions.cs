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
		public enum CanConvertToFuncResult
		{
			CannotConvert,
			ConvertToCall,
			ConvertToSend,
		}

		#region Private members
		CanConvertToFuncResult CanConvertToFunctionDeclaration(AssignmentExpressionSyntax node)
		{
			// For style, we only collapse assignments to the form function a.b()
			// if we're in a top-level context.
			if (m_BlockScope.Count > 0)
			{
				if (m_BlockScope.Back().m_eKind != ScopeKind.TopLevelChunk)
				{
					return CanConvertToFuncResult.CannotConvert;
				}
				else if (!(node.Parent is ExpressionStatementSyntax))
				{
					return CanConvertToFuncResult.CannotConvert;
				}
			}
			else if (!(node.Parent is BaseTypeDeclarationSyntax))
			{
				return CanConvertToFuncResult.CannotConvert;
			}

			var member = (node.Left as MemberAccessExpressionSyntax);
			if (null == member)
			{
				return CanConvertToFuncResult.CannotConvert;
			}

			// All parts of the member access expression must be
			// just identifiers to convert to the function declaration form.
			if (!member.IsDeclarationSafeMemberAccess())
			{
				return CanConvertToFuncResult.CannotConvert;
			}

			var lambda = node.Right.TryResolveLambda();
			if (null == lambda)
			{
				return CanConvertToFuncResult.CannotConvert;
			}

			var prms = lambda.ParameterList.Parameters;
			if (prms.Count < 1 || prms[0].Identifier.ValueText != "self")
			{
				return CanConvertToFuncResult.ConvertToCall;
			}

			return CanConvertToFuncResult.ConvertToSend;
		}

		public bool CanConvertToFunctionDeclaration(LocalDeclarationStatementSyntax node)
		{
			// Note that, unlike assignments, we must be aggressive with this
			// collapse behavior for functions. local function a() vs. local a = function()
			// has one semantic difference - the first form is visible to the body of
			// the function definition, the second is not. As a result, recursion is
			// supported by the first form but not by the second.

			var vars = node.Declaration.Variables;
			if (vars.Count != 1)
			{
				return false;
			}

			if (null == vars[0].Initializer)
			{
				return false;
			}

			var lambda = vars[0].Initializer.Value.TryResolveLambda();
			if (null == lambda)
			{
				return false;
			}

			return true;
		}

		bool HasVarargs(AnonymousMethodExpressionSyntax node)
		{
			// Must have at least one argument.
			if (node.ParameterList == null ||
				node.ParameterList.Parameters.Count < 1)
			{
				return false;
			}

			// TODO: May not always be correct, but also the only
			// way I'm aware to determine this. The lambda itself doesn't know that
			// its last argument is a params entry (vs. just an array). We
			// have to determine this from the delegate it is cast to.
			var cast = node.ResolveEnclosingCastExpression();
			if (null == cast)
			{
				return false;
			}

			// TODO: It's sad we need a semantic model for this.
			// Building that model takes a really long time (compile
			// times increase by about 3x due only to resolving the model).
			var typeInfo = m_Model.GetTypeInfo(cast.Type);

			// Compiler error.
			if (typeInfo.Type.Kind == SymbolKind.ErrorType)
			{
				throw new SlimCSCompilationException(cast.Type, $"The type or namespace name '{typeInfo.Type}' could not be found(are you missing a using directive or an assembly reference?)");
			}

			var typeSym = typeInfo.ConvertedType as INamedTypeSymbol;
			if (null == typeSym)
			{
				return false;
			}

			var methodInvoke = typeSym.DelegateInvokeMethod;
			if (null == methodInvoke)
			{
				return false;
			}

			var prms = methodInvoke.Parameters;
			return prms[prms.Length - 1].IsParams;
		}

		bool HasVarargs(ParameterListSyntax prms)
		{
			// Must have at least one argument.
			if (prms == null || prms.Parameters.Count < 1)
			{
				return false;
			}

			var lastParam = prms.Parameters[prms.Parameters.Count - 1];
			var bVarargs = lastParam.Modifiers.HasToken(SyntaxKind.ParamsKeyword);
			return bVarargs;
		}

		bool HasVarargs(ParenthesizedLambdaExpressionSyntax node)
		{
			// Must have at least one argument.
			if (node.ParameterList == null ||
				node.ParameterList.Parameters.Count < 1)
			{
				return false;
			}

			// TODO: May not always be correct, but also the only
			// way I'm aware to determine this. The lambda itself doesn't know that
			// its last argument is a params entry (vs. just an array). We
			// have to determine this from the delegate it is cast to.
			var cast = node.ResolveEnclosingCastExpression();
			if (null == cast)
			{
				return false;
			}

			// TODO: It's sad we need a semantic model for this.
			// Building that model takes a really long time (compile
			// times increase by about 3x due only to resolving the model).
			var typeInfo = m_Model.GetTypeInfo(cast.Type);

			// Compiler error.
			if (typeInfo.Type.Kind == SymbolKind.ErrorType)
			{
				throw new SlimCSCompilationException(cast.Type, $"The type or namespace name '{typeInfo.Type}' could not be found(are you missing a using directive or an assembly reference?)");
			}

			var typeSym = typeInfo.ConvertedType as INamedTypeSymbol;
			if (null == typeSym)
			{
				return false;
			}

			var methodInvoke = typeSym.DelegateInvokeMethod;
			if (null == methodInvoke)
			{
				return false;
			}

			var prms = methodInvoke.Parameters;
			return prms[prms.Length - 1].IsParams;
		}

		bool NeedsGeneratedDefaultConstructor(INamedTypeSymbol sym)
		{
			// Easy case - no declaration syntax.
			if (sym.DeclaringSyntaxReferences.IsDefaultOrEmpty)
			{
				return false;
			}

			var bReturn = false;
			foreach (var mem in sym.GetMembers())
			{
				switch (mem.Kind)
				{
					case SymbolKind.Method:
						{
							var method = (IMethodSymbol)mem;

							// If we've found a default instance constructor
							// that is explicitly defined, we can immediately
							// return false.
							if (method.MethodKind == MethodKind.Constructor &&
								!method.IsStatic &&
								!method.DeclaringSyntaxReferences.IsDefaultOrEmpty)
							{
								return false;
							}
						}
						break;

					case SymbolKind.Field:
						{
							var field = (IFieldSymbol)mem;
							if (field.IsStatic)
							{
								continue;
							}

							// Simple case.
							if (!field.Type.IsReferenceType)
							{
								bReturn = true;
								continue;
							}

							// Simple case if no initializer.
							if (field.DeclaringSyntaxReferences.IsDefaultOrEmpty)
							{
								continue;
							}
							var node = (VariableDeclaratorSyntax)field.DeclaringSyntaxReferences[0].GetSyntax();
							if (node?.Initializer == null)
							{
								continue;
							}

							// Otherwise, we need to determine if the initializer
							// for the field is null. Note that because field is likely
							// in a different syntax tree, we need to use a resolved model
							// here.
							var model = m_Model.SharedModel.GetSemanticModel(node.SyntaxTree);
							var constant = model.GetConstantValue(node.Initializer.Value);
							if (!constant.HasValue || !(constant.Value is null))
							{
								bReturn = true;
								continue;
							}
						}
						break;

					case SymbolKind.Property:
						{
							var prop = (IPropertySymbol)mem;

							// Don't need a constructor for static properties.
							if (prop.IsStatic)
							{
								continue;
							}

							// Don't need a constructor for non-auto properties.
							if (!prop.IsAutoProperty())
							{
								continue;
							}

							// Simple case - if the prop is not a reference type
							// and is automatic (from previous check), we must
							// emit a constructor for it.
							if (!prop.Type.IsReferenceType)
							{
								bReturn = true;
								continue;
							}

							// Simple case if no initializer.
							var node = (PropertyDeclarationSyntax)prop.DeclaringSyntaxReferences[0].GetSyntax();
							if (node.Initializer == null)
							{
								continue;
							}

							// Otherwise, we need to determine if the initializer
							// for the field is null. Note that because field is likely
							// in a different syntax tree, we need to use a resolved model
							// here.
							var model = m_Model.SharedModel.GetSemanticModel(node.SyntaxTree);
							var constant = model.GetConstantValue(node.Initializer.Value);
							if (!constant.HasValue || !(constant.Value is null))
							{
								bReturn = true;
								continue;
							}
						}
						break;
				}
			}

			return bReturn;
		}

		void ProcessAndWriteLocalTopLevelDependencies(AccessorDeclarationSyntax node)
		{
			// Now traverse the body of node looking for any nodes that resolves
			// to a local declaration which has not yet been declared.
			var body = (null != node.Body ? (SyntaxNode)node.Body : node.ExpressionBody);

			// Resolve.
			var methodSym = m_Model.GetDeclaredSymbol(node).Reduce();

			ProcessAndWriteLocalTopLevelDependencies(body, methodSym);
		}

		void ProcessAndWriteLocalTopLevelDependencies(BaseMethodDeclarationSyntax node)
		{
			// Now traverse the body of node looking for any nodes that resolves
			// to a local declaration which has not yet been declared.
			var body = (null != node.Body ? (SyntaxNode)node.Body : node.ExpressionBody);

			// Resolve.
			var methodSym = m_Model.GetDeclaredSymbol(node).Reduce();

			ProcessAndWriteLocalTopLevelDependencies(body, methodSym);
		}

		void ProcessAndWriteLocalTopLevelDependencies(
			SyntaxNode body,
			IMethodSymbol methodSym)
		{
			// Early out if no locals.
			if (m_TypeScope.Count == 0 ||
				null == m_TypeScope.Back().m_tDeclarationsBoundAsTopLevelLocals)
			{
				return;
			}

			// Early out if no body.
			if (null == body)
			{
				return;
			}

			// Get the method sym to filter out self references.
			foreach (var child in body.DescendantNodes())
			{
				// Resolve the symbol for the node - no symbol, we're done.
				// Also, if the symbol is the method context itself, we're done.
				var childSym = m_Model.GetSymbolInfo(child).Symbol;
				if (null == childSym || methodSym == childSym)
				{
					continue;
				}

				// Check if the symbol is a local reference - if not, we're done.
				if (!CanEmitAsLocal(childSym))
				{
					continue;
				}

				// Resolve the declaration node of the symbol.
				var declNode = childSym.DeclaringSyntaxReferences[0].GetSyntax();
				// Resolve field parent.
				if (childSym.Kind == SymbolKind.Field)
				{
					while (declNode.Kind() != SyntaxKind.FieldDeclaration)
					{
						declNode = declNode.Parent;
					}
				}

				// If not emitted, emit and mark as emitted.
				var scope = m_TypeScope.Back();
				if (!scope.m_tDeclarationsBoundAsTopLevelLocals[declNode])
				{
					// One method, so we can just emit the childSym.
					if (declNode is MethodDeclarationSyntax)
					{
						Write(kLuaLocal);
						Write(' ');
						Write(m_Model.LookupOutputId(childSym));
						Write(';');

						// Now declared.
						scope.m_tDeclarationsBoundAsTopLevelLocals[declNode] = true;
					}
					// TODO: With some refactoring of VisitFieldDeclaration,
					// we could tie the emit state to the individual symbol instead
					// of the entire declaration.
					//
					// For fields, emit all declarations.
					else if (declNode is FieldDeclarationSyntax)
					{
						var fieldDecl = (FieldDeclarationSyntax)declNode;
						foreach (var v in fieldDecl.Declaration.Variables)
						{
							var fieldSym = m_Model.GetDeclaredSymbol(v);

							if (NeedsWhitespaceSeparation) { Write(' '); }
							Write(kLuaLocal);
							Write(' ');
							Write(m_Model.LookupOutputId(fieldSym));
							Write(';');
						}

						// Now declared.
						scope.m_tDeclarationsBoundAsTopLevelLocals[declNode] = true;
					}
				}
			}
		}

		string RemapMethodDeclarationId(IMethodSymbol sym)
		{
            switch (sym.MethodKind)
            {
                case MethodKind.UserDefinedOperator:
                    switch (sym.Name)
                    {
                        case "op_Addition": return "__add";
                        case "op_Division": return "__div";
						case "op_LessThan": return "__lt";
						case "op_LessThanOrEqual": return "__le";
						case "op_Modulus": return "__mod";
                        case "op_Multiply": return "__mul";
                        case "op_Subtraction": return "__sub";
						case "op_UnaryNegation": return "__unm";
					}
                    return null;
                default:
                    // TODO: Finish and centralize.
                    if (sym.IsCsharpToString())
                    {
                        return "__tostring";
                    }
                    else
                    {
                        return null;
                    }
            }
		}

		IMethodSymbol ResolveBaseConstructor(INamedTypeSymbol sym)
		{
			var parent = sym.BaseType;
			while (null != parent)
			{
				IMethodSymbol defaultConstructor = null;
				foreach (var construct in parent.Constructors)
				{
					// Skip static constructors.
					if (construct.IsStatic)
					{
						continue;
					}

					// Constructor needs to be invoked if it is
					// a default constructor and is also textually
					// defined (not an implicit internal constructor
					// automatically generated by the Roslyn
					// compiler).
					if (construct.Parameters.Length == 0)
					{
						defaultConstructor = construct;
						if (!construct.DeclaringSyntaxReferences.IsDefaultOrEmpty)
						{
							return construct;
						}
					}
				}

				// We need to handle the case where a constructor
				// has been generated to initialize instance members.
				if (null != defaultConstructor &&
					!parent.DeclaringSyntaxReferences.IsDefaultOrEmpty)
				{
					if (NeedsGeneratedDefaultConstructor(parent))
					{
						return defaultConstructor;
					}
				}

				parent = parent.BaseType;
			}

			return null;
		}

		/// <summary>
		/// Utility used by VisitConstructorDeclaration() - inline
		/// instance member initialization or default values are
		/// moved into the constructor. This emits those members
		/// as needed.
		/// </summary>
		void EmitInstanceMemberInitializers(List<MemberDeclarationSyntax> mems)
		{
			// Early out if no members.
			if (null == mems || mems.Count == 0)
			{
				return;
			}

			// Emit any fields or automatic properties as self assignments.

			// TODO: This is one of two places, currently, where
			// we're emitting code at a different line number than its
			// original line (the other are the incrementors for complex
			// for loops, which fortunately are not a reasonable break
			// location for line-only debug breaks anyway). I don't think
			// there's a workaround.
			//
			// Fundamentally, instance field initializers need to be
			// emitted inside constructors in Lua, and if they're not
			// ordered appropriately in the original file, they need
			// to be re-ordered.
			//
			// As a consequence, if this sticks, we'll need to add
			// debugger line remapping. Until then, the debugger will
			// not correctly break if placed at instance field
			// initializers.
			using (var fixedLine = new FixedLine(this, mems.Back()))
			{
				foreach (var v in mems)
				{
					// Skip members that can be omitted at this stage.
					if (v.CanOmitDeclarationStatement(m_Model))
					{
						continue;
					}

					// To avoid extraneous white space, emit the field directly.
					int iStart = m_iWriteOffset;
					if (v is FieldDeclarationSyntax field)
					{
						VisitFieldDeclaration(field);
					}
					else
					{
						Visit(v);
					}

					if (iStart != m_iWriteOffset)
					{
						// TODO: The actual completely correct thing to
						// do here would be to only terminate between fields,
						// and then only emit a final ';' if we know that the constructor
						// has at least one statement.

						// This is a special loop, so we need to terminate each
						// of this emits with a semicolon.
						Write(';');
					}
				}
			}
		}

		void VisitConstructorDeclaration(ConstructorDeclarationSyntax node, List<MemberDeclarationSyntax> mems, bool bInstance)
		{
			if (null != node)
			{
				m_Model.Enforce(node);
			}

			// Get the anchor - node may be null, in which case fields must be non-null.
			var anchor = (null == node ? mems.Back() : node);
			SeparateForFirst(anchor);

			using (var scope = new Scope(this, ScopeKind.Function, (null != node ? node.Body : null)))
			{
				// If node is null, we're generating a constructor.
				var bVarargs = false;
				SeparatedSyntaxList<ParameterSyntax> prms = default(SeparatedSyntaxList<ParameterSyntax>);
				if (null == node)
				{
					if (bInstance)
					{
						// Get the default constructor reference. C# generates
						// a default constructor for all named types, even
						// if one is not explicitly defined in code.
						IMethodSymbol constructor = null;
						var typeSym = m_TypeScope.Back().m_Symbol;
						foreach (var v in typeSym.Constructors)
						{
							if (v.Parameters.Length == 0)
							{
								constructor = v;
								break;
							}
						}

						// Error.
						if (null == constructor)
						{
							throw new SlimCSCompilationException($"type '{typeSym}' does not contain a default constructor.");
						}

						var sId = m_Model.LookupOutputId(constructor);
						bVarargs = constructor.Parameters.HasVarargs();
						WriteMemberIdentifierSpecifier(null, new SyntaxTokenList(), SyntaxKind.ConstructorDeclaration);
						Write(sId);
						Write("()");
					}
					else
					{
						WriteMemberIdentifierSpecifier(null, new SyntaxTokenList(), SyntaxKind.ConstructorDeclaration);
						Write(kLuaBaseStaticConstructorName);
						Write("()");
					}
				}
				else
				{
					ProcessAndWriteLocalTopLevelDependencies(node);

					var sym = m_Model.GetDeclaredSymbol(node);
					var sId = m_Model.LookupOutputId(sym);
					bVarargs = sym.Parameters.HasVarargs();
					WriteMemberIdentifierSpecifier(node, node.Modifiers, node.Kind());
					Write(sId);

					// Params.
					Indent();
					Write('(');
					prms = node.ParameterList.Parameters;
					for (int i = 0; i < prms.Count; ++i)
					{
						if (0 != i)
						{
							Write(",");
						}

						if (bVarargs && i + 1 == prms.Count)
						{
							SeparateForFirst(prms[i]);
							Write("...");
						}
						else
						{
							// TODO: Default value handling.
							// TODO: params
							Visit(prms[i]);
						}

						if (i + 1 < prms.Count)
						{
							WriteInteriorComment(
								prms[i].GetStartColumn(),
								prms[i + 1].GetStartLine(),
								prms[i + 1].GetStartColumn());
						}
						else
						{
							WriteInteriorComment(
								prms[i].GetStartColumn(),
								node.ParameterList.CloseParenToken.GetStartLine(),
								node.ParameterList.CloseParenToken.GetStartColumn());
						}
					}
					Write(')');
					Outdent();
				}

				Indent();

				// Default base handling, as well as explicit 'base' and 'this' handling,
				// for instance constructors. Track if we explicitly invoked a 'this' -
				// in that case, we should *not* emit member initialization, or it
				// will be executed twice erroneously.
				var bEmitMems = true;
				if (bInstance && (null == node || !node.Modifiers.HasToken(SyntaxKind.StaticKeyword)))
				{
					if (null != node && node.Initializer != null)
					{
						// Track.
						var bHasExplicitThisConstructor = (node.Initializer.ThisOrBaseKeyword.Kind() == SyntaxKind.ThisKeyword);

						// Emit mems is now always false - either this
						// is an explicit base() invocation, in which
						// case we'll emit the member instance initializations
						// right now, or it is a this() invocation, in
						// which case we only want the inner most constructor
						// to perform the initialization.
						bEmitMems = false;

						// If a base construction invocation, we need to emit
						// any inline initialization *first*, so that potential
						// virtual invocations from the parent construction see
						// those inline initializations and not the uninitialized
						// values.
						if (!bHasExplicitThisConstructor)
						{
							EmitInstanceMemberInitializers(mems);
						}

						// Emit 'base' or 'this' invocations as necessary.
						Visit(node.Initializer);
					}
					// Otherwise, find the parent constructor we need
					// to invoke. If there is one, invoke it.
					else
					{
						var parentConstructor = ResolveBaseConstructor(m_TypeScope.Back().m_Symbol);
						if (null != parentConstructor)
						{
							// If a base construction invocation, we need to emit
							// any inline initialization *first*, so that potential
							// virtual invocations from the parent construction see
							// those inline initializations and not the uninitialized
							// values.
							EmitInstanceMemberInitializers(mems);
							bEmitMems = false;

							VisitConstructorInitializer(parentConstructor, new ArgumentSyntax[0]);
						}
					}
				}

				// If we haven't emitted members otherwise,
				// do so now.
				if (bEmitMems)
				{
					EmitInstanceMemberInitializers(mems);
				}

				// Body.
				if (null != node)
				{
					if (bVarargs) { AddVararg(prms[prms.Count - 1]); }
					Visit(node.Body);
					if (bVarargs) { RemoveVararg(prms[prms.Count - 1]); }
					SeparateForLast(node);
				}
				else
				{
					SeparateForLast(mems.Back());
				}
				Outdent();
				Write(kLuaEnd);
			}
		}

		void VisitConstructorInitializer(IMethodSymbol sym, IReadOnlyList<ArgumentSyntax> args)
		{
			// Separation.
			if (NeedsWhitespaceSeparation) { Write(' '); }

			// Invoke.
			Write(m_Model.LookupOutputId(sym.ContainingType));
			Write('.');
			Write(m_Model.LookupOutputId(sym));
			Write('(');
			if (!sym.IsStatic)
			{
				Write(kLuaSelf);
			}

			foreach (var arg in args)
			{
				if (m_LastChar != '(') { Write(','); }
				Visit(arg.Expression);
			}

			Write(')');
		}

		void VisitParenthesizedLambdaExpression(
			ParenthesizedLambdaExpressionSyntax node,
			bool bEllideFunction,
			bool bEllideArg0)
		{
			m_Model.Enforce(node);

			using (var scope = new Scope(this, ScopeKind.Lambda, node.Body))
			{
				bool bVarargs = HasVarargs(node);
				if (!bEllideFunction) Write(kLuaFunction);

				// Params.
				Indent();
				Write('(');
				var prms = node.ParameterList.Parameters;
				int iStart = (bEllideArg0 ? 1 : 0);
				for (int i = iStart; i < prms.Count; ++i)
				{
					if (iStart != i)
					{
						Write(",");
					}

					if (bVarargs && i + 1 == prms.Count)
					{
						SeparateForFirst(prms[i]);
						Write("...");
					}
					else
					{
						// TODO: Default value handling.
						// TODO: params
						Visit(prms[i]);
					}

					if (i + 1 < prms.Count)
					{
						WriteInteriorComment(
							prms[i].GetStartColumn(),
							prms[i + 1].GetStartLine(),
							prms[i + 1].GetStartColumn());
					}
					else
					{
						WriteInteriorComment(
							prms[i].GetStartColumn(),
							node.ParameterList.CloseParenToken.GetStartLine(),
							node.ParameterList.CloseParenToken.GetStartColumn());
					}
				}
				Write(')');
				Outdent();

				// Body.
				Indent();
				if (bVarargs) { AddVararg(prms[prms.Count - 1]); }
				Visit(node.Body);
				if (bVarargs) { RemoveVararg(prms[prms.Count - 1]); }
				SeparateForLast(node);
				Outdent();
				Write(kLuaEnd);
			}
		}

		void SynthesizePropLookup(IMethodSymbol sym, SyntaxNode node, SyntaxTokenList mods)
		{
			// Prefix, if required.
			var parentProp = (PropertyDeclarationSyntax)node.Parent.Parent;
			if (!CanEmitAsLocal(parentProp, out _))
			{
				if (sym.IsStatic)
				{
					Write(GetTypeScope());
				}
				else
				{
					Write(kLuaSelf);
				}
				Write('.');
			}

			// Identifier.
			WriteIdWithDedup(parentProp);
		}

		void SynthesizePropMethod(IMethodSymbol sym, SyntaxNode node, SyntaxTokenList mods)
		{
			// Only supported for accessors that implement the get/set of a property.
			switch (sym.MethodKind)
			{
				case MethodKind.PropertyGet:
					{
						Write(' ');
						Write(kLuaReturn);
						Write(' ');
						SynthesizePropLookup(sym, node, mods);
					}
					break;
				case MethodKind.PropertySet:
					{
						Write(' ');
						SynthesizePropLookup(sym, node, mods);
						Write('=');
						Write(kCsharpValueArg);
					}
					break;
				default:
					throw new SlimCSCompilationException(node, $"cannot synthesize accessor of type '{sym.MethodKind}'");
			}
		}
		#endregion

		public override SyntaxNode VisitAccessorDeclaration(AccessorDeclarationSyntax node)
		{
			m_Model.Enforce(node);

			using (var scope = new Scope(this, ScopeKind.Function, node.Body))
			{
				var sym = m_Model.GetDeclaredSymbol(node);
				var sId = m_Model.LookupOutputId(sym);
				bool bVarargs = sym.Parameters.HasVarargs();

				// Include modifiers from both the accessor as well as its
				// container parent (e.g. property or indexer).
				var mods = node.Modifiers;
				if (node.Parent?.Parent is BasePropertyDeclarationSyntax prop)
				{
					mods = mods.AddRange(prop.Modifiers);
				}

				// Before starting our method definition, emit any dependent
				// locals. Private methods and private static/const fields
				// are emitted as top-level local variables in some cases.
				//
				// Because of this, if the method we're about to emit accesses
				// any before their (textual) definition point, we need to
				// predeclare the local variable.
				ProcessAndWriteLocalTopLevelDependencies(node);

				// Generate.
				WriteMemberIdentifierSpecifier(node, mods, node.Kind());
				Write(sId);

				// Params - must be emitted with fixed line handling
				// for accessors, since we're always moving
				// the parameters to their final location.
				(var prms, var bAddValue) = node.GetParameterList();
				using (var _ = new FixedLine(this, node))
				{
					Indent();
					Write('(');
					for (int i = 0; i < prms.Count; ++i)
					{
						if (0 != i)
						{
							Write(',');
						}

						if (bVarargs && i + 1 == prms.Count)
						{
							SeparateForFirst(prms[i]);
							Write("...");
						}
						else
						{
							// TODO: Default value handling.
							// TODO: params
							Visit(prms[i]);
						}

						if (i + 1 < prms.Count)
						{
							WriteInteriorComment(
								prms[i].GetStartColumn(),
								prms[i + 1].GetStartLine(),
								prms[i + 1].GetStartColumn());
						}
						else
						{
							WriteInteriorComment(
								prms[i].GetStartColumn(),
								prms.Last().GetStartLine(),
								prms.Last().GetStartColumn());
						}
					}

					if (bAddValue)
					{
						if (prms.Count > 0)
						{
							Write(',');
						}
						Write(kCsharpValueArg);
					}

					Write(')');
					Outdent();
				}

				// Body.
				Indent();
				if (bVarargs) { AddVararg(prms[prms.Count - 1]); }
				if (null != node.ExpressionBody)
				{
					Write(kLuaReturn);
					Visit(node.ExpressionBody.Expression);
				}
				else
				{
					// Accessor special handling, attempt to synthesize if a get or set of an auto property.
					if (null == node.Body)
					{
						SynthesizePropMethod(sym, node, mods);
					}
					else
					{
						Visit(node.Body);
					}
				}
				if (bVarargs) { RemoveVararg(prms[prms.Count - 1]); }
				SeparateForLast(node);
				Outdent();
				Write(kLuaEnd);
			}

			return node;
		}

		public override SyntaxNode VisitAnonymousMethodExpression(AnonymousMethodExpressionSyntax node)
		{
			m_Model.Enforce(node);

			using (var scope = new Scope(this, ScopeKind.Lambda, node.Body))
			{
				bool bVarargs = HasVarargs(node);
				Write(kLuaFunction);

				// Params.
				Indent();
				Write('(');
				var prms = (null != node.ParameterList ? node.ParameterList.Parameters : new SeparatedSyntaxList<ParameterSyntax>());
				for (int i = 0; i < prms.Count; ++i)
				{
					if (0 != i)
					{
						Write(",");
					}

					if (bVarargs && i + 1 == prms.Count)
					{
						SeparateForFirst(prms[i]);
						Write("...");
					}
					else
					{
						// TODO: Default value handling.
						// TODO: params
						Visit(prms[i]);
					}

					if (i + 1 < prms.Count)
					{
						WriteInteriorComment(
							prms[i].GetStartColumn(),
							prms[i + 1].GetStartLine(),
							prms[i + 1].GetStartColumn());
					}
					else
					{
						WriteInteriorComment(
							prms[i].GetStartColumn(),
							node.ParameterList.CloseParenToken.GetStartLine(),
							node.ParameterList.CloseParenToken.GetStartColumn());
					}
				}
				Write(')');
				Outdent();

				// Body.
				Indent();
				if (bVarargs) { AddVararg(prms[prms.Count - 1]); }
				Visit(node.Body);
				if (bVarargs) { RemoveVararg(prms[prms.Count - 1]); }
				SeparateForLast(node);
				Outdent();
				Write(kLuaEnd);
			}
			return node;
		}

		public override SyntaxNode VisitConstructorInitializer(ConstructorInitializerSyntax node)
		{
			var sym = (IMethodSymbol)m_Model.GetSymbolInfo(node).Symbol;
			if (null == sym)
			{
				throw new SlimCSCompilationException(node, "no valid constructor target.");
			}

			// Resolve arguments.
			var args = ResolveArguments(node, sym.Parameters, node.ArgumentList.Arguments);

			// Emit.
			VisitConstructorInitializer(sym, args);

			return node;
		}

		public override SyntaxNode VisitConstructorDeclaration(ConstructorDeclarationSyntax node)
		{
			VisitConstructorDeclaration(node, null, node.Modifiers.IsInstanceMember());
			return node;
		}

		public override SyntaxNode VisitLocalFunctionStatement(LocalFunctionStatementSyntax node)
		{
			m_Model.Enforce(node);

			using (var scope = new Scope(this, ScopeKind.Function, node.Body))
			{
				bool bVarargs = HasVarargs(node.ParameterList);

				Write(kLuaLocal);
				Write(' ');
				Write(kLuaFunction);
				Write(' ');
				Write(node.Identifier);

				// Params.
				Indent();
				Write('(');
				var prms = node.ParameterList.Parameters;
				for (int i = 0; i < prms.Count; ++i)
				{
					if (0 != i)
					{
						Write(",");
					}

					if (bVarargs && i + 1 == prms.Count)
					{
						SeparateForFirst(prms[i]);
						Write("...");
					}
					else
					{
						// TODO: Default value handling.
						// TODO: params
						Visit(prms[i]);
					}

					if (i + 1 < prms.Count)
					{
						WriteInteriorComment(
							prms[i].GetStartColumn(),
							prms[i + 1].GetStartLine(),
							prms[i + 1].GetStartColumn());
					}
					else
					{
						WriteInteriorComment(
							prms[i].GetStartColumn(),
							node.ParameterList.CloseParenToken.GetStartLine(),
							node.ParameterList.CloseParenToken.GetStartColumn());
					}
				}
				Write(')');
				Outdent();

				// Body.
				Indent();
				if (bVarargs) { AddVararg(prms[prms.Count - 1]); }
				Visit(node.Body);
				if (bVarargs) { RemoveVararg(prms[prms.Count - 1]); }
				SeparateForLast(node);
				Outdent();
				Write(kLuaEnd);
			}

			return node;
		}

		public override SyntaxNode VisitMethodDeclaration(MethodDeclarationSyntax node)
		{
			m_Model.Enforce(node);

			using (var scope = new Scope(this, ScopeKind.Function, node.Body))
			{
				var sym = m_Model.GetDeclaredSymbol(node);
				var sId = RemapMethodDeclarationId(sym);
				if (null == sId)
				{
					sId = m_Model.LookupOutputId(sym);
				}
				bool bVarargs = sym.Parameters.HasVarargs();

				// Before starting our method definition, emit any dependent
				// locals. Private methods and private static/const fields
				// are emitted as top-level local variables in some cases.
				//
				// Because of this, if the method we're about to emit accesses
				// any before their (textual) definition point, we need to
				// predeclare the local variable.
				ProcessAndWriteLocalTopLevelDependencies(node);

				WriteMemberIdentifierSpecifier(node, node.Modifiers, node.Kind());
				Write(sId);

				// Params.
				Indent();
				Write('(');

				// Add type parameters if defined.
				if (null != node.TypeParameterList)
				{
					var typePrms = node.TypeParameterList.Parameters;
					for (int i = 0; i < typePrms.Count; ++i)
					{
						if (0 != i)
						{
							Write(',');
						}
						Visit(typePrms[i]);
					}
				}

				// Parameters count.
				var prms = node.ParameterList.Parameters;

				// Add self if this is a non-static method function
				// that's been emitted as a local top-level variable.
				var methodSym = (sym as IMethodSymbol);
				if (CanEmitAsLocal(methodSym) && // Must be first, true here implicitly means sym is not null.
					!methodSym.IsStatic)
				{
					if (m_LastChar != '(') { Write(','); }
					Visit(kThisExpression);
				}

				if (prms.Count > 0 && m_LastChar != '(') { Write(','); }
				for (int i = 0; i < prms.Count; ++i)
				{
					if (0 != i)
					{
						Write(',');
					}

					if (bVarargs && i + 1 == prms.Count)
					{
						SeparateForFirst(prms[i]);
						Write("...");
					}
					else
					{
						// TODO: Default value handling.
						// TODO: params
						Visit(prms[i]);
					}

					if (i + 1 < prms.Count)
					{
						WriteInteriorComment(
							prms[i].GetStartColumn(),
							prms[i + 1].GetStartLine(),
							prms[i + 1].GetStartColumn());
					}
					else
					{
						WriteInteriorComment(
							prms[i].GetStartColumn(),
							node.ParameterList.CloseParenToken.GetStartLine(),
							node.ParameterList.CloseParenToken.GetStartColumn());
					}
				}
				Write(')');
				Outdent();

				// Body.
				Indent();
				if (bVarargs) { AddVararg(prms[prms.Count - 1]); }
				if (null != node.ExpressionBody)
				{
					Write(kLuaReturn);
					Visit(node.ExpressionBody.Expression);
				}
				else
				{
					Visit(node.Body);
				}
				if (bVarargs) { RemoveVararg(prms[prms.Count - 1]); }
				SeparateForLast(node);
				Outdent();
				Write(kLuaEnd);

				// If valid, mark as declared.
				OnDeclare(node);
			}

			return node;
		}

        public override SyntaxNode VisitOperatorDeclaration(OperatorDeclarationSyntax node)
        {
            m_Model.Enforce(node);

            using (var scope = new Scope(this, ScopeKind.Function, node.Body))
            {
                var sym = m_Model.GetDeclaredSymbol(node);
                var sId = RemapMethodDeclarationId(sym);
                if (null == sId)
                {
                    sId = m_Model.LookupOutputId(sym);
                }
                bool bVarargs = sym.Parameters.HasVarargs();

                // Before starting our method definition, emit any dependent
                // locals. Private methods and private static/const fields
                // are emitted as top-level local variables in some cases.
                //
                // Because of this, if the method we're about to emit accesses
                // any before their (textual) definition point, we need to
                // predeclare the local variable.
                ProcessAndWriteLocalTopLevelDependencies(node);

                WriteMemberIdentifierSpecifier(node, node.Modifiers, node.Kind());
                Write(sId);

                // Params.
                Indent();
                Write('(');

                // Parameters count.
                var prms = node.ParameterList.Parameters;

                // Add self if this is a non-static method function
                // that's been emitted as a local top-level variable.
                var methodSym = (sym as IMethodSymbol);
                if (CanEmitAsLocal(methodSym) && // Must be first, true here implicitly means sym is not null.
                    !methodSym.IsStatic)
                {
                    if (m_LastChar != '(') { Write(','); }
                    Visit(kThisExpression);
                }

                if (prms.Count > 0 && m_LastChar != '(') { Write(','); }
                for (int i = 0; i < prms.Count; ++i)
                {
                    if (0 != i)
                    {
                        Write(',');
                    }

                    if (bVarargs && i + 1 == prms.Count)
                    {
                        SeparateForFirst(prms[i]);
                        Write("...");
                    }
                    else
                    {
                        // TODO: Default value handling.
                        // TODO: params
                        Visit(prms[i]);
                    }

                    if (i + 1 < prms.Count)
                    {
                        WriteInteriorComment(
                            prms[i].GetStartColumn(),
                            prms[i + 1].GetStartLine(),
                            prms[i + 1].GetStartColumn());
                    }
                    else
                    {
                        WriteInteriorComment(
                            prms[i].GetStartColumn(),
                            node.ParameterList.CloseParenToken.GetStartLine(),
                            node.ParameterList.CloseParenToken.GetStartColumn());
                    }
                }
                Write(')');
                Outdent();

                // Body.
                Indent();
                if (bVarargs) { AddVararg(prms[prms.Count - 1]); }
                if (null != node.ExpressionBody)
                {
                    Write(kLuaReturn);
                    Visit(node.ExpressionBody.Expression);
                }
                else
                {
                    Visit(node.Body);
                }
                if (bVarargs) { RemoveVararg(prms[prms.Count - 1]); }
                SeparateForLast(node);
                Outdent();
                Write(kLuaEnd);

                // If valid, mark as declared.
                OnDeclare(node);
            }

            return node;
        }

        public override SyntaxNode VisitParenthesizedLambdaExpression(ParenthesizedLambdaExpressionSyntax node)
		{
			VisitParenthesizedLambdaExpression(node, false, false);
			return node;
		}

		public override SyntaxNode VisitSimpleLambdaExpression(SimpleLambdaExpressionSyntax node)
		{
			using (var scope = new Scope(this, ScopeKind.Lambda, node.Body))
			{
				Write(kLuaFunction);

				// Param.
				Indent();
				Write('(');
				Visit(node.Parameter);
				Write(')');
				Outdent();

				// Body.
				Indent();

				if (node.Body is ExpressionSyntax)
				{
					Write(kLuaReturn);
				}
				Visit(node.Body);
				SeparateForLast(node);
				Outdent();
				Write(kLuaEnd);
			}

			return node;
		}
	}
}
