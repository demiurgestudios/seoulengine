/**
 * \file DataStoreParser.cpp
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

// Optimizations are enabled in Debug in DataStoreParser.cpp since this
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
#include "FileManager.h"
#include "HashTable.h"
#include "Lexer.h"
#include "Logger.h"
#include "Mutex.h"
#include "Path.h"
#include "SeoulFile.h"
#include "SeoulRegex.h"
#include "SeoulWildcard.h"
#include "StackOrHeapArray.h"

#include <rapidjson/error/en.h>
#include <rapidjson/reader.h>
#include <rapidjson/schema.h>
#include <rapidjson/stringbuffer.h>

namespace rapidjson
{

class SeoulRapidJsonAllocator SEOUL_SEALED
{
public:
	static const Seoul::Bool kNeedFree = true;

	SeoulRapidJsonAllocator()
	{
	}

	void* Malloc(size_t zSizeInBytes) const
	{
		return Seoul::MemoryManager::Allocate(zSizeInBytes, Seoul::MemoryBudgets::DataStore);
	}

	void* Realloc(void* pAddressToReallocate, size_t zOldSize, size_t zNewSize) const
	{
		return Seoul::MemoryManager::Reallocate(pAddressToReallocate, zNewSize, Seoul::MemoryBudgets::DataStore);
	}

	static void Free(void* pAddressToFree)
	{
		return Seoul::MemoryManager::Deallocate(pAddressToFree);
	}

private:
	SEOUL_DISABLE_COPY(SeoulRapidJsonAllocator);
}; // class SeoulRapidJsonAllocator
static SeoulRapidJsonAllocator s_Allocator;

enum class HandlerAdditionalErrorType
{
	kNone,

	/** Raised when a key in a table already exists. */
	kDuplicateTableKey,
};

class PlaceholderHandler SEOUL_SEALED
{
public:
	PlaceholderHandler()
	{
	}

	/** @return any additional error type from the parse. */
	HandlerAdditionalErrorType GetAdditionalErrorType() const
	{
		return HandlerAdditionalErrorType::kNone;
	}

	/** @return the last set table key, or the empty HString. */
	Seoul::HString GetTableKey() const
	{
		return Seoul::HString();
	}

private:
	SEOUL_DISABLE_COPY(PlaceholderHandler);
}; // class PlaceholderHandler

#if !SEOUL_SHIP
#define SEOUL_DUP_CHECK() \
	if (0u == (Seoul::DataStoreParserFlags::kAllowDuplicateTableKeys & m_uFlags)) \
	{ \
		if (m_Top.IsArray()) \
		{ \
			Seoul::DataNode unusedExistingNode; \
			if (m_r.GetValueFromArray(m_Top, m_uArrayIndex, unusedExistingNode)) \
			{ \
				m_eAdditionalErrorType = HandlerAdditionalErrorType::kDuplicateTableKey; \
				 return false; \
			} \
		} \
		else \
		{ \
			Seoul::DataNode unusedExistingNode; \
			if (m_r.GetValueFromTable(m_Top, m_TableKey, unusedExistingNode)) \
			{ \
				m_eAdditionalErrorType = HandlerAdditionalErrorType::kDuplicateTableKey; \
				 return false; \
			} \
		} \
	}
#else
#define SEOUL_DUP_CHECK() ((void)0)
#endif

class SeoulRapidJsonDataStoreHandler SEOUL_SEALED
{
public:
	static const Seoul::UInt32 kuInitialStack = 16u;

	SeoulRapidJsonDataStoreHandler(Seoul::DataStore& r, Seoul::UInt32 uFlags)
		: m_r(r)
		, m_vStack()
		, m_Top()
		, m_uArrayIndex(0u)
		, m_TableKey()
		, m_eAdditionalErrorType(HandlerAdditionalErrorType::kNone)
		, m_uFlags(uFlags)
	{
		m_vStack.Reserve(kuInitialStack);
	}

	Seoul::Bool Null()
	{
		SEOUL_DUP_CHECK();

		// Special erase mode
		if (Seoul::DataStoreParserFlags::kNullAsSpecialErase == (Seoul::DataStoreParserFlags::kNullAsSpecialErase & m_uFlags))
		{
			if (m_Top.IsArray())
			{
				return m_r.SetSpecialEraseToArray(m_Top, m_uArrayIndex++);
			}
			else
			{
				return m_r.SetSpecialEraseToTable(m_Top, m_TableKey);
			}
		}
		else
		{
			if (m_Top.IsArray())
			{
				return m_r.SetNullValueToArray(m_Top, m_uArrayIndex++);
			}
			else
			{
				return m_r.SetNullValueToTable(m_Top, m_TableKey);
			}
		}
	}

	Seoul::Bool Bool(Seoul::Bool b)
	{
		SEOUL_DUP_CHECK();

		if (m_Top.IsArray())
		{
			return m_r.SetBooleanValueToArray(m_Top, m_uArrayIndex++, b);
		}
		else
		{
			return m_r.SetBooleanValueToTable(m_Top, m_TableKey, b);
		}
	}

	void Comment(size_t zBegin, size_t zEnd) {}

	Seoul::Bool Int(Seoul::Int32 i)
	{
		SEOUL_DUP_CHECK();

		if (m_Top.IsArray())
		{
			return m_r.SetInt32ValueToArray(m_Top, m_uArrayIndex++, i);
		}
		else
		{
			return m_r.SetInt32ValueToTable(m_Top, m_TableKey, i);
		}
	}

	Seoul::Bool Uint(Seoul::UInt32 u)
	{
		SEOUL_DUP_CHECK();

		if (m_Top.IsArray())
		{
			return m_r.SetUInt32ValueToArray(m_Top, m_uArrayIndex++, u);
		}
		else
		{
			return m_r.SetUInt32ValueToTable(m_Top, m_TableKey, u);
		}
	}

	Seoul::Bool Int64(Seoul::Int64 i)
	{
		SEOUL_DUP_CHECK();

		if (m_Top.IsArray())
		{
			return m_r.SetInt64ValueToArray(m_Top, m_uArrayIndex++, i);
		}
		else
		{
			return m_r.SetInt64ValueToTable(m_Top, m_TableKey, i);
		}
	}

	Seoul::Bool Uint64(Seoul::UInt64 u)
	{
		SEOUL_DUP_CHECK();

		if (m_Top.IsArray())
		{
			return m_r.SetUInt64ValueToArray(m_Top, m_uArrayIndex++, u);
		}
		else
		{
			return m_r.SetUInt64ValueToTable(m_Top, m_TableKey, u);
		}
	}

	Seoul::Bool Double(Seoul::Double d)
	{
		SEOUL_DUP_CHECK();

		if (m_Top.IsArray())
		{
			return m_r.SetFloat32ValueToArray(m_Top, m_uArrayIndex++, (Seoul::Float32)d);
		}
		else
		{
			return m_r.SetFloat32ValueToTable(m_Top, m_TableKey, (Seoul::Float32)d);
		}
	}

	Seoul::Bool FilePath(Seoul::FilePath filePath)
	{
		SEOUL_DUP_CHECK();

		if (m_Top.IsArray())
		{
			return m_r.SetFilePathToArray(m_Top, m_uArrayIndex++, filePath);
		}
		else
		{
			return m_r.SetFilePathToTable(m_Top, m_TableKey, filePath);
		}
	}

	Seoul::Bool RawNumber(Seoul::Byte const* s, Seoul::UInt32 uSizeInBytes, Seoul::Bool bCopy)
	{
		// No supported - our parser flags should mean this is never called.
		SEOUL_FAIL("Unexpected invocation of RawNumber.");
		return false;
	}

	Seoul::Bool String(Seoul::Byte const* s, Seoul::UInt32 uSizeInBytes, Seoul::Bool bCopy)
	{
		SEOUL_DUP_CHECK();

		// Try to specialize as a FilePath.
		if (0 == (m_uFlags & Seoul::DataStoreParserFlags::kLeaveFilePathAsString))
		{
			Seoul::FilePath filePath;
			if (Seoul::DataStoreParser::StringAsFilePath(s, uSizeInBytes, filePath))
			{
				return FilePath(filePath);
			}
		}

		// Final, store as a plain string.
		if (m_Top.IsArray())
		{
			return m_r.SetStringToArray(m_Top, m_uArrayIndex++, s, uSizeInBytes);
		}
		else
		{
			return m_r.SetStringToTable(m_Top, m_TableKey, s, uSizeInBytes);
		}
	}

	Seoul::Bool StartObject()
	{
		SEOUL_DUP_CHECK();

		if (m_Top.IsNull())
		{
			m_r.MakeTable();
			m_Top = m_r.GetRootNode();
			m_uArrayIndex = 0u;
			m_TableKey = Seoul::HString();
			return true;
		}
		else if (m_Top.IsArray())
		{
			if (m_r.SetTableToArray(m_Top, m_uArrayIndex++))
			{
				StackFrame frame;
				frame.m_Node = m_Top;
				frame.m_uArrayIndex = m_uArrayIndex;
				frame.m_TableKey = m_TableKey;
				m_vStack.PushBack(frame);

				SEOUL_VERIFY(m_r.GetValueFromArray(m_Top, m_uArrayIndex-1, m_Top));
				m_uArrayIndex = 0u;
				m_TableKey = Seoul::HString();
				return true;
			}
		}
		else
		{
			if (m_r.SetTableToTable(m_Top, m_TableKey))
			{
				StackFrame frame;
				frame.m_Node = m_Top;
				frame.m_uArrayIndex = m_uArrayIndex;
				frame.m_TableKey = m_TableKey;
				m_vStack.PushBack(frame);

				SEOUL_VERIFY(m_r.GetValueFromTable(m_Top, m_TableKey, m_Top));
				m_uArrayIndex = 0u;
				m_TableKey = Seoul::HString();
				return true;
			}
		}

		return false;
	}

	Seoul::Bool Key(Seoul::Byte const* s, Seoul::UInt32 uSizeInBytes, Seoul::Bool bCopy)
	{
		m_TableKey = Seoul::HString(s, uSizeInBytes);
		return true;
	}

	Seoul::Bool EndObject(Seoul::UInt32 uMembers)
	{
		return End();
	}

	Seoul::Bool StartArray()
	{
		SEOUL_DUP_CHECK();

		if (m_Top.IsNull())
		{
			m_r.MakeArray();
			m_Top = m_r.GetRootNode();
			m_uArrayIndex = 0u;
			m_TableKey = Seoul::HString();
			return true;
		}
		else if (m_Top.IsArray())
		{
			if (m_r.SetArrayToArray(m_Top, m_uArrayIndex++))
			{
				StackFrame frame;
				frame.m_Node = m_Top;
				frame.m_uArrayIndex = m_uArrayIndex;
				frame.m_TableKey = m_TableKey;
				m_vStack.PushBack(frame);

				SEOUL_VERIFY(m_r.GetValueFromArray(m_Top, m_uArrayIndex-1, m_Top));
				m_uArrayIndex = 0u;
				m_TableKey = Seoul::HString();
				return true;
			}
		}
		else
		{
			if (m_r.SetArrayToTable(m_Top, m_TableKey))
			{
				StackFrame frame;
				frame.m_Node = m_Top;
				frame.m_uArrayIndex = m_uArrayIndex;
				frame.m_TableKey = m_TableKey;
				m_vStack.PushBack(frame);

				SEOUL_VERIFY(m_r.GetValueFromTable(m_Top, m_TableKey, m_Top));
				m_uArrayIndex = 0u;
				m_TableKey = Seoul::HString();
				return true;
			}
		}

		return false;
	}

	Seoul::Bool EndArray(Seoul::UInt32 uElements)
	{
		return End();
	}

	Seoul::Bool End()
	{
		if (m_vStack.IsEmpty())
		{
			m_Top = Seoul::DataNode();
			m_uArrayIndex = 0u;
			m_TableKey = Seoul::HString();
		}
		else
		{
			StackFrame const frame = m_vStack.Back();
			m_vStack.PopBack();
			m_Top = frame.m_Node;
			m_uArrayIndex = frame.m_uArrayIndex;
			m_TableKey = frame.m_TableKey;
		}
		return true;
	}

	/** @return any additional error type from the parse. */
	HandlerAdditionalErrorType GetAdditionalErrorType() const
	{
		return m_eAdditionalErrorType;
	}

	/** @return the last set table key, or the empty HString. */
	Seoul::HString GetTableKey() const
	{
		return m_TableKey;
	}

private:
	struct StackFrame SEOUL_SEALED
	{
		Seoul::DataNode m_Node;
		Seoul::UInt32 m_uArrayIndex;
		Seoul::HString m_TableKey;
	};

	Seoul::DataStore& m_r;
	typedef Seoul::Vector<StackFrame, Seoul::MemoryBudgets::DataStore> Stack;
	Stack m_vStack;
	Seoul::DataNode m_Top;
	Seoul::UInt32 m_uArrayIndex;
	Seoul::HString m_TableKey;
	HandlerAdditionalErrorType m_eAdditionalErrorType;
	Seoul::UInt32 const m_uFlags;

	SEOUL_DISABLE_COPY(SeoulRapidJsonDataStoreHandler);
}; // class SeoulRapidJsonDataStoreHandler

#undef SEOUL_DUP_CHECK

class SeoulRapidJsonDataStoreHandlerWrapper SEOUL_SEALED
{
public:
	SeoulRapidJsonDataStoreHandlerWrapper()
		: m_pWrapped(nullptr)
	{
	}

	SeoulRapidJsonDataStoreHandlerWrapper(SeoulRapidJsonDataStoreHandler& r)
		: m_pWrapped(&r)
	{
	}

	~SeoulRapidJsonDataStoreHandlerWrapper()
	{
	}

	Seoul::Bool Null() { return (nullptr != m_pWrapped ? m_pWrapped->Null() : true); }
	Seoul::Bool Bool(Seoul::Bool b) { return (nullptr != m_pWrapped ? m_pWrapped->Bool(b) : true); }
	Seoul::Bool Int(Seoul::Int32 i) { return (nullptr != m_pWrapped ? m_pWrapped->Int(i) : true); }
	Seoul::Bool Uint(Seoul::UInt32 u) { return (nullptr != m_pWrapped ? m_pWrapped->Uint(u) : true); }
	Seoul::Bool Int64(Seoul::Int64 i) { return (nullptr != m_pWrapped ? m_pWrapped->Int64(i) : true); }
	Seoul::Bool Uint64(Seoul::UInt64 u) { return (nullptr != m_pWrapped ? m_pWrapped->Uint64(u) : true); }
	Seoul::Bool Double(Seoul::Double d) { return (nullptr != m_pWrapped ? m_pWrapped->Double(d) : true); }
	Seoul::Bool FilePath(Seoul::FilePath filePath) { return (nullptr != m_pWrapped ? m_pWrapped->FilePath(filePath) : true); }
	Seoul::Bool RawNumber(Seoul::Byte const* s, Seoul::UInt32 uSizeInBytes, Seoul::Bool bCopy) { return (nullptr != m_pWrapped ? m_pWrapped->RawNumber(s, uSizeInBytes, bCopy) : true); }
	Seoul::Bool String(Seoul::Byte const* s, Seoul::UInt32 uSizeInBytes, Seoul::Bool bCopy) { return (nullptr != m_pWrapped ? m_pWrapped->String(s, uSizeInBytes, bCopy) : true); }
	Seoul::Bool StartObject() { return (nullptr != m_pWrapped ? m_pWrapped->StartObject() : true); }
	Seoul::Bool Key(Seoul::Byte const* s, Seoul::UInt32 uSizeInBytes, Seoul::Bool bCopy) { return (nullptr != m_pWrapped ? m_pWrapped->Key(s, uSizeInBytes, bCopy) : true); }
	Seoul::Bool EndObject(Seoul::UInt32 uMembers) { return (nullptr != m_pWrapped ? m_pWrapped->EndObject(uMembers) : true); }
	Seoul::Bool StartArray() { return (nullptr != m_pWrapped ? m_pWrapped->StartArray() : true); }
	Seoul::Bool EndArray(Seoul::UInt32 uElements) { return (nullptr != m_pWrapped ? m_pWrapped->EndArray(uElements) : true); }
	Seoul::Bool End() { return (nullptr != m_pWrapped ? m_pWrapped->End() : true); }

private:
	SeoulRapidJsonDataStoreHandler* m_pWrapped;

	SEOUL_DISABLE_COPY(SeoulRapidJsonDataStoreHandlerWrapper);
}; // class SeoulRapidJsonDataStoreHandlerWrapper

