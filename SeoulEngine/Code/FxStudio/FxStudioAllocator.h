/**
 * \file FxStudioAllocator.h
 * \brief Hook FxStudio memory allocations into SeoulEngine's MemoryManager.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FX_STUDIO_ALLOCATOR_H
#define FX_STUDIO_ALLOCATOR_H

#include "Prereqs.h"

#if SEOUL_WITH_FX_STUDIO
#include "FxStudioRT.h"

namespace Seoul::FxStudio
{

class Allocator SEOUL_SEALED : public ::FxStudio::Allocator
{
public:
	Allocator();
	~Allocator();

	// numBytes will always be greater than zero.
	virtual void* AllocateBytes(std::size_t nNumBytes, MemoryCategory eCategory) SEOUL_OVERRIDE;

	// pBytes will never be zero.
	virtual void  ReleaseBytes(void* pBytes, MemoryCategory eCategory) SEOUL_OVERRIDE;

private:
	// This class cannot be copied.
	SEOUL_DISABLE_COPY(Allocator);

	// Data...
	::FxStudio::Allocator& m_OldAllocator;
}; // class FxStudio::Allocator

} // namespace Seoul::FxStudio

#endif // /#if SEOUL_WITH_FX_STUDIO

#endif // include guard
