/**
 * \file SeoulHString.h
 * \brief HString is an interned string class. Strings are stored
 * in a global hash table. The HString object itself is a 32-bit index
 * into the table. HString should exhibit similar behavior to a symbol table.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SEOUL_HSTRING_H
#define SEOUL_HSTRING_H

#include "Atomic32.h"
#include "FixedArray.h"
#include "FromString.h"
#include "PerThreadStorage.h"
#include "Prereqs.h"
#include "UnsafeHandle.h"
namespace Seoul { class DataStore; }

#ifdef __OBJC__
#import <Foundation/NSString.h>
#endif

namespace Seoul
{

// Forward declarations
class String;

template <typename T>
struct HStringDataProperties
{
};

template <>
struct HStringDataProperties<UInt16>
{
	static const UInt32 GlobalArraySize = 0x10000;
	static const UInt32 IndexTypeBits = 16u;
	static const size_t HandleMask = 0x0000FFFF;
};

template <>
struct HStringDataProperties<UInt32>
{
	static const UInt32 GlobalArraySize = 0x40000;
	static const UInt32 IndexTypeBits = 32u;
	static const size_t HandleMask = 0xFFFFFFFF;
};

/**
 * Stores the data referneced by an HString. An HString itself
 * is a 32-bit handle into a static table of HString data, making HStrings
 * fast to compare and globally unique. HStringData is not meant to be used
 * outside of HString.
 */
struct HStringData SEOUL_SEALED
{
	typedef UInt32 InternalIndexType;

	HStringData()
		: m_pData("")
		, m_uHashValue(0u)
		, m_zSizeInBytes(0u)
		, m_bStaticReadOnlyMemory(true)
	{
	}

	Byte const* m_pData;
	UInt32 m_uHashValue;
	UInt16 m_zSizeInBytes;
	Bool m_bStaticReadOnlyMemory;
}; // struct HStringData

/**
 * Data structure of statistics used to track performance and usage
 * of global HString data.
 */
struct HStringStats
{
	Atomic32 m_TotalHStrings;
	Atomic32 m_TotalStaticAllocatedHStrings;
	Atomic32 m_TotalStaticAllocatedHStringMemory;
	Atomic32 m_CollisionCount;
	Atomic32 m_WorstCollision;

	/**
	 * HString calls this method to update the total collision count
	 * (the number of times the hash insert loop needed to iterate before
	 * finding an existing value or a free slot), as well as the worst count
	 * for a single HString construct attempt.
	 */
	void UpdateCollisionCount(Atomic32Type collisionCount)
	{
		Atomic32Type currentWorstCollision = m_WorstCollision;
		while (currentWorstCollision < collisionCount)
		{
			if (currentWorstCollision == m_WorstCollision.CompareAndSet(
				collisionCount,
				currentWorstCollision))
			{
				return;
			}

			currentWorstCollision = m_WorstCollision;
		}

		m_CollisionCount += collisionCount;
	}
};

/**
 * Object that contains the global data shared by all HStrings.
 */
class HStringGlobalData SEOUL_SEALED
{
public:
	HStringGlobalData();
	~HStringGlobalData();

#if defined(SEOUL_SERVER_BUILD) // Server support scoped replacement of the storage pointer.
	void Clear();
#endif // /#if defined(SEOUL_SERVER_BUILD)

private:
	friend class HString;

	typedef FixedArray<HStringData*, HStringDataProperties<HStringData::InternalIndexType>::GlobalArraySize> GlobalArrayTable;

	GlobalArrayTable m_apGlobalArrayTable;
	HStringStats m_HStringStats;

	SEOUL_DISABLE_COPY(HStringGlobalData);
}; // class HStringGlobalData

/**
 * Utility class - if you pass this class to an HString, the HString will assume
 * that the underlying string is a static const literal and will store a pointer to
 * the string data, instead of copying the data internally.
 *
 * \warning It is an error and will result in undefined behavior if this class is
 * constructed with a string that is not a static const string literal.
 */
