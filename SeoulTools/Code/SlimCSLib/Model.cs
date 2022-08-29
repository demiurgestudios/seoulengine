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
using System.Threading;
using System.Threading.Tasks;

using SlimCSLib.Generator;

namespace SlimCSLib
{
	public static class ModelExtensions
	{
		public static string ReducedName(this ISymbol sym)
		{
			switch (sym.Kind)
			{
				case SymbolKind.Discard:
					return Constants.kCsharpDiscard;

				case SymbolKind.Method:
					return ReducedName((IMethodSymbol)sym);

				case SymbolKind.DynamicType:
				case SymbolKind.Event:
				case SymbolKind.Field:
				case SymbolKind.Label:
				case SymbolKind.Local:
				case SymbolKind.Namespace:
				case SymbolKind.NamedType:
				case SymbolKind.Parameter:
				case SymbolKind.Property:
				case SymbolKind.TypeParameter:
					return sym.Name.ToValidLuaIdent();

				default:
					throw new SlimCSCompilationException($"name reduction of symbol kind of '{sym.Kind}' not implemented.");
			}
		}

		public static string ReducedName(this IMethodSymbol sym)
		{
			switch (sym.MethodKind)
			{
				case MethodKind.Constructor:
					return Constants.kLuaBaseConstructorName;
				case MethodKind.Destructor:
					return Constants.kLuaFinalizer;

				case MethodKind.ExplicitInterfaceImplementation:
					if (sym.ExplicitInterfaceImplementations.Length == 1)
					{
						return sym.ExplicitInterfaceImplementations[0].Name.ToValidLuaIdent();
					}
					else
					{
						return sym.Name.ToValidLuaIdent();
					}

				case MethodKind.Conversion:
				case MethodKind.DelegateInvoke:
				case MethodKind.EventAdd:
				case MethodKind.EventRemove:
				case MethodKind.LocalFunction:
				case MethodKind.Ordinary:
				case MethodKind.PropertySet:
				case MethodKind.UserDefinedOperator:
					return sym.Name.ToValidLuaIdent();

					// TODO: Generalize.
				case MethodKind.PropertyGet:
					if (sym.ContainingType.SpecialType == SpecialType.System_String &&
						sym.Name == "get_Length")
					{
						return Constants.kLuaStringLength;
					}
					else
					{
						return sym.Name.ToValidLuaIdent();
					}

				case MethodKind.StaticConstructor:
					return Constants.kLuaBaseStaticConstructorName;

				default:
					throw new SlimCSCompilationException($"name reduction of method kind of '{sym.MethodKind}' not implemented.");
			}
		}
	}
	/// <summary>
	/// SlimCS utility class around the SemanticModel.
	/// </summary>
	public sealed class Model
	{
		#region Private members
		readonly SharedModel m_Shared;
		readonly SyntaxTree m_SyntaxTree;
		SemanticModel m_LazyModel = null;

		SemanticModel SemanticModel
		{
			get
			{
				if (null == m_LazyModel)
				{
					m_LazyModel = m_Shared.GetSemanticModel(m_SyntaxTree);
				}

				return m_LazyModel;
			}
		}
		#endregion

		public Model(SharedModel shared, SyntaxTree syntaxTree)
		{
			m_Shared = shared;
			m_SyntaxTree = syntaxTree;
		}

		#region SemanticModel API
		public ControlFlowAnalysis AnalyzeControlFlow(SyntaxNode statement) { return SemanticModel.AnalyzeControlFlow(statement); }
		public DataFlowAnalysis AnalyzeDataFlow(SyntaxNode statementOrExpression) { return SemanticModel.AnalyzeDataFlow(statementOrExpression); }

		public Optional<object> GetConstantValue(SyntaxNode node)
		{
			// Handling for generated nodes that will not be in the input tree.
			if (SemanticModel.SyntaxTree != node.SyntaxTree) { return default(Optional<object>); }

			return SemanticModel.GetConstantValue(node);
		}

