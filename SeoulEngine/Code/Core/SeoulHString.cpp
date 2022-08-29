/**
 * \file SeoulHString.cpp
 * \brief HString is an immutable string class. Strings are stored
 * in a global hash table. The HString object itself is a 16-bit index
 * into the table. HString should exhibit similar behavior to a symbol table.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AtomicPointer.h"
#include "DataStore.h"
#include "HashFunctions.h"
#include "Logger.h"
#include "SeoulHString.h"
#include "SeoulMath.h"
#include "StringUtil.h"

#include <string.h>

// HString stats are recorded in non-Ship builds.
#if !SEOUL_SHIP || defined(SEOUL_PROFILING_BUILD) || defined(SEOUL_SERVER_BUILD)
#define SEOUL_HSTRING_STATS 1
#else
#define SEOUL_HSTRING_STATS 0
#endif

namespace Seoul
{

#if defined(SEOUL_SERVER_BUILD) // Server support scoped replacement of the storage pointer.
thread_local HStringGlobalData* HString::s_pGlobalData = &HString::GetDefaultGlobalData();
#endif

/** Hook for debugging purposes. */
const HStringGlobalData& HString::s_kGlobalData = HString::GetGlobalData();

/**
 * @return A new HStringData instance and copy of pData, if it
 * is heap allocated.
 */
static inline HStringData* AllocateHStringData(
	UInt32 uHashValue,
	Byte const* pData,
	UInt32 zSizeInBytes,
	Bool bStaticReadOnlyMemory)
{
	// Allocate a new data buffer, with room for the string data,
	// if it is heap allocated.
	void* pRawData = MemoryManager::AllocateAligned(
		sizeof(HStringData) + (bStaticReadOnlyMemory ? 0u : (zSizeInBytes + 1u)),
		MemoryBudgets::HString,
		SEOUL_ALIGN_OF(HStringData));
	HStringData* pHStringData = new (pRawData) HStringData;

	// If the string is heap allocated, copy it to immediately
	// after the header.
	if (!bStaticReadOnlyMemory)
	{
		memcpy((Byte*)pRawData + sizeof(HStringData), pData, zSizeInBytes);
		*((Byte*)pRawData + sizeof(HStringData) + zSizeInBytes) = '\0';
	}

	// Initialize the members of the HStringData.
	pHStringData->m_bStaticReadOnlyMemory = bStaticReadOnlyMemory;
	pHStringData->m_pData = (Byte*)(bStaticReadOnlyMemory ? pData : ((Byte const*)pRawData + sizeof(HStringData)));
	pHStringData->m_uHashValue = uHashValue;
	pHStringData->m_zSizeInBytes = zSizeInBytes;

	return pHStringData;
}

/**
 * Release previous data allocated with AllocateHStringData.
 */
static inline void DeallocateHStringData(HStringData*& rpData)
{
	// Swap the data.
	HStringData* pData = rpData;
	rpData = nullptr;

	// If defined, destroy and deallocate.
	if (pData)
	{
		pData->~HStringData();
		MemoryManager::Deallocate(pData);
	}
}

HStringGlobalData::HStringGlobalData()
	: m_apGlobalArrayTable((HStringData*)nullptr)
{
	// Insert an empty string into slot 0.
	m_apGlobalArrayTable[0] = AllocateHStringData(
		0u,
		"",
		0u,
		true);
}

HStringGlobalData::~HStringGlobalData()
{
	for (Int32 i = (Int32)m_apGlobalArrayTable.GetSize() - 1; i >= 0; --i)
	{
		DeallocateHStringData(m_apGlobalArrayTable[i]);
	}
}

#if defined(SEOUL_SERVER_BUILD) // Server support scoped replacement of the storage pointer.
void HStringGlobalData::Clear()
{
	m_HStringStats = HStringStats();

	// Don't deallocate element 0 - we leave the null entry for future use.
	for (Int32 i = (Int32)m_apGlobalArrayTable.GetSize() - 1; i >= 1; --i)
	{
		DeallocateHStringData(m_apGlobalArrayTable[i]);
	}
}
#endif // /#if defined(SEOUL_SERVER_BUILD)

struct HStringDataSort SEOUL_SEALED
{
	Bool operator()(HStringData* a, HStringData* b) const
	{
		return (strcmp(a->m_pData, b->m_pData) < 0);
	}
}; // struct HStringDataSort

/** Get the HString table as a DataStore (root will be an array). For debugging. */
void HString::GetAllHStrings(DataStore& rDataStore)
{
	Vector<HStringData*, MemoryBudgets::Developer> vHStringData;
	auto const& apGlobalArrayTable = GetGlobalData().m_apGlobalArrayTable;
	for (auto const pData : apGlobalArrayTable)
	{
		if (pData != nullptr)
		{
			vHStringData.PushBack(pData);
		}
	}

	HStringDataSort sorter;
	QuickSort(vHStringData.Begin(), vHStringData.End(), sorter);
	auto const uSize = vHStringData.GetSize();

	DataStore dataStore;
	dataStore.MakeArray(uSize);
	auto const root = dataStore.GetRootNode();
	for (auto i = 0u; i < uSize; ++i)
	{
		dataStore.SetStringToArray(root, i, vHStringData[i]->m_pData);
	}

	rDataStore.Swap(dataStore);
}

