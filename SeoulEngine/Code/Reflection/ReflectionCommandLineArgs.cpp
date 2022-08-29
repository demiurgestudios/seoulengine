/**
 * \file ReflectionCommandLineArgs.cpp
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Atomic32.h"
#include "CommandLineArgWrapper.h"
#include "FromString.h"
#include "Platform.h" // TODO: Can be removed if GetEnvironmentVariable is generalized.
#include "ReflectionAttributes.h"
#include "ReflectionCommandLineArgs.h"
#include "ReflectionDefine.h"
#include "ReflectionMethod.h"
#include "ReflectionRegistry.h"
#include "ReflectionType.h"
#include "ScopedAction.h"
#include "StringUtil.h"

namespace Seoul
{

static const HString kH("h");
static const HString kHelp("help");
static const HString kQuestionMark("?");
static const HString kSetCommandLineArgOffset("SetCommandLineArgOffset");

/* Max column width of args separated from their descriptions. */
static const UInt32 kuMaxArgColumnWidth = 25u;
static const UInt32 kuMaxRemarksWidth = 76u;

SEOUL_BEGIN_TEMPLATE_TYPE(
	CommandLineArgWrapper,                                                 // base type name
	(T),                                                                   // type template signature - SharedPtr<T>
	(typename T),                                                          // template declaration signature - template <typename T>
	("CommandLineArgWrapper<%s>", SEOUL_GET_FULLY_QUALIFIED_TYPE_NAME(T))) // expression used to generate a unique name for specializations of this type
	SEOUL_METHOD(SetCommandLineArgOffset)
SEOUL_END_TYPE()
SEOUL_SPEC_TEMPLATE_TYPE(CommandLineArgWrapper<HString>)
SEOUL_SPEC_TEMPLATE_TYPE(CommandLineArgWrapper<String>)
SEOUL_SPEC_TEMPLATE_TYPE(CommandLineArgWrapper<UInt32>)