		public ISymbol GetDeclaredSymbol(SyntaxNode declaration) { return SemanticModel.GetDeclaredSymbol(declaration); }
		public INamedTypeSymbol GetDeclaredSymbol(BaseTypeDeclarationSyntax declarationSyntax) { return SemanticModel.GetDeclaredSymbol(declarationSyntax); }
		public ISymbol GetDeclaredSymbol(MemberDeclarationSyntax declarationSyntax) { return SemanticModel.GetDeclaredSymbol(declarationSyntax); }
		public IPropertySymbol GetDeclaredSymbol(PropertyDeclarationSyntax declarationSyntax) { return SemanticModel.GetDeclaredSymbol(declarationSyntax); }
		public INamedTypeSymbol GetDeclaredSymbol(DelegateDeclarationSyntax declarationSyntax) { return SemanticModel.GetDeclaredSymbol(declarationSyntax); }
		public IFieldSymbol GetDeclaredSymbol(EnumMemberDeclarationSyntax declarationSyntax) { return SemanticModel.GetDeclaredSymbol(declarationSyntax); }
		public IMethodSymbol GetDeclaredSymbol(BaseMethodDeclarationSyntax declarationSyntax) { return SemanticModel.GetDeclaredSymbol(declarationSyntax); }
		public IRangeVariableSymbol GetDeclaredSymbol(JoinIntoClauseSyntax declarationSyntax) { return SemanticModel.GetDeclaredSymbol(declarationSyntax); }
		public INamespaceSymbol GetDeclaredSymbol(NamespaceDeclarationSyntax declarationSyntax) { return SemanticModel.GetDeclaredSymbol(declarationSyntax); }
		public IPropertySymbol GetDeclaredSymbol(IndexerDeclarationSyntax declarationSyntax) { return SemanticModel.GetDeclaredSymbol(declarationSyntax); }
		public ISymbol GetDeclaredSymbol(BasePropertyDeclarationSyntax declarationSyntax) { return SemanticModel.GetDeclaredSymbol(declarationSyntax); }
		public IPropertySymbol GetDeclaredSymbol(AnonymousObjectMemberDeclaratorSyntax declarationSyntax) { return SemanticModel.GetDeclaredSymbol(declarationSyntax); }
		public IRangeVariableSymbol GetDeclaredSymbol(QueryClauseSyntax declarationSyntax) { return SemanticModel.GetDeclaredSymbol(declarationSyntax); }
		public ILocalSymbol GetDeclaredSymbol(CatchDeclarationSyntax declarationSyntax) { return SemanticModel.GetDeclaredSymbol(declarationSyntax); }
		public ILocalSymbol GetDeclaredSymbol(ForEachStatementSyntax declarationSyntax) { return SemanticModel.GetDeclaredSymbol(declarationSyntax); }
		public ITypeParameterSymbol GetDeclaredSymbol(TypeParameterSyntax declarationSyntax) { return SemanticModel.GetDeclaredSymbol(declarationSyntax); }
		public IParameterSymbol GetDeclaredSymbol(ParameterSyntax declarationSyntax) { return SemanticModel.GetDeclaredSymbol(declarationSyntax); }
		public IAliasSymbol GetDeclaredSymbol(ExternAliasDirectiveSyntax declarationSyntax) { return SemanticModel.GetDeclaredSymbol(declarationSyntax); }
		public IAliasSymbol GetDeclaredSymbol(UsingDirectiveSyntax declarationSyntax) { return SemanticModel.GetDeclaredSymbol(declarationSyntax); }
		public ILabelSymbol GetDeclaredSymbol(SwitchLabelSyntax declarationSyntax) { return SemanticModel.GetDeclaredSymbol(declarationSyntax); }
		public IEventSymbol GetDeclaredSymbol(EventDeclarationSyntax declarationSyntax) { return SemanticModel.GetDeclaredSymbol(declarationSyntax); }
		public ILabelSymbol GetDeclaredSymbol(LabeledStatementSyntax declarationSyntax) { return SemanticModel.GetDeclaredSymbol(declarationSyntax); }
		public IRangeVariableSymbol GetDeclaredSymbol(QueryContinuationSyntax declarationSyntax) { return SemanticModel.GetDeclaredSymbol(declarationSyntax); }
		public ISymbol GetDeclaredSymbol(SingleVariableDesignationSyntax declarationSyntax) { return SemanticModel.GetDeclaredSymbol(declarationSyntax); }
		public INamedTypeSymbol GetDeclaredSymbol(AnonymousObjectCreationExpressionSyntax declarationSyntax) { return SemanticModel.GetDeclaredSymbol(declarationSyntax); }
		public ISymbol GetDeclaredSymbol(TupleElementSyntax declarationSyntax) { return SemanticModel.GetDeclaredSymbol(declarationSyntax); }
		public ISymbol GetDeclaredSymbol(VariableDeclaratorSyntax declarationSyntax) { return SemanticModel.GetDeclaredSymbol(declarationSyntax); }
		public INamedTypeSymbol GetDeclaredSymbol(TupleExpressionSyntax declarationSyntax) { return SemanticModel.GetDeclaredSymbol(declarationSyntax); }
		public ISymbol GetDeclaredSymbol(ArgumentSyntax declarationSyntax) { return SemanticModel.GetDeclaredSymbol(declarationSyntax); }
		public IMethodSymbol GetDeclaredSymbol(AccessorDeclarationSyntax declarationSyntax) { return SemanticModel.GetDeclaredSymbol(declarationSyntax); }

		public SymbolInfo GetSymbolInfo(SyntaxNode node)
		{
			// Handling for generated nodes that will not be in the input tree.
			if (SemanticModel.SyntaxTree != node.SyntaxTree) { return default(SymbolInfo); }

			return SemanticModel.GetSymbolInfo(node);
		}
		public TypeInfo GetTypeInfo(SyntaxNode node)
		{
			// Handling for generated nodes that will not be in the input tree.
			if (SemanticModel.SyntaxTree != node.SyntaxTree) { return default(TypeInfo); }

			return SemanticModel.GetTypeInfo(node);
		}

		public ImmutableArray<ISymbol> LookupBaseMembers(int position, string name = null) { return SemanticModel.LookupBaseMembers(position, name); }
		public ImmutableArray<ISymbol> LookupLabels(int position, string name = null) { return SemanticModel.LookupLabels(position, name); }
		public ImmutableArray<ISymbol> LookupNamespacesAndTypes(int position, INamespaceOrTypeSymbol container = null, string name = null) { return SemanticModel.LookupNamespacesAndTypes(position, container, name); }
		public ImmutableArray<ISymbol> LookupStaticMembers(int position, INamespaceOrTypeSymbol container = null, string name = null) { return SemanticModel.LookupStaticMembers(position, container, name); }
		public ImmutableArray<ISymbol> LookupSymbols(int position, INamespaceOrTypeSymbol container = null, string name = null, bool includeReducedExtensionMethods = false) { return SemanticModel.LookupSymbols(position, container, name, includeReducedExtensionMethods); }
		#endregion

