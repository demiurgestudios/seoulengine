/**
 * \file DataStore.cpp
 * \brief A key-value data structure that has the following properties:
 * - dynamic typing
 * - fast lookup by key, using HStrings
 * - support for nested tables and arrays
 * - values are stored in native types, to allow fast retrieval and conversion
 *   to target types.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

// Optimizations are enabled in Debug in DataStore.cpp since this
// is an extreme hot point and contributes greatly to prohibitive
// performance at runtime and prohibitive startup times.
//
// If you need to debug this file (breakpoints), enable this directive.
#define SEOUL_DISABLE_OPT 0
#if SEOUL_DISABLE_OPT && _DEBUG
#	if defined(_MSC_VER)
#		pragma optimize("", off)
#	endif
#endif

#include "DataStore.h"
#include "DataStoreParser.h"
#include "Lexer.h"
#include "Path.h"
#include "ScopedPtr.h"
#include "SeoulFile.h"
#include "SeoulFileReaders.h"
#include "SeoulFileWriters.h"
#include "SeoulMD5.h"
#include "StringUtil.h"

namespace Seoul
{

// Sanity test for our canonical NaN
#if !SEOUL_ASSERTIONS_DISABLED
namespace
{

struct Checker
{
	Checker()
	{
		union
		{
			UInt32 u;
			Float32 f;
		};
		u = kuDataNodeCanonicalNaNBits;
		SEOUL_ASSERT(IsNaN(f));
	}
};
static Checker s_Checker;

} // namespace
#endif // /#if !SEOUL_ASSERTIONS_DISABLED

using namespace DataStoreCommon;

/** Utility, swaps a vector with a new version that is as compact as possible. */
template <typename T>
static inline void CompactVector(T& rv)
{
	rv.ShrinkToFit();
}

/**
 * Helper utility, sorts 2 HStrings in descending lexicographical order.
 */
struct LexicalHStringSort
{
	Bool operator()(HString a, HString b) const
	{
		return (strcmp(a.CStr(), b.CStr()) < 0);
	}
};

/** Apply some reductions to compute a float's consistent MD5. */
static inline void ComputeFloatMD5(MD5& md5, Float f)
{
	// +0.0f and -0.0f normalized to +0.0f.
	f = (0.0f == f ? 0.0f : f);

	md5.AppendPOD(f);
}

DataStore::DataStore()
	: m_vHandleDataOffsets()
	, m_vData()
	, m_uDataSizeAfterLastCollection(kuDataStoreAutoGarbageCollectionMarkerSize)
	, m_uNextHandle(0)
	, m_uAllocatedHandles(0)
	, m_uSuppressGarbageCollection(0)
{
}

DataStore::DataStore(UInt32 zInitialCapacityInBytes)
	: m_vHandleDataOffsets()
	, m_vData()
	, m_uDataSizeAfterLastCollection(kuDataStoreAutoGarbageCollectionMarkerSize)
	, m_uNextHandle(0)
	, m_uAllocatedHandles(0)
	, m_uSuppressGarbageCollection(0)
{
	UInt32 const nCapacityInDataNodes = (UInt32)RoundUpToAlignment(zInitialCapacityInBytes, sizeof(DataNode)) / sizeof(DataNode);
	m_vData.Reserve(nCapacityInDataNodes);
}

DataStore::~DataStore()
{
}

/**
 * Reserve at least zCapacityInBytes in this DataStore's
 * internal heap.
 */
void DataStore::Reserve(UInt32 zCapacityInBytes)
{
	UInt32 const nCapacityInDataNodes = (UInt32)RoundUpToAlignment(zCapacityInBytes, sizeof(DataNode)) / sizeof(DataNode);
	m_vData.Reserve(nCapacityInDataNodes);
}

/**
 * @return A calculated checksum of this DataStore's contents.
 *
 * The checksum will be the same for two DataStores for which
 * DataStore::Equal() is true.
 */
String DataStore::ComputeMD5() const
{
	FixedArray<UInt8, MD5::kResultSize> aReturn;
	{
		MD5 md5(aReturn);
		InternalComputeMD5(md5, GetRootNode());
	}
	return HexDump(aReturn.Data(), aReturn.GetSize());
}

/**
 * Compact the heap memory of this DataStore to its minimum size
 *
 * It is best to call this method when you expect few (if any) mutations
 * to occur to this DataStore that will require heap allocation.
 */
void DataStore::CompactHeap()
{
	// Compact the heap of DataNodes if it is not empty.
	CompactVector(m_vData);

	// Remove unused handles and offsets.
	while (!m_vHandleDataOffsets.IsEmpty() && !m_vHandleDataOffsets.Back().IsValid())
	{
		m_vHandleDataOffsets.PopBack();
	}

	// Compact the heaps used by handle data and offsets.
	CompactVector(m_vHandleDataOffsets);
}

/**
 * @return True if valueA is equal to valueB, false otherwise.
 *
 * This is a "by value" equals, it will traverse arrays
 * and tables and check the equality of their members.
 */
Bool DataStore::Equals(
	const DataStore& dataStoreA,
	const DataNode& valueA,
	const DataStore& dataStoreB,
	const DataNode& valueB,
	Bool bNaNEquals /*= false*/)
{
	// Not possible for 2 types to be equal of different types.
	if (valueA.GetType() != valueB.GetType())
	{
		return false;
	}

	// Reference types need to be handled specially.
	switch (valueA.GetType())
	{
		// For arrays, compare each element.
	case DataNode::kArray:
		{
			UInt32 uArrayCountA = 0u;
			UInt32 uArrayCountB = 0u;
			if (!dataStoreA.GetArrayCount(valueA, uArrayCountA))
			{
				return false;
			}

			if (!dataStoreB.GetArrayCount(valueB, uArrayCountB))
			{
				return false;
			}

			if (uArrayCountA != uArrayCountB)
			{
				return false;
			}

			for (UInt32 i = 0u; i < uArrayCountA; ++i)
			{
				DataNode arrayValueA;
				if (!dataStoreA.GetValueFromArray(valueA, i, arrayValueA))
				{
					return false;
				}

				DataNode arrayValueB;
				if (!dataStoreB.GetValueFromArray(valueB, i, arrayValueB))
				{
					return false;
				}

				if (!Equals(dataStoreA, arrayValueA, dataStoreB, arrayValueB, bNaNEquals))
				{
					return false;
				}
			}

			return true;
		}

		// Just get the Boolean value and compare it.
	case DataNode::kBoolean:
		return (dataStoreA.AssumeBoolean(valueA) == dataStoreB.AssumeBoolean(valueB));

		// Use internal knowledge of FilePath representation.
	case DataNode::kFilePath:
		return (valueA.GetFilePath() == valueB.GetFilePath());

		// Just get the Float31 value and compare it.
	case DataNode::kFloat31:
		if (bNaNEquals)
		{
			auto const fA = dataStoreA.AssumeFloat31(valueA);
			auto const fB = dataStoreB.AssumeFloat31(valueB);
			return (fA == fB) || (IsNaN(fA) && IsNaN(fB));
		}
		else
		{
			return (dataStoreA.AssumeFloat31(valueA) == dataStoreB.AssumeFloat31(valueB));
		}

		// Just get the Float32 value and compare it.
	case DataNode::kFloat32:
		// Sanity checks - NaN should always be canonical and stored
		// as a Float31.
		SEOUL_ASSERT(!IsNaN(dataStoreA.AssumeFloat32(valueA)));
		SEOUL_ASSERT(!IsNaN(dataStoreB.AssumeFloat32(valueB)));
		return (dataStoreA.AssumeFloat32(valueA) == dataStoreB.AssumeFloat32(valueB));

		// Just get the Int32 value and compare it.
	case DataNode::kInt32Big:
		return (dataStoreA.AssumeInt32Big(valueA) == dataStoreB.AssumeInt32Big(valueB));

		// Just get the Int32 value and compare it.
	case DataNode::kInt32Small:
		return (dataStoreA.AssumeInt32Small(valueA) == dataStoreB.AssumeInt32Small(valueB));

		// Just get the Int64 value and compare it.
	case DataNode::kInt64:
		return (dataStoreA.AssumeInt64(valueA) == dataStoreB.AssumeInt64(valueB));

		// Null is just...null.
	case DataNode::kNull:
		return true;

		// SpecialErase is SpecialErase.
	case DataNode::kSpecialErase:
		return true;

		// Get the string values, then do a case sensitive comparison
		// of the strings.
	case DataNode::kString:
	 	{
			Byte const* sA = nullptr;
			UInt32 zLengthInBytesA = 0u;
			Byte const* sB = nullptr;
			UInt32 zLengthInBytesB = 0u;
			dataStoreA.AsString(valueA, sA, zLengthInBytesA);
	 		dataStoreB.AsString(valueB, sB, zLengthInBytesB);

	  		return (zLengthInBytesA == zLengthInBytesB && (0 == strncmp(sA, sB, zLengthInBytesA)));
		}

		// For tables, compare each element.
	case DataNode::kTable:
		{
			UInt32 uTableCountA = 0u;
			UInt32 uTableCountB = 0u;
			if (!dataStoreA.GetTableCount(valueA, uTableCountA))
			{
				return false;
			}

			if (!dataStoreB.GetTableCount(valueB, uTableCountB))
			{
				return false;
			}

			if (uTableCountA != uTableCountB)
			{
				return false;
			}

			auto const iBegin = dataStoreA.TableBegin(valueA);
			auto const iEnd = dataStoreA.TableEnd(valueA);
			for (auto i = iBegin; iEnd != i; ++i)
			{
				HString const keyA = i->First;
				DataNode tableValueB;
				if (!dataStoreB.GetValueFromTable(valueB, keyA, tableValueB))
				{
					return false;
				}

				if (!Equals(dataStoreA, i->Second, dataStoreB, tableValueB, bNaNEquals))
				{
					return false;
				}
			}

			return true;
		}

		// Just get the UInt32 value and compare it.
	case DataNode::kUInt32:
		return (dataStoreA.AssumeUInt32(valueA) == dataStoreB.AssumeUInt32(valueB));

		// Just get the Int64 value and compare it.
	case DataNode::kUInt64:
		return (dataStoreA.AssumeUInt64(valueA) == dataStoreB.AssumeUInt64(valueB));
	default:
		SEOUL_FAIL("Unexpected enum value.");
		return false;
	};
}

/**
 * Attempt to copy the tree defined by fromNode to the tree defined
 * by to. If this method returns false, the tree of to is left in an
 * incomplete/undefined state.
 *
 * @param[in] fromDataStore DataStore containing the source node.
 * @param[in] fromNode Source node to copy from
 * @param[in] to Source node to populate with a copy of from
 * @param[in] bAllowConflicts If false, fail when there is a key in both 'to' and 'from'.
 * @param[in] bOverwriteConflicts If a key is in both 'to' and 'from' and bAllowConflicts is true,
 *            this parameter determines if it should be overwritten.
 *
 * @return true on success, false on failure.
 *
 * \pre to must be from this DataStore.
 * \pre fromNode must be the same type as toNode
 * \pre fromNode must be an array or table type.
 */
Bool DataStore::DeepCopy(
	const DataStore& fromDataStore,
	const DataNode& fromNode,
	const DataNode& to,
	Bool bAllowConflicts /*= false*/,
	Bool bOverwriteConflicts /*= true*/)
{
	// Can't copy if the types of to and from differ.
	if (fromNode.GetType() != to.GetType())
	{
		return false;
	}

	// Handle copying an array.
	if (fromNode.IsArray())
	{
		UInt32 uArrayCount = 0u;
		if (!fromDataStore.GetArrayCount(fromNode, uArrayCount)) { return false; }

		// Iterate over all members of the source array.
		for (UInt32 i = 0u; i < uArrayCount; ++i)
		{
			// If conflicts are allowed but conflicts should not be overwritten, check for
			// that case and skip this entry if it applies.
			{
				DataNode unused;
				if (bAllowConflicts && !bOverwriteConflicts && GetValueFromArray(to, i, unused))
				{
					continue;
				}
			}

			// Cache the value in the source array.
			DataNode valueFrom;
			if (!fromDataStore.GetValueFromArray(fromNode, i, valueFrom))
			{
				return false;
			}

			// Perform the copy.
			if (!DeepCopyToArray(fromDataStore, valueFrom, to, i, bAllowConflicts, bOverwriteConflicts)) { return false; }
		}

		return true;
	}
	// Handle copying a table.
	else if (fromNode.IsTable())
	{
		// Iterate over all members of the input table.
		auto const iBegin = fromDataStore.TableBegin(fromNode);
		auto const iEnd = fromDataStore.TableEnd(fromNode);
		for (auto i = iBegin; iEnd != i; ++i)
		{
			// If conflicts are allowed but conflicts should not be overwritten, check for
			// that case and skip this entry if it applies.
			{
				DataNode unused;
				if (bAllowConflicts && !bOverwriteConflicts && GetValueFromTable(to, i->First, unused))
				{
					continue;
				}
			}

			if (!DeepCopyToTable(fromDataStore, i->Second, to, i->First, bAllowConflicts, bOverwriteConflicts)) { return false; }
		}

		return true;
	}
	// Deep copying any other type is an invalid operation.
	else
	{
		return false;
	}
}

Bool DataStore::DeepCopyToArray(
	const DataStore& fromDataStore,
	const DataNode& valueFrom,
	const DataNode& to,
	UInt32 i,
	Bool bAllowConflicts /*= false*/,
	Bool bOverwriteConflicts /*= true*/)
{
	// If overwriting is not allowed, check if a value is already
	// defined in the same slot in the destination array. If so, fail.
	DataNode valueTo;
	if (GetValueFromArray(to, i, valueTo) && !bAllowConflicts)
	{
		return false;
	}

	// Handle all possible source value types.
	switch (valueFrom.GetType())
	{
	case DataNode::kNull: if (!SetNullValueToArray(to, i)) { return false; } break;
	case DataNode::kSpecialErase: if (!SetSpecialEraseToArray(to, i)) { return false; } break;
	case DataNode::kBoolean: if (!SetBooleanValueToArray(to, i, fromDataStore.AssumeBoolean(valueFrom))) { return false; } break;
	case DataNode::kInt32Big: if (!SetInt32ValueToArray(to, i, fromDataStore.AssumeInt32Big(valueFrom))) { return false; } break;
	case DataNode::kInt32Small: if (!SetInt32ValueToArray(to, i, fromDataStore.AssumeInt32Small(valueFrom))) { return false; } break;
	case DataNode::kUInt32: if (!SetUInt32ValueToArray(to, i, fromDataStore.AssumeUInt32(valueFrom))) { return false; } break;
	case DataNode::kFloat31: if (!SetFloat32ValueToArray(to, i, fromDataStore.AssumeFloat31(valueFrom))) { return false; } break;
	case DataNode::kFloat32: if (!SetFloat32ValueToArray(to, i, fromDataStore.AssumeFloat32(valueFrom))) { return false; } break;
	case DataNode::kInt64: if (!SetInt64ValueToArray(to, i, fromDataStore.AssumeInt64(valueFrom))) { return false; } break;
	case DataNode::kUInt64: if (!SetUInt64ValueToArray(to, i, fromDataStore.AssumeUInt64(valueFrom))) { return false; } break;

	case DataNode::kFilePath:
	{
		FilePath filePath;
		SEOUL_VERIFY(fromDataStore.AsFilePath(valueFrom, filePath));
		if (!SetFilePathToArray(to, i, filePath)) { return false; }
	}
	break;
	case DataNode::kString:
	{
		String s;
		SEOUL_VERIFY(fromDataStore.AsString(valueFrom, s));
		if (!SetStringToArray(to, i, s)) { return false; }
	}
	break;
	case DataNode::kArray:
	{
		// If bAllowOverwrite is true, don't set an array to the
		// output slot if the output slot already contains an array.
		if (!bAllowConflicts || !valueTo.IsArray())
		{
			// This line potentially invalids valueTo, since it
			// replaces it.
			if (!SetArrayToArray(to, i)) { return false; }

			// Refresh value to and then DeepCopy the sub array.
			if (!GetValueFromArray(to, i, valueTo)) { return false; }
		}

		// Copy through the value.
		if (!DeepCopy(fromDataStore, valueFrom, valueTo, bAllowConflicts, bOverwriteConflicts)) { return false; }
	}
	break;
	case DataNode::kTable:
	{
		// If bAllowOverwrite is true, don't set a table to the
		// output slot if the output slot already contains a table.
		if (!bAllowConflicts || !valueTo.IsTable())
		{
			// This line potentially invalids valueTo, since it
			// replaces it.
			if (!SetTableToArray(to, i)) { return false; }

			// Refresh value to and then DeepCopy the sub tree.
			if (!GetValueFromArray(to, i, valueTo)) { return false; }
		}

		// Copy through the value.
		if (!DeepCopy(fromDataStore, valueFrom, valueTo, bAllowConflicts, bOverwriteConflicts)) { return false; }
	}
	break;
	default:
		return false;
	};

	return true;
}