/**
 * Sorts & logs all HStrings in the HStringGlobalData for analysis
 */
void HString::LogAllHStrings()
{
#if SEOUL_LOGGING_ENABLED
	Vector<HStringData*, MemoryBudgets::Developer> vHStringData;
	auto const& apGlobalArrayTable = GetGlobalData().m_apGlobalArrayTable;
	for (auto const pData : apGlobalArrayTable)
	{
		if (pData != nullptr)
		{
			vHStringData.PushBack(pData);
		}
	}

	HStringDataSort sorter;
	QuickSort(vHStringData.Begin(), vHStringData.End(), sorter);

	SEOUL_LOG_ENGINE("HStringData report:");
	for (auto const pData: vHStringData)
	{
		SEOUL_LOG_ENGINE("%s", pData->m_pData);
	}
#endif // /#if SEOUL_LOGGING_ENABLED
}

/**
 * Variation of Get() for Seoul::String.
 */
Bool HString::Get(HString& rOut, const String& s, Bool bCaseInsensitive /*= false*/)
{
	return InternalGet(rOut, s.CStr(), s.GetSize(), bCaseInsensitive, false);
}

/**
 * Convenience function that allows SetCanonicalString to be called
 * with a String argument.
 */
Bool HString::SetCanonicalString(const String& sString)
{
	return SetCanonicalString(sString.CStr(), sString.GetSize());
}

/**
 * Constructs an HString from sString.
 *
 * \warning This method is made thread-safe by locking a Mutex, which
 * means it can be a threading bottleneck. But since this constructor
 * is expensive in general, it just means that applications should
 * cache actual HStrings and not construct them from raw strings
 * during regular program flow.
 */
HString::HString(const String& sString, Bool bCaseInsensitive /*= false*/)
	: m_hHandle(0u)
{
	InternalConstruct(sString.CStr(), sString.GetSize(), bCaseInsensitive, false);
}

inline static Int InternalStaticCaseSensitiveCompare(Byte const* sA, Byte const* sB, UInt32 zStringLengthInBytes)
{
	return strncmp(sA, sB, zStringLengthInBytes);
}

inline static Int InternalStaticCaseInsensitiveCompare(Byte const* sA, Byte const* sB, UInt32 zStringLengthInBytes)
{
	return STRNCMP_CASE_INSENSITIVE(sA, sB, zStringLengthInBytes);
}

typedef Int (*HStringCompareFunction)(Byte const* sA, Byte const* sB, UInt32 zStringLengthInBytes);

inline Bool IsEqual(
	HStringCompareFunction pCompareFunction,
	const HStringData& data,
	UInt32 uHashValue,
	Byte const* pString,
	UInt32 zStringLengthInBytes)
{
	if (uHashValue != data.m_uHashValue)
	{
		return false;
	}

	if (zStringLengthInBytes != data.m_zSizeInBytes)
	{
		return false;
	}

	// Optimized check - if the data entry is not heap allocated, it is a string in the static
	// data area of the executable. In this case, we can do a pointer comparison and if equal,
	// assume the strings are equal.
	if (data.m_bStaticReadOnlyMemory && (data.m_pData == pString))
	{
		return true;
	}

	// Otherwise, we need to do a full string comparison.
	return (0 == pCompareFunction(data.m_pData, pString, zStringLengthInBytes));
}

/**
 * Really, the only computationally expensive function of the HString
 * structure. This constructor helper method will hash the pString parameter,
 * look for the string in a global string table and if not found, allocate
 * memory and insert a copy of the string in the table.
 *
 * This method is also expensive because it locks a Mutex to ensure
 * thread-safety of the HString table.
 */