		public bool CanInitInline(ISymbol sym) { return m_Shared.CanInitInline(sym); }

		public string LookupOutputId(ClassDeclarationSyntax node) { return LookupOutputId(GetDeclaredSymbol(node)); }
		public string LookupOutputId(EnumDeclarationSyntax node) { return LookupOutputId(GetDeclaredSymbol(node)); }
		public string LookupOutputId(IMethodSymbol sym) { return m_Shared.LookupOutputId(sym); }
		public string LookupOutputId(INamedTypeSymbol sym) { return m_Shared.LookupOutputId(sym); }
		public string LookupOutputId(InterfaceDeclarationSyntax node) { return LookupOutputId(GetDeclaredSymbol(node)); }
		public string LookupOutputId(ISymbol sym) { return m_Shared.LookupOutputId(sym); }

		public SharedModel SharedModel { get { return m_Shared; } }

		public SyntaxTree SyntaxTree { get { return m_SyntaxTree; } }
	}

	/// <summary>
	/// SlimCS utility around (roughly) CSharpCompilation. Shared
	/// data used by many Model instances.
	/// </summary>
	public sealed class SharedModel
	{
		#region Private members
		int m_AdditionalInitProgress = 0;
		readonly CommandArgs m_Args;
		readonly CSharpCompilation m_Compilation;
		readonly HashSet<ISymbol> m_CanInitInline;
		readonly Dictionary<ISymbol, string> m_SymbolLookup;
		readonly Dictionary<ITypeSymbol, (int, bool)> m_TypeDepOrder;

		static List<INamedTypeSymbol> BuildToProcess(
			CommandArgs args,
			CSharpCompilation compilation)
		{
			// Enumerate all member symbols in the entire
			// compilation. We do this into a list and then sort
			// to make this step deterministic.
			var toProcess = new List<INamedTypeSymbol>();
			foreach (var sym in compilation.GetSymbolsWithName((string s) => { return true; }, SymbolFilter.Type))
			{
				toProcess.Add((INamedTypeSymbol)sym);
			}

			// Sort the list into a deterministic order by full name (ToString()).
			toProcess.Sort((a, b) =>
			{
				var da = a.DeclaringSyntaxReferences;
				var db = b.DeclaringSyntaxReferences;

				if (da.IsDefaultOrEmpty && db.IsDefaultOrEmpty)
				{
					return 0;
				}
				else if (da.IsDefaultOrEmpty)
				{
					return -1;
				}
				else if (db.IsDefaultOrEmpty)
				{
					return 1;
				}
				else
				{
					int iReturn = da[0].SyntaxTree.FilePath.CompareTo(db[0].SyntaxTree.FilePath);
					if (0 == iReturn)
					{
						var na = da[0].GetSyntax();
						var nb = db[0].GetSyntax();

						return na.Span.Start.CompareTo(nb.Span.Start);
					}
					else
					{
						return iReturn;
					}
				}
			});

			return toProcess;
		}

		static bool ValueCanInitInline(SemanticModel model,  ExpressionSyntax expr)
		{
			// Functions definitions are allowed inline.
			if (expr is AnonymousFunctionExpressionSyntax)
			{
				return true;
			}

			if (model.GetConstantValue(expr).HasValue)
			{
				return true;
			}

			// Table initializers can also be done inline if they're either default,
			// or all their values are also allowed inlined.
			var objectCreate = expr as ObjectCreationExpressionSyntax;
			if (null == objectCreate)
			{
				return false;
			}

			var ti = model.GetTypeInfo(objectCreate);
			if (!ti.Type.IsLuaTable())
			{
				return false;
			}

			if ((null == objectCreate.Initializer || objectCreate.Initializer.Expressions.Count == 0) &&
				(null == objectCreate.ArgumentList || objectCreate.ArgumentList.Arguments.Count == 0))
			{
				return true;
			}

			// Check elements.
			if (null != objectCreate.Initializer)
			{
				foreach (var v in objectCreate.Initializer.Expressions)
				{
					var innerExpr = v;
					if (innerExpr is AssignmentExpressionSyntax assign)
					{
						var left = assign.Left;
						var right = assign.Right;

						if (left is ImplicitElementAccessSyntax impl)
						{
							left = impl.ArgumentList.Arguments[0].Expression;
						}

						if (!ValueCanInitInline(model, left)) { return false; }
						if (!ValueCanInitInline(model, right)) { return false; }
					}
					else
					{
						if (!ValueCanInitInline(model, innerExpr))
						{
							return false;
						}
					}
				}
			}
			if (null != objectCreate.ArgumentList)
			{
				foreach (var v in objectCreate.ArgumentList.Arguments)
				{
					var innerExpr = v.Expression;
					if (!ValueCanInitInline(model, innerExpr))
					{
						return false;
					}
				}
			}

			return true;
		}