namespace
{

#define SEOUL_ERR(fmt, ...) fprintf(stderr, "%s: error: " fmt "\n", GetAppName().CStr(), ##__VA_ARGS__)
#define SEOUL_MSG(fmt, ...) fprintf(stdout, fmt "\n", ##__VA_ARGS__)
#define SEOUL_IDENT "  "

// TODO: Break out into a general utility.
static String GetEnvironmentVar(const String& s)
{
#if SEOUL_PLATFORM_WINDOWS
	WCHAR aBuffer[MAX_PATH] = { 0 };
	DWORD const uResult = ::GetEnvironmentVariableW(
		s.WStr(),
		aBuffer,
		SEOUL_ARRAY_COUNT(aBuffer));

	// Success if uResult != 0 and uResult < SEOUL_ARRAY_COUNT(aBuffer).
	// A value greater than the buffer size means the value is too big,
	// and a value 0 usually means the variable is not found.
	if (uResult > 0u && uResult < SEOUL_ARRAY_COUNT(aBuffer))
	{
		// Make sure we nullptr terminate the buffer in all cases.
		aBuffer[uResult] = (WCHAR)'\0';
		return WCharTToUTF8(aBuffer);
	}
	// Otherwise, punt.
	else
	{
		return String();
	}
#else
	auto const sValue = getenv(s.CStr());
	if (nullptr == sValue)
	{
		return String();
	}
	else
	{
		return String(sValue);
	}
#endif
}

static String GetAppName()
{
#if SEOUL_PLATFORM_WINDOWS
	WCHAR a[MAX_PATH];
	::GetModuleFileNameW(nullptr, a, MAX_PATH);

	return Path::GetFileName(WCharTToUTF8(a));
#else
	// TODO:
	return String();
#endif
}

static Bool RawSetProperty(
	HString name,
	const Reflection::TypeInfo& typeInfo,
	Reflection::SimpleTypeInfo eSimpleTypeInfo,
	void* p,
	Byte const* sInValue,
	Bool* pbPushBackValue = nullptr)
{
	using namespace Reflection;

	// Only boolean simple type support a null/empty value.
	String sValue;
	if (nullptr == sInValue || '\0' == sInValue[0])
	{
		if (SimpleTypeInfo::kBoolean != eSimpleTypeInfo)
		{
			SEOUL_ERR("argument to '%s' is missing (expected 1 value)", name.CStr());
			return false;
		}
	}
	// Otherwise, trim.
	else
	{
		sValue = TrimWhiteSpace(sInValue);
	}

	switch (eSimpleTypeInfo)
	{
	case SimpleTypeInfo::kBoolean:
	{
		auto& r = *((Bool*)p);

		// If value is null, then we're just setting the boolean to true.
		if (nullptr == sInValue || '\0' == sInValue[0])
		{
			r = true;
			return true;
		}
		else
		{
			if (!FromString(sValue, r))
			{
				// Handle the ambiguous case (e.g.) -local <filename>, where <filename>
				// is a positional argument.
				if (nullptr != pbPushBackValue)
				{
					*pbPushBackValue = true;
					r = true;
					return true;
				}
				else
				{
					SEOUL_ERR("'%s' expects boolean", name.CStr());
					return false;
				}
			}

			return true;
		}
	};

	case SimpleTypeInfo::kEnum:
	{
		// First try parsing as an integer.
		Int i = 0;
		if (!FromString(sValue, i))
		{
			// String lookup.
			HString key;
			if (!HString::Get(key, sValue))
			{
				SEOUL_ERR("'%s' expects valid option in set, not '%s'", name.CStr(), sValue.CStr());
				return false;
			}

			auto const& enumInst = *typeInfo.GetType().TryGetEnum();
			if (!enumInst.TryGetValue(key, i))
			{
				SEOUL_ERR("'%s' expects valid option, not '%s'", name.CStr(), sValue.CStr());
				return false;
			}
		}

		auto const uSize = typeInfo.GetSizeInBytes();
		if (uSize >= sizeof(i))
		{
			memcpy(p, &i, sizeof(i));
		}
		else if (2 == uSize)
		{
			Int16 i16 = (Int16)i;
			memcpy(p, &i16, sizeof(i16));
		}
		else if (1 == uSize)
		{
			Int8 i8 = (Int8)i;
			memcpy(p, &i8, sizeof(i8));
		}
		else
		{
			return false;
		}

		return true;
	}

	case SimpleTypeInfo::kInt8:
	{
		auto& r = *((Int8*)p);
		if (!FromString(sValue, r))
		{
			SEOUL_ERR("'%s' expects int8", name.CStr());
			return false;
		}

		return true;
	}

	case SimpleTypeInfo::kInt16:
	{
		auto& r = *((Int16*)p);
		if (!FromString(sValue, r))
		{
			SEOUL_ERR("'%s' expects int16", name.CStr());
			return false;
		}

		return true;
	}

	case SimpleTypeInfo::kInt32:
	{
		auto& r = *((Int32*)p);
		if (!FromString(sValue, r))
		{
			SEOUL_ERR("'%s' expects int32", name.CStr());
			return false;
		}

		return true;
	}

	case SimpleTypeInfo::kInt64:
	{
		auto& r = *((Int64*)p);
		if (!FromString(sValue, r))
		{
			SEOUL_ERR("'%s' expects int64", name.CStr());
			return false;
		}

		return true;
	}

	case SimpleTypeInfo::kFloat32:
	{
		auto& r = *((Float32*)p);
		if (!FromString(sValue, r))
		{
			SEOUL_ERR("'%s' expects float32", name.CStr());
			return false;
		}

		return true;
	}

	case SimpleTypeInfo::kFloat64:
	{
		auto& r = *((Float64*)p);
		if (!FromString(sValue, r))
		{
			SEOUL_ERR("'%s' expects float64", name.CStr());
			return false;
		}

		return true;
	}

	case SimpleTypeInfo::kHString:
	{
		auto& r = *((HString*)p);
		r = HString(sValue);
		return true;
	}

	case SimpleTypeInfo::kString:
	{
		auto& r = *((String*)p);
		r = sValue;
		return true;
	}

	case SimpleTypeInfo::kUInt8:
	{
		auto& r = *((UInt8*)p);
		if (!FromString(sValue, r))
		{
			SEOUL_ERR("'%s' expects uint8", name.CStr());
			return false;
		}

		return true;
	}

	case SimpleTypeInfo::kUInt16:
	{
		auto& r = *((UInt16*)p);
		if (!FromString(sValue, r))
		{
			SEOUL_ERR("'%s' expects uint16", name.CStr());
			return false;
		}

		return true;
	}

	case SimpleTypeInfo::kUInt32:
	{
		auto& r = *((UInt32*)p);
		if (!FromString(sValue, r))
		{
			SEOUL_ERR("'%s' expects uint32", name.CStr());
			return false;
		}

		return true;
	}

	case SimpleTypeInfo::kUInt64:
	{
		auto& r = *((UInt64*)p);
		if (!FromString(sValue, r))
		{
			SEOUL_ERR("'%s' expects uint64", name.CStr());
			return false;
		}

		return true;
	}

	default:
		SEOUL_FAIL("Programmer error.");
		return false;
	};
}

Bool SetProperty(
	Reflection::Property const* pProp,
	Byte const* sInValue,
	Bool* pbPushBackValue = nullptr)
{
	using namespace Reflection;

	auto const& typeInfo = pProp->GetMemberTypeInfo();
	auto const eSimpleTypeInfo = typeInfo.GetSimpleTypeInfo();
	auto const pArg = pProp->GetAttributes().GetAttribute<Attributes::CommandLineArg>();
	auto name = pArg->m_Name;
	if (name.IsEmpty())
	{
		name = pArg->m_ValueLabel;
	}

	// Some special case handling.
	if (SimpleTypeInfo::kComplex == eSimpleTypeInfo)
	{
		if (typeInfo == TypeId< CommandLineArgWrapper<HString> >())
		{
			// TODO: Inconsistent with handling that is not wrapped.
			if (nullptr == sInValue || '\0' == sInValue[0])
			{
				((CommandLineArgWrapper<HString>*)((size_t)pProp->GetOffset()))->GetForWrite() = HString();
				return true;
			}
			else
			{
				return RawSetProperty(
					name,
					TypeId<HString>(),
					SimpleTypeInfo::kHString,
					&((CommandLineArgWrapper<HString>*)((size_t)pProp->GetOffset()))->GetForWrite(),
					sInValue);
			}
		}
		else if (typeInfo == TypeId< CommandLineArgWrapper<String> >())
		{
			// TODO: Inconsistent with handling that is not wrapped.
			if (nullptr == sInValue || '\0' == sInValue[0])
			{
				((CommandLineArgWrapper<String>*)((size_t)pProp->GetOffset()))->GetForWrite() = String();
				return true;
			}
			else
			{
				return RawSetProperty(
					name,
					TypeId<HString>(),
					SimpleTypeInfo::kString,
					&((CommandLineArgWrapper<String>*)((size_t)pProp->GetOffset()))->GetForWrite(),
					sInValue);
			}
		}
		else if (typeInfo == TypeId< CommandLineArgWrapper<UInt32> >())
		{
			return RawSetProperty(
				name,
				TypeId<UInt32>(),
				SimpleTypeInfo::kUInt32,
				&((CommandLineArgWrapper<UInt32>*)((size_t)pProp->GetOffset()))->GetForWrite(),
				sInValue);
		}
	}

	// Common handling.
	return RawSetProperty(
		name,
		typeInfo,
		eSimpleTypeInfo,
		(void*)((size_t)pProp->GetOffset()),
		sInValue,
		pbPushBackValue);
}

class Args SEOUL_SEALED
{
public:
	typedef HashTable<HString, Pair<Reflection::Property const*, Bool>, MemoryBudgets::Reflection> NamedArguments;
	typedef Vector< Pair<Reflection::Property const*, Bool>, MemoryBudgets::Reflection > PositionalArguments;

