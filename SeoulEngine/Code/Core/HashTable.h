/**
 * \file HashTable.h
 * \brief HashTable is an associative key-value array with the
 * following properties:
 * - the key type must define an "empty" value that is used to represent
 *   undefined entries in the table. The empty key can still be used
 *   as a key in the table, but it will be handled specially and incurs
 *   a slight runtime/memory cost compared to non-empty value keys.
 * - the capacity of the table is always a power of 2.
 * - values and keys are stored in arrays with no chaining.
 *
 * HashTable can be viewed as a rough equivalent to std::unordered_map<>, with
 * several differences, primarily:
 * - CamelCase function and members names.
 * - additional GetCapacity() method.
 * - additional Overwrite() and GetValue() methods.
 * - empty() is called IsEmpty().
 * - erase() equivalent Erase() returns a Bool instead of a size_t
 * - find() equivalent Find() returns just a Value pointer and not an iterator.
 *   (true instead of # of elements erased, since it is always 1).
 * - size() is called GetSize().
 * - SizeType is always a UInt32.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef HASH_TABLE
#define HASH_TABLE

#include "Algorithms.h"
#include "Atomic32.h"
#include "CheckedPtr.h"
#include "HashFunctions.h"
#include "Pair.h"
#include "Prereqs.h"

namespace Seoul
{

/**
 * Used to compare for equality between 2 keys in a hash table - will use
 * the hash a comparison if B_CHECK_HASH_BEFORE_EQUALS is true, otherwise
 * it just does a comparison. For some key types, using the hash first may
 * be a performance optimization (a string type with a cached hash value).
 */
template <typename KEY, Bool B_CHECK_HASH_BEFORE_EQUALS>
struct HashTableComparator;

template <typename KEY>
struct HashTableComparator<KEY, true>
{
	inline static Bool Equals(UInt32 uHashOfA, const KEY& a, const KEY& b)
	{
		return (uHashOfA == GetHash(b) && (a == b));
	}
};

template <typename KEY>
struct HashTableComparator<KEY, false>
{
	inline static Bool Equals(UInt32 uHashOfA, const KEY& a, const KEY& b)
	{
		return (a == b);
	}
};

/**
 * Implementation of a read-write access iterator for HashTable.
 */
template <typename KEY, typename VALUE, typename TRAITS>
struct HashTableIterator
{
	struct IteratorPair
	{
		IteratorPair(KEY& key, VALUE& value)
			: First(key)
			, Second(value)
		{
		}

		KEY const& First;
		VALUE& Second;
	};

	struct IteratorPairIndirect
	{
		IteratorPairIndirect(KEY& key, VALUE& value)
			: m_Pair(key, value)
		{
		}

		IteratorPair m_Pair;

		IteratorPair* operator->()
		{
			return &m_Pair;
		}

		IteratorPair const* operator->() const
		{
			return &m_Pair;
		}
	};

	HashTableIterator(Atomic32* pReferenceCount, KEY* pKeys, VALUE* pValues, UInt32 uCapacity, UInt32 uIndex = 0)
		: m_pKeys(pKeys)
		, m_pValues(pValues)
		, m_uCapacity(uCapacity)
		, m_uIndex(Index(uIndex))
#if !SEOUL_ASSERTIONS_DISABLED
		, m_pReferenceCount(pReferenceCount)
#endif
	{
#if !SEOUL_ASSERTIONS_DISABLED
		if (m_pReferenceCount.IsValid())
		{
			++(*m_pReferenceCount);
		}
#endif
	}

	HashTableIterator(const HashTableIterator& b)
		: m_pKeys(b.m_pKeys)
		, m_pValues(b.m_pValues)
		, m_uCapacity(b.m_uCapacity)
		, m_uIndex(b.m_uIndex)
#if !SEOUL_ASSERTIONS_DISABLED
		, m_pReferenceCount(b.m_pReferenceCount)
#endif
	{
#if !SEOUL_ASSERTIONS_DISABLED
		if (m_pReferenceCount.IsValid())
		{
			++(*m_pReferenceCount);
		}
#endif
	}

	~HashTableIterator()
	{
#if !SEOUL_ASSERTIONS_DISABLED
		if (m_pReferenceCount.IsValid())
		{
			--(*m_pReferenceCount);
		}
#endif
	}

	HashTableIterator& operator=(const HashTableIterator& b)
	{
		m_pKeys = b.m_pKeys;
		m_pValues = b.m_pValues;
		m_uCapacity = b.m_uCapacity;
		m_uIndex = b.m_uIndex;

#if !SEOUL_ASSERTIONS_DISABLED
		if (m_pReferenceCount.IsValid())
		{
			--(*m_pReferenceCount);
		}

		m_pReferenceCount = b.m_pReferenceCount;

		if (m_pReferenceCount.IsValid())
		{
			++(*m_pReferenceCount);
		}
#endif

		return *this;
	}

	IteratorPair operator*() const
	{
		return IteratorPair(m_pKeys[m_uIndex], m_pValues[m_uIndex]);
	}

	IteratorPairIndirect operator->() const
	{
		return IteratorPairIndirect(m_pKeys[m_uIndex], m_pValues[m_uIndex]);
	}

	HashTableIterator& operator++()
	{
		m_uIndex = Index(m_uIndex + 1);

		return *this;
	}

	HashTableIterator operator++(int)
	{
#if !SEOUL_ASSERTIONS_DISABLED
		HashTableIterator ret(m_pReferenceCount, m_pKeys, m_pValues, m_uCapacity, m_uIndex);
#else
		HashTableIterator ret(nullptr, m_pKeys, m_pValues, m_uCapacity, m_uIndex);
#endif

		m_uIndex = Index(m_uIndex + 1);

		return ret;
	}

	Bool operator==(const HashTableIterator& b) const
	{
		return (m_pKeys == b.m_pKeys && m_uIndex == b.m_uIndex);
	}

