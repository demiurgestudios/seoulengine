/**
 * \file DataStore.h
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

#pragma once
#ifndef DATA_STORE_H
#define DATA_STORE_H

#include "HashTable.h"
#include "FilePath.h"
#include "SharedPtr.h"
#include "SeoulHString.h"
#include "Vector.h"
namespace Seoul { class MD5; }

namespace Seoul
{

// Forward declarations
class DataStore;
struct DataStoreTableIterator;
template <typename T> class ScopedPtr;
class SyncFile;

/** Base marker size for automatically triggered garbage collection. */
static const UInt32 kuDataStoreAutoGarbageCollectionMarkerSize = 512u * 1024u;

/** Factor beyond the marked size that triggers garbage collection. */
static const UInt32 kuDataStoreAutoGarbageCollectionFactor = 2u;

/** Used to normalize NaN values. */
static const UInt32 kuDataNodeCanonicalNaNBits = 0xFFFFFFFE;

/**
* Total number of bits in the DataNode reserved for type info - the type info is structured:
* Bit 0  : 1 if the data is a float32, 0 otherwise.
* Bit 1-4: 4 bits of type info for all other types, if bit 0 is 1; otherwise, part of the float32 value.
*/
static const Int32 kiDataNodeTotalTypeBits = 5;
static const UInt32 kuDataNodeTotalTypeBits = 5u;
static const UInt32 kuDataNodeTypeBitsMask = (1 << kuDataNodeTotalTypeBits) - 1;

/** Maximum number of bits available for file type storage in a DataNode FilePath. */
static const UInt32 kuDataNodeFilePathFileTypeBits = 5u;
static const UInt32 kuDataNodeFilePathFileTypeMask = (1 << kuDataNodeFilePathFileTypeBits) - 1;

/** Maximum number of bits available for game directory storage in a DataNode FilePath. */
static const UInt32 kuDataNodeFilePathGameDirectoryBits = 3u;
static const UInt32 kuDataNodeFilePathGameDirectoryMask = (1 << kuDataNodeFilePathGameDirectoryBits) - 1;

/** Maximum number of bits available for the relative filename (HString) in a DataNode FilePath. */
static const UInt32 kuDataNodeFilePathRelativeFilenameBits = 19u;
static const UInt32 kuDataNodeFilePathRelativeFilenameMask = (1 << kuDataNodeFilePathRelativeFilenameBits) - 1;

/** Min/max values of an Int32 small (27 bits). */
static const Int32 kiDataNodeMaxInt32SmallValue = (1 << 26) - 1;
static const Int32 kiDataNodeMinInt32SmallValue = ~kiDataNodeMaxInt32SmallValue;

// Sanity checks.
SEOUL_STATIC_ASSERT((UInt32)FileType::FILE_TYPE_COUNT <= (1 << kuDataNodeFilePathFileTypeBits));
SEOUL_STATIC_ASSERT((UInt32)GameDirectory::GAME_DIRECTORY_COUNT <= (1 << kuDataNodeFilePathGameDirectoryBits));
SEOUL_STATIC_ASSERT(HStringDataProperties<HStringData::InternalIndexType>::GlobalArraySize <= (1 << kuDataNodeFilePathRelativeFilenameBits));
SEOUL_STATIC_ASSERT(SEOUL_FILEPATH_HSTRING_VALUE_SIZE == (1 << kuDataNodeFilePathRelativeFilenameBits));

/**
 * DataNode encapsulates dynamically typed data in a DataStore. The
 * concrete types supported by DataNode are listed in DataNode::Type.
 */
class DataNode SEOUL_SEALED
{
public:
	// NOTE: Values for enum are carefully selected to be even,
	// so that retrieving the type is cheaper. Type is stored as follows:
	// - bit    0: 1 if Float32, 0 if any other type.
	// - bits 0-4: type enum value below if bit 0 is 0.
	enum Type
	{
		/** "Empty" data - can be converted to identifier, FilePath, or string. */
		kNull = 0,

		// Float31 is the only value with an odd type number, since it is special (1
		// in bit 0 is always a Float31).
		/** 31-bit floating point variable. */
		kFloat31 = 1,

		/** A true/false Bool value. */
		kBoolean = 2,

		/** 32-bit signed integer, stored in 27 bits. */
		kInt32Small = 4,

		/** A FilePath reference to a file. */
		kFilePath = 6,

		// BEGIN REFERENCE VALUES

		/** A closed hash table of DataNode values. */
		kTable = 8,

		/** A contiguous array of DataNode values. */
		kArray = 10,

		/** UTF8 string data. */
		kString = 12,

		/** 32-bit signed integer, full 32-bits of storage. */
		kInt32Big = 14,

		/** 32-bit unsigned integer, full 32-bits of storage. */
		kUInt32 = 16,

		/** A 64-bit signed integer. */
		kInt64 = 18,

		/** A 64-bit unsigned integer. */
		kUInt64 = 20,

		// END ORIGINAL REFERENCE VALUES

		/** Special value - only appears in diff DataStores. Marks a field as "erased". */
		kSpecialErase = 22,

		/**
		 * A 32-bit floating point value. No precision loss, used when
		 * a value would incur precision loss when stored as a 31-bit float.
		 */
		kFloat32 = 24, // REFERENCE VALUE

		// Marker value, equal to the last valid enum.
		LAST_TYPE = kFloat32,
	};

	/** @return true if eType is a reference type. */
	static inline Bool IsByReference(Type eType)
	{
		switch (eType)
		{
		case kTable:
		case kArray:
		case kString:
		case kInt32Big:
		case kUInt32:
		case kInt64:
		case kUInt64:
		case kFloat32:
			return true;
		default:
			return false;
		};
	}

	/** @return True if this DataNode is an array. */
	Bool IsArray() const { return (kArray == GetType()); }

	/** @return True if this DataNode is a boolean value. */
	Bool IsBoolean() const { return (kBoolean == GetType()); }

	/** @return True if this DataNode is a heap allocated value type. */
	Bool IsByReference() const { return IsByReference(GetType()); }

	/** @return True if this DataNode contains a file path. */
	Bool IsFilePath() const { return (kFilePath == GetType()); }

	/** @return True if this DataNode contains a 31-bit floating point value. */
	Bool IsFloat31() const { return (kFloat31 == GetType()); }

	/** @return True if this DataNode contains a 32-bit precise floating point value (no loss of the 1-bit mantissa). */
	Bool IsFloat32() const { return (kFloat32 == GetType()); }

	/** @return True if this DataNode contains a big 32-bit signed integer. */
	Bool IsInt32Big() const { return (kInt32Big == GetType()); }

	/** @return True if this DataNode contains a small 32-bit signed integer. */
	Bool IsInt32Small() const { return (kInt32Small == GetType()); }

	/** @return True if this DataNode contains a 64-bit signed integer. */
	Bool IsInt64() const { return (kInt64 == GetType()); }

	/** @return True if this DataNode contains a null or "empty" value. */
	Bool IsNull() const { return (kNull == GetType()); }

	/** @return True if this DataNode is set to the special erase bit. */
	Bool IsSpecialErase() const { return (kSpecialErase == GetType()); }

	/** @return True if this DataNode contains UTF8 string data. */
	Bool IsString() const { return (kString == GetType()); }

	/** @return True if this DataNode contains a closed hash table. */
	Bool IsTable() const { return (kTable == GetType()); }

	/** @return True if this DataNode contains a 32-bit unsigned integer value. */
	Bool IsUInt32() const { return (kUInt32 == GetType()); }

	/** @return True if this DataNode contains a 64-bit unsigned integer value. */
	Bool IsUInt64() const { return (kUInt64 == GetType()); }

	/** For low-level use only - assume the given UInt32 value is a raw data value and generated an appropriate DataNode from it. */
	static DataNode FromRawDataValue(UInt32 uRawDataValue)
	{
		DataNode ret;
		ret.m_uData = uRawDataValue;
		return ret;
	}

	DataNode()
		: m_uData(0)
	{
	}

	DataNode(const DataNode& b)
		: m_uData(b.m_uData)
	{
	}

	DataNode& operator=(const DataNode& b)
	{
		m_uData = b.m_uData;
		return *this;
	}

	/**
	 * Equivalent to Handle<>, but defined internally so we can precisely control
	 * sizes and layout.
	 */
	struct Handle
	{
		/** Number of bits of a 27-bit unsigned integer allocated for the handle index. */
		static const UInt32 kIndexBits = 24;
		static const UInt32 kIndexMask = (1 << kIndexBits) - 1;

		/** Remaining binds of a 27-bit unsigned integer reserved for the generation ID. */
		static const UInt32 kGenerationBits = (27 - kIndexBits);
		static const UInt32 kGenerationMask = (1 << kGenerationBits) - 1;

		// struct
		// {
		//   /* Offset into the handle array. */
		//   UInt32 m_uIndex : 24;
		//   /* Generation id, used for some basic stale handle identiciation. */
		//   UInt32 m_uGenerationId : 3;
		//   /* Bits used in a DataNode for type info, unused in the handle data. */
		//   UInt32 m_uUnusedReserved : 5;
		// };
		uint32_t m_uData;

		UInt32 GetIndex() const
		{
			// Offset into the handle array.
			// UInt32 m_uIndex : 24;
			return (m_uData & 0x00FFFFFF);
		}

		void SetIndex(UInt32 u)
		{
			// Offset into the handle array.
			// UInt32 m_uIndex : 24;
			m_uData = (u & 0x00FFFFFF) | (m_uData & 0xFF000000);
		}

		UInt32 GetGenerationId() const
		{
			// Generation id, used for some basic stale handle identiciation.
			// UInt32 m_uGenerationId : 3;
			return (m_uData >> 24) & 0x7;
		}

		void SetGenerationId(UInt32 u)
		{
			// Generation id, used for some basic stale handle identiciation.
			// UInt32 m_uGenerationId : 3;
			m_uData = ((u & 0x7) << 24) | (m_uData & 0xF8FFFFFF);
		}

		void SetUnusedReserved(UInt32 u)
		{
			// Bits used in a DataNode for type info, unused in the handle data.
			// UInt32 m_uUnusedReserved : 5;
			m_uData = ((u & 0x1F) << 27) | (m_uData & 0x7FFFFFF);
		}

		/**
		 * @return The default Handle value, use to default construct an unassigned handle.
		 */
		static Handle Default()
		{
			Handle ret;
			ret.SetIndex(kIndexMask);
			ret.SetGenerationId(kGenerationMask);
			ret.SetUnusedReserved(0u);
			return ret;
		}

		Bool operator==(const Handle& b) const
		{
			return GetIndex() == b.GetIndex() && GetGenerationId() == b.GetGenerationId();
		}

		Bool operator!=(const Handle& b) const
		{
			return !(*this == b);
		}
	};

	/** @return This DataNode as a boolean, SEOUL_FAIL() if this DataNode is not a Boolean type. */
	Bool GetBoolean() const
	{
		SEOUL_ASSERT(IsBoolean());
		return (GetNotFloat31ValuePart() != 0u);
	}

	/** @return This DataNode as a FilePath, SEOUL_FAIL() if this DataNode is not a FilePath type. */
	FilePath GetFilePath() const
	{
		SEOUL_ASSERT(IsFilePath());

		// Get the raw bits.
		UInt32 const uRawValue = GetNotFloat31ValuePart();

		// Extract the relative filename.
		FilePathRelativeFilename relativeFilename;
		relativeFilename.SetHandleValue((uRawValue >> (kuDataNodeFilePathFileTypeBits + kuDataNodeFilePathGameDirectoryBits)) & kuDataNodeFilePathRelativeFilenameMask);

		// Extract the game directory and file type, and set all to the FilePath instance.
		FilePath ret;
		ret.SetDirectory((GameDirectory)(uRawValue & kuDataNodeFilePathGameDirectoryMask));
		ret.SetRelativeFilenameWithoutExtension(relativeFilename);
		ret.SetType((FileType)((uRawValue >> kuDataNodeFilePathGameDirectoryBits) & kuDataNodeFilePathFileTypeMask));
		return ret;
	}