	NamedArguments m_t;
	PositionalArguments m_v;

	Bool ConsumeAll(String const* iBegin, String const* iEnd)
	{
		if (!ConsumeCommandLine(iBegin, iEnd))
		{
			return false;
		}

		if (!ConsumeEnvironment())
		{
			return false;
		}

		if (!Verify())
		{
			return false;
		}

		return true;
	}

private:
	Bool ApplyNamedArg(
		Byte const* sKey,
		Byte const* sKeyEnd,
		Byte const* sValue,
		Int32 iCommandLineOffset,
		Bool* pbPushBackValue = nullptr)
	{
		using namespace Reflection;

		Bool bPrefix = false;
		HString key;
		if (!HString::Get(key, sKey, (UInt32)(sKeyEnd - sKey)))
		{
			// Check for prefix handling - prefix switches have the format (e.g.)
			// -D<key>, -D<key>=<value>, etc. Currently prefix switches can only
			// be a single letter.
			if (HString::Get(key, sKey, 1u))
			{
				auto p = m_t.Find(key);
				if (nullptr != p &&
					p->First->GetAttributes().GetAttribute<Attributes::CommandLineArg>()->m_bPrefix)
				{
					bPrefix = true;
				}
			}

			if (!bPrefix)
			{
				SEOUL_ERR("invalid argument '%s'", String(sKey, (UInt32)(sKeyEnd - sKey)).CStr());
				return false;
			}
		}

		// TODO: Generalize.
		if (kH == key || kHelp == key || kQuestionMark == key)
		{
			Reflection::CommandLineArgs::PrintHelp();
			return false;
		}

		auto p = m_t.Find(key);
		if (nullptr == p)
		{
			SEOUL_ERR("invalid argument '%s'", String(sKey, (UInt32)(sKeyEnd - sKey)).CStr());
			return false;
		}

		if (p->Second)
		{
			// TODO: Would like to still apply duplicate tracking to the the prefix+key.
			if (!bPrefix)
			{
				SEOUL_ERR("argument '%s' is defined twice", String(sKey, (UInt32)(sKeyEnd - sKey)).CStr());
				return false;
			}
		}

		auto pProp = p->First;
		if (bPrefix)
		{
			// TODO: Shouldn't need this requirement in general.
			//
			// Prefix arguments are currently expected to be -D<key>=<value>
			// or -D<key>. In both cases, value is a string and will be assumed
			// empty if not explicitly specified.
			auto pTable = pProp->GetMemberTypeInfo().GetType().TryGetTable();
			SEOUL_ASSERT(nullptr != pTable); // Enforced by Gather().

			// Assemble.
			auto const sFinalValue(TrimWhiteSpace(nullptr == sValue ? "" : sValue));

			Reflection::WeakAny thisPointer;
			if (!pProp->TryGetPtr(Reflection::WeakAny(), thisPointer) ||
				!pTable->TryOverwrite(thisPointer, String(sKey + 1, (UInt32)(sKeyEnd - (sKey + 1))), sFinalValue))
			{
				SEOUL_ERR("invalid prefix argument '%s'", String(sKey, (UInt32)(sKeyEnd - sKey)).CStr());
				return false;
			}
		}
		else
		{
			if (!SetProperty(pProp, sValue, pbPushBackValue))
			{
				return false;
			}
		}

		// Fill in offset if specified.
		{
			auto const& type = pProp->GetMemberTypeInfo().GetType();
			if (auto pMethod = type.GetMethod(kSetCommandLineArgOffset))
			{
				auto const thisPointer = type.GetPtrUnsafe((void*)((size_t)pProp->GetOffset()));
				Reflection::MethodArguments args;
				args[0] = iCommandLineOffset;
				if (!pMethod->TryInvoke(thisPointer, args))
				{
					SEOUL_ERR("failed invoking '%s' on named argument '%s'",
						kSetCommandLineArgOffset.CStr(),
						String(sKey, (UInt32)(sKeyEnd - sKey)).CStr());
					return false;
				}
			}
		}

		p->Second = true;
		return true;
	}
	Bool ApplyPositionalArg(Int iPosition, Byte const* sValue, Int32 iCommandLineOffset)
	{
		using namespace Reflection;
		using namespace Reflection::Attributes;

		// Checking.
		if (iPosition < 0 || (UInt32)iPosition >= m_v.GetSize())
		{
			SEOUL_ERR("too many positional arguments, at most %u expected", m_v.GetSize());
			return false;
		}

		auto& e = m_v[iPosition];
		auto pProp = e.First;
		if (e.Second)
		{
			SEOUL_ERR("position argument '%s' is defined twice", pProp->GetAttributes().GetAttribute<CommandLineArg>()->m_ValueLabel.CStr());
			return false;
		}

		if (!SetProperty(pProp, sValue))
		{
			return false;
		}

		// Fill in offset if specified.
		{
			auto const& type = pProp->GetMemberTypeInfo().GetType();
			if (auto pMethod = type.GetMethod(kSetCommandLineArgOffset))
			{
				auto const thisPointer = type.GetPtrUnsafe((void*)((size_t)pProp->GetOffset()));
				Reflection::MethodArguments args;
				args[0] = iCommandLineOffset;
				if (!pMethod->TryInvoke(thisPointer, args))
				{
					SEOUL_ERR("failed invoking '%s' on positional argument '%s'",
						kSetCommandLineArgOffset.CStr(),
						pProp->GetAttributes().GetAttribute<CommandLineArg>()->m_ValueLabel.CStr());
					return false;
				}
			}
		}

		e.Second = true;
		return true;
	}

