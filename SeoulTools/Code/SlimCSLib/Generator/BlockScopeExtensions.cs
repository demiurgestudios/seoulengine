// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using System.Collections.Generic;
using System.Collections.Immutable;
using static SlimCSLib.Generator.Constants;

namespace SlimCSLib.Generator
{
	/// <summary>
	/// Extension methods specific to the BlockScope type
	/// and its containers.
	/// </summary>
	public static class BlockScopeExtensions
	{
		public static bool Collides(this List<BlockScope> e, string sId, bool bCheckAllDefined)
		{
			// First level, check remap and types. Only check
			// defined variables if requested.
			int i = e.Count - 1;
			if (i >= 0)
			{
				var v = e[i];
				if (v.m_DedupId != null && v.m_DedupId.ContainsKey(sId))
				{
					return true;
				}
				if (v.m_Globals.Contains(sId))
				{
					return true;
				}
				if (bCheckAllDefined)
				{
					int iIndex = v.m_Flow.VariablesDeclared.IndexOf(sId);
					if (iIndex >= 0)
					{
						return true;
					}
				}
			}

			// All remaining levels, only check remap and defined variables.
			while (--i >= 0)
			{
				var v = e[i];
				if (v.m_DedupId != null && v.m_DedupId.ContainsKey(sId))
				{
					return true;
				}

				int iIndex = v.m_Flow.VariablesDeclared.IndexOf(sId);
				if (iIndex >= 0)
				{
					return true;
				}
			}

			// Final check, check for collision with Lua reserved words.
			if (kLuaReserved.Contains(sId))
			{
				return true;
			}

			// Done, no collision.
			return false;
		}

		public static bool CollidesWithType(this List<BlockScope> e, ISymbol sym)
		{
			var sId = sym.Name;

			// First level, check remap and types but not defined variables.
			int i = e.Count - 1;
			if (i >= 0)
			{
				var v = e[i];
				if (v.m_Globals.Contains(sId))
				{
					return true;
				}
			}

			// Done, no collision.
			return false;
		}

		public static bool Dedup(this List<BlockScope> e, ISymbol sym, ref string rsId)
		{
			// Check for a remap.
			for (int i = e.Count - 1; i >= 0; --i)
			{
				var v = e[i];
				if (v.m_DedupSym == null)
				{
					continue;
				}

				if (v.m_DedupSym.TryGetValue(sym, out rsId))
				{
					return true;
				}
			}

			return false;
		}

		static void DedupTopVar(this List<BlockScope> e, ISymbol v)
		{
			var top = e.Back();

			// Initial check - pass sym so we
			// can avoid checks against redundant symbol entries.
			if (!e.CollidesWithType(v) &&
				!kLuaReserved.Contains(v.Name))
			{
				return;
			}

			var sDedup = e.GenDedupedId(v.Name, false);

			// Need to insert the remap.
			if (sDedup != v.Name)
			{
				if (null == top.m_DedupSym)
				{
					top.m_DedupSym = new Dictionary<ISymbol, string>();
				}
				top.m_DedupSym.Add(v, sDedup);
			}
		}

		public static void DedupTop(this List<BlockScope> e)
		{
			var top = e.Back();
			var flow = top.m_Flow;

			// Skip if no declared variables.
			if (!flow.VariablesDeclared.IsEmpty)
			{
				// Enumerate, check for collisions, and generate
				// a new name as needed.
				foreach (var v in flow.VariablesDeclared)
				{
					e.DedupTopVar(v);
				}
			}

			// Skip if no declared variables.
			if (!flow.DataFlowsIn.IsEmpty)
			{
				// Enumerate, check for collisions, and generate
				// a new name as needed.
				foreach (var v in flow.DataFlowsIn)
				{
					// Only interested in parameters in this case.
					if (v.Kind != SymbolKind.Parameter)
					{
						continue;
					}

					e.DedupTopVar(v);
				}
			}
		}

		public static string GenDedupedId(this List<BlockScope> e, string sInitialId, bool bCheckAllDefined)
		{
			var top = e.Back();

			int iDedup = 0;
			var sDedup = sInitialId;
			while (e.Collides(sDedup, bCheckAllDefined))
			{
				sDedup = sInitialId + iDedup.ToString();
				++iDedup;
			}

			// Need to insert the remap.
			if (iDedup > 0)
			{
				if (null == top.m_DedupId)
				{
					top.m_DedupId = new Dictionary<string, string>();
				}
				top.m_DedupId.Add(sInitialId, sDedup);
			}

			return sDedup;
		}

		public static (HashSet<string>, BlockScope) GetGotoLabelsInScope(this List<BlockScope> e)
		{
			var ret = new HashSet<string>();

			// Find the nearest function context.
			int iContext = e.Count - 1;
			while (iContext >= 0)
			{
				// Once we get to the first function scope, break.
				var scope = e[iContext];
				if (scope.m_eKind == ScopeKind.Function ||
					scope.m_eKind == ScopeKind.Lambda ||
					scope.m_eKind == ScopeKind.TopLevelChunk)
				{
					break;
				}

				// Advance.
				--iContext;
			}

			var funcScope = e[iContext];

			// Find all label statements defined in the body of the function.
			foreach (var node in funcScope.m_Site.DescendantNodes())
			{
				if (node is LabeledStatementSyntax)
				{
					ret.Add(((LabeledStatementSyntax)node).Identifier.ValueText);
				}
			}

			// Append any defined labels.
			if (funcScope.HasUtilityGotoLabels)
			{
				ret.UnionWith(funcScope.UtilityGotoLabels);
			}

			return (ret, funcScope);
		}

		public static int IndexOf(this ImmutableArray<ISymbol> list, string sName)
		{
			int iLength = list.Length;
			for (int i = 0; i < iLength; ++i)
			{
				var v = list[i];
				if (v.Name == sName)
				{
					return i;
				}
			}

			return -1;
		}

		public static ITypeSymbol ReturnType(this List<BlockScope> e, Model model)
		{
			int iCount = e.Count;
			for (int i = iCount - 1; i >= 0; --i)
			{
				var scope = e[i];
				if (scope.m_eKind == ScopeKind.Function ||
					scope.m_eKind == ScopeKind.Lambda ||
					scope.m_eKind == ScopeKind.TopLevelChunk)
				{
					var node = scope.m_Site.Parent;
					var sym = model.GetDeclaredSymbol(node);
					if (null == sym && scope.m_eKind == ScopeKind.Lambda)
					{
						var symInfo = model.GetSymbolInfo(node);
						sym = symInfo.Symbol;
					}
					var methodSym = sym as IMethodSymbol;
					if (null != methodSym)
					{
						return methodSym.ReturnType;
					}
					else
					{
						return null;
					}
				}
			}

			return null;
		}
	}
}