Bool DataStore::DeepCopyToTable(
	const DataStore& fromDataStore,
	const DataNode& valueFrom,
	const DataNode& to,
	HString key,
	Bool bAllowConflicts /*= false*/,
	Bool bOverwriteConflicts /*= true*/)
{
	// If overwriting is not allowed, check if a value is already
	// defined in the same slot in the destination array. If so, fail.
	DataNode valueTo;
	if (GetValueFromTable(to, key, valueTo) && !bAllowConflicts)
	{
		return false;
	}

	// Handle all possible source value types.
	switch (valueFrom.GetType())
	{
	case DataNode::kNull: if (!SetNullValueToTable(to, key)) { return false; } break;
	case DataNode::kSpecialErase: if (!SetSpecialEraseToTable(to, key)) { return false; } break;
	case DataNode::kBoolean: if (!SetBooleanValueToTable(to, key, fromDataStore.AssumeBoolean(valueFrom))) { return false; } break;
	case DataNode::kInt32Big: if (!SetInt32ValueToTable(to, key, fromDataStore.AssumeInt32Big(valueFrom))) { return false; } break;
	case DataNode::kInt32Small: if (!SetInt32ValueToTable(to, key, fromDataStore.AssumeInt32Small(valueFrom))) { return false; } break;
	case DataNode::kUInt32: if (!SetUInt32ValueToTable(to, key, fromDataStore.AssumeUInt32(valueFrom))) { return false; } break;
	case DataNode::kFloat31: if (!SetFloat32ValueToTable(to, key, fromDataStore.AssumeFloat31(valueFrom))) { return false; } break;
	case DataNode::kFloat32: if (!SetFloat32ValueToTable(to, key, fromDataStore.AssumeFloat32(valueFrom))) { return false; } break;
	case DataNode::kInt64: if (!SetInt64ValueToTable(to, key, fromDataStore.AssumeInt64(valueFrom))) { return false; } break;
	case DataNode::kUInt64: if (!SetUInt64ValueToTable(to, key, fromDataStore.AssumeUInt64(valueFrom))) { return false; } break;

	case DataNode::kFilePath:
	{
		FilePath filePath;
		SEOUL_VERIFY(fromDataStore.AsFilePath(valueFrom, filePath));
		if (!SetFilePathToTable(to, key, filePath)) { return false; }
	}
	break;
	case DataNode::kString:
	{
		String s;
		SEOUL_VERIFY(fromDataStore.AsString(valueFrom, s));
		if (!SetStringToTable(to, key, s)) { return false; }
	}
	break;
	case DataNode::kArray:
	{
		// If bAllowOverwrite is true, don't set an array to the
		// output slot if the output slot already contains an array.
		if (!bAllowConflicts || !valueTo.IsArray())
		{
			// This line potentially invalids valueTo, since it
			// replaces it.
			if (!SetArrayToTable(to, key)) { return false; }

			// Refresh value to and then DeepCopy the sub array.
			if (!GetValueFromTable(to, key, valueTo)) { return false; }
		}

		// Copy through the value.
		if (!DeepCopy(fromDataStore, valueFrom, valueTo, bAllowConflicts, bOverwriteConflicts)) { return false; }
	}
	break;
	case DataNode::kTable:
	{
		// If bAllowOverwrite is true, don't set a table to the
		// output slot if the output slot already contains a table.
		if (!bAllowConflicts || !valueTo.IsTable())
		{
			// This line potentially invalids valueTo, since it
			// replaces it.
			if (!SetTableToTable(to, key)) { return false; }

			// Refresh value to and then DeepCopy the sub tree.
			if (!GetValueFromTable(to, key, valueTo)) { return false; }
		}

		// Copy through the value.
		if (!DeepCopy(fromDataStore, valueFrom, valueTo, bAllowConflicts, bOverwriteConflicts)) { return false; }
	}
	break;
	default:
		return false;
	};

	return true;
}

/**
 * If successful, updates the size of array to nNewSize.
 *
 * If nNewSize is greater than the current size of the array,
 * the new empty slots of the array will be populated with kNull DataNodes.
 *
 * @return True if the operation was successful, false otherwise. If this
 * method returns false, the size of the array is left unchanged. This
 * method can return false:
 * - if array is not of type kArray.
 * - if nNewSize is >= kBigArray.
 */
Bool DataStore::ResizeArray(const DataNode& array, UInt32 nNewSize)
{
	using namespace DataStoreCommon;

	if (nNewSize >= kBigArray)
	{
		return false;
	}

	// Early out if not an array.
	if (!array.IsArray())
	{
		return false;
	}

	// Check for a valid DataStore handle.
	DataNode::Handle const handle = array.GetHandle();
	if (!InternalIsValidHandle(handle))
	{
		return false;
	}

	Container* pContainer = (Container*)InternalGetDataEntryPtr(handle);
	if (pContainer->GetCountExcludingNull() < nNewSize)
	{
		if (nNewSize > pContainer->GetCapacityExcludingNull())
		{
			// Compute the old and new capacities.
			UInt32 const uOldTotalCapacity = pContainer->GetCapacityExcludingNull() + kuContainerSizeInDataEntries;
			UInt32 const uNewTotalCapacity = GetNextPowerOf2(nNewSize + kuContainerSizeInDataEntries);

			// Add kuContainerSizeInDataEntries to the capacity to include space for the Container header.
			// Perform reallocation and update.
			InternalReallocate(uOldTotalCapacity, uNewTotalCapacity, handle);
			pContainer = (Container*)InternalGetDataEntryPtr(handle);
			pContainer->SetCapacityExcludingNull((uNewTotalCapacity - kuContainerSizeInDataEntries));
		}
	}

	pContainer->SetCountExcludingNull(nNewSize);

	return true;
}

/**
 * @return A TableIterator at the head of table, or an iterator == TableEnd() if
 * the table is empty, or if table is not a table.
 */
DataStore::TableIterator DataStore::TableBegin(const DataNode& table) const
{
	using namespace DataStoreCommon;

	// Early out if not a table.
	if (!table.IsTable())
	{
		return TableIterator(nullptr, DataNode(), 0u, 0u);
	}

	// Check for a valid DataStore handle.
	DataNode::Handle const handle = table.GetHandle();
	if (!InternalIsValidHandle(handle))
	{
		return TableIterator(nullptr, DataNode(), 0u, 0u);
	}

	auto const uDataSize = m_vData.GetSize();
	auto const uDataOffset = m_vHandleDataOffsets[handle.GetIndex()].GetDataOffset();
	auto const uContainerOffset = uDataOffset + (sizeof(Container) / sizeof(DataNode));
	if (uContainerOffset > uDataSize)
	{
		return TableIterator(nullptr, DataNode(), 0u, 0u);
	}

	Container* pContainer = (Container*)m_vData.Get(uDataOffset);
	if (pContainer->GetCapacityExcludingNull() + pContainer->GetHasNullStorage() + uContainerOffset > uDataSize)
	{
		return TableIterator(nullptr, DataNode(), 0u, 0u);
	}

	return TableIterator(this, table, pContainer->GetCapacityExcludingNull(), 0u);
}

/**
 * @return A TableIterator at the end of table.
 */
DataStore::TableIterator DataStore::TableEnd(const DataNode& table) const
{
	using namespace DataStoreCommon;

	// Early out if not a table.
	if (!table.IsTable())
	{
		return TableIterator(nullptr, DataNode(), 0u, 0u);
	}

	// Check for a valid DataStore handle.
	DataNode::Handle const handle = table.GetHandle();
	if (!InternalIsValidHandle(handle))
	{
		return TableIterator(nullptr, DataNode(), 0u, 0u);
	}

	auto const uDataSize = m_vData.GetSize();
	auto const uDataOffset = m_vHandleDataOffsets[handle.GetIndex()].GetDataOffset();
	auto const uContainerOffset = uDataOffset + (sizeof(Container) / sizeof(DataNode));
	if (uContainerOffset > uDataSize)
	{
		return TableIterator(nullptr, DataNode(), 0u, 0u);
	}

	Container* pContainer = (Container*)m_vData.Get(uDataOffset);
	if (pContainer->GetCapacityExcludingNull() + pContainer->GetHasNullStorage() + uContainerOffset > uDataSize)
	{
		return TableIterator(nullptr, DataNode(), 0u, 0u);
	}

	// Deliberate - to handle the special null case, the End() iterator is 1 passed the
	// capacity if null is present. The iterator will "do the right thing" in this case:
	// - it will stop at the null entry, since GetCapacityExcludingNull() will stop iteration,
	//   whether the last entry is a null key or not.
	return TableIterator(this, table, pContainer->GetCapacityExcludingNull(), (pContainer->GetCapacityExcludingNull() + pContainer->GetHasNull()));
}

/** Remap a raw hstring handle value from serialized data. */
void DataStore::RemapForLoad(
	HStringData::InternalIndexType& ruRawHStringValue,
	RemapForLoadTable& tRemap)
{
	// Lookup the value in the table - a missing value means invalid data.
	HString remap;

	// TODO: Return false here instead of asserting?
	SEOUL_VERIFY(tRemap.GetValue(ruRawHStringValue, remap));

	// Remap the value to the runtime HString handle value.
	ruRawHStringValue = remap.GetHandleValue();
}

/**
 * Remap an arbitrary DataNode in serialized data to its runtime value -
 * most values are unchanged. HString values must be remaped using
 * the string lookup table.
 */
void DataStore::RemapForLoad(
	DataNode& rNode,
	const HandleDataOffsets& vHandleOffsets,
	Data& rvData,
	RemapForLoadTable& tFilePathRemap,
	RemapForLoadTable& tHStringRemap)
{
	switch (rNode.GetType())
	{
		// Enumerate the array and remap values.
	case DataNode::kArray:
		{
			// Get the offset into the data.
			UInt32 const uIndex = rNode.GetHandle().GetIndex();

			// Protect against out of range indices.
			if (uIndex >= vHandleOffsets.GetSize())
			{
				return;
			}

			HandleDataOffset const entry = vHandleOffsets[uIndex];
			auto const uContainerOffset = entry.GetDataOffset() + (sizeof(Container) / sizeof(DataNode));

			// Protect against invalid or corrupted data entries.
			if (uContainerOffset > rvData.GetSize())
			{
				return;
			}

			// Data header is a container.
			Container const container = rvData[entry.GetDataOffset()].AsContainer();

			// Protect against invalid container header.
			if (container.GetCountExcludingNull() > container.GetCapacityExcludingNull() ||
				container.GetCapacityExcludingNull() + uContainerOffset > rvData.GetSize())
			{
				return;
			}

			// If there is at least one value, get the value portion pointer and remap it.
			if (container.GetCountExcludingNull() > 0u)
			{
				// Value portion is immediately after the container header.
				DataNode* pValues = ((DataNode*)rvData.Get(entry.GetDataOffset() + kuContainerSizeInDataEntries));

				// Enumerate and remap.
				for (UInt32 i = 0u; i < container.GetCountExcludingNull(); ++i)
				{
					RemapForLoad(
						pValues[i],
						vHandleOffsets,
						rvData,
						tFilePathRemap,
						tHStringRemap);
				}
			}
		}
		break;

		// The relative filename of a FilePath must be remapped.
	case DataNode::kFilePath:
		{
			GameDirectory eDirectory = GameDirectory::kUnknown;
			HStringData::InternalIndexType uRawHStringValue = 0;
			FileType eFileType = FileType::kUnknown;

			// Get the raw value, remap it, and then restore the FilePath.
			rNode.GetFilePathRaw(eDirectory, uRawHStringValue, eFileType);
			RemapForLoad(uRawHStringValue, tFilePathRemap);
			rNode.SetFilePathRaw(eDirectory, uRawHStringValue, eFileType);
		}
		break;

		// Tables - we need to remap the keys and possibly the values.
	case DataNode::kTable:
		{
			// Get the table container header.
			UInt32 const uIndex = rNode.GetHandle().GetIndex();

			// Protect against out of range indices.
			if (uIndex >= vHandleOffsets.GetSize())
			{
				return;
			}

			HandleDataOffset const entry = vHandleOffsets[uIndex];

			// Protect against invalid or corrupted data entries.
			if (entry.GetDataOffset() >= rvData.GetSize())
			{
				return;
			}

			Container const container = rvData[entry.GetDataOffset()].AsContainer();

			// If the table has a non-zero main portion, or null storage, remap it.
			if ((container.GetCapacityExcludingNull() + container.GetHasNullStorage()) > 0)
			{
				// Values are immediately after the container header, keys are immediately
				// after the values.
				DataNode* pValues = ((DataNode*)rvData.Get(entry.GetDataOffset() + kuContainerSizeInDataEntries));
				HStringData::InternalIndexType* pKeys = (HStringData::InternalIndexType*)(pValues + container.GetCapacityExcludingNull() + container.GetHasNullStorage());

				// Enumerate the main portion and remap keys and values.
				for (UInt32 i = 0u; i < container.GetCapacityExcludingNull(); ++i)
				{
					// 0 in the serialized data indicates the placeholder key, so no need to remap it.
					// TODO: This relies on the assumption that 0 is always the
					// empty string in HString.
					if (0u != pKeys[i])
					{
						RemapForLoad(
							pKeys[i],
							tHStringRemap);

						RemapForLoad(
							pValues[i],
							vHandleOffsets,
							rvData,
							tFilePathRemap,
							tHStringRemap);
					}
				}

				// If there's a nullptr value in the nullptr storage, remap it.
				if (container.GetHasNull())
				{
					RemapForLoad(
						// nullptr value is stored at the end of the main portion of the values array.
						pValues[container.GetCapacityExcludingNull()],
						vHandleOffsets,
						rvData,
						tFilePathRemap,
						tHStringRemap);
				}
			}
		}
		break;
	default:
		// Nop
		break;
	};
}

/** Take a runtime HString value and remap it to a serializable value, adding to the serialized string table, if necessary. */
void DataStore::RemapForSave(
	Platform ePlatform,
	HStringData::InternalIndexType& ruRawHStringValue,
	SerializedStringTable& rvSerializedStringTable,
	UInt32& ruSerializedStrings,
	RemapForSaveTable& tRemap,
	Bool bRelativeFilename)
{
	// ruHStringValue is a valid runtime identifier on input.
	HString identifier;
	identifier.SetHandleValue(ruRawHStringValue);

	// Look if we already have a remap value - if so, just return it.
	HStringData::InternalIndexType uRemap = 0u;
	if (!tRemap.GetValue(identifier, uRemap))
	{
		// Remap value is the next position in the lookup table.
		auto const uNext = (HStringData::InternalIndexType)rvSerializedStringTable.GetSize();
		UInt32 const zStringLengthInBytes = identifier.GetSizeInBytes();
		Byte const* s = identifier.CStr();

		// Allocate space for the string data - this is the length of the string + 1.
		rvSerializedStringTable.Resize(
			uNext + zStringLengthInBytes + 1u,
			(Byte)0);

		// Copy it through.
		memcpy(rvSerializedStringTable.Get(uNext), s, zStringLengthInBytes);

		// Make sure it is nullptr terminate.
		rvSerializedStringTable[uNext + zStringLengthInBytes] = '\0';

		// If a relative filename, check if we need to convert directory separators.
		if (bRelativeFilename)
		{
			// Target platform differs from current, remap the separators in the copied string data.
			Byte const chTarget = (Byte)Path::GetDirectorySeparatorChar(ePlatform);
			Byte const chCurrent = (Byte)Path::GetDirectorySeparatorChar(keCurrentPlatform);
			if (chCurrent != chTarget)
			{
				for (UInt32 i = uNext; i < (uNext + zStringLengthInBytes); ++i)
				{
					Byte& ch = rvSerializedStringTable[i];
					if (chCurrent == ch)
					{
						ch = chTarget;
					}
				}
			}
		}

		// Insert the remap entry - must use 1-based since 0 is always the empty string.
		uRemap = ++ruSerializedStrings;

		// Make sure we don't exceed the max range of HString handle values.
		SEOUL_ASSERT(uRemap < SEOUL_FILEPATH_HSTRING_VALUE_SIZE);

		// Done, store the remap.
		SEOUL_VERIFY(tRemap.Insert(identifier, uRemap).Second);
	}

	// Resulting remap.
	ruRawHStringValue = uRemap;
}

/**
 * Iterate over a DataStore and convert it to fully serializable
 * data, populating the serializable string table while doing so.
 */
