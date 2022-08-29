/**
 * \file DataStoreParser.h
 * \brief DataStoreParser populates a DataStore from a text based file
 * (JSON format, using RapidJSON) or binary file (cooked DataStore).
 * Detection between the two is done automatically.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef DATA_STORE_PARSER_H
#define DATA_STORE_PARSER_H

#include "Delegate.h"
#include "FilePath.h"
#include "HashTable.h"
#include "Prereqs.h"
#include "SeoulString.h"
#include "Vector.h"
namespace Seoul { class DataNode; }
namespace Seoul { class DataStore; }
namespace Seoul { class DataStoreSchemaCache; }

namespace Seoul
{

/**
 * SeoulEngine custom signature, used to identify cooked binary versions of our
 * DataStore format.
 */
static const UByte kaCookedDataStoreBinarySignatureVersion0[8] =
{
	255u /* 0xFF */,
	255u /* 0xFF */,
	0u   /* 0x00 */,
	222u /* 0xDE */,
	167u /* 0xA7 */,
	127u /* 0x7F */,
	0u   /* 0x00 */,
	221u /* 0xDD */,
};

/**
 * SeoulEngine custom signature, used to identify cooked binary versions of our
 * DataStore format.
 */
static const UByte kaCookedDataStoreBinarySignature[8] =
{
	0xEB,
	0x4E,
	0x6D,
	0xBA,
	0xBD,
	0x66,
	0xD1,
	0xEC,
};

/** Current version of our cooked DataStore binary format. */
static const UInt32 kuCookedDataStoreBinaryVersion = 2u;

namespace DataStoreParserFlags
{
	enum Enum
	{
		/** Default, no flags. */
		kNone = 0,

		/** If set, parser errors will be sent to the log. */
		kLogParseErrors = (1 << 0),

		/**
		 * Workaround for some .json data we do not control - duplicate keys in a table are allowed by all but the
		 * last key are ignored.
		 */
		kAllowDuplicateTableKeys = (1 << 1),

		/**
		 * When set, null values are instead interpreted as "special erase" values. Useful for
		 * JSON data and DataStores that are delta patches.
		 */
		kNullAsSpecialErase = (1 << 2),

		/**
		 * Useful if a DataStore parse is being used as part of tools and you 
		 * want to leave FilePaths unnormalized. Must *not* be used if a DataStore is intended
		 * to be used at runtime, since AsFilePath() and other API will fail if the values
		 * are left as strings.
		 */
		kLeaveFilePathAsString = (1 << 3),
	};
} // namespace DataStoreParserFlags

class DataStoreParser SEOUL_SEALED
{
public:
	// Return true if the currently set stream is a cooked binary version of
	// a SeoulEngine text file, false otherwise.
	static Bool IsCookedBinary(Byte const* pData, UInt32 uDataSizeInBytes);

	static DataStoreSchemaCache* CreateSchemaCache(FilePath schemaLookup);
	static void DestroySchemaCache(DataStoreSchemaCache*& rp);

	static Bool FromFile(DataStoreSchemaCache* pCache, FilePath filePath, DataStore& rDataStore, UInt32 uFlags = 0u);
	static Bool FromString(DataStoreSchemaCache* pCache, Byte const* s, UInt32 uSizeInBytes, DataStore& rDataStore, UInt32 uFlags = 0u, FilePath filePath = FilePath());
	static Bool FromString(DataStoreSchemaCache* pCache, const String& s, DataStore& rDataStore, UInt32 uFlags = 0u, FilePath filePath = FilePath());

	static inline Bool FromFile(FilePath filePath, DataStore& rDataStore, UInt32 uFlags = 0u)
	{
		return FromFile(nullptr, filePath, rDataStore, uFlags);
	}

	static inline Bool FromString(Byte const* s, UInt32 uSizeInBytes, DataStore& rDataStore, UInt32 uFlags = 0u, FilePath filePath = FilePath())
	{
		return FromString(nullptr, s, uSizeInBytes, rDataStore, uFlags, filePath);
	}

	static inline Bool FromString(const String& s, DataStore& rDataStore, UInt32 uFlags = 0u, FilePath filePath = FilePath())
	{
		return FromString(nullptr, s, rDataStore, uFlags, filePath);
	}