struct CStringLiteral SEOUL_SEALED
{
	template <size_t zStringArrayLengthInBytes>
	explicit CStringLiteral(Byte const (&aString)[zStringArrayLengthInBytes])
		: m_sString(aString)
		, m_zStringLengthInBytes(zStringArrayLengthInBytes - 1u)
	{
		SEOUL_STATIC_ASSERT_MESSAGE(zStringArrayLengthInBytes > 0u, "Literal strings should never be 0 size.");
		SEOUL_ASSERT(strlen(m_sString) == m_zStringLengthInBytes);
	}

	Byte const* const m_sString;
	UInt32 const m_zStringLengthInBytes;
}; // struct CStringLiteral

/**
 * An immutable UTF-8 string class, where strings are globally cached
 * and guaranteed unique. The string is stored in a global, static
 * array. The HString structure itself contains only a 32-bit unsigned index which
 * is used to lookup the string data when needed.
 *
 * HString should not be used for regular string processing. It should
 * only be used for consistent, global identifiers - strings that are used
 * as symbols, strings that are short, and have a very limited set. Using
 * HString for purposes that generate a lot of temporary strings or string
 * variations will quickly fill the global HString table, either exhausting
 * the table or causing very poor performance.
 */
class HString SEOUL_SEALED
{
public:
	/** Get the HString table as a DataStore (root will be an array). For debugging. */
	static void GetAllHStrings(DataStore& rDataStore);

	/**
	 * Sorts & logs all hstrings for analysis
	 */
	static void LogAllHStrings();

	/**
	 * @return Performance and usage stats of the HString class.
	 */
	static HStringStats GetHStringStats()
	{
		return GetGlobalData().m_HStringStats;
	}

	/**
	 * Converts the HString handle to a generic UnsafeHandle.
	 * This function can be used to pass an HString as void* user data.
	 */
	static UnsafeHandle ToUnsafeHandle(HString hstring)
	{
		return UnsafeHandle(((size_t)0u) | hstring.m_hHandle);
	}

	/**
	 * Converts a Systemhandle to an HString handle to a generic UnsafeHandle.
	 * This function can be used to pass an HString as void* user data.
	 */
	static HString ToHString(UnsafeHandle handle)
	{
		static const size_t kMask = HStringDataProperties<HStringData::InternalIndexType>::HandleMask;

		size_t rawValue = StaticCast<size_t>(handle);

		HString ret;
		ret.m_hHandle = (HStringData::InternalIndexType)(rawValue & kMask);
		return ret;
	}

	/**
	 * Given a raw cstring, checks for and returns an HString, only if that HString already
	 * exists. This function can be used to query for HStrings in contexts where the caller
	 * knows that only valid HStrings will already have been created, to avoid creating
	 * spurious additional HStrings.
	 */
	static Bool Get(HString& rOut, Byte const* sString, UInt32 zStringLengthInBytes, Bool bCaseInsensitive = false)
	{
		return InternalGet(rOut, sString, zStringLengthInBytes, bCaseInsensitive, false);
	}

	/**
	 * Variation of Get() for CString literals.
	 */
	static Bool Get(HString& rOut, const CStringLiteral& sLiteral, Bool bCaseInsensitive = false)
	{
		return InternalGet(rOut, sLiteral.m_sString, sLiteral.m_zStringLengthInBytes, bCaseInsensitive, true);
	}

	/**
	 * Convenience function that allows Get to be called
	 * without a size argument.
	 */
	static Bool Get(HString& rOut, Byte const* sString, Bool bCaseInsensitive = false)
	{
		return InternalGet(rOut, sString, StrLen(sString), bCaseInsensitive, false);
	}

	// Variation of Get() for Seoul::String.
	static Bool Get(HString& rOut, const String& s, Bool bCaseInsensitive = false);