void DataStore::RemapForSave(
	Platform ePlatform,
	DataNode& rNode,
	const HandleDataOffsets& vHandleOffsets,
	Data& rvData,
	SerializedStringTable& rvSerializedFilePathTable,
	UInt32& ruSerializedFilePathTableStrings,
	RemapForSaveTable& tFilePathRemap,
	SerializedStringTable& rvSerializedHStringTable,
	UInt32& ruSerializedHStringTableStrings,
	RemapForSaveTable& tHStringRemap)
{
	switch (rNode.GetType())
	{
		// Arrays, need to remap all values in the array.
	case DataNode::kArray:
		{
			// Get the array container remap header.
			UInt32 const uIndex = rNode.GetHandle().GetIndex();

			// Protect against out of range indices.
			if (uIndex >= vHandleOffsets.GetSize())
			{
				return;
			}

			HandleDataOffset const entry = vHandleOffsets[uIndex];

			// Protect against out of range indices.
			if (entry.GetDataOffset() >= rvData.GetSize())
			{
				return;
			}

			Container const container = rvData[entry.GetDataOffset()].AsContainer();

			// If the array has at least one entry, get the values portion and remap it.
			if (container.GetCountExcludingNull() > 0u)
			{
				// Values are immediately after the container header.
				DataNode* pValues = ((DataNode*)rvData.Get(entry.GetDataOffset() + kuContainerSizeInDataEntries));

				// Iterate and remap each entry.
				for (UInt32 i = 0u; i < container.GetCountExcludingNull(); ++i)
				{
					RemapForSave(
						ePlatform,
						pValues[i],
						vHandleOffsets,
						rvData,
						rvSerializedFilePathTable,
						ruSerializedFilePathTableStrings,
						tFilePathRemap,
						rvSerializedHStringTable,
						ruSerializedHStringTableStrings,
						tHStringRemap);
				}
			}
		}
		break;

		// FilePath, need to remap the relative filename portion.
	case DataNode::kFilePath:
		{
			GameDirectory eDirectory = GameDirectory::kUnknown;
			HStringData::InternalIndexType uRawHStringValue = 0;
			FileType eFileType = FileType::kUnknown;

			// Get the raw FilePath values, remap the relative filename, and then
			// restore it. The final true argument applies special processing to
			// normalize directory separator characters to the target platform.
			rNode.GetFilePathRaw(eDirectory, uRawHStringValue, eFileType);
			RemapForSave(ePlatform, uRawHStringValue, rvSerializedFilePathTable, ruSerializedFilePathTableStrings, tFilePathRemap, true);
			rNode.SetFilePathRaw(eDirectory, uRawHStringValue, eFileType);
		}
		break;

		// Table, need to remap all keys and values.
	case DataNode::kTable:
		{
			// Get the table container header.
			UInt32 const uIndex = rNode.GetHandle().GetIndex();

			// Protect against out of range indices.
			if (uIndex >= vHandleOffsets.GetSize())
			{
				return;
			}

			HandleDataOffset const entry = vHandleOffsets[uIndex];
			auto const uContainerOffset = entry.GetDataOffset() + (sizeof(Container) / sizeof(DataNode));

			// Protect against invalid or corrupted data entries.
			if (uContainerOffset > rvData.GetSize())
			{
				return;
			}

			Container const container = rvData[entry.GetDataOffset()].AsContainer();

			// Protect against invalid container header.
			if (container.GetCountExcludingNull() > container.GetCapacityExcludingNull() ||
				container.GetCapacityExcludingNull() + uContainerOffset > rvData.GetSize())
			{
				return;
			}

			// Process if the table has at least 1 entry in the main portion, or nullptr storage.
			if ((container.GetCapacityExcludingNull() + container.GetHasNullStorage()) > 0)
			{
				// Values are immediately after the container header.
				DataNode* pValues = ((DataNode*)rvData.Get(entry.GetDataOffset() + kuContainerSizeInDataEntries));

				// Keys are immediately after the value portion.
				HStringData::InternalIndexType* pKeys = (HStringData::InternalIndexType*)(pValues + container.GetCapacityExcludingNull() + container.GetHasNullStorage());

				// Iterate over the main portion.
				for (UInt32 i = 0u; i < container.GetCapacityExcludingNull(); ++i)
				{
					// 0 in the data indicates the placeholder key, so no need to remap it.
					// TODO: This relies on the assumption that 0 is always the
					// empty string in HString.
					if (0u != pKeys[i])
					{
						RemapForSave(
							ePlatform,
							pKeys[i],
							rvSerializedHStringTable,
							ruSerializedHStringTableStrings,
							tHStringRemap,
							false);

						RemapForSave(
							ePlatform,
							pValues[i],
							vHandleOffsets,
							rvData,
							rvSerializedFilePathTable,
							ruSerializedFilePathTableStrings,
							tFilePathRemap,
							rvSerializedHStringTable,
							ruSerializedHStringTableStrings,
							tHStringRemap);
					}
				}

				// If there's a nullptr value in the nullptr storage, remap it
				if (container.GetHasNull())
				{
					RemapForSave(
						ePlatform,
						// nullptr value is stored immediately after the main portion of the values array.
						pValues[container.GetCapacityExcludingNull()],
						vHandleOffsets,
						rvData,
						rvSerializedFilePathTable,
						ruSerializedFilePathTableStrings,
						tFilePathRemap,
						rvSerializedHStringTable,
						ruSerializedHStringTableStrings,
						tHStringRemap);
				}
			}
		}
		break;
	default:
		// Nop
		break;
	};
}

/**
 * Write this DataStore in a serializable binary format to rFile.
 *
 * @param[in] ePlatform Target platform for the file. Affects endianess
 * and normalization of filenames for the output data.
 * @param[in] bCompact If true, CollectGarbageAndCompactHeap() is run
 *                     on the DataStore prior to save.
 */
Bool DataStore::Save(SyncFile& rFile, Platform ePlatform, Bool bCompact /*= true*/) const
{
	// TODO: Implement big endian support.
	SEOUL_STATIC_ASSERT(SEOUL_LITTLE_ENDIAN);

	if (!rFile.CanWrite())
	{
		return false;
	}

	DataStore thisCopy;
	thisCopy.CopyFrom(*this);
	if (bCompact)
	{
		thisCopy.CollectGarbageAndCompactHeap();
		// This is important prior to serialization (to minimize the
		// size of the handle offset table) but not safe in general,
		// since if a DataStore has public references to a handle,
		// this step can invalid those handles. Therefore, it is
		// a private method and can only be called on the copy here
		// as part of compaction during save.
		thisCopy.InternalCompactHandleOffsets();
	}

	SerializedStringTable vSerializedFilePathTable;
	UInt32 uSerializedFilePathTableStrings = 0u;
	RemapForSaveTable tFilePathRemap;
	SerializedStringTable vSerializedHStringTable;
	UInt32 uSerializedHStringTableStrings = 0u;
	RemapForSaveTable tHStringRemap;

	if (!thisCopy.m_vData.IsEmpty())
	{
		// Add an empty string at index 0.
		vSerializedFilePathTable.PushBack((Byte)0);
		SEOUL_VERIFY(tFilePathRemap.Insert(HString(), 0u).Second);
		vSerializedHStringTable.PushBack((Byte)0);
		SEOUL_VERIFY(tHStringRemap.Insert(HString(), 0u).Second);

		RemapForSave(
			ePlatform,
			thisCopy.m_vData.Front().AsDataNode(),
			thisCopy.m_vHandleDataOffsets,
			thisCopy.m_vData,
			vSerializedFilePathTable,
			uSerializedFilePathTableStrings,
			tFilePathRemap,
			vSerializedHStringTable,
			uSerializedHStringTableStrings,
			tHStringRemap);
	}

	// Write the cooked binary signature.
	if (sizeof(kaCookedDataStoreBinarySignature) != rFile.WriteRawData(&kaCookedDataStoreBinarySignature[0], sizeof(kaCookedDataStoreBinarySignature))) { return false; }

	// Write the cooked binary version.
	if (!WriteUInt32(rFile, kuCookedDataStoreBinaryVersion)) { return false; }

	// Serialize out the FilePath remapped string table.
	if (!WriteBuffer(rFile, vSerializedFilePathTable)) { return false; }

	// Serialize out the HString remapped string table.
	if (!WriteBuffer(rFile, vSerializedHStringTable)) { return false; }

	// Serialize all our internal vectors of data.
	if (!WriteBuffer(rFile, thisCopy.m_vHandleDataOffsets)) { return false; }
	if (!WriteBuffer(rFile, thisCopy.m_vData)) { return false; }

	// Individual state.
	if (!WriteUInt32(rFile, thisCopy.m_uDataSizeAfterLastCollection)) { return false; }
	if (!WriteUInt32(rFile, thisCopy.m_uNextHandle)) { return false; }
	if (!WriteUInt32(rFile, thisCopy.m_uAllocatedHandles)) { return false; }
	if (!WriteUInt32(rFile, thisCopy.m_uSuppressGarbageCollection)) { return false; }

	return true;
}

#if SEOUL_UNIT_TESTS
/* Hook for testing, selectively disable handle compaction on load. */
Bool g_bUnitTestOnlyDisableDataStoreHandleCompactionOnLoad = false;
#endif

/**
 * Read this DataStore from a serialized binary format in rFile.
 *
 * rFile must contained serailized data written with
 * DataStore::Save(), targeting the current platform.
 */
Bool DataStore::Load(SyncFile& rFile)
{
	// TODO: Implement big endian support.
	SEOUL_STATIC_ASSERT(SEOUL_LITTLE_ENDIAN);

	Bool bVersion0 = false;

	// Read and check the cooked binary signature string.
	{
		UByte aSignature[SEOUL_ARRAY_COUNT(kaCookedDataStoreBinarySignature)];
		if (sizeof(aSignature) != rFile.ReadRawData(&aSignature[0], sizeof(aSignature))) { return false; }
		if (0 != memcmp(&aSignature[0], &kaCookedDataStoreBinarySignature[0], sizeof(aSignature)))
		{
			// Check for a version 0 format blob.
			if (0 == memcmp(&aSignature[0], &kaCookedDataStoreBinarySignatureVersion0[0], sizeof(aSignature)))
			{
				bVersion0 = true;
			}
			else
			{
				return false;
			}
		}
	}

	// Read and check the cooked binary version, unless this is a version 0 file.
	Bool bUseDeprecatedStringByteOffsetForRemap = false;
	if (!bVersion0)
	{
		UInt32 uVersion = 0u;
		if (!ReadUInt32(rFile, uVersion)) { return false; }

		// We have backwards handling for version 1.
		SEOUL_STATIC_ASSERT(2u == kuCookedDataStoreBinaryVersion);
		if (kuCookedDataStoreBinaryVersion != uVersion)
		{
			// Version 1 stored string table offsets differently -
			// they were byte offsets into the single large string
			// table buffer instead of an offset based on string count.
			if (1u == uVersion)
			{
				bUseDeprecatedStringByteOffsetForRemap = true;
			}
			else
			{
				// Unsupported or unrecognized version.
				return false;
			}
		}
	}

	// Populate a local instance, so we don't leave this DataStore partially modified
	// on failure.
	DataStore dataStore;

	SerializedStringTable vSerializedFilePathTable;
	SerializedStringTable vSerializedHStringTable;

	// Deserialize the FilePath remapped string table.
	if (!ReadBuffer(rFile, vSerializedFilePathTable)) { return false; }

	// Deserialize the HString remapped string table.
	if (!ReadBuffer(rFile, vSerializedHStringTable)) { return false; }

	// Read all fields.
	if (!ReadBuffer(rFile, dataStore.m_vHandleDataOffsets)) { return false; }
	if (!ReadBuffer(rFile, dataStore.m_vData)) { return false; }

	if (!ReadUInt32(rFile, dataStore.m_uDataSizeAfterLastCollection)) { return false; }
	if (!ReadUInt32(rFile, dataStore.m_uNextHandle)) { return false; }
	if (!ReadUInt32(rFile, dataStore.m_uAllocatedHandles)) { return false; }
	if (!ReadUInt32(rFile, dataStore.m_uSuppressGarbageCollection)) { return false; }

	// Process the file path string table.
	RemapForLoadTable tFilePathRemap;
	if (!vSerializedFilePathTable.IsEmpty())
	{
		// TODO: Add a platform field to the DataStore data and
		// used that to determine this, instead of scanning for it.
		//
		// Before loading, scan for a directory separator. If it is not
		// the same as the current platform, continue and replace. Otherwise,
		// stop.
		for (auto& ch : vSerializedFilePathTable)
		{
			if (Path::kDirectorySeparatorChar == ch)
			{
				break;
			}
			else if (Path::kAltDirectorySeparatorChar == ch)
			{
				ch = Path::kDirectorySeparatorChar;
			}
		}

		// Now load the relative filename portion of the file paths.
		UInt32 uCount = 0u;
		UInt32 u = 0u;
		while (u < vSerializedFilePathTable.GetSize())
		{
			HString const hstring(vSerializedFilePathTable.Get(u), true);
			if (bUseDeprecatedStringByteOffsetForRemap)
			{
				// Version 1 format, remap index is offset into the buffer.
				SEOUL_VERIFY(tFilePathRemap.Insert(u, hstring).Second);
			}
			else
			{
				// Version 2 format, remap index is index of the string.
				SEOUL_VERIFY(tFilePathRemap.Insert(uCount, hstring).Second);
			}
			u += (hstring.GetSizeInBytes() + 1u);
			++uCount;
		}
	}

	// Process the HString table.
	RemapForLoadTable tHStringRemap;
	if (!vSerializedHStringTable.IsEmpty())
	{
		UInt32 uCount = 0u;
		UInt32 u = 0u;
		while (u < vSerializedHStringTable.GetSize())
		{
			HString const hstring(vSerializedHStringTable.Get(u), false);
			if (bUseDeprecatedStringByteOffsetForRemap)
			{
				// Version 1 format, remap index is offset into the buffer.
				SEOUL_VERIFY(tHStringRemap.Insert(u, hstring).Second);
			}
			else
			{
				// Version 2 format, remap index is index of the string.
				SEOUL_VERIFY(tHStringRemap.Insert(uCount, hstring).Second);
			}
			u += (hstring.GetSizeInBytes() + 1u);
			++uCount;
		}
	}

	if (!dataStore.m_vData.IsEmpty())
	{
		RemapForLoad(
			dataStore.m_vData.Front().AsDataNode(),
			dataStore.m_vHandleDataOffsets,
			dataStore.m_vData,
			tFilePathRemap,
			tHStringRemap);
	}

#if SEOUL_UNIT_TESTS
	/* Hook for testing, selectively disable handle compaction on load. */
	if (!g_bUnitTestOnlyDisableDataStoreHandleCompactionOnLoad)
#endif
	{
		// If m_vHandleDataOffsets is significantly larger than m_uAllocatedHandles, compact it to make
		// it take up less space. Note that InternalCompactHandleOffsets can only be called before anything
		// starts to access the DataStore, as it invalidates and recreates all handles.
		if (dataStore.m_vHandleDataOffsets.GetSize() > GetNextPowerOf2(dataStore.m_uAllocatedHandles))
		{
			dataStore.InternalCompactHandleOffsets();
		}
	}

	// Success - swap in the result and return success.
	Swap(dataStore);

	return true;
}

/**
 * Verify the integrity of the DataStore. This validates that all internal handles of the DataStore
 * are valid, and all DataNode types are reasonable. Typically, you want to run this after Load()
 * on data loaded from untrustable sources (user storage or clients).
 */
Bool DataStore::VerifyIntegrity() const
{
	return InternalVerifyIntegrity(GetRootNode());
}

#if SEOUL_UNIT_TESTS
/**
 * Unit test hook only, used to check if DataStore A and DataStore B are
 * byte-for-byte equal (memcmp on buffers and members).
 */
Bool DataStore::UnitTestHook_ByteForByteEqual(const DataStore& a, const DataStore& b)
{
	return (
		a.m_vHandleDataOffsets.GetSize() == b.m_vHandleDataOffsets.GetSize() &&
		(a.m_vHandleDataOffsets.IsEmpty() || (0 == memcmp(a.m_vHandleDataOffsets.Data(), b.m_vHandleDataOffsets.Data(), a.m_vHandleDataOffsets.GetSizeInBytes()))) &&
		a.m_vData.GetSize() == b.m_vData.GetSize() &&
		(a.m_vData.IsEmpty() || (0 == memcmp(a.m_vData.Data(), b.m_vData.Data(), a.m_vData.GetSizeInBytes()))) &&
		a.m_uDataSizeAfterLastCollection == b.m_uDataSizeAfterLastCollection &&
		a.m_uNextHandle == b.m_uNextHandle &&
		a.m_uAllocatedHandles == b.m_uAllocatedHandles &&
		a.m_uSuppressGarbageCollection == b.m_uSuppressGarbageCollection);
}

/**
 * Unit test hook only, fills the DataStore with intentionally corrupted data.
 */