	/**
	 * Assign out the parts of a FilePath as raw values.
	 */
	void GetFilePathRaw(
		GameDirectory& reDirectory,
		HStringData::InternalIndexType& ruRelativeFilenameWithoutExtension,
		FileType& reFileType) const
	{
		SEOUL_ASSERT(IsFilePath());

		// Get the raw bits.
		UInt32 const uRawValue = GetNotFloat31ValuePart();

		// Extract the game directory.
		reDirectory = (GameDirectory)(uRawValue & kuDataNodeFilePathGameDirectoryMask);

		// Extract the relative filename.
		ruRelativeFilenameWithoutExtension = (HStringData::InternalIndexType)
			((uRawValue >> (kuDataNodeFilePathFileTypeBits + kuDataNodeFilePathGameDirectoryBits)) & kuDataNodeFilePathRelativeFilenameMask);

		// Extract the file type.
		reFileType = (FileType)((uRawValue >> kuDataNodeFilePathGameDirectoryBits) & kuDataNodeFilePathFileTypeMask);
	}

	/** @return This DataNode as a Float32, SEOUL_FAIL() if this DataNode is not a Float32 type. */
	Float32 GetFloat32() const
	{
		SEOUL_ASSERT(IsFloat32());

		union
		{
			UInt32 uInValue;
			Float32 fOutValue;
		};

		// Grab the raw value, and then mask away the lowest bit (which is type info).
		uInValue = m_uData;
		uInValue &= ~0x1;
		return fOutValue;
	}

	/** @return This DataNode as a Float31, SEOUL_FAIL() if this DataNode is not a Float31 type. */
	Float32 GetFloat31() const
	{
		SEOUL_ASSERT(IsFloat31());

		union
		{
			UInt32 uInValue;
			Float32 fOutValue;
		};

		// Grab the raw value, and then mask away the lowest bit (which is type info).
		uInValue = m_uData;
		uInValue &= ~0x1;
		return fOutValue;
	}

	/** @return This DataNode as a by-reference Handle, SEOUL_FAIL() if this DataNode is not a Handle type. */
	Handle GetHandle() const
	{
		SEOUL_ASSERT(IsByReference());

		// Get the raw bits.
		UInt32 const uRawValue = GetNotFloat31ValuePart();

		// Extract the generation ID and handle index, and assign both to the Handle instance.
		Handle ret;
		ret.SetGenerationId(uRawValue & Handle::kGenerationMask);
		ret.SetIndex((uRawValue >> Handle::kGenerationBits) & Handle::kIndexMask);
		ret.SetUnusedReserved(0u);
		return ret;
	}

	/** @return This DataNode as an Int32 small, SEOUL_FAIL() if this DataNode is not a Int32 small type. */
	Int32 GetInt32Small() const
	{
		SEOUL_ASSERT(IsInt32Small());

		union
		{
			UInt32 uInValue;
			Int32 iOutValue;
		};

		// Do this a bit differently - assign
		// the raw data directly and then shift,
		// so we correctly sign extend a signed int.
		uInValue = m_uData;
		return (iOutValue >> kiDataNodeTotalTypeBits);
	}

	/** @return The raw data of this DataNode, can be used for quick by-value comparisons or low-level manipulations. */
	UInt32 GetRawDataValue() const
	{
		return m_uData;
	}

	/** @return The type of this DataNode. */
	Type GetType() const
	{
		// If lowest bit is 1, then we're a Float31.
		return ((m_uData & 0x1) != 0u
			? kFloat31
			// Otherwise, the type info is in the lowest 5 bits.
			: ((Type)(m_uData & kuDataNodeTypeBitsMask)));
	}

	/** Update this DataNode to a boolean value and type. */
	DataNode& SetBoolean(Bool bValue)
	{
		SetNotFloat31Value(bValue ? 1u : 0u, kBoolean);
		return *this;
	}

	/** Update this DataNode to a FilePath value and type. */
	DataNode& SetFilePath(FilePath filePath)
	{
		UInt32 const uRawValue = (
			((UInt32)filePath.GetRelativeFilenameWithoutExtension().GetHandleValue() << (kuDataNodeFilePathFileTypeBits + kuDataNodeFilePathGameDirectoryBits)) |
			((UInt32)filePath.GetType() << kuDataNodeFilePathGameDirectoryBits) |
			((UInt32)filePath.GetDirectory()));

		SetNotFloat31Value(uRawValue, kFilePath);
		return *this;
	}

	/** Update this DataNode to a FilePath value and type from raw FilePath parts.. */
	DataNode& SetFilePathRaw(
		GameDirectory eDirectory,
		HStringData::InternalIndexType uRelativeFilenameWithoutExtension,
		FileType eFileType)
	{
		UInt32 const uRawValue = (
			((UInt32)uRelativeFilenameWithoutExtension << (kuDataNodeFilePathFileTypeBits + kuDataNodeFilePathGameDirectoryBits)) |
			((UInt32)eFileType << kuDataNodeFilePathGameDirectoryBits) |
			((UInt32)eDirectory));

		SetNotFloat31Value(uRawValue, kFilePath);
		return *this;
	}

	/** Update this DataNode to a Float32 value and type. */
	DataNode& SetFloat31(Float32 fValue)
	{
		union
		{
			Float32 fInValue;
			UInt32 uOutValue;
		};

		// Set the float value for conversion.
		fInValue = fValue;

		// Use a consistent canonical NaN value that fits with our masking scheme.
		// Failure to do this can result in a NaN turning into an Infinity, etc.
		if (IsNaN(fInValue))
		{
			uOutValue = kuDataNodeCanonicalNaNBits;
		}

		// Sanity check.
		SEOUL_ASSERT((uOutValue & 0x1) == 0);

		// We use the lowest bit as a type bit that represents "is float", so mask it
		// away and then force it to 1.
		uOutValue &= ~0x1;
		uOutValue |= 0x1;

		// Set the value through.
		m_uData = uOutValue;
		return *this;
	}

	/** Update this DataNode to a by-reference Handle value and explicitly passed type. */
	DataNode& SetHandle(Handle h, Type eByReferenceType)
	{
		SEOUL_ASSERT(IsByReference(eByReferenceType));

		UInt32 const uRawValue = (
			(h.GetIndex() << Handle::kGenerationBits) | (h.GetGenerationId() & Handle::kGenerationMask));

		SetNotFloat31Value(uRawValue, eByReferenceType);
		return *this;
	}

	/**
	 * Update this DataNode to an Int32 small (inline, 27-bits) value and type.
	 *
	 * \pre iValue must be >= kiDataNodeMinInt32SmallValue and <= kiDataNodeMaxInt32SmallValue.
	 */
	DataNode& SetInt32Small(Int32 iValue)
	{
		SEOUL_ASSERT(iValue >= kiDataNodeMinInt32SmallValue && iValue <= kiDataNodeMaxInt32SmallValue);

		union
		{
			Int32 iInValue;
			UInt32 uOutValue;
		};

		iInValue = iValue;
		SetNotFloat31Value(uOutValue, kInt32Small);
		return *this;
	}

	/**
	 * Update this DataNode to a special erase bit.
	 *
	 * Used only in DataStores generated for patching. Indicates that ApplyPatch()
	 * should erase the corresponding entry from its enclosing table.
	 */
	DataNode& SetSpecialErase()
	{
		SetNotFloat31Value(0u, kSpecialErase);
		return *this;
	}

private:
	friend class DataStore;
	friend struct DataStoreTableIterator;

	/** @return The value portion of this DataNode, assuming this DataNode is not a float value. */
	UInt32 GetNotFloat31ValuePart() const
	{
		// Invalid for a float32 small value type.
		SEOUL_ASSERT(!IsFloat31());

		return (m_uData >> kuDataNodeTotalTypeBits);
	}

	/** Update the value and type portions of this DataNode, not a Float32. */
	void SetNotFloat31Value(UInt32 uRawValue, Type eType)
	{
		m_uData = (
			(uRawValue << kuDataNodeTotalTypeBits) | (((UInt32)eType) & kuDataNodeTypeBitsMask));
	}

	UInt32 m_uData;
}; // class DataNode

// Sanity check that we got all the sizes correct.
SEOUL_STATIC_ASSERT(sizeof(DataNode) == 4);

// Sanity check - if HString is updated to use a different type, the definition
// of DataStore must also be updated
SEOUL_STATIC_ASSERT(sizeof(HStringData::InternalIndexType) == sizeof(UInt32));

/**
 * Utility structure, used by DataStore to provide forward
 * const iterators to tables inside the DataStore.
 */
struct DataStoreTableIterator
{
	struct IteratorPair
	{
		IteratorPair(HString key, const DataNode& value)
			: First(key)
			, Second(value)
		{
		}

		HString First;
		DataNode Second;
	};

	struct IteratorPairIndirect
	{
		IteratorPairIndirect(HString key, const DataNode& value)
			: m_Pair(key, value)
		{
		}

		IteratorPair m_Pair;

		IteratorPair const* operator->() const
		{
			return &m_Pair;
		}
	};

	DataStoreTableIterator(DataStore const* pOwner, DataNode table, UInt32 nCapacity, UInt32 uIndex = 0)
		: m_pOwner(pOwner)
		, m_Table(table)
		, m_uCapacity(nCapacity)
		, m_uIndex(Index(uIndex))
	{
	}

	DataStoreTableIterator(const DataStoreTableIterator& b)
		: m_pOwner(b.m_pOwner)
		, m_Table(b.m_Table)
		, m_uCapacity(b.m_uCapacity)
		, m_uIndex(b.m_uIndex)
	{
	}

	DataStoreTableIterator& operator=(const DataStoreTableIterator& b)
	{
		m_pOwner = b.m_pOwner;
		m_Table = b.m_Table;
		m_uCapacity = b.m_uCapacity;
		m_uIndex = b.m_uIndex;

		return *this;
	}

	IteratorPair operator*() const
	{
		return IteratorPair(Key(m_uIndex), Value(m_uIndex));
	}

	IteratorPairIndirect operator->() const
	{
		return IteratorPairIndirect(Key(m_uIndex), Value(m_uIndex));
	}

	DataStoreTableIterator& operator++()
	{
		m_uIndex = Index(m_uIndex + 1);

		return *this;
	}

	DataStoreTableIterator operator++(int)
	{
		DataStoreTableIterator ret(m_pOwner, m_Table, m_uCapacity, m_uIndex);

		m_uIndex = Index(m_uIndex + 1);

		return ret;
	}

	Bool operator==(const DataStoreTableIterator& b) const
	{
		return (m_Table.GetRawDataValue() == b.m_Table.GetRawDataValue() && m_uIndex == b.m_uIndex);
	}

	Bool operator!=(const DataStoreTableIterator& b) const
	{
		return !(*this == b);
	}

private:
	DataStore const* m_pOwner;
	DataNode m_Table;
	UInt32 m_uCapacity;
	UInt32 m_uIndex;

	UInt32 Index(UInt32 uIndex) const
	{
		HString const kNullKey;
		while (uIndex < m_uCapacity && kNullKey == Key(uIndex))
		{
			uIndex++;
		}

		return uIndex;
	}

	HString Key(UInt32 uIndex) const;
	DataNode Value(UInt32 uIndex) const;
}; // class DataStoreTableIterator

namespace DataStoreCommon
{

struct Container
{
	// "excluding null" only has meaning for tables, but we
	// use the same header for all container types. For arrays
	// and strings, GetHasNullStorage() and GetHasNull() will always
	// be false (0), and the capacity and count "excluding null"
	// are just equal to the array and string capacities and counts.
	// struct
	// {
	//   uint32_t m_uCapacityExcludingNull : 31;
	//   uint32_t m_bHasNullStorage : 1;
	// };
	uint32_t m_uCapacity;
	// struct
	// {
	//   uint32_t m_uCountExcludingNull : 31;
	//   uint32_t m_bHasNull : 1;
	// };
	uint32_t m_uCount;

	UInt32 GetCapacityExcludingNull() const
	{
		// uint32_t m_uCapacityExcludingNull : 31;
		return (m_uCapacity & 0x7FFFFFFF);
	}
	void SetCapacityExcludingNull(UInt32 u)
	{
		// uint32_t m_uCapacityExcludingNull : 31;
		m_uCapacity = (u & 0x7FFFFFFF) | (m_uCapacity & 0x80000000);
	}

	UInt32 GetHasNullStorage() const
	{
		// uint32_t m_bHasNullStorage : 1;
		return (m_uCapacity >> 31) & 0x1;
	}
	void SetHasNullStorage(UInt32 u)
	{
		// uint32_t m_bHasNullStorage : 1;
		m_uCapacity = ((u & 0x1) << 31) | (m_uCapacity & 0x7FFFFFFF);
	}

	UInt32 GetCountExcludingNull() const
	{
		// uint32_t m_uCountExcludingNull : 31;
		return (m_uCount & 0x7FFFFFFF);
	}
	void SetCountExcludingNull(UInt32 u)
	{
		// uint32_t m_uCountExcludingNull : 31;
		m_uCount = (u & 0x7FFFFFFF) | (m_uCount & 0x80000000);
	}

