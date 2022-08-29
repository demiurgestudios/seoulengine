/**
 * \file ReflectionTableDetail.h
 * \brief Types used to construct subclasses of Reflection::Table that
 * defines table behavior for various types exposed through the
 * reflection system.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_TABLE_DETAIL_H
#define REFLECTION_TABLE_DETAIL_H

#include "ReflectionTable.h"
#include "ReflectionType.h"
#include "ReflectionTypeDetail.h"
#include "ToString.h"

namespace Seoul::Reflection
{

namespace TableDetail
{

/**
 * Utility struct used by TableT when constructing
 * a key for a concrete table, where the key is type T,
 * from a DataStore table key, which is always an HString.
 *
 * This class uses the copy constructor of type T, unless
 * the type T is an enum, in which case it uses the Reflection::Enum
 * object associated with the type to marshal the enum value to/from
 * an HString.
 */
template <typename T, Bool IS_ENUM> struct ConstructTableKey SEOUL_SEALED;

template<>
struct ConstructTableKey<HString, false> SEOUL_SEALED
{
	static inline Bool FromHString(HString key, HString& rOut)
	{
		rOut = key;
		return true;
	}

	static inline Bool ToHString(const HString& key, HString& rOut)
	{
		rOut = key;
		return true;
	}
};

template<>
struct ConstructTableKey<String, false> SEOUL_SEALED
{
	static inline Bool FromHString(HString key, String& rOut)
	{
		rOut = String(key);
		return true;
	}

	static inline Bool ToHString(const String& sKey, HString& rOut)
	{
		rOut = HString(sKey);
		return true;
	}
};

template<>
struct ConstructTableKey<Int32, false> SEOUL_SEALED
{
public:
	static inline Bool FromHString(HString key, Int32& rOut)
	{
		return key.ToInt32(rOut);
	}

	static inline Bool ToHString(Int32 key, HString& rOut)
	{
		rOut = HString(ToString(key));
		return true;
	}
};

template<>
struct ConstructTableKey<UInt32, false> SEOUL_SEALED
{
public:
	static inline Bool FromHString(HString key, UInt32& rOut)
	{
		return key.ToUInt32(rOut);
	}

	static inline Bool ToHString(UInt32 key, HString& rOut)
	{
		rOut = HString(ToString(key));
		return true;
	}
};

template<>
struct ConstructTableKey<Int64, false> SEOUL_SEALED
{
public:
	static inline Bool FromHString(HString key, Int64& rOut)
	{
		return key.ToInt64(rOut);
	}

	static inline Bool ToHString(Int64 key, HString& rOut)
	{
		rOut = HString(ToString(key));
		return true;
	}
};

template<>
struct ConstructTableKey<UInt64, false> SEOUL_SEALED
{
public:
	static inline Bool FromHString(HString key, UInt64& rOut)
	{
		return key.ToUInt64(rOut);
	}

	static inline Bool ToHString(UInt64 key, HString& rOut)
	{
		rOut = HString(ToString(key));
		return true;
	}
};

template<>
struct ConstructTableKey<WorldTime, false> SEOUL_SEALED
{
public:
	static inline Bool FromHString(HString key, WorldTime& rOut)
	{
		Int64 microseconds;
		Bool success = key.ToInt64(microseconds);
		if (success)
		{
			rOut.SetMicroseconds(microseconds);
		}
		return success;
	}

	static inline Bool ToHString(const WorldTime& key, HString& rOut)
	{
		rOut = HString(ToString(key.GetMicroseconds()));
		return true;
	}
};

template <typename T>
struct ConstructTableKey<T, true> SEOUL_SEALED
{
public:
	static inline Bool FromHString(HString key, T& rOut)
	{
		T eReturn = (T)-1;
		if (EnumOf<T>().TryGetValue(key, eReturn))
		{
			rOut = eReturn;
			return true;
		}

		return false;
	}

	static inline Bool ToHString(T eValue, HString& rOut)
	{
		HString ret;
		if (EnumOf<T>().TryGetName(eValue, ret))
		{
			rOut = ret;
			return true;
		}

		return false;
	}
};

/**
 * Utility struct, evaluates to true if the type T
 * has a method with signature Bool Erase(const T::KeyType), false otherwise.
 *
 * Used to determine if a table type implements an Erase() method.
 */