typedef GenericSchemaDocument<Value, SeoulRapidJsonAllocator> SeoulSchemaDocument;
typedef IGenericRemoteSchemaDocumentProvider<SeoulSchemaDocument> SeoulIRemoteSchemaDocumentProvider;
typedef internal::GenericRegex<SeoulSchemaDocument::EncodingType, SeoulRapidJsonAllocator> SeoulRegexType;
typedef internal::GenericRegexSearch<SeoulRegexType, SeoulRapidJsonAllocator> SeoulRegexSearchType;
typedef GenericSchemaValidator<SeoulSchemaDocument, SeoulRapidJsonDataStoreHandlerWrapper, SeoulRapidJsonAllocator> SeoulSchemaValidator;

template <typename ENCODING>
struct SeoulRangedInsituStringStreamT SEOUL_SEALED
{
	typedef typename ENCODING::Ch Ch;

	SeoulRangedInsituStringStreamT(Ch* pBegin, Ch* pEnd)
		: m_pBegin(pBegin)
		, m_pEnd(pEnd)
		, m_pIn(pBegin)
		, m_pOut(nullptr)
	{
	}

	// Read
	Ch Peek() const
	{
		if (SEOUL_LIKELY(m_pIn < m_pEnd)) { return *m_pIn; }
		else { return (Ch)0; }
	}
	Ch Take()
	{
		Ch ret;
		if (SEOUL_LIKELY(m_pIn < m_pEnd)) { ret = *m_pIn; }
		else { ret = (Ch)0; }
		m_pIn++;
		return ret;
	}
	size_t Tell() const { return (size_t)(m_pIn - m_pBegin); }

	// Write
	void Put(Ch c) { RAPIDJSON_ASSERT(m_pOut != nullptr); *m_pOut++ = c; }

	Ch* PutBegin() { return m_pOut = m_pIn; }
	size_t PutEnd(Ch* pBegin) { return (size_t)(m_pOut - pBegin); }
	void Flush() {}

	Ch* Push(size_t zCount) { Ch* pBegin = m_pOut; m_pOut += zCount; return pBegin; }
	void Pop(size_t zCount) { m_pOut -= zCount; }

	Ch* m_pBegin;
	Ch* m_pEnd;
	Ch* m_pIn;
	Ch* m_pOut;
}; // struct SeoulRangedInsituStringStreamT
template <typename ENCODING>
struct StreamTraits< SeoulRangedInsituStringStreamT<ENCODING> >
{
	enum
	{
		copyOptimization = 1
	};
}; // struct StreamTraits
typedef SeoulRangedInsituStringStreamT< UTF8<> > SeoulRangedInsituStringStream;

template <typename ENCODING>
struct SeoulRangedReadonlyStringStreamT SEOUL_SEALED
{
	typedef typename ENCODING::Ch Ch;

	SeoulRangedReadonlyStringStreamT(Ch const* pBegin, Ch const* pEnd)
		: m_pBegin(pBegin)
		, m_pEnd(pEnd)
		, m_pIn(pBegin)
	{
	}

	// Read
	Ch Peek() const
	{
		if (SEOUL_LIKELY(m_pIn < m_pEnd)) { return *m_pIn; }
		else { return (Ch)0; }
	}
	Ch Take()
	{
		Ch ret;
		if (SEOUL_LIKELY(m_pIn < m_pEnd)) { ret = *m_pIn; }
		else { ret = (Ch)0; }
		m_pIn++;
		return ret;
	}
	size_t Tell() const { return (size_t)(m_pIn - m_pBegin); }

	// Write - all disabled, must never be called.
	void Flush() { RAPIDJSON_ASSERT(false); }
	void Put(Ch) { RAPIDJSON_ASSERT(false); }
	Ch* PutBegin() { RAPIDJSON_ASSERT(false); return 0; }
	size_t PutEnd(Ch*) { RAPIDJSON_ASSERT(false); return 0; }

	Ch const* m_pBegin;
	Ch const* m_pEnd;
	Ch const* m_pIn;
}; // struct SeoulRangedReadonlyStringStreamT
template <typename ENCODING>
struct StreamTraits< SeoulRangedReadonlyStringStreamT<ENCODING> >
{
	enum
	{
		copyOptimization = 1
	};
}; // struct StreamTraits
typedef SeoulRangedReadonlyStringStreamT< UTF8<> > SeoulRangedReadonlyStringStream;

} // namespace rapidjson

namespace Seoul
{

/** Convenience. */
using namespace DataStoreParserFlags;

/** Shared flags across all our parsers. */
static const UInt32 kuCommonFlags =
	rapidjson::kParseValidateEncodingFlag |
	rapidjson::kParseCommentsFlag |
	rapidjson::kParseTrailingCommasFlag |
	rapidjson::kParseNanAndInfFlag;

static const HString kPattern("Pattern");
static const HString kSchema("Schema");

struct DataStoreSchemaCacheEntry SEOUL_SEALED
{
	DataStoreSchemaCacheEntry(
		rapidjson::SeoulIRemoteSchemaDocumentProvider const* pProvider = nullptr,
		rapidjson::Document const* pDocument = nullptr,
		rapidjson::SeoulSchemaDocument const* pSchema = nullptr)
		: m_pProvider(pProvider)
		, m_pDocument(pDocument)
		, m_pSchema(pSchema)
	{
	}

	rapidjson::SeoulIRemoteSchemaDocumentProvider const* m_pProvider;
	rapidjson::Document const* m_pDocument;
	rapidjson::SeoulSchemaDocument const* m_pSchema;
}; // struct DataStoreSchemaCacheEntry

struct DataStoreSchemaLookupEntry SEOUL_SEALED
{
	DataStoreSchemaLookupEntry(Wildcard* pWildcard = nullptr, FilePath schema = FilePath())
		: m_pWildcard(pWildcard)
		, m_FilePath(schema)
	{
	}