		static HashSet<ISymbol> BuildCanInitInline(
			CommandArgs args,
			CSharpCompilation compilation)
		{
			var ret = new HashSet<ISymbol>();
			foreach (var sym in compilation.GetSymbolsWithName((string s) => { return true; }, SymbolFilter.Member))
			{
				if (sym is IFieldSymbol field)
				{
					// Constants can be initialized inline.
					if (field.HasConstantValue)
					{
						ret.Add(sym);
						continue;
					}

					// Only applies to statics after this point.
					if (!field.IsStatic)
					{
						continue;
					}

					if (field.DeclaringSyntaxReferences.IsDefaultOrEmpty)
					{
						continue;
					}

					var vardecl = (VariableDeclaratorSyntax)field.DeclaringSyntaxReferences[0].GetSyntax();
					if (null == vardecl.Initializer)
					{
						ret.Add(sym);
						continue;
					}

					var model = compilation.GetSemanticModel(vardecl.SyntaxTree);
					var init = ((EqualsValueClauseSyntax)vardecl.Initializer).Value;
					if (ValueCanInitInline(model, init))
					{
						ret.Add(sym);
						continue;
					}
				}
				else if (sym is IPropertySymbol prop)
				{
					if (!prop.IsAutoProperty())
					{
						continue;
					}

					// Only applies to statics after this point.
					if (!prop.IsStatic)
					{
						continue;
					}

					var propdecl = (PropertyDeclarationSyntax)prop.DeclaringSyntaxReferences[0].GetSyntax();
					if (null == propdecl.Initializer)
					{
						ret.Add(sym);
						continue;
					}

					var init = ((EqualsValueClauseSyntax)propdecl.Initializer).Value;
					var model = compilation.GetSemanticModel(propdecl.SyntaxTree);
					if (model.GetConstantValue(init).HasValue)
					{
						ret.Add(sym);
						continue;
					}

					// Table initializers can also be done inline if they're either default,
					// or all their values are also allowed inlined.
					var objectCreate = init as ObjectCreationExpressionSyntax;
					if (null != objectCreate &&
						model.GetTypeInfo(objectCreate).Type.IsLuaTable())
					{
						if ((null == objectCreate.Initializer || objectCreate.Initializer.Expressions.Count == 0) &&
							(null == objectCreate.ArgumentList || objectCreate.ArgumentList.Arguments.Count == 0))
						{
							ret.Add(sym);
							continue;
						}
					}
				}
			}

			return ret;
		}