	Bool operator!=(const HashTableIterator& b) const
	{
		return !(*this == b);
	}

private:
	KEY* m_pKeys;
	VALUE* m_pValues;
	UInt32 m_uCapacity;
	UInt32 m_uIndex;
#if !SEOUL_ASSERTIONS_DISABLED
	CheckedPtr<Atomic32> m_pReferenceCount;
#endif

	UInt32 Index(UInt32 uIndex) const
	{
		KEY const kNullKey = TRAITS::GetNullKey();
		while (uIndex < m_uCapacity && kNullKey == m_pKeys[uIndex])
		{
			uIndex++;
		}

		return uIndex;
	}
}; // class HashTableIterator

/**
 * Implement of a const access iterator for HashTable.
 */
template <typename KEY, typename VALUE, typename TRAITS>
struct ConstHashTableIterator
{
	struct IteratorPair
	{
		IteratorPair(const KEY& key, const VALUE& value)
			: First(key)
			, Second(value)
		{
		}

		KEY const& First;
		VALUE const& Second;
	};


	struct IteratorPairIndirect
	{
		IteratorPairIndirect(const KEY& key, const VALUE& value)
			: m_Pair(key, value)
		{
		}

		IteratorPair m_Pair;

		IteratorPair const* operator->() const
		{
			return &m_Pair;
		}
	};

	ConstHashTableIterator(Atomic32* pReferenceCount, KEY const* pKeys, VALUE const* pValues, UInt32 uCapacity, UInt32 uIndex = 0)
		: m_pKeys(pKeys)
		, m_pValues(pValues)
		, m_uCapacity(uCapacity)
		, m_uIndex(Index(uIndex))
#if !SEOUL_ASSERTIONS_DISABLED
		, m_pReferenceCount(pReferenceCount)
#endif
	{
#if !SEOUL_ASSERTIONS_DISABLED
		if (m_pReferenceCount.IsValid())
		{
			++(*m_pReferenceCount);
		}
#endif
	}

	ConstHashTableIterator(const ConstHashTableIterator& b)
		: m_pKeys(b.m_pKeys)
		, m_pValues(b.m_pValues)
		, m_uCapacity(b.m_uCapacity)
		, m_uIndex(b.m_uIndex)
#if !SEOUL_ASSERTIONS_DISABLED
		, m_pReferenceCount(b.m_pReferenceCount)
#endif
	{
#if !SEOUL_ASSERTIONS_DISABLED
		if (m_pReferenceCount.IsValid())
		{
			++(*m_pReferenceCount);
		}
#endif
	}

	~ConstHashTableIterator()
	{
#if !SEOUL_ASSERTIONS_DISABLED
		if (m_pReferenceCount.IsValid())
		{
			--(*m_pReferenceCount);
		}
#endif
	}

	ConstHashTableIterator& operator=(const ConstHashTableIterator& b)
	{
		m_pKeys = b.m_pKeys;
		m_pValues = b.m_pValues;
		m_uCapacity = b.m_uCapacity;
		m_uIndex = b.m_uIndex;

#if !SEOUL_ASSERTIONS_DISABLED
		if (m_pReferenceCount.IsValid())
		{
			--(*m_pReferenceCount);
		}

		m_pReferenceCount = b.m_pReferenceCount;

		if (m_pReferenceCount.IsValid())
		{
			++(*m_pReferenceCount);
		}
#endif

		return *this;
	}

	IteratorPair operator*() const
	{
		return IteratorPair(m_pKeys[m_uIndex], m_pValues[m_uIndex]);
	}

	IteratorPairIndirect operator->() const
	{
		return IteratorPairIndirect(m_pKeys[m_uIndex], m_pValues[m_uIndex]);
	}

	ConstHashTableIterator& operator++()
	{
		m_uIndex = Index(m_uIndex + 1);

		return *this;
	}

	ConstHashTableIterator operator++(int)
	{
#if !SEOUL_ASSERTIONS_DISABLED
		ConstHashTableIterator ret(m_pReferenceCount, m_pKeys, m_pValues, m_uCapacity, m_uIndex);
#else
		ConstHashTableIterator ret(nullptr, m_pKeys, m_pValues, m_uCapacity, m_uIndex);
#endif

		m_uIndex = Index(m_uIndex + 1);

		return ret;
	}

	Bool operator==(const ConstHashTableIterator& b) const
	{
		return (m_pKeys == b.m_pKeys && m_uIndex == b.m_uIndex);
	}

	Bool operator!=(const ConstHashTableIterator& b) const
	{
		return !(*this == b);
	}

private:
	KEY const* m_pKeys;
	VALUE const* m_pValues;
	UInt32 m_uCapacity;
	UInt32 m_uIndex;
#if !SEOUL_ASSERTIONS_DISABLED
	CheckedPtr<Atomic32> m_pReferenceCount;
#endif

	UInt32 Index(UInt32 uIndex) const
	{
		KEY const kNullKey = TRAITS::GetNullKey();
		while (uIndex < m_uCapacity && kNullKey == m_pKeys[uIndex])
		{
			uIndex++;
		}

		return uIndex;
	}
}; // class ConstHashTableIterator

/**
 * HashTable stores keys in its bucketing array - as a result, it exhibits better
 * cache usage than a table with chaining, in cases where sizeof(KEY) is small and the loading
 * favor is not very high (typically a value of 0.75-0.8). In some situations, it can also
 * use less memory than Seoul::HashTable, for example, when both the key and values types
 * are smaller than 4 bytes.
 */
template <
	typename KEY,
	typename VALUE,
	int MEMORY_BUDGETS = MemoryBudgets::TBDContainer,
	typename TRAITS = DefaultHashTableKeyTraits<KEY> >
class HashTable SEOUL_SEALED
{
public:
	/** Type of keys in this HashTable. */
	typedef KEY KeyType;

	/** Type of values in this HashTable. */
	typedef VALUE ValueType;

