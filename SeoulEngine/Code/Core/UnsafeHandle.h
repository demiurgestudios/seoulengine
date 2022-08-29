/**
 * \file UnsafeHandle.h
 * \brief Opaque type for wrapping void* and size_t raw values.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UNSAFE_HANDLE_H
#define UNSAFE_HANDLE_H

#include "Prereqs.h"

namespace Seoul
{

/**
 * Wrapper for various void* and std::size_t handles
 * (OpenGL, Win32 handles, DirectX objects, etc.)
 *
 * Unsafe. No type checking is done, use with purpose and with care.
 */
struct UnsafeHandle SEOUL_SEALED
{
	UnsafeHandle()
		: P(nullptr)
	{
	}

	UnsafeHandle(size_t i)
		: V(i)
	{
	}

	template <typename T>
	UnsafeHandle(T* p)
		: P((void*)p)
	{
	}

	template <typename T>
	UnsafeHandle(T const* p)
		: P((void*)(p))
	{
	}

	Bool operator==(const UnsafeHandle& h) const
	{
		return (P == h.P);
	}

	Bool operator!=(const UnsafeHandle& h) const
	{
		return (P != h.P);
	}

	void Reset()
	{
		P = nullptr;
	}

	Bool IsValid() const
	{
		return (P != nullptr);
	}

	union
	{
		void* P;
		size_t V;
	};
}; // struct UnsafeHandle

// Make sure sizes are what we expect,
// avoid unpleasantness.
SEOUL_STATIC_ASSERT(sizeof(size_t) == sizeof(void*));

template <typename T>
inline T StaticCast(UnsafeHandle v)
{
	return (T)(v.P);
}

template <>
inline std::size_t StaticCast<size_t>(UnsafeHandle v)
{
	return (v.V);
}

/**
 * Same functionality as the SafeDelete which takes
 * a generic argument, but handles user data that is wrapped
 * in a UnsafeHandle.
 */
template <typename T>
inline void SafeDelete(UnsafeHandle& rhHandle)
{
	T* p = StaticCast<T*>(rhHandle);
	rhHandle.Reset();

	if (p)
	{
		SEOUL_DELETE p;
	}
}

/**
 * Same functionality as the SafeAcquire which takes
 * a generic argument, but handles user data that is wrapped
 * in a UnsafeHandle.
 */
template <typename T>
UInt SafeAcquire(UnsafeHandle hHandle)
{
	T* p = StaticCast<T*>(hHandle);

	if (p)
	{
		return p->AddRef();
	}

	return 0u;
}

/**
 * Same functionality as the SafeRelease which takes
 * a generic argument, but handles user data that is wrapped
 * in a UnsafeHandle.
 */
template <typename T>
unsigned long SafeRelease(UnsafeHandle& rhHandle)
{
	T* p = StaticCast<T*>(rhHandle);
	rhHandle.Reset();

	if (p)
	{
		unsigned long ret = p->Release();
		return ret;
	}

	return 0u;
}

} // namespace Seoul

#endif // include guard
