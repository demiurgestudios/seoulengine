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
using System.Globalization;
using System.IO;
using static SlimCSLib.Generator.Constants;

namespace SlimCSLib.Generator
{
	public sealed partial class Visitor : CSharpSyntaxVisitor<SyntaxNode>, IDisposable
	{
		sealed class FixedLine : IDisposable
		{
			readonly int m_iLineCheck;
			readonly Visitor m_Owner;
			readonly CSharpSyntaxNode m_Site;

			public FixedLine(Visitor owner, CSharpSyntaxNode site)
			{
				m_iLineCheck = owner.m_iOutputLine;
				m_Owner = owner;
				m_Site = site;

				++m_Owner.m_iFixedOutputLine;
			}

			public void Dispose()
			{
				--m_Owner.m_iFixedOutputLine;

				// Sanity check - if iLineCheck is not the same as m_iOutputLine,
				// some code allowed a line change when it wasn't supposed to.
				if (m_iLineCheck != m_Owner.m_iOutputLine)
				{
					throw new SlimCSCompilationException(
						m_Site,
						$"fixed line changed from '{m_iLineCheck}' to '{m_Owner.m_iOutputLine}', this is an internal compiler error.");
				}
			}
		}

		sealed class OutputLock : IDisposable
		{
			readonly Visitor m_Owner;

			public OutputLock(Visitor owner)
			{
				m_Owner = owner;
				++m_Owner.m_iOutputLock;
			}

			public void Dispose()
			{
				--m_Owner.m_iOutputLock;
			}
		}

		class IndentData
		{
			public IndentData(int iIndent, int iAdditional)
			{
				m_iIndent = iIndent;
				m_iAdditional = iAdditional;
				m_bInStatement = false;
				m_bWantIndentStatement = false;
				m_bDidIndentStatement = false;
			}

			public int FullIndent
			{
				get
				{
					return (m_iIndent + (m_bWantIndentStatement ? 1 : 0));
				}
			}

			public void StatementStart()
			{
				m_bInStatement = true;
			}

			public void OnApplyIndent()
			{
				if (m_bInStatement)
				{
					if (m_bWantIndentStatement)
					{
						m_bDidIndentStatement = true;
					}
				}
			}

			public void OnNewline()
			{
				if (m_bInStatement)
				{
					m_bWantIndentStatement = true;
				}
			}

			public void StatementEnd()
			{
				m_bDidIndentStatement = m_bWantIndentStatement = m_bInStatement = false;
			}

			public int m_iIndent = 0;
			public int m_iAdditional = 0;
			public bool m_bInStatement = false;
			public bool m_bIndentStatement = false;
			public bool m_bWantIndentStatement = false;
			public bool m_bDidIndentStatement = false;
		}

		readonly HashSet<ISymbol> m_Vargs = new HashSet<ISymbol>();
		readonly HashSet<SyntaxTrivia> m_CommentSet = new HashSet<SyntaxTrivia>();
		readonly List<SyntaxTrivia> m_Comments = new List<SyntaxTrivia>();
		readonly TextWriter m_Writer;
		int m_iComment = 0;
		readonly List<IndentData> m_Indent = new List<IndentData>();
		int m_iOutputLine = 0;
		// When non-zero, the writer stays at the current output line,
		// ignoring any line hints from the input nodes.
		int m_iFixedOutputLine = 0;
		int m_iIndentStackCountAtNewline = 0;
		bool m_bAtLineStart = true;
		char m_LastChar = '\0';
		char m_LastCharMinus1 = '\0';
		int m_iCommentWritingSuppressed;
		int m_iConstantEmitSuppressed;
		int m_iOutputLock;
		int m_iWriteOffset;

		void CheckOnFirst()
		{
			// Writing disabled.
			if (OutputLocked) { return; }

			if (!m_bAtLineStart)
			{
				return;
			}

			m_bAtLineStart = false;

			var iIndent = CurrentIndent;
			for (int i = 0; i < iIndent; ++i)
			{
				m_Writer.Write('\t');
				m_LastCharMinus1 = m_LastChar;
				m_LastChar = '\t';
				++m_iWriteOffset;
			}

			var iAdditional = CurrentAdditionalIndentSpaces;
			for (int i = 0; i < iAdditional; ++i)
			{
				m_Writer.Write(' ');
				m_LastCharMinus1 = m_LastChar;
				m_LastChar = ' ';
				++m_iWriteOffset;
			}

			m_Indent.Back().OnApplyIndent();
		}

