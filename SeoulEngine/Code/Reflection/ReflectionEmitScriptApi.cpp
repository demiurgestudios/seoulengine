/**
 * \file ReflectionEmitScriptApi.cpp
 * \brief Utility used to enumerate all Reflectable types and emit
 * an appropriate SlimCS API for them.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FileManager.h"
#include "Logger.h"
#include "Path.h"
#include "ReflectionArray.h"
#include "ReflectionAttributes.h"
#include "ReflectionMethod.h"
#include "ReflectionMethodTypeInfo.h"
#include "ReflectionRegistry.h"
#include "ReflectionTable.h"
#include "ReflectionType.h"
#include "SeoulFile.h"
#include "StringUtil.h"

namespace Seoul::Reflection
{

namespace
{

enum class EmitType
{
	kEmitType_None,

	kEmitType_Data,
	kEmitType_Interface,
};

const Byte* GetEmissionNamespaceName(EmitType eEmitType)
{
	switch(eEmitType)
	{
		case EmitType::kEmitType_Data:
			return "NativeData";
		case EmitType::kEmitType_Interface:
			return "Native";
		default:
			return nullptr;
	}
}

class Printer SEOUL_SEALED
{
public:
	Printer(EmitType eRootEmitType = EmitType::kEmitType_None)
		: m_eRootEmitType(eRootEmitType)
		, m_Buffer()
		, m_Eol(StrLen(SEOUL_EOL))
	{
	}

	const EmitType m_eRootEmitType;

	void Printf(const Byte *sFormat, ...) SEOUL_PRINTF_FORMAT_ATTRIBUTE(2, 3)
	{
		// Format.
		va_list args;
		va_start(args, sFormat);
		auto const s(String::VPrintf(sFormat, args));
		va_end(args);

		// Emit.
		if (!s.IsEmpty())
		{
			ReSharperBlankLinesAroundElementPostfix(s);
			m_Buffer.Write(s.CStr(), s.GetSize());
		}
	}

	/**
	 * Match ReSharper rule: csharp_blank_lines_around_local_method=1, and others.
	 *
	 * Call this method before emitting a method/element that should be surrounded by blank
	 * lines (a multiline method with a body). This handles prefix spacing, postfix spacing
	 * is handled automatically by Printf().
	 *
	 * See also SeoulEngine's .editorconfig file for Scripts.
	 */
	void ReSharperBlankLinesAroundElementPrefix()
	{
		auto p = CheckSeparate(nullptr);
		if (nullptr == p) { return; }

		// Append additional newline.
		AppendEol();
	}

	/**
	 * Commit the current buffer state to disk.
	 */
	Bool Save(const String& sFile)
	{
		ScopedPtr<SyncFile> pFile;
		if (!FileManager::Get()->OpenFile(sFile, File::kWriteTruncate, pFile) || !pFile->IsOpen())
		{
			fprintf(stderr, "Failed writing .cs file for type \"%s\"\n", sFile.CStr());
			return false;
		}

		return m_Buffer.Save(*pFile);
	}

	/**
	 * Implement ReSharper rule: csharp_max_line_length=160
	 *
	 * Call after writing all parts of a method declaration except the trailing semicolon and newline.
	 * Implements optional wrapping of the declaration to multiple lines to match ReSharper formatting
	 * rules.
	 *
	 * See also SeoulEngine's .editorconfig file for Scripts.
	 */
	void EndAndReSharperWrapMethodDeclarationToMaxLineLength()
	{
		Vector<Byte> vLine;
		{
			// Mark end before continuing
			auto const uSearchStart = m_Buffer.GetTotalDataSizeInBytes();

			// Terminate first.
			Printf(";" SEOUL_EOL);

			// Find the end of the previous line.
			auto const uEnd = m_Buffer.GetTotalDataSizeInBytes();
			auto uLineStart = uSearchStart;
			auto p = m_Buffer.GetBuffer();
			while (uLineStart > 0)
			{
				--uLineStart;
				if ('\n' == p[uLineStart])
				{
					++uLineStart;
					break;
				}
			}

			// Line limit of 160. Tabs count for 4.
			UInt32 uLineLength = 0u;
			for (auto u = uLineStart; u < uEnd; ++u)
			{
				if ('\t' == p[u])
				{
					uLineLength += 4u;
				}
				else
				{
					++uLineLength;
				}
			}

			// Short enough, nothing to do.
			if (uLineLength <= 160)
			{
				return;
			}

			// Capture the line to perform reflow.
			vLine.Resize(uEnd - uLineStart);
			memcpy(vLine.Data(), p + uLineStart, vLine.GetSize());

			// Remove the line from the current buffer state.
			m_Buffer.TruncateTo(uLineStart);
		}

		// Multiline convert the method declaration into
		// an element that requires a single line of spacing
		// surrounding it. Check for that and add it now.
		{
			auto p = CheckSeparate(nullptr);
			// Need separation unless we're the first method in a block,
			// or separation is already present (we are preceeded by
			// a multiline element).
			if (nullptr != p && '{' != *p && '\n' != *p)
			{
				AppendEol();
			}
		}

		// Reflow after the last open paren at depth 0 (ignore
		// nesting).
		if (vLine.Begin() != vLine.End())
		{
			auto iReflow = vLine.Begin();
			Int iParenCount = 0;
			for (auto i = iReflow, iEnd = vLine.End(); iEnd != i; ++i)
			{
				if ('(' == *i)
				{
					if (0 == iParenCount)
					{
						iReflow = i;
					}
					++iParenCount;
				}
				else if (')' == *i)
				{
					--iParenCount;
				}
			}

			if (vLine.Begin() != iReflow)
			{
				for (auto i = vLine.Begin(); iReflow != i; ++i)
				{
					auto ch = *i;
					m_Buffer.Write(&ch, 1);
				}
				vLine.Erase(vLine.Begin(), iReflow);
			}
		}

		// Reflow the line - add an additional newline before and after,
		// then add newlines after each comma.
		Bool bSkipWhiteSpace = false;
		Int iDepth = 0;
		for (auto const& ch : vLine)
		{
			// Skipping whitespace - trailing tabs or spaces after a comma or opening paren.
			if (bSkipWhiteSpace)
			{
				if (' ' == ch || '\t' == ch)
				{
					continue;
				}
			}

			// Depth tracking.
			if ('(' == ch || '<' == ch || '{' == ch)
			{
				++iDepth;
			}
			else if (')' == ch || '>' == ch || '}' == ch)
			{
				--iDepth;
			}

			// No longer skipping whitespace once we write a character.
			bSkipWhiteSpace = false;
			m_Buffer.Write(&ch, 1);
			if (iDepth <= 1)
			{
				// Need to perform a wrap at these characters.
				if ('(' == ch || ',' == ch) // New line break.
				{
					// Commit the newline.
					AppendEol();
					m_Buffer.Write("\t\t\t", 3); // Always 3 indent in the context of the emit API.
					bSkipWhiteSpace = true; // Skipping any trailing whitepspace after the comma or paren.
				}
			}
		}
	}

