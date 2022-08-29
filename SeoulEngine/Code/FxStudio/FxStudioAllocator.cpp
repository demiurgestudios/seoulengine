/**
 * \file FxStudioAllocator.cpp
 * \brief Hook FxStudio memory allocations into SeoulEngine's MemoryManager.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FxStudioAllocator.h"
#include "MemoryManager.h"

#if SEOUL_WITH_FX_STUDIO

namespace Seoul::FxStudio
{

/**
 * Constructor
 *
 * Registers this with FxStudio.  Keeps pointer to old allocator to register when this is destroyed
 */
Allocator::Allocator()
	: m_OldAllocator(::FxStudio::GetAllocator())
{
	::FxStudio::RegisterAllocator(*this);
}


/**
 * Destructor
 *
 * Re-registers the previous allocator used by FxStudio
 */
Allocator::~Allocator()
{
	::FxStudio::RegisterAllocator(m_OldAllocator);
}

/**
 * Memory allocator used by FxStudio
 *
 * Memory allocator used by FxStudio.
 *
 * @param[in] nNumBytes amount of memory to allocate
 *
 * @return A pointer to the allocated memory, of nullptr if memory could not be
 *         allocated
 */
void* Allocator::AllocateBytes(std::size_t nNumBytes, MemoryCategory eCategory)
{
	return MemoryManager::Allocate(nNumBytes, MemoryBudgets::Fx);
}


/**
 * Memory deallocator used by FxStudio
 *
 * Memory deallocator used by FxStudio.
 *
 * @param[in] pBytes Base address of memory to deallocate
 */
void Allocator::ReleaseBytes(void* pBytes, MemoryCategory eCategory)
{
	MemoryManager::Deallocate(pBytes);
}

} // namespace Seoul::FxStudio

#endif // /#if SEOUL_WITH_FX_STUDIO