		static int CountNewlines(string s)
		{
			int iReturn = 0;
			foreach (var ch in s)
			{
				if ('\n' == ch)
				{
					++iReturn;
				}
			}

			return iReturn;
		}

		bool CountNewlinesAndAddToOutputCount(string s)
		{
			var iCount = CountNewlines(s);
			if (iCount > 0)
			{
				m_iOutputLine += iCount;
				return true;
			}

			return false;
		}

		int CurrentAdditionalIndentSpaces
		{
			get
			{
				return m_Indent.Back().m_iAdditional;
			}
		}

		int CurrentIndent
		{
			get
			{
				return m_Indent.Back().FullIndent;
			}
		}

		bool FixedAtCurrentLine
		{
			get
			{
				return (0 != m_iFixedOutputLine);
			}
		}

		void GatherComments(SyntaxNode root)
		{
			foreach (var trivia in root.DescendantTrivia((node) => { return true; }, true))
			{
				var eKind = trivia.Kind();
				if (!eKind.IsComment())
				{
					continue;
				}

				if (m_CommentSet.Contains(trivia))
				{
					continue;
				}

				m_Comments.Add(trivia);
				m_CommentSet.Add(trivia);
			}

			m_Comments.Sort((a, b) =>
			{
				return a.SpanStart.CompareTo(b.SpanStart);
			});
		}

		void Indent()
		{
			if (!m_Indent.Back().m_bDidIndentStatement)
			{
				m_Indent.Back().m_bWantIndentStatement = false;
			}
			m_Indent.Add(new IndentData(CurrentIndent + 1, CurrentAdditionalIndentSpaces));
		}

		/// <summary>
		/// Query the current data flow analysis to determine if a variable is in the used
		/// set.
		/// </summary>
		bool IsUsed(string s)
		{
			// No used variables, not used.
			if (m_BlockScope.Count == 0)
			{
				return false;
			}

			var back = m_BlockScope.Back();

			// Check extra set if defined.
			if (null != back.m_ExtraRead)
			{
				if (back.m_ExtraRead.Contains(s))
				{
					return true;
				}
			}

			// Check if in the used set.
			foreach (var sym in back.m_Flow.ReadInside)
			{
				if (sym.Name == s)
				{
					return true;
				}
			}

			return false;
		}

		void AddVararg(ParameterSyntax node)
		{
			var sym = m_Model.GetDeclaredSymbol(node);
			m_Vargs.Add(sym);
		}

		void RemoveVararg(ParameterSyntax node)
		{
			var sym = m_Model.GetDeclaredSymbol(node);
			m_Vargs.Remove(sym);
		}

		/// <summary>
		/// Used for filtering tokens from their C# name to '...'.
		/// </summary>
		/// <param name="ident">The ident to filter.</param>
		/// <returns>true if ident is a vararg variable in the current context, false otherwise.</returns>
		bool IsVararg(SimpleNameSyntax node)
		{
			var sym = m_Model.GetSymbolInfo(node).Symbol;
			if (null == sym) { return false; }
			return m_Vargs.Contains(sym);
		}

		bool NeedsWhitespaceSeparation
		{
			get
			{
				// Start of line or buffer, don't need white space.
				if (m_bAtLineStart || m_LastChar == '\0')
				{
					return false;
				}

				// Get the previous ASCII character.
				char ch = m_LastChar;

				// Various characters, don't need separation.
				switch (ch)
				{
					case '\t':
					case ' ':
					case '\r':
					case '\n':
					case '(': // opening paren block.
					case '[': // opening brace block.
					case '-': // unary minus
					case '!': // unary not
					case '#': // Lua length operator.
					case ':': // Lua send oeprator.
						return false;

					case '.': // member access operator
						if ('.' == m_LastCharMinus1) { return true; }
						else { return false; }

					default:
						return true;
				}
			}
		}