private:
	StreamBuffer m_Buffer;
	UInt32 const m_Eol;

	/** Convenience, raw newline append. */
	void AppendEol()
	{
		m_Buffer.Write(SEOUL_EOL, m_Eol);
	}

	/** Utility, find the first the first char of sNext that is not ' ' or '\t' */
	static UniChar GetFirstNotSimpleWhiteSpace(const String& sNext)
	{
		for (auto i = sNext.Begin(), iEnd = sNext.End(); iEnd != i; ++i)
		{
			auto const ch = *i;
			if (ch != ' ' && ch != '\t')
			{
				return ch;
			}
		}

		return (UniChar)'\0';
	}

	/**
	 * Given an incoming string, check if that string needs an extra
	 * newline prefix, based on the current end of the buffer and the start of that string.
	 */
	Byte* CheckSeparate(String const* psNext)
	{
		if (m_Buffer.IsEmpty()) { return nullptr; } // Edge case.

		// Check if we need separation - brace characters, and some special cases,
		// never do.
		if (nullptr != psNext)
		{
			auto const chNext = GetFirstNotSimpleWhiteSpace(*psNext);
			if ('\0' == chNext ||
				chNext == '{' ||
				chNext == '}' ||
				chNext == '#')
			{
				return nullptr;
			}
		}

		// Relies on convention - newlines are generally appended not prepended
		// in formatting from this file. As such, we potentially add a newline if the
		// buffer already contains a single newline
		auto p = m_Buffer.GetBuffer() + m_Buffer.GetOffset() - 1;

		// Simple reverse iterate.
		auto const stepBack = [&]()
		{
			if (p <= m_Buffer.GetBuffer()) { return false; }
			--p;
			return true;
		};

		// End with newline or does not apply.
		if ('\n' != *p && '\r' != *p) { return nullptr; } // Nothing to do if not at the start of a line.
		// Terminate without any more data.
		if (!stepBack()) { return nullptr; }
		// Need to stepback again if Windows style "\r\n".
		if ('\r' == *p)
		{
			// No more data.
			if (!stepBack()) { return nullptr; }
		}

		// Found a trailing newline, point to just before it.
		return p;
	}

	/**
	 * Match ReSharper rule: csharp_blank_lines_around_local_method=1, and others.
	 *
	 * Add an additional trailing newline to surround the element if preceded
	 * immediately by a closing brace.
	 */
	void ReSharperBlankLinesAroundElementPostfix(const String& sNext)
	{
		// CheckSeparate tells us if the buffer ends with a newline - p points to
		// the character immediately preceeding that newline.
		auto p = CheckSeparate(&sNext);
		if (nullptr == p) { return; }

		// Append additional newline if we're starting a new thing after a closing brace.
		if ('}' == *p)
		{
			AppendEol();
		}
		// Somewhat hacky, but sufficient for what we're doing here - we also want to
		// append a newline to surround method declarations that have been turned into
		// multiline. Which can be detected by looking for a trailing declaration
		// ");" and then searching backward - if we hit a newline *before* we hit the
		// opening ( of the declaration, the declaration is multiline.
		else if (';' == *p)
		{
			auto pBegin = m_Buffer.GetBuffer();
			if (p > pBegin)
			{
				--p;
				if (')' == *p)
				{
					while (p > pBegin)
					{
						--p;
						if ('\n' == *p || '(' == *p)
						{
							break;
						}
					}

					if ('(' != *p)
					{
						AppendEol();
					}
				}
			}
		}
	}
};