		static Dictionary<ISymbol, string> BuildSymbolLookup(
			CommandArgs args,
			CSharpCompilation compilation,
			List<INamedTypeSymbol> toProcess)
		{
			// Types are globally deduped (all types, even if nested, are given
			// a unique identifier). Members are deduped within the type
			// and in consideration of parent types.
			var global = new HashSet<string>();
			var typeDedup = new Dictionary<ITypeSymbol, Dictionary<string, HashSet<ISymbol>>>();
			var retLookup = new Dictionary<ISymbol, string>();

			// Local methods used to dedup a member
			// in a type.
			HashSet<ISymbol> ResolveSet(
				Dictionary<string, HashSet<ISymbol>> dict,
				string sId)
			{
				HashSet<ISymbol> ret;
				if (!dict.TryGetValue(sId, out ret))
				{
					ret = new HashSet<ISymbol>();
					dict.Add(sId, ret);
				}
				return ret;
			}

			bool AddToType(
				Dictionary<string, HashSet<ISymbol>> dict,
				string sId,
				ISymbol memSym)
			{
				var set = ResolveSet(dict, sId);

				// We can add if:
				// 1. set is empty.
				// 2. set has only 1 entry and it's equal
				//    to memSym.
				// 3. all colliders are private and static or const
				//    and they are in different types.
				// 4. memSym is a method or property, and it overrides
				//    one of the symbols in the set.
				// 5. constructors are the last exception,
				//    they are allowed to collide, since they
				//    are invoked specially by the new
				//    implementation.
				if (set.Count == 0)
				{
					set.Add(memSym);
					return true;
				}

				// Case 2.
				if (set.Count == 1 && set.Contains(memSym))
				{
					return true;
				}

				// Case 3.
				if (memSym.DeclaredAccessibility == Accessibility.Private &&
					memSym.IsStatic)
				{
					var bOk = true;
					foreach (var existingMem in set)
					{
						// Not static, not private or in the same
						// type as the member we're trying to insert,
						// this duplication is not allowed.
						if (existingMem.ContainingType == memSym.ContainingType ||
							existingMem.DeclaredAccessibility != Accessibility.Private ||
							!existingMem.IsStatic)
						{
							bOk = false;
							break;
						}
					}

					if (bOk)
					{
						return true;
					}
				}

				// Case 4 - if memSym is a method.
				if (memSym is IMethodSymbol methodSym)
				{
					// Case 5 - constructors and finalizers are allowed to collide
					// with other constructors as long as the collision
					// does not occur within the same type (e.g if
					// A : B, A can define constructors that overlap
					// with constructors defined in B, but not other
					// constructors defined in A).
					if (methodSym.MethodKind == MethodKind.Constructor ||
						methodSym.MethodKind == MethodKind.Destructor ||
						methodSym.MethodKind == MethodKind.StaticConstructor)
					{
						var bOk = true;
						foreach (var existing in set)
						{
							// Can only collide with other constructors.
							if (!(existing is IMethodSymbol))
							{
								bOk = false;
								break;
							}
							var existingMethod = (IMethodSymbol)existing;
							if (existingMethod.MethodKind != methodSym.MethodKind)
							{
								bOk = false;
								break;
							}

							// Final check - if existingMethod has the same ContainingType
							// as methodSym, that's not ok.
							if (existingMethod.ContainingType == methodSym.ContainingType)
							{
								bOk = false;
								break;
							}
						}

						// Collision is acceptable.
						if (bOk)
						{
							set.Add(memSym);
							return true;
						}
					}

					// Case 4 - Now walk the set and see if any
					// of the existing entries are the method
					// overriden by the current method.
					foreach (var existing in set)
					{
						if (existing == methodSym.OverriddenMethod)
						{
							set.Add(memSym);
							return true;
						}
					}
				}
				// Case 4 - if memSym is a property.
				else if (memSym is IPropertySymbol propSym)
				{
					// Case 4 - Now walk the set and see if any
					// of the existing entries are the property
					// overriden by the current property.
					foreach (var existing in set)
					{
						if (existing == propSym.OverriddenProperty)
						{
							set.Add(memSym);
							return true;
						}
					}
				}

				// Done, failure.
				return false;
			}

			string GetQualifiedMethodId(IMethodSymbol methodSym)
			{
				// Generate a hashcode on the signature of the method.
				int iSignatureHash = 0;
				foreach (var typeParam in methodSym.TypeParameters)
				{
					var sName = typeParam.ToString();
					iSignatureHash = Utilities.IncrementalHash(iSignatureHash, sName.GetHashCode());
				}
				foreach (var prm in methodSym.Parameters)
				{
					var sName = prm.ToString();
					iSignatureHash = Utilities.IncrementalHash(iSignatureHash, sName.GetHashCode());
				}
				uint uSignatureHash = (uint)iSignatureHash;

				// Now generate the dedup name - format is <name><arity>_<hash>
				var sReturn =
					methodSym.ReducedName() +
					methodSym.Parameters.Length.ToString() +
					"_" +
					uSignatureHash.ToString("X");

				return sReturn;
			}

			void DedupMemberId(
				ITypeSymbol typeSym,
				Dictionary<string, HashSet<ISymbol>> dict,
				ISymbol memSym)
			{
				// Get the base id and is initial set.
				var sId = memSym.ReducedName();

				try
				{
					// On first collision, try deduping by prepending the
					// type id.
					if (!AddToType(dict, sId, memSym))
					{
						// If a method, dedup by generating a qualified name
						// fo the method (factoring in the signature).
						if (memSym.Kind == SymbolKind.Method)
						{
							sId = GetQualifiedMethodId((IMethodSymbol)memSym);

							// Done if add succeeds.
							if (AddToType(dict, sId, memSym))
							{
								return;
							}
						}

						// Next try deduping by prepending the type id.
						// We do this to make symbols as stable as possible
						// across changes (ideally, we don't want a change to
						// one file in a project to trigger a change to many
						// other files due to deduping).
						var sBaseId = sId = retLookup[typeSym] + sId;

						// Any additional steps we need to dedup with a number.
						int iDedup = 0;
						while (!AddToType(dict, sId, memSym))
						{
							sId = sBaseId + iDedup.ToString();
							++iDedup;
						}
					}
				}
				finally
				{
					// On final return, register the final id.
					retLookup.Add(memSym, sId);
				}
			}

			void DedupMember(
				ITypeSymbol typeSym,
				Dictionary<string, HashSet<ISymbol>> dict,
				ISymbol memSym)
			{
				switch (memSym.Kind)
				{
					// Skip named types, they're processed as top levels.
					case SymbolKind.NamedType:
						return;

					// All other types just dedup by name.
					case SymbolKind.Event:
					case SymbolKind.Field:
					case SymbolKind.Method:
					case SymbolKind.Property:
						DedupMemberId(typeSym, dict, memSym);
						break;

					// Fallback, catch types we haven't supported.
					default:
						throw new SlimCSCompilationException($"unexpected member type '{memSym.Kind}'.");
				}
			}

			Dictionary<string, HashSet<ISymbol>> GetDedupedTypeMembers(ITypeSymbol sym)
			{
				// Early out if we already have generated
				// the dictionary for the given type.
				Dictionary<string, HashSet<ISymbol>> ret = null;
				if (typeDedup.TryGetValue(sym, out ret))
				{
					return ret;
				}

				// Get the base type.
				var baseType = sym.BaseType;
				if (null != baseType)
				{
					// Populate initially from the immediate
					// base type's set.
					ret = new Dictionary<string, HashSet<ISymbol>>(GetDedupedTypeMembers(baseType));
				}
				else
				{
					// Otherwise, generate a new one.
					ret = new Dictionary<string, HashSet<ISymbol>>();
				}

				// Enumerate members.
				foreach (var v in sym.GetMembers())
				{
					DedupMember(sym, ret, v);
				}

				// Add and return here.
				typeDedup.Add(sym, ret);
				return ret;
			}

			void DedupType(INamedTypeSymbol typeSym)
			{
				// Make sure parents have deduped already.
				if (typeSym.BaseType != null)
				{
					DedupType(typeSym.BaseType);
				}

				// Early out if already deduped.
				if (retLookup.ContainsKey(typeSym))
				{
					return;
				}

				// If a specialized generic type, resolve
				// the name of the unbound/underlying named
				// type.
				var sId = typeSym.ReducedName();
				if (typeSym.IsGenericType && (typeSym != typeSym.ConstructedFrom))
				{
					var genericTypeSym = typeSym.ConstructedFrom;
					DedupType(genericTypeSym);
					sId = retLookup[genericTypeSym];
				}
				else
				{
					// Dedup the base name if necessary.
					int iDedup = 0;
					while (global.Contains(sId))
					{
						sId = sId + iDedup.ToString();
					}

					// Insert and add.
					global.Add(sId);
					retLookup.Add(typeSym, sId);
				}

				// Now dedup the members.
				GetDedupedTypeMembers(typeSym);
			}

			// Dedup types.
			foreach (var typeSym in toProcess)
			{
				// Perform the dedup.
				DedupType(typeSym);
			}

			return retLookup;
		}

