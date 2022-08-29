/**
 * \file MemoryManager.h
 * \brief MemoryManager is the global memory handler for SeoulEngine. It
 * overrides C++ new/delete operators and provides low level allocation
 * that should be used in place of malloc() and free().
 *
 * On Windows, MemoryManager uses jemalloc to handle allocations,
 * but adds a memory leak detection and tracking layer on top.
 *
 * On Android and iOS, MemoryManager uses just the system allocator. On iOS,
 * this is due to a widespread problem with mismatched new/free vs. malloc/delete
 * calls in the system API (MemoryManager overrides the new/free calls but cannot
 * override the malloc/delete calls).
 *
 * On Android, MemoryManager uses the system allocator due to a huge amount of PSS
 * overhead when using jemalloc (roughly about 90 MB on a 32-bit Android OS) and because
 * jemalloc itself became the Android system allocator starting in Android 21 (conditionally,
 * not all vendors switched to jemalloc until Android 24).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include "SeoulTypes.h"
#include "StandardPlatformIncludes.h"

#include <new>

namespace Seoul
{

// Forward declarations.
class SyncFile;

// Also, AddressSanitizer conflicts with global override of operator new
// and delete, so we disable them when it is enabled.
#if SEOUL_ADDRESS_SANITIZER
#	define SEOUL_OVERRIDE_GLOBAL_NEW 0
#else
#	define SEOUL_OVERRIDE_GLOBAL_NEW 1
#endif

#if SEOUL_OVERRIDE_GLOBAL_NEW
#if SEOUL_DEBUG
#	define SEOUL_NEW(x) new(__FILE__,__LINE__,x)
#	define SEOUL_DELETE delete
#else
#	define SEOUL_NEW(x) new(x)
#	define SEOUL_DELETE delete
#endif

#else

#if SEOUL_DEBUG
#	if SEOUL_PLATFORM_IOS
#		define SEOUL_NEW(x) new
#	else
#		define SEOUL_NEW(x) new(__FILE__,__LINE__)
#	endif
#	define SEOUL_DELETE delete
#else
#	define SEOUL_NEW(x) new
#	define SEOUL_DELETE delete
#endif

#endif // /SEOUL_OVERRIDE_GLOBAL_NEW

// Forward declarations
struct PerAllocationData;
class String;

class MemoryManager
{
public:
	/**
	 * The minimum alignment that will ever be used.
	 */
	static const size_t kMinimumAlignment = 4u;

	// Variations of Allocate(), Reallocate() and Deallocate() for
	// fulfilling alignment requests and adding debug information to allocations.

	static void* Allocate(size_t zRequestedMemory, MemoryBudgets::Type eType)
	{
		return InternalAllocate(zRequestedMemory, 0u, eType, 0u, nullptr);
	}

	static void* AllocateAligned(size_t zRequestedMemory, MemoryBudgets::Type eType, size_t zAlignment)
	{
		return InternalAllocate(zRequestedMemory, zAlignment, eType, 0u, nullptr);
	}

	static void* AllocateAligned(size_t zRequestedMemory, MemoryBudgets::Type eType, size_t zAlignment, const Byte* sCallerFileName, UInt32 iCallerLine)
	{
		return InternalAllocate(zRequestedMemory, zAlignment, eType, iCallerLine, sCallerFileName);
	}

	static void* Allocate(size_t zRequestedMemory, MemoryBudgets::Type eType, const Byte* sCallerFileName, UInt32 iCallerLine)
	{
		return InternalAllocate(zRequestedMemory, 0u, eType, iCallerLine, sCallerFileName);
	}

	static void* Reallocate(void* pAddressToReallocate, size_t zRequestedMemory, MemoryBudgets::Type eType)
	{
		return InternalReallocate(pAddressToReallocate, zRequestedMemory, 0u, eType, 0u, nullptr);
	}

	static void* ReallocateAligned(void* pAddressToReallocate, size_t zRequestedMemory, size_t zAlignment, MemoryBudgets::Type eType)
	{
		return InternalReallocate(pAddressToReallocate, zRequestedMemory, zAlignment, eType, 0u, nullptr);
	}

	static void* ReallocateAligned(void* pAddressToReallocate, size_t zRequestedMemory, size_t zAlignment, MemoryBudgets::Type eType, const Byte* sCallerFileName, UInt32 iCallerLine)
	{
		return InternalReallocate(pAddressToReallocate, zRequestedMemory, zAlignment, eType, iCallerLine, sCallerFileName);
	}

	static void* Reallocate(void* pAddressToReallocate, size_t zRequestedMemory, MemoryBudgets::Type eType, const Byte* sCallerFileName, UInt32 iCallerLine)
	{
		return InternalReallocate(pAddressToReallocate, zRequestedMemory, 0u, eType, iCallerLine, sCallerFileName);
	}

	static void Deallocate(void* pAddressToFree)
	{
		InternalDeallocate(pAddressToFree);
	}

	static size_t GetAllocationSizeInBytes(void* pAddress)
	{
		return InternalGetAllocationSizeInBytes(pAddress);
	}