// Special cases.
static const HString kFilePathRelativeFilename("FilePathRelativeFilename");
static const HString kScriptArrayIndex("ScriptArrayIndex");

}

typedef HashTable<Type const*, EmitType, MemoryBudgets::Reflection> TypeEmitTable;

static HashSet<HString> GetExclusions()
{
	// TODO: Eliminate app specific references.

	HashSet<HString> set;
	set.Insert(HString("AnalyticsProfileUpdate"));
	set.Insert(HString("AppPersistenceMigrations"));
	set.Insert(HString("GameBoardSettings"));
	set.Insert(HString("ReflectionTestUtility"));
	set.Insert(HString("String"));
	return set;
}

static const HashSet<HString> s_kExclusions(GetExclusions());

/**
 * When emitting names, reformat namespaces to use "_" in place of "::".
 * Also replace template specialized delimiters with "_".
 */
static inline String GetEmissionName(const String& sName)
{
	return sName.ReplaceAll("::", "_").ReplaceAll("<", "_").ReplaceAll(">", "_");
}

static inline String GetEmissionName(HString sName)
{
	return GetEmissionName((String)sName);
}

static inline EmitType NeedsEmit(Type const* type)
{
	auto p = type->GetAttribute<Attributes::ScriptClass>();
	if (nullptr != p && p->m_bEmit)
	{
		return EmitType::kEmitType_Data;
	}

	// Not unit testing fixtures.
	if (type->HasAttribute<Attributes::UnitTest>() ||
		type->HasAttribute<Attributes::CommandsInstance>())
	{
		return EmitType::kEmitType_None;
	}

	// TODO: Hack, should add another attribute for this.
	if (type->GetName().CStr() == strstr(type->GetName().CStr(), "ScriptTest") ||
		type->GetName().CStr() + type->GetName().GetSizeInBytes() - 4 == strstr(type->GetName().CStr(), "Test") ||
		s_kExclusions.HasKey(type->GetName()))
	{
		return EmitType::kEmitType_None;
	}

	// No methods.
	if (type->GetMethodCount() == 0u)
	{
		// We also emit interface style for command-line argument behinds.
		Bool bOk = false;
		{
			auto const uProperties = type->GetPropertyCount();
			for (UInt32 i = 0u; i < uProperties; ++i)
			{
				if (type->GetProperty(i)->GetAttributes().HasAttribute<Attributes::CommandLineArg>())
				{
					bOk = true;
					break;
				}
			}
		}

		if (!bOk)
		{
			return EmitType::kEmitType_None;
		}
	}

	// Good to go.
	return EmitType::kEmitType_Interface;
}

static inline Bool IsValueType(const TypeInfo& typeInfo, Bool bReturnValue)
{
	switch (typeInfo.GetSimpleTypeInfo())
	{
	case SimpleTypeInfo::kEnum:
		return bReturnValue;

	case SimpleTypeInfo::kBoolean:
	case SimpleTypeInfo::kInt8:
	case SimpleTypeInfo::kInt16:
	case SimpleTypeInfo::kInt32:
	case SimpleTypeInfo::kInt64:
	case SimpleTypeInfo::kFloat32:
	case SimpleTypeInfo::kFloat64:
	case SimpleTypeInfo::kUInt8:
	case SimpleTypeInfo::kUInt16:
	case SimpleTypeInfo::kUInt32:
	case SimpleTypeInfo::kUInt64:
		return true;
	default:
		return false;
	};
}