	/** Iterator for constant iteration of this HashTable. */
	typedef ConstHashTableIterator<KEY, VALUE, TRAITS> ConstIterator;

	/* range-based for loop */
	typedef ConstIterator const_iterator;

	/** Iterator for read-write iteration of this HashTable. */
	typedef HashTableIterator<KEY, VALUE, TRAITS> Iterator;

	/* range-based for loop */
	typedef Iterator iterator;

	/** Comparator that is used to determine if key A is equal to key B. */
	typedef HashTableComparator<KEY, TRAITS::kCheckHashBeforeEquals> Comparator;

	/** Size type used for storing capacities and counts of the hash table. */
	typedef UInt32 SizeType;

	/** Traits used by this HashTable<> */
	typedef TRAITS Traits;

	HashTable()
		: m_pKeys(nullptr)
		, m_pValues(nullptr)
		, m_uCapacityExcludingNull(0)
		, m_bHasNullStorage(false)
		, m_uCountExcludingNull(0)
		, m_bHasNull(false)
#if !SEOUL_ASSERTIONS_DISABLED
		, m_IteratorReferenceCount(0)
#endif
	{
	}

	/**
	 * Construct a HashTable with an initial capacity - avoids resizing while
	 * inserting elements later if a reasonable estimate of the final capacity is available
	 * initially.
	 */
	explicit HashTable(SizeType initialCapacity)
		: m_pKeys(nullptr)
		, m_pValues(nullptr)
		, m_uCapacityExcludingNull(0)
		, m_bHasNullStorage(false)
		, m_uCountExcludingNull(0)
		, m_bHasNull(false)
#if !SEOUL_ASSERTIONS_DISABLED
		, m_IteratorReferenceCount(0)
#endif
	{
		Grow(initialCapacity);
	}

	HashTable(const HashTable& b)
		: m_pKeys(nullptr)
		, m_pValues(nullptr)
		, m_uCapacityExcludingNull(b.m_uCapacityExcludingNull)
		, m_bHasNullStorage(b.m_bHasNullStorage)
		, m_uCountExcludingNull(b.m_uCountExcludingNull)
		, m_bHasNull(b.m_bHasNull)
#if !SEOUL_ASSERTIONS_DISABLED
		, m_IteratorReferenceCount(0)
#endif
	{
		m_pKeys = (KEY*)MemoryManager::AllocateAligned(
			(m_uCapacityExcludingNull + m_bHasNullStorage) * sizeof(KEY),
			(MemoryBudgets::Type)MEMORY_BUDGETS,
			SEOUL_ALIGN_OF(KEY));

		m_pValues = (VALUE*)MemoryManager::AllocateAligned(
			(m_uCapacityExcludingNull + m_bHasNullStorage) * sizeof(VALUE),
			(MemoryBudgets::Type)MEMORY_BUDGETS,
			SEOUL_ALIGN_OF(VALUE));

		KEY const kNullKey = TRAITS::GetNullKey();
		for (UInt32 i = 0u; i < m_uCapacityExcludingNull; ++i)
		{
			if (kNullKey != b.m_pKeys[i])
			{
				new (m_pValues + i) VALUE(b.m_pValues[i]);
				new (m_pKeys + i) KEY(b.m_pKeys[i]);
			}
			else
			{
				new (m_pKeys + i) KEY(kNullKey);
			}
		}

		if (m_bHasNull)
		{
			new (m_pKeys + m_uCapacityExcludingNull) KEY(b.m_pKeys[b.m_uCapacityExcludingNull]);
			new (m_pValues + m_uCapacityExcludingNull) VALUE(b.m_pValues[b.m_uCapacityExcludingNull]);
		}
	}

	HashTable& operator=(const HashTable& b)
	{
		if (this == &b)
		{
			return *this;
		}

		// Non-ship sanity check - it is invalid to call mutations on a HashTable
		// when iterators to the table are defined.
		SEOUL_ASSERT(0 == m_IteratorReferenceCount);

		Destroy();

		m_uCapacityExcludingNull = b.m_uCapacityExcludingNull;
		m_bHasNullStorage = b.m_bHasNullStorage;
		m_uCountExcludingNull = b.m_uCountExcludingNull;
		m_bHasNull = b.m_bHasNull;

		m_pKeys = (KEY*)MemoryManager::AllocateAligned(
			(m_uCapacityExcludingNull + m_bHasNullStorage) * sizeof(KEY),
			(MemoryBudgets::Type)MEMORY_BUDGETS,
			SEOUL_ALIGN_OF(KEY));

		m_pValues = (VALUE*)MemoryManager::AllocateAligned(
			(m_uCapacityExcludingNull + m_bHasNullStorage) * sizeof(VALUE),
			(MemoryBudgets::Type)MEMORY_BUDGETS,
			SEOUL_ALIGN_OF(VALUE));

		KEY const kNullKey = TRAITS::GetNullKey();
		for (UInt32 i = 0u; i < m_uCapacityExcludingNull; ++i)
		{
			if (kNullKey != b.m_pKeys[i])
			{
				new (m_pValues + i) VALUE(b.m_pValues[i]);
				new (m_pKeys + i) KEY(b.m_pKeys[i]);
			}
			else
			{
				new (m_pKeys + i) KEY(kNullKey);
			}
		}

		if (m_bHasNull)
		{
			new (m_pKeys + m_uCapacityExcludingNull) KEY(b.m_pKeys[b.m_uCapacityExcludingNull]);
			new (m_pValues + m_uCapacityExcludingNull) VALUE(b.m_pValues[b.m_uCapacityExcludingNull]);
		}

		return *this;
	}

	~HashTable()
	{
		// Non-ship sanity check - it is invalid to call mutations on a HashTable
		// when iterators to the table are defined.
		SEOUL_ASSERT(0 == m_IteratorReferenceCount);

		Destroy();
	}