Bool HString::InternalConstruct(Byte const* pString, UInt32 zStringLengthInBytes, Bool bCaseInsensitive, Bool bStaticReadOnlyMemory)
{
	static const Int kCapacity = HStringDataProperties<HStringData::InternalIndexType>::GlobalArraySize;

	HStringGlobalData& globalData = GetGlobalData();
	HStringGlobalData::GlobalArrayTable& table = globalData.m_apGlobalArrayTable;

	// Sanity check that the capacity is power of 2.
	SEOUL_STATIC_ASSERT((0 == (kCapacity & (kCapacity - 1))));

	HStringCompareFunction const pCompareFunction = (bCaseInsensitive
		? InternalStaticCaseInsensitiveCompare
		: InternalStaticCaseSensitiveCompare);

	if (pString && pString[0])
	{
		SEOUL_ASSERT(IsValidUTF8String(pString, zStringLengthInBytes));

		// A case insensitive hash is used so that case insensitive HStrings can be constructed without
		// modifying the "canonical" form of the underlying string (using tolower on the string before
		// constructing the HString). Put another way, case sensitive HStrings and case insensitive HStrings
		// exist in the same namespace and can be used (with care) interchangeably.
		UInt32 const uHash = Seoul::GetCaseInsensitiveHash(pString, zStringLengthInBytes);

		Int iIndex = uHash;

		Atomic32Type collisionCount = 0;
		while (true)
		{
			iIndex &= (kCapacity - 1);
			SEOUL_ASSERT(iIndex >= 0 && iIndex < (Int)table.GetSize());

			HStringData* pData = table[iIndex];

			// If we've hit a nullptr element, attempt to insert a new entry.
			if (nullptr == pData)
			{
				// Construct the HStringData object.
				pData = AllocateHStringData(
					uHash,
					pString,
					zStringLengthInBytes,
					bStaticReadOnlyMemory);

				// If the set succeeds, we're done.
				if (nullptr == AtomicPointerCommon::AtomicSetIfEqual(
					(void**)table.Get(iIndex),
					pData,
					nullptr))
				{
#if SEOUL_HSTRING_STATS
					++globalData.m_HStringStats.m_TotalHStrings;
					if (pData->m_bStaticReadOnlyMemory)
					{
						++globalData.m_HStringStats.m_TotalStaticAllocatedHStrings;
						globalData.m_HStringStats.m_TotalStaticAllocatedHStringMemory += (zStringLengthInBytes + 1);
					}

					globalData.m_HStringStats.UpdateCollisionCount(collisionCount);
#endif

					m_hHandle = (HStringData::InternalIndexType)iIndex;
					return true;
				}
				// Otherwise, cleanup the data and then decrement the index to re-evaluate the
				// same slot.
				else
				{
					DeallocateHStringData(pData);
					--iIndex;
				}
			}
			// Otherwise, do a equality comparison - if this suceeds, we're done, use the
			// existing entry.
			else if (IsEqual(pCompareFunction, *pData, uHash, pString, zStringLengthInBytes))
			{
				m_hHandle = (HStringData::InternalIndexType)iIndex;

				// Return false to indicate we found an existing entry.
				return false;
			}

			++collisionCount;
			++iIndex;

			// Full table check.
			if (collisionCount >= kCapacity)
			{
				// This is an error, flag it.
				SEOUL_FAIL("Ran out of HString space.");
				return false;
			}
		}
	}

	// Return false to indicate we found an existing entry.
	return false;
}

/**
 * Populate rHString with a valid HString if one already exists in the HString table, and return true,
 * or return false if no such HString has been added to the HString table.
 */
Bool HString::InternalGet(
	HString& rOut,
	Byte const* pString,
	UInt32 zStringLengthInBytes,
	Bool bCaseInsensitive,
	Bool bStaticReadOnlyMemory)
{
	static const Int kCapacity = HStringDataProperties<HStringData::InternalIndexType>::GlobalArraySize;

	HStringGlobalData& globalData = GetGlobalData();
	HStringGlobalData::GlobalArrayTable& table = globalData.m_apGlobalArrayTable;

	// Sanity check that the capacity is power of 2.
	SEOUL_STATIC_ASSERT((0 == (kCapacity & (kCapacity - 1))));

	HStringCompareFunction const pCompareFunction = (bCaseInsensitive
		? InternalStaticCaseInsensitiveCompare
		: InternalStaticCaseSensitiveCompare);

	if (!pString || !pString[0])
	{
		rOut = HString();
		return true;
	}

	SEOUL_ASSERT(IsValidUTF8String(pString, zStringLengthInBytes));

	// A case insensitive hash is used so that case insensitive HStrings can be constructed without
	// modifying the "canonical" form of the underlying string (using tolower on the string before
	// constructing the HString). Put another way, case sensitive HStrings and case insensitive HStrings
	// exist in the same namespace and can be used (with care) interchangeably.
	UInt32 const uHash = Seoul::GetCaseInsensitiveHash(pString, zStringLengthInBytes);

	Int iIndex = uHash;

	Atomic32Type collisionCount = 0;
	while (true)
	{
		iIndex &= (kCapacity - 1);
		SEOUL_ASSERT(iIndex >= 0 && iIndex < (Int)table.GetSize());

		HStringData* pData = table[iIndex];

		// If we've hit a nullptr element, we're done, the entry is not in the table.
		if (nullptr == pData)
		{
			return false;
		}
		// Otherwise, do a equality comparison - if this suceeds, we're done, we
		// found an entry.
		else if (IsEqual(pCompareFunction, *pData, uHash, pString, zStringLengthInBytes))
		{
			rOut.m_hHandle = (HStringData::InternalIndexType)iIndex;

			// Return success.
			return true;
		}

		++collisionCount;
		++iIndex;

		// Full table check.
		if (collisionCount >= kCapacity)
		{
			// Not found.
			return false;
		}
	}
}

/**
 * Equality with cstrings.
 */
Bool HString::operator==(Byte const* sCString) const
{
	return (0 == strcmp(CStr(), sCString));
}

} // namespace Seoul