static void PrintTypeInfo(
	Printer& r,
	const TypeEmitTable& tTypes,
	const TypeInfo& typeInfo,
	Bool bReturnValue = false)
{
	switch (typeInfo.GetSimpleTypeInfo())
	{
	case SimpleTypeInfo::kBoolean:
		r.Printf("bool"); break;

	case SimpleTypeInfo::kCString:
	case SimpleTypeInfo::kHString:
	case SimpleTypeInfo::kString:
		r.Printf("string");
		break;

		// Enums are converted to int on return but
		// take type object.
	case SimpleTypeInfo::kEnum:
		if (bReturnValue) { r.Printf("int"); }
		else { r.Printf("object"); }
		break;

	case SimpleTypeInfo::kInt8:
	case SimpleTypeInfo::kInt16:
	case SimpleTypeInfo::kInt32:
	case SimpleTypeInfo::kInt64:
	case SimpleTypeInfo::kUInt8:
	case SimpleTypeInfo::kUInt16:
	case SimpleTypeInfo::kUInt32:
	case SimpleTypeInfo::kUInt64:
		r.Printf("int");
		break;

	case SimpleTypeInfo::kFloat32:
	case SimpleTypeInfo::kFloat64:
		r.Printf("double");
		break;

	case SimpleTypeInfo::kComplex:
		if (auto pTable = typeInfo.GetType().TryGetTable())
		{
			auto const bValueType = IsValueType(pTable->GetValueTypeInfo(), bReturnValue);

			r.Printf(bValueType ? "SlimCS.TableV<" : "SlimCS.Table<");
			PrintTypeInfo(r, tTypes, pTable->GetKeyTypeInfo(), bReturnValue);
			r.Printf(", ");
			PrintTypeInfo(r, tTypes, pTable->GetValueTypeInfo(), bReturnValue);
			r.Printf(">");
		}
		else if (auto pArray = typeInfo.GetType().TryGetArray())
		{
			auto const bValueType = IsValueType(pArray->GetElementTypeInfo(), bReturnValue);

			r.Printf(bValueType ? "SlimCS.TableV<double, " : "SlimCS.Table<double, ");
			PrintTypeInfo(r, tTypes, pArray->GetElementTypeInfo(), bReturnValue);
			r.Printf(">");
		}
		else if (typeInfo.IsVoid())
		{
			r.Printf("void");
		}
		else if (kScriptArrayIndex == typeInfo.GetType().GetName())
		{
			r.Printf("int");
		}
		else if (kFilePathRelativeFilename == typeInfo.GetType().GetName())
		{
			r.Printf("string");
		}
		else if (tTypes.HasValue(&typeInfo.GetType()))
		{
			// If this happens to be one of our types, there is a chance that
			// we will need to namespace qualify it because we generate things
			// in two different namespaces based on their emit type
			if (const EmitType* pEmitType = tTypes.Find(&typeInfo.GetType()))
			{
				if (*pEmitType != r.m_eRootEmitType)
				{
					if (const Byte* sNamespace = GetEmissionNamespaceName(*pEmitType))
					{
						r.Printf("%s.", sNamespace);
					}
				}
			}

			// Print the type name.
			r.Printf("%s", GetEmissionName(typeInfo.GetType().GetName()).CStr());
		}
		else
		{
			// Fallback to dynamic Table.
			r.Printf("SlimCS.Table");
		}
		break;
	};
}

static Vector<HString, MemoryBudgets::Reflection> GetSignatureNames(Method const* pMethod)
{
	// Return structure.
	Vector<HString, MemoryBudgets::Reflection> v;

	// Custom signature, use it.
	auto pSignature = pMethod->GetAttributes().GetAttribute<Attributes::ScriptSignature>();
	if (nullptr != pSignature)
	{
		Vector<String> vs;
		SplitString(String(pSignature->m_Args), ',', vs, true);

		Vector<String> vsArg;
		for (auto const& s : vs)
		{
			vsArg.Clear();
			SplitString(s, ' ', vsArg, true);
			v.PushBack(HString(TrimWhiteSpace(vsArg.Back())));
		}
	}
	else
	{
		auto const& typeInfo = pMethod->GetTypeInfo();
		v.Reserve(typeInfo.m_uArgumentCount);
		for (UInt32 i = 0u; i < typeInfo.m_uArgumentCount; ++i)
		{
			v.PushBack(HString(String::Printf("a%u", i)));
		}
	}

	return v;
}

