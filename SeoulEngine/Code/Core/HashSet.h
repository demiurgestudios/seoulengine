/**
 * \file HashSet.h
 * \brief HashSet is an associative key array with the
 * following properties:
 * - the key type must define a "empty" value that is used to represent
 *   undefined entries in the set.
 * - the capacity of the set is always a power of 2.
 * - keys are stored in arrays with no chaining.
 *
 * HashSet can be viewed as a rough equivalent to std::unordered_set<>, with
 * several differences, primarily:
 * - CamelCase function and members names.
 * - additional GetCapacity() method.
 * - additional HasKey() method.
 * - empty() is called IsEmpty().
 * - erase() equivalent Erase() returns a Bool instead of a size_t.
 * - find() equivalent Find() returns a pointer to the Key, not an iterator.
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
#ifndef HASH_SET
#define HASH_SET

#include "Algorithms.h"
#include "Atomic32.h"
#include "CheckedPtr.h"
#include "HashTable.h"
#include "HashFunctions.h"
#include "Pair.h"
#include "Prereqs.h"

namespace Seoul
{

/**
 * Implementation of a read-write access iterator for HashSet.
 */
template <typename KEY, typename TRAITS>
struct HashSetIterator
{
	HashSetIterator(Atomic32* pReferenceCount, KEY* pKeys, UInt32 uCapacity, UInt32 uIndex = 0)
		: m_pKeys(pKeys)
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

	HashSetIterator(const HashSetIterator& b)
		: m_pKeys(b.m_pKeys)
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

	~HashSetIterator()
	{
#if !SEOUL_ASSERTIONS_DISABLED
		if (m_pReferenceCount.IsValid())
		{
			--(*m_pReferenceCount);
		}
#endif
	}

	HashSetIterator& operator=(const HashSetIterator& b)
	{
		m_pKeys = b.m_pKeys;
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

	const KEY& operator*() const
	{
		return m_pKeys[m_uIndex];
	}

	KEY* operator->() const
	{
		return m_pKeys + m_uIndex;
	}

	HashSetIterator& operator++()
	{
		m_uIndex = Index(m_uIndex + 1);

		return *this;
	}

	HashSetIterator operator++(int)
	{
#if !SEOUL_ASSERTIONS_DISABLED
		HashSetIterator ret(m_pReferenceCount, m_pKeys, m_uCapacity, m_uIndex);
#else
		HashSetIterator ret(nullptr, m_pKeys, m_uCapacity, m_uIndex);
#endif

		m_uIndex = Index(m_uIndex + 1);

		return ret;
	}

	Bool operator==(const HashSetIterator& b) const
	{
		return (m_pKeys == b.m_pKeys && m_uIndex == b.m_uIndex);
	}

	Bool operator!=(const HashSetIterator& b) const
	{
		return !(*this == b);
	}

private:
	KEY* m_pKeys;
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
}; // class HashSetIterator

/**
 * Implement of a const access iterator for HashSet.
 */
template <typename KEY, typename TRAITS>
struct ConstHashSetIterator
{
	ConstHashSetIterator(Atomic32* pReferenceCount, KEY const* pKeys, UInt32 uCapacity, UInt32 uIndex = 0)
		: m_pKeys(pKeys)
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

	ConstHashSetIterator(const ConstHashSetIterator& b)
		: m_pKeys(b.m_pKeys)
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

	~ConstHashSetIterator()
	{
#if !SEOUL_ASSERTIONS_DISABLED
		if (m_pReferenceCount.IsValid())
		{
			--(*m_pReferenceCount);
		}
#endif
	}