	static Bool StringAsFilePath(Byte const* s, UInt32 uSizeInBytes, FilePath& rFilePath);
	static inline Bool StringAsFilePath(Byte const* s, FilePath& rFilePath)
	{
		return StringAsFilePath(s, StrLen(s), rFilePath);
	}
	static inline Bool StringAsFilePath(const String& s, FilePath& rFilePath)
	{
		return StringAsFilePath(s.CStr(), s.GetSize(), rFilePath);
	}

	// Check if a DataStore contains a commands format body.
	static Bool IsJsonCommandFile(const DataStore& ds);

	// Resolve a commands format JSON file. See documentation in DataStoreParser.cpp
	// for more information on the structure of a "commands format" DataStore.
	typedef Delegate< SharedPtr<DataStore>(const String& sFileName, Bool bResolveCommands) > Resolver;
	static Bool ResolveCommandFile(
		const Resolver& includeResolver,
		const String& sBaseFilename,
		const DataStore& cmdStore,
		DataStore& rResolved,
		UInt32 uFlags = 0u);

	// Similar to ResolveCommandFile, but "merges" into rResolved. As a result,
	// rResolved *must not* be the same DataStore as cmdStore.
	static Bool ResolveCommandFileInPlace(
		const Resolver& includeResolver,
		const String& sBaseFilename,
		const DataStore& cmdStore,
		DataStore& rResolved,
		DataNode& target,
		UInt32 uFlags = 0u);

private:
	/** DataStoreParser is a static class. */
	DataStoreParser();
	DataStoreParser(const DataStoreParser&);
	DataStoreParser& operator=(const DataStoreParser&);
}; // class DataStoreParser

/**
 * Utility structure that contains hinting for pretty printing
 * a DataStore, including:
 * - original (source) order of table entries
 * - comments attached to data nodes.
 */
class DataStoreHint SEOUL_ABSTRACT
{
public:
	enum class Type
	{
		kArray,
		kLeaf,
		kNone,
		kTable,
	};

	virtual ~DataStoreHint()
	{
	}

	virtual void Append(HString tableKey, const SharedPtr<DataStoreHint>& p) { /* Nop */ }
	virtual void Erase(const SharedPtr<DataStoreHint>& p) { /* Nop */ }
	virtual void Set(const SharedPtr<DataStoreHint>& pKey, const SharedPtr<DataStoreHint>& pValue) { /* Nop */ }

	virtual Bool AsString(HString& r) const { return false; }
	virtual Bool AsUInt64(UInt64& r) const { return false; }

	virtual SharedPtr<DataStoreHint> operator[](const SharedPtr<DataStoreHint>& p) const = 0;
	virtual SharedPtr<DataStoreHint> operator[](HString h) const = 0;
	virtual SharedPtr<DataStoreHint> operator[](UInt32 u) const = 0;

	virtual void Finish(Bool bRoot = false) {}
	virtual UInt32 GetNextContainerOrder() const = 0;
	virtual Bool IndexFromHash(UInt32 uHash, UInt32& ruInOutIndex) const { return false; }

	/** Start pointer of an associated comment - nullptr if no comment. */
	Byte const* GetCommentBegin() const
	{
		return m_sCommentBegin;
	}

	/** End pointer of an associated comment - nullptr if no comment. */
	Byte const* GetCommentEnd() const
	{
		return m_sCommentEnd;
	}

	/** Used to disambiguate hints on array elements from modified bodies. */
	UInt32 GetHash() const
	{
		return m_uHash;
	}

	/** Original order (from lowest to highest) of a node if in a table container. */
	UInt32 GetOrder() const
	{
		return m_uOrder;
	}

	// Convenience for type of node.
	Bool IsArray() const { return Type::kArray == m_eType; }
	Bool IsLeaf() const { return Type::kLeaf == m_eType; }
	Bool IsNone() const { return Type::kNone == m_eType; }
	Bool IsTable() const { return Type::kTable == m_eType; }

	/** Update the comment associated with this node. */
	void SetComment(Byte const* sBegin, Byte const* sEnd)
	{
		m_sCommentBegin = sBegin;
		m_sCommentEnd = sEnd;
	}

	/** Update the order associated with this node. */
	void SetOrder(UInt32 uOrder)
	{
		m_uOrder = uOrder;
	}

protected:
	DataStoreHint(Type eType)
		: m_eType(eType)
		, m_sCommentBegin(nullptr)
		, m_sCommentEnd(nullptr)
		, m_uOrder(0u)
		, m_uHash(0u)
	{
	}