static Bool PrintSignature(
	Printer& r,
	const TypeEmitTable& tTypes,
	Method const* pMethod)
{
	auto const& typeInfo = pMethod->GetTypeInfo();

	// Custom signature, use it.
	auto pSignature = pMethod->GetAttributes().GetAttribute<Attributes::ScriptSignature>();

	// Return
	auto bVoidReturn = false;
	if (nullptr != pSignature)
	{
		r.Printf("%s", pSignature->m_Return.CStr());
		static const HString kVoid("void");
		if (pSignature->m_Return == kVoid)
		{
			bVoidReturn = true;
		}
	}
	else
	{
		PrintTypeInfo(r, tTypes, typeInfo.m_rReturnValueTypeInfo, true);
		bVoidReturn = typeInfo.m_rReturnValueTypeInfo.IsVoid();
	}

	// Name
	r.Printf(" %s(", GetEmissionName(pMethod->GetName()).CStr());

	// Args
	if (nullptr != pSignature) { r.Printf("%s", pSignature->m_Args.CStr()); }
	else
	{
		for (UInt32 i = 0u; i < typeInfo.m_uArgumentCount; ++i)
		{
			if (0u != i)
			{
				r.Printf(", ");
			}

			PrintTypeInfo(r, tTypes, typeInfo.GetArgumentTypeInfo(i));
			r.Printf(" a%u", i);
		}
	}

	// Terminate.
	r.Printf(")");

	return bVoidReturn;
}

static const HString kAdd("__add");
static const HString kEqual("__eq");
static const HString kLessEqual("__le");
static const HString kLessThan("__lt");
static const HString kSub("__sub");
static const HString kUnaryMinus("__unm");

static Bool PrintMethodSpecial(
	Printer& r,
	const TypeEmitTable& tTypes,
	Method const* pMethod)
{
	auto const& type = pMethod->GetTypeInfo().m_rClassTypeInfo;
	auto const h = pMethod->GetName();
	auto const& ret = pMethod->GetTypeInfo().m_rReturnValueTypeInfo;
	auto const& a1 = pMethod->GetTypeInfo().m_rArgument0TypeInfo;
	if (h == kAdd)
	{
		r.Printf("\t\tpublic static extern ");
		PrintTypeInfo(r, tTypes, ret, true);
		r.Printf(" operator+(");
		PrintTypeInfo(r, tTypes, type);
		r.Printf(" a0, ");
		PrintTypeInfo(r, tTypes, a1);
		r.Printf(" a1);" SEOUL_EOL);
		return true;
	}
	else if (h == kSub)
	{
		r.Printf("\t\tpublic static extern ");
		PrintTypeInfo(r, tTypes, ret, true);
		r.Printf(" operator-(");
		PrintTypeInfo(r, tTypes, type);
		r.Printf(" a0, ");
		PrintTypeInfo(r, tTypes, a1);
		r.Printf(" a1);" SEOUL_EOL);
		return true;
	}
	else if (h == kEqual)
	{
		// Don't need to emit anything explicit for equality.
		return true;
	}
	else if (h == kLessEqual)
	{
		r.Printf("\t\tpublic static extern ");
		PrintTypeInfo(r, tTypes, ret, true);
		r.Printf(" operator<=(");
		PrintTypeInfo(r, tTypes, type);
		r.Printf(" a0, ");
		PrintTypeInfo(r, tTypes, a1);
		r.Printf(" a1);" SEOUL_EOL);

		r.Printf("\t\tpublic static extern ");
		PrintTypeInfo(r, tTypes, ret, true);
		r.Printf(" operator>=(");
		PrintTypeInfo(r, tTypes, type);
		r.Printf(" a0, ");
		PrintTypeInfo(r, tTypes, a1);
		r.Printf(" a1);" SEOUL_EOL);
		return true;
	}
	else if (h == kLessThan)
	{
		r.Printf("\t\tpublic static extern ");
		PrintTypeInfo(r, tTypes, ret, true);
		r.Printf(" operator<(");
		PrintTypeInfo(r, tTypes, type);
		r.Printf(" a0, ");
		PrintTypeInfo(r, tTypes, a1);
		r.Printf(" a1);" SEOUL_EOL);

		r.Printf("\t\tpublic static extern ");
		PrintTypeInfo(r, tTypes, ret, true);
		r.Printf(" operator>(");
		PrintTypeInfo(r, tTypes, type);
		r.Printf(" a0, ");
		PrintTypeInfo(r, tTypes, a1);
		r.Printf(" a1);" SEOUL_EOL);
		return true;
	}
	else if (h == kUnaryMinus)
	{
		r.Printf("\t\tpublic static extern ");
		PrintTypeInfo(r, tTypes, ret, true);
		r.Printf(" operator-(");
		PrintTypeInfo(r, tTypes, type);
		r.Printf(" a0);" SEOUL_EOL);
		return true;
	}
	else
	{
		return false;
	}
}