void DataStore::UnitTestHook_FillWithCorruptedData(CorruptedDataType eType)
{
	DataStore emptyDataStore;
	Swap(emptyDataStore);

	MakeTable();
	SetArrayToTable(GetRootNode(), HString("A"));
	SetTableToTable(GetRootNode(), HString("B"));
	{
		DataNode node;
		GetValueFromTable(GetRootNode(), HString("A"), node);
		SetUInt32ValueToArray(node, 0u, 255u);
		SetBooleanValueToArray(node, 1u, true);
		SetStringToArray(node, 3u, "Hello There");
		SetFilePathToArray(node, 4u, FilePath::CreateConfigFilePath("Hi"));
		SetFloat32ValueToArray(node, 6u, 4.1f);
		SetFloat32ValueToArray(node, 7u, 1.5f);

		SetArrayToArray(node, 5u);
		GetValueFromArray(node, 5u, node);
		SetFloat32ValueToArray(node, 0u, 77.7f);
	}
	{
		DataNode node;
		GetValueFromTable(GetRootNode(), HString("B"), node);
		SetFloat32ValueToTable(node, HString("1"), 1.5f);
		SetUInt64ValueToTable(node, HString("2"), UInt64Max);
		SetNullValueToTable(node, HString("3"));
		SetInt64ValueToTable(node, HString("4"), IntMin);
		SetFloat32ValueToTable(node, HString("5"), 1.6666f);
	}

	switch (eType)
	{
	// Mess up the container header of the "5" array.
	case kCorruptedArrayCapacity:
		{
			DataNode node;
			SEOUL_VERIFY(GetValueFromTable(GetRootNode(), HString("A"), node));
			SEOUL_VERIFY(GetValueFromArray(node, 5u, node));
			Container* pTarget = (Container*)InternalGetDataEntryPtr(node.GetHandle());
			pTarget->SetCapacityExcludingNull((m_vData.GetSize() - m_vHandleDataOffsets[node.GetHandle().GetIndex()].GetDataOffset()) + 1u);
		}
		break;

	// Mess up the container header of the "5" array.
	case kCorruptedArrayCount:
		{
			DataNode node;
			SEOUL_VERIFY(GetValueFromTable(GetRootNode(), HString("A"), node));
			SEOUL_VERIFY(GetValueFromArray(node, 5u, node));
			Container* pTarget = (Container*)InternalGetDataEntryPtr(node.GetHandle());
			pTarget->SetCountExcludingNull(pTarget->GetCapacityExcludingNull() + 1u);
		}
		break;

		// Mess up the handle to the "5" array.
	case kCorruptedArrayHandle:
		{
			Container* pContainer = (Container*)InternalGetDataEntryPtr(GetRootNode().GetHandle());
			DataNode* pValues = (DataNode*)(pContainer + 1u);
			HString const* pKeys = (HString const*)(pValues + (pContainer->GetCapacityExcludingNull() + pContainer->GetHasNullStorage()));
			for (UInt32 i = 0; i < pContainer->GetCapacityExcludingNull(); ++i)
			{
				if (pKeys[i] == HString("A"))
				{
					Container* pInnerContainer = (Container*)InternalGetDataEntryPtr(pValues[i].GetHandle());
					DataNode* pInnerValues = (DataNode*)(pInnerContainer + 1u);

					DataNode::Handle h = DataNode::Handle::Default();
					h.SetGenerationId(2u);
					h.SetIndex(5923777u);
					pInnerValues[5u].SetHandle(h, DataNode::kArray);
					break;
				}
			}
		}
		break;

		// Mess up the offset data of the "5" array.
	case kCorruptedArrayOffset:
		{
			DataNode node;
			SEOUL_VERIFY(GetValueFromTable(GetRootNode(), HString("A"), node));
			SEOUL_VERIFY(GetValueFromArray(node, 5u, node));
			m_vHandleDataOffsets[node.GetHandle().GetIndex()].SetDataOffset(6351633u);
		}
		break;

		// Mess up the capacity data of a string.
	case kCorruptedStringCapacity:
		{
			DataNode node;
			SEOUL_VERIFY(GetValueFromTable(GetRootNode(), HString("A"), node));
			SEOUL_VERIFY(GetValueFromArray(node, 3u, node));
			Container* pTarget = (Container*)InternalGetDataEntryPtr(node.GetHandle());
			pTarget->SetCapacityExcludingNull((m_vData.GetSize() - m_vHandleDataOffsets[node.GetHandle().GetIndex()].GetDataOffset()) + 1u);
		}
		break;

		// Mess up the count data of a string.
	case kCorruptedStringCount:
		{
			DataNode node;
			SEOUL_VERIFY(GetValueFromTable(GetRootNode(), HString("A"), node));
			SEOUL_VERIFY(GetValueFromArray(node, 3u, node));
			Container* pTarget = (Container*)InternalGetDataEntryPtr(node.GetHandle());

			// Count is size of the string in bytes, excluding the null terminator (which is always present).
			pTarget->SetCountExcludingNull((pTarget->GetCapacityExcludingNull() * sizeof(DataNode) + 1u) + 1u);
		}
		break;

		// Mess up the handle of a string.
	case kCorruptedStringHandle:
		{
			Container* pContainer = (Container*)InternalGetDataEntryPtr(GetRootNode().GetHandle());
			DataNode* pValues = (DataNode*)(pContainer + 1u);
			HString const* pKeys = (HString const*)(pValues + (pContainer->GetCapacityExcludingNull() + pContainer->GetHasNullStorage()));
			for (UInt32 i = 0; i < pContainer->GetCapacityExcludingNull(); ++i)
			{
				if (pKeys[i] == HString("A"))
				{
					Container* pInnerContainer = (Container*)InternalGetDataEntryPtr(pValues[i].GetHandle());
					DataNode* pInnerValues = (DataNode*)(pInnerContainer + 1u);

					DataNode::Handle h = DataNode::Handle::Default();
					h.SetGenerationId(2u);
					h.SetIndex(2138666u);
					pInnerValues[3u].SetHandle(h, DataNode::kString);
					break;
				}
			}
		}
		break;

		// Mess up the offset of a string.
	case kCorruptedStringOffset:
		{
			DataNode node;
			SEOUL_VERIFY(GetValueFromTable(GetRootNode(), HString("A"), node));
			SEOUL_VERIFY(GetValueFromArray(node, 3u, node));
			m_vHandleDataOffsets[node.GetHandle().GetIndex()].SetDataOffset(3052173u);
		}
		break;

		// Mess up the container header of the "B" table.
	case kCorruptedTableCapacity:
		{
			DataNode node;
			SEOUL_VERIFY(GetValueFromTable(GetRootNode(), HString("B"), node));
			Container* pTarget = (Container*)InternalGetDataEntryPtr(node.GetHandle());
			pTarget->SetCapacityExcludingNull((m_vData.GetSize() - m_vHandleDataOffsets[node.GetHandle().GetIndex()].GetDataOffset()) + 1u);
		}
		break;

		// Mess up the container header of the "B" table.
	case kCorruptedTableCount:
		{
			DataNode node;
			SEOUL_VERIFY(GetValueFromTable(GetRootNode(), HString("B"), node));
			Container* pTarget = (Container*)InternalGetDataEntryPtr(node.GetHandle());
			pTarget->SetCountExcludingNull(pTarget->GetCapacityExcludingNull() + 1u);
		}
		break;

		// Mess up the handle to the "B" table.
	case kCorruptedTableHandle:
		{
			Container* pContainer = (Container*)InternalGetDataEntryPtr(GetRootNode().GetHandle());
			DataNode* pValues = (DataNode*)(pContainer + 1u);
			HString const* pKeys = (HString const*)(pValues + (pContainer->GetCapacityExcludingNull() + pContainer->GetHasNullStorage()));
			for (UInt32 i = 0; i < pContainer->GetCapacityExcludingNull(); ++i)
			{
				if (pKeys[i] == HString("B"))
				{
					DataNode::Handle h = DataNode::Handle::Default();
					h.SetGenerationId(4u);
					h.SetIndex(1082340u);
					pValues[i].SetHandle(h, DataNode::kTable);
					break;
				}
			}
		}
		break;

		// Mess up the offset data of the "B" table.
	case kCorruptedTableOffset:
		{
			DataNode node;
			SEOUL_VERIFY(GetValueFromTable(GetRootNode(), HString("B"), node));
			m_vHandleDataOffsets[node.GetHandle().GetIndex()].SetDataOffset(2135421u);
		}
		break;

		// Mess up the type field of a DataNode.
	case kCorruptedTypeData:
		{
			Container* pContainer = (Container*)InternalGetDataEntryPtr(GetRootNode().GetHandle());
			DataNode* pValues = (DataNode*)(pContainer + 1u);
			HString const* pKeys = (HString const*)(pValues + (pContainer->GetCapacityExcludingNull() + pContainer->GetHasNullStorage()));
			for (UInt32 i = 0; i < pContainer->GetCapacityExcludingNull(); ++i)
			{
				if (pKeys[i] == HString("A"))
				{
					Container* pInnerContainer = (Container*)InternalGetDataEntryPtr(pValues[i].GetHandle());
					DataNode* pInnerValues = (DataNode*)(pInnerContainer + 1u);

					pInnerValues[1u].SetNotFloat31Value(1u, (DataNode::Type)(DataNode::LAST_TYPE + 2));
					break;
				}
			}
		}
		break;

	default:
		SEOUL_FAIL("Out of sync enum.");
		break;
	};
}
#endif // /#if SEOUL_UNIT_TESTS

/**
 * @return Newly allocated memory capable of containing nNumberOfDataEntries
 * DataNode values, assigned to a new handle.
 */
DataNode::Handle DataStore::InternalAllocate(UInt32 nNumberOfDataEntries)
{
	// If we're using all Handles, first try to collect garbage to free up some handles.
	if (m_uAllocatedHandles > 0u &&
		m_vHandleDataOffsets.GetSize() == m_uAllocatedHandles &&
		m_vHandleDataOffsets.GetSize() >= kMaxHandleCount)
	{
		InternalCollectGarbage();
	}

	return InternalAllocateHandle(InternalAllocateRaw(nNumberOfDataEntries));
}

/**
 * @return An offset to newly allocated data - identical to InternalAllocate(),
 * except does not allocate a handle for the new data (can be used when
 * a new memory block will be swapped out for an old block in an existing
 * handle.
 */
UInt32 DataStore::InternalAllocateRaw(UInt32 nNumberOfDataEntries)
{
	// Garbage collect when necessary.
	if (m_uDataSizeAfterLastCollection * kuDataStoreAutoGarbageCollectionFactor <= m_vData.GetSizeInBytes())
	{
		InternalCollectGarbage();
	}

	UInt32 const dataOffset = m_vData.GetSize();
	m_vData.Resize(dataOffset + nNumberOfDataEntries, DataEntry::Default());

	return dataOffset;
}

/**
 * Resize the memory associated with a handle.
 */
void DataStore::InternalReallocate(
	UInt32 nOldNumberOfDataEntries,
	UInt32 nNewNumberOfDataEntries,
	DataNode::Handle handle)
{
	SEOUL_ASSERT(nNewNumberOfDataEntries > nOldNumberOfDataEntries);

	// Garbage collect when necessary.
	if (m_uDataSizeAfterLastCollection * kuDataStoreAutoGarbageCollectionFactor <= m_vData.GetSizeInBytes())
	{
		InternalCollectGarbage();
	}

	HandleDataOffset& rHandleDataOffset = m_vHandleDataOffsets[handle.GetIndex()];

	// If the data entry is at the end of our heap already, just grow the heap.
	if (rHandleDataOffset.GetDataOffset() + nOldNumberOfDataEntries == m_vData.GetSize())
	{
		m_vData.Resize(
			m_vData.GetSize() + (nNewNumberOfDataEntries - nOldNumberOfDataEntries),
			DataEntry::Default());
	}
	// Otherwise, allocate a new area at the end of the heap and copy the old data.
	else
	{
		UInt32 const dataOffset = m_vData.GetSize();
		m_vData.Resize(dataOffset + nNewNumberOfDataEntries, DataEntry::Default());
		memcpy(m_vData.Get(dataOffset), m_vData.Get(rHandleDataOffset.GetDataOffset()), nOldNumberOfDataEntries * sizeof(DataEntry));
		rHandleDataOffset.SetDataOffset(dataOffset);
	}
}

/**
 * @return A new handle pointing at the DataNode in m_vData at uDataOffset.
 */
DataNode::Handle DataStore::InternalAllocateHandle(UInt32 uDataOffset)
{
	// Sanity check the data offset.
	SEOUL_ASSERT(uDataOffset < HandleDataOffset::kuInvalidHandleOffset);

	// If we're using all Handles in the current handles vector
	// or if the vector size is now power of 2, increase its size.
	if (m_vHandleDataOffsets.GetSize() == m_uAllocatedHandles ||
		!IsPowerOfTwo(m_vHandleDataOffsets.GetSize()))
	{
		UInt32 const nOldSize = m_vHandleDataOffsets.GetSize();
		UInt32 const nNewSize = GetNextPowerOf2(Max(1u, nOldSize + 1));

		// Sanity checks.
		SEOUL_ASSERT(nNewSize <= kMaxHandleCount);
		SEOUL_ASSERT(nNewSize > nOldSize);

		// Resize and zero out new handle data.
		m_vHandleDataOffsets.Resize(nNewSize, HandleDataOffset::Default());

		// Update the next handle to point at the next entry.
		m_uNextHandle = nOldSize;
	}

	// There is guaranteed to be at least 1u free handle in the table by the previous code,
	// so this will return.
	while (true)
	{
		// Wrap the next handle index around.
		SEOUL_ASSERT(!m_vHandleDataOffsets.IsEmpty());
		SEOUL_ASSERT(IsPowerOfTwo(m_vHandleDataOffsets.GetSize()));
		m_uNextHandle = (m_uNextHandle & (m_vHandleDataOffsets.GetSize() - 1u));

		// If the current handle does not have a valid data offset, we have a free handle.
		if (!m_vHandleDataOffsets[m_uNextHandle].IsValid())
		{
			// The handle index must be in the range of the max handle count - catch it
			// if we've gone out of range.
			SEOUL_ASSERT(m_uNextHandle <= (kMaxHandleCount - 1u));

			HandleDataOffset& rHandleDataOffset = m_vHandleDataOffsets[m_uNextHandle];

			// Increment the generation ID - it is fine (and expected, eventually) for this
			// to overflow.
			rHandleDataOffset.SetGenerationId(rHandleDataOffset.GetGenerationId() + 1);

			// The data offset is a 32-bit unsigned integer.
			rHandleDataOffset.SetDataOffset(uDataOffset);

			// Create the handle before increment values.
			DataNode::Handle ret;
			ret.SetGenerationId(rHandleDataOffset.GetGenerationId());
			ret.SetIndex(m_uNextHandle);
			ret.SetUnusedReserved(0u);

			// Increment the next handle start point, and the total number of handles.
			++m_uNextHandle;
			++m_uAllocatedHandles;

			return ret;
		}

		++m_uNextHandle;
	}
}

/**
 * Clear the handle table - this resets index and offset of all handles but
 * leaves the generation id unchanged.
 */
void DataStore::InternalClearHandles()
{
	UInt32 const uHandleDataOffsets = m_vHandleDataOffsets.GetSize();
	for (UInt32 i = 0u; i < uHandleDataOffsets; ++i)
	{
		m_vHandleDataOffsets[i].SetDataOffset(HandleDataOffset::kuInvalidHandleOffset);
	}

	m_uAllocatedHandles = 0u;
}

/**
 * @return A handle to a new array with initial capacity uInitialCapacity.
 */
DataNode::Handle DataStore::InternalCreateArray(UInt32 uInitialCapacity)
{
	DataNode::Handle handle = InternalAllocate(uInitialCapacity + kuContainerSizeInDataEntries);

	Container* pContainer = (Container*)InternalGetDataEntryPtr(handle);
	pContainer->SetCapacityExcludingNull(uInitialCapacity);
	pContainer->SetCountExcludingNull(0u);

	return handle;
}

/**
 * @return A handle to a copy of string sString allocated on the DataStore's heap.
 */
DataNode::Handle DataStore::InternalCreateString(
	Byte const* sString,
	UInt32 zStringLengthInBytes)
{
	// Get the capacity - all capacities are in DataNodes, so we round up the size
	// needed for the string, plus kuContainerSizeInDataEntries for the Container header.
	UInt32 const nCapacity = (UInt32)((RoundUpToAlignment(zStringLengthInBytes + 1u, sizeof(DataNode)) / sizeof(DataNode)));
	DataNode::Handle handle = InternalAllocate(nCapacity + kuContainerSizeInDataEntries);

	// Setup the container header - note that the m_nCount variable for a string
	// is the size of the string in bytes, while the capacity is the capacity
	// in DataNodes.
	Container* pContainer = (Container*)InternalGetDataEntryPtr(handle);
	pContainer->SetCapacityExcludingNull(nCapacity);
	pContainer->SetCountExcludingNull(zStringLengthInBytes);

	// Copy the string data, separately set the null terminator so it is guaranteed to be valid.
	memcpy(pContainer + 1u, sString, zStringLengthInBytes);
	*(((Byte*)(pContainer + 1u)) + zStringLengthInBytes) = '\0';

	return handle;
}

/**
 * @return A handle to a copy of string sString allocated on the DataStore's heap. The
 * string sString will be parsed for escape characters, which will be removed. The
 * final string must be zStringLengthInBytesAfterResolvingEscapes in bytes or the results
 * of this call are undefined.
 */
DataNode::Handle DataStore::InternalUnescapeAndCreateString(Byte const* sString, UInt32 zStringLengthInBytesAfterResolvingEscapes)
{
	// Get the capacity - all capacities are in DataNodes, so we round up the size
	// needed for the string, plus kuContainerSizeInDataEntries for the Container header.
	UInt32 const nCapacity = (UInt32)(RoundUpToAlignment(zStringLengthInBytesAfterResolvingEscapes + 1u, sizeof(DataNode)) / sizeof(DataNode));
	DataNode::Handle handle = InternalAllocate(nCapacity + kuContainerSizeInDataEntries);

	// Setup the container header - note that the m_nCount variable for a string
	// is the size of the string in bytes, while the capacity is the capacity
	// in DataNodes.
	Container* pContainer = (Container*)InternalGetDataEntryPtr(handle);
	pContainer->SetCapacityExcludingNull(nCapacity);
	pContainer->SetCountExcludingNull(zStringLengthInBytesAfterResolvingEscapes);

	// Escape the string.
	JSONUnescape(sString, (Byte*)(pContainer + 1u), zStringLengthInBytesAfterResolvingEscapes + 1u);

	// Sanity check the results.
	SEOUL_ASSERT(StrLen((Byte const*)(pContainer + 1u)) == zStringLengthInBytesAfterResolvingEscapes);

	return handle;
}

/**
 * @return A handle to a new table with initial capacity uInitialCapacity.
 */