	Type const m_eType;
	Byte const* m_sCommentBegin;
	Byte const* m_sCommentEnd;
	UInt32 m_uOrder;
	UInt32 m_uHash;

	SEOUL_REFERENCE_COUNTED(DataStoreHint);

private:
	SEOUL_DISABLE_COPY(DataStoreHint);
}; // class DataStoreHint

/** Node in hinting data that represents a JSON array. */
class DataStoreHintArray SEOUL_SEALED : public DataStoreHint
{
public:
	DataStoreHintArray()
		: DataStoreHint(DataStoreHint::Type::kArray)
	{
	}

	typedef Vector< SharedPtr<DataStoreHint>, MemoryBudgets::DataStore > Array;
	Array m_a;
	typedef HashTable< UInt32, Int64, MemoryBudgets::DataStore > ByHash;
	ByHash m_tByHash;

	virtual void Append(HString tableKey, const SharedPtr<DataStoreHint>& p) SEOUL_OVERRIDE;
	virtual void Erase(const SharedPtr<DataStoreHint>& p) SEOUL_OVERRIDE;
	virtual void Set(const SharedPtr<DataStoreHint>& pKey, const SharedPtr<DataStoreHint>& pValue) SEOUL_OVERRIDE;

	virtual SharedPtr<DataStoreHint> operator[](const SharedPtr<DataStoreHint>& p) const SEOUL_OVERRIDE;
	virtual SharedPtr<DataStoreHint> operator[](HString h) const SEOUL_OVERRIDE;
	virtual SharedPtr<DataStoreHint> operator[](UInt32 u) const SEOUL_OVERRIDE;

	virtual void Finish(Bool bRoot = false) SEOUL_OVERRIDE;
	virtual UInt32 GetNextContainerOrder() const SEOUL_OVERRIDE { return m_a.GetSize(); }
	virtual Bool IndexFromHash(UInt32 uHash, UInt32& ruInOutIndex) const SEOUL_OVERRIDE;

private:
	SEOUL_DISABLE_COPY(DataStoreHintArray);
	SEOUL_REFERENCE_COUNTED_SUBCLASS(DataStoreHintArray);
}; // class DataStoreHintArray

/** Node in hinting data that represents any scalar element (not a container). */
class DataStoreHintLeaf SEOUL_SEALED : public DataStoreHint
{
public:
	enum class ValueType : Int32
	{
		kNull,
		kBool,
		kDouble,
		kInt64,
		kStringHeap,
		kStringRef,
		kUInt64,
	};

	explicit DataStoreHintLeaf(UInt32 uHash);
	DataStoreHintLeaf(Bool b, UInt32 uHash);
	DataStoreHintLeaf(Double f, UInt32 uHash);
	DataStoreHintLeaf(Int64 i, UInt32 uHash);
	DataStoreHintLeaf(Byte const* s, UInt32 u, Bool bCopy, UInt32 uHash);
	DataStoreHintLeaf(UInt64 u, UInt32 uHash);
	~DataStoreHintLeaf();

	virtual Bool AsString(HString& r) const SEOUL_OVERRIDE;
	virtual Bool AsUInt64(UInt64& r) const SEOUL_OVERRIDE;

	virtual SharedPtr<DataStoreHint> operator[](const SharedPtr<DataStoreHint>& p) const SEOUL_OVERRIDE;
	virtual SharedPtr<DataStoreHint> operator[](HString h) const SEOUL_OVERRIDE;
	virtual SharedPtr<DataStoreHint> operator[](UInt32 u) const SEOUL_OVERRIDE;

	virtual UInt32 GetNextContainerOrder() const SEOUL_OVERRIDE { return 0u; }

private:
	SEOUL_DISABLE_COPY(DataStoreHintLeaf);
	SEOUL_REFERENCE_COUNTED_SUBCLASS(DataStoreHintLeaf);

	union
	{
		Bool b;
		Double f;
		Int64 i;
		UInt64 u;
		Byte const* s;
	} m_v{};
	UInt32 m_uStringSize{};
	ValueType m_eValueType{};
}; // class DataStoreHintLeaf

/** Placeholder for no-node - null pattern. */
class DataStoreHintNone SEOUL_SEALED : public DataStoreHint
{
public:
	DataStoreHintNone()
		: DataStoreHint(DataStoreHint::Type::kNone)
	{
	}

