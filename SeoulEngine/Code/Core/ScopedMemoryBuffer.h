/**
 * \file ScopedMemoryBuffer.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCOPED_MEMORY_BUFFER_H
#define SCOPED_MEMORY_BUFFER_H

#include "Prereqs.h"

namespace Seoul
{

/** Destroys a low-level allocated pointer on destruction. */
class ScopedMemoryBuffer SEOUL_SEALED
{
public:
	ScopedMemoryBuffer()
		: m_p(nullptr)
		, m_u(0u)
	{
	}

	ScopedMemoryBuffer(ScopedMemoryBuffer&& rp)
		: m_p(nullptr)
		, m_u(0u)
	{
		Swap(rp);
	}

	explicit ScopedMemoryBuffer(void* p, UInt32 u)
		: m_p(p)
		, m_u(u)
	{
	}

	~ScopedMemoryBuffer()
	{
		MemoryManager::Deallocate(m_p);
		m_p = nullptr;
		m_u = 0u;
	}

	/** Assigns a new raw pointer to this ScopedMemoryBuffer. */
	void Reset(void* p = nullptr, UInt32 u = 0u)
	{
		auto pPrev = m_p;
		m_u = 0u;
		m_p = nullptr;

		// Destroy existing.
		MemoryManager::Deallocate(pPrev);

		m_p = p;
		m_u = u;
	}

	/** Accessor for the raw pointer stored in this ScopedMemoryBuffer. */
	void* Get() const
	{
		return m_p;
	}

	/** Accessor that returns the buffer size. */
	UInt32 GetSize() const
	{
		return m_u;
	}

	/** @return true if this ScopedMemoryBuffer's pointer is non-null, false otherwise. */
	Bool IsValid() const
	{
		return (m_p != nullptr);
	}

	/** Cheap swap between this ScopedMemoryBuffer and a ScopedMemoryBuffer b. */
	void Swap(ScopedMemoryBuffer& b)
	{
		Seoul::Swap(m_p, b.m_p);
		Seoul::Swap(m_u, b.m_u);
	}

	/** Cheap swap between this ScopedMemoryBuffer and a ScopedMemoryBuffer b. */
	void Swap(void*& rp, UInt32& ru)
	{
		Seoul::Swap(m_p, rp);
		Seoul::Swap(m_u, ru);
	}

private:
	void* m_p;
	UInt32 m_u;

	SEOUL_DISABLE_COPY(ScopedMemoryBuffer);
}; // class ScopedMemoryBuffer

inline Bool operator==(const ScopedMemoryBuffer& a, const ScopedMemoryBuffer& b)
{
	return (a.Get() == b.Get());
}

inline Bool operator==(const ScopedMemoryBuffer& a, void* b)
{
	return (a.Get() == b);
}

inline Bool operator==(void* a, const ScopedMemoryBuffer& b)
{
	return (a == b.Get());
}

inline Bool operator!=(const ScopedMemoryBuffer& a, const ScopedMemoryBuffer& b)
{
	return (a.Get() != b.Get());
}

} // namespace Seoul

#endif // include guard