DataNode::Handle DataStore::InternalCreateTable(UInt32 uInitialCapacity)
{
	// Tables must always have a capacity that is a power of 2.
	uInitialCapacity = GetNextPowerOf2(uInitialCapacity);

	UInt32 const zTableDataSize = GetTableDataSize(uInitialCapacity);
	DataNode::Handle handle = InternalAllocate(zTableDataSize);

	// Get the container header and clear out the entire table memory to 0.
	Container* pContainer = (Container*)InternalGetDataEntryPtr(handle);

	// Set the capacity and count.
	pContainer->SetCapacityExcludingNull(uInitialCapacity);
	pContainer->SetHasNullStorage(false);
	pContainer->SetCountExcludingNull(0u);
	pContainer->SetHasNull(false);

	return handle;
}

/**
 * Garbage collect this DataStore - reduces the memory used
 * to the smallest possible contiguous block.
 */
void DataStore::InternalCollectGarbage(Bool bCompactContainers /*= false*/)
{
	// Early out if 0 != m_nSuppressGarbageCollection
	if (0 != m_uSuppressGarbageCollection)
	{
		return;
	}

	// Nothing to do if we have no data.
	if (m_vData.IsEmpty())
	{
		// Update data size for tracking.
		m_uDataSizeAfterLastCollection = Max(m_vData.GetSizeInBytes(), kuDataStoreAutoGarbageCollectionMarkerSize);
		return;
	}

	UInt32 const zDataSize = m_vData.GetSize();
	UInt32 const zHandlesSize = m_vHandleDataOffsets.GetSize();

	Data vNewData;
	vNewData.Reserve(zDataSize);

	// Offsets are initially all 0.
	HandleDataOffsets vNewHandleDataOffsets;
	vNewHandleDataOffsets.Resize(zHandlesSize, HandleDataOffset::Default());

	// First copy the root node, then copy its contained values.
	DataEntry node = m_vData.Front();
	vNewData.PushBack(node);
	InternalCopyData(node.AsDataNode(), vNewHandleDataOffsets, vNewData, bCompactContainers);

	// Now restore the allocated handles count.
	m_uAllocatedHandles = 0u;
	for (UInt32 i = 0u; i < vNewHandleDataOffsets.GetSize(); ++i)
	{
		if (vNewHandleDataOffsets[i].IsValid())
		{
			++m_uAllocatedHandles;
		}
	}

	// Swap old handle data for new.
	m_vHandleDataOffsets.Swap(vNewHandleDataOffsets);
	m_vData.Swap(vNewData);

	// Update data size for tracking.
	m_uDataSizeAfterLastCollection = Max(m_vData.GetSizeInBytes(), kuDataStoreAutoGarbageCollectionMarkerSize);
}

/**
 * Walks the DataStore hierarchy and computes and MD5.
 * Containers are normalized as needed during the walk so the MD5
 * will be the same if DataStore::Equal() returns true.
 */
void DataStore::InternalComputeMD5(MD5& r, const DataNode& node) const
{
	// Append the type, so that identical valued nodes with
	// different types produce a different MD5.
	r.AppendPOD((Int32)node.GetType());

	switch (node.GetType())
	{
	case DataNode::kArray:
		{
			UInt32 u = 0u;
			(void)GetArrayCount(node, u);

			// Include the array size in case we have a DataStore of only empty containers.
			r.AppendPOD(u);

			for (UInt32 i = 0u; i < u; ++i)
			{
				DataNode child;
				SEOUL_VERIFY(GetValueFromArray(node, i, child));
				InternalComputeMD5(r, child);
			}
		}
		break;
	case DataNode::kBoolean:
		{
			UInt32 const uValue = (AssumeBoolean(node) ? 1u : 0u);
			r.AppendPOD(uValue);
		}
		break;
	case DataNode::kFilePath:
		{
			FilePath filePath;
			SEOUL_VERIFY(AsFilePath(node, filePath));

			// We hash the normalized content URL, converted to lowercase,
			// since FilePaths are case insensitive but locally canonical (the
			// case can differ between two file paths that are equal in two different
			// environments).
			//
			// SeoulEngine FilePaths do not allow unicode characters, so ToLowerASCII() is
			// valid here (Unicode is allowed in the user's absolute path, but the relative
			// file system represented by FilePath must contain only ASCII characters).
			r.AppendData(filePath.ToSerializedUrl().ToLowerASCII());
		}
		break;
	case DataNode::kFloat31:
		// We have to make the floating point number canonical to guarantee
		// equivalent checksum generation for equivalent values.
		ComputeFloatMD5(r, AssumeFloat31(node));
		break;
	case DataNode::kFloat32:
		// We have to make the floating point number canonical to guarantee
		// equivalent checksum generation for equivalent values.
		ComputeFloatMD5(r, AssumeFloat32(node));
		break;
	case DataNode::kInt32Big:
		r.AppendPOD(AssumeInt32Big(node));
		break;
	case DataNode::kInt32Small:
		r.AppendPOD(AssumeInt32Small(node));
		break;
	case DataNode::kInt64:
		r.AppendPOD(AssumeInt64(node));
		break;
	case DataNode::kNull:
		r.AppendPOD((Int32)0);
		break;
	case DataNode::kSpecialErase:
		r.AppendPOD((Int32)DataNode::kSpecialErase);
		break;
	case DataNode::kString:
		{
			Byte const* s = nullptr;
			UInt32 u = 0u;
			(void)AsString(node, s, u);
			r.AppendData(s, u);
		}
		break;
	case DataNode::kTable:
		{
			// TODO: Heap allocation here is unfortunate.

			Vector<HString, MemoryBudgets::DataStore> vScratch;
			{
				auto const iBegin = TableBegin(node);
				auto const iEnd = TableEnd(node);
				for (auto i = iBegin; iEnd != i; ++i)
				{
					vScratch.PushBack(i->First);
				}
			}

			// Now sort so that ordering is consistent.
			LexicalHStringSort sorter;
			QuickSort(vScratch.Begin(), vScratch.End(), sorter);

			// Include the array size in case we have a DataStore of only empty containers.
			r.AppendPOD((UInt32)vScratch.GetSize());

			for (auto i = vScratch.Begin(); vScratch.End() != i; ++i)
			{
				DataNode child;
				SEOUL_VERIFY(GetValueFromTable(node, *i, child));

				r.AppendData(*i);
				InternalComputeMD5(r, child);
			}
		}
		break;
	case DataNode::kUInt32:
		r.AppendPOD(AssumeUInt32(node));
		break;
	case DataNode::kUInt64:
		r.AppendPOD(AssumeUInt64(node));
		break;
	default:
		break;
	};
}

/**
 * Copy the dependent data of node to new handle vector rvNewHandles and
 * new data vector rvNewData.
 *
 * \warning This method will enter infinite recursion if circular references are present
 * in this DataStore - circular references were not possible when this method
 * was written, but it will need to be updated if such data structures are added.
 */
void DataStore::InternalCopyData(
	const DataNode& node,
	HandleDataOffsets& rvNewHandleDataOffsets,
	Data& rvNewData,
	Bool bCompactContainers) const
{
	switch (node.GetType())
	{
	case DataNode::kNull: // fall-through
	case DataNode::kSpecialErase: // fall-through
	case DataNode::kBoolean: // fall-through
	case DataNode::kInt32Small: // fall-through
	case DataNode::kFloat31: // fall-through
	case DataNode::kFilePath:
		// Nothing more to copy, data is inline.
		break;

		// Arrays, strings, and tables are handled essentially the same,
		// with some differences in size calculation and whether
		// dependencies need to be traversed.
	case DataNode::kArray:
	case DataNode::kString:
	case DataNode::kTable:
		{
			// Cache the handle index
			UInt32 const uIndex = node.GetHandle().GetIndex();

			// Protect against invalid or corrupted handle indices.
			if (uIndex >= m_vHandleDataOffsets.GetSize())
			{
				return;
			}

			// If the new entry is already valid, it means we
			// have multiple references to this data, so we don't need
			// to process it again.
			if (rvNewHandleDataOffsets[uIndex].IsValid())
			{
				return;
			}

			// Get pointers to the handle entries to be updated.
			HandleDataOffset const oldEntry = m_vHandleDataOffsets[uIndex];

			// Protect against invalid or corrupted data entries.
			if (oldEntry.GetDataOffset() >= m_vData.GetSize())
			{
				return;
			}

			// Get the container header.
			Container const container = m_vData[oldEntry.GetDataOffset()].AsContainer();

			// The container size is the size of the header.
			UInt32 zDataSizeInBytes = sizeof(Container);

			// Conditional - if compacting containers, use the smallest size instead
			// of the capacity for the new container.
			if (bCompactContainers && DataNode::kArray == node.GetType())
			{
				zDataSizeInBytes += (container.GetCountExcludingNull() + container.GetHasNull()) * sizeof(DataNode);
			}
			// Special handling for tables - use the next power of 2 of the count + 1u (we always need at least 1u free slot in a table).
			else if (bCompactContainers && DataNode::kTable == node.GetType())
			{
				zDataSizeInBytes += (GetNextPowerOf2(container.GetCountExcludingNull() + 1u) + container.GetHasNull()) * sizeof(DataNode);
			}
			// Otherwise, just use the capacity.
			else
			{
				zDataSizeInBytes += (container.GetCapacityExcludingNull() + container.GetHasNullStorage()) * sizeof(DataNode);
			}

			// If the type is a table, we also need to add the key array,
			// which is the capacity of the table times the size of
			// an HString.
			if (DataNode::kTable == node.GetType())
			{
				// If compacting, the key array will be just big enough to contain the current keys.
				if (bCompactContainers)
				{
					zDataSizeInBytes += (GetNextPowerOf2(container.GetCountExcludingNull() + 1u) + container.GetHasNull()) * sizeof(HString);
				}
				// Otherwise, it is just a straight copy of the source table.
				else
				{
					zDataSizeInBytes += (container.GetCapacityExcludingNull() + container.GetHasNullStorage()) * sizeof(HString);
				}
			}

			// Finally round up the to be a multiple of a DataNode.
			zDataSizeInBytes = (UInt32)RoundUpToAlignment(zDataSizeInBytes, sizeof(DataNode));
			SEOUL_ASSERT(0u == (zDataSizeInBytes % sizeof(DataNode)));

			// Copy the data to the new data buffer.
			UInt32 const uCopyTo = rvNewData.GetSize();
			rvNewData.Resize(rvNewData.GetSize() + (zDataSizeInBytes / sizeof(DataNode)));

			// Cache the table capacity - this does not include the 1u slot of extra storage
			// for a nullptr key, if present. We need this to determine whether we want
			// to compact the table and reinsert keys and values.
			//
			// IMPORTANT: It is necessary to only run the reinsertion algorithm if the capacity
			// will change, since otherwise if we save a DataStore, then load it, then save it
			// again, the compaction may not be idempotent until after the second time the
			// compaction is run, based on the home key order that existed in the original
			// DataStore.
			Bool bCompactTable = false;
			UInt32 uNewTableCapacity = 0u;
			if (bCompactContainers && DataNode::kTable == node.GetType())
			{
				uNewTableCapacity = (GetNextPowerOf2(container.GetCountExcludingNull() + 1u));
				if (uNewTableCapacity != container.GetCapacityExcludingNull())
				{
					bCompactTable = true;
				}
			}

			// Special handling for tables when compacting - use a simplified reinsertion,
			// done in 2 passes:
			// - first pass, only insert key-values that will map to their home positions.
			// - second pass, only insert key-values that will *not* map to their home
			//   position (we need to use probing to find their insertion slot).
			if (bCompactTable)
			{
				// Clear the new buffer area.
				memset(rvNewData.Get(uCopyTo), 0, zDataSizeInBytes);

				// Initialize the new container header - we only have nullptr storage
				// in the new container if the source container has a nullptr value, and
				// the new capacity is reduced compared to the pervious compacity.
				rvNewData[uCopyTo].AsContainer().SetHasNull(container.GetHasNull());
				rvNewData[uCopyTo].AsContainer().SetHasNullStorage(container.GetHasNull());
				rvNewData[uCopyTo].AsContainer().SetCapacityExcludingNull(uNewTableCapacity);
				rvNewData[uCopyTo].AsContainer().SetCountExcludingNull(container.GetCountExcludingNull());

				// Cache pointers to the output memory.
				DataNode* pOutValues = ((DataNode*)rvNewData.Get(uCopyTo + kuContainerSizeInDataEntries));
				HString* pOutKeys = (HString*)(pOutValues + uNewTableCapacity + container.GetHasNull());

				// Cache pointers to the input memory.
				DataNode const* pInValues = ((DataNode*)m_vData.Get(oldEntry.GetDataOffset() + kuContainerSizeInDataEntries));
				HString const* pInKeys = (HString const*)(pInValues + container.GetCapacityExcludingNull() + container.GetHasNullStorage());

				// First pass - iterate over source key-value pairs, and insert those
				// that will directly map to their home position (the output
				// slot does not yet already have a key in it).
				for (UInt32 i = 0u; i < container.GetCapacityExcludingNull(); ++i)
				{
					// Non-null source keys indicate a valid key-value pair.
					if (!pInKeys[i].IsEmpty())
					{
						// Compute the insertion index.
						UInt32 const uIndex = (pInKeys[i].GetHash() & (uNewTableCapacity - 1u));

						// In this pass, don't insert key-values into the new container
						// if they can't be inserted into their home position. This is
						// indicated by a non-empty key already in the output buffer
						// at the home position.
						if (pOutKeys[uIndex].IsEmpty())
						{
							pOutKeys[uIndex] = pInKeys[i];
							pOutValues[uIndex] = pInValues[i];
						}
					}
				}

				// Second pass - iterate over source key-value pairs and insert any remaining
				// pairs. These are all pairs which do *not* map to their home position (a key
				// is already defined for the output slot that is not equal to the key
				// we're trying to insert).
				for (UInt32 i = 0u; i < container.GetCapacityExcludingNull(); ++i)
				{
					// Non-null source keys indicate a valid key-value pair.
					if (!pInKeys[i].IsEmpty())
					{
						// Compute the initial insertion index.
						UInt32 uIndex = (pInKeys[i].GetHash() & (uNewTableCapacity - 1u));

						// In this pass, only insert key-values into the new container
						// if they can't be inserted into their home position, using
						// linear probing with wrap.
						if (!pOutKeys[uIndex].IsEmpty() && pOutKeys[uIndex] != pInKeys[i])
						{
							// Probe until we find an empty insertion index.
							while (!pOutKeys[uIndex].IsEmpty())
							{
								uIndex++;
								uIndex &= (uNewTableCapacity - 1u);
							}

							// Insert the kye-value pair.
							pOutKeys[uIndex] = pInKeys[i];
							pOutValues[uIndex] = pInValues[i];
						}
					}
				}

				// Copy through the null value if present.
				if (container.GetHasNull())
				{
					// Null key is in a hidden slot at the end of the normal buffer for key-value
					// pairs.
					pOutValues[uNewTableCapacity] = pInValues[container.GetCapacityExcludingNull()];
				}
			}
			else
			{
				memcpy(rvNewData.Get(uCopyTo), m_vData.Get(oldEntry.GetDataOffset()), zDataSizeInBytes);
			}

			// If compacting, adjust the new container's capacity to match the count.
			if (bCompactContainers && DataNode::kArray == node.GetType())
			{
				rvNewData[uCopyTo].AsContainer().SetCapacityExcludingNull(rvNewData[uCopyTo].AsContainer().GetCountExcludingNull());
			}
			else if (bCompactContainers && DataNode::kTable == node.GetType())
			{
				rvNewData[uCopyTo].AsContainer().SetCapacityExcludingNull(GetNextPowerOf2(container.GetCountExcludingNull() + 1u));
			}

			// Now update the new handle entry and advance the copy to index.
			rvNewHandleDataOffsets[uIndex].SetDataOffset(uCopyTo);
			rvNewHandleDataOffsets[uIndex].SetGenerationId(oldEntry.GetGenerationId());

			// Tables and arrays must have their contained values processed. Only
			// valid with a capacity > 0u
			if (0u == (container.GetCapacityExcludingNull() + container.GetHasNullStorage()))
			{
				return;
			}

			DataNode const* pValues = ((DataNode*)m_vData.Get(oldEntry.GetDataOffset() + kuContainerSizeInDataEntries));

			// For tables, we need to process the entire data structure,
			// otherwise we only need to process the first m_nCount entries.
			if (node.GetType() == DataNode::kTable)
			{
				HString const* pKeys = (HString const*)(pValues + container.GetCapacityExcludingNull() + container.GetHasNullStorage());

				for (UInt32 i = 0u; i < container.GetCapacityExcludingNull(); ++i)
				{
					if (!pKeys[i].IsEmpty())
					{
						InternalCopyData(pValues[i], rvNewHandleDataOffsets, rvNewData, bCompactContainers);
					}
				}

				// Copy the nullptr key-value if defined.
				if (container.GetHasNull())
				{
					InternalCopyData(pValues[container.GetCapacityExcludingNull()], rvNewHandleDataOffsets, rvNewData, bCompactContainers);
				}
			}
			else if (node.GetType() == DataNode::kArray)
			{
				for (UInt32 i = 0u; i < container.GetCountExcludingNull(); ++i)
				{
					InternalCopyData(pValues[i], rvNewHandleDataOffsets, rvNewData, bCompactContainers);
				}
			}
		}
		break;

	case DataNode::kFloat32: // fall-through
	case DataNode::kInt32Big: // fall-through
	case DataNode::kUInt32:
		{
			// Cache the handle index.
			UInt32 const uIndex = node.GetHandle().GetIndex();

			// Protect against invalid or corrupted handle indices.
			if (uIndex >= m_vHandleDataOffsets.GetSize())
			{
				return;
			}

			// If the new entry is already valid, it means we
			// have multiple references to this data, so we don't need
			// to process it again.
			if (rvNewHandleDataOffsets[uIndex].IsValid())
			{
				return;
			}

			// Int32Big and UInt32Big must have thier by-reference value copied.
			HandleDataOffset const oldEntry = m_vHandleDataOffsets[uIndex];

			// Protect against invalid or corrupted data entries.
			if (oldEntry.GetDataOffset() >= m_vData.GetSize())
			{
				return;
			}

			// Copy the data to the new slot.
			UInt32 const uCopyTo = rvNewData.GetSize();

			// Need one slot for the new Int32/UInt32 data.
			rvNewData.PushBack(DataEntry::Default());

			// Set the data.
			switch (node.GetType())
			{
			case DataNode::kFloat32:
				rvNewData[uCopyTo].AsFloat32Value() = m_vData[oldEntry.GetDataOffset()].AsFloat32Value();
				break;
			case DataNode::kInt32Big:
				rvNewData[uCopyTo].AsInt32BigValue() = m_vData[oldEntry.GetDataOffset()].AsInt32BigValue();
				break;
			case DataNode::kUInt32:
				rvNewData[uCopyTo].AsUInt32Value() = m_vData[oldEntry.GetDataOffset()].AsUInt32Value();
				break;
			default:
				SEOUL_FAIL("Out-of-sync enum.");
				break;
			};

			// Update the new handle entry and advance the copy to index.
			rvNewHandleDataOffsets[uIndex].SetDataOffset(uCopyTo);
			rvNewHandleDataOffsets[uIndex].SetGenerationId(oldEntry.GetGenerationId());
		}
		break;

	case DataNode::kInt64: // fall-through
	case DataNode::kUInt64:
		{
			// Cache the handle index.
			UInt32 const uIndex = node.GetHandle().GetIndex();

			// Protect against invalid or corrupted handle indices.
			if (uIndex >= m_vHandleDataOffsets.GetSize())
			{
				return;
			}

			// If the new entry is already valid, it means we
			// have multiple references to this data, so we don't need
			// to process it again.
			if (rvNewHandleDataOffsets[uIndex].IsValid())
			{
				return;
			}

			// Int64 and UInt64 must have thier by-reference value copied.
			HandleDataOffset const oldEntry = m_vHandleDataOffsets[uIndex];

			// Protect against invalid or corrupted data entries.
			if (oldEntry.GetDataOffset() >= m_vData.GetSize())
			{
				return;
			}

			// Copy the data to the new slot.
			UInt32 const uCopyTo = rvNewData.GetSize();

			// Need two slots for the new Int64/UInt64 data.
			rvNewData.PushBack(DataEntry::Default());
			rvNewData.PushBack(DataEntry::Default());

			// Set the data.
			if (node.GetType() == DataNode::kInt64)
			{
				rvNewData[uCopyTo].SetInt64Value(m_vData[oldEntry.GetDataOffset()].AsInt64Value());
			}
			else
			{
				rvNewData[uCopyTo].SetUInt64Value(m_vData[oldEntry.GetDataOffset()].AsUInt64Value());
			}

			// Update the new handle entry and advance the copy to index.
			rvNewHandleDataOffsets[uIndex].SetDataOffset(uCopyTo);
			rvNewHandleDataOffsets[uIndex].SetGenerationId(oldEntry.GetGenerationId());
		}
		break;

	default:
		SEOUL_FAIL("Unknown enum value.");
		break;
	};
}