	Bool ConsumeCommandLine(String const* iBegin, String const* iEnd)
	{
		Int iPosition = 0;
		for (auto i = iBegin; iEnd != i; ++i)
		{
			auto const& s = *i;
			auto const iCommandLineOffset = (Int32)(i - iBegin);

			// Named argument.
			if ('-' == s[0] || '/' == s[0])
			{
				auto iMinusCount = 1;

				// Check for a second minus.
				if ('/' != s[0] && '-' == s[1])
				{
					++iMinusCount;
				}

				// Start of the key.
				auto const sKey = s.CStr() + iMinusCount;

				// Invalid key, skip, and also warn.
				if ('\0' == sKey[0] || '-' == sKey[0] || '=' == sKey[0])
				{
					SEOUL_ERR("Invalid named arg: '%s'", s.CStr());
					return false;
				}

				// Check for an '=', otherwise the entire argument is the key.
				const char* sKeyEnd = strchr(sKey, '=');
				Byte const* sValue = nullptr;
				if (nullptr == sKeyEnd)
				{
					sKeyEnd = sKey + s.GetSize() - iMinusCount;
				}
				else
				{
					sValue = sKeyEnd + 1;
				}

				Bool bPushBackValue = false;
				Bool* pbPushBackValue = nullptr;

				// If we didn't find a value (it's argument style -key value or --key value *not*
				// -key=value), then check the next arg, if there is one.
				if (nullptr == sValue)
				{
					// Terminator case, assume the key has an empty value.
					if (i + 1 >= iEnd)
					{
						sValue = "";
					}
					// Otherwise, check i.
					else
					{
						auto const& s2 = *(i + 1);

						// Cannot start with a dash or a slash - if this happens, we assume the
						// current value is empty and don't advance.
						if ('/' == s2[0] || '-' == s2[0])
						{
							sValue = "";
						}
						// Otherwise, set the value and skip i.
						else
						{
							sValue = s2.CStr();
							++i;

							// Push back is potentially allowed for this value.
							pbPushBackValue = &bPushBackValue;
						}
					}
				}

				// One way or another, if we get here, we must have non-null key, key end, and value.
				SEOUL_ASSERT(nullptr != sKey);
				SEOUL_ASSERT(nullptr != sKeyEnd);
				SEOUL_ASSERT(nullptr != sValue);

				// Apply.
				if (!ApplyNamedArg(sKey, sKeyEnd, sValue, iCommandLineOffset, pbPushBackValue))
				{
					return false;
				}

				// Handle push back.
				if (nullptr != pbPushBackValue && *pbPushBackValue)
				{
					--i;
				}
			}
			// Positional argument.
			else
			{
				// Apply.
				if (!ApplyPositionalArg(iPosition, s.CStr(), iCommandLineOffset))
				{
					return false;
				}

				// If this position arg is marked "terminator", it means we've reached
				// the end of consumption - remaining args (if any) are used by the environment.
				//
				// Mark the end point and terminate.
				if (m_v[iPosition].First->GetAttributes().GetAttribute<Reflection::Attributes::CommandLineArg>()->m_bTerminator)
				{
					// Done.
					iPosition++;
					return true;
				}

				// Advance and continue.
				iPosition++;
			}
		}

		return true;
	}
	Bool ConsumeEnvironment()
	{
		// TODO: Allow prefix to be configured.
		// Only applies to named, converted to uppercase and prefixed with SEOUL_ENV_
		for (auto const& pair : m_t)
		{
			auto& e = pair.Second;

			// Already specified.
			if (e.Second) { continue; }

			// Get key.
			String const sKey("SEOUL_ENV_" + String(pair.First).ToUpperASCII());
			String const sValue(GetEnvironmentVar(sKey));
			if (!sValue.IsEmpty())
			{
				if (!SetProperty(e.First, sValue.CStr()))
				{
					return false;
				}

				e.Second = true;
			}
		}

		return true;
	}