	Wildcard* m_pWildcard;
	FilePath m_FilePath;
}; // struct DataStoreSchemaLookupEntry
typedef Vector<DataStoreSchemaLookupEntry, MemoryBudgets::DataStore> SchemaLookup;

static inline void DestroySchemaLookup(SchemaLookup& rv)
{
	SchemaLookup v;
	v.Swap(rv);
	for (auto i = v.Begin(); v.End() != i; ++i)
	{
		SafeDelete(i->m_pWildcard);
	}
}

/** Loads a file that defines mappings from paths on disk into specific schema files. */
static Bool LoadSchemaLookup(FilePath filePath, SchemaLookup& rv)
{
	// Load the file - if this fails, we're done.
	DataStore dataStore;
	if (!DataStoreParser::FromFile(filePath, dataStore, DataStoreParserFlags::kLogParseErrors))
	{
		return false;
	}

	auto const root = dataStore.GetRootNode();

	UInt32 uArrayCount = 0u;
	(void)dataStore.GetArrayCount(root, uArrayCount);

	SchemaLookup v;
	v.Reserve(uArrayCount);

	// Enumerate the list of lookups.
	DataNode node;
	DataNode value;
	for (UInt32 i = 0u; i < uArrayCount; ++i)
	{
		(void)dataStore.GetValueFromArray(root, i, node);

		// Get the pattern string and convert to a regular expression.
		String sWildcard;
		(void)dataStore.GetValueFromTable(node, kPattern, value);
		(void)dataStore.AsString(value, sWildcard);

		// Get the schema mapping.
		FilePath mappingFilePath;
		(void)dataStore.GetValueFromTable(node, kSchema, value);
		if (!dataStore.AsFilePath(value, mappingFilePath) && !value.IsNull())
		{
			SEOUL_WARN("%s: failed loading schema lookup, entry %u has an invalid file path.",
				mappingFilePath.GetRelativeFilenameInSource().CStr(),
				i);
			DestroySchemaLookup(v);
			return false;
		}

		// If we have a valid regex, insert the entry. This allows
		// entries that deliberately map to "no schema".
		if (!sWildcard.IsEmpty())
		{
			auto pWildcard = SEOUL_NEW(MemoryBudgets::DataStore) Wildcard(sWildcard);
			v.PushBack(DataStoreSchemaLookupEntry(pWildcard, mappingFilePath));
		}
	}

	// Done, swap in and succeed.
	rv.Swap(v);
	return true;
}

/** Handles cooked binary text files - the Lexer just immediately passes the blob to the parser. */
static Bool ParseCookedBinary(
	Byte const* pData,
	UInt32 zDataSizeInBytes,
	DataStore& rDataStore)
{
	FullyBufferedSyncFile syncFile(
		(void*)pData,
		zDataSizeInBytes,
		false);

	DataStore dataStore;
	if (dataStore.Load(syncFile))
	{
		rDataStore.Swap(dataStore);
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * @return True if the currently set stream is a cooked binary version of
 * a SeoulEngine text file, false otherwise.
 */
Bool DataStoreParser::IsCookedBinary(Byte const* pData, UInt32 uDataSizeInBytes)
{
	if (uDataSizeInBytes < sizeof(kaCookedDataStoreBinarySignature))
	{
		return false;
	}

	return (
		0 == memcmp(pData, kaCookedDataStoreBinarySignature, sizeof(kaCookedDataStoreBinarySignature)) ||
		0 == memcmp(pData, kaCookedDataStoreBinarySignatureVersion0, sizeof(kaCookedDataStoreBinarySignatureVersion0)));
}

// The following block of functions are used exclusively to report parse errors to the log.
#if SEOUL_LOGGING_ENABLED
/**
 * Scan a buffer to derive the line/column based on zOffset into the buffer.
 *
 * Only valid after read-only parsing (not insitu parsing). Destructive parsing
 * can invalid line information (e.g. "\n").
 */
static void ResolveLineAndColumn(Byte const* pData, size_t zOffset, Int32& riLine, Int32& riColumn)
{
	LexerContext context;
	context.SetStream(pData, (UInt32)zOffset);

	Byte const* const pEnd = (pData + zOffset);
	while (context.GetStream() < pEnd)
	{
		context.Advance();
	}

	riColumn = context.GetColumn();
	riLine = context.GetLine();
}

/** Minor convenience utility to convert various control characters into human friendly strings. */
static inline Byte const* ControlToHumanReadableString(Byte const ch)
{
	switch (ch)
	{
	case '\n': return "<newline>";
	case '\r': return "<carriage-return>";
	case '\t': return "<tab>";
	default:
		return "";
	};
}

/** Generate the lookup table for human friendly strings, used with schema validation. */
static inline HashTable<HString, Byte const*, MemoryBudgets::DataStore> GetInvalidKeywordLookup()
{
	HashTable<HString, Byte const*, MemoryBudgets::DataStore> t;
	t.Insert(HString("additionalItems"), "Unknown item.");
	t.Insert(HString("additionalProperties"), "Unknown property.");
	t.Insert(HString("anyOf"), "Type is not any of the allowed types.");
	t.Insert(HString("minimum"), "Value is below the minimum range.");
	t.Insert(HString("maximum"), "Value is above the maximum range.");
	t.Insert(HString("minProperties"), "Too few properties.");
	t.Insert(HString("maxProperties"), "Too many properties");
	t.Insert(HString("minItems"), "Too few items.");
	t.Insert(HString("maxItems"), "Too many items.");
	t.Insert(HString("oneOf"), "Type is not one of the allowed types.");
	t.Insert(HString("pattern"), "String does not match its formatting pattern. Check for a typo in the string syntax.");
	t.Insert(HString("patternProperties"), "Key matched a property pattern but did not fulfill its definition.");
	t.Insert(HString("required"), "A required property is not defined.");
	t.Insert(HString("type"), "Value is not of the expected type.");

	// TODO:
	/*
	RAPIDJSON_STRING_(Null, 'n', 'u', 'l', 'l')
	RAPIDJSON_STRING_(Boolean, 'b', 'o', 'o', 'l', 'e', 'a', 'n')
	RAPIDJSON_STRING_(Object, 'o', 'b', 'j', 'e', 'c', 't')
	RAPIDJSON_STRING_(Array, 'a', 'r', 'r', 'a', 'y')
	RAPIDJSON_STRING_(String, 's', 't', 'r', 'i', 'n', 'g')
	RAPIDJSON_STRING_(Number, 'n', 'u', 'm', 'b', 'e', 'r')
	RAPIDJSON_STRING_(Integer, 'i', 'n', 't', 'e', 'g', 'e', 'r')
	RAPIDJSON_STRING_(Enum, 'e', 'n', 'u', 'm')
	RAPIDJSON_STRING_(AllOf, 'a', 'l', 'l', 'O', 'f')
	RAPIDJSON_STRING_(Not, 'n', 'o', 't')
	RAPIDJSON_STRING_(Properties, 'p', 'r', 'o', 'p', 'e', 'r', 't', 'i', 'e', 's')
	RAPIDJSON_STRING_(Dependencies, 'd', 'e', 'p', 'e', 'n', 'd', 'e', 'n', 'c', 'i', 'e', 's')
	RAPIDJSON_STRING_(PatternProperties, 'p', 'a', 't', 't', 'e', 'r', 'n', 'P', 'r', 'o', 'p', 'e', 'r', 't', 'i', 'e', 's')
	RAPIDJSON_STRING_(Items, 'i', 't', 'e', 'm', 's')
	RAPIDJSON_STRING_(UniqueItems, 'u', 'n', 'i', 'q', 'u', 'e', 'I', 't', 'e', 'm', 's')
	RAPIDJSON_STRING_(MinLength, 'm', 'i', 'n', 'L', 'e', 'n', 'g', 't', 'h')
	RAPIDJSON_STRING_(MaxLength, 'm', 'a', 'x', 'L', 'e', 'n', 'g', 't', 'h')
	RAPIDJSON_STRING_(ExclusiveMinimum, 'e', 'x', 'c', 'l', 'u', 's', 'i', 'v', 'e', 'M', 'i', 'n', 'i', 'm', 'u', 'm')
	RAPIDJSON_STRING_(ExclusiveMaximum, 'e', 'x', 'c', 'l', 'u', 's', 'i', 'v', 'e', 'M', 'a', 'x', 'i', 'm', 'u', 'm')
	RAPIDJSON_STRING_(MultipleOf, 'm', 'u', 'l', 't', 'i', 'p', 'l', 'e', 'O', 'f')
	*/

	return t;
}

/** Return a friendly string corresponding to the given invalid schema keyword. */
static Byte const* GetInvalidSchemaKeywordString(Byte const* s)
{
	static const HashTable<HString, Byte const*, MemoryBudgets::DataStore> kTable(GetInvalidKeywordLookup());

	// Handle null, possible for rapidjson to return a null for this case.
	if (nullptr == s)
	{
		return "Unknown schema error (check for duplicate table key).";
	}

	Byte const* sReturn = s;
	(void)kTable.GetValue(HString(s), sReturn);
	return sReturn;
}

/** Handles parse error reporting when requested. */
template <typename T>
static void ReportError(
	Byte const* pData,
	const rapidjson::ParseResult& result,
	FilePath filePath,
	const T& handler,
	FilePath schemaFilePath,
	rapidjson::SeoulSchemaValidator const* pValidator)
{
	// Resolve line/column with the parser position.
	Int32 iLine, iColumn;
	ResolveLineAndColumn(pData, result.Offset(), iLine, iColumn);

	using namespace rapidjson;
	switch (result.Code())
	{
	case kParseErrorDocumentRootNotSingular: //!< The document root must not follow by other values.
	case kParseErrorObjectMissName: //!< Missing a name for object member.
	case kParseErrorObjectMissColon: //!< Missing a colon after a name of object member.
	case kParseErrorObjectMissCommaOrCurlyBracket: //!< Missing a comma or '}' after an object member.
	case kParseErrorArrayMissCommaOrSquareBracket: //!< Missing a comma or ']' after an array element.
	case kParseErrorStringUnicodeSurrogateInvalid: //!< The surrogate pair in string is invalid.
	case kParseErrorStringMissQuotationMark: //!< Missing a closing quotation mark in string.
	case kParseErrorStringInvalidEncoding: //!< Invalid encoding in string.
	case kParseErrorNumberTooBig: //!< Number too big to be stored in double.
	case kParseErrorNumberMissFraction: //!< Miss fraction part in number.
	case kParseErrorNumberMissExponent: //!< Miss exponent in number.
	case kParseErrorTermination: //!< Parsing was terminated.
		// Possible special handling in this case if a validator was used and
		// the failure was due to validation.
		if (nullptr != pValidator && !pValidator->IsValid())
		{
			auto const invalidDocumentPtr = pValidator->GetInvalidDocumentPointer();
			rapidjson::StringBuffer invalidDocumentUri;
			(void)invalidDocumentPtr.StringifyUriFragment(invalidDocumentUri);

			auto const invalidSchemaPtr = pValidator->GetInvalidSchemaPointer();
			rapidjson::StringBuffer invalidSchemaUri;
			(void)invalidSchemaPtr.StringifyUriFragment(invalidSchemaUri);

			SEOUL_WARN("%s(%d, %d): Schema '%s' rule at '%s' was violated at '%s': %s\n",
				filePath.GetRelativeFilenameInSource().CStr(),
				iLine,
				iColumn,
				schemaFilePath.GetRelativeFilenameInSource().CStr(),
				invalidSchemaUri.GetString(),
				invalidDocumentUri.GetString(),
				GetInvalidSchemaKeywordString(pValidator->GetInvalidSchemaKeyword()));

			break;
		}
		// Otherwise, fall-through.
	case kParseErrorUnspecificSyntaxError: //!< Unspecific syntax error.
	case kParseErrorStringEscapeInvalid: //!< Invalid escape character in string.
	case kParseErrorValueInvalid: //!< Invalid value.
	case kParseErrorStringUnicodeEscapeInvalidHex: //!< Incorrect hex digit after \\u escape in string.
		{
			// Special handling on kParseErrorTermination if we have additional info.
			if (result.Code() == kParseErrorTermination &&
				handler.GetAdditionalErrorType() == rapidjson::HandlerAdditionalErrorType::kDuplicateTableKey)
			{
				SEOUL_WARN("%s(%d, %d): Duplicate table key '%s'\n",
					filePath.GetRelativeFilenameInSource().CStr(),
					iLine,
					iColumn,
					handler.GetTableKey().CStr());
			}
			else
			{
				Byte const ch = pData[result.Offset()];
				switch (ch)
				{
				case '\n': // fall-through
				case '\r': // fall-through
				case '\t':
					SEOUL_WARN("%s(%d, %d): %s At '%s'\n",
						filePath.GetRelativeFilenameInSource().CStr(),
						iLine,
						iColumn,
						GetParseError_En(result.Code()),
						ControlToHumanReadableString(ch));
					break;

				default:
					SEOUL_WARN("%s(%d, %d): %s At '%c'\n",
						filePath.GetRelativeFilenameInSource().CStr(),
						iLine,
						iColumn,
						GetParseError_En(result.Code()),
						ch);
					break;
				};
			}
		}
		break;

	case kParseErrorNone: //!< No error.
	case kParseErrorDocumentEmpty: //!< The document is empty.
	default:
		break;
	};
}
#endif // /#if SEOUL_LOGGING_ENABLED

static inline Byte* SkipBom(Byte const* pData, UInt32& ruDataSizeInBytes)
{
	// Skip the UTF8 BOM if it is present.
	LexerContext context;
	context.SetStream(pData, ruDataSizeInBytes);
	ruDataSizeInBytes -= (UInt32)(context.GetStream() - context.GetStreamBegin());
	return (Byte*)context.GetStream();
}

class DataStoreSchemaDocumentProvider SEOUL_SEALED : public rapidjson::SeoulIRemoteSchemaDocumentProvider
{
public:
	DataStoreSchemaDocumentProvider(FilePath filePath, DataStoreSchemaCache& r)
		: m_FilePath(filePath)
		, m_r(r)
	{
	}

private:
	FilePath m_FilePath;
	DataStoreSchemaCache& m_r;

	virtual const rapidjson::SeoulSchemaDocument* GetRemoteDocument(const Ch* uri, rapidjson::SizeType length) SEOUL_OVERRIDE;

	SEOUL_DISABLE_COPY(DataStoreSchemaDocumentProvider);
}; // class DataStoreSchemaDocumentProvider

/**
 * A schema cache loads schema files for validation of .json file. Schema cache also implements the remote
 * schema provider for external schema references.
 */
class DataStoreSchemaCache SEOUL_SEALED
{
public:
	DataStoreSchemaCache(const SchemaLookup& vLookup)
		: m_tCache()
		, m_Mutex()
		, m_vLookup(vLookup)
	{
	}

	~DataStoreSchemaCache()
	{
		Lock lock(m_Mutex);

		// Cleanup all our entries prior to shutdown.
		{
			auto const iBegin = m_tCache.Begin();
			auto const iEnd = m_tCache.End();
			for (auto i = iBegin; iEnd != i; ++i)
			{
				SafeDelete(i->Second.m_pSchema);
				SafeDelete(i->Second.m_pDocument);
				SafeDelete(i->Second.m_pProvider);
			}
		}
		m_tCache.Clear();

		// Cleanup our lookup entires prior to shutdown.
		DestroySchemaLookup(m_vLookup);
	}

	/** Given a .json file, resolve the schema file (if any) that applies to it. */
	FilePath FindSchema(FilePath filePath) const
	{
		Lock lock(m_Mutex);

		for (auto i = m_vLookup.Begin(); m_vLookup.End() != i; ++i)
		{
			if (i->m_pWildcard->IsExactMatch(filePath.GetRelativeFilenameWithoutExtension().CStr()))
			{
				return i->m_FilePath;
			}
		}

		return FilePath();
	}

	/** Get an existing cached schema, or load and cache it, if possible. May return null if the schema is not defined. */
	const rapidjson::SeoulSchemaDocument* LoadOrGetSchema(FilePath filePath)
	{
		// Check the cache and return immediately if defined.
		{
			DataStoreSchemaCacheEntry ret;
			Lock lock(m_Mutex);
			if (m_tCache.GetValue(filePath, ret))
			{
				return ret.m_pSchema;
			}
		}

		// Load and cache the schema.
		UInt32 uSizeInBytes = 0u;
		void* pData = nullptr;
		if (!FileManager::Get()->ReadAll(filePath, pData, uSizeInBytes, 0u, MemoryBudgets::DataStore))
		{
			return nullptr;
		}

		// Document instance to parse into.
		auto pDocument = SEOUL_NEW(MemoryBudgets::DataStore) rapidjson::Document;

		// If a binary format, we need to load it into a DataStore, then output it
		// as a JSON string.
		if (DataStoreParser::IsCookedBinary((Byte const*)pData, uSizeInBytes))
		{
			String sBody;
			{
				DataStore dataStore;
				{
					FullyBufferedSyncFile file(pData, uSizeInBytes, true);
					if (!dataStore.Load(file))
					{
						SEOUL_WARN("Schema '%s' is invalid cooked binary.", filePath.CStr());
						MemoryManager::Deallocate(pData);
						return nullptr;
					}
				}
				dataStore.ToString(dataStore.GetRootNode(), sBody, true, 0, true);
			}

			// Relinquish the string into a buffer.
			sBody.RelinquishBuffer(pData, uSizeInBytes);
		}

		// Now parse the data, either original or the binary blob converted to strings.
		pDocument->Parse<kuCommonFlags>((Byte const*)pData, uSizeInBytes);

		// Error in the schema document itself.
		if (pDocument->HasParseError())
		{
#if SEOUL_LOGGING_ENABLED
			rapidjson::PlaceholderHandler handler;
			ReportError((Byte const*)pData, *pDocument, filePath, handler, FilePath(), nullptr);
#endif // /#if SEOUL_LOGGING_ENABLED

			// Release the raw data read.
			MemoryManager::Deallocate(pData); pData = nullptr;

			SafeDelete(pDocument);
			return nullptr;
		}

		// Release the raw data read.
		MemoryManager::Deallocate(pData); pData = nullptr;

		// Create a provider, then insert it into the cache.
		auto pProvider = SEOUL_NEW(MemoryBudgets::DataStore) DataStoreSchemaDocumentProvider(filePath, *this);
		auto const sUri(filePath.GetRelativeFilename());
		rapidjson::SeoulSchemaDocument const* pSchema = SEOUL_NEW(MemoryBudgets::DataStore) rapidjson::SeoulSchemaDocument(
			*pDocument,
			sUri.CStr(),
			sUri.GetUnicodeLength(),
			pProvider,
			&rapidjson::s_Allocator);

		{
			DataStoreSchemaCacheEntry entry(pProvider, pDocument, pSchema);

			Lock lock(m_Mutex);

			// Insertion may fail (if this schema was loaded between
			// the above and this mutex locks), so we need to cleanup
			// the entry in that case.
			auto const e = m_tCache.Insert(filePath, entry);
			if (!e.Second)
			{
				SafeDelete(entry.m_pSchema);
				SafeDelete(entry.m_pDocument);
				SafeDelete(entry.m_pProvider);

				// Now update p to be the found entry.
				pSchema = e.First->Second.m_pSchema;
			}
		}

		return pSchema;
	}

private:
	typedef HashTable<FilePath, DataStoreSchemaCacheEntry, MemoryBudgets::DataStore> Cache;
	Cache m_tCache;
	Mutex m_Mutex;
	SchemaLookup m_vLookup;

	SEOUL_DISABLE_COPY(DataStoreSchemaCache);
}; // class DataStoreSchemaCache

/** Provides resolution of external URIs (references from a schema to another schema. */
const rapidjson::SeoulSchemaDocument* DataStoreSchemaDocumentProvider::GetRemoteDocument(const Ch* uri, rapidjson::SizeType length)
{
	String const sUri((Byte const*)uri, (UInt32)length);

	FilePath filePath;
	if (!DataStoreParser::StringAsFilePath(sUri, filePath))
	{
		String const sDirectory(Path::GetDirectoryName(m_FilePath.GetAbsoluteFilename()));
		String const sFileName(Path::Combine(sDirectory, sUri));

		filePath = FilePath::CreateFilePath(m_FilePath.GetDirectory(), sFileName);
	}

	return m_r.LoadOrGetSchema(filePath);
}

// Shared finisher of multiple parse functions.
static Bool FinalizeParse(
	Byte const* pData,
	const rapidjson::ParseResult& result,
	FilePath filePath,
	const rapidjson::SeoulRapidJsonDataStoreHandler& handler,
	FilePath schemaFilePath,
	rapidjson::SeoulSchemaValidator const* pValidator,
	Bool bReportErrors,
	DataStore& rIn,
	DataStore& rOut)
{
	using namespace rapidjson;

	if (result)
	{
		rOut.Swap(rIn);
		rOut.CollectGarbageAndCompactHeap();
		return true;
	}
	// We allow empty documents, handle that as a special case.
	else if (result.Code() == kParseErrorDocumentEmpty)
	{
		DataStore emptyDataStore;
		rOut.Swap(emptyDataStore);
		return true;
	}
	else
	{
#if SEOUL_LOGGING_ENABLED
		if (bReportErrors)
		{
			ReportError(pData, result, filePath, handler, schemaFilePath, pValidator);
		}
#endif // /#if SEOUL_LOGGING_ENABLED

		return false;
	}
}

static Bool InsituParse(
	FilePath schemaFilePath,
	rapidjson::SeoulSchemaDocument const* pSchema,
	Byte* pData,
	UInt32 uDataSizeInBytes,
	DataStore& rDataStore,
	FilePath filePath,
	UInt32 uFlags)
{
	using namespace rapidjson;

	// Skip the UTF8 BOM if it is present.
	pData = SkipBom(pData, uDataSizeInBytes);

	GenericReader<UTF8<>, UTF8<>, SeoulRapidJsonAllocator> reader(&s_Allocator);

	DataStore dataStore;
	SeoulRapidJsonDataStoreHandler handler(dataStore, uFlags);

	SeoulRangedInsituStringStream stream(pData, (pData + uDataSizeInBytes));
	ParseResult result;

	// If we have a schema, parse with schema validation.
	if (nullptr != pSchema)
	{
		SeoulRapidJsonDataStoreHandlerWrapper wrapper(handler);
		SeoulSchemaValidator validator(*pSchema, wrapper, &s_Allocator);
		result = reader.Parse<kParseInsituFlag|kuCommonFlags>(stream, validator);
	}
	// Otherwise, parse typically.
	else
	{
		result = reader.Parse<kParseInsituFlag|kuCommonFlags>(stream, handler);
	}

	// Insitu parsing can never report errors as the buffer has been modified
	// in such a way to prevent column/line number reporting.
	return FinalizeParse(pData, result, filePath, handler, FilePath(), nullptr, false, dataStore, rDataStore);
}

static Bool ReadOnlyParse(
	FilePath schemaFilePath,
	rapidjson::SeoulSchemaDocument const* pSchema,
	Byte const* pData,
	UInt32 uDataSizeInBytes,
	DataStore& rDataStore,
	UInt32 uFlags,
	FilePath filePath)
{
	using namespace rapidjson;

	// Skip the UTF8 BOM if it is present.
	pData = SkipBom(pData, uDataSizeInBytes);

	GenericReader<UTF8<>, UTF8<>, SeoulRapidJsonAllocator> reader(&s_Allocator);

	DataStore dataStore;
	SeoulRapidJsonDataStoreHandler handler(dataStore, uFlags);

	SeoulRangedReadonlyStringStream stream(pData, pData + uDataSizeInBytes);

	// If we have a schema, parse with schema validation.
	if (nullptr != pSchema)
	{
		SeoulRapidJsonDataStoreHandlerWrapper wrapper(handler);
		SeoulSchemaValidator validator(*pSchema, wrapper, &s_Allocator);
		auto const result = reader.Parse<kuCommonFlags>(stream, validator);
		return FinalizeParse(pData, result, filePath, handler, schemaFilePath, &validator, (kLogParseErrors == (kLogParseErrors & uFlags)), dataStore, rDataStore);
	}
	// Otherwise, parse typically.
	else
	{
		auto const result = reader.Parse<kuCommonFlags>(stream, handler);
		return FinalizeParse(pData, result, filePath, handler, FilePath(), nullptr, (kLogParseErrors == (kLogParseErrors & uFlags)), dataStore, rDataStore);
	}
}

DataStoreSchemaCache* DataStoreParser::CreateSchemaCache(FilePath schemaLookup)
{
	SchemaLookup vLookup;
	if (schemaLookup.IsValid())
	{
		if (!LoadSchemaLookup(schemaLookup, vLookup))
		{
			return nullptr;
		}
	}

	return SEOUL_NEW(MemoryBudgets::DataStore) DataStoreSchemaCache(vLookup);
}

void DataStoreParser::DestroySchemaCache(DataStoreSchemaCache*& rp)
{
	SafeDelete(rp);
}

static Bool ResolveSchema(
	DataStoreSchemaCache* pCache,
	FilePath filePath,
	UInt32 uFlags,
	FilePath& schemaFilePath,
	rapidjson::SeoulSchemaDocument const*& rp)
{
	if (nullptr == pCache)
	{
		rp = nullptr;
		return true;
	}

	schemaFilePath = pCache->FindSchema(filePath);
	if (!schemaFilePath.IsValid())
	{
		rp = nullptr;
		return true;
	}

	auto pSchema = pCache->LoadOrGetSchema(schemaFilePath);
	if (nullptr == pSchema)
	{
#if SEOUL_LOGGING_ENABLED
		if (kLogParseErrors == (kLogParseErrors & uFlags))
		{
			SEOUL_WARN("%s: follows schema '%s' but "
				"loading of the schema failed (or the file does not exist).",
				filePath.GetRelativeFilenameInSource().CStr(),
				schemaFilePath.GetRelativeFilenameInSource().CStr());
		}
#endif // /#if SEOUL_LOGGING_ENABLED

		return false;
	}

	rp = pSchema;
	return true;
}

Bool DataStoreParser::FromFile(DataStoreSchemaCache* pCache, FilePath filePath, DataStore& rDataStore, UInt32 uFlags /* = 0u*/)
{
	UInt32 uSizeInBytes = 0u;
	void* pData = nullptr;
	if (!FileManager::Get()->ReadAll(filePath, pData, uSizeInBytes, 0u, MemoryBudgets::DataStore))
	{
		return false;
	}

	// Check for cooked binary.
	if (IsCookedBinary((Byte const*)pData, uSizeInBytes))
	{
		// Parse as cooked, deallocate, and return.
		Bool const bReturn = ParseCookedBinary((Byte const*)pData, uSizeInBytes, rDataStore);
		MemoryManager::Deallocate(pData);
		return bReturn;
	}
	// Otherwise, normal parse.
	else
	{
		Bool bReturn = true;

		FilePath schemaFilePath;
		rapidjson::SeoulSchemaDocument const* pSchema = nullptr;
		if (!ResolveSchema(pCache, filePath, uFlags, schemaFilePath, pSchema))
		{
			bReturn = false;
		}

		// Perform the parse - need to use a read-only parse if kLogParseErrors
		// was specified, since otherwise the Insitu process can invalidate
		// line computation (e.g. "\n" in a string).
		if (bReturn)
		{
			if (kLogParseErrors == (kLogParseErrors & uFlags))
			{
				bReturn = ReadOnlyParse(schemaFilePath, pSchema, (Byte const*)pData, uSizeInBytes, rDataStore, uFlags, filePath);
			}
			else
			{
				bReturn = InsituParse(schemaFilePath, pSchema, (Byte*)pData, uSizeInBytes, rDataStore, filePath, uFlags);
			}
		}

		// Release the memory.
		MemoryManager::Deallocate(pData);

		// Done.
		return bReturn;
	}
}

Bool DataStoreParser::FromString(DataStoreSchemaCache* pCache, Byte const* s, UInt32 uSizeInBytes, DataStore& rDataStore, UInt32 uFlags /* = 0u*/, FilePath filePath /* = FilePath()*/)
{
	// Check for cooked binary.
	if (IsCookedBinary(s, uSizeInBytes))
	{
		// Parse as cooked.
		return ParseCookedBinary(s, uSizeInBytes, rDataStore);
	}
	// Normal parse.
	else
	{
		FilePath schemaFilePath;
		rapidjson::SeoulSchemaDocument const* pSchema = nullptr;
		if (!ResolveSchema(pCache, filePath, uFlags, schemaFilePath, pSchema))
		{
			return false;
		}

		// Perform the parse.
		return ReadOnlyParse(schemaFilePath, pSchema, s, uSizeInBytes, rDataStore, uFlags, filePath);
	}
}

Bool DataStoreParser::FromString(DataStoreSchemaCache* pCache, const String& s, DataStore& rDataStore, UInt32 uFlags /* = 0u*/, FilePath filePath /* = FilePath()*/)
{
	// NOTE: SeoulString cannot be a cooked binary.
	FilePath schemaFilePath;
	rapidjson::SeoulSchemaDocument const* pSchema = nullptr;
	if (!ResolveSchema(pCache, filePath, uFlags, schemaFilePath, pSchema))
	{
		return false;
	}

	// Perform the parse.
	return ReadOnlyParse(schemaFilePath, pSchema, s.CStr(), s.GetSize(), rDataStore, uFlags, filePath);
}

static String const kasSchemePrefixes[] =
{
	// Although tempting, don't use GameDirectory::kasScehems here, due to
	// static initialization order.
	String(),
	String("config://"),
	String("content://"),
	String("log://"),
	String("save://"),
	String("tools://"),
	String("video://"),
};
SEOUL_STATIC_ASSERT(SEOUL_ARRAY_COUNT(kasSchemePrefixes) == (UInt32)GameDirectory::GAME_DIRECTORY_COUNT);

static inline Bool StringAsFilePathUtil(Byte const* s, UInt32 uSizeInBytes, GameDirectory eTo, FilePath& rFilePath)
{
	UInt32 const uPrefix = kasSchemePrefixes[(UInt32)eTo].GetSize();
	Byte const* const sPrefix = kasSchemePrefixes[(UInt32)eTo].CStr();
	if (uSizeInBytes >= uPrefix && 0 == strncmp(sPrefix, s, uPrefix))
	{
		rFilePath = FilePath::CreateFilePath(eTo, String(s + uPrefix, uSizeInBytes - uPrefix));
		if (rFilePath.IsValid())
		{
			return true;
		}
	}

	return false;
}

Bool DataStoreParser::StringAsFilePath(Byte const* s, UInt32 uSizeInBytes, FilePath& rFilePath)
{
	if (0u == uSizeInBytes)
	{
		return false;
	}

	switch (s[0])
	{
	case 'c':
		if (StringAsFilePathUtil(s, uSizeInBytes, GameDirectory::kConfig, rFilePath) ||
			StringAsFilePathUtil(s, uSizeInBytes, GameDirectory::kContent, rFilePath))
		{
			return true;
		}
		break;

	case 'l':
		if (StringAsFilePathUtil(s, uSizeInBytes, GameDirectory::kLog, rFilePath))
		{
			return true;
		}
		break;
	case 's':
		if (StringAsFilePathUtil(s, uSizeInBytes, GameDirectory::kSave, rFilePath))
		{
			return true;
		}
		break;
	case 't':
		if (StringAsFilePathUtil(s, uSizeInBytes, GameDirectory::kToolsBin, rFilePath))
		{
			return true;
		}
		break;
	case 'v':
		if (StringAsFilePathUtil(s, uSizeInBytes, GameDirectory::kVideos, rFilePath))
		{
			return true;
		}
		break;
	default:
		break;
	};

	return false;
}

// Start of code that handles DataStore "commands" syntax. Commands
// is JSON in an expected format that will be evaluated to generate
// more complex DataStore at runtime. Identification of a "commands"
// format DataStore is done via duck typing. A DataStore of the following form:
//
// [
//   ["$include", ... ]
// ]
//
// Where "$include" can be any valid "commands" syntax command ("$append", "$erase",
// "$include", "$object", or "$set").

// Currently valid "commands syntax" DataStore commands.
static const HString kAppendOp("$append");
static const HString kEraseOp("$erase");
static const HString kIncludeOp("$include");
static const HString kObjectOp("$object");
static const HString kSetOp("$set");

// Error reporting utilities.
#if SEOUL_LOGGING_ENABLED
static inline String NodeToString(const DataStore& ds, const DataNode& node)
{
	String s;
	ds.ToString(node, s, false, 0, true);
	return s;
}

/** Utility because the Core project does not have access to reflection - for error reporting. */
static inline Byte const* TypeToString(const DataNode& node)
{
	switch (node.GetType())
	{
	case DataNode::kNull: return "Null";
	case DataNode::kBoolean: return "Boolean";
	case DataNode::kUInt32: return "UInt32";
	case DataNode::kInt32Big: return "Int32Big";
	case DataNode::kInt32Small: return "Int32Small";
	case DataNode::kFloat31: return "Float31";
	case DataNode::kFloat32: return "Float32";
	case DataNode::kFilePath: return "FilePath";
	case DataNode::kTable: return "Table";
	case DataNode::kArray: return "Array";
	case DataNode::kString: return "String";
	case DataNode::kInt64: return "Int64";
	case DataNode::kUInt64: return "UInt64";
	default:
		return "Unknown";
	};
}

#define CMDSTR(val) NodeToString(cmdStore, (val)).CStr()
#define CMD_ERR(fmt, ...) \
	if (kLogParseErrors == (kLogParseErrors & uFlags)) \
	{ \
		SEOUL_WARN("%s(%d): " fmt, sBaseFilename.CStr(), uCmd, ##__VA_ARGS__); \
	}
#else
#define CMDSTR(val) ""
#define CMD_ERR(fmt, ...) ((void)0)
#endif // /#if SEOUL_LOGGING_ENABLED

namespace
{

// Utility, marks an identifier or index at any level
// of path resolution.
struct IdentOrIndex
{
	/** Currently an identifier if true, otherwise an index. */
	Bool IsIdent() const { return m_bIdent; }

	// Read access - does not mutate the current type.
	HString ReadIdent() const { return m_Ident; }
	UInt32 ReadIndex() const { return m_uIndex; }

	// Write access - mutates the current type of node (ident or index).
	HString& WriteIdent() { m_bIdent = true; m_uIndex = 0u; return m_Ident; }
	UInt32& WriteIndex() { m_bIdent = false; m_Ident = HString(); return m_uIndex; }

private:
	HString m_Ident;
	UInt32 m_uIndex = 0u;
	Bool m_bIdent = false;
};

}

/**
 * A "$search" command finds the entry in an array with the matching key-value pair.
 *
 * For example, ["$search", "foo", true] finds the first object entry in the target array
 * that has a property named "foo" with a value of true.
 */
static Bool ResolveSearch(
	const String& sBaseFilename,
	UInt32 uCmd,
	UInt32 uFlags,
	const DataStore& cmdStore,
	const DataNode& search,
	const DataStore& ds,
	const DataNode& node,
	IdentOrIndex& rKey)
{
	// node must be an array for a search.
	if (!node.IsArray())
	{
		CMD_ERR("path-resolve: attempting to perform array search on an element that is not an array");
		return false;
	}

	// Extract bits - for searching.
	DataNode val;
	HString key;
	if (!cmdStore.GetValueFromArray(search, 1u, val) || !cmdStore.AsString(val, key))
	{
		CMD_ERR("path-resolve: array search requires 2 arguments, first argument not defined or is not a string");
		return false;
	}
	if (!cmdStore.GetValueFromArray(search, 2u, val))
	{
		CMD_ERR("path-resolve: array search requires 2 arguments, second argument not defined");
		return false;
	}

	// Enumerate and search.
	DataNode cmp;
	DataNode tmp;
	UInt32 u = 0u;
	SEOUL_VERIFY(ds.GetArrayCount(node, u)); // Must succeed, verified as an array above.

	// O(n) walk of the array to perform the search.
	for (UInt32 i = 0u; i < u; ++i)
	{
		// Get element - must succeed, verified size above.
		SEOUL_VERIFY(ds.GetValueFromArray(node, i, tmp));

		// Get and check.
		if (ds.GetValueFromTable(tmp, key, cmp))
		{
			if (DataStore::Equals(cmdStore, val, ds, cmp))
			{
				// Done, found.
				rKey.WriteIndex() = i;
				return true;
			}
		}
	}

	CMD_ERR("path-resolve: array search on property '%s' failed, could find an element with value '%s'", key.CStr(), CMDSTR(val));
	return false;
}

/**
 * Mutation commands ("$append", "$erase", and "$set") all operate
 * on a path (e.g. ["$set", "a", "b", "c", true] will set the
 * value of a["b"]["c"] to true, where a, b, and c are objects.
 *
 * This function resolves the path to the target node and outputs
 * the result to rNode and rKey, if successful.
 */
static Bool ResolvePath(
	const String& sBaseFilename,
	UInt32 uCmd,
	UInt32 uFlags,
	const DataStore& cmdStore,
	const DataNode& cmd,
	DataStore& ds,
	Bool bErase,
	DataNode& rNode,
	IdentOrIndex& rKey)
{
	// Get count - must succeed, caller verified cmd is an array.
	UInt32 uCount = 0u;
	SEOUL_VERIFY(cmdStore.GetArrayCount(cmd, uCount));

	// Check argument length.
	if ((!bErase && uCount < 3u) || (bErase && uCount < 2u))
	{
		CMD_ERR("path-resolve: insufficient arguments %u for cmd", uCount);
		return false;
	}

	// Get the first - must always be an identifier.
	DataNode val;
	if (!cmdStore.GetValueFromArray(cmd, 1u  /* 0u is the command */, val) || !cmdStore.AsString(val, rKey.WriteIdent()))
	{
		CMD_ERR("path-resolve: path part 1 not defined or not a string");
		return false;
	}

	// Adjust count.
	uCount -= (bErase ? 0u : 1u);

	// Now perform further resolve.
	for (UInt32 i = 2u; i < uCount; ++i)
	{
		// Cound and type verified, must succeed.
		SEOUL_VERIFY(cmdStore.GetValueFromArray(cmd, i, val));

		// If we need to implicitly create the next level, determine whether it
		// should be an array or a table.
		auto const bNextArray = !val.IsString();

		// Current node is a table, handle accordingly.
		if (rNode.IsTable())
		{
			// Error if we were expecting an array.
			if (!rKey.IsIdent())
			{
				CMD_ERR("path-resolve: index '%u' specified but container is a table, not an array", rKey.ReadIndex());
				return false;
			}

			// "Implicit" case - if the target does not yet access, insert an empty
			// container of the appropriate type as implied by the type of key.
			if (!ds.GetValueFromTable(rNode, rKey.ReadIdent(), rNode))
			{
				if (bNextArray)
				{
					// Must succeed, types validated.
					SEOUL_VERIFY(ds.SetArrayToTable(rNode, rKey.ReadIdent()));
				}
				else
				{
					// Must succeed, types validated.
					SEOUL_VERIFY(ds.SetTableToTable(rNode, rKey.ReadIdent()));
				}

				// Types validated, must succeed.
				SEOUL_VERIFY(ds.GetValueFromTable(rNode, rKey.ReadIdent(), rNode));
			}
		}
		// Current node is an array, handle accordingly.
		else
		{
			// Error if we were expecting a table.
			if (rKey.IsIdent())
			{
				CMD_ERR("path-resolve: key '%s' specified but container is an array, not a table", rKey.ReadIdent().CStr());
				return false;
			}

			// "Implicit" case - if the target does not yet access, insert an empty
			// container of the appropriate type as implied by the type of key.
			if (!ds.GetValueFromArray(rNode, rKey.ReadIndex(), rNode))
			{
				if (bNextArray)
				{
					// Must succeed, types validated.
					SEOUL_VERIFY(ds.SetArrayToArray(rNode, rKey.ReadIndex()));
				}
				else
				{
					// Must succeed, types validated.
					SEOUL_VERIFY(ds.SetTableToArray(rNode, rKey.ReadIndex()));
				}

				// Types validated, must succeed.
				SEOUL_VERIFY(ds.GetValueFromArray(rNode, rKey.ReadIndex(), rNode));
			}
		}

		// Special case handling for search.
		if (val.IsArray())
		{
			if (!ResolveSearch(sBaseFilename, uCmd, uFlags, cmdStore, val, ds, rNode, rKey))
			{
				return false;
			}
		}
		// Key for a table.
		else if (val.IsString())
		{
			// Must succeed, type validated.
			SEOUL_VERIFY(cmdStore.AsString(val, rKey.WriteIdent()));
		}
		// Else, must be an unsigned index for an array.
		else
		{
			if (!cmdStore.AsUInt32(val, rKey.WriteIndex()))
			{
				CMD_ERR("path-resolve: path part %u is of type %s, must be an integer or a string.", i, TypeToString(val));
				return false;
			}
		}
	}

	return true;
}

/**
 * true if a DataStore contains JSON commands.
 */
Bool DataStoreParser::IsJsonCommandFile(const DataStore& ds)
{
	// Must be an array.
	if (!ds.GetRootNode().IsArray()) { return false; }

	// Must have at least 1 entry.
	UInt32 uCount = 0u;
	ds.GetArrayCount(ds.GetRootNode(), uCount);
	if (0u == uCount) { return false; }

	// Elements must be arrays (we only check the first).
	DataNode sub;
	ds.GetValueFromArray(ds.GetRootNode(), 0u, sub);
	if (!sub.IsArray()) { return false; }

	// First element must be a known operator type.
	DataNode val;
	Byte const* s = nullptr;
	UInt32 u = 0u;
	if (!ds.GetValueFromArray(sub, 0u, val) || !ds.AsString(val, s, u)) { return false; }

	// Must start with a '$'.
	if (0u == u || '$' != s[0]) { return false; }

	// Check for known operators.
	HString cmd;
	if (!HString::Get(cmd, s, u)) { return false; }

	// Check.
	if (cmd == kAppendOp ||
		cmd == kEraseOp ||
		cmd == kIncludeOp ||
		cmd == kObjectOp ||
		cmd == kSetOp)
	{
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * Used to check if the command at the given index in the array
 * of commands (in cmds) is an object command (e.g. ["$object", "a"]
 */
static Bool IsObjectCommand(
	const DataStore& cmdStore,
	const DataNode& cmds,
	UInt32 uCmd)
{
	// Must successful get a command from the specified index
	// and then get the value at index zero of the command, which
	// will be the command type itself.
	DataNode cmd;
	if (!cmdStore.GetValueFromArray(cmds, uCmd, cmd)) { return false; }
	if (!cmdStore.GetValueFromArray(cmd, 0u, cmd)) { return false; }

	// Validate the command.
	HString h;
	if (!cmdStore.AsString(cmd, h)) { return false; }
	return (kObjectOp == h);
}

// Similar to ResolveCommandFile, but "merges" into rResolved. As a result,
// rResolved *must not* be the same DataStore as cmdStore.
Bool DataStoreParser::ResolveCommandFileInPlace(
	const Resolver& includeResolver,
	const String& sBaseFilename,
	const DataStore& cmdStore,
	DataStore& rResolved,
	DataNode& target,
	UInt32 uFlags /*= 0u*/)
{
	// Process.
	auto const root = cmdStore.GetRootNode();

	// Handling for includes.
	auto const sBasePath(Path::GetDirectoryName(sBaseFilename));

	// Get the number of commands and then iterate and process.
	DataNode cmd;
	DataNode val;
	UInt32 uCmds = 0u;
	(void)cmdStore.GetArrayCount(root, uCmds);
	for (UInt32 uCmd = 0u; uCmd < uCmds; ++uCmd)
	{
		// Get the current command.
		cmdStore.GetValueFromArray(root, uCmd, cmd);

		// Get the cmd type.
		Byte const* s = nullptr;
		UInt32 u = 0u;
		if (!cmdStore.GetValueFromArray(cmd, 0u, val) || !cmdStore.AsString(val, s, u))
		{
			CMD_ERR("cmd is not a string: '%s'", CMDSTR(val));
			return false;
		}

		// Convert the command string to an hstring for processing.
		HString c;
		if (!HString::Get(c, s, u))
		{
			CMD_ERR("cmd is not known: %s", String(s, u).CStr());
			return false;
		}

		// "$include" command.
		if (kIncludeOp == c)
		{
			// Get file.
			String sPath;
			if (!cmdStore.GetValueFromArray(cmd, 1u, val) || !cmdStore.AsString(val, sPath))
			{
				CMD_ERR("$include requires 1 string argument");
				return false;
			}

			// Get target.
			String s;
			SEOUL_VERIFY(Path::CombineAndSimplify(sBasePath, sPath, s));

			// If the very first include and if the include is immediately
			// followed by an object command, we can just clone the resolved
			// data store.
			if (rResolved.GetRootNode().IsNull() && IsObjectCommand(cmdStore, root, uCmd + 1u))
			{
				// Perform the resolve.
				auto p(includeResolver(s, true));
				if (!p.IsValid())
				{
					CMD_ERR("$include \"%s\" failed to resolve, check for typo or missing file", s.CStr());
					return false;
				}

				// Populate from.
				rResolved.CopyFrom(*p);
			}
			// Otherwise, need to handle specially - somewhat like
			// multiple inheritance. Any include after the ds has
			// some contents must instead by loaded as its
			// raw commands form, and then applied to the in-progress.
			// DataStore.
			else
			{
				// Perform the resolve.
				auto p(includeResolver(s, false));
				if (!p.IsValid())
				{
					CMD_ERR("$include \"%s\" failed to resolve, check for typo or missing file", s.CStr());
					return false;
				}

				// If an inner commands file, resolve on top.
				if (DataStoreParser::IsJsonCommandFile(*p))
				{
					if (!ResolveCommandFileInPlace(
						includeResolver,
						s,
						*p,
						rResolved,
						target,
						uFlags))
					{
						return false;
					}
				}
				// Otherwise, just deep copy into the target.
				else
				{
					// Populate the target root if necessary.
					if (rResolved.GetRootNode().IsNull()) { rResolved.MakeTable(); }

					if (!rResolved.DeepCopy(*p, p->GetRootNode(), rResolved.GetRootNode(), true))
					{
						CMD_ERR("$include \"%s\" failed, file exists, is included file a root array or otherwise invalid (must be a table)?", s.CStr());
						return false;
					}
				}
			}
		}
		// "$object" command, sets the active target table of mutation commands.
		else if (kObjectOp == c)
		{
			// Get to name.
			HString to;
			if (!cmdStore.GetValueFromArray(cmd, 1u, val) || !cmdStore.AsString(val, to))
			{
				CMD_ERR("$object requires at least 1 string argument");
				return false;
			}

			// If op only has 2 arguments ("$object" and the name), then we
			// check for a table at the target and if not yet defined,
			// we create an empty table. Otherwise, check for from name
			// and perform a deep copy.
			UInt32 uArgs = 0u;
			// Must succeed, type validated.
			SEOUL_VERIFY(cmdStore.GetArrayCount(cmd, uArgs));

			// Done if 2 (this is an object command of the form ["$object", "a"]
			if (2u == uArgs)
			{
				// Check.
				if (rResolved.GetValueFromTable(rResolved.GetRootNode(), to, target))
				{
					// Sanity.
					if (!target.IsTable())
					{
						CMD_ERR("$object table '%s' already exists but is not a table", to.CStr());
						return false;
					}

					// Done.
					continue;
				}

				// Populate the target root if necessary.
				if (rResolved.GetRootNode().IsNull()) { rResolved.MakeTable(); }

				// Set an empty table - must succeed, since either we just
				// set the root to a table in the previous line, or all other
				// paths would have set it to a table ($include can't include
				// root array .json files).
				SEOUL_VERIFY(rResolved.SetTableToTable(rResolved.GetRootNode(), to));

				// Set active - must succeed, types validated.
				SEOUL_VERIFY(rResolved.GetValueFromTable(rResolved.GetRootNode(), to, target));

				// Done, continue.
				continue;
			}

			// Must be a parent to inherit from at index 2 of the command.
			HString from;
			if (!cmdStore.GetValueFromArray(cmd, 2u, val) || !cmdStore.AsString(val, from))
			{
				CMD_ERR("$object parent is undefined or not a string");
				return false;
			}

			// Get the from now to copy.
			DataNode fromNode;
			if (!rResolved.GetValueFromTable(rResolved.GetRootNode(), from, fromNode))
			{
				CMD_ERR("$object parent '%s' does not exist", from.CStr());
				return false;
			}

			// Populate the target root if necessary.
			if (rResolved.GetRootNode().IsNull()) { rResolved.MakeTable(); }

			// Perform a deep copy - this initializes the target table with the body of the parent/from.
			if (!rResolved.DeepCopyToTable(rResolved, fromNode, rResolved.GetRootNode(), to))
			{
				CMD_ERR("$object parent '%s' to child '%s' copy operation failed, check for duplicate keys or an existing element at the target.", from.CStr(), to.CStr());
				return false;
			}

			// Set active - must succeed, types validated.
			SEOUL_VERIFY(rResolved.GetValueFromTable(rResolved.GetRootNode(), to, target));
		}
		// "$append", "$erase", or "$set" command.
		else if (kAppendOp == c || kEraseOp == c || kSetOp == c)
		{
			// Implicitly the root if not otherwise set.
			if (target.IsNull())
			{
				if (rResolved.GetRootNode().IsNull()) { rResolved.MakeTable(); }
				target = rResolved.GetRootNode();
			}

			// Resolve the path.
			DataNode lookup = target;
			IdentOrIndex key;
			if (!ResolvePath(sBaseFilename, uCmd, uFlags, cmdStore, cmd, rResolved, (kEraseOp == c), lookup, key))
			{
				CMD_ERR("mutation op path resolve failed, check for missing dependencies (e.g. missing table or array along the path, or a search target that is not an array)");
				return false;
			}

			// Now perform the operation.
			if (kEraseOp == c)
			{
				// Perform the erase, from array or table.
				if (lookup.IsArray())
				{
					// Must be an index for an array.
					if (key.IsIdent())
					{
						CMD_ERR("$erase at key '%s' but container is an array", key.ReadIdent().CStr());
						return false;
					}

					// Erase failure likely means out-of-range.
					if (!rResolved.EraseValueFromArray(lookup, key.ReadIndex()))
					{
						CMD_ERR("$erase operation at element '%u' failed, check for out-of-range (element not defined?)", key.ReadIndex());
						return false;
					}
				}
				else
				{
					// Must be a key for a table.
					if (!key.IsIdent())
					{
						CMD_ERR("$erase at element '%u' but container is a table", key.ReadIndex());
						return false;
					}

					// Erase failure likely means invalid key.
					if (!rResolved.EraseValueFromTable(lookup, key.ReadIdent()))
					{
						CMD_ERR("$erase operation at key '%s' failed, check for missing element (key not defined in table?)", key.ReadIdent().CStr());
						return false;
					}
				}
			}
			else
			{
				// Collapse append so it can be handled in the same manner as set.
				if (kAppendOp == c)
				{
					// Append to an element of an array.
					if (lookup.IsArray())
					{
						if (key.IsIdent())
						{
							CMD_ERR("$append at key '%s' but container is an array", key.ReadIdent().CStr());
							return false;
						}

						// Value is already an array, so switch the lookup to the existing
						// array and then update the lookup index to be the array count
						// of that array, since we're appending to the end.
						DataNode existing;
						if (rResolved.GetValueFromArray(lookup, key.ReadIndex(), existing))
						{
							if (!existing.IsArray())
							{
								CMD_ERR("$append target at element '%u' exists but it is not an array", key.ReadIndex());
								return false;
							}
							else
							{
								// Must succeed, types verified.
								SEOUL_VERIFY(rResolved.GetArrayCount(existing, key.WriteIndex()));
								lookup = existing;
							}
						}
						// Implicit case - for $append, if the target is empty, then we
						// insert an empty array to append to.
						else
						{
							// Types verified, must succeed.
							SEOUL_VERIFY(rResolved.SetArrayToArray(lookup, key.ReadIndex()));
							SEOUL_VERIFY(rResolved.GetValueFromArray(lookup, key.ReadIndex(), lookup));
							key.WriteIndex() = 0u;
						}
					}
					// Append to a value in a table.
					else
					{
						if (!key.IsIdent())
						{
							CMD_ERR("$append at key '%u' but container is a table", key.ReadIndex());
							return false;
						}

						// Value is already an array, so switch the lookup to the existing
						// array and then update the lookup index to be the array count
						// of that array, since we're appending to the end.
						DataNode existing;
						if (rResolved.GetValueFromTable(lookup, key.ReadIdent(), existing))
						{
							if (!existing.IsArray())
							{
								CMD_ERR("$append target at key '%s' exists but it is not an array", key.ReadIdent().CStr());
								return false;
							}
							else
							{
								// Must succeed, types verified.
								SEOUL_VERIFY(rResolved.GetArrayCount(existing, key.WriteIndex()));
								lookup = existing;
							}
						}
						// Implicit case - for $append, if the target is empty, then we
						// insert an empty array to append to.
						else
						{
							// Types verified, must succeed.
							SEOUL_VERIFY(rResolved.SetArrayToTable(lookup, key.ReadIdent()));
							SEOUL_VERIFY(rResolved.GetValueFromTable(lookup, key.ReadIdent(), lookup));
							key.WriteIndex() = 0u;
						}
					}
				}

				// Get the value.
				DataNode toset;
				UInt32 uCount = 0u;
				// Must succeed, type validated above.
				SEOUL_VERIFY(cmdStore.GetArrayCount(cmd, uCount));
				// Must succeed, count validated above.
				SEOUL_ASSERT(uCount > 1u);
				// Must succeed, type validated above.
				SEOUL_VERIFY(cmdStore.GetValueFromArray(cmd, uCount - 1u, toset));

				// Process the set into an array.
				if (lookup.IsArray())
				{
					if (key.IsIdent())
					{
						CMD_ERR("mutation at key '%s' but container is an array", key.ReadIdent().CStr());
						return false;
					}

					// TODO: Maintaining old semantics from our deprecated INI file syntax - don't
					// merge, so clear. Need to do this in a more efficient/less redundant way with above code.
					if (toset.IsArray() || toset.IsTable())
					{
						rResolved.SetNullValueToArray(lookup, key.ReadIndex());
					}

					// Must succeed due to type validation and overwrite = true.
					SEOUL_VERIFY(rResolved.DeepCopyToArray(cmdStore, toset, lookup, key.ReadIndex(), true));
				}
				// Process the set into a table.
				else
				{
					if (!key.IsIdent())
					{
						CMD_ERR("mutation at element '%u' but container is a table", key.ReadIndex());
						return false;
					}

					// TODO: Maintaining old semantics from our deprecated INI file syntax - don't
					// merge, so clear. Need to do this in a more efficient/less redundant way with above code.
					if (toset.IsArray() || toset.IsTable())
					{
						rResolved.SetNullValueToTable(lookup, key.ReadIdent());
					}

					// Must succeed due to type validation and overwrite = true.
					SEOUL_VERIFY(rResolved.DeepCopyToTable(cmdStore, toset, lookup, key.ReadIdent(), true));
				}
			}
		}
		// Final fall through - this case happens if the command name portion of the command is
		// a valid HString but not a valid command.
		else
		{
			CMD_ERR("cmd '%s' is unknown or unsupported, check for a typo", c.CStr());
			return false;
		}
	}

	// Done.
	return true;
}

/**
 * JSON command files are just JSON files with a certain structure (duck typing).
 *
 * Somewhat similar to a JSON patch file (http://jsonpatch.com/), they are a series
 * of commands that are resolved "in place" in order to generate a flat JSON blob
 * in a DataStore.
 *
 * They are meant to match similar functionality that existed in SeoulEngine's now
 * deprecated INI syntax.
 */
Bool DataStoreParser::ResolveCommandFile(
	const Resolver& includeResolver,
	const String& sBaseFilename,
	const DataStore& cmdStore,
	DataStore& rResolved,
	UInt32 uFlags /*= 0u*/)
{
	// Identical to ResolveCommandFileInPlace(), except that
	// the DataStore and active target are fresh.
	DataStore ds;
	DataNode target;
	if (!ResolveCommandFileInPlace(includeResolver, sBaseFilename, cmdStore, ds, target, uFlags))
	{
		return false;
	}

	rResolved.Swap(ds);
	return true;
}

#undef CMD_ERR

// TODO: Defined here so we don't include rapidjson headers in multiple CPP files.
// That's a hacky workaround but is convenient - likely, we will need to replace this
// with a more sophisticated regex parser someday, and we can resolve this niggle
// at that time.
Regex::Regex(const String& sRegex)
	: m_p(SEOUL_NEW(MemoryBudgets::DataStore) rapidjson::SeoulRegexType(sRegex.CStr(), &rapidjson::s_Allocator))
{
}

Regex::~Regex()
{
	auto p = (rapidjson::SeoulRegexType*)m_p;
	m_p = nullptr;

	SafeDelete(p);
}

Bool Regex::IsExactMatch(Byte const* sInput) const
{
	auto p = (rapidjson::SeoulRegexType*)m_p;
	if (!p->IsValid())
	{
		return false;
	}

	rapidjson::SeoulRegexSearchType search(*p, &rapidjson::s_Allocator);
	return search.Match(sInput);
}

Bool Regex::IsMatch(Byte const* sInput) const
{
	auto p = (rapidjson::SeoulRegexType*)m_p;
	if (!p->IsValid())
	{
		return false;
	}

	rapidjson::SeoulRegexSearchType search(*p, &rapidjson::s_Allocator);
	return search.Search(sInput);
}

} // namespace Seoul

namespace
{

static inline Seoul::UInt32 GetFloat32Hash(Seoul::Double fIn)
{
	using namespace Seoul;
	union
	{
		UInt32 u;
		Float32 f;
	};
	if (IsNaN(fIn)) { u = kuDataNodeCanonicalNaNBits; }
	else { f = (Float32)fIn; }

	return Seoul::GetHash(u);
}

static inline Seoul::UInt32 GetFloat32Hash(Seoul::Float32 fIn)
{
	using namespace Seoul;
	union
	{
		UInt32 u;
		Float32 f;
	};
	if (IsNaN(fIn)) { u = kuDataNodeCanonicalNaNBits; }
	else { f = fIn; }

	return Seoul::GetHash(u);
}

} // namespace anonymous

namespace rapidjson
{

using namespace Seoul;

/** rapidjson handler for generating hinting data for printing. */
class SeoulRapidJsonHintHandler SEOUL_SEALED
{
public:
	static const Seoul::UInt32 kuInitialStack = 16u;

	SeoulRapidJsonHintHandler(Seoul::Byte const* s)
		: m_s(s)
		, m_sCommentBegin(nullptr)
		, m_sCommentEnd(nullptr)
		, m_vStack()
		, m_pLast()
	{
		m_vStack.Reserve(kuInitialStack);
	}

	Seoul::Bool Null()
	{
		return Leaf(Seoul::GetHash(0u));
	}

	Seoul::Bool Bool(Seoul::Bool b)
	{
		return Leaf(b, Seoul::GetHash(b));
	}

	void Comment(size_t zBegin, size_t zEnd)
	{
		if (nullptr == m_sCommentBegin)
		{
			m_sCommentBegin = (m_s + zBegin);
		}
		m_sCommentEnd = (m_s + zEnd);
	}

	Seoul::Bool Int(Seoul::Int32 i)
	{
		return Leaf((Seoul::Int64)i, Seoul::GetHash(i));
	}

	Seoul::Bool Uint(Seoul::UInt32 u)
	{
		return Leaf((Seoul::UInt64)u, Seoul::GetHash(u));
	}

	Seoul::Bool Int64(Seoul::Int64 i)
	{
		return Leaf(i, Seoul::GetHash(i));
	}

	Seoul::Bool Uint64(Seoul::UInt64 u)
	{
		return Leaf(u, Seoul::GetHash(u));
	}

	Seoul::Bool Double(Seoul::Double d)
	{
		return Leaf(d, GetFloat32Hash(d));
	}

	Seoul::Bool RawNumber(Seoul::Byte const* s, Seoul::UInt32 uSizeInBytes, Seoul::Bool bCopy)
	{
		SEOUL_FAIL("Unexpected call to raw number.");
		return false;
	}

	Seoul::Bool String(Seoul::Byte const* s, Seoul::UInt32 uSizeInBytes, Seoul::Bool bCopy)
	{
		return Leaf(s, uSizeInBytes, bCopy, Seoul::GetHash(s, uSizeInBytes));
	}

	Seoul::Bool StartObject()
	{
		StackFrame frame;
		frame.m_p.Reset(SEOUL_NEW(MemoryBudgets::DataStore) DataStoreHintTable);
		frame.m_TableKey = HString();

		// New object is an entry in existing container.
		if (!m_vStack.IsEmpty())
		{
			auto const& back = m_vStack.Back();
			back.m_p->Append(back.m_TableKey, frame.m_p);
		}

		// New active object.
		m_vStack.PushBack(frame);

		m_pLast = frame.m_p;
		ApplyComment();
		return true;
	}

	Seoul::Bool Key(Seoul::Byte const* s, Seoul::UInt32 uSizeInBytes, Seoul::Bool bCopy)
	{
		m_vStack.Back().m_TableKey = HString(s, uSizeInBytes);
		return true;
	}

	Seoul::Bool EndObject(Seoul::UInt32 uMembers)
	{
		return End();
	}

	Seoul::Bool StartArray()
	{
		StackFrame frame;
		frame.m_p.Reset(SEOUL_NEW(MemoryBudgets::DataStore) DataStoreHintArray);
		frame.m_TableKey = HString();

		// New object is an entry in existing container.
		if (!m_vStack.IsEmpty())
		{
			auto const& back = m_vStack.Back();
			back.m_p->Append(back.m_TableKey, frame.m_p);
		}

		// New active array.
		m_vStack.PushBack(frame);

		m_pLast = frame.m_p;
		ApplyComment();
		return true;
	}

	Seoul::Bool EndArray(Seoul::UInt32 uElements)
	{
		return End();
	}

	Seoul::Bool End()
	{
		m_pLast = m_vStack.Back().m_p;
		m_sCommentBegin = nullptr;
		m_sCommentEnd = nullptr;
		m_vStack.PopBack();
		return true;
	}

	struct StackFrame SEOUL_SEALED
	{
		SharedPtr<DataStoreHint> m_p;
		HString m_TableKey;
	};

	typedef Vector<StackFrame, MemoryBudgets::DataStore> Stack;

	const SharedPtr<DataStoreHint>& GetLast() const { return m_pLast; }
	const Stack& GetStack() const { return m_vStack; }

private:
	Seoul::Byte const* const m_s;
	Seoul::Byte const* m_sCommentBegin;
	Seoul::Byte const* m_sCommentEnd;
	Stack m_vStack;
	SharedPtr<DataStoreHint> m_pLast;

	void ApplyComment()
	{
		if (nullptr != m_sCommentBegin)
		{
			m_pLast->SetComment(m_sCommentBegin, m_sCommentEnd);
		}
		m_sCommentBegin = nullptr;
		m_sCommentEnd = nullptr;
	}

	Seoul::Bool Leaf(UInt32 uHash)
	{
		SharedPtr<DataStoreHint> p(SEOUL_NEW(MemoryBudgets::DataStore) DataStoreHintLeaf(uHash));
		return Node(p);
	}

	template <typename T>
	Seoul::Bool Leaf(T v, UInt32 uHash)
	{
		SharedPtr<DataStoreHint> p(SEOUL_NEW(MemoryBudgets::DataStore) DataStoreHintLeaf(v, uHash));
		return Node(p);
	}

	Seoul::Bool Leaf(Seoul::Byte const* s, UInt32 uSizeInBytes, Seoul::Bool bCopy, UInt32 uHash)
	{
		SharedPtr<DataStoreHint> p(SEOUL_NEW(MemoryBudgets::DataStore) DataStoreHintLeaf(s, uSizeInBytes, bCopy, uHash));
		return Node(p);
	}

	Seoul::Bool Node(const SharedPtr<DataStoreHint>& p)
	{
		auto& r(m_vStack.Back());
		r.m_p->Append(r.m_TableKey, p);
		m_pLast = p;
		ApplyComment();
		return true;
	}

	SEOUL_DISABLE_COPY(SeoulRapidJsonHintHandler);
}; // class SeoulRapidJsonHintHandler

/**
 * rapidjson handler for generating hinting data for printing
 * when also flattening the data.
 *
 * In this case, "flattening" occurs when a JSON "commands"
 * style file is converted into its resulting JSON representation.
 */
class SeoulRapidJsonHintWithFlatteningHandler SEOUL_SEALED
{
public:
	static const Seoul::UInt32 kuInitialStack = 16u;

	SeoulRapidJsonHintWithFlatteningHandler(Seoul::Byte const* s, UInt32 u)
		: m_pRoot(SEOUL_NEW(MemoryBudgets::DataStore) DataStoreHintTable)
		, m_pTarget(m_pRoot)
		, m_ValueHandler(s)
		, m_sBegin(s)
		, m_sEnd(s + u)
	{
	}

	Seoul::Bool Null()
	{
		if (!m_ValueHandler.Null()) { return false; }
		return true;
	}

	Seoul::Bool Bool(Seoul::Bool b)
	{
		if (!m_ValueHandler.Bool(b)) { return false; }
		return true;
	}

	void Comment(size_t zBegin, size_t zEnd)
	{
		m_ValueHandler.Comment(zBegin, zEnd);
	}

	Seoul::Bool Int(Seoul::Int32 i)
	{
		if (!m_ValueHandler.Int(i)) { return false; }
		return true;
	}

	Seoul::Bool Uint(Seoul::UInt32 u)
	{
		if (!m_ValueHandler.Uint(u)) { return false; }
		return true;
	}

	Seoul::Bool Int64(Seoul::Int64 i)
	{
		if (!m_ValueHandler.Int64(i)) { return false; }
		return true;
	}

	Seoul::Bool Uint64(Seoul::UInt64 u)
	{
		if (!m_ValueHandler.Uint64(u)) { return false; }
		return true;
	}

	Seoul::Bool Double(Seoul::Double d)
	{
		if (!m_ValueHandler.Double(d)) { return false; }
		return true;
	}

	Seoul::Bool RawNumber(Seoul::Byte const* s, Seoul::UInt32 uSizeInBytes, Seoul::Bool bCopy)
	{
		SEOUL_FAIL("Unexpected call to raw number.");
		return false;
	}

	Seoul::Bool String(Seoul::Byte const* s, Seoul::UInt32 uSizeInBytes, Seoul::Bool /*bCopy*/)
	{
		// bCopy will always be false since we're not parsing insitu. Instead, we look
		// at the value of s, which will be into our buffer unless the string needed to
		// be escaped.
		auto const bCopy = (s < m_sBegin || (s + uSizeInBytes) > m_sEnd);

		if (!m_ValueHandler.String(s, uSizeInBytes, bCopy)) { return false; }
		return true;
	}

	Seoul::Bool StartObject()
	{
		if (!m_ValueHandler.StartObject()) { return false; }
		return true;
	}

	Seoul::Bool Key(Seoul::Byte const* s, Seoul::UInt32 uSizeInBytes, Seoul::Bool bCopy)
	{
		if (!m_ValueHandler.Key(s, uSizeInBytes, bCopy)) { return false; }
		return true;
	}

	Seoul::Bool EndObject(Seoul::UInt32 uMembers)
	{
		if (!m_ValueHandler.EndObject(uMembers)) { return false; }
		return true;
	}

	Seoul::Bool StartArray()
	{
		if (!m_ValueHandler.StartArray()) { return false; }
		return true;
	}

	Seoul::Bool EndArray(Seoul::UInt32 uElements)
	{
		if (!m_ValueHandler.EndArray(uElements)) { return false; }

		// If we just completed an array under the root array (stack
		// level will be at level 1, inside the root array).
		// process it as a command. This is the meat of the unique
		// functionality that SeoulRapidJsonHintWithFlatteningHandler
		// performs beyond its component SeoulRapidJsonHintHandler.
		if (m_ValueHandler.GetStack().GetSize() == 1u && m_ValueHandler.GetLast()->IsArray())
		{
			auto const& arr = *((DataStoreHintArray const*)m_ValueHandler.GetLast().GetPtr());

			// Get the command - if known, process. Otherwise ignore. We aren't aggressive
			// about failure since we're providing formatting hints, not semantic requirements.
			HString cmd;
			arr[0u]->AsString(cmd);

			// Object command setting the new root object.
			//
			// ["$object", "<object-name>", "<optional-object-parent-name>"]
			if (kObjectOp == cmd)
			{
				// Acquire an existing object.
				m_pTarget = (*m_pRoot)[arr[1u]];

				// If it doesn't exist already, create it.
				if (m_pTarget->IsNone())
				{
					m_pTarget.Reset(SEOUL_NEW(MemoryBudgets::DataStore) DataStoreHintTable);

					HString h;
					arr[1u]->AsString(h);
					m_pRoot->Append(h, m_pTarget);
				}

				// A second argument means the object should merge in a clone of another table.
				if (!arr[2u]->IsNone())
				{
					// TODO: Implement inheritance.
				}

				// Propagate command comment to object if the object doesn't already have one.
				if (arr.GetCommentBegin() != arr.GetCommentEnd() &&
					m_pTarget->GetCommentBegin() == m_pTarget->GetCommentEnd())
				{
					m_pTarget->SetComment(arr.GetCommentBegin(), arr.GetCommentEnd());
				}
			}
			// Common handling for mutation commands.
			//
			// ["$append", "path-part", ..., "path-part", <value>]
			// ["$erase", "path-part", ..., "path-part"]
			// ["$set", "path-part", ..., "path-part", <value>]
			else if (kAppendOp == cmd || kEraseOp == cmd || kSetOp == cmd)
			{
				// Erase commands need at least 2 parts (command and path)
				// Append/set require at least 3 (command, path, value).
				auto const uMin = (kEraseOp == cmd ? 2u : 3u);

				// Check and process.
				auto const uSize = arr.m_a.GetSize();
				if (uSize >= uMin)
				{
					// We resolve up to but not including the last part of
					// the path first.
					UInt32 uPathLength = (uSize - (uMin - 1u));

					// Resolve pContainer to the last container of the path chain
					// (this is why we subtract 1 above, since the last path part
					// is the key used on the last container to which the value
					// will be set).
					auto pContainer = m_pTarget;
					for (UInt32 i = 1u; i < uPathLength; ++i)
					{
						// If next element is not defined and op is anything other than
						// kEraseOp, implicitly create the container based on the type
						// of the *next* path element.
						if (kEraseOp != cmd)
						{
							if (((*pContainer)[arr[i]])->IsNone())
							{
								SharedPtr<DataStoreHint> pNewContainer;
								UInt64 uUnused;
								if (arr[i + 1]->AsUInt64(uUnused))
								{
									pNewContainer.Reset(SEOUL_NEW(MemoryBudgets::DataStore) DataStoreHintArray);
								}
								else
								{
									pNewContainer.Reset(SEOUL_NEW(MemoryBudgets::DataStore) DataStoreHintTable);
								}

								pContainer->Set(arr[i], pNewContainer);
							}
						}

						// Resolve.
						pContainer = (*pContainer)[arr[i]];
					}

					// Either the value to append/set, or the last
					// part of the path to erase.
					auto pLeaf(arr[uSize - 1u]);

					// If not an erase, propagate command comment.
					if (kEraseOp != cmd)
					{
						// Propagate command comment to object if the object doesn't already have one.
						if (arr.GetCommentBegin() != arr.GetCommentEnd() &&
							pLeaf->GetCommentBegin() == pLeaf->GetCommentEnd())
						{
							pLeaf->SetComment(arr.GetCommentBegin(), arr.GetCommentEnd());
						}
					}

					// Perform the command.
					if (kEraseOp == cmd)
					{
						// Erase the last path part from the last container.
						pContainer->Erase(pLeaf);
					}
					else if (kAppendOp == cmd)
					{
						// Implicitly create an array if not already defined.
						if ((*pContainer)[arr[uSize - 2u]]->IsNone())
						{
							pContainer->Set(arr[uSize - 2u], SharedPtr<DataStoreHint>(SEOUL_NEW(MemoryBudgets::DataStore) DataStoreHintArray));
						}

						// Resolve the last container (exception to case above,
						// since the "leaf" of the path is expected to be an array
						// that we append to).
						pContainer = (*pContainer)[arr[uSize - 2u]];

						// Append to the target array.
						pContainer->Append(HString(), pLeaf);
					}
					else if (kSetOp == cmd)
					{
						// Set to the key of the last container.
						pContainer->Set(arr[uSize - 2u], pLeaf);
					}
				}
			}
			else if (kIncludeOp == cmd)
			{
				// TODO: Support for ["$include", ...] commands.
			}
		}

		// If we just completed the root array, glue any comment on the root
		// command list to the root table.
		if (m_ValueHandler.GetStack().GetSize() == 0u &&
			m_pRoot->GetCommentBegin() == m_pRoot->GetCommentEnd() &&
			m_ValueHandler.GetLast()->GetCommentBegin() != m_ValueHandler.GetLast()->GetCommentEnd())
		{
			m_pRoot->SetComment(
				m_ValueHandler.GetLast()->GetCommentBegin(),
				m_ValueHandler.GetLast()->GetCommentEnd());
		}

		return true;
	}

	const SharedPtr<DataStoreHint>& GetRoot() const { return m_pRoot; }

private:
	SharedPtr<DataStoreHint> m_pRoot;
	SharedPtr<DataStoreHint> m_pTarget;
	SeoulRapidJsonHintHandler m_ValueHandler;
	Seoul::Byte const* const m_sBegin;
	Seoul::Byte const* const m_sEnd;

	SEOUL_DISABLE_COPY(SeoulRapidJsonHintWithFlatteningHandler);
}; // class SeoulRapidJsonHintWithFlatteningHandler

} // namespace rapidjson

namespace Seoul
{

namespace
{

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

} // namespace anonymous

void DataStoreHintArray::Append(HString tableKey, const SharedPtr<DataStoreHint>& p)
{
	p->SetOrder(m_a.GetSize());
	m_a.PushBack(p);
}

void DataStoreHintArray::Erase(const SharedPtr<DataStoreHint>& pKey)
{
	// Can erase if the key is convertible to a uint
	// and is within the range of our array.
	UInt64 u = 0;
	if (pKey->AsUInt64(u) && u < (UInt64)m_a.GetSize())
	{
		// Perform the erase.
		m_a.Erase(m_a.Begin() + (size_t)u);

		// Now from the new element in the erased slot
		// to the end of the order, refresh order.
		for (auto i = m_a.Begin() + (size_t)u; m_a.End() != i; ++i)
		{
			(*i)->SetOrder((*i)->GetOrder() - 1u);
		}
	}
}

void DataStoreHintArray::Set(const SharedPtr<DataStoreHint>& pKey, const SharedPtr<DataStoreHint>& pValue)
{
	// Can set if the key is convertible to a uint.
	// If the index is beyond the end of our array,
	// then we fill in the missing array with Null
	// leaf values.
	UInt64 u = 0;
	if (pKey->AsUInt64(u))
	{
		// Fill in.
		while (m_a.GetSize() <= u)
		{
			SharedPtr<DataStoreHint> pLeaf(SEOUL_NEW(MemoryBudgets::DataStore) DataStoreHintLeaf(Seoul::GetHash(0u)));
			pLeaf->SetOrder(m_a.GetSize());
			m_a.PushBack(pLeaf);
		}

		// Set the order of the element we're about to set.
		pValue->SetOrder((UInt32)u);

		// Commit it.
		m_a[(UInt32)u] = pValue;
	}
}

void DataStoreHintArray::Finish(Bool bRoot /*= false*/)
{
	// Convenience - if we're the root,
	// and the first element doesn't have a comment,
	// we push our comment to that element. Normally
	// a comment on the very root element is not processed.
	if (bRoot)
	{
		if (GetCommentBegin() != GetCommentEnd() &&
			!m_a.IsEmpty() &&
			m_a.Front()->GetCommentBegin() == m_a.Front()->GetCommentEnd())
		{
			auto pBegin = GetCommentBegin();
			auto pEnd = GetCommentEnd();
			SetComment(nullptr, nullptr);
			m_a.Front()->SetComment(pBegin, pEnd);
		}
	}

	// Finish children first.
	for (auto& e : m_a)
	{
		e->Finish();
	}

	m_uHash = 0u;
	auto const uSize = m_a.GetSize();
	for (UInt32 i = 0u; i < uSize; ++i)
	{
		auto p(m_a[i]);
		auto const uHash(p->GetHash());
		if (!m_tByHash.Insert(uHash, (Int64)i).Second)
		{
			// If multiple resolves, insert a negative one to indicate no lookup.
			(void)m_tByHash.Overwrite(uHash, -1);
		}

		// Child containers do not contribute to the identifier hash.
		if (p->IsArray() || p->IsTable()) { continue; }

		// Accumulate.
		IncrementalHash(m_uHash, uHash);
	}
}

Bool DataStoreHintArray::IndexFromHash(UInt32 uHash, UInt32& ruInOutIndex) const
{
	Int64 i = -1;
	if (!m_tByHash.GetValue(uHash, i) || i < 0)
	{
		return false;
	}

	ruInOutIndex = (UInt32)i;
	return true;
}

SharedPtr<DataStoreHint> DataStoreHintArray::operator[](const SharedPtr<DataStoreHint>& pKey) const
{
	// Can query if u is a uint and is within the range
	// of our array.
	UInt64 u = 0u;
	if (pKey->AsUInt64(u) && u < (UInt64)m_a.GetSize())
	{
		return m_a[(UInt32)u];
	}
	// TODO: Add support for ["$search", ...] commands. Iterate the array,
	// look at each member as a table, then look for the corresponding key with
	// the corresponding value.

	// Otherwise we return the none placeholder.
	return SharedPtr<DataStoreHint>(SEOUL_NEW(MemoryBudgets::DataStore) DataStoreHintNone);
}

SharedPtr<DataStoreHint> DataStoreHintArray::operator[](HString h) const
{
	return SharedPtr<DataStoreHint>(SEOUL_NEW(MemoryBudgets::DataStore) DataStoreHintNone);
}

SharedPtr<DataStoreHint> DataStoreHintArray::operator[](UInt32 u) const
{
	if (u < m_a.GetSize())
	{
		return m_a[u];
	}

	return SharedPtr<DataStoreHint>(SEOUL_NEW(MemoryBudgets::DataStore) DataStoreHintNone);
}

DataStoreHintLeaf::DataStoreHintLeaf(UInt32 uHash)
	: DataStoreHint(Type::kLeaf)
{
	m_uHash = uHash;
	m_eValueType = ValueType::kNull;
}

DataStoreHintLeaf::DataStoreHintLeaf(Bool b, UInt32 uHash)
	: DataStoreHint(Type::kLeaf)
{
	m_uHash = uHash;
	m_v.b = b;
	m_eValueType = ValueType::kBool;
}

DataStoreHintLeaf::DataStoreHintLeaf(Double f, UInt32 uHash)
	: DataStoreHint(Type::kLeaf)
{
	m_uHash = uHash;
	m_v.f = f;
	m_eValueType = ValueType::kDouble;
}

DataStoreHintLeaf::DataStoreHintLeaf(Int64 i, UInt32 uHash)
	: DataStoreHint(Type::kLeaf)
{
	m_uHash = uHash;
	m_v.i = i;
	m_eValueType = ValueType::kInt64;
}

DataStoreHintLeaf::DataStoreHintLeaf(Byte const* s, UInt32 u, Bool bCopy, UInt32 uHash)
	: DataStoreHint(Type::kLeaf)
{
	m_uHash = uHash;
	m_uStringSize = u;

	// If we need to copy the string data, allocate a new block
	// and copy it through.
	if (bCopy)
	{
		m_v.s = (Byte const*)MemoryManager::Allocate(u + 1u, MemoryBudgets::DataStoreData);
		memcpy((void*)m_v.s, s, u);
		((Byte*)m_v.s)[u] = '\0';

		m_eValueType = ValueType::kStringHeap;
	}
	// Otherwise we can just reference the buffer. It is assumed to have a lifespan
	// longer than this DataStoreHintLeaf instance.
	else
	{
		m_v.s = s;
		m_eValueType = ValueType::kStringRef;
	}
}

DataStoreHintLeaf::DataStoreHintLeaf(UInt64 u, UInt32 uHash)
	: DataStoreHint(Type::kLeaf)
{
	m_uHash = uHash;
	m_v.u = u;
	m_eValueType = ValueType::kUInt64;
}

DataStoreHintLeaf::~DataStoreHintLeaf()
{
	if (ValueType::kStringHeap == m_eValueType)
	{
		MemoryManager::Deallocate((void*)m_v.s);
		m_v.s = nullptr;
	}
}

Bool DataStoreHintLeaf::AsString(HString& r) const
{
	// As string if one of the string types.
	if (ValueType::kStringHeap == m_eValueType || ValueType::kStringRef == m_eValueType)
	{
		r = HString(m_v.s, m_uStringSize);
		return true;
	}

	return false;
}

Bool DataStoreHintLeaf::AsUInt64(UInt64& r) const
{
	// As uint if the uint type.
	if (ValueType::kUInt64 == m_eValueType)
	{
		r = m_v.u;
		return true;
	}

	return false;
}

SharedPtr<DataStoreHint> DataStoreHintLeaf::operator[](const SharedPtr<DataStoreHint>& p) const
{
	return SharedPtr<DataStoreHint>(SEOUL_NEW(MemoryBudgets::DataStore) DataStoreHintNone);
}

SharedPtr<DataStoreHint> DataStoreHintLeaf::operator[](HString h) const
{
	return SharedPtr<DataStoreHint>(SEOUL_NEW(MemoryBudgets::DataStore) DataStoreHintNone);
}

SharedPtr<DataStoreHint> DataStoreHintLeaf::operator[](UInt32 u) const
{
	return SharedPtr<DataStoreHint>(SEOUL_NEW(MemoryBudgets::DataStore) DataStoreHintNone);
}

SharedPtr<DataStoreHint> DataStoreHintNone::operator[](const SharedPtr<DataStoreHint>& p) const
{
	return SharedPtr<DataStoreHint>(SEOUL_NEW(MemoryBudgets::DataStore) DataStoreHintNone);
}

SharedPtr<DataStoreHint> DataStoreHintNone::operator[](HString h) const
{
	return SharedPtr<DataStoreHint>(SEOUL_NEW(MemoryBudgets::DataStore) DataStoreHintNone);
}

SharedPtr<DataStoreHint> DataStoreHintNone::operator[](UInt32 u) const
{
	return SharedPtr<DataStoreHint>(SEOUL_NEW(MemoryBudgets::DataStore) DataStoreHintNone);
}

void DataStoreHintTable::Append(HString tableKey, const SharedPtr<DataStoreHint>& p)
{
	p->SetOrder(m_uNextOrder++);
	m_t.Overwrite(tableKey, p);
}

void DataStoreHintTable::Erase(const SharedPtr<DataStoreHint>& pKey)
{
	// Can erase if the key is convertible to a string.
	HString h;
	if (pKey->AsString(h))
	{
		m_t.Erase(h);
	}
}

void DataStoreHintTable::Set(const SharedPtr<DataStoreHint>& pKey, const SharedPtr<DataStoreHint>& pValue)
{
	// Can set if the key is convertible to a string.
	HString h;
	if (pKey->AsString(h))
	{
		// Set the order of the value we're about to set. Always comes after any
		// existing values.
		pValue->SetOrder(m_uNextOrder++);

		// Commit the value.
		m_t.Overwrite(h, pValue);
	}
}

void DataStoreHintTable::Finish(Bool bRoot /*= false*/)
{
	// Finish children first.
	for (auto const& pair : m_t)
	{
		pair.Second->Finish();
	}

	Vector<HString> v;
	for (auto const& pair : m_t)
	{
		v.PushBack(pair.First);
	}

	LexicalHStringSort sort;
	QuickSort(v.Begin(), v.End(), sort);

	// Convenience - if we're the root,
	// and the first element doesn't have a comment,
	// we push our comment to that element. Normally
	// a comment on the very root element is not processed.
	if (bRoot && !m_t.IsEmpty())
	{
		auto pFirst(*m_t.Find(v.Front()));
		if (GetCommentBegin() != GetCommentEnd() &&
			pFirst->GetCommentBegin() == pFirst->GetCommentEnd())
		{
			auto pBegin = GetCommentBegin();
			auto pEnd = GetCommentEnd();
			SetComment(nullptr, nullptr);
			pFirst->SetComment(pBegin, pEnd);
		}
	}

	m_uHash = 0u;
	for (auto const& e : v)
	{
		// Must exist.
		auto p(*m_t.Find(e));

		// Child containers do not contribute to the identifier hash.
		if (p->IsArray() || p->IsTable()) { continue; }

		// Accumulate.
		IncrementalHash(m_uHash, Seoul::GetHash(e));
		IncrementalHash(m_uHash, p->GetHash());
	}
}

SharedPtr<DataStoreHint> DataStoreHintTable::operator[](const SharedPtr<DataStoreHint>& pKey) const
{
	// Can query if the key is convertible to a string.
	HString h;
	if (pKey->AsString(h))
	{
		return (*this)[h];
	}

	// Return the none placeholder if not queryable.
	return SharedPtr<DataStoreHint>(SEOUL_NEW(MemoryBudgets::DataStore) DataStoreHintNone);
}

SharedPtr<DataStoreHint> DataStoreHintTable::operator[](HString h) const
{
	auto pp = m_t.Find(h);
	if (nullptr != pp)
	{
		return *pp;
	}

	return SharedPtr<DataStoreHint>(SEOUL_NEW(MemoryBudgets::DataStore) DataStoreHintNone);
}

SharedPtr<DataStoreHint> DataStoreHintTable::operator[](UInt32 u) const
{
	return SharedPtr<DataStoreHint>(SEOUL_NEW(MemoryBudgets::DataStore) DataStoreHintNone);
}

Bool DataStorePrinter::ParseHintsNoCopy(Byte const* s, UInt32 u, SharedPtr<DataStoreHint>& rp)
{
	using namespace rapidjson;

	// Skip the UTF8 BOM if it is present.
	s = SkipBom(s, u);

	// Setup for hint parsing.
	GenericReader<UTF8<>, UTF8<>, SeoulRapidJsonAllocator> reader(&s_Allocator);
	SeoulRapidJsonHintHandler handler(s);
	SeoulRangedReadonlyStringStream stream(s, s + u);

	// Execute.
	auto const result = reader.Parse<kuCommonFlags>(stream, handler);
	if (result.IsError())
	{
		return false;
	}

	// The last node tracked is the output on success.
	rp = handler.GetLast();
	rp->Finish(true);
	return true;
}

Bool DataStorePrinter::ParseHintsNoCopyWithFlattening(Byte const* s, UInt32 u, SharedPtr<DataStoreHint>& rp)
{
	using namespace rapidjson;

	// Skip the UTF8 BOM if it is present.
	s = SkipBom(s, u);

	// Setup for hint parsing.
	GenericReader<UTF8<>, UTF8<>, SeoulRapidJsonAllocator> reader(&s_Allocator);
	SeoulRapidJsonHintWithFlatteningHandler handler(s, u);
	SeoulRangedReadonlyStringStream stream(s, s + u);

	// Execute.
	auto const result = reader.Parse<kuCommonFlags>(stream, handler);
	if (result.IsError())
	{
		return false;
	}

	// The last node tracked is the output on success.
	rp = handler.GetRoot();
	rp->Finish(true);
	return true;
}

/** Pretty printing a DataStore. */
void DataStorePrinter::PrintWithHints(
	const DataStore& ds,
	const DataNode& root,
	const SharedPtr<DataStoreHint>& pHint,
	String& rs)
{
	String s;
	InternalPrintWithHints(ds, root, pHint, 0, s);
	rs.Swap(s);
}

/** Pretty printing a DataStore. */
void DataStorePrinter::PrintWithHints(
	const DataStore& ds,
	const SharedPtr<DataStoreHint>& pHint,
	String& rs)
{
	PrintWithHints(ds, ds.GetRootNode(), pHint, rs);
}

namespace
{

struct OrderEntry
{
	HString m_A;
	UInt32 m_u;
};
struct OrderSort
{
	Bool operator()(const OrderEntry& a, const OrderEntry& b) const
	{
		return a.m_u < b.m_u;
	}
};
struct OrderByKeySort
{
	Bool operator()(const OrderEntry& a, const OrderEntry& b) const
	{
		return (strcmp(a.m_A.CStr(), b.m_A.CStr()) < 0);
	}
};

void InternalSerializeAsString(
	Byte const* s,
	UInt32 zStringLengthInBytes,
	String& rsOutput)
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
 * Used to determine wrap point of a container on a live.
 *
 * "Effective" because indentation and # of elements in
 * a container both count towards this count.
 */
static const Int32 kiMaxEffectiveIndentationPerLine = 40;

static inline Bool SingleLineUtil(Int32& riIndentation, const DataStore& ds, const DataNode& node, const SharedPtr<DataStoreHint>& pHint);

/** Utility. */
static inline Bool IsNumber(const DataNode& val)
{
	return (
		val.IsFloat31() ||
		val.IsFloat32() ||
		val.IsInt32Big() ||
		val.IsInt32Small() ||
		val.IsInt64() ||
		val.IsUInt32() ||
		val.IsUInt64());
}

/** Utility. */
static inline Int ApproximateIndentIncrease(const DataStore& ds, const DataNode& node)
{
	// Arrays and tables alone are just 1 (for terminators).
	if (node.IsArray() || node.IsTable()) { return 1; }

	// Null, and booleans, always assume increase of 1.
	if (node.IsBoolean() || node.IsNull()) { return 1; }

	// Numbers, always assume 2.
	if (IsNumber(node)) { return 2; }

	// Get size and divide by 4.
	{
		Byte const* pUnused = nullptr;
		UInt32 uSize = 0u;
		if (ds.AsString(node, pUnused, uSize))
		{
			// Add 1 for delimiter and separators.
			return (Int32)(RoundUpToAlignment(uSize, 4) / 4) + 1;
		}
	}

	// Get size and divide by 4.
	{
		FilePath filePath;
		if (ds.AsFilePath(node, filePath))
		{
			// Add 1 for delimiter and separators.
			return (Int32)(RoundUpToAlignment(filePath.GetRelativeFilenameWithoutExtension().GetSizeInBytes(), 4) / 4) + 1;
		}
	}

	// Fallback to 1.
	return 1;
}

// Forward declarations.
static inline UInt32 GetResolveHash(const DataStore& ds, const DataNode& value);
static Bool ResolveArrayElemHint(const DataStore& ds, const DataNode& value, const SharedPtr<DataStoreHint>& pParentHint, UInt32& ruIndex, SharedPtr<DataStoreHint>& rp);

/** Check if a container should be on a single line or not. */
static inline Bool SingleLine(Int32& riIndentation, const DataStore& ds, const DataNode& node, const SharedPtr<DataStoreHint>& pHint, UInt32& ruElementsPerLine)
{
	auto const bRoot = (node.GetRawDataValue() == ds.GetRootNode().GetRawDataValue());

	// Increase by approximation.
	riIndentation += ApproximateIndentIncrease(ds, node);

	// 1 by default.
	ruElementsPerLine = 1u;

	// Immediately done if we're over the limit.
	if (riIndentation > kiMaxEffectiveIndentationPerLine)
	{
		return false;
	}

	if (node.IsArray())
	{
		UInt32 uArrayCount = 0u;
		SEOUL_VERIFY(ds.GetArrayCount(node, uArrayCount));

		// All roots must be multiline unless they are empty.
		if (bRoot && uArrayCount > 0u) { return false; }

		auto const iStartIndentation = riIndentation;
		UInt32 uContainerCount = 0u;
		SharedPtr<DataStoreHint> pValHint;
		for (UInt32 i = 0u, uHint = 0u; i < uArrayCount; ++i, ++uHint)
		{
			// Resolve child.
			DataNode val;
			SEOUL_VERIFY(ds.GetValueFromArray(node, i, val));

			// Resolve child hint.
			(void)ResolveArrayElemHint(ds, val, pHint, uHint, pValHint);

			// Recurse.
			if (!SingleLineUtil(riIndentation, ds, val, pValHint))
			{
				// Try to wrap a number only array with a limited length.
				goto fallback;
			}

			uContainerCount += (val.IsArray() || val.IsTable()) ? 1 : 0;
			if (uContainerCount > 1u)
			{
				return false;
			}
		}

		return true;

	fallback:
		// Special case - if the base indentation level is less
		// than the max, and we're an array of all numbers,
		// display multi-line but split into
		// (kiMaxEffectiveIndentationPerLine - iStartIndentation)
		// count per line.
		if (iStartIndentation < kiMaxEffectiveIndentationPerLine)
		{
			for (UInt32 i = 0u, uHint = 0u; i < uArrayCount; ++i, ++uHint)
			{
				// Get val and check.
				DataNode val;
				SEOUL_VERIFY(ds.GetValueFromArray(node, i, val));

				// Must be a number.
				if (!IsNumber(val))
				{
					return false;
				}

				// Resolve child hint to check for a comment - can
				// only apply if we resolve the hint reliably, since
				// comments are ignored on unreliable hinting.
				//
				// If any child has a comment, cannot have multiple children
				// per line.
				if (ResolveArrayElemHint(ds, val, pHint, uHint, pValHint) &&
					pValHint->GetCommentBegin() != pValHint->GetCommentEnd())
				{
					return false;
				}
			}

			// If we get here, we're a multiline (return false)
			// but with an elements per line of (kiMaxEffectiveIndentationPerLine - iStartIndentation) / 2
			// (2 is the indentation estimate for a number).
			ruElementsPerLine = (UInt32)Max((kiMaxEffectiveIndentationPerLine - iStartIndentation) / 2, 1);
			return false;
		}
	}
	else if (node.IsTable())
	{
		UInt32 uTableCount = 0u;
		SEOUL_VERIFY(ds.GetTableCount(node, uTableCount));

		// All roots must be multiline unless they are empty.
		if (bRoot && uTableCount > 0u) { return false; }


		UInt32 uContainerCount = 0u;
		auto const iBegin = ds.TableBegin(node);
		auto const iEnd = ds.TableEnd(node);
		for (auto i = iBegin; iEnd != i; ++i)
		{
			// Add the size contribution of the table key, + 1 for delimiters and separators.
			riIndentation += (Int32)(RoundUpToAlignment(i->First.GetSizeInBytes(), 4) / 4) + 1;
			// Recurse - SingleLineUtil will add contribution of value.
			if (!SingleLineUtil(riIndentation, ds, i->Second, (*pHint)[i->First])) { return false; }

			uContainerCount += (i->Second.IsArray() || i->Second.IsTable()) ? 1 : 0;
			if (uContainerCount > 1u)
			{
				return false;
			}
		}

		return true;
	}
	else
	{
		return true;
	}

	return false;
}

static inline Bool SingleLineUtil(Int32& riIndentation, const DataStore& ds, const DataNode& node, const SharedPtr<DataStoreHint>& pHint)
{
	// Any node prefixed with a comment must emit on a new line.
	if (pHint->GetCommentBegin() != pHint->GetCommentEnd()) { return false; }

	UInt32 uUnusedElementsPerLine = 0u;
	return SingleLine(riIndentation, ds, node, pHint, uUnusedElementsPerLine);
}

static void InternalNewLine(String& rsOutput, Int iIndentationLevel)
{
	// Remove trailing \t that may have been added by previous InternalNewLine().
	while (!rsOutput.IsEmpty() && rsOutput[rsOutput.GetSize() - 1] == '\t')
	{
		rsOutput.PopBack();
	}

	rsOutput.Append(SEOUL_EOL);
	rsOutput.Append('\t', iIndentationLevel);
}

static Bool InteranlHandleComment(
	const DataStore& ds,
	const DataNode& value,
	const SharedPtr<DataStoreHint>& pHint,
	Int iIndentationLevel,
	String& rsOutput)
{
	Bool bReturn = false;

	// Comment handling.
	if (pHint->GetCommentBegin() != pHint->GetCommentEnd())
	{
		// Get full comment.
		auto const iEnd = pHint->GetCommentEnd();
		auto iLine = pHint->GetCommentBegin();
		while (iLine != iEnd && IsSpace(*iLine))
		{
			++iLine;
		}

		// Additional newline if previous character is not a block opener.
		auto iPendingNewLine = 1;
		if (iLine != iEnd &&
			!rsOutput.IsEmpty() &&
			rsOutput[rsOutput.GetSize() - 1u] != '{' &&
			rsOutput[rsOutput.GetSize() - 1u] != '[')
		{
			++iPendingNewLine;
		}

		// Emit.
		for (; iEnd != iLine; )
		{
			auto ch = *iLine;
			if ('\r' == ch)
			{
				++iLine;
				continue;
			}

			Bool bNewLine = false;
			if ('\n' == ch)
			{
				InternalNewLine(rsOutput, iIndentationLevel);
				bNewLine = true;
			}
			else
			{
				while (iPendingNewLine > 0)
				{
					InternalNewLine(rsOutput, iIndentationLevel);
					bNewLine = true;
					--iPendingNewLine;
				}
				rsOutput.Append(ch);
				bReturn = true;
			}

			// Advance.
			++iLine;
			if (bNewLine)
			{
				while (iLine != iEnd && IsSpace(*iLine))
				{
					++iLine;
				}
			}
		}
	}

	return bReturn;
}

/**
 * Generate a hash used to disambiguate changed array members between
 * printed output and hinting data.
 */
static inline UInt32 GetResolveHash(const DataStore& ds, const DataNode& value)
{
	UInt32 u = 0u;
	switch (value.GetType())
	{
	case DataNode::kSpecialErase: // fall-through
	case DataNode::kNull: u = Seoul::GetHash(0u); break;
	case DataNode::kBoolean: u = Seoul::GetHash(ds.AssumeBoolean(value)); break;
	case DataNode::kUInt32: u = Seoul::GetHash(ds.AssumeUInt32(value)); break;
	case DataNode::kInt32Big: u = Seoul::GetHash(ds.AssumeInt32Big(value)); break;
	case DataNode::kInt32Small: u = Seoul::GetHash(ds.AssumeInt32Small(value)); break;
	case DataNode::kFloat31: u = GetFloat32Hash(ds.AssumeFloat31(value)); break;
	case DataNode::kFloat32: u = GetFloat32Hash(ds.AssumeFloat32(value)); break;
	case DataNode::kFilePath:
	{
		FilePath filePath;
		(void)ds.AsFilePath(value, filePath);
		u = Seoul::GetHash(filePath.ToSerializedUrl());
	}
	break;
	// Hash is "shallow", so we don't recurse into children if they
	// are not leaves. This means the only table/array that will be recursed
	// is the root call.
	case DataNode::kTable:
	{
		auto const iBegin = ds.TableBegin(value);
		auto const iEnd = ds.TableEnd(value);
		Vector<HString> v;
		for (auto i = iBegin; iEnd != i; ++i)
		{
			v.PushBack(i->First);
		}

		LexicalHStringSort sort;
		QuickSort(v.Begin(), v.End(), sort);

		for (auto const& e : v)
		{
			DataNode child;
			SEOUL_VERIFY(ds.GetValueFromTable(value, e, child));

			// Skip nested containers.
			if (child.IsArray() || child.IsTable()) { continue; }

			IncrementalHash(u, Seoul::GetHash(e));
			IncrementalHash(u, GetResolveHash(ds, child));
		}
	}
	break;
	case DataNode::kArray:
	{
		UInt32 uArrayCount = 0u;
		(void)ds.GetArrayCount(value, uArrayCount);
		for (UInt32 i = 0u; i < uArrayCount; ++i)
		{
			DataNode child;
			SEOUL_VERIFY(ds.GetValueFromArray(value, i, child));

			// Skip nested containers.
			if (child.IsArray() || child.IsTable()) { continue; }

			IncrementalHash(u, GetResolveHash(ds, child));
		}
	}
	break;
	case DataNode::kString:
	{
		Byte const* sStr = nullptr;
		UInt32 uStr = 0u;
		(void)ds.AsString(value, sStr, uStr);
		u = Seoul::GetHash(sStr, uStr);
	}
	break;
	case DataNode::kInt64: u = Seoul::GetHash(ds.AssumeInt64(value)); break;
	case DataNode::kUInt64: u = Seoul::GetHash(ds.AssumeUInt64(value)); break;
	};
	return u;
}

/**
 * There can be ambiguity resolving the hinting data for an array element
 * in light of mutations, removes, and adds. We use the following approach
 * to make a best guess to find the hint data:
 * - match hint to data at index uIndex - if hashes match, accept the hinting
 * - if hashes don't match but both the hinting data and the value are leaves
 *   (not a table or an array), accept the hinting.
 * - do a lookup into the parent array on the hash of the value - if there is
 *   an exact match (there is 1 and only 1 entry in the parent array that has
 *   the given value hash), then accept the found hinting.
 * - final fallback - use the original hinting (associated with the index in
 *   the parent array), but report that we're unsure of the match. This will
 *   prevent the hint data from being used in certain cases (we won't include
 *   any comments on the root of the hint data).
 */
static Bool ResolveArrayElemHint(
	const DataStore& ds,
	const DataNode& value,
	const SharedPtr<DataStoreHint>& pParentHint,
	UInt32& ruIndex,
	SharedPtr<DataStoreHint>& rp)
{
	auto const uHash(GetResolveHash(ds, value));
	rp = (*pParentHint)[ruIndex];
	if (rp->GetHash() == uHash)
	{
		return true;
	}
	// Stick with p if value and pHint are both simple types.
	else if (rp->IsLeaf() && !value.IsArray() && !value.IsTable())
	{
		return true;
	}

	// Attempt to find a better match with hash lookup - if this
	// fails, we currently just fall back to the original p, but
	// return false.
	if (pParentHint->IndexFromHash(uHash, ruIndex))
	{
		rp = (*pParentHint)[ruIndex];
		return true;
	}

	return false;
}

} // namespace anonymous

void DataStorePrinter::InternalPrintWithHints(
	const DataStore& ds,
	const DataNode& value,
	const SharedPtr<DataStoreHint>& pHint,
	Int32 iIndentationLevel,
	String& rsOutput)
{
	switch (value.GetType())
	{
	case DataNode::kNull: // fall-through
	case DataNode::kSpecialErase: // TODO: Not ideal, since we lose the deletion info - no text based version of the diff.
		rsOutput.Append("null");
		break;
	case DataNode::kBoolean:
		rsOutput.Append(ds.AssumeBoolean(value) ? "true" : "false");
		break;
	case DataNode::kInt32Big:
		rsOutput.Append(String::Printf("%d", ds.AssumeInt32Big(value)));
		break;
	case DataNode::kInt32Small:
		rsOutput.Append(String::Printf("%d", ds.AssumeInt32Small(value)));
		break;
	case DataNode::kUInt32:
		rsOutput.Append(String::Printf("%u", ds.AssumeUInt32(value)));
		break;
	case DataNode::kFloat31:
	case DataNode::kFloat32:
	{
		Float32 const fValue = (DataNode::kFloat32 == value.GetType() ? ds.AssumeFloat32(value) : ds.AssumeFloat31(value));

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
		SEOUL_VERIFY(ds.AsFilePath(value, filePath));

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
		// Check if contianer should be emitted on a single line.
		UInt32 uUnusedElementsPerLine = 1u;
		auto iEffectiveIndentation = iIndentationLevel;
		auto const bSingleLine = SingleLine(iEffectiveIndentation, ds, value, pHint, uUnusedElementsPerLine);

		// Start the table and increase the indentation level.
		rsOutput.Append('{');
		++iIndentationLevel;

		// Get the total table size, then allocate a vector for all the keys.
		UInt32 nTableCount = 0u;
		SEOUL_VERIFY(ds.GetTableCount(value, nTableCount));
		Vector<OrderEntry, MemoryBudgets::DataStore> vEntries;
		vEntries.Reserve(nTableCount);
		Vector<OrderEntry, MemoryBudgets::DataStore> vToSortEntries;

		// Insert keys into the vector.
		{
			auto const iBegin = ds.TableBegin(value);
			auto const iEnd = ds.TableEnd(value);
			for (auto i = iBegin; iEnd != i; ++i)
			{
				auto pTableHint((*pHint)[i->First]);
				OrderEntry entry;
				entry.m_A = i->First;

				// No hinting, will be placed at end
				// in lexographically sorted order.
				if (pTableHint->IsNone())
				{
					entry.m_u = 0u;
					vToSortEntries.PushBack(entry);
				}
				else
				{
					entry.m_u = pTableHint->GetOrder();
					vEntries.PushBack(entry);
				}
			}
		}

		// Resolve and append any to be sorted entries.
		if (!vToSortEntries.IsEmpty())
		{
			// Sort by name.
			OrderByKeySort sorter;
			QuickSort(vToSortEntries.Begin(), vToSortEntries.End(), sorter);

			// Give an order, then append them.
			auto uOrder = pHint->GetNextContainerOrder();
			for (auto& e : vToSortEntries)
			{
				e.m_u = uOrder++;
			}

			// Append and clear.
			vEntries.Append(vToSortEntries);
			vToSortEntries.Clear();
		}

		// Sort the vector.
		OrderSort sorter;
		QuickSort(vEntries.Begin(), vEntries.End(), sorter);

		// Now write the entries
		{
			for (UInt32 i = 0u; i < nTableCount; ++i)
			{
				const OrderEntry& entry = vEntries[i];
				DataNode tableValue;
				SEOUL_VERIFY(ds.GetValueFromTable(value, entry.m_A, tableValue));

				auto pTableHint((*pHint)[entry.m_A]);

				if (i > 0u)
				{
					if (bSingleLine)
					{
						rsOutput.Append(", ");
					}
					else
					{
						rsOutput.Append(',');
					}
				}

				// Emit a comment if defined and needed.
				InteranlHandleComment(ds, tableValue, pTableHint, iIndentationLevel, rsOutput);

				// If in multiline mode, insert a newline and indent.
				if (!bSingleLine)
				{
					InternalNewLine(rsOutput, iIndentationLevel);
				}

				InternalSerializeAsString(entry.m_A.CStr(), entry.m_A.GetSizeInBytes(), rsOutput);

				rsOutput.Append(": ");
				InternalPrintWithHints(ds, tableValue, pTableHint, iIndentationLevel, rsOutput);
			}
		}

		// Decrease the indentation level before terminating.
		--iIndentationLevel;

		// If in multiline formatting mode, conditionally insert a newline before inserting the table terminator.
		if (!bSingleLine)
		{
			// Don't insert a newline before the terminator if the table is empty.
			if (ds.TableBegin(value) != ds.TableEnd(value))
			{
				InternalNewLine(rsOutput, iIndentationLevel);
			}
		}

		rsOutput.Append('}');
	}
	break;
	case DataNode::kArray:
	{
		UInt32 uArrayCount = 0u;
		SEOUL_VERIFY(ds.GetArrayCount(value, uArrayCount));

		// Check if contianer should be emitted on a single line.
		UInt32 uElementsPerLine = 1u;
		auto iEffectiveIndentation = iIndentationLevel;
		auto const bSingleLine = SingleLine(iEffectiveIndentation, ds, value, pHint, uElementsPerLine);
		rsOutput.Append('[');
		++iIndentationLevel;
		for (UInt32 i = 0u, uHint = 0u; i < uArrayCount; ++i, ++uHint)
		{
			auto const bNewLine = (1u == uElementsPerLine || (0 == (i % uElementsPerLine)));

			if (i > 0u)
			{
				if ((bSingleLine || !bNewLine))
				{
					rsOutput.Append(", ");
				}
				else
				{
					rsOutput.Append(',');
				}
			}

			DataNode arrayValue;
			SEOUL_VERIFY(ds.GetValueFromArray(value, i, arrayValue));

			// On success, we've found an exact match for the array hint.
			// Otherwise, we've fallen back to a guess, which means we
			// should ignore any comments on the resolved hint, since they
			// will almost certainly be incorrect.
			SharedPtr<DataStoreHint> pArrayHint;
			if (ResolveArrayElemHint(ds, arrayValue, pHint, uHint, pArrayHint))
			{
				// Emit a comment if defined and needed.
				(void)InteranlHandleComment(ds, arrayValue, pArrayHint, iIndentationLevel, rsOutput);
			}

			if (!bSingleLine && bNewLine)
			{
				InternalNewLine(rsOutput, iIndentationLevel);
			}

			// Emit string value.
			InternalPrintWithHints(ds, arrayValue, pArrayHint, iIndentationLevel, rsOutput);
		}
		--iIndentationLevel;
		if (!bSingleLine)
		{
			InternalNewLine(rsOutput, iIndentationLevel);
		}
		rsOutput.Append(']');
	}
	break;
	case DataNode::kString:
	{
		Byte const* s = nullptr;
		UInt32 zStringLengthInBytes = 0u;
		SEOUL_VERIFY(ds.AsString(value, s, zStringLengthInBytes));
		InternalSerializeAsString(s, zStringLengthInBytes, rsOutput);
	}
	break;
	case DataNode::kInt64:
		rsOutput.Append(String::Printf("%" PRId64, ds.AssumeInt64(value)));
		break;
	case DataNode::kUInt64:
		rsOutput.Append(String::Printf("%" PRIu64, ds.AssumeUInt64(value)));
		break;
	default:
		SEOUL_FAIL("Unknown enum");
		break;
	};
}

} // namespace Seoul