/**
 * Compacts the vector of handle offsets to take up as little space as possible.
 *
 * \warning This method cannot be called once the DataStore is in use as it invalidates
 * all handles.
 *
 * \warning This method will enter infinite recursion if circular references are present
 * in this DataStore - circular references were not possible when this method
 * was written, but it will need to be updated if such data structures are added.
 */
void DataStore::InternalCompactHandleOffsets()
{
#if SEOUL_UNIT_TESTS
	DataStore preCompactCopy;
	if (g_bRunningUnitTests)
	{
		preCompactCopy.CopyFrom(*this);
	}
#endif //SEOUL_UNIT_TESTS

	// Do nothing if the datastore is empty.
	if (m_vData.IsEmpty())
	{
		return;
	}

	// This is a mapping of old handle offsets in the existing vector to the new offsets
	// we will be creating.
	HashTable<UInt32, UInt32, MemoryBudgets::Saving> tHandleOffsetsMap;
	// The new offset vector that we are building.
	HandleDataOffsets vNewHandleDataOffsets;

	// Loop over the existing offset vector and build the new one.
	UInt32 offsetSize = m_vHandleDataOffsets.GetSize();
	for (UInt32 index = 0; index < offsetSize; ++index)
	{
		auto const handleOffset = m_vHandleDataOffsets[index];

		// When we find an in-use handle offset, remap it.
		if (handleOffset.IsValid())
		{
			HandleDataOffset newHandleOffset = HandleDataOffset::Default();

			// Give the new handle offset the data offset of the one it's being remapped from.
			newHandleOffset.SetDataOffset(handleOffset.GetDataOffset());
			// Give the new handle offset the default generation id that handles use.
			newHandleOffset.SetGenerationId(DataNode::Handle::kGenerationMask);

			// Add the new handle offset to new vector and update the map.
			vNewHandleDataOffsets.PushBack(newHandleOffset);
			SEOUL_VERIFY(tHandleOffsetsMap.Insert(index, vNewHandleDataOffsets.GetSize() - 1).Second);
		}
	}

	// Starting with the root node, use the remapping we just created to recreate all the
	// handles in the DataStore.
	InternalCompactHandleOffsetsInner(m_vData.Front().AsDataNode(), tHandleOffsetsMap);

	// Truncate to minimize the size of the offset vector.
	vNewHandleDataOffsets.ShrinkToFit();

	// Swap to using the new handle offset vector. Note that between InternalCompactHandleOffsetsInner
	// being called and this call, the DataStore is totally inoperative because all its handles are
	// referring to values in the new offset vector.
	m_vHandleDataOffsets.Swap(vNewHandleDataOffsets);

	// The next handle is the size of our new vector, because it is tightly packed.
	m_uNextHandle = m_vHandleDataOffsets.GetSize();

#if SEOUL_UNIT_TESTS
	if (g_bRunningUnitTests)
	{
		SEOUL_ASSERT(DataStore::Equals(*this, GetRootNode(), preCompactCopy, preCompactCopy.GetRootNode()));
	}
#endif // SEOUL_UNIT_TESTS
}

/**
 * Recursively go through the DataStore and update the handles accoring to the passed
 * in map - rvHandleOffsetsMap.
 */
void DataStore::InternalCompactHandleOffsetsInner(DataNode& node, HashTable<UInt32, UInt32, MemoryBudgets::Saving>& rvHandleOffsetsMap)
{
	switch (node.GetType())
	{
	default:
		// Nothing to update, data is inline.
		break;
	case DataNode::kArray:
	case DataNode::kTable:
	{
		// Cache the handle index
		UInt32 const uIndex = node.GetHandle().GetIndex();

		HandleDataOffset const oldEntry = m_vHandleDataOffsets[uIndex];

		// Get the container header.
		Container const container = m_vData[oldEntry.GetDataOffset()].AsContainer();

		// Only inner processing if the container has some values. Necessary - empty
		// containers at the end of the DataStore data segment will generate an assertion
		// on m_vData.Get().
		if ((container.GetCapacityExcludingNull() + container.GetHasNullStorage()) > 0u)
		{
			DataNode* pValues = ((DataNode*)m_vData.Get(oldEntry.GetDataOffset() + kuContainerSizeInDataEntries));

			// For tables, we need to process the entire data structure,
			// otherwise we only need to process the first m_nCount entries.
			if (node.GetType() == DataNode::kTable)
			{
				HString const* pKeys = (HString const*)(pValues + container.GetCapacityExcludingNull() + container.GetHasNullStorage());

				for (UInt32 i = 0u; i < container.GetCapacityExcludingNull(); ++i)
				{
					if (!pKeys[i].IsEmpty())
					{
						InternalCompactHandleOffsetsInner(pValues[i], rvHandleOffsetsMap);
					}
				}

				// Update the nullptr key-value if defined.
				if (container.GetHasNull())
				{
					InternalCompactHandleOffsetsInner(pValues[container.GetCapacityExcludingNull()], rvHandleOffsetsMap);
				}
			}
			else if (node.GetType() == DataNode::kArray)
			{
				for (UInt32 i = 0u; i < container.GetCountExcludingNull(); ++i)
				{
					InternalCompactHandleOffsetsInner(pValues[i], rvHandleOffsetsMap);
				}
			}
		}
	}
	break;
	};

	// If the node is a reference type, update it's own handle. This is intentionally
	// done after recursing to make sure we don't invalidate handles we still need to
	// navigate by.
	if (DataNode::IsByReference(node.GetType()))
	{
		UInt32 const uIndex = node.GetHandle().GetIndex();

		DataNode::Handle newHandle = DataNode::Handle::Default();
		UInt32 uNewIndex = 0;
		SEOUL_VERIFY(rvHandleOffsetsMap.GetValue(uIndex, uNewIndex));
		newHandle.SetIndex(uNewIndex);
		node.SetHandle(newHandle, node.GetType());
	}
}

/**
 * Utility function, attempts to parse a String into a FilePath. Useful when
 * the source that populated a DataStore is JSON data and the FilePath special type
 * info was lost.
 */
Bool DataStore::InternalFilePathFromString(const DataNode& value, FilePath& rFilePath) const
{
	// Handle String as a valid type on the DataStore side, to support JSON populated DataStores.
	Byte const* sString = nullptr;
	UInt32 zStringLengthInBytes = 0u;
	InternalGetStringData(value.GetHandle(), sString, zStringLengthInBytes);

	return DataStoreParser::StringAsFilePath(sString, zStringLengthInBytes, rFilePath);
}

/**
 * Utility function, attempts to set an arbitrary DataNode value value to
 * the array array at element uIndex.
 *
 * @return True if the operation was successful, false otherwise.
 */
Bool DataStore::InternalSetArrayValue(const DataNode& array, UInt32 uIndex, const DataNode& value)
{
	// Early out if not an array.
	if (!array.IsArray())
	{
		return false;
	}

	// Must suppress garbage collection here, as it could invalidate value.
	ScopedSuppress scope(*this);

	// Check for a valid DataStore handle.
	DataNode::Handle const handle = array.GetHandle();
	if (!InternalIsValidHandle(handle))
	{
		return false;
	}

	// Grow the array if the index is beyond the current array bounds.
	Container* pContainer = (Container*)InternalGetDataEntryPtr(handle);
	if (pContainer->GetCountExcludingNull() <= uIndex)
	{
		if (!ResizeArray(array, uIndex + 1u))
		{
			return false;
		}
		pContainer = (Container*)InternalGetDataEntryPtr(handle);
	}

	// The head of the array is immediately after the Container header.
	DataNode* pData = (DataNode*)(pContainer + 1u);

	// Update the value.
	pData[uIndex] = value;

	return true;
}

/**
 * Utility function, attempts to set an arbitrary DataNode value value to
 * the table table at key key.
 *
 * @return True if the operation was successful, false otherwise.
 */
Bool DataStore::InternalSetTableValue(const DataNode& table, HString key, const DataNode& value)
{
	// Early out if not a table.
	if (!table.IsTable())
	{
		return false;
	}

	// Must suppress garbage collection here, as it could invalidate value.
	ScopedSuppress scope(*this);

	// Check for a valid DataStore handle.
	DataNode::Handle handle = table.GetHandle();
	if (!InternalIsValidHandle(handle))
	{
		return false;
	}

	// Special nullptr key handling.
	HString const kNullKey;
	if (key == kNullKey)
	{
		// If we don't have storage, make room.
		Container* pContainer = (Container*)InternalGetDataEntryPtr(handle);
		if (!pContainer->GetHasNullStorage())
		{
			// Cache values, since we're about to invalid pContainer with a reallocate.
			UInt32 const nOldCapacity = pContainer->GetCapacityExcludingNull();
			UInt32 const nOldDataSizeInDataNodes = GetTableDataSize(nOldCapacity + pContainer->GetHasNullStorage());

			// Increase by 2 DataNodes - enough space for an additional key and value.
			InternalReallocate(nOldDataSizeInDataNodes, nOldDataSizeInDataNodes + 2, handle);

			// Reacquire a valid Container pointer.
			pContainer = (Container*)InternalGetDataEntryPtr(handle);

			// Shift the keys forward in memory, since we need to make room for 1u more value,
			// and keys are located immediately after values.
			memmove(
				(DataEntry*)pContainer + kuContainerSizeInDataEntries + nOldCapacity + 1u,
				(DataEntry*)pContainer + kuContainerSizeInDataEntries + nOldCapacity,
				nOldCapacity * sizeof(HString));

			// Done, we now have storage for the nullptr key and values.
			pContainer->SetHasNullStorage(true);
		}

		// Values are immediately after the container header.
		DataNode* pValues = (DataNode*)(pContainer + 1u);

		// Keys are immediately after the values array.
		HString* pKeys = (HString*)(pValues + pContainer->GetCapacityExcludingNull() + pContainer->GetHasNullStorage());

		// Set key and value and set GetHasNull() to true.
		pKeys[pContainer->GetCapacityExcludingNull()] = key;
		pValues[pContainer->GetCapacityExcludingNull()] = value;
		pContainer->SetHasNull(true);
		return true;
	}

	Container* pContainer = (Container*)InternalGetDataEntryPtr(handle);

	// If increasing the number of elements by 1u will place us at or above
	// the load factor for an HString, grow the array capacity to the next power of 2
	// beyond the current capacity.
	if ((UInt32)(pContainer->GetCountExcludingNull() + 1u) >= (UInt32)(pContainer->GetCapacityExcludingNull() * DefaultHashTableKeyTraits<HString>::GetLoadFactor()))
	{
		// Always grow so there will be at least one null entry in the table - all the
		// querying logic depends on this, in exchange, the logic requires fewer branches.
		InternalGrowTable(table, handle.GetIndex(), GetNextPowerOf2(pContainer->GetCapacityExcludingNull() + 2u));
		pContainer = (Container*)InternalGetDataEntryPtr(handle);
	}

	// Sanity checks
	SEOUL_ASSERT(pContainer->GetCountExcludingNull() <= pContainer->GetCapacityExcludingNull());
	SEOUL_ASSERT(IsPowerOfTwo(pContainer->GetCapacityExcludingNull()));
	SEOUL_ASSERT(((Byte const*)pContainer) + sizeof(Container) + ((pContainer->GetCapacityExcludingNull() + pContainer->GetHasNullStorage()) * sizeof(DataEntry)) + ((pContainer->GetCapacityExcludingNull() + pContainer->GetHasNullStorage()) * sizeof(HString)) <= (Byte const*)(m_vData.Get(0u) + m_vData.GetSize()));

	UInt32 const uHash = key.GetHash();

	// Mask the value so it is in range of the table.
	UInt32 uIndex = uHash;
	uIndex &= (pContainer->GetCapacityExcludingNull() - 1u);
	SEOUL_ASSERT(uIndex < pContainer->GetCapacityExcludingNull());

	// Values are immediately after the container header.
	DataNode* pValues = (DataNode*)(pContainer + 1u);

	// Keys are immediately after the values array.
	HString* pKeys = (HString*)(pValues + pContainer->GetCapacityExcludingNull() + pContainer->GetHasNullStorage());

	// Step one is to apply "anti-clustering" - if the "home" index (the initial hashed
	// index) of the key being inserted is already occupied by an entry with a home index
	// other than the index it occupies, we replace the existing entry with the new entry,
	// and then reinsert the existing entry. This maximizes the number of keys at their
	// home.
	HString const entryKey(pKeys[uIndex]);
	if (kNullKey != entryKey)
	{
		UInt32 const uEntryHash = entryKey.GetHash();
		UInt32 const uEntryHomeIndex = (uEntryHash & (pContainer->GetCapacityExcludingNull() - 1u));

		if (uEntryHomeIndex != uIndex)
		{
			DataNode const replacedValue(pValues[uIndex]);

			pKeys[uIndex] = key;
			pValues[uIndex] = value;

			(void)InternalSetTableValueInner(pContainer, entryKey, replacedValue, uEntryHash, uEntryHomeIndex);
			return true;
		}
		// The home index is occupied by an entry that is already in its home, so we need
		// to continue trying to insert the new key-value pair.
		else
		{
			(void)InternalSetTableValueInner(pContainer, key, value, uHash, uIndex);
			return true;
		}
	}
	// The entry is unoccupied, so we can just insert the new key-value pair.
	else
	{
		pKeys[uIndex] = key;
		pValues[uIndex] = value;
		pContainer->SetCountExcludingNull(pContainer->GetCountExcludingNull() + 1);

		return true;
	}
}

/**
 * Attempt to erase key from the table contained in pContainer.
 *
 * @return True if the element was removed from the table, false otherwise.
 */