#if SEOUL_ENABLE_MEMORY_TOOLING
	// Return true if verbose memory leak detection is currently enabled.
	static Bool GetVerboseMemoryLeakDetectionEnabled();

	// Runtime control of verbose memory leak detection. Useful
	// in tools and other scenarios where we want a developer
	// build (with logging, assertions, etc. enabled) but don't
	// want the overhead of verbose memory leak tracking.
	static void SetVerboseMemoryLeakDetectionEnabled(Bool bEnabled);

	// Current memory allocation count of memory of type eType.
	static Int32 GetAllocations(MemoryBudgets::Type eType);

	// Associate a new memory type at pAddress, if user code has determined
	// that meaning/type of that memory has changed.
	static void ChangeBudget(void *pAddress, MemoryBudgets::Type eNewType);

	// Current memory used by memory of type eType. This includes
	// the actual memory size, including oversizing/overhead of the
	// memory allocator.
	static Int32 GetUsageInBytes(MemoryBudgets::Type eType);

	// Current memory used in total.
	static Int64 GetTotalUsageInBytes();

	// Sets the filename that will be used to write memory leaks on program
	// exit. If not specified, a file called "memory_leaks.txt" will be
	// written to the current directory.
	static void SetMemoryLeaksFilename(const String& sFilename);

	// If called and memory allocator debug information is available,
	// prints memory detail to printfLike. If bRaw is true, logs every tracked
	// memory block for the given MemoryBudget. Otherwise, prints a summarize
	// view, keyed on an identifying stack frame stored with the blocks.
	//
	// NOTE: Pass MemoryBudgets::Unknown to include all memory budgets.
	typedef void(*PrintfLike)(void* userData, const Byte *sFormat, ...);
	static void PrintMemoryDetails(
		MemoryBudgets::Type eType,
		PrintfLike printfLike,
		void* userData,
		Bool bRaw = false);
#endif // /SEOUL_ENABLE_MEMORY_TOOLING

private:
	static void* InternalAllocate(
		size_t zSizeInBytes,
		size_t zAlignment,
		MemoryBudgets::Type eType,
		UInt32 uLineNumber,
		Byte const* psCallFilename);
	static void* InternalReallocate(
		void* pAddressToReallocate,
		size_t zSizeInBytes,
		size_t zAlignment,
		MemoryBudgets::Type eType,
		UInt32 uLineNumber,
		Byte const* psCallFilename);
	static void InternalDeallocate(
		void* pAddressToDeallocate);
	static size_t InternalGetAllocationSizeInBytes(
		void* pAllocatedAddress);

	// Not constructable, purely static class.
	MemoryManager();
	~MemoryManager();

	SEOUL_DISABLE_COPY(MemoryManager);
}; // MemoryManager class

} // namespace Seoul

#if SEOUL_OVERRIDE_GLOBAL_NEW

// Seoul engine extension to the standard new and delete operators
//
// NOTE: Many of the delete operators here are superfluous in function
// but are required by the compiler - it won't generate operator delete() calls
// in some cases unless an operator delete() is defined with a signature
// that exactly matches the signature of the operator new() (except
// that the size_t memory size argument is replaced with a void* memory
// pointer argument).
void* operator new (
	size_t zRequestedMemory,
	Seoul::MemoryBudgets::Type eType) SEOUL_THROWS(std::bad_alloc);