	virtual SharedPtr<DataStoreHint> operator[](const SharedPtr<DataStoreHint>& p) const SEOUL_OVERRIDE;
	virtual SharedPtr<DataStoreHint> operator[](HString h) const SEOUL_OVERRIDE;
	virtual SharedPtr<DataStoreHint> operator[](UInt32 u) const SEOUL_OVERRIDE;

	virtual UInt32 GetNextContainerOrder() const SEOUL_OVERRIDE { return 0u; }

private:
	SEOUL_DISABLE_COPY(DataStoreHintNone);
	SEOUL_REFERENCE_COUNTED_SUBCLASS(DataStoreHintNone);
}; // class DataStoreHintNone

/** Node in hinting data that represents a JSON table. */
class DataStoreHintTable SEOUL_SEALED : public DataStoreHint
{
public:
	DataStoreHintTable()
		: DataStoreHint(DataStoreHint::Type::kTable)
		, m_uNextOrder(0u)
	{
	}

	typedef HashTable< HString, SharedPtr<DataStoreHint>, MemoryBudgets::DataStore > Table;
	Table m_t;

	virtual void Append(HString tableKey, const SharedPtr<DataStoreHint>& p) SEOUL_OVERRIDE;
	virtual void Erase(const SharedPtr<DataStoreHint>& p) SEOUL_OVERRIDE;
	virtual void Set(const SharedPtr<DataStoreHint>& pKey, const SharedPtr<DataStoreHint>& pValue) SEOUL_OVERRIDE;

	virtual SharedPtr<DataStoreHint> operator[](const SharedPtr<DataStoreHint>& p) const SEOUL_OVERRIDE;
	virtual SharedPtr<DataStoreHint> operator[](HString h) const SEOUL_OVERRIDE;
	virtual SharedPtr<DataStoreHint> operator[](UInt32 u) const SEOUL_OVERRIDE;

	virtual void Finish(Bool bRoot = false) SEOUL_OVERRIDE;
	virtual UInt32 GetNextContainerOrder() const SEOUL_OVERRIDE { return m_uNextOrder; }

private:
	UInt32 m_uNextOrder;

	SEOUL_DISABLE_COPY(DataStoreHintTable);
	SEOUL_REFERENCE_COUNTED_SUBCLASS(DataStoreHintTable);
}; // class DataStoreHintTable

/**
 * Static class that encapsulates utilities for pretty printing a DataStore
 * to JSON. Unlike DataStore::ToString(), this functionality can:
 * - maintain table order and comments by using hint data parsed with
 *   ParseHintsNoCopy()
 * - apply more advance formatting rules, including maximum line length.
 */
class DataStorePrinter SEOUL_SEALED
{
public:
	// Populate rp with a hint structure for the given JSON data.
	//
	// IMPORTANT: This function does *not* copy s but instead references
	// s directly from rp. As a result, s must be in scope longer than rp is
	// in scope.
	static Bool ParseHintsNoCopy(Byte const* s, UInt32 u, SharedPtr<DataStoreHint>& rp);

	// Equivalent to ParseHintsNoCopy, except when run on a file in JSON "commands" format,
	// the file will be evaluated as if flattened (as if the commands have been evaluted).
	//
	// ["$include", ...] commands  are not supported and will cause the parse to fail,
	// so you should check for these commands before calling this function.
	static Bool ParseHintsNoCopyWithFlattening(Byte const* s, UInt32 u, SharedPtr<DataStoreHint>& rp);

	static void PrintWithHints(
		const DataStore& ds,
		const DataNode& root,
		const SharedPtr<DataStoreHint>& pHint,
		String& rs);
	static void PrintWithHints(
		const DataStore& ds,
		const SharedPtr<DataStoreHint>& pHint,
		String& rs);

private:
	static void InternalPrintWithHints(
		const DataStore& ds,
		const DataNode& value,
		const SharedPtr<DataStoreHint>& pHint,
		Int32 iIndentationLevel,
		String& rsOutput);

	/** DataStorePrinter is a static class. */
	DataStorePrinter();
	DataStorePrinter(const DataStorePrinter&);
	DataStorePrinter& operator=(const DataStorePrinter&);
}; // class DataStorePrinter

} // namespace Seoul

#endif // include guard