static void PrintMethod(
	Printer& r,
	const TypeEmitTable& tTypes,
	Method const* pMethod)
{
	auto const& typeInfo = pMethod->GetTypeInfo();

	if (PrintMethodSpecial(r, tTypes, pMethod))
	{
		return;
	}

	// Const methods are marked as [Pure] in C#.
	auto const bPure = typeInfo.IsConst();

	// Static methods have bodies that call through
	// to a wrapper interface.
	auto const bStatic = typeInfo.IsStatic();

	// TODO: Hack, should add another attribute for this.
	{
		static HString const kConstruct("Construct");
		if (pMethod->GetName() == kConstruct)
		{
			return;
		}
	}

	// Applies when static since we generated a bodied wrapper.
	if (bStatic) { r.ReSharperBlankLinesAroundElementPrefix(); }

	// Pure attribute.
	Byte const* sPure = (bPure ? "[Pure] " : "");
	if (bStatic) { r.Printf("\t\t%spublic static ", sPure); }
	else { r.Printf("\t\t%spublic abstract ", sPure); }

	// Special cases.
	static const HString kGetType("GetType");
	static const HString kToString("ToString");
	if (pMethod->GetName() == kGetType ||
		pMethod->GetName() == kToString)
	{
		r.Printf("new ");
	}

	// Signature.
	auto const bVoidReturn = PrintSignature(r, tTypes, pMethod);

	// Static methods get a body that invokes through a static interface API.
	if (bStatic)
	{
		r.Printf(SEOUL_EOL "\t\t{" SEOUL_EOL "\t\t\t");
		if (!bVoidReturn) { r.Printf("return "); }
		r.Printf("s_udStaticApi.");
		r.Printf("%s", GetEmissionName(pMethod->GetName()).CStr());
		r.Printf("(");
		auto const vNames(GetSignatureNames(pMethod));
		for (UInt32 i = 0u; i < vNames.GetSize(); ++i)
		{
			if (0u != i)
			{
				r.Printf(", ");
			}

			r.Printf("%s", vNames[i].CStr());
		}
		r.Printf(");" SEOUL_EOL "\t\t}" SEOUL_EOL);
	}
	// Otherwise just terminate immediately - declaration only.
	else
	{
		r.EndAndReSharperWrapMethodDeclarationToMaxLineLength(); // Terminates and potentially reflows for line length.
	}
}

static inline Bool Contains(Type const* type, Method const* pMethod)
{
	if (type->GetMethod(pMethod->GetName()) != nullptr)
	{
		return true;
	}

	if (type->GetParentCount() > 0u)
	{
		return Contains(type->GetParent(0u), pMethod);
	}

	return false;
}

static void PrintStaticApi(
	Printer& r,
	const TypeEmitTable& tTypes,
	Type const* type)
{
	r.Printf("\t\tinterface IStatic" SEOUL_EOL);
	r.Printf("\t\t{" SEOUL_EOL);
	{
		for (auto i = 0u; i < type->GetMethodCount(); ++i)
		{
			if (type->GetMethod(i)->GetTypeInfo().IsStatic())
			{
				r.Printf("\t\t\t");
				PrintSignature(r, tTypes, type->GetMethod(i));
				r.EndAndReSharperWrapMethodDeclarationToMaxLineLength(); // Terminates and potentially reflows for line length.
			}
		}
	}
	{
		for (auto i = 0u; i < type->GetPropertyCount(); ++i)
		{
			auto const pProperty = type->GetProperty(i);
			if (pProperty->GetAttributes().HasAttribute<Attributes::CommandLineArg>())
			{
				r.Printf("\t\t\t");
				PrintTypeInfo(r, tTypes, pProperty->GetMemberTypeInfo(), true);
				r.Printf(" %s();" SEOUL_EOL, pProperty->GetName().CStr());
			}
		}
	}
	r.Printf("\t\t}" SEOUL_EOL);
	r.Printf("\t\tstatic IStatic s_udStaticApi;" SEOUL_EOL);
	r.ReSharperBlankLinesAroundElementPrefix();
	r.Printf("\t\tstatic %s()" SEOUL_EOL, GetEmissionName(type->GetName()).CStr());
	r.Printf("\t\t{" SEOUL_EOL);
	r.Printf("\t\t\ts_udStaticApi = SlimCS.dyncast<IStatic>(CoreUtilities.DescribeNativeUserData(\"%s\"));" SEOUL_EOL, GetEmissionName(type->GetName()).CStr());
	r.Printf("\t\t}" SEOUL_EOL);
}

