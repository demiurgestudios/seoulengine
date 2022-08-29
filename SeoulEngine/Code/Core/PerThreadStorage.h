/**
 * \file PreThreadStorage.h
 * \brief Allows a void* size block of data to be stored such
 * that the value is unique per-thread.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef PER_THREAD_STORAGE_H
#define PER_THREAD_STORAGE_H

#include "Prereqs.h"

namespace Seoul
{

/**
 * PerThreadStorage allows a void*
 * size block of data to be stored such that the value
 * is unique per-thread.
 *
 * If SetPerThreadStorage() is called by thread A, setting the value to
 * non-nullptr data, and GetPerThreadStorage() is called by thread B, thread B
 * will get a value of nullptr (the default), while thread A
 * will get the value that was set to the storage in SetPerThreadStorage().
 *
 * PerThreadStorage should be used whenever an object must be unique
 * per-thread, for example, a memory allocator or a queue used
 * in a concurrent job manager.
 *
 * \warning Every platform has a hard limit to the number of per-thread
 * storage slots that can be allocated (typically 64). This limit
 * is only enforced with an SEOUL_ASSERT() in various PerThreadStorage methods.
 * It is up to an application to avoid using too many slots to
 * stay under the per-platform limit.
 *
 * \warning PerThreadStorage does not "own" whatever is in its storage slot.
 * As a result, if the storage slot contains a pointer to heap allocated
 * memory, the code that uses PerThreadStorage *must* cleanup that memory
 * outside of PerThreadStorage, and it *must* do so for every version
 * that was allocated for every thread. This will typically require an
 * additional layer on top of PerThreadStorage to manage the lifespan
 * of these per-thread objects. See also: HeapAllocatedPerThreadStorage
 */
class PerThreadStorage SEOUL_SEALED
{
public:
	PerThreadStorage();
	~PerThreadStorage();

	void* GetPerThreadStorage() const;
	void SetPerThreadStorage(void* pData);

	/**
	 * @return True if the per-thread storage for this PerThreadStorage
	 * object has been assigned to a non-nullptr value for the current thread,
	 * false otherwise.
	 */
	Bool IsValid() const
	{
		return (nullptr != GetPerThreadStorage());
	}

private:
	PerThreadStorageIndexType m_ThreadLocalStorageIndex;

private:
	SEOUL_DISABLE_COPY(PerThreadStorage);
}; // class PerThreadStorage

} // namespace Seoul

#endif // include guard