		static Dictionary<ITypeSymbol, (int, bool)> BuildTypeDep(
			CommandArgs args,
			CSharpCompilation compilation,
			List<INamedTypeSymbol> toProcess,
			HashSet<ISymbol> canInitInline)
		{
			var typeDepend = new Dictionary<ITypeSymbol, (bool, HashSet<ITypeSymbol>)>();
			var requireLookup = new Dictionary<string, HashSet<ITypeSymbol>>();
			var deferredDep = new List<(ITypeSymbol, string)>();
			var visited = new Dictionary<SyntaxNode, HashSet<ITypeSymbol>>();

			void AddDependencies(ITypeSymbol sym)
			{
				switch (sym.TypeKind)
				{
					// Limited types support - some types don't need tracking.
					case TypeKind.Array:
					case TypeKind.Delegate:
					case TypeKind.Error:
					case TypeKind.Module:
					case TypeKind.Pointer:
					case TypeKind.Struct:
					case TypeKind.Submission:
					case TypeKind.TypeParameter:
					case TypeKind.Unknown:
						return;

					// Enums are included but are not scanned
					// for dependencies, since they can have
					// no dependencies and their values are
					// always inlined by the compiler.
					case TypeKind.Enum:
						typeDepend.Add(sym, (false, new HashSet<ITypeSymbol>()));
						return;

					// Interfaces are included but are not scanned
					// for dependencies, since their dependencies are
					// just tracked by string.
					case TypeKind.Interface:
						typeDepend.Add(sym, (false, new HashSet<ITypeSymbol>()));
						return;
				}

				// Check if this is a type we've defined.
				// We don't need to track other dependencies.
				if (sym.DeclaringSyntaxReferences.IsDefaultOrEmpty)
				{
					return;
				}

				// Skip any pseudo or special types.
				if (sym.GetLuaKind() != LuaKind.NotSpecial ||
					sym.IsSlimCSSystemLib() ||
					(null != sym.ContainingType && sym.ContainingType.IsSlimCSSystemLib()))
				{
					return;
				}

				var rootDeclSyntax = sym.DeclaringSyntaxReferences[0];
				var bStaticConstructor = false;

				HashSet<ITypeSymbol> ScanInstanceConstructors(INamedTypeSymbol namedSym)
				{
					var ret = new HashSet<ITypeSymbol>();
					foreach (var mem in namedSym.GetMembers())
					{
						if (mem is IMethodSymbol method)
						{
							if (method.MethodKind == MethodKind.Constructor &&
								!method.IsStatic)
							{
								foreach (var declRef in method.DeclaringSyntaxReferences)
								{
									ret.UnionWith(Scan(declRef.GetSyntax()));
								}
							}
						}
					}
					return ret;
				}

				HashSet<ITypeSymbol> Scan(SyntaxNode node)
				{
					// Early out if we've already scanned the given node.
					HashSet<ITypeSymbol> set = null;
					if (visited.TryGetValue(node, out set))
					{
						return set;
					}
					else
					{
						set = new HashSet<ITypeSymbol>();
						visited.Add(node, set);
					}

					SemanticModel model = compilation.GetSemanticModel(node.SyntaxTree);
					foreach (var child in node.DescendantNodes())
					{
						LuaKind eLuaKind = LuaKind.NotSpecial;

						// Invocations and object creation require additional handling.
						ISymbol invokeTarget = null;
						if (child.Kind() == SyntaxKind.InvocationExpression)
						{
							// Resolve the target method and if defined, scan that as well.
							var invoke = (InvocationExpressionSyntax)child;
							invokeTarget = model.GetSymbolInfo(invoke.Expression).Symbol;

							// Very special case - if a type appears in an astable() expression,
							// we need to scan any of its *instance* constructors, since
							// this special function effectively allows a class to be passed around
							// as a dynamic type that can be used to instantiate instances of itself.
							if (invoke.Expression.IsLuaAsTable())
							{
								if (invoke.ArgumentList.Arguments.Count > 0 &&
									invoke.ArgumentList.Arguments[0].Expression is TypeOfExpressionSyntax typeOf)
								{
									var inner = model.GetSymbolInfo(typeOf.Type).Symbol as INamedTypeSymbol;
									if (inner != null)
									{
										set.UnionWith(ScanInstanceConstructors(inner));
									}
								}
							}
						}
						else if (child.Kind() == SyntaxKind.ObjectCreationExpression)
						{
							var create = (ObjectCreationExpressionSyntax)child;
							invokeTarget = model.GetSymbolInfo(create).Symbol;
						}

						if (null != invokeTarget)
						{
							eLuaKind = invokeTarget.GetLuaKind();
							if (eLuaKind == LuaKind.NotSpecial)
							{
								if (!invokeTarget.DeclaringSyntaxReferences.IsDefaultOrEmpty)
								{
									var inner = invokeTarget.DeclaringSyntaxReferences[0].GetSyntax();
									if (inner != node)
									{
										set.UnionWith(Scan(inner));
									}
								}
							}
						}

						var childSymbol = model.GetDeclaredSymbol(child) as ITypeSymbol;
						if (null == childSymbol)
						{
							childSymbol = model.GetTypeInfo(child).ConvertedType;
							if (null == childSymbol)
							{
								childSymbol = model.GetSymbolInfo(child).Symbol as ITypeSymbol;
							}
						}

						if (null != childSymbol)
						{
							// Skip certain symbols that can never be actual dependencies.
							if (childSymbol.TypeKind == TypeKind.TypeParameter ||
								childSymbol.TypeKind == TypeKind.Enum ||
								childSymbol.TypeKind == TypeKind.Error ||
								childSymbol.TypeKind == TypeKind.Interface ||
								childSymbol.TypeKind == TypeKind.Delegate)
							{
								continue;
							}

							// Invocations require additional handling.
							if (child.Kind() == SyntaxKind.InvocationExpression)
							{
								var invoke = (InvocationExpressionSyntax)child;

								// Special handling for require() directives.
								if (eLuaKind == LuaKind.Require)
								{
									var constant = model.GetConstantValue(invoke.ArgumentList.Arguments[0].Expression);
									if (constant.HasValue)
									{
										var sRequire = (string)constant.Value;
										deferredDep.Add((sym, sRequire));
									}

									continue;
								}
							}

							// Done if not a textual definition otherwise.
							if (childSymbol.DeclaringSyntaxReferences.IsDefaultOrEmpty)
							{
								continue;
							}

							// Skip any pseudo or special types.
							if (eLuaKind != LuaKind.NotSpecial ||
								childSymbol.IsTupleType ||
								childSymbol.IsSlimCSSystemLib() ||
								(null != childSymbol.ContainingType && childSymbol.ContainingType.IsSlimCSSystemLib()))
							{
								continue;
							}

							set.Add(childSymbol);
						}
					}

					return set;
				}

				var totalSet = new HashSet<ITypeSymbol>();

				// Setup a lookup for require directives.
				{
					var node = sym.DeclaringSyntaxReferences[0].GetSyntax();
					var sModule = Utilities.GetModulePath(node.SyntaxTree.FilePath, args.m_sInDirectory);

					HashSet<ITypeSymbol> requireSet = null;
					if (requireLookup.TryGetValue(sModule, out requireSet))
					{
						totalSet.Add(sym);
					}
					else
					{
						requireSet = new HashSet<ITypeSymbol>();
						requireSet.Add(sym);
						requireLookup[sModule] = requireSet;
					}
				}

				// Special case - if one of the placeholder classes,
				// the entire thing needs to be scanned for dependencies.
				if (sym.IsTopLevelChunkClass())
				{
					var node = sym.DeclaringSyntaxReferences[0].GetSyntax();
					totalSet.UnionWith(Scan(node));
				}
				// Enumerate members.
				else
				{
					foreach (var v in sym.GetMembers())
					{
						// Dependencies are form by static initializers
						// and static constructors.
						if (!v.IsStatic)
						{
							continue;
						}

						// Skip members without source definitions (auto generated
						// in some way).
						if (v.DeclaringSyntaxReferences.IsDefaultOrEmpty)
						{
							continue;
						}

						SyntaxNode node = null;
						switch (v.Kind)
						{
							case SymbolKind.Method:
								var methodSym = (IMethodSymbol)v;
								if (methodSym.MethodKind != MethodKind.StaticConstructor)
								{
									continue;
								}

								node = methodSym.DeclaringSyntaxReferences[0].GetSyntax();
								bStaticConstructor = true;
								break;
							case SymbolKind.Field:
								{
									var fieldSym = (IFieldSymbol)v;
									node = fieldSym.DeclaringSyntaxReferences[0].GetSyntax();
									bStaticConstructor = bStaticConstructor || !canInitInline.Contains(fieldSym);
								}
								break;
							case SymbolKind.Property:
								{
									var propertySym = (IPropertySymbol)v;
									node = propertySym.DeclaringSyntaxReferences[0].GetSyntax();
									bStaticConstructor = bStaticConstructor || (propertySym.IsAutoProperty() && !canInitInline.Contains(propertySym));
								}
								break;
						}

						// Now visit the node and accumulate any type symbols as dependencies.
						if (null != node)
						{
							totalSet.UnionWith(Scan(node));
						}
					}
				}

				typeDepend.Add(sym, (bStaticConstructor, totalSet));
			}

			// Process dependencies.
			foreach (var typeSym in toProcess)
			{
				// Add any dependent types.
				AddDependencies(typeSym);
			}

			// Now merge in any deferred.
			foreach (var defer in deferredDep)
			{
				// Skip dependencies not available - assumed to be
				// external and outside the scope of the SlimCS compiler.
				HashSet<ITypeSymbol> dependencies = null;
				if (!requireLookup.TryGetValue(defer.Item2, out dependencies))
				{
					continue;
				}

				foreach (var dep in dependencies)
				{
					typeDepend[defer.Item1].Item2.Add(dep);
				}
			}

			var typeDepOrder = new Dictionary<ITypeSymbol, (int, bool)>();

			var cyclicCheck = new HashSet<ITypeSymbol>();
			var cyclicStack = new List<ITypeSymbol>();
			int DeriveOrder(ITypeSymbol sym)
			{
				(int, bool) e;
				if (!typeDepOrder.TryGetValue(sym, out e)) { return 0; }

				int iOrder = e.Item1;
				if (iOrder >= 0)
				{
					return iOrder;
				}

				// Early out if already part of the process.
				if (cyclicCheck.Contains(sym))
				{
					// Fine if this is a self dependency.
					if (cyclicStack.Back() == sym)
					{
						return 0;
					}

					var builder = new System.Text.StringBuilder();
					foreach (var v in cyclicStack)
					{
						if (builder.Length > 0)
						{
							builder.Append(" depends on ");
						}
						builder.Append(v.ToString());
					}
					builder.Append($" depends on {sym.Name}. This is a cyclic static dependency chain.");

					throw new SlimCSCompilationException(builder.ToString());
				}

				cyclicCheck.Add(sym);
				cyclicStack.Add(sym);
				int iMax = 0;
				foreach (var dep in typeDepend[sym].Item2)
				{
					iMax = Utilities.Max(iMax, DeriveOrder(dep) + 1);
				}
				cyclicStack.PopBack();
				cyclicCheck.Remove(sym);

				typeDepOrder[sym] = (iMax, e.Item2);
				return iMax;
			}

			// Now, enumerate all type dependencies to figure out
			// ordering for static initialization.
			foreach (var e in typeDepend)
			{
				typeDepOrder.Add(e.Key, (-1, e.Value.Item1));
			}
			foreach (var e in typeDepend)
			{
				typeDepOrder[e.Key] = (DeriveOrder(e.Key), e.Value.Item1);
			}

			return typeDepOrder;
		}