		void Outdent() { m_Indent.PopBack(); }

		bool CommentWritingSuppressed
		{
			get
			{
				return (0 != m_iCommentWritingSuppressed);
			}
		}

		bool OutputLocked
		{
			get
			{
				return (0 != m_iOutputLock);
			}
		}

		void SeparateForFirst(SyntaxNode node, bool bForceWhitespace = false)
		{
			// No separation needed for
			// blocks that have already been delimited.
			if (node.Kind() == SyntaxKind.Block)
			{
				if (!((BlockSyntax)node).NeedsDelimiters())
				{
					return;
				}
			}

			int iStartLine = node.GetStartLine();
			if (m_iOutputLine < iStartLine && !FixedAtCurrentLine)
			{
				WriteNewlineToTarget(iStartLine);
			}
			else if (bForceWhitespace || NeedsWhitespaceSeparation)
			{
				Write(' ');
			}
		}

		void SeparateForLast(SyntaxNode node)
		{
			int iEndLine = node.GetEndLine();
			if (m_iOutputLine < iEndLine && !FixedAtCurrentLine)
			{
				WriteNewlineToTarget(iEndLine);
			}
			else if (NeedsWhitespaceSeparation)
			{
				Write(' ');
			}
		}

		void Write(char c)
		{
			// Writing disabled.
			if (OutputLocked) { return; }

			CheckOnFirst();
			m_Writer.Write(c);
			m_LastCharMinus1 = m_LastChar;
			m_LastChar = c;
			++m_iWriteOffset;
		}

		void Write(string s)
		{
			// Writing disabled.
			if (OutputLocked) { return; }

			if (!string.IsNullOrEmpty(s))
			{
				CheckOnFirst();
				m_Writer.Write(s);
				if (s.Length < 2) { m_LastCharMinus1 = m_LastChar; }
				else { m_LastCharMinus1 = s[s.Length - 2]; }
				m_LastChar = s[s.Length - 1];
				m_iWriteOffset += s.Length;
			}
		}

		void Write(SyntaxToken tok, bool bCheckUnused = false)
		{
			// Writing disabled.
			if (OutputLocked) { return; }

			var eKind = tok.Kind();
			switch (eKind)
			{
				// One-to-one mappings.
				case SyntaxKind.AsteriskToken:
				case SyntaxKind.BoolKeyword:
				case SyntaxKind.ByteKeyword:
				case SyntaxKind.DoubleKeyword:
				case SyntaxKind.EqualsEqualsToken:
				case SyntaxKind.FalseKeyword:
				case SyntaxKind.FloatKeyword:
				case SyntaxKind.GreaterThanEqualsToken:
				case SyntaxKind.GreaterThanToken:
				case SyntaxKind.IntKeyword:
				case SyntaxKind.LessThanEqualsToken:
				case SyntaxKind.LessThanToken:
				case SyntaxKind.LongKeyword:
				case SyntaxKind.MinusToken:
				case SyntaxKind.ObjectKeyword:
				case SyntaxKind.PercentToken:
				case SyntaxKind.PlusToken:
				case SyntaxKind.SByteKeyword:
				case SyntaxKind.ShortKeyword:
				case SyntaxKind.SlashToken:
				case SyntaxKind.TrueKeyword:
				case SyntaxKind.UIntKeyword:
				case SyntaxKind.ULongKeyword:
				case SyntaxKind.UnderscoreToken:
				case SyntaxKind.UShortKeyword:
					Write(tok.ValueText);
					break;

				case SyntaxKind.CharacterLiteralToken:
					// Convert char literals to its integral value.
					Write(((int)((char)tok.Value)).ToString());
					break;

				case SyntaxKind.StringKeyword:
					Write(kLuaStringGlobal);
					break;

				case SyntaxKind.AmpersandAmpersandToken:
					Write(kLuaAnd);
					break;
				case SyntaxKind.BarBarToken:
					Write(kLuaOr);
					break;
				case SyntaxKind.DotToken:
					Write('.');
					break;
				case SyntaxKind.ExclamationEqualsToken:
					Write(kLuaNotEqual);
					break;
				case SyntaxKind.ExclamationToken:
					Write(kLuaNot);
					break;
				case SyntaxKind.AddKeyword:
					Write(kCsharpPropertyAddPrefix);
					break;
				case SyntaxKind.GetKeyword:
					Write(kCsharpPropertyGetPrefix);
					break;
				case SyntaxKind.RemoveKeyword:
					Write(kCsharpPropertyRemovePrefix);
					break;
				case SyntaxKind.SetKeyword:
					Write(kCsharpPropertySetPrefix);
					break;
				case SyntaxKind.IdentifierToken:
					if (bCheckUnused && !IsUsed(tok.ValueText))
					{
						Write('_');
					}
					else
					{
						Write(tok.ValueText);
					}
					break;

				// TODO: May need more specialized handling here
				// (e.g. check range? Add LL or ULL specifiers for Int64 and UInt64?)
				case SyntaxKind.NumericLiteralToken:
					Write(tok.ValueText);
					break;

				case SyntaxKind.NullKeyword:
					Write(kLuaNil);
					break;

				case SyntaxKind.InterpolatedStringTextToken:
				case SyntaxKind.StringLiteralToken:
					WriteStringLiteral(tok.ValueText, (SyntaxKind.InterpolatedStringTextToken == eKind));
					break;

				case SyntaxKind.QuestionQuestionToken:
					Write(kLuaOr);
					break;

				case SyntaxKind.ThisKeyword:
					Write(kLuaSelf);
					break;
				default:
					throw new SlimCSCompilationException($"type '{eKind}' is not a supported token type.");
			}
		}