	/**
	 * When called, sets the "canonical" version of a the string data underlying
	 * an HString - this only has meaning when using the bCaseInsensitive argument to
	 * construct an HString. This will be the string casing that will be returned when
	 * an HString is constructed with this argument, when calling CStr() (with caveats,
	 * see notes).
	 *
	 * @return True if the canonical string was updated, false otherwise. This function
	 * will return false if a string that already matches the canonical version already exists
	 * in the HString table, ignoring case.
	 *
	 * This function must be called before any string that matches it
	 * is inserted into the HString table (ignoring case). If it is not, the
	 * canonical version will be whatever version was set first and cannot
	 * be changed.
	 */
	static Bool SetCanonicalString(Byte const* sString, UInt32 zStringLengthInBytes)
	{
		HString hstring;
		return hstring.InternalConstruct(sString, zStringLengthInBytes, true, false);
	}

	/**
	 * Variation of SetCanonicalString for constant literal strings.
	 */
	static Bool SetCanonicalString(const CStringLiteral& sLiteral)
	{
		HString hstring;
		return hstring.InternalConstruct(sLiteral.m_sString, sLiteral.m_zStringLengthInBytes, true, true);
	}

	/**
	 * Convenience function that allows SetCanonicalString to be called
	 * without a size argument.
	 */
	static Bool SetCanonicalString(Byte const* sString)
	{
		return SetCanonicalString(sString, StrLen(sString));
	}

	// Convenience function that allows SetCanonicalString to be called
	// with a String argument.
	static Bool SetCanonicalString(const String& sString);

	HString()
		: m_hHandle(0u)
	{}

	/**
	 * Constructs an HString from sLiteral.
	 *
	 * @param[in] bCaseInsensitive Passing true changes the HString
	 * constructor so that it will resolve to any string that matches
	 * the input string without considering case. Note that case
	 * insensitivity in this case is only in ASCII characters, this does
	 * not properly handle case insensitivity of Unicode characters.
	 */
	HString(const CStringLiteral& sLiteral, Bool bCaseInsensitive = false)
		: m_hHandle(0u)
	{
		InternalConstruct(sLiteral.m_sString, sLiteral.m_zStringLengthInBytes, bCaseInsensitive, true);
	}

	/**
	 * Constructs an HString from sString.
	 *
	 * @param[in] bCaseInsensitive Passing true changes the HString
	 * constructor so that it will resolve to any string that matches
	 * the input string without considering case. Note that case
	 * insensitivity in this case is only in ASCII characters, this does
	 * not properly handle case insensitivity of Unicode characters.
	 */
	explicit HString(Byte const* sString, Bool bCaseInsensitive = false)
		: m_hHandle(0u)
	{
		InternalConstruct(sString, StrLen(sString), bCaseInsensitive, false);
	}

	/**
	 * Constructs an HString from sString.
	 *
	 * @param[in] bCaseInsensitive Passing true changes the HString
	 * constructor so that it will resolve to any string that matches
	 * the input string without considering case. Note that case
	 * insensitivity in this case is only in ASCII characters, this does
	 * not properly handle case insensitivity of Unicode characters.
	 */
	HString(Byte const* sString, UInt32 zStringLengthInBytes, Bool bCaseInsensitive = false)
		: m_hHandle(0u)
	{
		InternalConstruct(sString, zStringLengthInBytes, bCaseInsensitive, false);
	}

	explicit HString(const String& sString, Bool bCaseInsensitive = false);

	HString(const HString& hstring)
		: m_hHandle(hstring.m_hHandle)
	{}

	~HString()
	{}

	HString& operator=(HString hstring)
	{
		m_hHandle = hstring.m_hHandle;
		return *this;
	}

	/**
	 * Returns a 32-bit hash value based on the string attached
	 * to this HString.
	 */
	UInt32 GetHash() const
	{
		return GetGlobalData().m_apGlobalArrayTable[m_hHandle]->m_uHashValue;
	}

	/**
	 * The size in bytes of the string data attached to this
	 * HString, excluding the null terminator.
	 */
	UInt32 GetSizeInBytes() const
	{
		return GetGlobalData().m_apGlobalArrayTable[m_hHandle]->m_zSizeInBytes;
	}