	ConstHashSetIterator& operator=(const ConstHashSetIterator& b)
	{
		m_pKeys = b.m_pKeys;
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

	const KEY& operator*() const
	{
		return m_pKeys[m_uIndex];
	}

	KEY const* operator->() const
	{
		return m_pKeys + m_uIndex;
	}

	ConstHashSetIterator& operator++()
	{
		m_uIndex = Index(m_uIndex + 1);

		return *this;
	}

	ConstHashSetIterator operator++(int)
	{
#if !SEOUL_ASSERTIONS_DISABLED
		ConstHashSetIterator ret(m_pReferenceCount, m_pKeys, m_uCapacity, m_uIndex);
#else
		ConstHashSetIterator ret(nullptr, m_pKeys, m_uCapacity, m_uIndex);
#endif

		m_uIndex = Index(m_uIndex + 1);

		return ret;
	}

	Bool operator==(const ConstHashSetIterator& b) const
	{
		return (m_pKeys == b.m_pKeys && m_uIndex == b.m_uIndex);
	}

	Bool operator!=(const ConstHashSetIterator& b) const
	{
		return !(*this == b);
	}

private:
	KEY const* m_pKeys;
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
}; // class ConstHashSetIterator

/**
 * HashSet stores keys in its bucketing array - as a result, it exhibits better
 * cache usage than a table with chaining, in cases where sizeof(KEY) is small and the loading
 * favor is not very high (typically a value of 0.75-0.8). In some situations, it can also
 * use less memory than Seoul::HashSet, for example, when the key type is smaller than 4 bytes.
 */
template <
	typename KEY,
	int MEMORY_BUDGETS = MemoryBudgets::TBDContainer,
	typename TRAITS = DefaultHashTableKeyTraits<KEY> >
class HashSet SEOUL_SEALED
{
public:
	/** Type of keys in this HashSet. */
	typedef KEY KeyType;

	/** Iterator for constant iteration of this HashSet. */
	typedef ConstHashSetIterator<KEY, TRAITS> ConstIterator;

	/* range-based for loop */
	typedef ConstIterator const_iterator;

	/** Iterator for read-write iteration of this HashSet. */
	typedef HashSetIterator<KEY, TRAITS> Iterator;

	/* range-based for loop */
	typedef Iterator iterator;

	/** Comparator that is used to determine if key A is equal to key B. */
	typedef HashTableComparator<KEY, TRAITS::kCheckHashBeforeEquals> Comparator;

	/** Size type used for storing capacities and counts of the hash table. */
	typedef UInt32 SizeType;

	/** Traits used by this HashSet<> */
	typedef TRAITS Traits;

	HashSet()
		: m_pKeys(nullptr)
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
	 * Construct a HashSet with an initial capacity - avoids resizing while
	 * inserting elements later if a reasonable estimate of the final capacity is available
	 * initially.
	 */
	explicit HashSet(SizeType initialCapacity)
		: m_pKeys(nullptr)
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

	HashSet(const HashSet& b)
		: m_pKeys(nullptr)
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

		KEY const kNullKey = TRAITS::GetNullKey();
		for (UInt32 i = 0u; i < m_uCapacityExcludingNull; ++i)
		{
			if (kNullKey != b.m_pKeys[i])
			{
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
		}
	}

	template <typename SEQ_ITER>
	HashSet(SEQ_ITER iBegin, SEQ_ITER iEnd, SizeType initialCapacity)
		: m_pKeys(nullptr)
		, m_uCapacityExcludingNull(0)
		, m_bHasNullStorage(false)
		, m_uCountExcludingNull(0)
		, m_bHasNull(false)
#if !SEOUL_ASSERTIONS_DISABLED
		, m_IteratorReferenceCount(0)
#endif
	{
		Grow(initialCapacity);

		for (SEQ_ITER iData = iBegin; iData != iEnd; ++iData)
		{
			Insert(*iData);
		}
	}