static void PrintInterfaceType(
	Printer& r,
	const TypeEmitTable& tTypes,
	Type const* type)
{
	r.Printf("\tpublic abstract class %s", GetEmissionName(type->GetName()).CStr());
	if (type->GetParentCount() > 0u)
	{
		if (tTypes.HasValue(type->GetParent(0u)))
		{
			r.Printf(" : %s", GetEmissionName(type->GetParent(0u)->GetName()).CStr());
		}
	}
	r.Printf(SEOUL_EOL);
	r.Printf("\t{" SEOUL_EOL);

	// Check if we need static and if so, emit the api hook
	// for calling static methods.
	auto bNeedStatic = false;
	for (auto i = 0u; i < type->GetMethodCount(); ++i)
	{
		if (type->GetMethod(i)->GetTypeInfo().IsStatic())
		{
			bNeedStatic = true;
			break;
		}
	}
	if (!bNeedStatic)
	{
		for (auto i = 0u; i < type->GetPropertyCount(); ++i)
		{
			auto const pProperty = type->GetProperty(i);
			if (pProperty->GetAttributes().HasAttribute<Attributes::CommandLineArg>())
			{
				bNeedStatic = true;
				break;
			}
		}
	}

	// Handle static emit if needed.
	if (bNeedStatic)
	{
		PrintStaticApi(r, tTypes, type);
	}

	for (auto i = 0u; i < type->GetMethodCount(); ++i)
	{
		// Skip methods contained in parents - treat as an override.
		if (type->GetParentCount() > 0u)
		{
			if (Contains(type->GetParent(0u), type->GetMethod(i)))
			{
				continue;
			}
		}

		PrintMethod(r, tTypes, type->GetMethod(i));
	}
	{
		for (auto i = 0u; i < type->GetPropertyCount(); ++i)
		{
			auto const pProperty = type->GetProperty(i);
			if (pProperty->GetAttributes().HasAttribute<Attributes::CommandLineArg>())
			{
				r.ReSharperBlankLinesAroundElementPrefix();
				r.Printf("\t\tpublic static ");
				PrintTypeInfo(r, tTypes, pProperty->GetMemberTypeInfo(), true);
				r.Printf(" %s()" SEOUL_EOL, pProperty->GetName().CStr());
				r.Printf("\t\t{" SEOUL_EOL);
				r.Printf("\t\t\treturn s_udStaticApi.%s();" SEOUL_EOL, pProperty->GetName().CStr());
				r.Printf("\t\t}" SEOUL_EOL);
			}
		}
	}

	r.Printf("\t}" SEOUL_EOL);
}

static void PrintDataType(
	Printer& r,
	const TypeEmitTable& tTypes,
	Type const* type)
{
	r.Printf("\tpublic sealed class %s", GetEmissionName(type->GetName()).CStr());
	r.Printf(SEOUL_EOL);
	r.Printf("\t{" SEOUL_EOL);

	// Emit Properties.
	for (auto i = 0u; i < type->GetPropertyCount(); ++i)
	{
		auto typeProperty = type->GetProperty(i);

		r.Printf("\t\tpublic ");
		PrintTypeInfo(r, tTypes, typeProperty->GetMemberTypeInfo());
		r.Printf(" %s;", GetEmissionName(typeProperty->GetName()).CStr());
		r.Printf(SEOUL_EOL);
	}
	r.Printf("\t}" SEOUL_EOL);
}

static inline Bool HasPure(Type const* type)
{
	for (auto i = 0u; i < type->GetMethodCount(); ++i)
	{
		// Skip methods contained in parents - treat as an override.
		if (type->GetParentCount() > 0u)
		{
			if (Contains(type->GetParent(0u), type->GetMethod(i)))
			{
				continue;
			}
		}

		if (type->GetMethod(i)->GetTypeInfo().IsConst())
		{
			return true;
		}
	}

	return false;
}