	/**
	 * Gets the cstring data attached to this HString.
	 */
	const Byte* CStr() const
	{
		// All in-use HStrings should be defined. If a nullptr or empty string
		// was used to construct an HString, it will use the 0th HString,
		// which is a defined, empty string. If with hit nullptr data here,
		// it means the HString handle is set to an invalid, unallocated
		// entry in the HString table.
		SEOUL_ASSERT(
			nullptr != GetGlobalData().m_apGlobalArrayTable[m_hHandle] &&
			nullptr != GetGlobalData().m_apGlobalArrayTable[m_hHandle]->m_pData);

		return GetGlobalData().m_apGlobalArrayTable[m_hHandle]->m_pData;
	}

#ifdef __OBJC__
	/**
	 * Converts this HString to an NSString.  The returned string is
	 * autoreleased.
	 */
	NSString *ToNSString() const SEOUL_RETURNS_AUTORELEASED
	{
		return [NSString stringWithUTF8String:CStr()];
	}
#endif

	// Convenience functions for HString -> integer conversions.
	Bool ToInt8(Int8& iValue) const { return FromString(CStr(), iValue); }
	Bool ToInt16(Int16& iValue) const { return FromString(CStr(), iValue); }
	Bool ToInt32(Int32& iValue) const { return FromString(CStr(), iValue); }
	Bool ToInt64(Int64& iValue) const { return FromString(CStr(), iValue); }
	Bool ToUInt8(UInt8& uValue) const { return FromString(CStr(), uValue); }
	Bool ToUInt16(UInt16& uValue) const { return FromString(CStr(), uValue); }
	Bool ToUInt32(UInt32& uValue) const { return FromString(CStr(), uValue); }
	Bool ToUInt64(UInt64& uValue) const { return FromString(CStr(), uValue); }

	// Equality with cstrings.
	Bool operator==(Byte const* sCString) const;

	/**
	 * Inequality with cstrings.
	 */
	Bool operator!=(Byte const* sCString) const
	{
		return !(*this == sCString);
	}

	/**
	 * Equality, two HStrings are equal if their internal
	 * handle data is equal.
	 */
	Bool operator==(const HString& hstring) const
	{
		return (m_hHandle == hstring.m_hHandle);
	}

	/**
	 * Inequality, two HStrings are not equal if their
	 * internal handle data is not equal.
	 */
	Bool operator!=(const HString& hstring) const
	{
		return (m_hHandle != hstring.m_hHandle);
	}

	/**
	 * Returns true if this HString is an empty string,
	 * false otherwise. This will also return true for nullptr string data.
	 */
	Bool IsEmpty() const
	{
		return (0u == m_hHandle);
	}

	/**
	 * The HString operator< does not perform a string
	 * lexographical compare. Sorting HStrings using this function will not
	 * result in an alphanumerically ascending order.
	 */
	Bool operator<(const HString& hstring) const
	{
		return (m_hHandle < hstring.m_hHandle);
	}

	/**
	 * The HString operator> does not perform a string
	 * lexographical compare. Sorting HStrings using this function will not
	 * result in an alphanumerically descending order.
	 */
	Bool operator>(const HString& hstring) const
	{
		return (m_hHandle > hstring.m_hHandle);
	}

	/**
	 * @return The raw handle value that links this HString to its
	 * string entry in the global handle table.
	 */
	HStringData::InternalIndexType GetHandleValue() const
	{
		return m_hHandle;
	}