template <typename T>
struct TableHasErase SEOUL_SEALED
{
private:
	typedef Byte True[1];
	typedef Byte False[2];

	template <typename U, U MEMBER_FUNC> struct Tester {};

	template <typename U>
	static True& TestFoErase(Tester< Bool (U::*)(const typename U::KeyType&), &U::Erase >*);

	template <typename U>
	static False& TestFoErase(...);

public:
	static const Bool Value = (
		sizeof(TestFoErase<T>(nullptr)) == sizeof(True));
}; // struct TableHasErase

/**
 * Erases an element from a table, if it implements an
 * Erase method, or a nop which always returns false if the
 * table does not implement an Erase method.
 */
template <typename T, Bool B_TABLE_HAS_ERASE>
struct TableErase
{
	typedef typename T::KeyType KeyType;

	static Bool TryErase(T& rTable, const KeyType& key)
	{
		return rTable.Erase(key);
	}
};

template <typename T>
struct TableErase<T, false>
{
	typedef typename T::KeyType KeyType;

	static Bool TryErase(T& /*rTable*/, const KeyType& /*key*/)
	{
		return false;
	}
};

/**
 * Utility struct, evaluates to true if the type T
 * has a method with signature Bool Overwrite(const T::KeyType), false otherwise.
 *
 * Used to determine if a table type implements an Overwrite() method.
 */
template <typename T>
struct TableHasOverwrite SEOUL_SEALED
{
private:
	typedef typename T::Iterator IteratorType;

	typedef Byte True[1];
	typedef Byte False[2];

	template <typename U, U MEMBER_FUNC> struct Tester {};

	template <typename U>
	static True& TestFoOverwrite(Tester< Pair<IteratorType, Bool> (U::*)(const typename U::KeyType&, const typename U::ValueType&), &U::Overwrite >*);

	template <typename U>
	static False& TestFoOverwrite(...);

public:
	static const Bool Value = (
		sizeof(TestFoOverwrite<T>(nullptr)) == sizeof(True));
}; // struct TableHasOverwrite

/**
 * Overwrites an element from a table, if it implements an
 * Overwrite method, or a nop which always returns false if the
 * table does not implement an Overwrite method.
 */
template <typename T, Bool B_TABLE_HAS_ERASE>
struct TableOverwrite
{
	typedef typename T::KeyType KeyType;
	typedef typename T::ValueType ValueType;

	static Bool TryOverwrite(T& rTable, const KeyType& key, const ValueType& value)
	{
		return rTable.Overwrite(key, value).Second;
	}
};

template <typename T>
struct TableOverwrite<T, false>
{
	typedef typename T::KeyType KeyType;
	typedef typename T::ValueType ValueType;

	static Bool TryOvewrite(T& /*rTable*/, const KeyType& /*key*/, const ValueType& /*value*/)
	{
		return false;
	}
};

/**
 * Executes the appropriate operation to Clear
 * a table type T, depending on the value type of the table.
 * - if the value type is a CheckedPtr<>, calls SafeDeleteTable()
 * - if the value type is a pointer, calls SafeDeleteTable()
 * - in all other cases, calls Clear() on the table.
 */
template <typename TABLE_TYPE, typename VALUE_TYPE>
struct ClearTableT
{
	static inline void Clear(TABLE_TYPE& rTable)
	{
		rTable.Clear();
	}
};

template <typename TABLE_TYPE, typename T>
struct ClearTableT<TABLE_TYPE, T*>
{
	static inline void Clear(TABLE_TYPE& rTable)
	{
		SafeDeleteTable(rTable);
	}
};

template <typename TABLE_TYPE, typename T>
struct ClearTableT< TABLE_TYPE, CheckedPtr<T> >
{
	static inline void Clear(TABLE_TYPE& rTable)
	{
		SafeDeleteTable(rTable);
	}
};

template <typename T>
class TableEnumeratorT SEOUL_SEALED : public Seoul::Reflection::TableEnumerator
{
public:
	/** Carry through the KeyType of the table we represent. */
	typedef typename T::KeyType KeyType;

	/** Carry through the ValueType of the table we represent. */
	typedef typename T::ValueType ValueType;