	HashSet& operator=(const HashSet& b)
	{
		if (this == &b)
		{
			return *this;
		}

		// Non-ship sanity check - it is invalid to call mutations on a HashSet
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

		KEY const kNullKey = TRAITS::GetNullKey();
		for (UInt32 i = 0u; i < m_uCapacityExcludingNull; ++i)
		{
			if (kNullKey != b.m_pKeys[i])
			{
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
		}

		return *this;
	}

	~HashSet()
	{
		// Non-ship sanity check - it is invalid to call mutations on a HashSet
		// when iterators to the table are defined.
		SEOUL_ASSERT(0 == m_IteratorReferenceCount);

		Destroy();
	}

	/**
	 * @return Start of this HashSet for iteration.
	 */
	ConstIterator Begin() const
	{
		return ConstIterator(GetIteratorReferenceCountPtr(), m_pKeys, m_uCapacityExcludingNull);
	}

	/* range-based for loop */
	const_iterator begin() const { return Begin(); }

	/**
	 * @return Start of this HashSet for read-write iteration.
	 */
	Iterator Begin()
	{
		return Iterator(GetIteratorReferenceCountPtr(), m_pKeys, m_uCapacityExcludingNull);
	}

	/* range-based for loop */
	iterator begin() { return Begin(); }

	/**
	 * @return End of this HashSet for iteration.
	 */
	ConstIterator End() const
	{
		// Deliberate - to handle the special null case, the End() iterator is 1 passed the
		// capacity if null is present. The iterator will "do the right thing" in this case:
		// - it will stop at the null entry, since m_uCapacityExcludingNull will stop iteration,
		//   whether the last entry is a null key or not.
		return ConstIterator(GetIteratorReferenceCountPtr(), m_pKeys, m_uCapacityExcludingNull, m_uCapacityExcludingNull + m_bHasNull);
	}

	/* range-based for loop */
	const_iterator end() const { return End(); }

	/**
	 * @return End of this HashSet for read-write iteration.
	 */
	Iterator End()
	{
		// Deliberate - to handle the special null case, the End() iterator is 1 passed the
		// capacity if null is present. The iterator will "do the right thing" in this case:
		// - it will stop at the null entry, since m_uCapacityExcludingNull will stop iteration,
		//   whether the last entry is a null key or not.
		return Iterator(GetIteratorReferenceCountPtr(), m_pKeys, m_uCapacityExcludingNull, m_uCapacityExcludingNull + m_bHasNull);
	}

	/* range-based for loop */
	iterator end() { return End(); }

	/**
	 * Destroy all entries in the HashSet and set its size by to 0 - does
	 * not reduce the hash set capacity.
	 */
	void Clear()
	{
		// Non-ship sanity check - it is invalid to call mutations on a HashSet
		// when iterators to the table are defined.
		SEOUL_ASSERT(0 == m_IteratorReferenceCount);

		KEY const kNullKey = TRAITS::GetNullKey();
		for (ptrdiff_t i = (ptrdiff_t)m_uCapacityExcludingNull - 1; i >= 0; --i)
		{
			if (kNullKey != m_pKeys[i])
			{
				m_pKeys[i] = kNullKey;
			}
		}

		if (m_bHasNull)
		{
			m_pKeys[m_uCapacityExcludingNull] = kNullKey;
			m_bHasNull = false;
		}

		m_uCountExcludingNull = 0;
	}

	/**
	 * Destroy all entries in the HashSet and set its size by to 0, also
	 * deallocating any heap memory used by the HashSet.
	 */
	void Destroy()
	{
		// Non-ship sanity check - it is invalid to call mutations on a HashSet
		// when iterators to the table are defined.
		SEOUL_ASSERT(0 == m_IteratorReferenceCount);

		for (ptrdiff_t i = (ptrdiff_t)m_uCapacityExcludingNull - 1; i >= 0; --i)
		{
			m_pKeys[i].~KEY();
		}

		if (m_bHasNull)
		{
			m_pKeys[m_uCapacityExcludingNull].~KEY();
			m_bHasNull = false;
		}

		MemoryManager::Deallocate(m_pKeys);
		m_pKeys = nullptr;

		m_bHasNullStorage = false;
		m_uCapacityExcludingNull = 0;
		m_bHasNull = false;
		m_uCountExcludingNull = 0;
	}

	/**
	 * Remove an element with key key from this HashSet.
	 *
	 * @return True if an element was removed, false otherwise.
	 *
	 * \pre key must be != to kNullKey.
	 */
	Bool Erase(const KEY& key)
	{
		// Non-ship sanity check - it is invalid to call mutations on a HashSet
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
						if (Insert(innerEntryKey).Second)
						{
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
	 * @return A pointer to the existing key equal to key,
	 * or nullptr if no such entry exists in this HashSet.
	 */
	KEY* Find(const KEY& key)
	{
		// Special case - nothing to do if no entries in the set.
		if (0 == (m_uCountExcludingNull + m_bHasNull))
		{
			return nullptr;
		}

		KEY const kNullKey = TRAITS::GetNullKey();

		// Special case handling of kNullKey.
		if (key == kNullKey)
		{
			return m_bHasNull ? (m_pKeys + m_uCapacityExcludingNull) : nullptr;
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
				return (m_pKeys + uIndex);
			}
			// Otherwise, if we hit a nullptr key, the entry is not in the table.
			else if (entryKey == kNullKey)
			{
				return nullptr;
			}

			++uIndex;
		}
	}

	/**
	 * @return The existing key equal to key, or nullptr
	 * if no entry exists in this HashSet.
	 */
	KEY const* Find(const KEY& key) const
	{
		return const_cast<HashSet*>(this)->Find(key);
	}

	/**
	 * @return The number of entires in this HashSet.
	 */
	SizeType GetSize() const
	{
		return (m_uCountExcludingNull + m_bHasNull);
	}

	/**
	 * @return The total size of this HashSet's bucketing array.
	 */
	SizeType GetCapacity() const
	{
		return m_uCapacityExcludingNull;
	}

	/** @return The total memory footprint of this HashSet in bytes. */
	UInt32 GetMemoryUsageInBytes() const
	{
		return (UInt32)(sizeof(*this) + (m_uCapacityExcludingNull + m_bHasNull) * sizeof(KEY));
	}

	/**
	 * Increase the size of this HashSet to the next power of 2,
	 * greater than or equal to nNewCapacity.
	 */
	void Grow(SizeType nNewCapacity)
	{
		KEY const kNullKey = TRAITS::GetNullKey();
		nNewCapacity = GetNextPowerOf2(nNewCapacity);
		SizeType const nOldCapacity = m_uCapacityExcludingNull;

		if (nNewCapacity > nOldCapacity)
		{
			// Non-ship sanity check - it is invalid to call mutations on a HashSet
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

			// Keys need to be initialized to nullptr.
			for (SizeType i = 0; i < nNewCapacity; ++i)
			{
				new (pKeys + i) KEY(kNullKey);
			}

			// Initialize the key for the special nullptr member if present.
			if (m_bHasNull)
			{
				new (pKeys + nNewCapacity) KEY(kNullKey);
			}

			// Switch the old and new keys in preparation
			// for reinserting.
			Seoul::Swap(pKeys, m_pKeys);

			// Update nullptr presence - m_bHasNull is always set to
			// false. m_bHasNullstorage is only true if m_bHasNull
			// was true.
			Bool const bHasNull = m_bHasNull;
			m_bHasNullStorage = m_bHasNull;
			m_bHasNull = false;

			// "Empty" the count and increase the capacity.
			m_uCountExcludingNull = 0;
			m_uCapacityExcludingNull = nNewCapacity;

			// Now insert all the old keys.
			for (SizeType i = 0u; i < nOldCapacity; ++i)
			{
				if (kNullKey != pKeys[i])
				{
					SEOUL_VERIFY(Insert(pKeys[i]).Second);
				}
			}

			// Insert the special nullptr key if defined.
			if (bHasNull)
			{
				SEOUL_ASSERT(pKeys[nOldCapacity] == kNullKey);
				SEOUL_ASSERT(m_bHasNullStorage);
				SEOUL_VERIFY(Insert(pKeys[nOldCapacity]).Second);
				SEOUL_ASSERT(m_bHasNull);
			}

			// Cleanup the old nullptr key if present.
			if (bHasNull)
			{
				pKeys[nOldCapacity].~KEY();
			}

			// Now cleanup the old normal data.
			for (ptrdiff_t i = (ptrdiff_t)nOldCapacity - 1; i >= 0; --i)
			{
				pKeys[i].~KEY();
			}

			MemoryManager::Deallocate(pKeys);
		}
	}

	/**
	 * @return True if key is in this HashSet, false otherwise.
	 */
	Bool HasKey(const KEY& key) const
	{
		// Special case - nothing to do if no entries in the set.
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
	 * Inserts a key into this HashSet.
	 *
	 * @return A pair object whose first element is
	 * an iterator pointing either to the newly inserted
	 * element or to the element whose key is equivalent,
	 * and a bool value indicating whether the element
	 * was successfully inserted or not.
	 */
	Pair<Iterator, Bool> Insert(const KEY& key)
	{
		typedef Pair<Iterator, Bool> ReturnType;

		// Non-ship sanity check - it is invalid to call mutations on a HashSet
		// when iterators to the table are defined.
		SEOUL_ASSERT(0 == m_IteratorReferenceCount);

		// If increasing the number of elements by 1 will place us at or above
		// the load factor for KEY, grow the array capacity to the next power of 2
		// beyond the current capacity.
		if ((SizeType)(m_uCountExcludingNull + 1) >= (SizeType)(m_uCapacityExcludingNull * TRAITS::GetLoadFactor()))
		{
			// Always grow so there will be at least one nullptr entry in the set - all the
			// querying logic depends on this, in exchange, the logic requires fewer branches.
			Grow(GetNextPowerOf2(m_uCapacityExcludingNull + 2));
		}

		KEY const kNullKey = TRAITS::GetNullKey();

		// Special case handling of kNullKey.
		if (key == kNullKey)
		{
			if (!m_bHasNull)
			{
				// Make sure we have the extra slot for the special
				// case nullptr key-value.
				CheckAndGrowForNullKey();

				// Key is always assigned, since it is always initialized.
				m_pKeys[m_uCapacityExcludingNull] = key;

				// Always have a nullptr key-value pair at this point.
				m_bHasNull = true;

				return ReturnType(Iterator(GetIteratorReferenceCountPtr(), m_pKeys, m_uCapacityExcludingNull, m_uCapacityExcludingNull), true);
			}
			else
			{
				return ReturnType(Iterator(GetIteratorReferenceCountPtr(), m_pKeys, m_uCapacityExcludingNull, m_uCapacityExcludingNull), false);
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

			// If the existing entry is not in its home, replace it with the new key,
			// then reinsert the existing entry.
			if (uEntryHomeIndex != uIndex)
			{
				m_pKeys[uIndex] = key;

				// Reinserting the existing entry must always succeed, and the return iterator
				// points at the just inserted values.
				SEOUL_VERIFY(InternalInsert(entryKey, uEntryHash, uEntryHomeIndex).Second);
				return ReturnType(Iterator(GetIteratorReferenceCountPtr(), m_pKeys, m_uCapacityExcludingNull, uIndex), true);
			}
			// The home index is occupied by an entry that is already in its home, so we need
			// to continue trying to insert the new key.
			else
			{
				return InternalInsert(key, uHash, uIndex);
			}
		}
		// The entry is unoccupied, so we can just insert the new key.
		else
		{
			m_pKeys[uIndex] = key;
			++m_uCountExcludingNull;

			return ReturnType(Iterator(GetIteratorReferenceCountPtr(), m_pKeys, m_uCapacityExcludingNull, uIndex), true);
		}
	}

	/**
	 * @return True if this HashSet has no entries, false otherwise.
	 */
	Bool IsEmpty() const
	{
		return (0 == (m_uCountExcludingNull + m_bHasNull));
	}

	/**
	 * Swap the state of this HashSet with b.
	 */
	void Swap(HashSet& b)
	{
		// Non-ship sanity check - it is invalid to call mutations on a HashSet
		// when iterators to the table are defined.
		SEOUL_ASSERT(0 == m_IteratorReferenceCount);

		Seoul::Swap(m_pKeys, b.m_pKeys);
		Seoul::Swap(m_nCapacityExcludingNullAndHasNullStorage, b.m_nCapacityExcludingNullAndHasNullStorage);
		Seoul::Swap(m_nCountExcludingNullAndHasNull, b.m_nCountExcludingNullAndHasNull);
	}

	/*
	 * Returns true if all the elements in this set are the same
	 * as the elements in the other set.
	 */
	template <
		typename OTHER_KEY,
		int OTHER_MEMORY_BUDGETS,
		typename OTHER_TRAITS >
	Bool operator==(const HashSet<OTHER_KEY, OTHER_MEMORY_BUDGETS, OTHER_TRAITS>& setRight) const
	{
		if (setRight.GetSize() != GetSize())
		{
			return false;
		}

		// We know the sets are the same size;
		// Make sure all the elements in this set are
		// in the other set.
		return Contains(setRight);
	}

	/*
	* Returns true if all the elements in this set are
	* *not* the same as the elements in the other set.
	*/
	template <
		typename OTHER_KEY,
		int OTHER_MEMORY_BUDGETS,
		typename OTHER_TRAITS >
	Bool operator!=(const HashSet<OTHER_KEY, OTHER_MEMORY_BUDGETS, OTHER_TRAITS>& setRight) const
	{
		return !(*this == setRight);
	}

	/*
	* Returns true if all the elements in the provided set are
	* in this set.
	*/
	template <
		typename OTHER_KEY,
		int OTHER_MEMORY_BUDGETS,
		typename OTHER_TRAITS >
	bool Contains(const HashSet<OTHER_KEY, OTHER_MEMORY_BUDGETS, OTHER_TRAITS>& setRight) const
	{
		for (auto const &oRight : setRight)
		{
			if (!HasKey(oRight))
			{
				return false;
			}
		}
		return true;
	}

	/*
	* Returns true if *none* of the elements in the provided set are
	* in this set.
	*/
	template <
		typename OTHER_KEY,
		int OTHER_MEMORY_BUDGETS,
		typename OTHER_TRAITS >
	bool Disjoint(const HashSet<OTHER_KEY, OTHER_MEMORY_BUDGETS, OTHER_TRAITS>& setRight) const
	{
		for (auto const &oRight : setRight)
		{
			if (HasKey(oRight))
			{
				return false;
			}
		}
		return true;
	}

private:
	KEY* m_pKeys;
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
	 * Special handling for the special-case nullptr key. A key
	 * which legitimately uses the nullptr key is stored in a "hidden"
	 * extra slot at the end of the key memory buffers. We
	 * don't add this hiddens lot until it is needed.
	 */
	void CheckAndGrowForNullKey()
	{
		// Nothing to do if we already have nullptr storage.
		if (!m_bHasNullStorage)
		{
			// Non-ship sanity check - it is invalid to call mutations on a HashSet
			// when iterators to the table are defined.
			SEOUL_ASSERT(0 == m_IteratorReferenceCount);

			KEY const kNullKey = TRAITS::GetNullKey();

			// Allocate new buffers, m_uCapacityExcludingNull + 1
			// to add room for the nullptr key.
			KEY* pKeys = (KEY*)MemoryManager::AllocateAligned(
				(m_uCapacityExcludingNull + 1) * sizeof(KEY),
				(MemoryBudgets::Type)MEMORY_BUDGETS,
				SEOUL_ALIGN_OF(KEY));

			// Initialize all keys - this is simpler
			// than a standard resize, since we're just copying through
			// all the normal elements.
			for (SizeType i = 0; i < m_uCapacityExcludingNull; ++i)
			{
				new (pKeys + i) KEY(m_pKeys[i]);
			}

			// Initialize the nullptr key slot.
			new (pKeys + m_uCapacityExcludingNull) KEY(kNullKey);

			// Swap buffers and then cleanup the buffers we're
			// about to deallocate.
			Seoul::Swap(pKeys, m_pKeys);

			// Now cleanup the old data.
			for (ptrdiff_t i = (ptrdiff_t)m_uCapacityExcludingNull - 1; i >= 0; --i)
			{
				pKeys[i].~KEY();
			}

			MemoryManager::Deallocate(pKeys);

			// Done - this HashSet now has storage for a nullptr key.
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
		UInt32 const uHash,
		UInt32 uIndex)
	{
		typedef Pair<Iterator, Bool> ReturnType;

		KEY const kNullKey = TRAITS::GetNullKey();

		while (true)
		{
			// If an entry exists that has the same key as the entry being inserted.
			KEY const entryKey(m_pKeys[uIndex]);
			if (Comparator::Equals(uHash, key, entryKey))
			{
				// Return the existing entry and false to indicate it was already in the set
				return ReturnType(Iterator(GetIteratorReferenceCountPtr(), m_pKeys, m_uCapacityExcludingNull, uIndex), false);
			}
			// Otherwise, if we've hit a nullptr key, we're done. Insert the key and return success.
			else if (entryKey == kNullKey)
			{
				m_pKeys[uIndex] = key;
				++m_uCountExcludingNull;

				return ReturnType(Iterator(GetIteratorReferenceCountPtr(), m_pKeys, m_uCapacityExcludingNull, uIndex), true);
			}

			++uIndex;
			uIndex &= (m_uCapacityExcludingNull - 1);
		}
	}
}; // class HashSet

/**
 * Equivalent to std::swap(). Override specifically for HashSet<>.
 */
template <typename KEY, int MEMORY_BUDGETS, typename TRAITS>
inline void Swap(HashSet<KEY, MEMORY_BUDGETS, TRAITS>& a, HashSet<KEY, MEMORY_BUDGETS, TRAITS>& b)
{
	a.Swap(b);
}

/** Utility, gather all HashTable keys into a Vector<>. */
template <typename FROM, typename TO>
static inline void GetHashSetKeys(const FROM& from, TO& rOut)
{
	TO to;
	to.Reserve(from.GetSize());
	{
		typename FROM::ConstIterator const iBegin = from.Begin();
		typename FROM::ConstIterator const iEnd = from.End();
		for (typename FROM::ConstIterator i = iBegin; iEnd != i; ++i)
		{
			to.PushBack(*i);
		}
	}

	rOut.Swap(to);
}

} // namespace Seoul

#endif // include guard
