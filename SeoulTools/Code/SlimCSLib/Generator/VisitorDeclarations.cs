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
		(List<MemberDeclarationSyntax>, List<ConstructorDeclarationSyntax>, bool) GatherMembersAndConstructors(ClassDeclarationSyntax node, bool bInstance)
		{
			bool bAtLeastOneMemberMustBeInitialized = false;
			List<ConstructorDeclarationSyntax> constructors = null;
			List<MemberDeclarationSyntax> mems = null;
			foreach (var m in node.Members)
			{
				var field = (m as FieldDeclarationSyntax);
				var prop = (m as PropertyDeclarationSyntax);
				if (null != field)
				{
					if (field.Modifiers.IsInstanceMember() == bInstance)
					{
						// If supports inline initialization, don't include.
						var bCanInline = true;
						foreach (var v in field.Declaration.Variables)
						{
							if (!m_Model.CanInitInline(m_Model.GetDeclaredSymbol(v)))
							{
								bCanInline = false;
								break;
							}
						}
						if (bCanInline)
						{
							continue;
						}

						if (null == mems)
						{
							mems = new List<MemberDeclarationSyntax>();
						}
						mems.Add(field);

						// Check - used at the end to determine
						// if we can just avoid returning anything.
						if (!bAtLeastOneMemberMustBeInitialized)
						{
							bAtLeastOneMemberMustBeInitialized = !field.CanOmitDeclarationStatement(m_Model);
						}
					}
				}
				else if (null != prop)
				{
					if (prop.Modifiers.IsInstanceMember() == bInstance &&
						prop.IsAutomatic())
					{
						// If supports inline initialization, don't include.
						if (m_Model.CanInitInline(m_Model.GetDeclaredSymbol(prop)))
						{
							continue;
						}

						if (null == mems)
						{
							mems = new List<MemberDeclarationSyntax>();
						}
						mems.Add(prop);

						// Check - used at the end to determine
						// if we can just avoid returning anything.
						if (!bAtLeastOneMemberMustBeInitialized)
						{
							bAtLeastOneMemberMustBeInitialized = !prop.CanOmitDeclarationStatement(m_Model);
						}
					}
				}
				else
				{
					var constructor = (m as ConstructorDeclarationSyntax);
					if (null != constructor && constructor.Modifiers.IsInstanceMember() == bInstance)
					{
						if (null == constructors)
						{
							constructors = new List<ConstructorDeclarationSyntax>();
						}
						constructors.Add(constructor);
					}
				}
			}

			return (mems, constructors, bAtLeastOneMemberMustBeInitialized);
		}

		void AddBaseInterfaces(INamedTypeSymbol type, HashSet<INamedTypeSymbol> interfaces)
		{
			foreach (var t in type.Interfaces)
			{
				interfaces.Add(t);
				AddBaseInterfaces(t, interfaces);
			}
		}

		(INamedTypeSymbol, TypeSyntax, INamedTypeSymbol[]) GetBases(ClassDeclarationSyntax node)
		{
			if (null == node.BaseList)
			{
				return (null, null, null);
			}

			INamedTypeSymbol baseType = null;
			TypeSyntax baseClassNode = null;
			HashSet<INamedTypeSymbol> interfaces = null;
			foreach (var v in node.BaseList.Types)
			{
				var type = m_Model.GetTypeInfo(v.Type).ConvertedType as INamedTypeSymbol;
				if (type.TypeKind != TypeKind.Interface)
				{
					baseClassNode = v.Type;
					baseType = type;
				}
				else
				{
					if (null == interfaces) { interfaces = new HashSet<INamedTypeSymbol>(); }
					interfaces.Add(type);

					// We also need to emit all the parent interfaces of the type,
					// since interfaces do not exist as queryable objects at runtime.
					AddBaseInterfaces(type, interfaces);
				}
			}

			// Generate a sorted interface array.
			INamedTypeSymbol[] aInterfaces = null;
			if (null != interfaces)
			{
				aInterfaces = new INamedTypeSymbol[interfaces.Count];
				interfaces.CopyTo(aInterfaces);
				Array.Sort(aInterfaces, (a, b) =>
				{
					return a.Name.CompareTo(b.Name);
				});
			}

			return (baseType, baseClassNode, aInterfaces);
		}

		INamedTypeSymbol[] GetBases(InterfaceDeclarationSyntax node)
		{
			if (null == node.BaseList)
			{
				return null;
			}

			HashSet<INamedTypeSymbol> interfaces = null;
			foreach (var v in node.BaseList.Types)
			{
				var type = m_Model.GetTypeInfo(v.Type).ConvertedType as INamedTypeSymbol;
				if (null == interfaces) { interfaces = new HashSet<INamedTypeSymbol>(); }
				interfaces.Add(type);

				// We also need to emit all the parent interfaces of the type,
				// since interfaces do not exist as queryable objects at runtime.
				AddBaseInterfaces(type, interfaces);
			}

			// Generate a sorted interface array.
			INamedTypeSymbol[] aInterfaces = null;
			if (null != interfaces)
			{
				aInterfaces = new INamedTypeSymbol[interfaces.Count];
				interfaces.CopyTo(aInterfaces);
				Array.Sort(aInterfaces, (a, b) =>
				{
					return a.Name.CompareTo(b.Name);
				});
			}

			return aInterfaces;
		}
		#endregion

		public override SyntaxNode VisitAccessorList(AccessorListSyntax node)
		{
			foreach (var access in node.Accessors)
			{
				Visit(access);
			}
			return node;
		}

		public override SyntaxNode VisitClassDeclaration(ClassDeclarationSyntax node)
		{
			// Enforce SlimCS constraints.
			m_Model.Enforce(node);

			// Scope the new type.
			using (var scope = new Scope(this, ScopeKind.Type, node))
			{
				// Get the id.
				var sId = m_Model.LookupOutputId(node);
				var sOrigId = node.Identifier.ValueText;

				// Get base class and interfaces.
				(var baseClassType, var baseClassNode, var interfaces) = GetBases(node);

				// Determine whether or not we need a short original name.
				var bOrigName = (sOrigId != sId);

				// Determine whether or not we need a qualified name.
				// Need to qualify if the name we're using to
				// identify the type is not the same as
				// the fully qualified name (what you get
				// if you call ToString() on an instance of the type
				// that doesn't explicitly override ToString()).
				var bQualifiedName = false;
				var namedType = m_Model.GetDeclaredSymbol(node);
				if ((namedType.ContainingNamespace != null && !namedType.ContainingNamespace.IsGlobalNamespace) ||
					namedType.ContainingType != null ||
					namedType.Name != sId)
				{
					bQualifiedName = true;
				}

				// No parens to the class call if only 1 argument,
				// since the argument is a string.
				var bOneArgument = (null == baseClassType && !bQualifiedName && null == interfaces && !bOrigName);

				// Emit a local variable that shadows the class for the file
				// it is defined in.
				//
				// Skip this emit if the class is empty - this helps to avoid
				// a few cases of hitting the local variable limit in Lua
				// (200), for files with a large number of stub classes.
				if (node.Members.Count > 0)
				{
					Write(kLuaLocal); Write(' '); Write(sId); Write(" = ");
				}

				// Start the type - one of two functions depending on
				// whether the class if static.
				var bStatic = node.Modifiers.HasToken(SyntaxKind.StaticKeyword);
				if (bStatic) { Write(kLuaClassStatic); }
				else { Write(kLuaClass); }

				// Open paren or space delimited if we're omitting the parens.
				if (bOneArgument) { Write(' '); }
				else { Write('('); }

				// Class id as a string.
				WriteStringLiteral(sId);

				// Now base and qualification, if needed.
				int iOptionals = 0;
				{
					if (null != baseClassType)
					{
						++iOptionals;

						// Generics we need to use a lazy lookup.
						if (baseClassType.IsGenericType)
						{
							Write(',');
							Visit(baseClassNode);
						}
						else
						{
							var sBaseId = m_Model.LookupOutputId(baseClassType);
							Write(", ");
							WriteStringLiteral(sBaseId);
						}
					}

					// If the fully qualified C# name will differ from sId,
					// we need to write the fully qualified name as a third
					// argument so that class() can implement the default __tostring()
					// properly.
					if (bQualifiedName)
					{
						while (iOptionals < 1)
						{
							Write(", nil");
							++iOptionals;
						}

						++iOptionals;
						Write(", ");
						Write('\'');
						WriteTypeScope('+', true);
						Write('\'');
					}

					// If short original name, write that now.
					if (bOrigName)
					{
						while (iOptionals < 2)
						{
							Write(", nil");
							++iOptionals;
						}

						++iOptionals;
						Write(", ");
						WriteStringLiteral(sOrigId);
					}
				}

				// Finally interfaces, if defined.
				if (null != interfaces)
				{
					// Need to emit placeholders if not defined.
					while (iOptionals < 3)
					{
						Write(", nil");
						++iOptionals;
					}

					// Now write the interfaces as string literals.
					foreach (var e in interfaces)
					{
						Write(", ");
						WriteStringLiteral(m_Model.LookupOutputId(e));
					}
				}

				// Terminate call if we didn't omit parens.
				if (!bOneArgument) { Write(')'); }

				// Now members and anything special.

				// first, gather any instance fields and any constructors.
				// These must be converted into assignments
				// and added to the constructors (or we need to generate a default
				// constructor if the class has non otherwise).
				(var instanceMems, var instanceConstructors, var bInstanceAtLeastOne) = GatherMembersAndConstructors(node, true);
				var bInstanceMembers = (null != instanceMems);
				(var staticMems, var staticConstructors, var bStaticAtLeastOne) = GatherMembersAndConstructors(node, false);
				var bStaticMembers = (null != staticMems);
				var visited = new HashSet<MemberDeclarationSyntax>();

				// First, pre-declare any statics that cannot be emitted inline
				// (these will be hoisted into a static constructor instead).
				if (null != staticMems)
				{
					using (var fixedLine = new FixedLine(this, node))
					{
						foreach (var mem in staticMems)
						{
							// Always visited - will only be emitted as part of a later
							// constructor.
							visited.Add(mem);

							// Only need to pre-declare static members
							// that can be converted to locals (private members).
							if (!CanEmitAsLocal(mem, out _))
							{
								continue;
							}

							// Always a field or a prop - if a prop, always an auto prop.
							if (mem is FieldDeclarationSyntax field)
							{
								foreach (var v in field.Declaration.Variables)
								{
									// Declaration only.
									Write("; ");
									Write(kLuaLocal);
									Write(' ');
									WriteIdWithDedup(v);

									// Also need the initializer in this case.
									if (m_Model.CanInitInline(m_Model.GetDeclaredSymbol(v)))
									{
										if (null == v.Initializer)
										{
											WriteDefaultValue(field.Declaration.Type);
										}
										else
										{
											var eInitKind = v.Initializer.Kind();
											if (eInitKind == SyntaxKind.EqualsValueClause)
											{
												Visit(((EqualsValueClauseSyntax)v.Initializer).Value);
											}
											else
											{
												Visit(v.Initializer);
											}
										}
									}
								}
							}
							else
							{
								var prop = (PropertyDeclarationSyntax)mem;

								Write("; ");
								Write(kLuaLocal);
								Write(' ');
								WriteIdWithDedup(prop);
							}

							// Now declared.
							OnDeclare(mem);
						}
					}
				}

				// members
				foreach (var m in node.Members)
				{
					// If we have fields and hit a constructor, emit the constructor
					// with the members.
					if (bInstanceMembers)
					{
						if (m is ConstructorDeclarationSyntax constructor)
						{
							if (constructor.Modifiers.IsInstanceMember())
							{
								VisitConstructorDeclaration(constructor, instanceMems, true);
								continue;
							}
						}
						// Otherwise, if we have no constructors and we've reached the last member,
						// generate a constructor and initialize the members.
						else if (null == instanceConstructors && instanceMems.Back() == m)
						{
							// We can skip the actual emit here if
							// none of the members need to be initialized,
							// since we're only generating the constructor
							// in this case to initialize instance members.
							if (bInstanceAtLeastOne)
							{
								VisitConstructorDeclaration(null, instanceMems, true);
							}
							continue;
						}
						// Final case - if we're at a field or auto property that is in the mems
						// list, skip it.
						//
						// It will be emitted with the final member or with explicitly defined
						// constructors.
						else if (m is FieldDeclarationSyntax)
						{
							var field = (FieldDeclarationSyntax)m;
							if (field.Modifiers.IsInstanceMember())
							{
								continue;
							}
						}
						else if (m is PropertyDeclarationSyntax)
						{
							var prop = (PropertyDeclarationSyntax)m;
							if (prop.Modifiers.IsInstanceMember() &&
								prop.IsAutomatic())
							{
								continue;
							}
						}
					}

					// If we have static fields and hit a static constructor, emit the constructor
					// with the members.
					if (bStaticMembers)
					{
						if (m is ConstructorDeclarationSyntax constructor)
						{
							if (!constructor.Modifiers.IsInstanceMember())
							{
								VisitConstructorDeclaration(constructor, staticMems, false);
								continue;
							}
						}
						else if (null == staticConstructors && staticMems.Back() == m)
						{
							// We can skip the actual emit here if
							// none of the members need to be initialized,
							// since we're only generating the constructor
							// in this case to initialize instance members.
							if (bStaticAtLeastOne)
							{
								VisitConstructorDeclaration(null, staticMems, false);
							}
							continue;
						}
					}

					// Filter pure abstract/extern method declarations.
					if (m is BaseMethodDeclarationSyntax method)
					{
						if (method.Modifiers.IsAbstractOrExtern())
						{
							continue;
						}
					}
					// Filter pure abstract/extern property declarations.
					else if (m is PropertyDeclarationSyntax prop)
					{
						if (prop.Modifiers.IsAbstractOrExtern())
						{
							continue;
						}
					}
					// Filter pure abstract/extern indexer declarations.
					else if (m is IndexerDeclarationSyntax idx)
					{
						if (idx.Modifiers.IsAbstractOrExtern())
						{
							continue;
						}
					}

					if (!visited.Contains(m))
					{
						visited.Add(m);
						Visit(m);
					}
				}

				// finally, special case - synthesized auto properties.
				// only applies to instance member properties that
				// implement a base or interface. Always out of line
				// (no definition in text), so just emit last.
				foreach (var m in node.Members)
				{
					var prop = m as PropertyDeclarationSyntax;
					if (null == prop) { continue; }

					if (!prop.Modifiers.IsInstanceMember() ||
						!prop.IsAutomatic() ||
						!m_Model.ShouldSynthesizeAccessors(prop))
					{
						continue;
					}

					// Synthesize any accessor that has a null body.
					foreach (var accessor in prop.AccessorList.Accessors)
					{
						if (accessor.Body == null && accessor.ExpressionBody == null)
						{
							Write("; ");
							VisitAccessorDeclaration(accessor);
						}
					}
				}
			}

			return node;
		}

		public override SyntaxNode VisitDeclarationExpression(DeclarationExpressionSyntax node)
		{
			if (node.Designation is ParenthesizedVariableDesignationSyntax variableDesignation)
			{
				var eParentKind = node.Parent.Kind();
				var vars = variableDesignation.Variables;

				// Note that we exclude the local keyword if we're in the
				// initializer block of a foreach loop, since the equivalent
				// lua for k,v in does not use it in that context.
				if (eParentKind != SyntaxKind.ForEachVariableStatement &&
					vars.Count > 0)
				{
					Write(kLuaLocal);
					Write(' ');
				}

				// TODO: Just removing the parens is not always correct,
				// sometimes need to collapse into a table if the assignment is to a single
				// variable.
				for (int i = 0; i < vars.Count; ++i)
				{
					if (0 != i)
					{
						Write(", ");
					}

					var desig = vars[i];
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
			}
			else
			{
				throw new UnsupportedNodeException(node, $"Unsupported DeclarationExpression {node.Designation.Kind()}");
			}

			return node;
		}

		public override SyntaxNode VisitDelegateDeclaration(DelegateDeclarationSyntax node)
		{
			m_Model.Enforce(node);

			// TODO: A delegate will need to turn into an actual thing
			// in the following cases:
			// - if the function being bound is an instance function - the delegate will
			//   need to close around self and invoke the bound function appropriately.
			// - if the delegate has optional arguments specified - it will need to be
			//   a function closure that calls the wrapped function with the default
			//   arguments.
			// - like several other cases (Enum), we may need an object to represent
			//   some amount of RTTI.

			return node;
		}

		public override SyntaxNode VisitEnumDeclaration(EnumDeclarationSyntax node)
		{
			// Enforce SlimCS constraints.
			m_Model.Enforce(node);

			// Start the type.
			Write(m_Model.LookupOutputId(node));
			Write(" = {");

			// Now members and anything special.
			using (var scope = new Scope(this, ScopeKind.Type, node))
			{
				// members
				int iValue = 0;
				for (int i = 0; i < node.Members.Count; ++i)
				{
					// Cache.
					var m = node.Members[i];

					// Enforce SlimCS constraints.
					m_Model.Enforce(m);

					// Comma delimit.
					if (0 != i)
					{
						Write(',');
					}

					// Now emit the declaration.
					SeparateForFirst(m);
					Write(m.Identifier);
					Write(" = ");
					if (null != m.EqualsValue && null != m.EqualsValue.Value)
					{
						var v = m.EqualsValue.Value;
						var constant = m_Model.GetConstantValue(v);
						if (!constant.HasValue || !(constant.Value is int))
						{
							throw new UnsupportedNodeException(v, "enum values must be constant integer values.");
						}
						iValue = (int)constant.Value;
					}
					Write(iValue.ToString());

					// Advance to the next index.
					++iValue;
				}

				// Terminate the enum.
				SeparateForLast(node);
				Write('}');
			}

			return node;
		}

		public override SyntaxNode VisitEventDeclaration(EventDeclarationSyntax node)
		{
			m_Model.Enforce(node);
			Visit(node.AccessorList);
			return node;
		}

		public override SyntaxNode VisitEventFieldDeclaration(EventFieldDeclarationSyntax node)
		{
			// Enforce SlimCS constraints.
			m_Model.Enforce(node);

			// Get important modifiers.
			var modifiers = node.Modifiers;

			// Write the prefix for a member's identifier -
			// either a local variable definition, or
			// a path into the type nested of this member.
			(var sSpecifier, var _) = GetMemberIdentifierSpecifier(node, node.Modifiers, node.Kind());
			Write(sSpecifier);

			// Now emit the declaration.
			Visit(node.Declaration);

			return node;
		}

		public override SyntaxNode VisitFieldDeclaration(FieldDeclarationSyntax node)
		{
			// Enforce SlimCS constraints.
			m_Model.Enforce(node);

			// Early out if can be omitted.
			if (node.CanOmitDeclarationStatement(m_Model) &&
				!CanEmitAsLocal(node, out _))
			{
				return node;
			}

			// Get important modifiers.
			var modifiers = node.Modifiers;

			// Now emit the declaration.
			(var sPrefix, var eType) = GetMemberIdentifierSpecifier(node, node.Modifiers, node.Kind());
			VisitVariableDeclaration(node.Declaration, sPrefix, eType);

			// If valid, mark as declared.
			OnDeclare(node);

			return node;
		}

		public override SyntaxNode VisitIndexerDeclaration(IndexerDeclarationSyntax node)
		{
			m_Model.Enforce(node);
			Visit(node.AccessorList);
			return node;
		}

		public override SyntaxNode VisitInterfaceDeclaration(InterfaceDeclarationSyntax node)
		{
			// Enforce SlimCS constraints.
			m_Model.Enforce(node);

			// Scope the new type.
			using (var scope = new Scope(this, ScopeKind.Type, node))
			{
				// Get the id.
				var sId = m_Model.LookupOutputId(node);
				var sOrigId = node.Identifier.ValueText;

				// Determine whether or not we need a short original name.
				var bOrigName = (sOrigId != sId);

				// Get base class and interfaces.
				var interfaces = GetBases(node);

				// Determine whether or not we need a qualified name.
				// Need to qualify if the name we're using to
				// identify the type is not the same as
				// the fully qualified name (what you get
				// if you call ToString() on an instance of the type
				// that doesn't explicitly override ToString()).
				var bQualifiedName = false;
				var namedType = m_Model.GetDeclaredSymbol(node);
				if ((namedType.ContainingNamespace != null && !namedType.ContainingNamespace.IsGlobalNamespace) ||
					namedType.ContainingType != null ||
					namedType.Name != sId)
				{
					bQualifiedName = true;
				}

				// No parens to the class call if only 1 argument,
				// since the argument is a string.
				var bOneArgument = (!bQualifiedName && null == interfaces && !bOrigName);

				// Start the type.
				Write(kLuaInterface);

				// Open paren or space delimited if we're omitting the parens.
				if (bOneArgument) { Write(' '); }
				else { Write('('); }

				// Class id as a string.
				WriteStringLiteral(sId);

				// Now qualification, if needed.
				{
					// If the fully qualified C# name will differ from sId,
					// we need to write the fully qualified name as a second
					// argument so that interface() can implement the default __tostring()
					// properly.
					if (bQualifiedName)
					{
						Write(", ");
						Write('\'');
						WriteTypeScope('+', true);
						Write('\'');
					}

					// If short original name, write that now.
					if (bOrigName)
					{
						if (!bQualifiedName)
						{
							Write(", nil");
						}

						Write(", ");
						WriteStringLiteral(sOrigId);
					}
				}

				// Finally interfaces, if defined.
				if (null != interfaces)
				{
					// Need to emit placeholders if not defined.
					if (bOrigName)
					{
						if (!bQualifiedName)
						{
							Write(", nil");
						}
					}
					else
					{
						if (!bQualifiedName)
						{
							Write(", nil, nil");
						}
						else
						{
							Write(", nil");
						}
					}

					// Now write the interfaces as string literals.
					foreach (var e in interfaces)
					{
						Write(", ");
						WriteStringLiteral(m_Model.LookupOutputId(e));
					}
				}

				// Terminate call if we didn't omit parens.
				if (!bOneArgument) { Write(')'); }
			}

			return node;
		}

		public override SyntaxNode VisitNamespaceDeclaration(NamespaceDeclarationSyntax node)
		{
			foreach (var mem in node.Members)
			{
				Visit(mem);
			}

			return node;
		}

		public override SyntaxNode VisitPropertyDeclaration(PropertyDeclarationSyntax node)
		{
			m_Model.Enforce(node);

			// Automatic properties just become fields.
			if (node.IsAutomatic())
			{
				// Get important modifiers.
				var modifiers = node.Modifiers;

				// Write the prefix for a member's identifier -
				// either a local variable definition, or
				// a path into the type nested of this member.
				WriteMemberIdentifierSpecifier(node, node.Modifiers, node.Kind());

				// Now write the identifier.
				WriteIdWithDedup(node);

				// Now emit the declaration.
				var init = node.Initializer;
				if (null != init)
				{
					Write(" =");
					Visit(init.Value);
				}
				// Add an explicit initializer to make sure the default value
				// is reasonable.
				else
				{
					Write(" = ");
					WriteDefaultValue(node.Type);
				}
			}
			else
			{
				Visit(node.AccessorList);
			}

			return node;
		}

		void VisitVariableDeclaration(
			VariableDeclarationSyntax node,
			string sPrefix = "",
			MemberIdentifierSpecifierType eType = MemberIdentifierSpecifierType.None)
		{
			var vars = node.Variables;

			// If we're in an instance setting context, we can omit variables that are
			// otherwise written to within the same context (this avoids redundantly
			// initializing a variable to a default value when it's already
			// explicitly initialized).
			List<bool> omitted = null;
			if (eType == MemberIdentifierSpecifierType.Instance)
			{
				var written = (m_BlockScope.Count == 0 ? null : m_BlockScope.Back().m_ExtraWrite);
				if (null != written)
				{
					var bAllOmitted = true;
					for (int i = 0; i < vars.Count; ++i)
					{
						var sym = m_Model.GetDeclaredSymbol(vars[i]);
						if (written.Contains(sym))
						{
							if (null == omitted) { omitted = new List<bool>(); }
							while (omitted.Count < i)
							{
								omitted.Add(false);
							}
							omitted.Add(true);
						}
						else
						{
							bAllOmitted = false;
						}
					}

					// Nothing to write.
					if (bAllOmitted)
					{
						return;
					}
				}
			}

			bool bHasInitializers = false;
			for (int i = 0; i < vars.Count; ++i)
			{
				// Skip ommitted vars.
				if (null != omitted && omitted[i])
				{
					continue;
				}

				var v = vars[i];
				if (0 != i)
				{
					Write(", ");
				}

				if (NeedsWhitespaceSeparation) { Write(' '); }

				// Emit the variable prefix if one was defined.
				if (!string.IsNullOrEmpty(sPrefix))
				{
					if (0 == i || eType != MemberIdentifierSpecifierType.Local)
					{
						Write(sPrefix);
					}
				}

				WriteIdWithDedup(v);
				bHasInitializers = bHasInitializers || (null != v.Initializer);
			}

			// Includes explicit initialization.
			if (bHasInitializers)
			{
				Write(" =");
				for (int i = 0; i < vars.Count; ++i)
				{
					// Skip ommitted vars.
					if (null != omitted && omitted[i])
					{
						continue;
					}

					var v = vars[i];
					if (0 != i)
					{
						Write(", ");
					}

					if (null == v.Initializer)
					{
						WriteDefaultValue(node.Type);
					}
					else
					{
						var eInitKind = v.Initializer.Kind();
						if (eInitKind == SyntaxKind.EqualsValueClause)
						{
							Visit(((EqualsValueClauseSyntax)v.Initializer).Value);
						}
						else
						{
							Visit(v.Initializer);
						}
					}
				}
			}
			// Add an explicit initializer to make sure the default value
			// is reasonable.
			else
			{
				Write(" = ");
				for (int i = 0; i < vars.Count; ++i)
				{
					if (0 != i) { Write(", "); }
					WriteDefaultValue(node.Type);
				}
			}
		}

		public override SyntaxNode VisitVariableDeclaration(VariableDeclarationSyntax node)
		{
			VisitVariableDeclaration(node, null);
			return node;
		}
	}
}