Bool DataStore::InternalEraseTableValue(Container* pContainer, HString key)
{
	HString const kNullKey;

	// Special case - nothing to do if no entries in the table.
	if (0 == (pContainer->GetCountExcludingNull() + pContainer->GetHasNull()))
	{
		return false;
	}

	// nullptr key special handling.
	if (key == kNullKey)
	{
		if (pContainer->GetHasNull())
		{
			// Values are immediately after the container header.
			DataNode* pValues = (DataNode*)(pContainer + 1u);

			// Keys are immediately after the values array.
			HString* pKeys = (HString*)(pValues + pContainer->GetCapacityExcludingNull() + pContainer->GetHasNullStorage());

			// DataNode() to default, key to the nullptr key.
			pValues[pContainer->GetCapacityExcludingNull()] = DataNode();
			pKeys[pContainer->GetCapacityExcludingNull()] = kNullKey;

			// No longer have a nullptr key-value pair.
			pContainer->SetHasNull(false);
			return true;
		}

		return false;
	}

	UInt32 const uHash = GetHash(key);
	UInt32 uIndex = uHash;

	// Values are immediately after the container header.
	DataNode* pValues = (DataNode*)(pContainer + 1u);

	// Keys are immediately after the values array.
	HString* pKeys = (HString*)(pValues + pContainer->GetCapacityExcludingNull() + pContainer->GetHasNullStorage());

	while (true)
	{
		uIndex &= (pContainer->GetCapacityExcludingNull() - 1u);

		HString const entryKey(pKeys[uIndex]);

		// If entryKey is the entry we want to remove.
		if (key == entryKey)
		{
			// Delete the entry.
			pValues[uIndex] = DataNode();
			pKeys[uIndex] = kNullKey;
			pContainer->SetCountExcludingNull(pContainer->GetCountExcludingNull() - 1);

			// Now we need to compact the array, by filling in holes - walk the
			// array starting at the next element, attempting to reinsert every element,
			// until we hit an existing null element (an existing hol).
			++uIndex;
			while (true)
			{
				uIndex &= (pContainer->GetCapacityExcludingNull() - 1u);

				HString const innerEntryKey(pKeys[uIndex]);

				// If the key is null, we're done - return true for a successful erase.
				if (kNullKey == innerEntryKey)
				{
					return true;
				}
				// Otherwise, if the entry key is not in its home position, try to reinsert it.
				else
				{
					UInt32 const uInnerEntryHash(GetHash(innerEntryKey));
					UInt32 const uInnerEntryHomeIndex((uInnerEntryHash & (pContainer->GetCapacityExcludingNull() - 1u)));

					if (uInnerEntryHomeIndex != uIndex)
					{
						// First decrement the count - either we'll restore it (if the insertion
						// fails and the entry stays in the same slot), or it'll be incremented
						// on the successful reinsertion of the entry.
						pContainer->SetCountExcludingNull(pContainer->GetCountExcludingNull() - 1);

						// If the key was inserted (which can only happen if it didn't end up in the same
						// slot, destroy the old entry. Insert() will also have incremented the count, so
						// we don't need to do it again.
						DataNode const innerEntryValue(pValues[uIndex]);
						if (InternalSetTableValueInner(pContainer, innerEntryKey, innerEntryValue, uInnerEntryHash, uInnerEntryHomeIndex))
						{
							pValues[uIndex] = DataNode();
							pKeys[uIndex] = kNullKey;
						}
						// Otherwise, restore the count, since it means the entry just hashed to the same
						// location (eventually, the Insert() hit the existing element).
						else
						{
							pContainer->SetCountExcludingNull(pContainer->GetCountExcludingNull() + 1);
						}
					}
				}

				++uIndex;
			}
		}
		// If we hit a nullptr key, the entry does not exist, so nothing to erase.
		else if (entryKey == kNullKey)
		{
			return false;
		}

		++uIndex;
	}
}

/**
 * Utility function used internally by InternalSetTableValue.
 *
 * @return True if the value was inserted into an empty slot, false
 * if the value replaced an existing entry with the same key.
 */
Bool DataStore::InternalSetTableValueInner(
	Container* pContainer,
	HString key,
	const DataNode& value,
	UInt32 const uHash,
	UInt32 uIndex)
{
	HString const kNullKey;

	// Values are immediately after the container header.
	DataNode* pValues = (DataNode*)(pContainer + 1u);

	// Keys are immediately after the values array.
	HString* pKeys = (HString*)(pValues + pContainer->GetCapacityExcludingNull() + pContainer->GetHasNullStorage());

	while (true)
	{
		// If an entry exists that has the same key as the entry being inserted.
		HString const entryKey(pKeys[uIndex]);
		if (key == entryKey)
		{
			pValues[uIndex] = value;
			return false;
		}
		// Otherwise, if we've hit a nullptr key, we're done. Insert the key and return success.
		else if (entryKey == kNullKey)
		{
			pKeys[uIndex] = key;
			pValues[uIndex] = value;
			pContainer->SetCountExcludingNull(pContainer->GetCountExcludingNull() + 1);

			return true;
		}

		++uIndex;
		uIndex &= (pContainer->GetCapacityExcludingNull() - 1u);
		SEOUL_ASSERT(uIndex < pContainer->GetCapacityExcludingNull());
	}
}

/**
 * Utility used by ToString(). Writes an escaped
 * string to rsOutput from s of size zStringLengthInBytes.
 *
 * The format of the output is formed for consumption
 * by the DataStore parser. The string will be quoted and escaped.
 */
void DataStore::InternalSerializeAsString(
	String& rsOutput,
	Byte const* s,
	UInt32 zStringLengthInBytes) const
{
	// Compute the escaped string size.
	UInt32 const zEscapedStringLengthInBytes = JSONEscapedLength(s, zStringLengthInBytes);

	// If the input and escape strings have the same size, don't escape, just use
	// the string directly.
	if (zEscapedStringLengthInBytes == zStringLengthInBytes)
	{
		rsOutput.Reserve(rsOutput.GetSize() + zStringLengthInBytes + 2u);
		rsOutput.Append('"');
		rsOutput.Append(s, zStringLengthInBytes);
		rsOutput.Append('"');
	}
	// Otherwise, compute the escaped string.
	else
	{
		// TODO: Heap allocation here is unfortunate.
		Vector<Byte> v;
		v.Resize(zEscapedStringLengthInBytes + 1u);
		JSONEscape(s, v.Get(0u), v.GetSizeInBytes());

		rsOutput.Reserve(rsOutput.GetSize() + zEscapedStringLengthInBytes + 2u);
		rsOutput.Append('"');
		rsOutput.Append(v.Get(0u), zEscapedStringLengthInBytes);
		rsOutput.Append('"');
	}
}

/**
 * @return The data in value formatted and output to a String.
 *
 * @param[in] bMultiline If true, newlines will be inserted when formatting
 * tables. Otherwise, all output will be on a single line.
 *
 * @param[in] iIndentation Current indentation, in tabs - no meaning unless bMultiline is true.
 *
 * @param[in] bSortTableKeysAlphabetical If true, the keys of a table will be sorted in alphabetical
 * order prior to outputing to a string. Otherwise, the table will be written in the order that
 * the keys exist in the table.
 *
 * This function is expensive - it is useful for printing data values,
 * or serializing a DataStore to a string format, but likely not the right
 * solution for much else. It is not intended to be part of normal usage patterns
 * of a DataStore.
 */
void DataStore::InternalToString(
	const DataNode& value,
	String& rsOutput,
	Bool bMultiline /*= false*/,
	Int iIndentationLevel /*= 0*/,
	Bool bSortTableKeysAlphabetical /*= false*/) const
{
	switch (value.GetType())
	{
	case DataNode::kNull: // fall-through
	case DataNode::kSpecialErase: // TODO: Not ideal, since we lose the deletion info - no text based version of the diff.
		rsOutput.Append("null");
		break;
	case DataNode::kBoolean:
		rsOutput.Append(AssumeBoolean(value) ? "true" : "false");
		break;
	case DataNode::kInt32Big:
		rsOutput.Append(String::Printf("%d", AssumeInt32Big(value)));
		break;
	case DataNode::kInt32Small:
		rsOutput.Append(String::Printf("%d", AssumeInt32Small(value)));
		break;
	case DataNode::kUInt32:
		rsOutput.Append(String::Printf("%u", AssumeUInt32(value)));
		break;
	case DataNode::kFloat31:
	case DataNode::kFloat32:
		{
			Float32 const fValue = (DataNode::kFloat32 == value.GetType() ? AssumeFloat32(value) : AssumeFloat31(value));

			// Note: JSON standard does not actually support NaN or Infinity,
			// but JavaScript does allow those tokens.
			//
			// http://json.org
			if (IsNaN(fValue))
			{
				// Non-standard - JSON avoided the hard decision and
				// leaves various implementations to deal with infinity
				// and NaN.
				rsOutput.Append("NaN");
			}
			else if (IsInf(fValue))
			{
				// Non-standard - JSON avoided the hard decision and
				// leaves various implementations to deal with infinity
				// and NaN.
				if (fValue < 0.0f)
				{
					rsOutput.Append("-Infinity");
				}
				else
				{
					rsOutput.Append("Infinity");
				}
			}
			else
			{
				// Normal floating point output:
				rsOutput.Append(String::Printf("%g", fValue));
			}
		}
		break;
	case DataNode::kFilePath:
		{
			FilePath filePath;
			SEOUL_VERIFY(AsFilePath(value, filePath));

			// If FilePath is invalid, write out a null value, since
			// "://" is invalid syntax.
			if (!filePath.IsValid())
			{
				rsOutput.Append("null");
			}
			else
			{
				// JSON has no "file path" type, only strings, so quote file paths.
				rsOutput.Append('"');

				// Append the FilePath as a serialized URL
				rsOutput.Append(filePath.ToSerializedUrl());
				rsOutput.Append('"');
			}
		}
		break;
	case DataNode::kTable:
		{
			// Start the table and increase the indentation level.
			rsOutput.Append('{');
			++iIndentationLevel;

			// Write the keys alphabetically
			if (bSortTableKeysAlphabetical)
			{
				// Get the total table size, then allocate a vector for all the keys.
				UInt32 nTableCount = 0u;
				SEOUL_VERIFY(GetTableCount(value, nTableCount));
				Vector<HString, MemoryBudgets::DataStore> vEntries(nTableCount);

				// Insert keys into the vector.
				{
					UInt32 uIndex = 0u;
					auto const iBegin = TableBegin(value);
					auto const iEnd = TableEnd(value);
					for (auto i = iBegin; iEnd != i; ++i)
					{
						vEntries[uIndex++] = i->First;
					}
				}

				// Sort the vector.
				LexicalHStringSort lexicalHStringSort;
				QuickSort(vEntries.Begin(), vEntries.End(), lexicalHStringSort);

				// Now write the entries
				{
					for (UInt32 i = 0u; i < nTableCount; ++i)
					{
						HString key = vEntries[i];
						DataNode tableValue;
						SEOUL_VERIFY(GetValueFromTable(value, key, tableValue));

						if (i > 0u)
						{
							rsOutput.Append(',');
						}

						// If in multiline mode, insert a newline and indent.
						if (bMultiline)
						{
							rsOutput.Append('\n');
							rsOutput.Append('\t', iIndentationLevel);
						}

						InternalSerializeAsString(rsOutput, key.CStr(), key.GetSizeInBytes());

						rsOutput.Append(':');
						InternalToString(tableValue, rsOutput, bMultiline, iIndentationLevel, bSortTableKeysAlphabetical);
					}
				}
			}
			// Otherwise, just walk the table.
			else
			{
				UInt32 uIndex = 0u;
				auto const iBegin = TableBegin(value);
				auto const iEnd = TableEnd(value);
				for (auto i = iBegin; iEnd != i; ++i)
				{
					if (uIndex > 0u)
					{
						rsOutput.Append(',');
					}

					// If in multiline mode, insert a newline and indent.
					if (bMultiline)
					{
						rsOutput.Append('\n');
						rsOutput.Append('\t', iIndentationLevel);
					}

					InternalSerializeAsString(rsOutput, i->First.CStr(), i->First.GetSizeInBytes());

					rsOutput.Append(':');
					InternalToString(i->Second, rsOutput, bMultiline, iIndentationLevel, bSortTableKeysAlphabetical);

					++uIndex;
				}
			}

			// Decrease the indentation level before terminating.
			--iIndentationLevel;

			// If in multiline formatting mode, conditionally insert a newline before inserting the table terminator.
			if (bMultiline)
			{
				// Don't insert a newline before the terminator if the table is empty.
				if (TableBegin(value) != TableEnd(value))
				{
					rsOutput.Append('\n');
					rsOutput.Append('\t', iIndentationLevel);
				}
			}

			rsOutput.Append('}');
		}
		break;
	case DataNode::kArray:
		{
			UInt32 uArrayCount = 0u;
			SEOUL_VERIFY(GetArrayCount(value, uArrayCount));
			rsOutput.Append('[');
			for (UInt32 i = 0u; i < uArrayCount; ++i)
			{
				if (i > 0u)
				{
					if (bMultiline)
					{
						rsOutput.Append(", ");
					}
					else
					{
						rsOutput.Append(",");
					}
				}

				DataNode arrayValue;
				SEOUL_VERIFY(GetValueFromArray(value, i, arrayValue));
				InternalToString(arrayValue, rsOutput, bMultiline, iIndentationLevel, bSortTableKeysAlphabetical);
			}
			rsOutput.Append(']');
		}
		break;
	case DataNode::kString:
		{
			Byte const* s = nullptr;
			UInt32 zStringLengthInBytes = 0u;
			SEOUL_VERIFY(AsString(value, s, zStringLengthInBytes));
			InternalSerializeAsString(rsOutput,s, zStringLengthInBytes);
		}
		break;
	case DataNode::kInt64:
		rsOutput.Append(String::Printf("%" PRId64, AssumeInt64(value)));
		break;
	case DataNode::kUInt64:
		rsOutput.Append(String::Printf("%" PRIu64, AssumeUInt64(value)));
		break;
	default:
		SEOUL_FAIL("Unknown enum");
		break;
	};
}

/** Recursive function used by VerifyIntegrity(). */
Bool DataStore::InternalVerifyIntegrity(const DataNode& node) const
{
	auto const eType = node.GetType();
	switch (eType)
	{
		// Simple types, simple values.
	case DataNode::kNull:
	case DataNode::kSpecialErase:
		return true;

		// Simple values - TODO: Maybe verify the value portion of the DataNode, but that would require checking the API in some cases as well (bool?).
	case DataNode::kBoolean:
	case DataNode::kInt32Small:
	case DataNode::kFilePath: // TODO: Verify FilePath members in particular?
	case DataNode::kFloat31:
		return true;

		// Reference types.
	case DataNode::kArray:
	case DataNode::kFloat32:
	case DataNode::kInt32Big:
	case DataNode::kInt64:
	case DataNode::kString:
	case DataNode::kTable:
	case DataNode::kUInt32:
	case DataNode::kUInt64:
		{
			// Get the size of the target type in bytes.
			UInt32 uDataSizeInDataNodes = 0u;
			switch (eType)
			{
			case DataNode::kArray:
			case DataNode::kString:
			case DataNode::kTable:
				uDataSizeInDataNodes = (sizeof(Container) / sizeof(DataNode));
				break;

			case DataNode::kInt64:
			case DataNode::kUInt64:
				uDataSizeInDataNodes = (sizeof(UInt64) / sizeof(DataNode));
				break;

			case DataNode::kFloat32:
			case DataNode::kInt32Big:
			case DataNode::kUInt32:
				uDataSizeInDataNodes = (sizeof(UInt32) / sizeof(DataNode));
				break;

			default:
				SEOUL_FAIL("Logic error.");
				break;
			};

			// Get the handle index.
			UInt32 const uIndex = node.GetHandle().GetIndex();

			// Verify range.
			if (uIndex >= m_vHandleDataOffsets.GetSize())
			{
				return false;
			}

			// Get pointers to the handle entries to be updated.
			auto const entry = m_vHandleDataOffsets[uIndex];

			// Verify range.
			if (entry.GetDataOffset() + uDataSizeInDataNodes > m_vData.GetSize())
			{
				return false;
			}

			// For container types, verify container data and enumerate.
			if (eType == DataNode::kArray || eType == DataNode::kTable || eType == DataNode::kString)
			{
				auto const container = m_vData[entry.GetDataOffset()].AsContainer();

				// Verify count - for array and table, it is in DataNodes. For strings, it
				// is the size of the string data in bytes, minus the null terminator (which is
				// always present).
				if (eType == DataNode::kArray || eType == DataNode::kTable)
				{
					if (container.GetCountExcludingNull() > container.GetCapacityExcludingNull())
					{
						return false;
					}
				}
				else
				{
					if (container.GetCountExcludingNull() + 1u > (container.GetCapacityExcludingNull() * sizeof(DataNode)))
					{
						return false;
					}
				}

				uDataSizeInDataNodes += (container.GetCapacityExcludingNull() + container.GetHasNullStorage());

				// Verify range.
				if (entry.GetDataOffset() + uDataSizeInDataNodes > m_vData.GetSize())
				{
					return false;
				}
			}

			// Now, enumerate container types.
			if (eType == DataNode::kArray)
			{
				UInt32 uCount = 0u;
				SEOUL_VERIFY(GetArrayCount(node, uCount));
				for (UInt32 i = 0u; i < uCount; ++i)
				{
					DataNode child;
					SEOUL_VERIFY(GetValueFromArray(node, i, child));
					if (!InternalVerifyIntegrity(child))
					{
						return false;
					}
				}
			}
			else if (eType == DataNode::kTable)
			{
				auto const iBegin = TableBegin(node);
				auto const iEnd = TableEnd(node);
				for (auto i = iBegin; iEnd != i; ++i)
				{
					// TODO: Verify HString key integrity?

					if (!InternalVerifyIntegrity(i->Second))
					{
						return false;
					}
				}
			}
		}
		return true;

		// Invalid type, invalid DataStore.
	default:
		return false;
	};
}