	/**
	 * @return Start of this HashTable for iteration.
	 */
	ConstIterator Begin() const
	{
		return ConstIterator(GetIteratorReferenceCountPtr(), m_pKeys, m_pValues, m_uCapacityExcludingNull);
	}

	/* range-based for loop */
	const_iterator begin() const { return Begin(); }

	/**
	 * @return Start of this HashTable for read-write iteration.
	 */
	Iterator Begin()
	{
		return Iterator(GetIteratorReferenceCountPtr(), m_pKeys, m_pValues, m_uCapacityExcludingNull);
	}

	/* range-based for loop */
	iterator begin() { return Begin(); }

	/**
	 * @return End of this HashTable for iteration.
	 */
	ConstIterator End() const
	{
		// Deliberate - to handle the special null case, the End() iterator is 1 passed the
		// capacity if null is present. The iterator will "do the right thing" in this case:
		// - it will stop at the null entry, since m_uCapacityExcludingNull will stop iteration,
		//   whether the last entry is a null key or not.
		return ConstIterator(GetIteratorReferenceCountPtr(), m_pKeys, m_pValues, m_uCapacityExcludingNull, m_uCapacityExcludingNull + m_bHasNull);
	}

	/* range-based for loop */
	const_iterator end() const { return End(); }

	/**
	 * @return End of this HashTable for read-write iteration.
	 */
	Iterator End()
	{
		// Deliberate - to handle the special null case, the End() iterator is 1 passed the
		// capacity if null is present. The iterator will "do the right thing" in this case:
		// - it will stop at the null entry, since m_uCapacityExcludingNull will stop iteration,
		//   whether the last entry is a null key or not.
		return Iterator(GetIteratorReferenceCountPtr(), m_pKeys, m_pValues, m_uCapacityExcludingNull, m_uCapacityExcludingNull + m_bHasNull);
	}

	/* range-based for loop */
	iterator end() { return End(); }

	/**
	 * Destroy all entries in the HashTable and set its size by to 0 - does
	 * not reduce the hash table capacity.
	 */
	void Clear()
	{
		// Non-ship sanity check - it is invalid to call mutations on a HashTable
		// when iterators to the table are defined.
		SEOUL_ASSERT(0 == m_IteratorReferenceCount);

		KEY const kNullKey = TRAITS::GetNullKey();
		for (ptrdiff_t i = (ptrdiff_t)m_uCapacityExcludingNull - 1; i >= 0; --i)
		{
			if (kNullKey != m_pKeys[i])
			{
				m_pValues[i].~VALUE();
				m_pKeys[i] = kNullKey;
			}
		}

		if (m_bHasNull)
		{
			m_pValues[m_uCapacityExcludingNull].~VALUE();
			m_pKeys[m_uCapacityExcludingNull] = kNullKey;
			m_bHasNull = false;
		}

		m_uCountExcludingNull = 0;
	}

	/**
	 * Destroy all entries in the HashTable and set its size by to 0, also
	 * deallocating any heap memory used by the HashTable.
	 */
	void Destroy()
	{
		// Non-ship sanity check - it is invalid to call mutations on a HashTable
		// when iterators to the table are defined.
		SEOUL_ASSERT(0 == m_IteratorReferenceCount);

		KEY const kNullKey = TRAITS::GetNullKey();
		for (ptrdiff_t i = (ptrdiff_t)m_uCapacityExcludingNull - 1; i >= 0; --i)
		{
			if (kNullKey != m_pKeys[i])
			{
				m_pValues[i].~VALUE();
			}
			m_pKeys[i].~KEY();
		}

		if (m_bHasNull)
		{
			m_pValues[m_uCapacityExcludingNull].~VALUE();
			m_pKeys[m_uCapacityExcludingNull].~KEY();
			m_bHasNull = false;
		}

		MemoryManager::Deallocate(m_pValues);
		m_pValues = nullptr;

		MemoryManager::Deallocate(m_pKeys);
		m_pKeys = nullptr;

		m_bHasNullStorage = false;
		m_uCapacityExcludingNull = 0;
		m_bHasNull = false;
		m_uCountExcludingNull = 0;
	}