		static (HashSet<ISymbol> , Dictionary<ISymbol, string>, Dictionary<ITypeSymbol, (int, bool)>) ParallelGenerate(
			CommandArgs args,
			CSharpCompilation compilation,
			List<INamedTypeSymbol> toProcess)
		{
			var canInitInline = BuildCanInitInline(args, compilation);
			Dictionary<ISymbol, string> symbolLookup = null;
			Dictionary<ITypeSymbol, (int, bool)> typeDepOrder = null;
			Parallel.Invoke(
			() =>
			{
				symbolLookup = BuildSymbolLookup(args, compilation, toProcess);
			},
			() =>
			{
				typeDepOrder = BuildTypeDep(args, compilation, toProcess, canInitInline);
			});

			return (canInitInline, symbolLookup, typeDepOrder);
		}
		#endregion

		public SharedModel(CommandArgs args, CSharpCompilation compilation)
		{
			var toProcess = BuildToProcess(args, compilation);
		
			m_Args = args;
			m_Compilation = compilation;
			(m_CanInitInline, m_SymbolLookup, m_TypeDepOrder) = ParallelGenerate(args, compilation, toProcess);
			MainMethod = compilation.GetEntryPoint(default(System.Threading.CancellationToken));
		}

		public int AdditionalInitProgress { get { return m_AdditionalInitProgress; } }