		void WriteArrayReadPlaceholder(ExpressionSyntax node)
		{
			// Writing disabled.
			if (OutputLocked) { return; }

			var type = m_Model.GetTypeInfo(node).ConvertedType;
			WriteArrayReadPlaceholder(type);
		}

		void WriteArrayReadPlaceholder(ITypeSymbol inType)
		{
			// Writing disabled.
			if (OutputLocked) { return; }

			var type = (inType as IArrayTypeSymbol);

			// If a type parameter, need to write an appropriate lookup.
			if (null != type && type.ElementType.TypeKind == TypeKind.TypeParameter)
			{
				// Write a lookup of the type.
				var elemType = type.ElementType;
				if (elemType.ContainingSymbol.Kind != SymbolKind.Method)
				{
					Write(kLuaSelf);
					Write('.');
				}
				Write(m_Model.LookupOutputId(elemType));
				Write('.');
				Write(kLuaArrReadPlaceholder);
			}
			// For everything else, the placeholder is nil.
			else
			{
				Write("nil");
			}
		}

        void WriteComment(string s)
	    {
			// Perform some basic reformatting of comments:
			// - convert .cs to .lua.
			// - sanitize characters that are comment terminators in
			//   lua (e.g. ]]).
			for (int i = 0; i < s.Length; ++i)
		    {
				switch (s[i])
				{
					// Possible extension - convert .cs to .lua.
					case '.':
						if (i + 2 < s.Length &&
							s[i + 1] == 'c' &&
							s[i + 2] == 's')
						{
							Write(".lua");
							i += 2;
							continue;
						}
						break;

					// Possible lua comment terminator ]].
					case ']':
						if (i + 1 < s.Length &&
							s[i+1] == ']')
						{
							Write("))");
							++i;
							continue;
						}
						break;
				}

				// Just carry through.
                Write(s[i]);
		    }
	    }