	/**
	 * Set the raw handle value that links this HString to its
	 * string entry in the global handle table.
	 *
	 * \warning Use with care - must only be called with values
	 * returned from GetHandleValue() called on this or other HStrings.
	 */
	void SetHandleValue(HStringData::InternalIndexType value)
	{
		SEOUL_ASSERT(nullptr != GetGlobalData().m_apGlobalArrayTable[value]);
		m_hHandle = value;
	}

private:
	/**
	 * @return The default global data used for default initialization.
	 */
	static HStringGlobalData& GetDefaultGlobalData()
	{
		static HStringGlobalData s_Data;
		return s_Data;
	}

#if defined(SEOUL_SERVER_BUILD) // Server support scoped replacement of the storage pointer.
	/**
	 * @return The global data shared by all HStrings.
	 */
	static HStringGlobalData& GetGlobalData()
	{
		// Can happen due to static initialization order if we're
		// called before the global pointer is initially constructed.
		if (nullptr == s_pGlobalData) { return GetDefaultGlobalData(); }
		return *s_pGlobalData;
	}

	thread_local static HStringGlobalData* s_pGlobalData;
	friend class HStringGlobalStorageScope;

#else // #if !defined(SEOUL_SERVER_BUILD)
	/**
	 * @return The global data shared by all HStrings.
	 */
	static HStringGlobalData& GetGlobalData()
	{
		return GetDefaultGlobalData();
	}
#endif // /#if !defined(SEOUL_SERVER_BUILD)

	/** Hook for the debugger. WARNING: DO NOT USE - will likely lead to static initialization order crashes. */
	static const HStringGlobalData& s_kGlobalData;

	HStringData::InternalIndexType m_hHandle;

	// Find the handle value associated with a string - returns true if a new entry
	// was created, false if an existing entry was used.
	Bool InternalConstruct(Byte const* pString, UInt32 zStringLengthInBytes, Bool bCaseInsensitive, Bool bStaticReadOnlyMemory);

	// Populate rHString with a valid HString if one already exists in the HString table, and return true,
	// or return false if no such HString has been added to the HString table.
	static Bool InternalGet(HString& rOut, Byte const* pString, UInt32 zStringLengthInBytes, Bool bCaseInsensitive, Bool bStaticReadOnlyMemory);
}; // class HString
template <> struct CanMemCpy<HString> { static const Bool Value = true; };
template <> struct CanZeroInit<HString> { static const Bool Value = true; };

/**
 * @return True if sString is equal to hstring, using a case sensitive string compare, false otherwise.
 */
inline Bool operator==(Byte const* sString, HString hstring)
{
	return (hstring == sString);
}

/**
 * @return True if sString is not equal to hstring, using a case sensitive string compare, false otherwise.
 */
inline Bool operator!=(Byte const* sString, HString hstring)
{
	return (hstring != sString);
}

template <>
struct DefaultHashTableKeyTraits<HString>
{
	inline static Float GetLoadFactor()
	{
		return 0.75f;
	}

	inline static HString GetNullKey()
	{
		return HString();
	}

	static const Bool kCheckHashBeforeEquals = false;
};

/**
 * Server builds only support special functionality that
 * allows for the substitution of alternative global HString
 * tables for a scope.
 *
 * WARNING: This is very dangerous functionality in general
 * that requires weakly enforced constraints on the environment
 *
 * In particular, the environment must guarantee that an HString
 * that is generated while in a scope never "escapes" that scope
 * (it is never used when a different scope is active).
 *
 * The necessary constraints are enforced in a straightforward manner
 * in server code (since there is a wrapper layer between SeoulEngine
 * code and go code) but this is not generally true.
 */
#if defined(SEOUL_SERVER_BUILD) // Server support scoped replacement of the storage pointer.
class HStringGlobalStorageScope SEOUL_SEALED
{
public:
	HStringGlobalStorageScope(HStringGlobalData* pNew)
		: m_pNew(pNew)
		, m_pOld((HStringGlobalData*)HString::s_pGlobalData)
	{
		HString::s_pGlobalData = m_pNew;
	}

	~HStringGlobalStorageScope()
	{
		HString::s_pGlobalData = m_pOld;
	}

private:
	SEOUL_DISABLE_COPY(HStringGlobalStorageScope);

	HStringGlobalData* const m_pNew;
	HStringGlobalData* const m_pOld;
}; // class HStringGlobalStorageScope
#endif // /#if defined(SEOUL_SERVER_BUILD)

} // namespace Seoul

#endif // include guard