	UInt32 GetHasNull() const
	{
		// uint32_t m_bHasNull : 1;
		return (m_uCount >> 31) & 0x1;
	}
	void SetHasNull(UInt32 u)
	{
		// uint32_t m_bHasNull : 1;
		m_uCount = ((u & 0x1) << 31) | (m_uCount & 0x7FFFFFFF);
	}
};
SEOUL_STATIC_ASSERT(sizeof(Container) == 8);

struct DataEntry
{
	UInt32 m_uData;

	static DataEntry Default()
	{
		DataEntry ret;
		ret.m_uData = 0u;
		return ret;
	}

	/**
	 * Cast this DataEntry to a Container header.
	 *
	 * \warning Low-level function, no checking. If this DataEntry
	 * is not a container, value of Container is undefined.
	 */
	Container AsContainer() const
	{
		return *reinterpret_cast<Container const*>(this);
	}

	/**
	 * Cast this DataEntry to a Container header reference.
	 *
	 * \warning Low-level function, no checking. If this DataEntry
	 * is not a container, value of Container is undefined.
	 */
	Container& AsContainer()
	{
		return *reinterpret_cast<Container*>(this);
	}

	/**
	 * Cast this DataEntry to a DataNode.
	 *
	 * \warning Low-level function, no checking. If this DataEntry
	 * is not a DataNode, value of DataNode is undefined.
	 */
	DataNode AsDataNode() const
	{
		return *reinterpret_cast<DataNode const*>(this);
	}

	/**
	 * Cast this DataEntry to a DataNode reference.
	 *
	 * \warning Low-level function, no checking. If this DataEntry
	 * is not a DataNode, value of DataNode is undefined.
	 */
	DataNode& AsDataNode()
	{
		return *reinterpret_cast<DataNode*>(this);
	}

	/**
	 * Cast this DataEntry to a Float32 value.
	 *
	 * \warning Low-level function, no checking. If this DataEntry
	 * is not a Float32 precise value, value of Float32 is undefined.
	 */
	Float32 AsFloat32Value() const
	{
		return *reinterpret_cast<Float32 const*>(this);
	}

	/**
	 * Cast this DataEntry to a Float32 value.
	 *
	 * \warning Low-level function, no checking. If this DataEntry
	 * is not a Float32 precise value, value of Float32 is undefined.
	 */
	Float32& AsFloat32Value()
	{
		return *reinterpret_cast<Float32*>(this);
	}

	/**
	 * Cast this DataEntry to an Int32 big value.
	 *
	 * \warning Low-level function, no checking. If this DataEntry
	 * is not an Int32 big value, value of Int32 is undefined.
	 */
	Int32 AsInt32BigValue() const
	{
		return *reinterpret_cast<Int32 const*>(this);
	}

	/**
	 * Cast this DataEntry to an Int32 big value reference.
	 *
	 * \warning Low-level function, no checking. If this DataEntry
	 * is not an Int32 big value, value of Int32 is undefined.
	 */
	Int32& AsInt32BigValue()
	{
		return *reinterpret_cast<Int32*>(this);
	}

	/**
	 * Cast this DataEntry to a UInt32 value.
	 *
	 * \warning Low-level function, no checking. If this DataEntry
	 * is not a UInt32 value, value of UInt32 is undefined.
	 */
	UInt32 AsUInt32Value() const
	{
		return *reinterpret_cast<UInt32 const*>(this);
	}

	/**
	 * Cast this DataEntry to a UInt32 value reference.
	 *
	 * \warning Low-level function, no checking. If this DataEntry
	 * is not a UInt32 value, value of UInt32 is undefined.
	 */
	UInt32& AsUInt32Value()
	{
		return *reinterpret_cast<UInt32*>(this);
	}

	/**
	 * Cast this DataEntry to an Int64 value.
	 *
	 * \warning Low-level function, no checking. If this DataEntry
	 * is not an Int64 value, value of Int64 is undefined.
	 */
	Int64 AsInt64Value() const
	{
		// Don't just cast here, raw data may not be aligned.
		Int64 iReturn = 0;
		memcpy(&iReturn, this, sizeof(Int64));
		return iReturn;
	}

	/**
	 * Update the Int64 value of this DataEntry.
	 *
	 * \warning Low level function, no checking. If this DataEntry
	 * is not the start of an Int64 value, results are undefined.
	 */
	void SetInt64Value(Int64 iValue)
	{
		memcpy(this, &iValue, sizeof(iValue));
	}

	/**
	 * Cast this DataEntry to a UInt64 value.
	 *
	 * \warning Low-level function, no checking. If this DataEntry
	 * is not a UInt64 value, value of UInt64 is undefined.
	 */
	UInt64 AsUInt64Value() const
	{
		// Don't just cast here, raw data may not be aligned.
		UInt64 uReturn = 0;
		memcpy(&uReturn, this, sizeof(Int64));
		return uReturn;
	}

	/**
	 * Update the UInt64 value of this DataEntry.
	 *
	 * \warning Low level function, no checking. If this DataEntry
	 * is not the start of a UInt64 value, results are undefined.
	 */
	void SetUInt64Value(UInt64 uValue)
	{
		memcpy(this, &uValue, sizeof(uValue));
	}

	/**
	 * Populate this DataEntry to a DataNode.
	 *
	 * \warning Low level function, no checking. If this DataEntry
	 * is not a valid target for DataNode, results are undefined.
	 */
	void MakeDataNode(DataNode node)
	{
		SEOUL_STATIC_ASSERT(sizeof(node) == sizeof(m_uData));
		m_uData = node.GetRawDataValue();
	}
}; // class DataEntry

// Sanity checking.
SEOUL_STATIC_ASSERT(sizeof(DataEntry) == 4u);
SEOUL_STATIC_ASSERT(sizeof(Container) % sizeof(DataEntry) == 0u);

struct HandleDataOffset SEOUL_SEALED
{
	// Handles default to a generation id of 7, so we default to 0
	// to maximize the distance between the default.
	static const UInt32 kuDefaultGenerationId = 0;

	// Invalid handle is the max value.
	static const UInt32 kuInvalidHandleOffset = (1 << 29) - 1;

	// struct
	// {
	//   UInt32 m_uDataOffset : 29;
	//   UInt32 m_uGenerationId : 3;
	// };
	UInt32 m_uData;

	UInt32 GetDataOffset() const
	{
		// UInt32 m_uDataOffset : 29;
		return (m_uData & 0x1FFFFFFF);
	}
	void SetDataOffset(UInt32 u)
	{
		// UInt32 m_uDataOffset : 29;
		m_uData = (u & 0x1FFFFFFF) | (m_uData & 0xE0000000);
	}

	UInt32 GetGenerationId() const
	{
		// UInt32 m_uGenerationId : 3;
		return (m_uData >> 29) & 0x7;
	}
	void SetGenerationId(UInt32 u)
	{
		// UInt32 m_uGenerationId : 3;
		m_uData = ((u & 0x7) << 29) | (m_uData & 0x1FFFFFFF);
	}

	/**
	 * @return The default HandleOffset, used to default construct an unassigned
	 * HandleOffset.
	 */
	static HandleDataOffset Default()
	{
		HandleDataOffset ret;
		ret.SetDataOffset(kuInvalidHandleOffset);
		ret.SetGenerationId(kuDefaultGenerationId);
		return ret;
	}

	/**
	 * @return True if this is a valid handle entry, false if this
	 * entry is free for reallocation.
	 */
	Bool IsValid() const
	{
		return (kuInvalidHandleOffset != GetDataOffset());
	}
}; // struct HandleDataOffset
SEOUL_STATIC_ASSERT(sizeof(HandleDataOffset) == 4u);

} // namespace DataStoreCommon

/**
 * DataStore contains hierarchical data with dynamic typing. Leaves
 * of the DataStore include 32-bit and 64-bit integer types, 32-bit floating
 * point types, strings, "identifiers", and file paths. Branches include
 * arrays and tables.
 *
 * An "identifier" is a subset of HStrings that obey C-style identifier
 * syntax rules - alphanumeric characters, plus '_', where the first character
 * is not a digit.
 */
class DataStore SEOUL_SEALED
{
public:
	/** Iterator used when enumerating the elements of a table inside this DataStore. */
	typedef DataStoreTableIterator TableIterator;

	/** The largest array size that can be defined in a DataStore. */
	static const UInt32 kBigArray = (1 << 20u);

	/** The maximum number of handles supported - 2^kHandleBits. */
	static const UInt32 kMaxHandleCount = (1 << DataNode::Handle::kIndexBits);

	DataStore();
	DataStore(UInt32 zInitialCapacityInBytes);
	~DataStore();

	// Reserve at least zCapacityInBytes in the DataStore's internal heap.
	void Reserve(UInt32 zCapacityInBytes);

	/**
	 * @return A calculated checksum of this DataStore's contents.
	 */
	String ComputeMD5() const;

	/**
	 * Collect all items to the head of this DataStore's heap.
	 *
	 * @param[in] bCompactContainers If true, arrays and tables are compacted
	 * to their minimize size, vs. the capacity in the current heap. Likely,
	 * only use as part of a final step when populating a read-only DataStore,
	 * followed by a call to CompactHeap().
	 *
	 * This method does not reduce the overall system memory used by this
	 * DataStore. Call CompactHeap() after calling CollectGarbage() to reduce
	 * this DataStore to its minimum size - in most cases, this should be done
	 * when you expect few (if any) mutations to occur to this DataStore
	 * after the call.
	 */
	void CollectGarbage(Bool bCompactContainers = false)
	{
	 	 InternalCollectGarbage(bCompactContainers);
	}

	// Actually reduces the system memory used by this DataStore - in almost all cases, should
	// be preceeded by a call to CollectGarbage() and should be called when you do not expect
	// any more mutations to occur to this DataStore.
	void CompactHeap();

	/**
	 * Convenience function, calls CollectGarbage() and then CompactHeap().
	 *
	 * Calls CollectGarbage(true), to maximize size reduction (compresses
	 * tables to their minimum size).
	 */
	void CollectGarbageAndCompactHeap()
	{
		CollectGarbage(true);
		CompactHeap();
	}

	// Return true if valueA is equal to valueB, recursively, false
	// otherwise. This is a "by value" equals.
	static Bool Equals(
		const DataStore& dataStoreA,
		const DataNode& valueA,
		const DataStore& dataStoreB,
		const DataNode& valueB,
		Bool bNaNEquals = false);

	/**
	 * @return The total heap capacity of this DataStore in bytes -
	 * this is the actual total system heap memory being occupied by
	 * this DataStore.
	 */
	UInt32 GetHeapCapacityInBytes() const
	{
		return m_vData.GetCapacityInBytes();
	}

	/**
	 * @return The total heap memory usage of this DataStore.
	 */
	UInt32 GetInUseHeapSizeInBytes() const
	{
		return m_vData.GetSizeInBytes();
	}

	/**
	 * @return The DataNode of the root of this DataStore.
	 */
	DataNode GetRootNode() const
	{
		DataNode ret;
		if (!m_vData.IsEmpty())
		{
			ret = m_vData.Front().AsDataNode();
		}

		return ret;
	}

	/**
	 * Converts the root node to an array type - invalidates all existing
	 * data in this DataStore.
	 */
	void MakeArray(UInt32 nInitialCapacity = 0u)
	{
		using namespace DataStoreCommon;

		InternalClearHandles();
		m_vData.Clear();
		m_uDataSizeAfterLastCollection = kuDataStoreAutoGarbageCollectionMarkerSize;

		m_vData.Resize(1u, DataEntry::Default());
		DataNode::Handle const hArray = InternalCreateArray(nInitialCapacity);
		m_vData.Front().MakeDataNode(DataNode().SetHandle(hArray, DataNode::kArray));
	}

	/**
	 * Converts the root node to a table type - invalidates all existing
	 * data in this DataStore.
	 *
	 * nInitialCapacity will be rounded to the next largest power of 2, if it is not a power of 2 value.
	 */
	void MakeTable(UInt32 nInitialCapacity = 0u)
	{
		using namespace DataStoreCommon;

		InternalClearHandles();
		m_vData.Clear();
		m_uDataSizeAfterLastCollection = kuDataStoreAutoGarbageCollectionMarkerSize;

		m_vData.Resize(1u, DataEntry::Default());
		DataNode::Handle const hTable = InternalCreateTable(nInitialCapacity);
		m_vData.Front().MakeDataNode(DataNode().SetHandle(hTable, DataNode::kTable));
	}