		/// <summary>
		/// Used for tracking explicit calls to __oninitprogress__ so
		/// we can accurately set the total progress.
		/// </summary>
		public void OnInitProgress()
		{
			Interlocked.Increment(ref m_AdditionalInitProgress);
		}

		internal Dictionary<ITypeSymbol, (int, bool)> TypeDepOrder
		{
			get
			{
				return m_TypeDepOrder;
			}
		}

		internal SemanticModel GetSemanticModel(SyntaxTree syntaxTree)
		{
			return m_Compilation.GetSemanticModel(syntaxTree);
		}

		public bool CanInitInline(ISymbol sym)
		{
			return m_CanInitInline.Contains(sym);
		}

		public string LookupOutputId(IMethodSymbol sym)
		{
			sym = sym.Reduce();

			string sId;
			if (!SymbolLookup.TryGetValue(sym, out sId))
			{
				sId = sym.ReducedName();
			}

			return sId;
		}
		public string LookupOutputId(INamedTypeSymbol sym)
		{
			string sId;
			if (!SymbolLookup.TryGetValue(sym, out sId))
			{
				if (!SymbolLookup.TryGetValue(sym.OriginalDefinition, out sId))
				{
					sId = sym.ReducedName();
				}
			}

			// Special handling.
			if (sym.TypeKind == TypeKind.Struct &&
				sym.ConstructedFrom.SpecialType == SpecialType.System_Nullable_T)
			{
				sId += LookupOutputId(sym.TypeArguments[0]);
			}

			return sId;
		}
		public string LookupOutputId(ISymbol sym)
		{
			switch (sym.Kind)
			{
				case SymbolKind.Method: return LookupOutputId((IMethodSymbol)sym);
				case SymbolKind.NamedType: return LookupOutputId((INamedTypeSymbol)sym);

				default:
					string sId;
					if (!SymbolLookup.TryGetValue(sym, out sId))
					{
						sId = sym.ReducedName();
					}

					return sId;
			}
		}

		public IMethodSymbol MainMethod { get; private set; }

		internal Dictionary<ISymbol, string> SymbolLookup { get { return m_SymbolLookup; } }
	}
}