	Bool Verify() const
	{
		using namespace Reflection;
		using namespace Reflection::Attributes;

		// Check that all arguments mark required were specified.
		for (auto const& e : m_v)
		{
			if (!e.Second)
			{
				auto pArg = e.First->GetAttributes().GetAttribute<CommandLineArg>();
				if (pArg->m_bRequired)
				{
					SEOUL_ERR("missing required argument '%s'", pArg->m_ValueLabel.CStr());
					return false;
				}
			}
		}
		for (auto const& pair : m_t)
		{
			auto const& e = pair.Second;
			if (!e.Second)
			{
				auto pArg = e.First->GetAttributes().GetAttribute<CommandLineArg>();
				if (pArg->m_bRequired)
				{
					SEOUL_ERR("required argument '%s' was not specified", pArg->m_Name.CStr());
					return false;
				}
			}
		}

		// Check that only the last arg is a terminator, if any.
		{
			auto const u = m_v.GetSize();
			for (UInt32 i = 0u; i < u; ++i)
			{
				auto pArg = m_v[i].First->GetAttributes().GetAttribute<CommandLineArg>();
				if (pArg->m_bTerminator)
				{
					if (i + 1 != u)
					{
						SEOUL_ERR("argument '%s' is marked as terminator but is not the last positional arg.", pArg->m_Name.CStr());
						return false;
					}
				}
			}
		}

		return true;
	}
};

static Args Gather()
{
	using namespace Reflection;
	using namespace Reflection::Attributes;

	Args args;
	auto const uCount = Registry::GetRegistry().GetTypeCount();

	// Gather disables first.
	HashSet<HString> disable;
	for (UInt32 i = 0u; i < uCount; ++i)
	{
		auto const& type = *Registry::GetRegistry().GetType(i);
		if (auto pDisable = type.GetAttribute<Attributes::DisableCommandLineArgs>())
		{
			disable.Insert(pDisable->m_TypeName);
		}
	}

	for (UInt32 i = 0u; i < uCount; ++i)
	{
		auto const& type = *Registry::GetRegistry().GetType(i);

		// Skip.
		if (disable.HasKey(type.GetName()))
		{
			continue;
		}

		auto const uProps = type.GetPropertyCount();
		for (UInt32 j = 0u; j < uProps; ++j)
		{
			auto const& prop = *type.GetProperty(j);
			auto const pArg = prop.GetAttributes().GetAttribute<CommandLineArg>();
			if (nullptr == pArg)
			{
				// Not interesting.
				continue;
			}

			// Sanity, command-line args are only valid as static properties.
			SEOUL_ASSERT_MESSAGE(prop.IsStatic(), String::Printf("%s::%s is marked as CommandLineArg but is not static",
				type.GetName().CStr(),
				prop.GetName().CStr()).CStr());
			// Skip.
			if (!prop.IsStatic())
			{
				continue;
			}

			// Track.
			if (!pArg->m_Name.IsEmpty())
			{
				auto const e = args.m_t.Insert(pArg->m_Name, MakePair(&prop, false));
				SEOUL_ASSERT_MESSAGE(e.Second, String::Printf("%s::%s as named CommandLineArg '%s' is defined twice",
					type.GetName().CStr(),
					prop.GetName().CStr(),
					pArg->m_Name.CStr()).CStr());
			}
			else
			{
				SEOUL_ASSERT_MESSAGE(pArg->m_iPosition >= 0, String::Printf("%s::%s as positional CommandLineArg has negative position",
					type.GetName().CStr(),
					prop.GetName().CStr()).CStr());
				if (pArg->m_iPosition < 0) { continue; }

				args.m_v.Resize(Max((UInt32)pArg->m_iPosition + 1, args.m_v.GetSize()));
				auto& e = args.m_v[pArg->m_iPosition];
				SEOUL_ASSERT_MESSAGE(nullptr == e.First, String::Printf("%s::%s as positional CommandLineArg '%d' overlaps arg %s::%s at the same position",
					type.GetName().CStr(),
					prop.GetName().CStr(),
					pArg->m_iPosition,
					e.First->GetClassTypeInfo().GetType().GetName().CStr(),
					e.First->GetName().CStr()).CStr());
				if (nullptr != e.First) { continue; }

				e = MakePair(&prop, false);
			}
		}
	}

	// Last step - verify that all position args are filled.
	{
		auto const u = args.m_v.GetSize();
		for (UInt32 i = 0u; i < u; ++i)
		{
			SEOUL_ASSERT_MESSAGE(nullptr != args.m_v[i].First, String::Printf("%u positional CommandLineArgs have been defined but no arg for position '%u' is defined",
				u,
				i).CStr());
			if (nullptr == args.m_v[i].First)
			{
				// Resize position to this nil value and stop processing.
				args.m_v.Resize(i);
				break;
			}
		}
	}

	return args;
}

static Bool DoParse(String const* iBegin, String const* iEnd)
{
	// TODO: Simplification given current application setup, but
	// ugly and should be fixed if we can unify how different SeoulEngine
	// apps are assembled, configured, and bootstrapped.
	Bool bPrev = false;
	auto const scoped(MakeScopedAction(
		[&]() { bPrev = g_bInMain; g_bInMain = true; },
		[&]() { g_bInMain = bPrev; }));

	// Gather command line arguments.
	auto args(Gather());

	// Now process.
	return args.ConsumeAll(iBegin, iEnd);
}

} // namespace anonymous