static Bool CopyValueToTable(
	const DataStore& from,
	const DataNode& fromValue,
	DataStore& rTo,
	const DataNode& toTable,
	HString key)
{
	switch (fromValue.GetType())
	{
	case DataNode::kNull: return rTo.SetNullValueToTable(toTable, key);
	case DataNode::kSpecialErase: return rTo.SetSpecialEraseToTable(toTable, key);
	case DataNode::kFloat31: return rTo.SetFloat32ValueToTable(toTable, key, from.AssumeFloat31(fromValue));
	case DataNode::kBoolean: return rTo.SetBooleanValueToTable(toTable, key, from.AssumeBoolean(fromValue));
	case DataNode::kInt32Small: return rTo.SetInt32ValueToTable(toTable, key, from.AssumeInt32Small(fromValue));
	case DataNode::kFilePath:
		{
			FilePath filePath;
			return (from.AsFilePath(fromValue, filePath) && rTo.SetFilePathToTable(toTable, key, filePath));
		}
		break;
	case DataNode::kTable:
		{
			DataNode valueTable;
			if (rTo.SetTableToTable(toTable, key) &&
				rTo.GetValueFromTable(toTable, key, valueTable) &&
				rTo.DeepCopy(from, fromValue, valueTable))
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		break;
	case DataNode::kArray:
		{
			DataNode valueArray;
			if (rTo.SetArrayToTable(toTable, key) &&
				rTo.GetValueFromTable(toTable, key, valueArray) &&
				rTo.DeepCopy(from, fromValue, valueArray))
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		break;
	case DataNode::kString:
		{
			String s;
			return (from.AsString(fromValue, s) && rTo.SetStringToTable(toTable, key, s));
		}
		break;
	case DataNode::kFloat32: return rTo.SetFloat32ValueToTable(toTable, key, from.AssumeFloat32(fromValue));
	case DataNode::kInt32Big: return rTo.SetInt32ValueToTable(toTable, key, from.AssumeInt32Big(fromValue));
	case DataNode::kUInt32: return rTo.SetUInt32ValueToTable(toTable, key, from.AssumeUInt32(fromValue));
	case DataNode::kInt64: return rTo.SetInt64ValueToTable(toTable, key, from.AssumeInt64(fromValue));
	case DataNode::kUInt64: return rTo.SetUInt64ValueToTable(toTable, key, from.AssumeUInt64(fromValue));
	default:
		SEOUL_FAIL("Out-of-sync enum.");
		return false;
	}
}

/**
 * Patch equivalent to ComputeDiff().
 */
static Bool ApplyDiff(
	const DataStore& fromDataStore,
	const DataNode& fromNode,
	DataStore& toDataStore,
	const DataNode& toNode)
{
	// Can't copy if the types of to and from differ.
	if (fromNode.GetType() != toNode.GetType())
	{
		return false;
	}

	// Handle copying an array.
	if (fromNode.IsArray())
	{
		UInt32 uArrayCount = 0u;
		if (!fromDataStore.GetArrayCount(fromNode, uArrayCount)) { return false; }
		if (!toDataStore.ResizeArray(toNode, uArrayCount)) { return false; }

		// Iterate over all members of the source array.
		for (UInt32 i = 0u; i < uArrayCount; ++i)
		{
			DataNode valueTo;
			toDataStore.GetValueFromArray(toNode, i, valueTo);

			// Cache the value in the source array.
			DataNode valueFrom;
			if (!fromDataStore.GetValueFromArray(fromNode, i, valueFrom)) { return false; }

			// Handle all possible source value types.
			switch (valueFrom.GetType())
			{
			case DataNode::kNull: if (!toDataStore.SetNullValueToArray(toNode, i)) { return false; } break;
			case DataNode::kSpecialErase: if (!toDataStore.SetSpecialEraseToArray(toNode, i)) { return false; } break;
			case DataNode::kBoolean: if (!toDataStore.SetBooleanValueToArray(toNode, i, fromDataStore.AssumeBoolean(valueFrom))) { return false; } break;
			case DataNode::kInt32Big: if (!toDataStore.SetInt32ValueToArray(toNode, i, fromDataStore.AssumeInt32Big(valueFrom))) { return false; } break;
			case DataNode::kInt32Small: if (!toDataStore.SetInt32ValueToArray(toNode, i, fromDataStore.AssumeInt32Small(valueFrom))) { return false; } break;
			case DataNode::kUInt32: if (!toDataStore.SetUInt32ValueToArray(toNode, i, fromDataStore.AssumeUInt32(valueFrom))) { return false; } break;
			case DataNode::kFloat31: if (!toDataStore.SetFloat32ValueToArray(toNode, i, fromDataStore.AssumeFloat31(valueFrom))) { return false; } break;
			case DataNode::kFloat32: if (!toDataStore.SetFloat32ValueToArray(toNode, i, fromDataStore.AssumeFloat32(valueFrom))) { return false; } break;
			case DataNode::kInt64: if (!toDataStore.SetInt64ValueToArray(toNode, i, fromDataStore.AssumeInt64(valueFrom))) { return false; } break;
			case DataNode::kUInt64: if (!toDataStore.SetUInt64ValueToArray(toNode, i, fromDataStore.AssumeUInt64(valueFrom))) { return false; } break;

			case DataNode::kFilePath:
				{
					FilePath filePath;
					SEOUL_VERIFY(fromDataStore.AsFilePath(valueFrom, filePath));
					if (!toDataStore.SetFilePathToArray(toNode, i, filePath)) { return false; }
				}
				break;
			case DataNode::kString:
				{
					String s;
					SEOUL_VERIFY(fromDataStore.AsString(valueFrom, s));
					if (!toDataStore.SetStringToArray(toNode, i, s)) { return false; }
				}
				break;
			case DataNode::kArray:
				{
					if (!toDataStore.SetArrayToArray(toNode, i)) { return false; }
					if (!toDataStore.GetValueFromArray(toNode, i, valueTo)) { return false; }
					if (!toDataStore.DeepCopy(fromDataStore, valueFrom, valueTo)) { return false; }
				}
				break;
			case DataNode::kTable:
				{
					if (!toDataStore.SetTableToArray(toNode, i)) { return false; }
					if (!toDataStore.GetValueFromArray(toNode, i, valueTo)) { return false; }
					if (!toDataStore.DeepCopy(fromDataStore, valueFrom, valueTo)) { return false; }
				}
				break;
			default:
				return false;
			};
		}

		return true;
	}
	// Handle copying a table.
	else if (fromNode.IsTable())
	{
		// Iterate over all members of the input table.
		auto const iBegin = fromDataStore.TableBegin(fromNode);
		auto const iEnd = fromDataStore.TableEnd(fromNode);
		for (auto i = iBegin; iEnd != i; ++i)
		{
			DataNode valueTo;
			toDataStore.GetValueFromTable(toNode, i->First, valueTo);

			// Cache the value in the source table.
			DataNode const valueFrom = i->Second;

			// Handle all possible source value types.
			switch (valueFrom.GetType())
			{
			case DataNode::kNull: if (!toDataStore.SetNullValueToTable(toNode, i->First)) { return false; } break;
			case DataNode::kSpecialErase:
				if (!toDataStore.EraseValueFromTable(toNode, i->First))
				{
					// No value is sufficient, don't fail if an erase was specified
					// targeting a value that already does not exist.
					DataNode unusedOut;
					if (toDataStore.GetValueFromTable(toNode, i->First, unusedOut))
					{
						return false;
					}
				}
				break;
			case DataNode::kBoolean: if (!toDataStore.SetBooleanValueToTable(toNode, i->First, fromDataStore.AssumeBoolean(valueFrom))) { return false; } break;
			case DataNode::kInt32Big: if (!toDataStore.SetInt32ValueToTable(toNode, i->First, fromDataStore.AssumeInt32Big(valueFrom))) { return false; } break;
			case DataNode::kInt32Small: if (!toDataStore.SetInt32ValueToTable(toNode, i->First, fromDataStore.AssumeInt32Small(valueFrom))) { return false; } break;
			case DataNode::kUInt32: if (!toDataStore.SetUInt32ValueToTable(toNode, i->First, fromDataStore.AssumeUInt32(valueFrom))) { return false; } break;
			case DataNode::kFloat31: if (!toDataStore.SetFloat32ValueToTable(toNode, i->First, fromDataStore.AssumeFloat31(valueFrom))) { return false; } break;
			case DataNode::kFloat32: if (!toDataStore.SetFloat32ValueToTable(toNode, i->First, fromDataStore.AssumeFloat32(valueFrom))) { return false; } break;
			case DataNode::kInt64: if (!toDataStore.SetInt64ValueToTable(toNode, i->First, fromDataStore.AssumeInt64(valueFrom))) { return false; } break;
			case DataNode::kUInt64: if (!toDataStore.SetUInt64ValueToTable(toNode, i->First, fromDataStore.AssumeUInt64(valueFrom))) { return false; } break;

			case DataNode::kFilePath:
				{
					FilePath filePath;
					SEOUL_VERIFY(fromDataStore.AsFilePath(valueFrom, filePath));
					if (!toDataStore.SetFilePathToTable(toNode, i->First, filePath)) { return false; }
				}
				break;
			case DataNode::kString:
				{
					String s;
					SEOUL_VERIFY(fromDataStore.AsString(valueFrom, s));
					if (!toDataStore.SetStringToTable(toNode, i->First, s)) { return false; }
				}
				break;
			case DataNode::kArray:
				{
					if (!toDataStore.SetArrayToTable(toNode, i->First)) { return false; }
					if (!toDataStore.GetValueFromTable(toNode, i->First, valueTo)) { return false; }
					if (!toDataStore.DeepCopy(fromDataStore, valueFrom, valueTo)) { return false; }
				}
				break;
			case DataNode::kTable:
				{
					if (!valueTo.IsTable())
					{
						if (!toDataStore.SetTableToTable(toNode, i->First)) { return false; }
					}

					// Refresh value to and then DeepCopy the sub tree.
					if (!toDataStore.GetValueFromTable(toNode, i->First, valueTo)) { return false; }
					if (!ApplyDiff(fromDataStore, valueFrom, toDataStore, valueTo)) { return false; }
				}
				break;
			default:
				return false;
			};
		}

		return true;
	}
	// Deep copying any other type is an invalid operation.
	else
	{
		return false;
	}
}

/** Compute the additive portion of a diff. */
static Bool ComputeDiffAdditive(
	const DataStore& a,
	const DataNode& nodeA,
	const DataStore& b,
	const DataNode& nodeB,
	DataStore& rDiff,
	const DataNode& nodeDiff)
{
	auto const iBegin = b.TableBegin(nodeB);
	auto const iEnd = b.TableEnd(nodeB);
	for (auto i = iBegin; iEnd != i; ++i)
	{
		if (!a.TableContainsKey(nodeA, i->First))
		{
			if (!CopyValueToTable(
				b,
				i->Second,
				rDiff,
				nodeDiff,
				i->First))
			{
				return false;
			}
		}
		else
		{
			DataNode existingValue;
			SEOUL_VERIFY(a.GetValueFromTable(nodeA, i->First, existingValue));
			if (!DataStore::Equals(
				a,
				existingValue,
				b,
				i->Second))
			{
				// Perform a merge on tables.
				if (i->Second.IsTable() && existingValue.IsTable())
				{
					DataNode outTable;
					if (!rDiff.SetTableToTable(nodeDiff, i->First) ||
						!rDiff.GetValueFromTable(nodeDiff, i->First, outTable))
					{
						return false;
					}

					if (!ComputeDiffAdditive(
						a,
						existingValue,
						b,
						i->Second,
						rDiff,
						outTable))
					{
						return false;
					}
				}
				else if (!CopyValueToTable(
					b,
					i->Second,
					rDiff,
					nodeDiff,
					i->First))
				{
					return false;
				}
			}
		}
	}

	return true;
}

/** Compute the subtractive (removes) portion of a diff. */
static Bool ComputeDiffSubtractive(
	const DataStore& a,
	const DataNode& nodeA,
	const DataStore& b,
	const DataNode& nodeB,
	DataStore& rDiff,
	const DataNode& nodeDiff)
{
	auto const iBegin = a.TableBegin(nodeA);
	auto const iEnd = a.TableEnd(nodeA);
	for (auto i = iBegin; iEnd != i; ++i)
	{
		// If b does not contain a key in A, set the diff to have a special
		// erase entry as the value.
		if (!b.TableContainsKey(nodeB, i->First))
		{
			rDiff.SetSpecialEraseToTable(nodeDiff, i->First);
		}
		else
		{
			DataNode existingValue;
			SEOUL_VERIFY(b.GetValueFromTable(nodeB, i->First, existingValue));
			if (!DataStore::Equals(
				a,
				i->Second,
				b,
				existingValue))
			{
				// Perform a merge on tables.
				if (i->Second.IsTable() && existingValue.IsTable())
				{
					DataNode outTable;
					if (!rDiff.GetValueFromTable(nodeDiff, i->First, outTable))
					{
						return false;
					}

					if (!ComputeDiffSubtractive(
						a,
						i->Second,
						b,
						existingValue,
						rDiff,
						outTable))
					{
						return false;
					}
				}
			}
		}
	}

	return true;
}

/**
 * Diffing utility - generates a "delta" DataStore such that, when the delta
 * is applied to a DataStore a with ApplyDiff(), the resulting DataStore
 * will be exactly equal to b.
 */
static Bool ComputeDiff(
	const DataStore& a,
	const DataNode& nodeA,
	const DataStore& b,
	const DataNode& nodeB,
	DataStore& rDiff,
	const DataNode& nodeDiff)
{
	if (!ComputeDiffAdditive(a, nodeA, b, nodeB, rDiff, nodeDiff))
	{
		return false;
	}

	if (!ComputeDiffSubtractive(a, nodeA, b, nodeB, rDiff, nodeDiff))
	{
		return false;
	}

	return true;
}

/**
 * Patch equivalent to ComputeDiff().
 */
Bool ApplyDiff(const DataStore& diff, DataStore& rTarget)
{
	// If diff's root not is null, we leave rTarget unchanged.
	if (diff.GetRootNode().IsNull())
	{
		return true;
	}

	// If rTarget's root is null, or if its type differs
	// from the diff, replace the root with a new root
	// that matches the target. This is a full overwrite.
	//
	// We can't just perform a copy here due to kSpecialErase.
	// We want those types to be resolved and not left
	// in the lingering in the output.
	if (rTarget.GetRootNode().IsNull() ||
		rTarget.GetRootNode().GetType() != diff.GetRootNode().GetType())
	{
		if (diff.GetRootNode().GetType() == DataNode::kArray)
		{
			DataStore emptyArray;
			emptyArray.MakeArray();
			rTarget.Swap(emptyArray);
		}
		else
		{
			DataStore emptyTable;
			emptyTable.MakeTable();
			rTarget.Swap(emptyTable);
		}
	}

	// Apply the diff hierarchy.
	return ApplyDiff(diff, diff.GetRootNode(), rTarget, rTarget.GetRootNode());
}

/**
 * Diffing utility - generates a DataStore rDiff that when
 * applied to a DataStore a, generates a DataStore b.
 */
Bool ComputeDiff(const DataStore& a, const DataStore& b, DataStore& rDiff)
{
	// Base case - if a is null, rDiff is just a copy of b.
	if (a.GetRootNode().IsNull())
	{
		rDiff.CopyFrom(b);
		return true;
	}

	// For a null of b, we use a replacement B that has an empty container
	// of whatever type is at the root of a.
	if (b.GetRootNode().IsNull())
	{
		DataStore emptyB;
		if (a.GetRootNode().IsArray())
		{
			emptyB.MakeArray();
		}
		else
		{
			emptyB.MakeTable();
		}

		return ComputeDiff(a, emptyB, rDiff);
	}

	// Special case - if the root node of b is not equal to the root node of
	// a, rDiff just becomes a copy of b.
	if (a.GetRootNode().GetType() != b.GetRootNode().GetType())
	{
		rDiff.CopyFrom(b);
		return true;
	}

	// For root arrays, also just perform a copy.
	if (a.GetRootNode().IsArray())
	{
		rDiff.CopyFrom(b);
		return true;
	}
	else
	{
		// All other cases, apply the diff computation to the appropriate root
		// node of rDiff.
		DataStore diff;
		diff.MakeTable();

		if (ComputeDiff(
			a,
			a.GetRootNode(),
			b,
			b.GetRootNode(),
			diff,
			diff.GetRootNode()))
		{
			rDiff.Swap(diff);
			return true;
		}
		else
		{
			return false;
		}
	}
}

} // namespace Seoul