	/** Carry through the ConstIteratorType of the table we represents. */
	typedef typename T::ConstIterator ConstIteratorType;

	/** Carry through the IteratorType of the table we represent. */
	typedef typename T::Iterator IteratorType;

	TableEnumeratorT(const T& table)
		: m_t(table)
		, m_Begin(table.Begin())
		, m_Current(m_Begin)
		, m_End(table.End())
	{
	}

	~TableEnumeratorT()
	{
	}

	virtual Bool TryGetNext(Any& rKey, Any& rValue) SEOUL_OVERRIDE
	{
		// Failure if already at m_End.
		if (m_End == m_Current)
		{
			return false;
		}

		rKey = m_Current->First;
		rValue = m_Current->Second;
		++m_Current;
		return true;
	}

private:
	const T& m_t;
	ConstIteratorType const m_Begin;
	ConstIteratorType m_Current;
	ConstIteratorType const m_End;

	SEOUL_DISABLE_COPY(TableEnumeratorT);
}; // class TableEnumeratorT

template <typename T>
class TableT SEOUL_SEALED : public Seoul::Reflection::Table
{
public:
	/** Carry through the KeyType of the table we represent. */
	typedef typename T::KeyType KeyType;

	/** Carry through the ValueType of the table we represent. */
	typedef typename T::ValueType ValueType;

	/** Carry through the ConstIteratorType of the table we represents. */
	typedef typename T::ConstIterator ConstIteratorType;

	/** Carry through the IteratorType of the table we represent. */
	typedef typename T::Iterator IteratorType;

	/** Bypass some polymorphism when handling individual values. */
	typedef typename TypeTDiscovery<ValueType>::Type Handler;

	/** Enumerator type of the table. */
	typedef TableEnumeratorT<T> EnumeratorType;

	TableT()
		: Table(TableHasErase<T>::Value ? TableFlags::kErase : TableFlags::kNone)
	{
	}

	~TableT()
	{
	}

	/** @return the TypeInfo of keys of this Table. */
	virtual const TypeInfo& GetKeyTypeInfo() const SEOUL_OVERRIDE
	{
		return TypeId<typename T::KeyType>();
	}

	/** @return the TypeInfo of values of this Table. */
	virtual const TypeInfo& GetValueTypeInfo() const SEOUL_OVERRIDE
	{
		return TypeId<typename T::ValueType>();
	}

	// Allocator a forward enumerator for the table. Returns
	// nullptr on error. If non-null, the returned enumerator
	// must be deallocated with SEOUL_DELETE/SafeDelete().
	virtual TableEnumerator* NewEnumerator(const WeakAny& tablePointer) const SEOUL_OVERRIDE
	{
		T const* pTable = nullptr;
		if (!PointerCast(tablePointer, pTable))
		{
			return nullptr;
		}

		return SEOUL_NEW(MemoryBudgets::Reflection) EnumeratorType(*pTable);
	}

	/** Attempt to assign a read-write pointer to the value at key to rValue. */
	virtual Bool TryGetValuePtr(const WeakAny& tablePointer, const WeakAny& inputKey, WeakAny& rValue, Bool bInsert = false) const SEOUL_OVERRIDE
	{
		T* pTable = nullptr;
		KeyType key;
		if (PointerCast(tablePointer, pTable) &&
			TypeConstruct(inputKey, key))
		{
			SEOUL_ASSERT(nullptr != pTable);

			typename T::ValueType* pValue = pTable->Find(key);
			if (nullptr == pValue && bInsert)
			{
				pValue = &(pTable->Insert(key, ValueType()).First->Second);
			}

			if (nullptr != pValue)
			{
				rValue = pValue;
				return true;
			}
		}

		return false;
	}

	/** Attempt to assign a read-only pointer to the value at key to rValue. */
	virtual Bool TryGetValueConstPtr(const WeakAny& tablePointer, const WeakAny& inputKey, WeakAny& rValue) const SEOUL_OVERRIDE
	{
		T const* pTable = nullptr;
		KeyType key;
		if (PointerCast(tablePointer, pTable) &&
			TypeConstruct(inputKey, key))
		{
			SEOUL_ASSERT(nullptr != pTable);

			typename T::ValueType const* pValue = pTable->Find(key);
			if (nullptr != pValue)
			{
				rValue = pValue;
				return true;
			}
		}

		return false;
	}