	/**
	 * Take the current root of the DataStore, and make it a value of a new root table
	 * with the given key.
	 *
	 * @param[in] The key to place the old root into the new table.
	 * @param[in] The initial capacity of the new table.
	 */
	void MoveRootIntoNewRootTable(HString key, UInt32 nInitialCapacity = 0u)
	{
		DataNode const node = GetRootNode();
		DataNode::Handle const hTable = InternalCreateTable(nInitialCapacity);
		m_vData.Front().MakeDataNode(DataNode().SetHandle(hTable, DataNode::kTable));
		(void)InternalSetTableValue(GetRootNode(), key, node);
	}

	Bool MoveNodeBetweenTables(const DataNode& tableFrom, HString keyFrom, const DataNode& tableTo, HString keyTo)
	{
		DataNode node;
		if (!GetValueFromTable(tableFrom, keyFrom, node))
		{
			return false;
		}

		// Early out without moving/erasing if this is a noop
		if (keyFrom == keyTo && tableFrom.GetRawDataValue() == tableTo.GetRawDataValue())
		{
			return true;
		}

		if (!InternalSetTableValue(tableTo, keyTo, node))
		{
			return false;
		}

		return EraseValueFromTable(tableFrom, keyFrom);
	}

	/**
	 * Given an element of an array, replaces the root of the table with that element, if
	 * the element is a table or array.
	 */
	Bool ReplaceRootWithArrayElement(const DataNode& array, UInt32 uIndex)
	{
		DataNode value;
		if (GetValueFromArray(array, uIndex, value) && (
			DataNode::kArray == value.GetType() ||
			DataNode::kTable == value.GetType()))
		{
			m_vData.Front().MakeDataNode(value);
			return true;
		}

		return false;
	}

	/**
	 * Given an element of a table, replaces the root of the table with that element, if
	 * the element is a table or array.
	 */
	Bool ReplaceRootWithTableElement(const DataNode& table, HString key)
	{
		DataNode value;
		if (GetValueFromTable(table, key, value) && (
			DataNode::kArray == value.GetType() ||
			DataNode::kTable == value.GetType()))
		{
			m_vData.Front().MakeDataNode(value);
			return true;
		}

		return false;
	}

	/**
	 * @return True if array contains the value identifierValue.
	 *
	 * Returns false if the array does not contain the value, or if array
	 * is not an array.
	 */
	Bool ArrayContains(const DataNode& array, HString identifier) const
	{
		using namespace DataStoreCommon;

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
		DataNode* pValues = (DataNode*)(pContainer + 1u);

		for (UInt32 i = 0u; i < pContainer->GetCountExcludingNull(); ++i)
		{
			DataNode value = pValues[i];

			// Check the string against the identifier.
			if (value.IsString())
			{
				Byte const* s = nullptr;
				UInt32 u = 0u;
				SEOUL_VERIFY(AsString(value, s, u));
				HString v;
				if (HString::Get(v, s, u) && v == identifier)
				{
					return true;
				}
			}
		}

		return false;
	}

	/**
	 * If successful, sets rnCapacity to the capacity of array.
	 *
	 * @return True if the operation was successful, false otherwise. If this
	 * method returns false, rnCapacity is left unchanged, otherwise it
	 * is set to the capacity of array. This method can return false:
	 * - if array is not of type kArray.
	 */
	Bool GetArrayCapacity(const DataNode& array, UInt32& rnCapacity) const
	{
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

		rnCapacity = InternalGetDataEntry(handle).AsContainer().GetCapacityExcludingNull();
		return true;
	}

	/**
	 * If successful, sets rnCount to the number of elements in array.
	 *
	 * @return True if the operation was successful, false otherwise. If this
	 * method returns false, rnCount is left unchanged, otherwise it
	 * is set to the count of array. This method can return false:
	 * - if array is not of type kArray.
	 */
	Bool GetArrayCount(const DataNode& array, UInt32& rnCount) const
	{
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

		rnCount = InternalGetDataEntry(handle).AsContainer().GetCountExcludingNull();
		return true;
	}

	// If successful, updates the size of array to nNewSize.
	Bool ResizeArray(const DataNode& array, UInt32 nNewSize);

	// Return a TableIterator at the head of table, or an iterator == TableEnd() if
	// the table is empty, or if table is not a table.
	TableIterator TableBegin(const DataNode& table) const;

	// Return a TableIterator at the end of table.
	TableIterator TableEnd(const DataNode& table) const;

	/**
	 * If successful, sets rnCapacity to the capacity of table.
	 *
	 * @return True if the operation was successful, false otherwise. If this
	 * method returns false, rnCapacity is left unchanged, otherwise it
	 * is set to the capacity of table. This method can return false:
	 * - if table is not of type kTable.
	 */
	Bool GetTableCapacity(const DataNode& table, UInt32& rnCapacity) const
	{
		// Early out if not a table.
		if (!table.IsTable())
		{
			return false;
		}

		// Check for a valid DataStore handle.
		DataNode::Handle const handle = table.GetHandle();
		if (!InternalIsValidHandle(handle))
		{
			return false;
		}

		rnCapacity = InternalGetDataEntry(handle).AsContainer().GetCapacityExcludingNull();
		return true;
	}

	/**
	 * If successful, sets rnCount to the number of entries in table.
	 *
	 * @return True if the operation was successful, false otherwise. If this
	 * method returns false, rnCount is left unchanged, otherwise it
	 * is set to the count of table. This method can return false:
	 * - if table is not of type kTable.
	 */
	Bool GetTableCount(const DataNode& table, UInt32& rnCount) const
	{
		using namespace DataStoreCommon;

		// Early out if not a table.
		if (!table.IsTable())
		{
			return false;
		}

		// Check for a valid DataStore handle.
		DataNode::Handle const handle = table.GetHandle();
		if (!InternalIsValidHandle(handle))
		{
			return false;
		}

		Container const container = InternalGetDataEntry(handle).AsContainer();
		rnCount = container.GetCountExcludingNull() + container.GetHasNull();
		return true;
	}

	/**
	 * @return The Bool data in value. If value is not of type kBoolean, the
	 * return value from this method is undefined.
	 */
	Bool AssumeBoolean(const DataNode& value) const
	{
		return value.GetBoolean();
	}

	/**
	 * Assign rbValue with the value of value if it can be trivially
	 * converted to a boolean.
	 *
	 * @return True if rbValue was modified, false otherwise.
	 */
	Bool AsBoolean(const DataNode& value, Bool& rbValue) const
	{
		switch (value.GetType())
		{
		case DataNode::kBoolean:
			rbValue = value.GetBoolean();
			return true;
		case DataNode::kInt32Small:
			rbValue = (0 != value.GetInt32Small());
			return true;
		default:
			break;
		};

		return false;
	}

	/**
	 * Assign rFilePath with the value of value if it can be trivially
	 * converted to a FilePath.
	 *
	 * @return True if rFilePath was modified, false otherwise.
	 */
	Bool AsFilePath(const DataNode& value, FilePath& rFilePath) const
	{
		switch (value.GetType())
		{
		case DataNode::kFilePath:
			rFilePath = value.GetFilePath();
			return true;
		case DataNode::kNull:
			rFilePath.Reset();
			return true;
		case DataNode::kString:
			return InternalFilePathFromString(value, rFilePath);
		default:
			break;
		};

		return false;
	}

	/**
	 * Assign rsString with the value of value if it can be trivially
	 * converted to a string. rzSizeInBytes will contain the size of the string
	 * excluding the null terminator.
	 *
	 * @return True if rsString and rzSizeInBytes were modified, false otherwise.
	 *
	 * \warning Any mutations of this DataStore will invalidate the pointer returned
	 * in rsString. This includes calls to Set*(), CollectGarbage(), CompactHeap(),
	 * MakeArray(), MakeTable(), and ResizeArray(). As a rule, this method is provided for efficiency
	 * and needs to be used with extreme care (or only on DataStores that will never be
	 * mutated after they are initially populated).
	 */
	Bool AsString(const DataNode& value, Byte const*& rsString, UInt32& rzSizeInBytes) const
	{
		switch (value.GetType())
		{
		case DataNode::kString:
			{
				auto const h = value.GetHandle();
				if (!InternalIsValidHandle(h))
				{
					return false;
				}

				InternalGetStringData(h, rsString, rzSizeInBytes);
				return true;
			}
		case DataNode::kNull:
			rsString = "";
			rzSizeInBytes = 0u;
			return true;
		default:
			break;
		};

		return false;
	}

	/**
	 * Assign rsString with the value of value if it can be trivially
	 * converted to a string.
	 *
	 * @return True if rsString was modified, false otherwise.
	 */
	Bool AsString(const DataNode& value, String& rsString) const
	{
		switch (value.GetType())
		{
		case DataNode::kString:
			{
				Byte const* sString = nullptr;
				UInt32 zSizeInBytes = 0u;
				InternalGetStringData(value.GetHandle(), sString, zSizeInBytes);

				rsString.Assign(sString, zSizeInBytes);
				return true;
			}
		case DataNode::kNull:
			rsString.Clear();
			return true;
		default:
			break;
		};

		return false;
	}

	/**
	 * Assign rHString with the value of value if it can be trivially
	 * converted to a string.
	 *
	 * @return True if rHString was modified, false otherwise.
	 */
	Bool AsString(const DataNode& value, HString& rHString) const
	{
		Byte const* s = nullptr;
		UInt32 u = 0u;
		if (!AsString(value, s, u))
		{
			return false;
		}

		rHString = HString(s, u);
		return true;
	}

	/**
	 * @return The Int32 data in value. If value is not of type kInt32Big, the
	 * return value from this method is undefined.
	 */
	Int32 AssumeInt32Big(const DataNode& value) const
	{
		SEOUL_ASSERT(value.IsInt32Big());

		return InternalGetDataEntry(value.GetHandle()).AsInt32BigValue();
	}

	/**
	 * @return The Int32 data in value. If value is not of type kInt32Small, the
	 * return value from this method is undefined.
	 */
	Int32 AssumeInt32Small(const DataNode& value) const
	{
		return value.GetInt32Small();
	}

	/**
	 * Assign riValue with the value of value if it can be trivially
	 * converted to an Int32.
	 *
	 * @return True if riValue was modified, false otherwise.
	 */
	Bool AsInt32(const DataNode& value, Int32& riValue) const
	{
		switch (value.GetType())
		{
		case DataNode::kInt32Small:
			riValue = value.GetInt32Small();
			return true;
		case DataNode::kInt32Big:
			riValue = InternalGetDataEntry(value.GetHandle()).AsInt32BigValue();
			return true;
		default:
			break;
		};

		return false;
	}

	/**
	 * @return The Int64 data in value. If value is not of type kInt64, the
	 * return value from this method is undefined.
	 */
	Int64 AssumeInt64(const DataNode& value) const
	{
		SEOUL_ASSERT(value.IsInt64());

		return InternalGetDataEntry(value.GetHandle()).AsInt64Value();
	}

	/**
	 * Assign riValue with the value of value if it can be trivially
	 * converted to an Int64.
	 *
	 * @return True if riValue was modified, false otherwise.
	 */
	Bool AsInt64(const DataNode& value, Int64& riValue) const
	{
		switch (value.GetType())
		{
		case DataNode::kInt64:
			riValue = InternalGetDataEntry(value.GetHandle()).AsInt64Value();
			return true;
		case DataNode::kInt32Big:
			riValue = InternalGetDataEntry(value.GetHandle()).AsInt32BigValue();
			return true;
		case DataNode::kInt32Small:
			riValue = value.GetInt32Small();
			return true;
		case DataNode::kUInt32:
			riValue = (Int64)InternalGetDataEntry(value.GetHandle()).AsUInt32Value();
			return true;
		default:
			break;
		};

		return false;
	}

	/**
	 * @return The UInt32 data in value. If value is not of type kUInt32, the
	 * return value from this method is undefined.
	 */
	UInt32 AssumeUInt32(const DataNode& value) const
	{
		SEOUL_ASSERT(value.IsUInt32());

		return InternalGetDataEntry(value.GetHandle()).AsUInt32Value();
	}