		void WriteSingleLineDocumentComment(string s)
		{
			// Single line document comments are annoying. The strings are
			// the sections between the various comment bits (e.g. <summary></summary>),
			// including newlines. So we need to look for newlines, followed
			// by whitespace, and then replace any leading // with --.

			// Perform some basic reformatting of comments - convert .cs to .lua.
			bool bLookingForSlashes = false;
			bool bInSlashes = false;
			for (int i = 0; i < s.Length; ++i)
			{
				char c = s[i];
				switch (c)
				{
					// Extension replacement.
					case '.':
						{
							if (i + 2 < s.Length &&
								c == '.' &&
								s[i + 1] == 'c' &&
								s[i + 2] == 's')
							{
								Write(".lua");
								i += 2;
								continue;
							}
						}
						break;

					// Skip '\r'
					case '\r':
						continue;

					// Newline, marks bLookingForSlahes
					case '\n':
						bLookingForSlashes = true;
						bInSlashes = false;
						WriteNewline();
						continue;

					default:
						// If whitespace, various handling depends on state.
						if (char.IsWhiteSpace(c))
						{
							// If in slashes, whitespace means we're now after slashes.
							if (bInSlashes) { bInSlashes = false; }
							// If we're looking for slashes, ignore whitespace.
							else if (bLookingForSlashes) { continue; }
						}
						// '/' character means we're in slashes if we were looking or
						// already in, and we write out a '-' instead.
						else if ('/' == c && (bInSlashes || bLookingForSlashes))
						{
							bInSlashes = true;
							Write('-');
							continue;
						}
						// All other characters terminate both looking and in but
						// are otherwise written verbatim.
						else
						{
							bInSlashes = false;
							bLookingForSlashes = false;
						}
						break;
				}

				Write(c);
			}
		}

		void WriteConstant(object o)
		{
			// Writing disabled.
			if (OutputLocked) { return; }

			if (o is bool)
			{
				var b = (bool)o;
				Write(b ? kLuaTrue : kLuaFalse);
			}
			else if (o is string)
			{
				WriteStringLiteral((string)o, false);
			}
			else if (o is null)
			{
				Write(kLuaNil);
			}
			else
			{
				// Ensure Lua source compatible formatting invariant of system culture.
				Write(string.Format(CultureInfo.InvariantCulture, "{0}", o));
			}
		}

		void WriteContinueLabelIfDefined()
		{
			// Writing disabled.
			if (OutputLocked) { return; }

			var e = m_BlockScope.Back();
			if (string.IsNullOrEmpty(e.m_sContinueLabel))
			{
				return;
			}

			if (NeedsWhitespaceSeparation) { Write(' '); }
			Write("::");
			Write(e.m_sContinueLabel);
			Write("::");
		}

		void WriteDefaultValue(TypeSyntax typeSyntax)
		{
			// Writing disabled.
			if (OutputLocked) { return; }

			var type = m_Model.GetTypeInfo(typeSyntax).ConvertedType;
			var eType = type.SpecialType;
			switch (eType)
			{
				case SpecialType.None:
					// Expected, type name - generic handling.
					if (type.TypeKind == TypeKind.TypeParameter)
					{
						if (type.ContainingSymbol.Kind != SymbolKind.Method)
						{
							Write(kLuaSelf);
							Write('.');
						}
						Write(m_Model.LookupOutputId(type));
						Write('.');
						Write(kLuaDefaultValueSpecial);
					}
					// Fallback the same as other reference types.
					else
					{
						Write("nil");
					}
					break;

					// Object and string (reference types), nil initialization.
				case SpecialType.System_Object:
				case SpecialType.System_String:
					Write("nil");
					break;

				case SpecialType.System_Boolean:
					Write("false");
					break;

				case SpecialType.System_Byte:
				case SpecialType.System_Double:
				case SpecialType.System_Int16:
				case SpecialType.System_Int32:
				case SpecialType.System_Int64:
				case SpecialType.System_SByte:
				case SpecialType.System_UInt16:
				case SpecialType.System_UInt32:
				case SpecialType.System_UInt64:
					Write("0");
					break;

				default:
					throw new UnsupportedNodeException(typeSyntax, $"unsupported default value type '{type}'.");
			}
		}