static void AddTypes(TypeEmitTable& tTypes, Type const* parentType)
{
	auto count = parentType->GetPropertyCount();
	for (auto i = 0u; i < count; i++)
	{
		auto const pProp = parentType->GetProperty(i);
		// Skip non-complex properties
		if (pProp->GetMemberTypeInfo().GetSimpleTypeInfo() != SimpleTypeInfo::kComplex
			|| pProp->GetMemberTypeInfo().GetType().TryGetArray() != nullptr
			|| pProp->GetMemberTypeInfo().GetType().TryGetTable() != nullptr)
		{
			continue;
		}

		auto pType = &pProp->GetMemberTypeInfo().GetType();
		auto pEmitType = tTypes.Find(pType);
		if (nullptr != pEmitType)
		{
			if (*pEmitType == EmitType::kEmitType_None)
			{
				*pEmitType = EmitType::kEmitType_Data;
				AddTypes(tTypes, pType);
			}
		}
	}
}

Bool EmitScriptApi(const String& sPath)
{
	// Clean out any existing files before writing new files.
	if (FileManager::Get()->IsDirectory(sPath))
	{
		if (!FileManager::Get()->DeleteDirectory(sPath, true))
		{
			fprintf(stderr, "Failed cleaning up output path \"%s\"\n", sPath.CStr());
			return false;
		}
	}

	if (!FileManager::Get()->CreateDirPath(sPath))
	{
		fprintf(stderr, "Failed creating output path \"%s\"\n", sPath.CStr());
		return false;
	}

	auto const uTypes = Registry::GetRegistry().GetTypeCount();

	// Build a mapping of all types to how they should be emitted
	TypeEmitTable tTypes;
	for (UInt32 i = 0u; i < uTypes; ++i)
	{
		auto type = Registry::GetRegistry().GetType(i);
		(void)tTypes.Insert(type, NeedsEmit(type));
	}

	// Recursively update types for properties so that needed child
	// properties are emitted
	for (auto const& typePair : tTypes)
	{
		if (typePair.Second == EmitType::kEmitType_Data)
		{
			AddTypes(tTypes, typePair.First);
		}
	}

	// Now remove anything which still does not need to be emitted.
	TypeEmitTable tEmittedTypes;
	for (auto const& typePair : tTypes)
	{
		if (typePair.Second != EmitType::kEmitType_None)
		{
			tEmittedTypes.Insert(typePair.First, typePair.Second);
		}
	}
	tTypes.Swap(tEmittedTypes);

	// Now enumerate and emit.
	for (auto const& typePair : tTypes)
	{
		// Check if any of the methods will be [Pure], and emit the necessary using
		// declare if so.
		auto const bHasPure = HasPure(typePair.First);

		Printer printer(typePair.Second);
		printer.Printf("/*" SEOUL_EOL);
		printer.Printf("\t%s.cs" SEOUL_EOL, GetEmissionName(typePair.First->GetName()).CStr());
		printer.Printf("\tAUTO GENERATED - DO NOT MODIFY" SEOUL_EOL);
		printer.Printf("\tAPI FOR NATIVE CLASS INSTANCE" SEOUL_EOL);
		printer.Printf(SEOUL_EOL);
		printer.Printf("\tRun GenerateScriptBindings.bat in the Utilities folder to re-generate bindings." SEOUL_EOL);
		printer.Printf(SEOUL_EOL);
		printer.Printf("\tCopyright (c) 2018-2022 Demiurge Studios Inc.  All rights reserved." SEOUL_EOL);
		printer.Printf("*/" SEOUL_EOL);
		printer.Printf(SEOUL_EOL);

		// Emit [Pure] using if needed.
		if (bHasPure)
		{
			printer.Printf("using System.Diagnostics.Contracts;" SEOUL_EOL);
			printer.Printf(SEOUL_EOL);
		}

		auto pPreprocessor = typePair.First->GetAttribute<Attributes::ScriptPreprocessorDirective>();
		if (nullptr != pPreprocessor)
		{
			printer.Printf("#if %s" SEOUL_EOL, pPreprocessor->m_Name.CStr());
		}

		if (const Byte* sNamespace = GetEmissionNamespaceName(typePair.Second))
		{
			printer.Printf("namespace %s", sNamespace);
			printer.Printf(SEOUL_EOL);
			printer.Printf("{" SEOUL_EOL);

			switch (typePair.Second)
			{
			case EmitType::kEmitType_Data:
				PrintDataType(printer, tTypes, typePair.First);
				break;
			case EmitType::kEmitType_Interface:
				PrintInterfaceType(printer, tTypes, typePair.First);
				break;
			default:
				break;
			}

			printer.Printf("}" SEOUL_EOL);
		}

		if (nullptr != pPreprocessor)
		{
			printer.Printf("#endif" SEOUL_EOL);
		}

		auto const sFile = Path::Combine(sPath, GetEmissionName(typePair.First->GetName()) + ".cs");
		if (!printer.Save(sFile))
		{
			return false;
		}
	}

	return true;
}

} // namespace Seoul::Reflection