void* operator new[](
	size_t zRequestedMemory,
	Seoul::MemoryBudgets::Type eType) SEOUL_THROWS(std::bad_alloc);

void* operator new (
	size_t zRequestedMemory,
	char const* sCallerFilename,
	int iCallerLine) SEOUL_THROWS(std::bad_alloc);

void* operator new (
	size_t zRequestedMemory,
	char const* sCallerFilename,
	int iCallerLine,
	Seoul::MemoryBudgets::Type eType) SEOUL_THROWS(std::bad_alloc);

void* operator new[](
	size_t zRequestedMemory,
	char const* sCallerFilename,
	int iCallerLine) SEOUL_THROWS(std::bad_alloc);

void* operator new[](
	size_t zRequestedMemory,
	char const* sCallerFilename,
	int iCallerLine,
	Seoul::MemoryBudgets::Type eType) SEOUL_THROWS(std::bad_alloc);

void operator delete (
	void* pAddressToDeallocate,
	Seoul::MemoryBudgets::Type eType) SEOUL_THROW0();

void operator delete[](
	void* pAddressToDeallocate,
	Seoul::MemoryBudgets::Type eType) SEOUL_THROWS(std::bad_alloc);

void operator delete (
	void* pAddressToDeallocate,
	char const* sCallerFilename,
	int iCallerLine) SEOUL_THROWS(std::bad_alloc);

void operator delete (
	void* pAddressToDeallocate,
	char const* sCallerFilename,
	int iCallerLine,
	Seoul::MemoryBudgets::Type eType) SEOUL_THROWS(std::bad_alloc);

void operator delete[](
	void* pAddressToDeallocate,
	char const* sCallerFilename,
	int iCallerLine) SEOUL_THROWS(std::bad_alloc);

void operator delete[](
	void* pAddressToDeallocate,
	char const* sCallerFilename,
	int iCallerLine,
	Seoul::MemoryBudgets::Type eType) SEOUL_THROWS(std::bad_alloc);

// Standard new and delete operators
void* operator new(
	size_t zRequestedMemory); SEOUL_THROWS(std::bad_alloc);

void* operator new(
	size_t zRequestedMemory,
	const std::nothrow_t&) SEOUL_THROW0();

void* operator new(
	size_t zRequestedMemory,
	size_t zRequestedAlignment) SEOUL_THROWS(std::bad_alloc);

void* operator new(
	size_t zRequestedMemory,
	size_t zRequestedAlignment,
	const std::nothrow_t&) SEOUL_THROW0();

void* operator new[](
	size_t zRequestedMemory); SEOUL_THROWS(std::bad_alloc);

void* operator new[](
	size_t zRequestedMemory,
	const std::nothrow_t&) SEOUL_THROW0();

void* operator new[](
	size_t zRequestedMemory,
	size_t zRequestedAlignment) SEOUL_THROWS(std::bad_alloc);

void* operator new[](
	size_t zRequestedMemory,
	size_t zRequestedAlignment,
	const std::nothrow_t&) SEOUL_THROW0();

void operator delete (void* pAddressToDeallocate) SEOUL_THROW0();
void operator delete (void* pAddressToDeallocate, size_t zRequestedAlignment) SEOUL_THROW0();
void operator delete (void* pAddressToDeallocate, const std::nothrow_t&) SEOUL_THROW0();
void operator delete (void* pAddressToDeallocate, size_t zRequestedAlignment, const std::nothrow_t&) SEOUL_THROW0();

void operator delete[] (void* pAddressToDeallocate) SEOUL_THROW0();
void operator delete[] (void* pAddressToDeallocate, size_t zRequestedAlignment) SEOUL_THROW0();
void operator delete[] (void* pAddressToDeallocate, const std::nothrow_t&) SEOUL_THROW0();
void operator delete[] (void* pAddressToDeallocate, size_t zRequestedAlignment, const std::nothrow_t&) SEOUL_THROW0();

#endif // /SEOUL_OVERRIDE_GLOBAL_NEW

#endif // include guard