		void WriteEscaped(string s, char delim, bool bInterpolatedStringTextToken = false)
		{
			// Writing disabled.
			if (OutputLocked) { return; }

			int iLength = s.Length;
			for (int i = 0; i < iLength; ++i)
			{
				char c = s[i];
				switch (c)
				{
					case '\a': Write("\\a"); break;
					case '\b': Write("\\b"); break;
					case '\f': Write("\\f"); break;
					case '\n': Write("\\n"); break;
					case '\r': Write("\\r"); break;
					case '\t': Write("\\t"); break;
					case '\v': Write("\\v"); break;
					case '\\': Write("\\\\"); break;
					case '\"':
						if ('"' == delim) { Write("\\\""); }
						else { Write(c); }
						break;
					case '\'':
						if ('\'' == delim) { Write("\\\'"); }
						else { Write(c); }
						break;

					// Bit of unexpected handling - {{ and }} escapes
					// in interpolated string text tokens are *not* unescaped
					// by Roslyn, so we need to unescape them here, even
					// though we're generating an "escaped" string.
					case '{':
						if (bInterpolatedStringTextToken &&
							i + 1 < iLength &&
							'{' == s[i + 1])
						{
							// Skip the next '{'.
							++i;
						}
						Write(c);
						break;
					case '}':
						if (bInterpolatedStringTextToken &&
							i + 1 < iLength &&
							'}' == s[i + 1])
						{
							// Skip the next '}'.
							++i;
						}
						Write(c);
						break;

					// All other characters - if ASCII, just append, otherwise,
					// generate an appropriate escape sequence.
					default:
						// TODO: Write encoding for non-ASCII characters.
						if (m_Writer.Encoding == System.Text.Encoding.UTF8 || c.IsAscii())
						{
							Write(c);
						}
						else
						{
							// TODO:
							throw new NotImplementedException("Unicode characters are not supported.");
						}
						break;
				}
			}
		}

		(string, bool) GetFullNamespace(INamespaceSymbol sym)
		{
			string sReturn = string.Empty;
			if (null != sym.ContainingNamespace &&
				!sym.ContainingNamespace.IsGlobalNamespace)
			{
				(var s, var b) = GetFullNamespace(sym.ContainingNamespace);
				sReturn += s;
				if (b)
				{
					sReturn += ".";
				}
			}

			if (!sym.IsGlobalNamespace)
			{
				sReturn += sym.Name;
				return (sReturn, true);
			}

			return (sReturn, false);
		}

		string GetFullTypePath(ISymbol sym, char sep = '.')
		{
			string sReturn = string.Empty;
			if (null != sym.ContainingType)
			{
				sReturn += GetFullTypePath(sym.ContainingType, sep);
				sReturn += sep.ToString();
			}
			sReturn += sym.Name;
			return sReturn;
		}

		void WriteInteriorComment(int iOrigStartColumn, Int32 iOrigEndLine, int iOrigEndColumn)
		{
			// Writing disabled.
			if (OutputLocked) { return; }

			// Skip if not at an active comment.
			if (m_iComment >= m_Comments.Count)
			{
				return;
			}

			// Leave to standard processing if not a terminated
			// comment (multiline) or if we're not at the
			// necessary line.
			var comment = m_Comments[m_iComment];
			var iCommentLine = comment.GetStartLine();
			if (iCommentLine != m_iOutputLine)
			{
				return;
			}

			// Skip comments that do not follow the given node.
			var iCommentColumn = comment.GetStartColumn();
			if (iCommentColumn < iOrigStartColumn ||
				iCommentLine > iOrigEndLine ||
				(iCommentLine == iOrigEndLine && iCommentColumn > iOrigEndColumn))
			{
				return;
			}

			// Can only write interior comments if a multiline comment
			// (since single line comments must terminate the line).
			var eKind = comment.Kind();
			if (eKind != SyntaxKind.MultiLineCommentTrivia)
			{
				return;
			}

			var s = comment.ToString();
			var iNewlineCount = CountNewlines(s);

			// If a multiline comment has any newlines, we
			// can't write it as an interior comment.
			if (0 != iNewlineCount)
			{
				return;
			}

			// Don't actually write if comment writing is disabled.
			if (!CommentWritingSuppressed)
			{
				// Write the comment.
				s = s.Substring(2, s.Length - 4);
				Write("--[[");
				WriteComment(s);
				Write("]]");
			}

			// Advance to the next comment.
			++m_iComment;
		}

		enum MemberIdentifierSpecifierType
		{
			None,
			Local,
			Instance,
			Qualified,
		}