	/**
	 * Remove an element with key key from this HashTable.
	 *
	 * @return True if an element was removed, false otherwise.
	 */
	Bool Erase(const KEY& key)
	{
		// Non-ship sanity check - it is invalid to call mutations on a HashTable
		// when iterators to the table are defined.
		SEOUL_ASSERT(0 == m_IteratorReferenceCount);

		// Special case - nothing to do if no entries in the table.
		if (0 == (m_uCountExcludingNull + m_bHasNull))
		{
			return false;
		}

		KEY const kNullKey = TRAITS::GetNullKey();

		// Special case handling of kNullKey.
		if (key == kNullKey)
		{
			if (m_bHasNull)
			{
				m_pValues[m_uCapacityExcludingNull].~VALUE();
				m_pKeys[m_uCapacityExcludingNull] = kNullKey;
				m_bHasNull = false;
				return true;
			}

			return false;
		}

		UInt32 const uHash = GetHash(key);
		UInt32 uIndex = uHash;

		while (true)
		{
			uIndex &= (m_uCapacityExcludingNull - 1);

			KEY const entryKey(m_pKeys[uIndex]);

			// If entryKey is the entry we want to remove.
			if (Comparator::Equals(uHash, key, entryKey))
			{
				// Delete the entry.
				m_pValues[uIndex].~VALUE();
				m_pKeys[uIndex] = kNullKey;
				m_uCountExcludingNull--;

				// Now we need to compact the array, by filling in holes - walk the
				// array starting at the next element, attempting to reinsert every element,
				// until we hit an existing null element (an existing hole).
				++uIndex;
				while (true)
				{
					uIndex &= (m_uCapacityExcludingNull - 1);

					KEY const innerEntryKey(m_pKeys[uIndex]);

					// If the key is null, we're done - return true for a successful erase.
					if (kNullKey == innerEntryKey)
					{
						return true;
					}
					// Otherwise, if the entry key is not in its home position, try to reinsert it.
					else if ((GetHash(innerEntryKey) & (m_uCapacityExcludingNull - 1)) != uIndex)
					{
						// First decrement the count - either we'll restore it (if the insertion
						// fails and the entry stays in the same slot), or it'll be incremented
						// on the successful reinsertion of the entry.
						m_uCountExcludingNull--;

						// If the key was inserted (which can only happen if it didn't end up in the same
						// slot, destroy the old entry. Insert() will also have incremented the count, so
						// we don't need to do it again.
						if (Insert(innerEntryKey, m_pValues[uIndex]).Second)
						{
							m_pValues[uIndex].~VALUE();
							m_pKeys[uIndex] = kNullKey;
						}
						// Otherwise, restore the count, since it means the entry just hashed to the same
						// location (eventually, the Insert() hit the existing element).
						else
						{
							m_uCountExcludingNull++;
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
	 * Add a new entry to this HashTable - fails if an entry already
	 * has the same key as key.
	 *
	 * @return A pair object whose first element is an iterator pointing either
	 * to the newly inserted element or to the element whose key is equivalent,
	 * and a bool value indicating whether the element was successfully inserted or not.
	 */
	Pair<Iterator, Bool> Insert(const KEY& key, const VALUE& value)
	{
		// Non-ship sanity check - it is invalid to call mutations on a HashTable
		// when iterators to the table are defined.
		SEOUL_ASSERT(0 == m_IteratorReferenceCount);

		return Insert(key, value, false);
	}

	/**
	 * Add a new entry to this HashTable - fails if bOverwrite is false
	 * and an entry already has the same key as key.
	 *
	 * @return A pair object whose first element is an iterator pointing either
	 * to the newly inserted element or to the element whose key is equivalent,
	 * and a bool value indicating whether the element was successfully inserted or not.
	 */
	Pair<Iterator, Bool> Insert(const KEY& key, const VALUE& value, Bool bOverwrite)
	{
		typedef Pair<Iterator, Bool> ReturnType;

		// Non-ship sanity check - it is invalid to call mutations on a HashTable
		// when iterators to the table are defined.
		SEOUL_ASSERT(0 == m_IteratorReferenceCount);

		// If increasing the number of elements by 1 will place us at or above
		// the load factor for KEY, grow the array capacity to the next power of 2
		// beyond the current capacity.
		if ((SizeType)(m_uCountExcludingNull + 1) >= (SizeType)(m_uCapacityExcludingNull * TRAITS::GetLoadFactor()))
		{
			// Always grow so there will be at least one nullptr entry in the table - all the
			// querying logic depends on this, in exchange, the logic requires fewer branches.
			Grow(GetNextPowerOf2(m_uCapacityExcludingNull + 2));
		}

		KEY const kNullKey = TRAITS::GetNullKey();

		// Special case handling of kNullKey.
		if (key == kNullKey)
		{
			if (!m_bHasNull || bOverwrite)
			{
				// Make sure we have the extra slot for the special
				// case nullptr key-value.
				CheckAndGrowForNullKey();

				// Key is always assigned, since it is always initialized.
				m_pKeys[m_uCapacityExcludingNull] = key;

				// Value is assigned if already initialized, otherwise we need
				// to construct it with placement new.
				if (m_bHasNull)
				{
					m_pValues[m_uCapacityExcludingNull] = value;
				}
				else
				{
					new (m_pValues + m_uCapacityExcludingNull) VALUE(value);
				}

				// Always have a nullptr key-value pair at this point.
				m_bHasNull = true;

				return ReturnType(Iterator(GetIteratorReferenceCountPtr(), m_pKeys, m_pValues, m_uCapacityExcludingNull, m_uCapacityExcludingNull), true);
			}
			else
			{
				return ReturnType(Iterator(GetIteratorReferenceCountPtr(), m_pKeys, m_pValues, m_uCapacityExcludingNull, m_uCapacityExcludingNull), false);
			}
		}

		UInt32 const uHash = GetHash(key);

		UInt32 uIndex = uHash;
		uIndex &= (m_uCapacityExcludingNull - 1);

		// Step one is to apply "anti-clustering" - if the "home" index (the initial hashed
		// index) of the key being inserted is already occupied by an entry with a home index
		// other than the index it occupies, we replace the existing entry with the new entry,
		// and then reinsert the existing entry. This maximizes the number of keys at their
		// home.
		KEY const entryKey(m_pKeys[uIndex]);
		if (kNullKey != entryKey)
		{
			UInt32 const uEntryHash = GetHash(entryKey);
			UInt32 const uEntryHomeIndex = (uEntryHash & (m_uCapacityExcludingNull - 1));

			// If the existing entry is not in its home, replace it with the new value,
			// then reinsert the existing entry.
			if (uEntryHomeIndex != uIndex)
			{
				VALUE const replacedValue(m_pValues[uIndex]);

				m_pKeys[uIndex] = key;
				m_pValues[uIndex] = value;

				// Reinserting the existing entry must always succeed, and the return iterator
				// points at the just inserted values.
				SEOUL_VERIFY(InternalInsert(entryKey, replacedValue, uEntryHash, uEntryHomeIndex, bOverwrite).Second);
				return ReturnType(Iterator(GetIteratorReferenceCountPtr(), m_pKeys, m_pValues, m_uCapacityExcludingNull, uIndex), true);
			}
			// The home index is occupied by an entry that is already in its home, so we need
			// to continue trying to insert the new key-value pair.
			else
			{
				return InternalInsert(key, value, uHash, uIndex, bOverwrite);
			}
		}
		// The entry is unoccupied, so we can just insert the new key-value pair.
		else
		{
			m_pKeys[uIndex] = key;
			new (m_pValues + uIndex) VALUE(value);
			++m_uCountExcludingNull;

			return ReturnType(Iterator(GetIteratorReferenceCountPtr(), m_pKeys, m_pValues, m_uCapacityExcludingNull, uIndex), true);
		}
	}

	/**
	 * @return The number of entires in this HashTable.
	 */
	SizeType GetSize() const
	{
		return (m_uCountExcludingNull + m_bHasNull);
	}

	/**
	 * @return The total size of this HashTable's bucketing array.
	 */
	SizeType GetCapacity() const
	{
		return m_uCapacityExcludingNull;
	}

	/**
	 * @return The value associated with key, or nullptr if key is not in this
	 * HashTable.
	 */
	VALUE* Find(const KEY& key)
	{
		// Special case - nothing to do if no entries in the table.
		if (0 == (m_uCountExcludingNull + m_bHasNull))
		{
			return nullptr;
		}

		KEY const kNullKey = TRAITS::GetNullKey();

		// Special case handling of kNullKey.
		if (key == kNullKey)
		{
			if (m_bHasNull)
			{
				return (m_pValues + m_uCapacityExcludingNull);
			}

			return nullptr;
		}

		UInt32 const uHash = GetHash(key);
		UInt32 uIndex = uHash;

		while (true)
		{
			uIndex &= (m_uCapacityExcludingNull - 1);

			// If we've found the desired key, assign the value and return true.
			KEY const entryKey(m_pKeys[uIndex]);
			if (Comparator::Equals(uHash, key, entryKey))
			{
				return (m_pValues + uIndex);
			}
			// If we hit a nullptr key, the entry is not in the table.
			else if (entryKey == kNullKey)
			{
				return nullptr;
			}

			++uIndex;
		}
	}

	/**
	 * @return The value associated with key, or nullptr if key is not in this
	 * HashTable.
	 */
	VALUE const* Find(const KEY& key) const
	{
		return const_cast<HashTable*>(this)->Find(key);
	}

	/**
	 * Attempt to retrieve the value associated with key in this HashTable,
	 * assigning it to rOutValue.
	 *
	 * @return True if the value was assigned, false otherwise.
	 */
	Bool GetValue(const KEY& key, VALUE& rOutValue) const
	{
		VALUE const* pValue = Find(key);
		if (nullptr != pValue)
		{
			rOutValue = *pValue;
			return true;
		}

		return false;
	}

	/** @return The total memory footprint of this HashTable in bytes. */
	UInt32 GetMemoryUsageInBytes() const
	{
		return (UInt32)(sizeof(*this) + (m_uCapacityExcludingNull + m_bHasNull) * (sizeof(KEY) + sizeof(VALUE)));
	}

	/**
	 * Increase the size of this HashTable to the next power of 2,
	 * greater than or equal to nNewCapacity.
	 */
	void Grow(SizeType nNewCapacity)
	{
		KEY const kNullKey = TRAITS::GetNullKey();
		nNewCapacity = GetNextPowerOf2(nNewCapacity);
		SizeType const nOldCapacity = m_uCapacityExcludingNull;

		if (nNewCapacity > nOldCapacity)
		{
			// Non-ship sanity check - it is invalid to call mutations on a HashTable
			// when iterators to the table are defined.
			SEOUL_ASSERT(0 == m_IteratorReferenceCount);

			// Deliberate - we use a resize as an opportunity to drop
			// the extra storage for the special case nullptr key and value,
			// so we only include the space if m_bHasNull is true, not
			// if m_bHasNullStorage is true.
			KEY* pKeys = (KEY*)MemoryManager::AllocateAligned(
				(nNewCapacity + m_bHasNull) * sizeof(KEY),
				(MemoryBudgets::Type)MEMORY_BUDGETS,
				SEOUL_ALIGN_OF(KEY));

			VALUE* pValues = (VALUE*)MemoryManager::AllocateAligned(
				(nNewCapacity + m_bHasNull)  * sizeof(VALUE),
				(MemoryBudgets::Type)MEMORY_BUDGETS,
				SEOUL_ALIGN_OF(VALUE));

			// Keys need to be initialized to nullptr, values are left
			// uninitialized until they are assigned.
			for (SizeType i = 0; i < nNewCapacity; ++i)
			{
				new (pKeys + i) KEY(kNullKey);
			}

			// Initialize the key for the special nullptr member if present.
			if (m_bHasNull)
			{
				new (pKeys + nNewCapacity) KEY(kNullKey);
			}

			// Switch the old and new keys/values in preparation
			// for reinserting.
			Seoul::Swap(pKeys, m_pKeys);
			Seoul::Swap(pValues, m_pValues);

			// Update nullptr presence - m_bHasNull is always set to
			// false. m_bHasNullstorage is only true if m_bHasNull
			// was true.
			Bool const bHasNull = m_bHasNull;
			m_bHasNullStorage = m_bHasNull;
			m_bHasNull = false;

			// "Empty" the count and increase the capacity.
			m_uCountExcludingNull = 0;
			m_uCapacityExcludingNull = nNewCapacity;

			// Now insert all the old key/value pairs.
			for (SizeType i = 0; i < nOldCapacity; ++i)
			{
				if (kNullKey != pKeys[i])
				{
					SEOUL_VERIFY(Insert(pKeys[i], pValues[i]).Second);
				}
			}

			// Insert the special nullptr value if defined.
			if (bHasNull)
			{
				SEOUL_ASSERT(pKeys[nOldCapacity] == kNullKey);
				SEOUL_ASSERT(m_bHasNullStorage);
				SEOUL_VERIFY(Insert(pKeys[nOldCapacity], pValues[nOldCapacity]).Second);
				SEOUL_ASSERT(m_bHasNull);
			}

			// Cleanup the old nullptr value if present.
			if (bHasNull)
			{
				pValues[nOldCapacity].~VALUE();
				pKeys[nOldCapacity].~KEY();
			}

			// Now cleanup the old normal data.
			for (ptrdiff_t i = (ptrdiff_t)nOldCapacity - 1; i >= 0; --i)
			{
				if (kNullKey != pKeys[i])
				{
					pValues[i].~VALUE();
				}
				pKeys[i].~KEY();
			}

			MemoryManager::Deallocate(pValues);
			MemoryManager::Deallocate(pKeys);
		}
	}

	/**
	 * @return True if key is in this HashTable, false otherwise.
	 */
	Bool HasValue(const KEY& key) const
	{
		// Special case - nothing to do if no entries in the table.
		if (0 == (m_uCountExcludingNull + m_bHasNull))
		{
			return false;
		}

		KEY const kNullKey = TRAITS::GetNullKey();

		// Special case handling of kNullKey.
		if (key == kNullKey)
		{
			return m_bHasNull;
		}

		UInt32 const uHash = GetHash(key);
		UInt32 uIndex = uHash;

		while (true)
		{
			uIndex &= (m_uCapacityExcludingNull - 1);

			// If we've found the desired key, return true.
			KEY const entryKey(m_pKeys[uIndex]);
			if (Comparator::Equals(uHash, key, entryKey))
			{
				return true;
			}
			// Otherwise, if we hit a nullptr key, the entry is not in the table.
			else if (entryKey == kNullKey)
			{
				return false;
			}

			++uIndex;
		}
	}

	/**
	 * @return True if this HashTable has no entries, false otherwise.
	 */
	Bool IsEmpty() const
	{
		return (0 == (m_uCountExcludingNull + m_bHasNull));
	}

	/**
	 * Syntactic sugar for Insert() with a bOverwrite argument of true.
	 */
	Pair<Iterator, Bool> Overwrite(const KEY& key, const VALUE& value)
	{
		return Insert(key, value, true);
	}

	/**
	 * Increase the capacity of this HashTable
	 * to at least nNewCapacity.
	 */
	void Reserve(SizeType nNewCapacity)
	{
		if (nNewCapacity > m_uCapacityExcludingNull)
		{
			Grow(nNewCapacity);
		}
	}

	/**
	 * Swap the state of this HashTable with b.
	 */
	void Swap(HashTable& b)
	{
		// Non-ship sanity check - it is invalid to call mutations on a HashTable
		// when iterators to the table are defined.
		SEOUL_ASSERT(0 == m_IteratorReferenceCount);

		Seoul::Swap(m_pKeys, b.m_pKeys);
		Seoul::Swap(m_pValues, b.m_pValues);
		Seoul::Swap(m_nCapacityExcludingNullAndHasNullStorage, b.m_nCapacityExcludingNullAndHasNullStorage);
		Seoul::Swap(m_nCountExcludingNullAndHasNull, b.m_nCountExcludingNullAndHasNull);
	}

private:
	KEY* m_pKeys;
	VALUE* m_pValues;
	union
	{
		struct
		{
			SizeType m_uCapacityExcludingNull : 31;
			SizeType m_bHasNullStorage : 1;
		};
		SizeType m_nCapacityExcludingNullAndHasNullStorage;
	};
	union
	{
		struct
		{
			SizeType m_uCountExcludingNull : 31;
			SizeType m_bHasNull: 1;
		};
		SizeType m_nCountExcludingNullAndHasNull;
	};
#if !SEOUL_ASSERTIONS_DISABLED
	Atomic32 m_IteratorReferenceCount;
#endif

	Atomic32* GetIteratorReferenceCountPtr() const
	{
#if !SEOUL_ASSERTIONS_DISABLED
		return const_cast<Atomic32*>(&m_IteratorReferenceCount);
#else
		return nullptr;
#endif
	}

	/**
	 * Special handling for the special-case nullptr key. A key-value
	 * pair which legitimately uses the nullptr key is stored in a "hidden"
	 * extra slot at the end of the key and value memory buffers. We
	 * don't add this hiddens slot until it is needed.
	 */
	void CheckAndGrowForNullKey()
	{
		// Nothing to do if we already have nullptr storage.
		if (!m_bHasNullStorage)
		{
			// Non-ship sanity check - it is invalid to call mutations on a HashTable
			// when iterators to the table are defined.
			SEOUL_ASSERT(0 == m_IteratorReferenceCount);

			KEY const kNullKey = TRAITS::GetNullKey();

			// Allocate new buffers, m_uCapacityExcludingNull + 1
			// to add room for the nullptr key.
			KEY* pKeys = (KEY*)MemoryManager::AllocateAligned(
				(m_uCapacityExcludingNull + 1) * sizeof(KEY),
				(MemoryBudgets::Type)MEMORY_BUDGETS,
				SEOUL_ALIGN_OF(KEY));

			VALUE* pValues = (VALUE*)MemoryManager::AllocateAligned(
				(m_uCapacityExcludingNull + 1)  * sizeof(VALUE),
				(MemoryBudgets::Type)MEMORY_BUDGETS,
				SEOUL_ALIGN_OF(VALUE));

			// Initialize all keys and values - this is simpler
			// than a standard resize, since we're just copying through
			// all the normal elements.
			for (SizeType i = 0; i < m_uCapacityExcludingNull; ++i)
			{
				new (pKeys + i) KEY(m_pKeys[i]);
				if (kNullKey != m_pKeys[i])
				{
					new (pValues + i) VALUE(m_pValues[i]);
				}
			}

			// Initialize the nullptr key slot.
			new (pKeys + m_uCapacityExcludingNull) KEY(kNullKey);

			// Swap buffers and then cleanup the buffers we're
			// about to deallocate.
			Seoul::Swap(pKeys, m_pKeys);
			Seoul::Swap(pValues, m_pValues);

			// Now cleanup the old data.
			for (ptrdiff_t i = (ptrdiff_t)m_uCapacityExcludingNull - 1; i >= 0; --i)
			{
				if (kNullKey != pKeys[i])
				{
					pValues[i].~VALUE();
				}
				pKeys[i].~KEY();
			}

			MemoryManager::Deallocate(pValues);
			MemoryManager::Deallocate(pKeys);

			// Done - this HashTable now has storage for a nullptr key-value pair.
			m_bHasNullStorage = true;
		}
	}

	/**
	 * Helper function only used by Insert() - uHash is the already computed hash
	 * of key, and uIndex is the already rounded (uIndex & (m_uCapacityExcludingNull - 1)) index
	 * to start searching for a free slot from.
	 */
	Pair<Iterator, Bool> InternalInsert(
		const KEY& key,
		const VALUE& value,
		UInt32 const uHash,
		UInt32 uIndex,
		Bool bOverwrite)
	{
		typedef Pair<Iterator, Bool> ReturnType;

		KEY const kNullKey = TRAITS::GetNullKey();

		while (true)
		{
			// If an entry exists that has the same key as the entry being inserted.
			KEY const entryKey(m_pKeys[uIndex]);
			if (Comparator::Equals(uHash, key, entryKey))
			{
				// If bOverwrite is true, replace the existing value, otherwise fail.
				if (bOverwrite)
				{
					m_pValues[uIndex] = value;
					return ReturnType(Iterator(GetIteratorReferenceCountPtr(), m_pKeys, m_pValues, m_uCapacityExcludingNull, uIndex), true);
				}
				else
				{
					return ReturnType(Iterator(GetIteratorReferenceCountPtr(), m_pKeys, m_pValues, m_uCapacityExcludingNull, uIndex), false);
				}
			}
			// Otherwise, if we've hit a nullptr key, we're done. Insert the key and return success.
			else if (entryKey == kNullKey)
			{
				m_pKeys[uIndex] = key;
				new (m_pValues + uIndex) VALUE(value);
				++m_uCountExcludingNull;

				return ReturnType(Iterator(GetIteratorReferenceCountPtr(), m_pKeys, m_pValues, m_uCapacityExcludingNull, uIndex), true);
			}

			++uIndex;
			uIndex &= (m_uCapacityExcludingNull - 1);
		}
	}
}; // class HashTable

/** @return True if HashTableA == HashTableB, false otherwise. */
template <typename TKA, typename TKB, int MEMORY_BUDGETS_A, typename TVA, typename TVB, int MEMORY_BUDGETS_B>
inline Bool operator==(const HashTable<TKA, TVA, MEMORY_BUDGETS_A>& tA, const HashTable<TKB, TVB, MEMORY_BUDGETS_B>& tB)
{
	if (tA.GetSize() != tB.GetSize())
	{
		return false;
	}

	for (auto const& e : tA)
	{
		auto pB = tB.Find(e.First);
		if (nullptr == pB)
		{
			return false;
		}

		if (e.Second != *pB)
		{
			return false;
		}
	}

	return true;
}

/** @return True if HashTableA == HashTableB, false otherwise. */
template <typename TKA, typename TKB, int MEMORY_BUDGETS_A, typename TVA, typename TVB, int MEMORY_BUDGETS_B>
inline Bool operator!=(const HashTable<TKA, TVA, MEMORY_BUDGETS_A>& tA, const HashTable<TKB, TVB, MEMORY_BUDGETS_B>& tB)
{
	return !(tA == tB);
}

/**
 * Deletes heap allocated objects in the value slot of the hash
 * table rtHashTable and then clears the table.
 */
template <typename T, typename U, int MEMORY_BUDGETS, typename TRAITS>
inline void SafeDeleteTable(HashTable<T, U*, MEMORY_BUDGETS, TRAITS>& rtHashTable)
{
	for (auto i = rtHashTable.Begin(); i != rtHashTable.End(); ++i)
	{
		SEOUL_DELETE i->Second;
		i->Second = nullptr;
	}
	rtHashTable.Clear();
}

/**
 * Deletes heap allocated objects in the value slot of the hash
 * table rtHashTable and then clears the table.
 */
template <typename T, typename U, int MEMORY_BUDGETS, typename TRAITS>
inline void SafeDeleteTable(HashTable<T, CheckedPtr<U>, MEMORY_BUDGETS, TRAITS>& rtHashTable)
{
	for (auto i = rtHashTable.Begin(); i != rtHashTable.End(); ++i)
	{
		SEOUL_DELETE i->Second;
		i->Second = nullptr;
	}
	rtHashTable.Clear();
}

/**
 * Equivalent to std::swap(). Override specifically for HashTable<>.
 */
template <typename KEY, typename VALUE, int MEMORY_BUDGETS, typename TRAITS >
inline void Swap(HashTable<KEY, VALUE, MEMORY_BUDGETS, TRAITS>& a, HashTable<KEY, VALUE, MEMORY_BUDGETS, TRAITS>& b)
{
	a.Swap(b);
}

/** Utility, gather all HashTable key and value pairs into a container. */
template <typename FROM, typename TO>
static inline void GetHashTableEntries(const FROM& from, TO& rOut)
{
	TO to;
	to.Reserve(from.GetSize());
	{
		typename FROM::ConstIterator const iBegin = from.Begin();
		typename FROM::ConstIterator const iEnd = from.End();
		for (typename FROM::ConstIterator i = iBegin; iEnd != i; ++i)
		{
			to.PushBack(MakePair(i->First, i->Second));
		}
	}

	rOut.Swap(to);
}

/** Utility, gather all HashTable keys into a container. */
template <typename FROM, typename TO>
static inline void GetHashTableKeys(const FROM& from, TO& rOut)
{
	TO to;
	to.Reserve(from.GetSize());
	{
		typename FROM::ConstIterator const iBegin = from.Begin();
		typename FROM::ConstIterator const iEnd = from.End();
		for (typename FROM::ConstIterator i = iBegin; iEnd != i; ++i)
		{
			to.PushBack(i->First);
		}
	}

	rOut.Swap(to);
}

} // namespace Seoul

#endif // include guard
