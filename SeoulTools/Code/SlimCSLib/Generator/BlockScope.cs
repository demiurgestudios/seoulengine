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

namespace SlimCSLib.Generator
{
	[Flags]
	public enum TryCatchFinallyUsingControlOption
	{
		None = 0,
		Break = (1 << 0),
		Continue = (1 << 1),
		Return = (1 << 2),
		ReturnMultiple = (1 << 3),
	}

	/// <summary>
	/// Defines values and state per scope frame of the current stack
	/// while the Visitor is traversing an AST.
	/// </summary>
	public sealed class BlockScope
	{
		#region Private members
		Dictionary<object, string> m_SwitchLabels;
		Dictionary<SwitchSectionSyntax, string> m_SwitchSections;
		HashSet<string> m_UtilityGotoLabels;
		#endregion

		public BlockScope(
			CSharpSyntaxNode site,
			ScopeKind eKind,
			DataFlowAnalysis flow,
			HashSet<string> globals,
			HashSet<string> extraRead,
			HashSet<ISymbol> extraWrite)
		{
			m_Site = site;
			m_eKind = eKind;
			m_Flow = flow;
			m_Globals = globals;
			m_DedupId = null;
			m_DedupSym = null;
			m_ExtraRead = extraRead;
			m_ExtraWrite = extraWrite;
		}

		public readonly HashSet<string> m_ExtraRead;
		public readonly HashSet<ISymbol> m_ExtraWrite;
		public readonly CSharpSyntaxNode m_Site;
		public readonly ScopeKind m_eKind;
		public readonly DataFlowAnalysis m_Flow;
		public readonly HashSet<string> m_Globals;
		public Dictionary<string, string> m_DedupId;
		public Dictionary<ISymbol, string> m_DedupSym;
		public string m_sContinueLabel = null;
		public TryCatchFinallyUsingControlOption m_eControlOptions;

		// This symbol is set by assignment contexts to indicate
		// that the corresponding property, event, or indexer should
		// emit a setter form instead of a getter form.
		public IMethodSymbol m_LHS = null;

		public bool HasUtilityGotoLabels { get { return (null != m_UtilityGotoLabels); } }

		public class LabelComparer : IEqualityComparer<object>
		{
			bool IEqualityComparer<object>.Equals(object a, object b)
			{
				// Coerce for more flexible comparisons.
				a = (a is char ? (int)((char)a) : a);
				b = (b is char ? (int)((char)b) : b);
				return a.Equals(b);
			}

			public int GetHashCode(object obj)
			{
				obj = (obj is char ? (int)((char)obj) : obj);
				return obj.GetHashCode();
			}
		}

		public Dictionary<object, string> SwitchLabels
		{
			get
			{
				if (null == m_SwitchLabels)
				{
					m_SwitchLabels = new Dictionary<object, string>(new LabelComparer());
				}
				return m_SwitchLabels;
			}
		}

		public Dictionary<SwitchSectionSyntax, string> SwitchSections
		{
			get
			{
				if (null == m_SwitchSections)
				{
					m_SwitchSections = new Dictionary<SwitchSectionSyntax, string>();
				}
				return m_SwitchSections;
			}
		}

		public HashSet<string> UtilityGotoLabels
		{
			get
			{
				if (null == m_UtilityGotoLabels)
				{
					m_UtilityGotoLabels = new HashSet<string>();
				}
				return m_UtilityGotoLabels;
			}
		}
	}
}