		/// <summary>
		/// Utility, emits a specifier to a variable assignment into a type
		/// being defined in the generated file.
		/// </summary>
		(string, MemberIdentifierSpecifierType) GetMemberIdentifierSpecifier(SyntaxNode node, SyntaxTokenList mods, SyntaxKind eKind)
		{
			(var bConstOrStatic, var bPrivate) = mods.IsConstOrStaticAndPrivate();
			var bType = (
				eKind == SyntaxKind.ClassDeclaration ||
				eKind == SyntaxKind.InterfaceDeclaration ||
				eKind == SyntaxKind.EnumDeclaration ||
				eKind == SyntaxKind.StructDeclaration);
			var bFunction = (
				eKind == SyntaxKind.AddAccessorDeclaration ||
				eKind == SyntaxKind.ConstructorDeclaration ||
				eKind == SyntaxKind.DestructorDeclaration ||
				eKind == SyntaxKind.GetAccessorDeclaration ||
				eKind == SyntaxKind.MethodDeclaration ||
                eKind == SyntaxKind.OperatorDeclaration ||
				eKind == SyntaxKind.RemoveAccessorDeclaration ||
				eKind == SyntaxKind.SetAccessorDeclaration);
			var bInstance = (!bConstOrStatic && (eKind == SyntaxKind.FieldDeclaration || eKind == SyntaxKind.PropertyDeclaration));

			// Fields marked as such are emitted as local.
			bool bDeclared = false;
			string sReturn = string.Empty;
			if (CanEmitAsLocal(node, out bDeclared))
			{
				// Declaration is not needed because this node was already declared
				// (it is accessed textually before it is defined, which is
				// allowed in SlimCS/C#, so prior to that usage, it was
				// already emitted as a pre-declaration).
				if (!bDeclared)
				{
					// local b =
					sReturn += kLuaLocal + " ";
				}
				if (bFunction) { sReturn += kLuaFunction + " "; }
				return (sReturn, MemberIdentifierSpecifierType.Local);
			}
			// Instance - will be emitted in a constructor context.
			else if (bInstance)
			{
				sReturn += kLuaSelf + ".";
				return (sReturn, MemberIdentifierSpecifierType.Instance);
			}
			// Everything else, must be qualified.
			else
			{
				// a.b =

				if (bFunction) { sReturn += kLuaFunction + " "; }
				sReturn += GetTypeScope();
				sReturn += ((bFunction && !bConstOrStatic) ? ':' : '.');
				return (sReturn, MemberIdentifierSpecifierType.Qualified);
			}
		}

		void WriteMemberIdentifierSpecifier(SyntaxNode node, SyntaxTokenList mods, SyntaxKind eKind)
		{
			(var s, var _) = GetMemberIdentifierSpecifier(node, mods, eKind);
			Write(s);
		}

		void WriteNewline()
		{
			// Writing disabled.
			if (OutputLocked) { return; }

			// Refuse to newline if we're fixed at the current line.
			if (FixedAtCurrentLine)
			{
				return;
			}

			m_Writer.Write(Environment.NewLine);
			m_LastCharMinus1 = m_LastChar;
			m_LastChar = Environment.NewLine[Environment.NewLine.Length - 1];
			m_iWriteOffset += Environment.NewLine.Length;
			++m_iOutputLine;
			m_bAtLineStart = true;

			m_Indent.Back().OnNewline();

			// We only allow a single indent level increase on a newline.
			// If more indents have been triggered, we filter them away
			// by just copying through the newline depth at the last
			// depth + 1.
			for (int i = (m_iIndentStackCountAtNewline + 1); i < m_Indent.Count; ++i)
			{
				m_Indent[i] = m_Indent[m_iIndentStackCountAtNewline];
			}
			m_iIndentStackCountAtNewline = m_Indent.Count;
		}