	/**
	 * Assign ruValue with the value of value if it can be trivially
	 * converted to a UInt32.
	 *
	 * @return True if ruValue was modified, false otherwise.
	 */
	Bool AsUInt32(const DataNode& value, UInt32& ruValue) const
	{
		switch (value.GetType())
		{
		case DataNode::kUInt32:
			ruValue = InternalGetDataEntry(value.GetHandle()).AsUInt32Value();
			return true;
		case DataNode::kInt32Big:
			{
				Int32 const iValue = InternalGetDataEntry(value.GetHandle()).AsInt32BigValue();
				if (iValue >= 0)
				{
					ruValue = (UInt32)iValue;
					return true;
				}
			}
			break;
		case DataNode::kInt32Small:
			{
				Int32 const iValue = value.GetInt32Small();
				if (iValue >= 0)
				{
					ruValue = (UInt32)iValue;
					return true;
				}
			}
			break;
		default:
			break;
		};

		return false;
	}

	/**
	 * @return The Float32 data in value. If value is not of type kFloat31, the
	 * return value from this method is undefined.
	 */
	Float32 AssumeFloat31(const DataNode& value) const
	{
		return value.GetFloat31();
	}

	/**
	 * @return The Float32 data in value. If value is not of type kFloat32, the
	 * return value from this method is undefined.
	 */
	Float32 AssumeFloat32(const DataNode& value) const
	{
		SEOUL_ASSERT(value.IsFloat32());

		return InternalGetDataEntry(value.GetHandle()).AsFloat32Value();
	}

	/**
	 * Assign rfValue with the value of value if it can be
	 * converted to a Float32.
	 *
	 * @return True if rfValue was modified, false otherwise.
	 */
	Bool AsFloat32(const DataNode& value, Float32& rfValue) const
	{
		switch (value.GetType())
		{
		case DataNode::kFloat31:
			rfValue = value.GetFloat31();
			return true;

		case DataNode::kFloat32:
			rfValue = (Float32)InternalGetDataEntry(value.GetHandle()).AsFloat32Value();
			return true;

		case DataNode::kInt32Small:
			rfValue = (Float32)value.GetInt32Small();
			return true;

		case DataNode::kInt32Big:
			rfValue = (Float32)InternalGetDataEntry(value.GetHandle()).AsInt32BigValue();
			return true;

		case DataNode::kUInt32:
			rfValue = (Float32)InternalGetDataEntry(value.GetHandle()).AsUInt32Value();
			return true;

		case DataNode::kInt64:
			rfValue = (Float32)InternalGetDataEntry(value.GetHandle()).AsInt64Value();
			return true;

		case DataNode::kUInt64:
			rfValue = (Float32)InternalGetDataEntry(value.GetHandle()).AsUInt64Value();
			return true;

		default:
			break;
		};

		return false;
	}

	/**
	 * @return The Int64 data in value. If value is not of type kInt64, the
	 * return value from this method is undefined.
	 */
	UInt64 AssumeUInt64(const DataNode& value) const
	{
		return InternalGetDataEntry(value.GetHandle()).AsUInt64Value();
	}

	/**
	 * Assign ruValue with the value of value if it can be trivially
	 * converted to a UInt64.
	 *
	 * @return True if ruValue was modified, false otherwise.
	 */
	Bool AsUInt64(const DataNode& value, UInt64& ruValue) const
	{
		switch (value.GetType())
		{
		case DataNode::kUInt64:
			ruValue = InternalGetDataEntry(value.GetHandle()).AsUInt64Value();
			return true;
			break;
		case DataNode::kInt64:
			{
				Int64 const iValue = InternalGetDataEntry(value.GetHandle()).AsInt64Value();
				if (iValue >= 0)
				{
					ruValue = (UInt64)iValue;
					return true;
				}
			}
			break;
		case DataNode::kInt32Big:
			{
				Int32 const iValue = InternalGetDataEntry(value.GetHandle()).AsInt32BigValue();
				if (iValue >= 0)
				{
					ruValue = (UInt64)iValue;
					return true;
				}
			}
			break;
		case DataNode::kInt32Small:
			{
				Int32 const iValue = value.GetInt32Small();
				if (iValue >= 0)
				{
					ruValue = (UInt64)iValue;
					return true;
				}
			}
			break;
		case DataNode::kUInt32:
			ruValue = InternalGetDataEntry(value.GetHandle()).AsUInt32Value();
			return true;
		default:
			break;
		};

		return false;
	}

	/**
	 * Gets the value at index uIndex in array array, and sets it to
	 * rValue.
	 *
	 * @return True if the get was successful, false otherwise. If this method
	 * returns true, rValue will contain the value, otherwise it is left
	 * unchanged. This method can return false if:
	 * - array is not of type kArray
	 * - uIndex is outside the bounds of the array
	 */
	Bool GetValueFromArray(const DataNode& array, UInt32 uIndex, DataNode& rValue) const
	{
		using namespace DataStoreCommon;

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
		if (pContainer->GetCountExcludingNull() <= uIndex)
		{
			return false;
		}

		// Array is 1 passed the container header.
		rValue = ((DataNode*)(pContainer + 1u))[uIndex];
		return true;
	}

	/**
	 * Gets the value at key key in table table, and sets it to
	 * rValue.
	 *
	 * @return True if the get was successful, false otherwise. If this method
	 * returns true, rValue will contain the value, otherwise it is left
	 * unchanged. This method can return false if:
	 * - table is not of type kTable
	 * - key is not in the table.
	 */
	Bool GetValueFromTable(const DataNode& table, HString key, DataNode& rValue) const
	{
		using namespace DataStoreCommon;

		// Early out if not a table.
		if (!table.IsTable())
		{
			return false;
		}

		// Check for a valid DataStore handle.
		DataNode::Handle const handle = table.GetHandle();
		if (!InternalIsValidHandle(handle))
		{
			return false;
		}

		Container* pContainer = (Container*)InternalGetDataEntryPtr(handle);
		DataNode const* pValues = (DataNode const*)(pContainer + 1u);
		HString const* pKeys = (HString const*)(pValues + (pContainer->GetCapacityExcludingNull() + pContainer->GetHasNullStorage()));

		// Special case - nothing to do if no entries in the table.
		if (0 == (pContainer->GetCountExcludingNull() + pContainer->GetHasNull()))
		{
			return false;
		}

		HString const kNullKey;

		// Special nullptr key handling.
		if (key == kNullKey)
		{
			if (pContainer->GetHasNull())
			{
				rValue = pValues[pContainer->GetCapacityExcludingNull()];
				return true;
			}

			return false;
		}

		UInt32 const uHash = key.GetHash();
		UInt32 uIndex = uHash;

		while (true)
		{
			uIndex &= (pContainer->GetCapacityExcludingNull() - 1u);
			SEOUL_ASSERT(uIndex < pContainer->GetCapacityExcludingNull());

			// If we've found the desired key, assign the value and return true.
			HString const entryKey(pKeys[uIndex]);
			if (key == entryKey)
			{
				rValue = pValues[uIndex];
				return true;
			}
			// If we hit a nullptr key, the entry is not in the table.
			else if (entryKey == kNullKey)
			{
				return false;
			}

			++uIndex;
		}

		return false;
	}

	/**
	 * Tests if the given table node contains the given key
	 *
	 * @return True if the given data node is a table and contains the given
	 *         key, or false otherwise
	 */
	Bool TableContainsKey(const DataNode& table, HString key) const
	{
		DataNode ignoredValue;
		return GetValueFromTable(table, key, ignoredValue);
	}

	/**
	 * Attempts to the erase the value at index uIndex in
	 * the array referenced by array.
	 *
	 * @return True if the erase was successful, false otherwise. If this
	 * method returns true, the array in array will be 1 element smaller
	 * than prior to the call to this function, and all elements passed
	 * uIndex will be shift 1 element forward in the array. If this method
	 * returns false, it was due to:
	 * - array is not of type kArray
	 * - key is not in the table.
	 * - uIndex is outside the bounds of the array
	 */
	Bool EraseValueFromArray(const DataNode& array, UInt32 uIndex)
	{
		using namespace DataStoreCommon;

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

		// Cannot erase an index at or beyond the length of the array.
		if (uIndex >= pContainer->GetCountExcludingNull())
		{
			return false;
		}

		// The head of the array is immediately after the Container header.
		DataNode* pData = (DataNode*)(pContainer + 1u);

		// Erase the value by moving the block after it, if the value being erased is not the last value.
		if (uIndex + 1u != pContainer->GetCountExcludingNull())
		{
			memmove(pData + uIndex, pData + uIndex + 1u, sizeof(DataNode) * (pContainer->GetCountExcludingNull() - (uIndex + 1u)));
		}

		// Element has been removed.
		pContainer->SetCountExcludingNull(pContainer->GetCountExcludingNull() - 1);

		// Success.
		return true;
	}

	/**
	 * Attempts to the erase the value at key key in
	 * the table referenced by table.
	 *
	 * @return True if the erase was successful, false otherwise. If this
	 * method returns true, key will no longer exist in table (calls
	 * to GetValueFromTable() will return false for key). If this method
	 * returns false:
	 * - table is not of type kTable
	 * - key is not in the table.
	 */
	Bool EraseValueFromTable(const DataNode& table, HString key)
	{
		using namespace DataStoreCommon;

		// Early out if not a table.
		if (!table.IsTable())
		{
			return false;
		}

		// Check for a valid DataStore handle.
		DataNode::Handle handle = table.GetHandle();
		if (!InternalIsValidHandle(handle))
		{
			return false;
		}

		Container* pContainer = (Container*)InternalGetDataEntryPtr(handle);

		return InternalEraseTableValue(pContainer, key);
	}

	/**
	 * Set the element at index uIndex in array array to an array with
	 * an initial capacity of nInitialCapacity.
	 *
	 * @return True if the operation was successful, false otherwise. This method
	 * will return false:
	 * - if array is not an array
	 * - if uIndex is outside the bounds of the maximum array size
	 *
	 * If element uIndex is already an array, calling this operation will
	 * clear the existing array.
	 */
	Bool SetArrayToArray(const DataNode& array, UInt32 uIndex, UInt32 nInitialCapacity = 0u)
	{
		DataNode node;
		node.SetHandle(
			InternalCreateArray(nInitialCapacity),
			DataNode::kArray);

		return InternalSetArrayValue(array, uIndex, node);
	}

	/**
	 * Set the element at index uIndex in array array to a table with
	 * an initial capacity of nInitialCapacity.
	 *
	 * @return True if the operation was successful, false otherwise. This method
	 * will return false:
	 * - if array is not an array
	 * - if uIndex is outside the bounds of the maximum array size
	 *
	 * If element uIndex is already a table, calling this operation will
	 * clear the existing table.
	 */
	Bool SetTableToArray(const DataNode& array, UInt32 uIndex, UInt32 nInitialCapacity = 0u)
	{
		DataNode node;
		node.SetHandle(
			InternalCreateTable(nInitialCapacity),
			DataNode::kTable);

		return InternalSetArrayValue(array, uIndex, node);
	}

	/**
	 * Update the element at index uIndex in array array to the boolean
	 * value bValue
	 *
	 * @return True if the operation was successful, false otherwise. This method can
	 * return false:
	 * - if array is not an array.
	 * - if uIndex is outside the maximum bounds of an array.
	 */
	Bool SetBooleanValueToArray(const DataNode& array, UInt32 uIndex, Bool bValue)
	{
		DataNode value;
		value.SetBoolean(bValue);

		return InternalSetArrayValue(array, uIndex, value);
	}

	/**
	 * Update the element at index uIndex in array array to the file path
	 * value filePath
	 *
	 * @return True if the operation was successful, false otherwise. This method can
	 * return false:
	 * - if array is not an array.
	 * - if uIndex is outside the maximum bounds of an array.
	 */
	Bool SetFilePathToArray(const DataNode& array, UInt32 uIndex, FilePath filePath)
	{
		DataNode value;
		value.SetFilePath(filePath);

		return InternalSetArrayValue(array, uIndex, value);
	}

	/**
	 * Set a null DataNode value to array at index uIndex.
	 *
	 * @return True on success, false otherwise. If this method returns true,
	 * the array array will have a null value at uIndex, otherwise
	 * it will be left unchanged.
	 */
	Bool SetNullValueToArray(const DataNode& array, UInt32 uIndex)
	{
		return InternalSetArrayValue(array, uIndex, DataNode());
	}

	/**
	 * Update the element at index uIndex in array array to the UInt32
	 * value uValue
	 *
	 * @return True if the operation was successful, false otherwise. This method can
	 * return false:
	 * - if array is not an array.
	 * - if uIndex is outside the maximum bounds of an array.
	 */
	Bool SetUInt32ValueToArray(const DataNode& array, UInt32 uIndex, UInt32 uValue)
	{
		DataNode const value = InternalMakeUInt32DataNode(uValue);
		return InternalSetArrayValue(array, uIndex, value);
	}