	/** Attempt to erase key from this Table. */
	virtual Bool TryErase(const WeakAny& tablePointer, const WeakAny& inputKey) const SEOUL_OVERRIDE
	{
		T* pTable = nullptr;
		KeyType key;
		if (PointerCast(tablePointer, pTable) &&
			TypeConstruct(inputKey, key))
		{
			SEOUL_ASSERT(nullptr != pTable);

			return TableErase<T, TableHasErase<T>::Value>::TryErase(*pTable, key);
		}

		return false;
	}

	/** Call this method to attempt to set data to this Table, into the instance in thisPointer. */
	virtual Bool TryOverwrite(const WeakAny& tablePointer, const WeakAny& inputKey, const WeakAny& inputValue) const SEOUL_OVERRIDE
	{
		T* pTable = nullptr;
		KeyType key;
		if (PointerCast(tablePointer, pTable) &&
			TypeConstruct(inputKey, key))
		{
			SEOUL_ASSERT(nullptr != pTable);
			typename RemoveConst< typename RemoveReference<ValueType>::Type >::Type value;
			if (TypeConstruct(inputValue, value))
			{
				return TableOverwrite<T, TableHasOverwrite<T>::Value>::TryOverwrite(*pTable, key, value);
			}
		}

		return false;
	}

	/**
	 * Attempt to deserialize the DataStore table in dataNode into
	 * the C++ table object in objectThis.
	 *
	 * @return True if deserialization was successful, false otherwise. If this method
	 * returns false, HandlError() was called in rContext and returned false,
	 * indicating an unhandlable error.
	 */
	virtual Bool TryDeserialize(
		SerializeContext& rContext,
		const DataStore& dataStore,
		const DataNode& table,
		const WeakAny& objectThis,
		Bool bSkipPostSerialize = false) const SEOUL_OVERRIDE
	{
		// Get the pointer to the table object - failure here is always an unhandlable error.
		T* pTable = nullptr;
		if (!PointerCast(objectThis, pTable))
		{
			return false;
		}
		SEOUL_ASSERT(nullptr != pTable);

		// Remove all existing entries from the table.
		ClearTableT<T, ValueType>::Clear(*pTable);

		// Enumerate the members of the DataStore table.
		auto const iBegin = dataStore.TableBegin(table);
		auto const iEnd = dataStore.TableEnd(table);
		for (auto i = iBegin; iEnd != i; ++i)
		{
			// Attempt to insert an empty element.
			KeyType key;
			if (!ConstructTableKey<KeyType, IsEnum<KeyType>::Value>::FromHString(i->First, key))
			{
				SerializeContextScope scope(rContext, i->Second, GetValueTypeInfo(), i->First);
				if (!rContext.HandleError(SerializeError::kFailedSettingValueToTable))
				{
					return false;
				}
				// If this error is handled, continue, as falling through
				// will result in a crash, due to an invalid second member of
				// the pair
				else
				{
					continue;
				}
			}

			auto iter = pTable->Insert(key, ValueType());

			// If insertion fails, signal the error.
			if (!iter.Second)
			{
				SerializeContextScope scope(rContext, i->Second, GetValueTypeInfo(), i->First);
				if (!rContext.HandleError(SerializeError::kFailedSettingValueToTable))
				{
					return false;
				}
				// If this error is handled, continue, as falling through
				// will result in a crash, due to an invalid second member of
				// the pair
				else
				{
					continue;
				}
			}

			SerializeContextScope scope(rContext, i->Second, GetValueTypeInfo(), i->First);

			// Direct deserialize the table member - if this fails, deserialization fails.
			if (!Handler::DirectDeserialize(
				rContext,
				dataStore,
				i->Second,
				iter.First->Second,
				bSkipPostSerialize))
			{
				if (!rContext.HandleError(SerializeError::kFailedSettingValueToTable))
				{
					return false;
				}
				// If this error is handled, try to erase the element we inserted.
				// Fail despite the handling if we fail to erase the element.
				else
				{
					if (!TableErase<T, TableHasErase<T>::Value>::TryErase(*pTable, key))
					{
						return false;
					}
					// Otherwise, continue onto the next element.
					else
					{
						continue;
					}
				}
			}
		}

		return true;
	}