		void WriteNewlineToTarget(int iTarget)
		{
			// Writing disabled.
			if (OutputLocked) { return; }

			// Refuse if fixing the current line.
			if (FixedAtCurrentLine)
			{
				return;
			}

			// Loop, writing newlines and comments as needed.
			while (m_iOutputLine < iTarget)
			{
				int iLineStart = m_iOutputLine;
				var iComments = m_Comments.Count;
				while (m_iComment < iComments)
				{
					var comment = m_Comments[m_iComment];
					var iCommentLine = comment.GetStartLine();

					// If we missed this comment, skip it and try again.
					if (iCommentLine < m_iOutputLine)
					{
						++m_iComment;
						continue;
					}

					// If we're not at the comment line yet,
					// break.
					if (iCommentLine != m_iOutputLine)
					{
						break;
					}

					var eKind = comment.Kind();
					switch (eKind)
					{
						case SyntaxKind.MultiLineCommentTrivia:
						case SyntaxKind.MultiLineDocumentationCommentTrivia:
							{
								// Don't actually write if comment writing is disabled.
								if (!CommentWritingSuppressed)
								{
									var s = comment.ToString();
									if (s.StartsWith("/**"))
									{
										s = s.Substring(3);
									}
									else if (s.StartsWith("/*"))
									{
										s = s.Substring(2);
									}
									if (s.EndsWith("*/"))
									{
										s = s.Substring(0, s.Length - 2);
									}
									Write("--[[");
									WriteComment(s);
									Write("]]");
									CountNewlinesAndAddToOutputCount(s);
								}
							}
							break;
						case SyntaxKind.SingleLineCommentTrivia:
							{
								// Don't actually write if comment writing is disabled.
								if (!CommentWritingSuppressed)
								{
									// Pad if necessary.
									if (NeedsWhitespaceSeparation) { Write(' '); }

									var s = comment.ToString().Substring(2);
									Write("--");
									WriteComment(s);

									// If the comment itself didn't perform a newline,
									// we need to add one explicitly for this kind of comment.
									if (!CountNewlinesAndAddToOutputCount(s)) { WriteNewline(); }
								}
							}
							break;

						case SyntaxKind.SingleLineDocumentationCommentTrivia:
							{
								// Don't actually write if comment writing is disabled.
								if (!CommentWritingSuppressed)
								{
									// Pad if necessary.
									if (NeedsWhitespaceSeparation) { Write(' '); }

									var s = comment.ToString();
									Write("---");
									WriteSingleLineDocumentComment(s);
								}
							}
							break;
						default:
							throw new NotImplementedException("comment type not implemented: " + eKind.ToString());
					}

					++m_iComment;

					// Done until the next outer loop.
					break;
				}

				// If we didn't advance a newline via comments,
				// perform the advancement now.
				if (iLineStart == m_iOutputLine)
				{
					WriteNewline();
				}
			}
		}

		void WriteStringLiteral(string s, bool bInterpolatedStringTextToken = false)
		{
			// Writing disabled.
			if (OutputLocked) { return; }

			char delim = (s.IndexOf('\'') >= 0 ? '"' : '\'');
			Write(delim);
			WriteEscaped(s, delim, bInterpolatedStringTextToken);
			Write(delim);
		}

		string GetFullTypeString(ISymbol sym, char sep = '.')
		{
			// Namespace.
			(var sFullNamespace, var b) = GetFullNamespace(sym.ContainingNamespace);
			var sReturn = sFullNamespace;
			if (b)
			{
				sReturn += ".";
			}

			sReturn += GetFullTypePath(sym, sep);

			return sReturn;
		}

		/// <summary>
		/// Emits the current stack of types as a dot separate
		/// member access expression. Meant to prefix
		/// a member lookup or definition.
		/// </summary>
		string GetTypeScope(char sep = '.', bool bFull = false)
		{
			int iCount = m_TypeScope.Count;

			// Early out, nothing.
			if (0 == iCount)
			{
				return string.Empty;
			}

			// Get the symbol we're printing.
			var sym = m_TypeScope.Back().m_Symbol;

			// Fully qualified name.
			string sReturn;
			if (bFull)
			{
				sReturn = GetFullTypeString(sym, sep);
			}
			// Otherwise, just the name.
			else
			{
				sReturn = m_Model.LookupOutputId(sym);
			}

			return sReturn;
		}

		void WriteTypeScope(char sep = '.', bool bFull = false)
		{
			// Writing disabled.
			if (OutputLocked) { return; }

			Write(GetTypeScope(sep, bFull));
		}
	}
}