	/**
	 * Update the element at index uIndex in array array to the Int32
	 * value iValue
	 *
	 * @return True if the operation was successful, false otherwise. This method can
	 * return false:
	 * - if array is not an array.
	 * - if uIndex is outside the maximum bounds of an array.
	 */
	Bool SetInt32ValueToArray(const DataNode& array, UInt32 uIndex, Int32 iValue)
	{
		DataNode const value = InternalMakeInt32DataNode(iValue);
		return InternalSetArrayValue(array, uIndex, value);
	}

	/**
	 * Update the element at index uIndex in array array to the Float32
	 * value fValue
	 *
	 * @return True if the operation was successful, false otherwise. This method can
	 * return false:
	 * - if array is not an array.
	 * - if uIndex is outside the maximum bounds of an array.
	 */
	Bool SetFloat32ValueToArray(const DataNode& array, UInt32 uIndex, Float32 fValue)
	{
		DataNode const value = InternalMakeFloat32DataNode(fValue);
		return InternalSetArrayValue(array, uIndex, value);
	}

	/**
	 * Update the element at index uIndex in array array to a "special erase"
	 * value.
	 *
	 * @return True if the operation was successul, false otherwise. This method can
	 * return false:
	 * - if array is not an array.
	 * - if uIndex is outside the maximum bounds of an array.
	 *
	 * A "special erase" only appears in "delta" DataStores generated with
	 * the ComputeDiff() function. It marks an entry as erased and is used
	 * when applying the patch DataStore.
	 */
	Bool SetSpecialEraseToArray(const DataNode& array, UInt32 uIndex)
	{
		DataNode node;
		node.SetSpecialErase();
		return InternalSetArrayValue(array, uIndex, node);
	}

	/**
	 * Update the element at index uIndex in array array to the string
	 * value value
	 *
	 * @return True if the operation was successful, false otherwise. This method can
	 * return false:
	 * - if array is not an array.
	 * - if uIndex is outside the maximum bounds of an array.
	 */
	Bool SetStringToArray(const DataNode& array, UInt32 uIndex, HString value)
	{
		return SetStringToArray(array, uIndex, value.CStr(), value.GetSizeInBytes());
	}

	/**
	 * Update the element at index uIndex in array array to the string
	 * value sValue
	 *
	 * @return True if the operation was successful, false otherwise. This method can
	 * return false:
	 * - if array is not an array.
	 * - if uIndex is outside the maximum bounds of an array.
	 */
	Bool SetStringToArray(const DataNode& array, UInt32 uIndex, const String& sValue)
	{
		return SetStringToArray(array, uIndex, sValue.CStr(), sValue.GetSize());
	}

	/**
	 * Update the element at index uIndex in array array to the string
	 * value sValue
	 *
	 * @return True if the operation was successful, false otherwise. This method can
	 * return false:
	 * - if array is not an array.
	 * - if uIndex is outside the maximum bounds of an array.
	 */
	Bool SetStringToArray(const DataNode& array, UInt32 uIndex, Byte const* sValue)
	{
		return SetStringToArray(array, uIndex, sValue, StrLen(sValue));
	}

	/**
	 * Update the element at index uIndex in array array to the string
	 * value sValue
	 *
	 * @return True if the operation was successful, false otherwise. This method can
	 * return false:
	 * - if array is not an array.
	 * - if uIndex is outside the maximum bounds of an array.
	 */
	Bool SetStringToArray(const DataNode& array, UInt32 uIndex, Byte const* sValue, UInt32 zStringLengthInBytes)
	{
		DataNode value;
		value.SetHandle(
			InternalCreateString(sValue, zStringLengthInBytes),
			DataNode::kString);

		return InternalSetArrayValue(array, uIndex, value);
	}

	/**
	 * Utility function - identical to SetStringToArray(), except it will parse the string
	 * for escape characters '\\' and remove them. For example, the pattern \\ will become \,
	 * \" will become ", etc.
	 *
	 * \pre zStringLengthInBytesAfterResolvingEscapes must be exactly equal to the length of the
	 * string in bytes that will result after resolving escapes in sValue, excluding the NULL terminator,
	 * or this method will SEOUL_FAIL().
	 *
	 * Due to the previous precondition, this method is intended for low-level parser use, to avoid
	 * heap and intermediate memory allocations for the escaped string, not general use.
	 *
	 * @return True on success, false otherwise. This method will succeed or fail in the
	 * same conditions as SetStringToArray().
	 */
	Bool UnescapeAndSetStringToArray(const DataNode& array, UInt32 uIndex, Byte const* sValue, UInt32 zStringLengthAfterResolvingEscapes)
	{
		DataNode value;
		value.SetHandle(
			InternalUnescapeAndCreateString(sValue, zStringLengthAfterResolvingEscapes),
			DataNode::kString);

		return InternalSetArrayValue(array, uIndex, value);
	}

	/**
	 * Update the element at index uIndex in array array to the Int64
	 * value iValue
	 *
	 * @return True if the operation was successful, false otherwise. This method can
	 * return false:
	 * - if array is not an array.
	 * - if uIndex is outside the maximum bounds of an array.
	 */
	Bool SetInt64ValueToArray(const DataNode& array, UInt32 uIndex, Int64 iValue)
	{
		DataNode const value = InternalMakeInt64DataNode(iValue);
		return InternalSetArrayValue(array, uIndex, value);
	}

	/**
	 * Update the element at index uIndex in array array to the UInt64
	 * value uValue
	 *
	 * @return True if the operation was successful, false otherwise. This method can
	 * return false:
	 * - if array is not an array.
	 * - if uIndex is outside the maximum bounds of an array.
	 */
	Bool SetUInt64ValueToArray(const DataNode& array, UInt32 uIndex, UInt64 uValue)
	{
		DataNode const value = InternalMakeUInt64DataNode(uValue);
		return InternalSetArrayValue(array, uIndex, value);
	}

	/**
	 * Set the element at key key in table table to an array with
	 * an initial capacity of nInitialCapacity.
	 *
	 * @return True if the operation was successful, false otherwise. This method
	 * will return false:
	 * - if table is not a table
	 *
	 * If element key is already an array, calling this operation will
	 * clear the existing array.
	 */
	Bool SetArrayToTable(const DataNode& table, HString key, UInt32 nInitialCapacity = 0u)
	{
		DataNode node;
		node.SetHandle(
			InternalCreateArray(nInitialCapacity),
			DataNode::kArray);

		return InternalSetTableValue(table, key, node);
	}

	/**
	 * Set the element at key key in table table to a table with
	 * an initial capacity of nInitialCapacity.
	 *
	 * @return True if the operation was successful, false otherwise. This method
	 * will return false:
	 * - if table is not a table
	 *
	 * If element key is already a table, calling this operation will
	 * clear the existing table.
	 */
	Bool SetTableToTable(const DataNode& table, HString key, UInt32 nInitialCapacity = 0u)
	{
		DataNode node;
		node.SetHandle(
			InternalCreateTable(nInitialCapacity),
			DataNode::kTable);

		return InternalSetTableValue(table, key, node);
	}

	/**
	 * Update the element at key key in table table to the boolean
	 * value bValue
	 *
	 * @return True if the operation was successful, false otherwise. This method can
	 * return false:
	 * - if table is not a table.
	 */
	Bool SetBooleanValueToTable(const DataNode& table, HString key, Bool bValue)
	{
		DataNode value;
		value.SetBoolean(bValue);

		return InternalSetTableValue(table, key, value);
	}

	/**
	 * Update the element at key key in table table to the file path
	 * value filePath
	 *
	 * @return True if the operation was successful, false otherwise. This method can
	 * return false:
	 * - if table is not a table.
	 */
	Bool SetFilePathToTable(const DataNode& table, HString key, FilePath filePath)
	{
		DataNode value;
		value.SetFilePath(filePath);

		return InternalSetTableValue(table, key, value);
	}

	/**
	 * Update the element at key key in table table to a "special erase"
	 * value.
	 *
	 * @return True if the operation was successul, false otherwise. This method can
	 * return false:
	 * - if table is not a table.
	 *
	 * A "special erase" only appears in "delta" DataStores generated with
	 * the ComputeDiff() function. It marks an entry as erased and is used
	 * when applying the patch DataStore.
	 */
	Bool SetSpecialEraseToTable(const DataNode& table, HString key)
	{
		DataNode node;
		node.SetSpecialErase();
		return InternalSetTableValue(table, key, node);
	}

	/**
	 * Update the element at key key in table table to the string
	 * value value
	 *
	 * @return True if the operation was successful, false otherwise. This method can
	 * return false:
	 * - if table is not a table.
	 */
	Bool SetStringToTable(const DataNode& table, HString key, HString value)
	{
		return SetStringToTable(table, key, value.CStr(), value.GetSizeInBytes());
	}

	/**
	 * Update the element at key key in table table to the string
	 * value sValue
	 *
	 * @return True if the operation was successful, false otherwise. This method can
	 * return false:
	 * - if table is not a table.
	 */
	Bool SetStringToTable(const DataNode& table, HString key, const String& sValue)
	{
		return SetStringToTable(table, key, sValue.CStr(), sValue.GetSize());
	}

	/**
	 * Update the element at key key in table table to the string
	 * value sValue
	 *
	 * @return True if the operation was successful, false otherwise. This method can
	 * return false:
	 * - if table is not a table.
	 */
	Bool SetStringToTable(const DataNode& table, HString key, Byte const* sValue)
	{
		return SetStringToTable(table, key, sValue, StrLen(sValue));
	}

	/**
	 * Update the element at key key in table table to the string
	 * value sValue
	 *
	 * @return True if the operation was successful, false otherwise. This method can
	 * return false:
	 * - if table is not a table.
	 */
	Bool SetStringToTable(const DataNode& table, HString key, Byte const* sValue, UInt32 zStringLengthInBytes)
	{
		DataNode value;
		value.SetHandle(
			InternalCreateString(sValue, zStringLengthInBytes),
			DataNode::kString);

		return InternalSetTableValue(table, key, value);
	}

	/**
	 * Utility function - identical to SetStringToTable(), except it will parse the string
	 * for escape characters '\\' and remove them. For example, the pattern \\ will become \,
	 * \" will become ", etc. Special patterns \n, \r, and \t will be converted into a newline,
	 * a carriage return, and a tab respectively.
	 *
	 * \pre zStringLengthInBytesAfterResolvingEscapes must be exactly equal to the length of the
	 * string in bytes that will result after resolving escapes in sValue, excluding the NULL terminator,
	 * or this method will SEOUL_FAIL().
	 *
	 * Due to the previous precondition, this method is intended for low-level parser use, to avoid
	 * heap and intermediate memory allocations for the escaped string, not general use.
	 *
	 * @return True on success, false otherwise. This method will succeed or fail in the
	 * same conditions as SetStringToTable().
	 */
	Bool UnescapeAndSetStringToTable(const DataNode& array, HString key, Byte const* sValue, UInt32 zStringLengthInBytesAfterResolvingEscapes)
	{
		DataNode value;
		value.SetHandle(
			InternalUnescapeAndCreateString(sValue, zStringLengthInBytesAfterResolvingEscapes),
			DataNode::kString);

		return InternalSetTableValue(array, key, value);
	}

	/**
	 * Set a null DataNode value to table at key key.
	 *
	 * @return True on success, false otherwise. If this method returns true,
	 * the table table will have a null value at uIndex, otherwise
	 * it will be left unchanged.
	 */
	Bool SetNullValueToTable(const DataNode& table, HString key)
	{
		return InternalSetTableValue(table, key, DataNode());
	}

	/**
	 * Update the element at key key in table table to the UInt32
	 * value uValue
	 *
	 * @return True if the operation was successful, false otherwise. This method can
	 * return false:
	 * - if table is not a table.
	 */
	Bool SetUInt32ValueToTable(const DataNode& table, HString key, UInt32 uValue)
	{
		DataNode const value = InternalMakeUInt32DataNode(uValue);
		return InternalSetTableValue(table, key, value);
	}

	/**
	 * Update the element at key key in table table to the Int32
	 * value iValue
	 *
	 * @return True if the operation was successful, false otherwise. This method can
	 * return false:
	 * - if table is not a table.
	 */
	Bool SetInt32ValueToTable(const DataNode& table, HString key, Int32 iValue)
	{
		DataNode const value = InternalMakeInt32DataNode(iValue);
		return InternalSetTableValue(table, key, value);
	}