namespace Reflection
{

/** Enter main - call once per application. */
Bool CommandLineArgs::Parse(String const* iBegin, String const* iEnd)
{
	// Now process arguments.
	return DoParse(iBegin, iEnd);
}

Bool CommandLineArgs::Parse(Byte const* const* iBegin, Byte const* const* iEnd)
{
	Vector<String> vs;
	for (auto i = iBegin; iEnd != i; ++i)
	{
		vs.PushBack(*i);
	}

	return DoParse(vs.Begin(), vs.End());
}

Bool CommandLineArgs::Parse(wchar_t const* const* iBegin, wchar_t const* const* iEnd)
{
	Vector<String> vs;
	for (auto i = iBegin; iEnd != i; ++i)
	{
		vs.PushBack(WCharTToUTF8(*i));
	}

	return DoParse(vs.Begin(), vs.End());
}

void CommandLineArgs::PrintHelp()
{
	// Gather command line arguments.
	auto args(Gather());

	// Utility.
	auto const computeWidth([&](HString name, Reflection::Property const* pProp) -> UInt32
	{
		auto uReturn = name.GetSizeInBytes() + 1 + 1; // +1 for the leading '-', +1 for at least 1 trailing space.
		// Check for and add value.
		if (auto pArg = pProp->GetAttributes().GetAttribute<Attributes::CommandLineArg>())
		{
			if (!pArg->m_ValueLabel.IsEmpty())
			{
				uReturn += pArg->m_ValueLabel.GetSizeInBytes() + 2; // +2 for surrounding <>.
				if (!pArg->m_bPrefix)
				{
					uReturn += 1u; // +1 for leading ' ',
				}
			}
		}

		return uReturn;
	});

	fprintf(stdout, "\nUSAGE: %s", GetAppName().CStr());
	if (!args.m_t.IsEmpty())
	{
		fprintf(stdout, " [options]");
	}

	Bool bHasRemarks = false;
	for (auto const& e : args.m_v)
	{
		auto const pProp = e.First;
		if (auto pArg = pProp->GetAttributes().GetAttribute<Attributes::CommandLineArg>())
		{
			if (pArg->m_bRequired)
			{
				fprintf(stdout, " %s", pArg->m_ValueLabel.CStr());
			}
			else
			{
				fprintf(stdout, " [%s]", pArg->m_ValueLabel.CStr());
			}
		}

		// We add a remarks section either when specified explicitly, or to enumerate
		// valid enum values.
		bHasRemarks =
			bHasRemarks ||
			pProp->GetAttributes().HasAttribute<Attributes::Remarks>() ||
			pProp->GetMemberTypeInfo().GetSimpleTypeInfo() == SimpleTypeInfo::kEnum;
	}

	fprintf(stdout, "\n");

	Vector<HString> vNamed;
	if (!args.m_t.IsEmpty())
	{
		fprintf(stdout, "\nOPTIONS:\n");

		UInt32 uColumnWidth = 0u;
		for (auto const& e : args.m_t)
		{
			uColumnWidth = Max(uColumnWidth, computeWidth(e.First, e.Second.First) + 1);
			vNamed.PushBack(e.First);
		}
		QuickSort(vNamed.Begin(), vNamed.End(), [](HString a, HString b) { return (strcmp(a.CStr(), b.CStr()) < 0); });

		// Clamp.
		uColumnWidth = Min(uColumnWidth, kuMaxArgColumnWidth);

		for (auto const& name : vNamed)
		{
			auto const& e = *args.m_t.Find(name);
			auto const pProp = e.First;
			auto const pArg = pProp->GetAttributes().GetAttribute<Attributes::CommandLineArg>();

			fprintf(stdout, "  -%s", name.CStr());
			if (!pArg->m_ValueLabel.IsEmpty())
			{
				auto const sPrefix = (pArg->m_bPrefix ? "" : " ");
				fprintf(stdout, "%s<%s>", sPrefix, pArg->m_ValueLabel.CStr());
			}

			if (auto pDesc = pProp->GetAttributes().GetAttribute<Attributes::Description>())
			{
				auto const uWidth = computeWidth(name, e.First);
				UInt32 uSpacing = 0u;
				if (uWidth > uColumnWidth)
				{
					uSpacing = 1;
				}
				else
				{
					uSpacing = (uWidth - uColumnWidth);
				}

				fprintf(stdout, "%*s%s", uSpacing, "", pDesc->m_DescriptionText.CStr());
			}
			fprintf(stdout, "\n");

			// We add a remarks section either when specified explicitly, or to enumerate
			// valid enum values.
			bHasRemarks =
				bHasRemarks ||
				pProp->GetAttributes().HasAttribute<Attributes::Remarks>() ||
				pProp->GetMemberTypeInfo().GetSimpleTypeInfo() == SimpleTypeInfo::kEnum;
		}
	}

	if (bHasRemarks)
	{
		fprintf(stdout, "\nREMARKS:\n");

		auto const remarksUtil = [&](Byte const* sPrefix, Byte const* s, UInt32 u)
		{
			while (u > kuMaxRemarksWidth)
			{
				// Find the last space.
				UInt32 uLength = kuMaxRemarksWidth;
				while (uLength > 0u && ' ' != s[uLength])
				{
					--uLength;
				}

				if (0u == uLength)
				{
					break;
				}
				else
				{
					fprintf(stdout, "%s%*.*s\n", sPrefix, 0u, uLength, s);
					++uLength; // Also exclude the space.
					s += uLength;
					u -= uLength;
				}

				sPrefix = "    ";
			}

			fprintf(stdout, "%s%s\n", sPrefix, s);
		};

		auto const remarks = [&](HString label, Reflection::Property const* pProp)
		{
			auto const pRemarks = pProp->GetAttributes().GetAttribute<Attributes::Remarks>();
			if (nullptr != pRemarks)
			{
				Byte const* sPrefix = "  - ";
				Byte const* s = pRemarks->m_sRemarks.CStr();
				UInt32 u = pRemarks->m_sRemarks.GetSizeInBytes();
				remarksUtil(sPrefix, s, u);
			}
			// Auto generate remarks for enums.
			else if (pProp->GetMemberTypeInfo().GetSimpleTypeInfo() == SimpleTypeInfo::kEnum)
			{
				auto pEnum = pProp->GetMemberTypeInfo().GetType().TryGetEnum();
				UInt32 uColumnWidth = 0u;
				for (auto const& e : pEnum->GetNames())
				{
					uColumnWidth = Max(uColumnWidth, e.GetSizeInBytes() + 1 /* trailing space */ + 2 /* prefix chars */);
				}

				fprintf(stdout, "  - <%s>:\n", label.CStr());
				for (UInt32 i = 0u; i < pEnum->GetNames().GetSize(); ++i)
				{
					auto const& e = pEnum->GetNames()[i];
					if (e.IsEmpty())
					{
						continue;
					}

					auto const& attrs = pEnum->GetAttributes()[i];

					fprintf(stdout, "    * %s", e.CStr());
					if (auto pDesc = attrs.GetAttribute<Attributes::Description>())
					{
						// To column width.
						auto const uWidth = e.GetSizeInBytes() + 2 /* prefix chars */;
						UInt32 uSpacing = 0u;
						if (uWidth > uColumnWidth)
						{
							uSpacing = 1;
						}
						else
						{
							uSpacing = (uWidth - uColumnWidth);
						}
						fprintf(stdout, "%*s%s\n", uSpacing, "", pDesc->m_DescriptionText.CStr());
					}
					else
					{
						fprintf(stdout, "\n");
					}
				}
			}
		};

		for (auto const& e : args.m_v)
		{
			auto const pProp = e.First;
			auto const pCmd = pProp->GetAttributes().GetAttribute<Attributes::CommandLineArg>();

			remarks(pCmd->m_ValueLabel, pProp);
		}

		for (auto const& name : vNamed)
		{
			auto const& e = *args.m_t.Find(name);
			auto const pProp = e.First;
			auto const pCmd = pProp->GetAttributes().GetAttribute<Attributes::CommandLineArg>();

			remarks(pCmd->m_Name, pProp);
		}
	}
}

} // namespace Reflection

} // namespace Seoul