	/**
	 * Serialize the Table in objectThis into a DataNode
	 * table in table.
	 *
	 * @return True if serialization was successful, false otherwise. If this
	 * method returns false, dataNode may be in a partially serialized, modified
	 * state.
	 */
	virtual Bool TrySerialize(
		SerializeContext& rContext,
		DataStore& rDataStore,
		const DataNode& table,
		const WeakAny& objectThis,
		Bool bSkipPostSerialize = false) const SEOUL_OVERRIDE
	{
		// Get the pointer to the table object - failure here is always an unhandlable error.
		T const* pTable = nullptr;
		if (!PointerCast(objectThis, pTable))
		{
			return false;
		}
		SEOUL_ASSERT(nullptr != pTable);

		// Enumerate the members of the pTable.
		auto const iBegin = pTable->Begin();
		auto const iEnd = pTable->End();
		for (auto i = iBegin; iEnd != i; ++i)
		{
			// Construct an hstring key from the table entry key.
			HString key;
			if (!ConstructTableKey<KeyType, IsEnum<KeyType>::Value>::ToHString(i->First, key))
			{
				if (!rContext.HandleError(SerializeError::kFailedGettingTableKeyString))
				{
					return false;
				}

				continue;
			}

			SerializeContextScope scope(rContext, DataNode(), GetValueTypeInfo(), key);

			// If serialization of the element fails, fail overall serialization.
			if (!Handler::DirectSerializeToTable(
				rContext,
				rDataStore,
				table,
				key,
				i->Second,
				bSkipPostSerialize))
			{
				return false;
			}
		}

		return true;
	}

	virtual void FromScript(lua_State* pVm, Int iOffset, const WeakAny& objectThis) const SEOUL_OVERRIDE
	{
		// Get the pointer to the table object - failure here is always an unhandlable error.
		T* pTable = nullptr;
		if (!PointerCast(objectThis, pTable))
		{
			// TODO: I think I can assert here, as all callers
			// will have enforced this.
			return;
		}
		SEOUL_ASSERT(nullptr != pTable);

		// Remove all existing entries from the table.
		ClearTableT<T, ValueType>::Clear(*pTable);

		// State
		auto const& keyType = TypeOf<KeyType>();
		auto const& valueType = TypeOf<ValueType>();
		KeyType key;
		ValueType value;

		// Iterate the table.
		Int const iTable = (iOffset < 0 ? iOffset - 1 : iOffset);
		lua_pushnil(pVm);
		while (0 != lua_next(pVm, iTable))
		{
			// Get the key from script.
			keyType.FromScript(pVm, -2, &key);

			// Get the value from script.
			valueType.FromScript(pVm, -1, &value);

			// Must succeed, since script enforces uniqueness
			// on the key the same as native.
			SEOUL_VERIFY(pTable->Insert(key, value).Second);

			// Remove the value from the stack. Key stays on
			// the stack per lua_next() semantics.
			lua_pop(pVm, 1);
		}
	}

	virtual void ToScript(lua_State* pVm, const WeakAny& objectThis) const SEOUL_OVERRIDE
	{
		// Get the pointer to the table object - failure here is always an unhandlable error.
		T const* pTable = nullptr;
		if (!PointerCast(objectThis, pTable))
		{
			// TODO: I think I can assert here, as all callers
			// will have enforced this.
			lua_pushnil(pVm);
			return;
		}
		SEOUL_ASSERT(nullptr != pTable);

		// State.
		auto const& keyType = TypeOf<KeyType>();
		auto const& valueType = TypeOf<ValueType>();
		lua_createtable(pVm, pTable->GetSize(), pTable->GetSize());

		// Enumerate the members of the pTable.
		auto const iBegin = pTable->Begin();
		auto const iEnd = pTable->End();
		for (auto i = iBegin; iEnd != i; ++i)
		{
			// Push the key to script.
			keyType.ToScript(pVm, &i->First);

			// Push the value to script.
			valueType.ToScript(pVm, &i->Second);

			// Commit the key-value pair to the table.
			lua_rawset(pVm, -3);
		}
	}

private:
	SEOUL_DISABLE_COPY(TableT);
}; // class TableT

} // namespace TableDetail

} // namespace Seoul::Reflection

#endif // include guard