	/**
	 * Update the element at key key in table table to the Float32
	 * value fValue
	 *
	 * @return True if the operation was successful, false otherwise. This method can
	 * return false:
	 * - if table is not a table.
	 */
	Bool SetFloat32ValueToTable(const DataNode& table, HString key, Float32 fValue)
	{
		DataNode const value = InternalMakeFloat32DataNode(fValue);
		return InternalSetTableValue(table, key, value);
	}

	/**
	 * Update the element at key key in table table to the Int64
	 * value iValue
	 *
	 * @return True if the operation was successful, false otherwise. This method can
	 * return false:
	 * - if table is not a table.
	 */
	Bool SetInt64ValueToTable(const DataNode& table, HString key, Int64 iValue)
	{
		DataNode const value = InternalMakeInt64DataNode(iValue);
		return InternalSetTableValue(table, key, value);
	}

	/**
	 * Update the element at key key in table table to the UInt64
	 * value uValue
	 *
	 * @return True if the operation was successful, false otherwise. This method can
	 * return false:
	 * - if table is not an table.
	 */
	Bool SetUInt64ValueToTable(const DataNode& table, HString key, UInt64 uValue)
	{
		DataNode const value = InternalMakeUInt64DataNode(uValue);
		return InternalSetTableValue(table, key, value);
	}

	/**
	 * Converts this DataStore into an exact clone of dataStore.
	 *
	 * "Exact" in this case means exact, including any wasted space
	 * in the DataStore heap. You should therefore call CollectGarbage() and
	 * CompactHeap() on the source DataStore if you want a compact heap before
	 * cloning it to this DataStore.
	 */
	void CopyFrom(const DataStore& dataStore)
	{
		m_vHandleDataOffsets = dataStore.m_vHandleDataOffsets;
		m_vData = dataStore.m_vData;

		m_uDataSizeAfterLastCollection = dataStore.m_uDataSizeAfterLastCollection;
		m_uNextHandle = dataStore.m_uNextHandle;
		m_uAllocatedHandles = dataStore.m_uAllocatedHandles;
		m_uSuppressGarbageCollection = dataStore.m_uSuppressGarbageCollection;
	}

	/**
	 * Swap the contents of this DataStore with the contents of rDataStore.
	 */
	void Swap(DataStore& rDataStore)
	{
		m_vHandleDataOffsets.Swap(rDataStore.m_vHandleDataOffsets);
		m_vData.Swap(rDataStore.m_vData);

		Seoul::Swap(m_uDataSizeAfterLastCollection, rDataStore.m_uDataSizeAfterLastCollection);
		Seoul::Swap(m_uNextHandle, rDataStore.m_uNextHandle);
		Seoul::Swap(m_uAllocatedHandles, rDataStore.m_uAllocatedHandles);
		Seoul::Swap(m_uSuppressGarbageCollection, rDataStore.m_uSuppressGarbageCollection);
	}

	/**
	 * Attempt to copy the tree defined by from to the tree defined by to.
	 *
	 * @param[in] fromNode Source node to copy from
	 * @param[in] to Source node to populate with a copy of from
	 * @param[in] bAllowOverwrite If true, existing keys in the hierarchy of to will be overwritten.
	 *
	 * \pre fromNode and to must be from this DataStore.
	 * \pre fromNode must be the same type as toNode
	 * \pre fromNode must be an array or table type.
	 */
	Bool DeepCopy(const DataNode& fromNode, const DataNode& to, Bool bAllowOverwrite = false)
	{
		return DeepCopy(*this, fromNode, to, bAllowOverwrite);
	}

	// Attempt to copy the tree defined by fromNode to the tree defined by to.
	//
	// Return true on success, false on failure. If this method returns false,
	// the tree of to is left in an incomplete/undefined state.
	Bool DeepCopy(
		const DataStore& fromDataStore,
		const DataNode& fromNode,
		const DataNode& to,
		Bool bAllowConflicts = false,
		Bool bOverwriteConflicts = true);

	// Similar to DeepCopy but handles leaf node values as well as interior nodes.
	Bool DeepCopyToArray(
		const DataStore& fromDataStore,
		const DataNode& fromNode,
		const DataNode& to,
		UInt32 uArrayIndex,
		Bool bAllowConflicts = false,
		Bool bOverwriteConflicts = true);
	Bool DeepCopyToTable(
		const DataStore& fromDataStore,
		const DataNode& fromNode,
		const DataNode& to,
		HString key,
		Bool bAllowConflicts = false,
		Bool bOverwriteConflicts = true);

	// Output this DataStore as a string.
	void ToString(
		const DataNode& value,
		String& rsOutput,
		Bool bMultiline = false,
		Int iIndentationLevel = 0,
		Bool bSortTableKeysAlphabetical = false) const
	{
		// Clear the output string.
		rsOutput.Clear();

		InternalToString(
			value,
			rsOutput,
			bMultiline,
			iIndentationLevel,
			bSortTableKeysAlphabetical);
	}

	// Load raw binary data from a file - expected to be endian correct for the current architecture.
	Bool Load(SyncFile& rFile);

	// Save raw binary data to a file, for the specified target platform.
	Bool Save(SyncFile& rFile, Platform ePlatform, Bool bCompact = true) const;

	// Verify the integrity of the DataStore. This validates that all internal handles of the DataStore
	// are valid, and all DataNode types are reasonable. Typically, you want to run this after Load()
	// on data loaded from untrustable sources (user storage or clients).
	Bool VerifyIntegrity() const;

	/** @return Total memory usage of this DataStore, in bytes. */
	UInt32 GetTotalMemoryUsageInBytes() const
	{
		return
			(UInt32)sizeof(*this) +
			m_vData.GetCapacityInBytes() +
			m_vHandleDataOffsets.GetCapacityInBytes();
	}

#if SEOUL_UNIT_TESTS
	// Unit test hook only, used to check if DataStore A and DataStore B are
	// byte-for-byte equal (memcmp on buffers and members).
	static Bool UnitTestHook_ByteForByteEqual(const DataStore& a, const DataStore& b);

	enum CorruptedDataType
	{
		kCorruptedArrayCapacity,
		kCorruptedArrayCount,
		kCorruptedArrayHandle,
		kCorruptedArrayOffset,
		kCorruptedStringCapacity,
		kCorruptedStringCount,
		kCorruptedStringHandle,
		kCorruptedStringOffset,
		kCorruptedTableCapacity,
		kCorruptedTableCount,
		kCorruptedTableHandle,
		kCorruptedTableOffset,
		kCorruptedTypeData,
		CORRUPTION_TYPES,
	};

	// Unit test hook only, fills the DataStore with intentionally corrupted data.
	void UnitTestHook_FillWithCorruptedData(CorruptedDataType eType);

	// Unit test hook only, calls InternalCompactHandleOffsets.
	void UnitTestHook_CallInternalCompactHandleOffsets()
	{
		InternalCompactHandleOffsets();
	}
#endif // /SEOUL_UNIT_TESTS

private:
	friend struct DataStoreTableIterator;

	void InternalToString(
		const DataNode& value,
		String& rsOutput,
		Bool bMultiline = false,
		Int iIndentationLevel = 0,
		Bool bSortTableKeysAlphabetical = false) const;

	Bool InternalVerifyIntegrity(const DataNode& node) const;

	typedef Vector<DataStoreCommon::HandleDataOffset, MemoryBudgets::DataStore> HandleDataOffsets;

	typedef Vector<DataStoreCommon::DataEntry, MemoryBudgets::DataStoreData> Data;

	// Sanity checking.
	static const UInt32 kuContainerSizeInDataEntries = (sizeof(DataStoreCommon::Container) / sizeof(DataStoreCommon::DataEntry));

	HandleDataOffsets m_vHandleDataOffsets;
	Data m_vData;

	UInt32 m_uDataSizeAfterLastCollection;
	UInt32 m_uNextHandle;
	UInt32 m_uAllocatedHandles;
	UInt32 m_uSuppressGarbageCollection;

	struct ScopedSuppress SEOUL_SEALED
	{
		ScopedSuppress(DataStore& r)
			: m_r(r)
		{
			++m_r.m_uSuppressGarbageCollection;
		}

		~ScopedSuppress()
		{
			SEOUL_ASSERT(m_r.m_uSuppressGarbageCollection > 0u);
			--m_r.m_uSuppressGarbageCollection;
		}

		DataStore& m_r;
	}; // struct ScopedSuppress

	/** @return True if a handle is valid, false otherwise. */
	Bool InternalIsValidHandle(DataNode::Handle h) const
	{
		using namespace DataStoreCommon;

		// Out of range.
		if (h.GetIndex() >= m_vHandleDataOffsets.GetSize())
		{
			return false;
		}

		// Valid and matching value.
		HandleDataOffset const offset = m_vHandleDataOffsets[h.GetIndex()];
		return (
			offset.IsValid() &&
			offset.GetGenerationId() == h.GetGenerationId() &&
			offset.GetDataOffset() < m_vData.GetSize());
	}

	const DataStoreCommon::DataEntry& InternalGetDataEntry(DataNode::Handle h) const
	{
		return m_vData[m_vHandleDataOffsets[h.GetIndex()].GetDataOffset()];
	}

	DataStoreCommon::DataEntry& InternalGetDataEntry(DataNode::Handle h)
	{
		return m_vData[m_vHandleDataOffsets[h.GetIndex()].GetDataOffset()];
	}

	DataStoreCommon::DataEntry const* InternalGetDataEntryPtr(DataNode::Handle h) const
	{
		return m_vData.Get(m_vHandleDataOffsets[h.GetIndex()].GetDataOffset());
	}

	DataStoreCommon::DataEntry* InternalGetDataEntryPtr(DataNode::Handle h)
	{
		return m_vData.Get(m_vHandleDataOffsets[h.GetIndex()].GetDataOffset());
	}

	/**
	 * Assumes pContainer is the header for string data, returns a pointer to the data
	 * in rsString and the size in bytes of the data (excluding nullptr terminator) in rzSizeInBytes
	 */
	void InternalGetStringData(DataNode::Handle handle, Byte const*& rsString, UInt32& rzSizeInBytes) const
	{
		using namespace DataStoreCommon;

		// A Container header will be located at the data's entry pointer.
		Container* pContainer = (Container*)InternalGetDataEntryPtr(handle);

		// The string data is immediately after the container header.
		rsString = (Byte const*)(pContainer + 1u);

		// The string size in bytes is equal to the container's count.
		rzSizeInBytes = pContainer->GetCountExcludingNull();
	}

	/**
	 * Utility shared between several functions, generates a DataNode with Float32 data,
	 * either kInt32Small, kInt32Big, or kFloat32, depending on the value of iValue.
	 */
	DataNode InternalMakeFloat32DataNode(Float32 fValue)
	{
		// TODO: Eliminate redundant IsNaN checks in this method.
		if (!IsNaN(fValue))
		{
			// To maintain type rules, if the Float32 value is an integer value that
			// is representable as a UInt64 or Int64, we store it as the appropriate
			// integer type. Otherwise, we store it as a Float32.
			if (fValue < 0.0f)
			{
				Int64 const iValue = (Int64)fValue;
				Float32 const fTestValue = (Float32)iValue;

				// Exactly representable, store as Int64 (or smaller type, depending on specific value).
				if (fTestValue == fValue)
				{
					return InternalMakeInt64DataNode(iValue);
				}
			}
			else
			{
				UInt64 const uValue = (UInt64)fValue;
				Float32 const fTestValue = (Float32)uValue;

				// Exactly representable, store as UInt64 (or smaller type, depending on specific value).
				if (fTestValue == fValue)
				{
					return InternalMakeUInt64DataNode(uValue);
				}
			}
		}

		union
		{
			Float32 fInValue;
			UInt32 uOutValue;
		};

		// Set the float value for conversion.
		fInValue = fValue;

		// Use a consistent canonical NaN value that fits with our masking scheme.
		// Failure to do this can result in a NaN turning into an Infinity, etc.
		if (IsNaN(fInValue))
		{
			uOutValue = kuDataNodeCanonicalNaNBits;
		}

		// Small value.
		DataNode ret;
		if ((uOutValue & 0x1) == 0)
		{
			ret.SetFloat31(fValue);
		}
		// Precise value.
		else
		{
			DataNode::Handle const hHandle = InternalAllocate(1u);
			InternalGetDataEntry(hHandle).AsFloat32Value() = fValue;
			ret.SetHandle(hHandle, DataNode::kFloat32);
		}

		return ret;
	}

	/**
	 * Utility shared between several functions, generates a DataNode with Int32 data,
	 * either kInt32Small or kInt32Big depending on the value of iValue.
	 */
	DataNode InternalMakeInt32DataNode(Int32 iValue)
	{
		DataNode ret;
		if (iValue >= kiDataNodeMinInt32SmallValue && iValue <= kiDataNodeMaxInt32SmallValue)
		{
			ret.SetInt32Small(iValue);
		}
		else
		{
			DataNode::Handle const hHandle = InternalAllocate(1u);
			InternalGetDataEntry(hHandle).AsInt32BigValue() = iValue;
			ret.SetHandle(hHandle, DataNode::kInt32Big);
		}

		return ret;
	}

	/**
	 * Utility shared between several functions, generates a DataNode with Int64 data,
	 * either kInt32Small, kInt32Big, kUInt32, or kInt64 depending on the value of iValue.
	 */
	DataNode InternalMakeInt64DataNode(Int64 iValue)
	{
		// To obey type rules, if the value is in the range of an Int32, store the
		// value as an Int32.
		if (iValue >= IntMin && iValue <= IntMax)
		{
			return InternalMakeInt32DataNode((Int32)iValue);
		}
		// Otherwise, if it is in the range of UInt32, store it as a UInt32.
		else if (iValue > IntMax && iValue <= UIntMax)
		{
			return InternalMakeUInt32DataNode((UInt32)iValue);
		}
		else
		{
			DataNode::Handle const hHandle = InternalAllocate(2u);
			InternalGetDataEntry(hHandle).SetInt64Value(iValue);

			DataNode ret;
			ret.SetHandle(hHandle, DataNode::kInt64);
			return ret;
		}
	}

	/**
	 * Utility shared between several functions, generates a DataNode with UInt32 data,
	 * either kInt32Small, kInt32Big, or kUInt32, depending on the value of uValue.
	 */
	DataNode InternalMakeUInt32DataNode(UInt32 uValue)
	{
		// To fulfill type rules, values in the range of an Int32 are always
		// stored as Int32. We check this based on whether the UInt32 cast to an Int32
		// results in a negative number or not.
		union
		{
			Int32 iCheckValue;
			UInt32 uInValue;
		};
		uInValue = uValue;

		// Positive number can be represented as an Int32, so use an Int32.
		if (iCheckValue >= 0)
		{
			return InternalMakeInt32DataNode((Int32)uValue);
		}
		// Otherwise, must use a UInt32.
		else
		{
			DataNode::Handle const hHandle = InternalAllocate(1u);
			InternalGetDataEntry(hHandle).AsUInt32Value() = uValue;

			DataNode ret;
			ret.SetHandle(hHandle, DataNode::kUInt32);
			return ret;
		}
	}

	/**
	 * Utility shared between several functions, generates a DataNode with UInt64 data,
	 * either kInt32Small, kInt32Big, kUInt32, kInt64, or kUInt64 depending on the value of uValue.
	 */
	DataNode InternalMakeUInt64DataNode(UInt64 uValue)
	{
		// If the value is <= the max value of an Int64, we can set it
		// as an Int64 (which will also handle other possibilities, such
		// as conversion to an Int32 or a UInt32.
		if (uValue <= (UInt64)Int64Max)
		{
			return InternalMakeInt64DataNode((Int64)uValue);
		}
		// Otherwise, must store as a UInt64.
		else
		{
			DataNode::Handle const hHandle = InternalAllocate(2u);
			InternalGetDataEntry(hHandle).SetUInt64Value(uValue);

			DataNode ret;
			ret.SetHandle(hHandle, DataNode::kUInt64);
			return ret;
		}
	}

	DataNode::Handle InternalAllocate(UInt32 nNumberOfDataEntries);
	UInt32 InternalAllocateRaw(UInt32 nNumberOfDataEntries);
	void InternalReallocate(UInt32 nOldNumberOfDataEntries, UInt32 nNewNumberOfDataEntries, DataNode::Handle handle);
	DataNode::Handle InternalAllocateHandle(UInt32 uDataOffset);
	void InternalClearHandles();
	DataNode::Handle InternalCreateArray(UInt32 uInitialCapacity);
	DataNode::Handle InternalCreateString(Byte const* sString, UInt32 zStringLengthInBytes);
	DataNode::Handle InternalUnescapeAndCreateString(Byte const* sString, UInt32 zStringLengthInBytesAfterResolvingEscapes);
	DataNode::Handle InternalCreateTable(UInt32 uInitialCapacity);

	void InternalCollectGarbage(Bool bCompactContainers = false);
	void InternalComputeMD5(MD5& r, const DataNode& node) const;
	void InternalCopyData(
		const DataNode& node,
		HandleDataOffsets& rvNewHandleOffsets,
		Data& rvNewData,
		Bool bCompactContainers) const;

	void InternalCompactHandleOffsets();
	void InternalCompactHandleOffsetsInner(
		DataNode& node,
		HashTable<UInt32, UInt32, MemoryBudgets::Saving>& rvHandleOffsetsMap);


	Bool InternalFilePathFromString(const DataNode& value, FilePath& rFilePath) const;
	Bool InternalSetArrayValue(const DataNode& array, UInt32 uIndex, const DataNode& value);
	Bool InternalSetTableValue(const DataNode& table, HString key, const DataNode& value);
	Bool InternalEraseTableValue(DataStoreCommon::Container* pContainer, HString key);
	Bool InternalSetTableValueInner(
		DataStoreCommon::Container* pContainer,
		HString key,
		const DataNode& value,
		UInt32 const uHash,
		UInt32 uIndex);

	/**
	 * @return Given a desired capacity, return the total size in DataNode entries
	 * required for the table (including Container header data).
	 */
	inline static UInt32 GetTableDataSize(UInt32 nCapacity)
	{
		using namespace DataStoreCommon;

		return (UInt32)RoundUpToAlignment(
			sizeof(Container) + // size of header in bytes
			(nCapacity * sizeof(DataNode)) + // size of values in bytes
			(nCapacity * sizeof(HString)), // size of keys in bytes
			sizeof(DataNode)) / sizeof(DataNode);
	}

	/**
	 * Increase the capacity of the table table.
	 */
	void InternalGrowTable(const DataNode& table, UInt32 uHandleIndex, UInt32 nNewCapacity)
	{
		using namespace DataStoreCommon;

		// Sanity checks.
		SEOUL_ASSERT(table.IsTable());

		// Get the existing offset and lookup the existing capacity.
		UInt32 uOldOffset = m_vHandleDataOffsets[uHandleIndex].GetDataOffset();
		UInt32 const nOldCapacity = m_vData[uOldOffset].AsContainer().GetCapacityExcludingNull();
		UInt32 const bHasNull = m_vData[uOldOffset].AsContainer().GetHasNull();

		HString const kNullKey;
		nNewCapacity = GetNextPowerOf2(nNewCapacity);

		if (nNewCapacity > nOldCapacity)
		{
			// Get a new location for the table data
			UInt32 uNewOffset = InternalAllocateRaw(GetTableDataSize(nNewCapacity + bHasNull));

			// Refresh offset after potential reallocation.
			uOldOffset = m_vHandleDataOffsets[uHandleIndex].GetDataOffset();

			// Get a pointer to the new header data.
			Container* pNewContainer = ((Container*)m_vData.Get(uNewOffset));

			// Get pointers to the old and new keys, and the old values.
			HString* pOldKeys = (HString*)(m_vData.Get(uOldOffset + kuContainerSizeInDataEntries + nOldCapacity + bHasNull));
			DataNode* pOldValues = (DataNode*)(m_vData.Get(uOldOffset + kuContainerSizeInDataEntries));
			HString* pNewKeys = (HString*)(m_vData.Get(uNewOffset + kuContainerSizeInDataEntries + nNewCapacity + bHasNull));

			// Initialize the key for the special nullptr member if present.
			if (bHasNull)
			{
				memset(pNewKeys + nNewCapacity, 0, sizeof(HString));
			}

			// Keys need to be initialized to nullptr, values are left
			// uninitialized until they are assigned.
			memset(pNewKeys, 0, nNewCapacity * sizeof(HString));

			// Update the new container members - we only have
			// nullptr storage if we have a nullptr value, otherwise
			// we use this as an opportunity to drop the storage.
			pNewContainer->SetHasNullStorage(bHasNull);
			pNewContainer->SetHasNull(false);

			// Empty out the new table and update the offset
			pNewContainer->SetCountExcludingNull(0u);
			pNewContainer->SetCapacityExcludingNull(nNewCapacity);
			m_vHandleDataOffsets[uHandleIndex].SetDataOffset(uNewOffset);

			// Insert the special nullptr key if defined.
			if (bHasNull)
			{
				InternalSetTableValue(table, pOldKeys[nOldCapacity], pOldValues[nOldCapacity]);
			}

			// Now insert all the old key/value pairs.
			for (UInt32 i = 0u; i < nOldCapacity; ++i)
			{
				if (kNullKey != pOldKeys[i])
				{
					InternalSetTableValue(table, pOldKeys[i], pOldValues[i]);
				}
			}
		}
	}

	void InternalSerializeAsString(String& rsOutput,Byte const* s,UInt32 zStringLengthInBytes) const;

	typedef HashTable<UInt32, HString, MemoryBudgets::DataStore> RemapForLoadTable;
	typedef HashTable<HString, UInt32, MemoryBudgets::DataStore> RemapForSaveTable;
	typedef Vector<Byte, MemoryBudgets::DataStore> SerializedStringTable;

	static void RemapForLoad(
		HStringData::InternalIndexType& ruRawHStringValue,
		RemapForLoadTable& tRemap);
	static void RemapForLoad(
		DataNode& rNode,
		const HandleDataOffsets& vHandleOffsets,
		Data& rvData,
		RemapForLoadTable& tFilePathRemap,
		RemapForLoadTable& tHStringRemap);
	static void RemapForSave(
		Platform ePlatform,
		HStringData::InternalIndexType& ruRawHStringValue,
		SerializedStringTable& rvSerializedTable,
		UInt32& ruSerializedStrings,
		RemapForSaveTable& tRemap,
		Bool bRelativeFilename);
	static void RemapForSave(
		Platform ePlatform,
		DataNode& rNode,
		const HandleDataOffsets& vHandleOffsets,
		Data& rvData,
		SerializedStringTable& rvSerializedFilePathTable,
		UInt32& ruSerializedFilePathTableStrings,
		RemapForSaveTable& tFilePathRemap,
		SerializedStringTable& rvSerializedHStringTable,
		UInt32& ruSerializedHStringTableStrings,
		RemapForSaveTable& tHStringRemap);

	SEOUL_REFERENCE_COUNTED(DataStore);
	SEOUL_DISABLE_COPY(DataStore);
}; // class DataStore

/**
 * @return The HString in the key portion of this DataStoreTableIterators table at uIndex, or
 * the empty HString() if this is an invalid iterator.
 */
inline HString DataStoreTableIterator::Key(UInt32 uIndex) const
{
	using namespace DataStoreCommon;

	if (nullptr == m_pOwner)
	{
		return HString();
	}
	else
	{
		Container* pContainer = (Container*)m_pOwner->InternalGetDataEntryPtr(m_Table.GetHandle());
		DataNode const* pValues = (DataNode const*)(pContainer + 1u);
		HString const* pKeys = (HString const*)(pValues + pContainer->GetCapacityExcludingNull() + pContainer->GetHasNullStorage());
		return pKeys[uIndex];
	}
}

/**
 * @return The DataNode in the value portion of this DataStoreTableIterators table at uIndex, or
 * the null DataNode() if this is an invalid iterator.
 */
inline DataNode DataStoreTableIterator::Value(UInt32 uIndex) const
{
	using namespace DataStoreCommon;

	if (nullptr == m_pOwner)
	{
		return DataNode();
	}
	else
	{
		Container* pContainer = (Container*)m_pOwner->InternalGetDataEntryPtr(m_Table.GetHandle());
		DataNode const* pValues = (DataNode const*)(pContainer + 1u);
		return pValues[uIndex];
	}
}

/**
 * Patch equivalent to ComputeDiff().
 */
Bool ApplyDiff(const DataStore& diff, DataStore& rTarget);

/**
 * Diffing utility - generates a DataStore rDiff that when
 * applied to a DataStore a, generates a DataStore b.
 */
Bool ComputeDiff(const DataStore& a, const DataStore& b, DataStore& rDiff);

} // namespace Seoul

#endif // include guard
