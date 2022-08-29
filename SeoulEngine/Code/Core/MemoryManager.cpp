/**
 * \file MemoryManager.cpp
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

#include "AtomicPointer.h"
#include "Logger.h"
#include "MemoryManager.h"
#include "Mutex.h"
#include "Prereqs.h"
#include "SeoulFile.h"
#include "SeoulMath.h"
#include "SeoulString.h"

#if SEOUL_PLATFORM_WINDOWS || SEOUL_PLATFORM_LINUX

#	define JEMALLOC_EXPORT
#	include <jemalloc/jemalloc.h>
#	include <jemalloc/internal/jemalloc_internal_defs.h>

	// Some sanity checks that we've got the jemalloc configuration correct.
	SEOUL_STATIC_ASSERT(sizeof(long) == (1 << LG_SIZEOF_LONG));
	SEOUL_STATIC_ASSERT(sizeof(void*) == (1 << LG_SIZEOF_PTR));

	/** 8 is the minimum guaranteed alignment across all platforms from jemalloc. */
	static const size_t kzMinimumMallocAlignment = 8u;

#	define SEOUL_SYS_ALIGNED_ALLOC(align, sz) seoul_je_aligned_alloc((align), (sz))
#	define SEOUL_SYS_FREE(p) seoul_je_free(p)
#	define SEOUL_SYS_MALLOC(sz) seoul_je_malloc(sz)
#	define SEOUL_SYS_REALLOC(p, sz) seoul_je_realloc((p), (sz))
#	define SEOUL_SYS_USABLE_SIZE(p) seoul_je_malloc_usable_size(p)

#elif SEOUL_PLATFORM_ANDROID
#	include <malloc.h>
/** 8 is the minimum guaranteed alignment on Android. */
static const size_t kzMinimumSystemAlignment = 8u;

/**
 * Handling in and around malloc_usable size - malloc_usable_size wasn't exported to libc.so
 * until API 17, so if we're compiling against a target less than that, we need to check for
 * the dynamic version and use it (for running against newer libc), otherwise we fallback
 * to our own internal version (that assumes the libc.so malloc functions are using dlmalloc).
 * See also: https://android.googlesource.com/platform/bionic/+/jb-mr2-release/libc/upstream-dlmalloc/malloc.c#1246
 *
 * This is a bit gnarly. The basic approach of checking for newer functions is safe and supported
 * (e.g. it is how cpufeatures.c, an Android library, checks for newer functionality). However,
 * using a hand written implementation in DlmallocAndroid16UsableSize() that assumes libc.so contains
 * dlmalloc is dangerous, since vendors are allowed to replace libc.so with whatever customizations
 * they want.
 *
 * That said, this applies only to API-16 devices, since API-17 and newer must have malloc_usable_size
 * exported in libc.so. As such, given that (as of this writing):
 * - the GetAllocationSizeInBytes() API is never called in ship (due to all allocations being <= 8
 *   alignment and due to all public usages of GetAllocationSizeInBytes() being debug/developer only).
 * - we have yet to see an exception to the dlmalloc assumption on our mobile device testing (dozens of Android
 *   device models)
 * - the alternative would be to remove the GetAllocationSizeInBytes() and ReallocateAligned() API from
 *   all platforms) or to prematurely deprecate API-16 support.
 *
 * We have decided to assume the risk and support as many API-16 devices as we can without fundamentally
 * changing our MemoryManager API.
 */
#if __ANDROID_API__ < 17
#include "AtomicPointer.h"
#include "Core.h"
#include "MemoryBarrier.h"
#include "MemoryManagerInternalAndroid16.h"
#include <android/log.h>
#include <dlfcn.h>
extern "C"
{
	typedef size_t(*malloc_usable_size_func)(const void* __ptr);
}

namespace Seoul
{

/** Directly to memalign in API < 17. */
#define android_memalign memalign

// Declaration.
static size_t initial_android_malloc_usable_size(const void* __ptr);

/** Cache a handle to libc.so if we opened it for malloc_usable_size. */
static AtomicPointer<void> s_pLibcSo;
static void CloseLibcSo()
{
	// Acquire.
	void* p = s_pLibcSo;
	// Check.
	while (nullptr != p)
	{
		// This is our method of taking ownership of the pointer - it will
		// still be p if we "win" a race with another shutdown thread. If not,
		// try again. If we get nullptr (check in the while loop header), it means
		// another thread has already closed libc.so for us.
		if (p == s_pLibcSo.CompareAndSet(nullptr, p))
		{
			dlclose(p);
			break;
		}

		// Reacquire for another try.
		p = s_pLibcSo;
	}
}

/** The pointer redirection hook - initially a lookup, resolved to the final version on first call. */
static volatile malloc_usable_size_func android_malloc_usable_size = initial_android_malloc_usable_size;

/**
 * Initial implementation loads libc.so, looks for dynamic malloc_usable_size, and
 * falls back to our own defined DlmallocAndroid16UsableSize() (which assumes
 * dlmalloc in Android API-16).
 *
 * NOTE: This startup is thread-safe, assuming the results of queries are always the same,
 * which they must be on a given device. Either the static DlmallocAndroid16UsableSize()
 * is used, or we use a dynamically bound function from libc.so, and the binding of that
 * function is made thread-safe using atomics.
 */
static size_t initial_android_malloc_usable_size(const void* __ptr)
{
	// Get libc.so.
	dlerror();
	auto lib = dlopen("libc.so", RTLD_NOW);
	SEOUL_ASSERT_NO_LOG(nullptr != lib);

	// Resolve the func.
	auto func = (malloc_usable_size_func)dlsym(lib, "malloc_usable_size");
	if (nullptr == func)
	{
		// Not found, use the static version.
		dlclose(lib);
		android_malloc_usable_size = DlmallocAndroid16UsableSize;

		// Reporting for execution on mobile device testing - need to be very
		// careful with the logging since we're in a memory allocation
		// function. NOTE: We really want to check g_bRunningAutomatedTests here,
		// but that can't be set early enough (memory management hooks are triggered
		// during global static initialization, typically).
#if SEOUL_AUTO_TESTS
		SeoulMemoryBarrier();
		__android_log_write(ANDROID_LOG_INFO, "Seoul", "initial_android_malloc_usable_size(): using static DlmallocAndroid16UsableSize.");
#endif

		return DlmallocAndroid16UsableSize(__ptr);
	}
	else
	{
		// Compute value to return first, prior to management of the .so.
		//
		// IMPORTANT: Must call func() directory here (vs. e.g. using the
		// value after set to android_malloc_usable_size) due to the possibility
		// of multiple threads.
		auto const zReturn = func(__ptr);

		// If we win, apply the dynamic version. Track the lib we opened with
		// a thread-safe pointer set, in case we've raced another thread.
		// If we "lose", close our instance (after calling the function).
		//
		// Return value of CompareAndSet() is the previous value - so, if nullptr is
		// returned, it means we "won", since we've successfully replaced nullptr
		// with lib local. So in this case, set the func and register an atexit()
		// handler to cleanup the lib.
		if (nullptr == s_pLibcSo.CompareAndSet(lib, nullptr))
		{
			// Apply and register atexit() to release the refcount on libc.so.
			android_malloc_usable_size = func;
			(void)atexit(CloseLibcSo);

			// Reporting for execution on mobile device testing - need to be very
			// careful with the logging since we're in a memory allocation
			// function. NOTE: We really want to check g_bRunningAutomatedTests here,
			// but that can't be set early enough (memory management hooks are triggered
			// during global static initialization, typically).
#if SEOUL_AUTO_TESTS
			SeoulMemoryBarrier();
			__android_log_write(ANDROID_LOG_INFO, "Seoul", "initial_android_malloc_usable_size(): using dynamic malloc_usable_size from libc.so.");
#endif
		}
		// Otherwise, we "lost", and we need to cleanup our refcount to the lib.
		else
		{
			dlclose(lib);
			lib = nullptr;
		}

		// In all cases, return the requested size.
		return zReturn;
	}
}

} // namespace Seoul
#else // !(__ANDROID_API__ < 17)

/**
 * Wrapper around memalign() - we prefer posix_memalign()
 * when available, because it guarantees that the returned
 * pointer can be freed. memalign() technically returns
 * a value that may not be freeable - however, as of
 * (at least) API-16, memalign() in Android bionic always returns
 * a freeable pointer. So, we can safely use memalign()
 * in API-16 and then use posix_memalign() going forward
 * to "future proof" the API.
 */
static inline void* android_memalign(size_t zAlignmentInBytes, size_t zSizeInBytes)
{
	void* pReturn = nullptr;
	SEOUL_VERIFY_NO_LOG(0 == posix_memalign(&pReturn, zAlignmentInBytes, zSizeInBytes));
	return pReturn;
}

/** Fortunately added in API-17 - just a macro to allow the API to be uniform. */
#define android_malloc_usable_size malloc_usable_size
#endif // /!(__ANDROID_API__ < 17)

#elif SEOUL_PLATFORM_IOS
#	include <malloc/malloc.h>
/** 16 is the minimum guaranteed alignment on iOS. */
static const size_t kzMinimumSystemAlignment = 16u;
#endif

#include <stdio.h>
#include <stdlib.h>

// Needed for the _SH_DENYWR constant on windows
#if SEOUL_PLATFORM_WINDOWS
#include <share.h>
#endif

namespace Seoul
{

#if SEOUL_ENABLE_MEMORY_TOOLING
/** Runtime variable to turn off the overhead of verbose memory leak detection. */
static Bool s_bEnableVerboseMemoryLeakDetection = false;
#endif // /#if SEOUL_ENABLE_MEMORY_TOOLING

namespace MemoryManagerDetail
{

	// Forward declarations.
	static void* AllocateAligned(size_t zSizeInBytes, size_t zAlignment);
	static size_t GetAllocationSizeInBytes(void* pAllocatedAddress);
	static void* ReallocateAligned(void* pAddressToReallocate, size_t zSizeInBytes, size_t zAlignment);
	static void Deallocate(void* pAddressToDeallocate);

	/**
	 * Platform dependent implementation for opening
	 * the stream that will be used to report memory leaks.
	 *
	 * @return True if successful, false otherwise.
	 *
	 * \warning Like all functions involved in memory leak detection,
	 * avoid any heap allocation using the normal paths (SEOUL_NEW or
	 * MemoryManager::Allocate*()). It's dicey to use heap allocation
	 * at the point in shutdown where memory leak detection occurs.
	 *
	 * \pre rpStream must be nullptr before calling this method.
	 */
	inline Bool OpenFileStream(Byte const* sFilename, FILE*& rpStream)
	{
		SEOUL_ASSERT_NO_LOG(nullptr == rpStream);

#if SEOUL_PLATFORM_WINDOWS
		// Allow shared reading but not shared writing
		rpStream = _fsopen(
			sFilename,
			"wb",
			_SH_DENYWR);
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
		rpStream = fopen(sFilename, "wb");
		// No file writing under native client, output to standard error.
#else
#error "Define for this platform."
#endif

		return (nullptr != rpStream);
	}

	/**
	 * Flush the file stream rpStream.
	 *
	 * Works with any file stream, even streams not
	 * opened with OpenFileStream.
	 */
	inline void FlushFileStream(FILE* pStream)
	{
		if (pStream)
		{
			fflush(pStream);
		}
	}

	/**
	 * Platform dependent implementation for closing
	 * the stream opened by OpenFileStream().
	 *
	 * \pre rpStream must have been opened by OpenFileStream
	 * or this method can have undefined results.
	 */
	inline void CloseFileStream(FILE*& rpStream)
	{
		if (rpStream)
		{
			FILE* pStream = rpStream;
			rpStream = nullptr;
			fclose(pStream);
		}
	}

	///////////////////////////////////////////////////////////////////////////
	// Linux, and PC implementations
	// of AllocateAligned(), GetAllocationSizeInBytes(),
	// ReallocateAligned(), and Deallocate().
	///////////////////////////////////////////////////////////////////////////
#if SEOUL_PLATFORM_WINDOWS || SEOUL_PLATFORM_LINUX
#if SEOUL_ENABLE_MEMORY_TOOLING
	/** Associate a user data pointer-sized value with a particular memory block. */
	static void* GetUserData(void* pAddress)
	{
		if (nullptr == pAddress)
		{
			return nullptr;
		}

		size_t const zAllocationSizeInBytes = SEOUL_SYS_USABLE_SIZE(pAddress);
		SEOUL_ASSERT_NO_LOG(zAllocationSizeInBytes >= sizeof(void*));
		SEOUL_ASSERT_NO_LOG(0 == (size_t)((Byte*)pAddress + zAllocationSizeInBytes - sizeof(void*)) % SEOUL_ALIGN_OF(void*));

		return *((void**)((Byte*)pAddress + zAllocationSizeInBytes - sizeof(void*)));
	}

	/**
	 * Update the userdata associated with pAddress.
	 *
	 * @return The previous userdata and the actual allocation size
	 * of the data at pAddress.
	 */
	static Pair<void*, size_t> SetUserData(void* pAddress, void* pUserData)
	{
		if (nullptr == pAddress)
		{
			return MakePair(nullptr, 0u);
		}

		size_t const zAllocationSizeInBytes = SEOUL_SYS_USABLE_SIZE(pAddress);
		SEOUL_ASSERT_NO_LOG(zAllocationSizeInBytes >= sizeof(void*));
		SEOUL_ASSERT_NO_LOG(0 == (size_t)((Byte*)pAddress + zAllocationSizeInBytes - sizeof(void*)) % SEOUL_ALIGN_OF(void*));

		auto& r = *((void**)((Byte*)pAddress + zAllocationSizeInBytes - sizeof(void*)));
		auto const pReturn = r;
		r = pUserData;
		return MakePair(pReturn, zAllocationSizeInBytes - sizeof(void*));
	}

	/**
	 * Update the userdata associated with pAddress,
	 * given that zActualSizeInBytes is the total
	 * size of pAddress as returned by GetAllocationSizeInBytes().
	 *
	 * @return The previous userdata and the actual allocation size
	 * of the data at pAddress.
	 */
	static void SetUserDataWithSize(size_t zActualSizeInBytes, void* pAddress, void* pUserData)
	{
		if (nullptr == pAddress)
		{
			return;
		}

		SEOUL_ASSERT_NO_LOG(0 == (size_t)((Byte*)pAddress + zActualSizeInBytes) % SEOUL_ALIGN_OF(void*));
		*((void**)((Byte*)pAddress + zActualSizeInBytes)) = pUserData;
	}
#endif // /SEOUL_ENABLE_MEMORY_TOOLING

	/**
	 * @return A block of memory aligned to zAlignmentInBytes
	 * or nullptr if the request could not be fulfilled.
	 *
	 * \pre zAlignmentInBytes must be a power of 2.
	 */
	static void* AllocateAligned(size_t zSizeInBytes, size_t zAlignmentInBytes)
	{
		// Make sure alignment is at our minimum supported size.
		zAlignmentInBytes = Max(zAlignmentInBytes, MemoryManager::kMinimumAlignment);

		// Can only accomodate power of 2 alignments.
		SEOUL_ASSERT_NO_LOG(IsPowerOfTwo(zAlignmentInBytes));

#if SEOUL_ENABLE_MEMORY_TOOLING
		// Add space for user data.
		zSizeInBytes = RoundUpToAlignment(zSizeInBytes, SEOUL_ALIGN_OF(void*)) + sizeof(void*);
#endif // /SEOUL_ENABLE_MEMORY_TOOLING

		// Perform the allocation - use malloc or aligned malloc
		// depending on whether the request is at-below or
		// above the minimum.
		void* pReturn = nullptr;
		if (zAlignmentInBytes <= kzMinimumMallocAlignment)
		{
			pReturn = SEOUL_SYS_MALLOC(zSizeInBytes);
		}
		else
		{
			pReturn = SEOUL_SYS_ALIGNED_ALLOC(zAlignmentInBytes, zSizeInBytes);
		}

		// Out of memory assertion - only applies if size > 0
		// (since malloc implementations are allowed to return nullptr when size == 0,
		// and realloc *must* return nullptr if zSizeInBytes == 0 and pAddressToReallocate
		// is not nullptr).
#if !SEOUL_ASSERTIONS_DISABLED
		SEOUL_ASSERT_MESSAGE(nullptr != pReturn || zSizeInBytes == 0, "Out of memory.");
#else
		if (nullptr == pReturn && zSizeInBytes > 0)
		{
			// Trigger a segmentation fault explicitly.
			(*((UInt volatile*)nullptr)) = 1;
		}
#endif

		// Sanity check.
		SEOUL_ASSERT_NO_LOG(0u == ((size_t)pReturn % zAlignmentInBytes));

		return pReturn;
	}

	/**
	 * @return The size of the user block of pAllocatedAddress,
	 * or 0u if pAllocatedAddress.
	 *
	 * \pre pAllocatedAddress must be nullptr or a valid heap managed
	 * address, or this method will SEOUL_FAIL().
	 */
	static size_t GetAllocationSizeInBytes(void* pAllocatedAddress)
	{
		size_t zReturnSizeInBytes = SEOUL_SYS_USABLE_SIZE(pAllocatedAddress);

		// Remove the space allocated for user data.
#if SEOUL_ENABLE_MEMORY_TOOLING
		SEOUL_ASSERT_NO_LOG(zReturnSizeInBytes >= sizeof(void*));
		zReturnSizeInBytes -= sizeof(void*);
#endif // /#if SEOUL_ENABLE_MEMORY_TOOLING

		return zReturnSizeInBytes;
	}

	/**
	 * @return Memory that fulfills zSizeInBytes and
	 * zAlignmentInBytes. If possible, the existing
	 * memory in pAddressToReallocate will be reused, otherwise
	 * a new memory block will be allocated and pAddressToReallocate
	 * will be freed.
	 */
	static void* ReallocateAligned(
		void* pAddressToReallocate,
		size_t zSizeInBytes,
		size_t zAlignmentInBytes)
	{
		// For consistent handling and consistency with std realloc (in C89 - the
		// standards committee has since opened this up so std realloc can do
		// either or - deallocate the buffer or act like malloc and potentially
		// return a "0 size" block).
		if (nullptr != pAddressToReallocate && 0 == zSizeInBytes)
		{
			Deallocate(pAddressToReallocate);
			return nullptr;
		}

		// If no address to reallocate, just use AllocateAligned().
		if (nullptr == pAddressToReallocate)
		{
			return AllocateAligned(zSizeInBytes, zAlignmentInBytes);
		}
		else
		{
			// Make sure alignment is at our minimum supported size.
			zAlignmentInBytes = Max(zAlignmentInBytes, MemoryManager::kMinimumAlignment);

			// Can only accomodate power of 2 alignments.
			SEOUL_ASSERT_NO_LOG(IsPowerOfTwo(zAlignmentInBytes));

#if SEOUL_ENABLE_MEMORY_TOOLING
			// Add space for user data.
			zSizeInBytes = RoundUpToAlignment(zSizeInBytes, SEOUL_ALIGN_OF(void*)) + sizeof(void*);
#endif // /SEOUL_ENABLE_MEMORY_TOOLING

			// If the alignment is <= kzMinimumMallocAlignment, use *realloc(), since
			// that is the worst case supported minimum alignment of *malloc.
			if (zAlignmentInBytes <= kzMinimumMallocAlignment)
			{
				void* pReturn = SEOUL_SYS_REALLOC(pAddressToReallocate, zSizeInBytes);

				// Out of memory assertion - only applies if size > 0
				// (since malloc implementations are allowed to return nullptr when size == 0,
				// and realloc *must* return nullptr if zSizeInBytes == 0 and pAddressToReallocate
				// is not nullptr).
#if !SEOUL_ASSERTIONS_DISABLED
				SEOUL_ASSERT_MESSAGE(nullptr != pReturn || 0 == zSizeInBytes, "Out of memory.");
#else
				if (nullptr == pReturn && zSizeInBytes > 0)
				{
					// Trigger a segmentation fault explicitly.
					(*((UInt volatile*)nullptr)) = 1;
				}
#endif

				// Sanity check.
				SEOUL_ASSERT_NO_LOG(0u == ((size_t)pReturn % zAlignmentInBytes));

				return pReturn;
			}
			else
			{
				size_t const zExistingAllocationSize = GetAllocationSizeInBytes(
					pAddressToReallocate);

				// If allocation fails, return nullptr, and leave the old data untouched.
				void* pReturn = AllocateAligned(zSizeInBytes, zAlignmentInBytes);

				memcpy(
					pReturn,
					pAddressToReallocate,
					Min(zExistingAllocationSize, zSizeInBytes));

				Deallocate(pAddressToReallocate);

				return pReturn;
			}
		}
	}

	/**
	 * Free the memory at pAddressToDeallocate.
	 *
	 * This method becomes a nop if pAddressToDeallocate
	 * is nullptr.
	 *
	 * \pre pAddressToDeallocate must be nullptr or a pointer to
	 * memory allocated with either AllocateAligned() or
	 * ReallocateAligned().
	 */
	static void Deallocate(void* pAddressToDeallocate)
	{
		SEOUL_SYS_FREE(pAddressToDeallocate);
	}

	///////////////////////////////////////////////////////////////////////////
	// Android and iOS implementations of AllocateAligned(),
	// GetAllocationSizeInBytes(), ReallocateAligned(), and Deallocate().
	///////////////////////////////////////////////////////////////////////////
#elif SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_IOS
	/** Convenience around various options. */
	static inline size_t low_level_malloc_usable_size(void const* ptr)
	{
#if SEOUL_PLATFORM_IOS
		return malloc_size(ptr);
#elif SEOUL_PLATFORM_ANDROID
		return android_malloc_usable_size(ptr);
#else // !SEOUL_PLATFORM_ANDROID
		return malloc_usable_size(ptr);
#endif // /!SEOUL_PLATFORM_IOS
	}

#if SEOUL_ENABLE_MEMORY_TOOLING
	/** Associate a user data pointer-sized value with a particular memory block. */
	static void* GetUserData(void* pAddress)
	{
		if (nullptr == pAddress)
		{
			return nullptr;
		}

		size_t const zAllocationSizeInBytes = low_level_malloc_usable_size(pAddress);
		SEOUL_ASSERT_NO_LOG(zAllocationSizeInBytes >= sizeof(void*));
		SEOUL_ASSERT_NO_LOG(0 == (size_t)((Byte*)pAddress + zAllocationSizeInBytes - sizeof(void*)) % SEOUL_ALIGN_OF(void*));

		return *((void**)((Byte*)pAddress + zAllocationSizeInBytes - sizeof(void*)));
	}

	/**
	 * Update the userdata associated with pAddress.
	 *
	 * @return The previous userdata and the actual allocation size
	 * of the data at pAddress.
	 */
	static Pair<void*, size_t> SetUserData(void* pAddress, void* pUserData)
	{
		if (nullptr == pAddress)
		{
			return MakePair(nullptr, 0u);
		}

		size_t const zAllocationSizeInBytes = low_level_malloc_usable_size(pAddress);
		SEOUL_ASSERT_NO_LOG(zAllocationSizeInBytes >= sizeof(void*));
		SEOUL_ASSERT_NO_LOG(0 == (size_t)((Byte*)pAddress + zAllocationSizeInBytes - sizeof(void*)) % SEOUL_ALIGN_OF(void*));

		auto& r = *((void**)((Byte*)pAddress + zAllocationSizeInBytes - sizeof(void*)));
		auto const pReturn = r;
		r = pUserData;
		return MakePair(pReturn, zAllocationSizeInBytes - sizeof(void*));
	}

	/**
	 * Update the userdata associated with pAddress,
	 * given that zActualSizeInBytes is the total
	 * size of pAddress as returned by GetAllocationSizeInBytes().
	 *
	 * @return The previous userdata and the actual allocation size
	 * of the data at pAddress.
	 */
	static void SetUserDataWithSize(size_t zActualSizeInBytes, void* pAddress, void* pUserData)
	{
		if (nullptr == pAddress)
		{
			return;
		}

		SEOUL_ASSERT_NO_LOG(0 == (size_t)((Byte*)pAddress + zActualSizeInBytes) % SEOUL_ALIGN_OF(void*));
		*((void**)((Byte*)pAddress + zActualSizeInBytes)) = pUserData;
	}
#endif // /SEOUL_ENABLE_MEMORY_TOOLING

	/**
	 * @return A block of memory aligned to zAlignmentInBytes
	 * or nullptr if the request could not be fulfilled.
	 *
	 * \pre zAlignmentInBytes must be a power of 2.
	 */
	static void* AllocateAligned(size_t zSizeInBytes, size_t zAlignmentInBytes)
	{
		// Make sure alignment is at our minimum supported size.
		zAlignmentInBytes = Max(zAlignmentInBytes, MemoryManager::kMinimumAlignment);

		// Can only accomodate power of 2 alignments.
		SEOUL_ASSERT_NO_LOG(IsPowerOfTwo(zAlignmentInBytes));

#if SEOUL_ENABLE_MEMORY_TOOLING
		// Add space for user data.
		zSizeInBytes = RoundUpToAlignment(zSizeInBytes, SEOUL_ALIGN_OF(void*)) + sizeof(void*);
#endif // /SEOUL_ENABLE_MEMORY_TOOLING

		// Perform the allocation - use malloc or aligned malloc
		// depending on whether the request is at-below or
		// above the minimum.
		void* pReturn = nullptr;

		// If we're at or under the minimum alignment, use malloc().
		if (zAlignmentInBytes <= kzMinimumSystemAlignment)
		{
			pReturn = malloc(zSizeInBytes);
		}
		// Otherwise use aligned alloc.
		else
		{
#if SEOUL_PLATFORM_ANDROID
			pReturn = android_memalign(zAlignmentInBytes, zSizeInBytes);
#else // !SEOUL_PLATFORM_ANDROID
			SEOUL_VERIFY_NO_LOG(0 == posix_memalign(&pReturn, zAlignmentInBytes, zSizeInBytes));
#endif // /!SEOUL_PLATFORM_ANDROID
		}

		// Out of memory assertion - only applies if size > 0
		// (since malloc implementations are allowed to return nullptr when size == 0,
		// and realloc *must* return nullptr if zSizeInBytes == 0 and pAddressToReallocate
		// is not nullptr).
#if !SEOUL_ASSERTIONS_DISABLED
		SEOUL_ASSERT_MESSAGE(nullptr != pReturn || 0 == zSizeInBytes, "Out of memory.");
#else
		if (nullptr == pReturn && zSizeInBytes > 0)
		{
			// Trigger a segmentation fault explicitly.
			(*((UInt volatile*)nullptr)) = 1;
		}
#endif

		// Sanity check.
		SEOUL_ASSERT_NO_LOG(0u == ((size_t)pReturn % zAlignmentInBytes));

		return pReturn;
	}

	/**
	 * @return The size of the user block of pAllocatedAddress,
	 * or 0u if pAllocatedAddress.
	 *
	 * \pre pAllocatedAddress must be nullptr or a valid heap managed
	 * address, or this method will SEOUL_FAIL().
	 */
	static size_t GetAllocationSizeInBytes(void* pAllocatedAddress)
	{
		size_t zReturnSizeInBytes = low_level_malloc_usable_size(pAllocatedAddress);

		// Remove the space allocated for user data.
#if SEOUL_ENABLE_MEMORY_TOOLING
		SEOUL_ASSERT_NO_LOG(zReturnSizeInBytes >= sizeof(void*));
		zReturnSizeInBytes -= sizeof(void*);
#endif // /#if SEOUL_ENABLE_MEMORY_TOOLING

		return zReturnSizeInBytes;
	}

	/**
	 * @return Memory that fulfills zSizeInBytes and
	 * zAlignmentInBytes. If possible, the existing
	 * memory in pAddressToReallocate will be reused, otherwise
	 * a new memory block will be allocated and pAddressToReallocate
	 * will be freed.
	 */
	static void* ReallocateAligned(
		void* pAddressToReallocate,
		size_t zSizeInBytes,
		size_t zAlignmentInBytes)
	{
		// For consistent handling and consistency with std realloc (in C89 - the
		// standards committee has since opened this up so std realloc can do
		// either or - deallocate the buffer or act like malloc and potentially
		// return a "0 size" block).
		if (nullptr != pAddressToReallocate && 0 == zSizeInBytes)
		{
			Deallocate(pAddressToReallocate);
			return nullptr;
		}

		// If no address to reallocate, just use AllocateAligned().
		if (nullptr == pAddressToReallocate)
		{
			return AllocateAligned(zSizeInBytes, zAlignmentInBytes);
		}
		else
		{
			// Make sure alignment is at our minimum supported size.
			zAlignmentInBytes = Max(zAlignmentInBytes, MemoryManager::kMinimumAlignment);

			// Can only accomodate power of 2 alignments.
			SEOUL_ASSERT_NO_LOG(IsPowerOfTwo(zAlignmentInBytes));

#if SEOUL_ENABLE_MEMORY_TOOLING
			// Add space for user data.
			zSizeInBytes = RoundUpToAlignment(zSizeInBytes, SEOUL_ALIGN_OF(void*)) + sizeof(void*);
#endif // /SEOUL_ENABLE_MEMORY_TOOLING

			// If the alignment is <= kzMinimumSystemAlignment, use realloc(), since
			// that is the worst case supported minimum alignment on iOS.
			if (zAlignmentInBytes <= kzMinimumSystemAlignment)
			{
				void* pReturn = realloc(pAddressToReallocate, zSizeInBytes);

				// Out of memory assertion - only applies if size > 0
				// (since malloc implementations are allowed to return nullptr when size == 0,
				// and realloc *must* return nullptr if zSizeInBytes == 0 and pAddressToReallocate
				// is not nullptr).
#if !SEOUL_ASSERTIONS_DISABLED
				SEOUL_ASSERT_MESSAGE(nullptr != pReturn || 0 == zSizeInBytes, "Out of memory.");
#else
				if (nullptr == pReturn && zSizeInBytes > 0)
				{
					// Trigger a segmentation fault explicitly.
					(*((UInt volatile*)nullptr)) = 1;
				}
#endif

				// Sanity check.
				SEOUL_ASSERT_NO_LOG(0u == ((size_t)pReturn % zAlignmentInBytes));

				return pReturn;
			}
			else
			{
				size_t const zExistingAllocationSize = GetAllocationSizeInBytes(
					pAddressToReallocate);

				// If allocation fails, return nullptr, and leave the old data untouched.
				void* pReturn = AllocateAligned(zSizeInBytes, zAlignmentInBytes);

				memcpy(
					pReturn,
					pAddressToReallocate,
					Min(zExistingAllocationSize, zSizeInBytes));

				Deallocate(pAddressToReallocate);

				return pReturn;
			}
		}
	}

	/**
	 * Free the memory at pAddressToDeallocate.
	 *
	 * This method becomes a nop if pAddressToDeallocate
	 * is nullptr.
	 *
	 * \pre pAddressToDeallocate must be nullptr or a pointer to
	 * memory allocated with either AllocateAligned() or
	 * ReallocateAligned().
	 */
	static void Deallocate(void* pAddressToDeallocate)
	{
		free(pAddressToDeallocate);
	}
#else
#	error "Define for this platform."
#endif

} // namespace MemoryManagerDetail

/**
 * @return A human readable string representation of the memory
 * budget type eType.
 */
const char* MemoryBudgets::ToString(MemoryBudgets::Type eType)
{
	switch (eType)
	{
	case Analytics: return "Analytics";
	case Animation: return "Animation";
	case Animation2D: return "Animation2D";
	case Animation3D: return "Animation3D";
	case Audio: return "Audio";
	case Commerce: return "Commerce";
	case Compression: return "Compression";
	case Config: return "Config";
	case Content: return "Content";
	case Cooking: return "Cooking";
	case Coroutines: return "Coroutines";
	case Curves: return "Curves";
	case DataStore: return "DataStore";
	case DataStoreData: return "DataStoreData";
	case Debug: return "Debug";
	case Developer: return "Developer";
	case DevUI: return "DevUI";
	case Editor: return "Editor";
	case Encryption: return "Encryption";
	case Event: return "Event";
	case Falcon: return "Falcon";
	case FalconFont: return "FalconFont";
	case Fx: return "Fx";
	case Game: return "Game";
	case HString: return "HString";
	case Input: return "Input";
	case Io: return "Io";
	case Jobs: return "Jobs";
	case Navigation: return "Navigation";
	case Network: return "Network";
	case None: return "None";
	case OperatorNew: return "Operator New";
	case OperatorNewArray: return "Operator New Array";
	case Particles: return "Particles";
	case Performance: return "Performance";
	case Persistence: return "Persistence";
	case Physics: return "Physics";
	case Profiler: return "Profiler";
	case Reflection: return "Reflection";
	case RenderCommandStream: return "RenderCommandStream";
	case Rendering: return "Rendering";
	case Saving: return "Saving";
	case Scene: return "Scene";
	case SceneComponent: return "SceneComponent";
	case SceneObject: return "SceneObject";
	case Scripting: return "Scripting";
	case SpatialSorting: return "SpatialSorting";
	case StateMachine: return "StateMachine";
	case Strings: return "Strings";
	case TBD: return "TBD";
	case TBDContainer: return "TBDContainer";
	case Threading: return "Threading";
	case UIData: return "UIData";
	case UIDebug: return "UIDebug";
	case UIRawMemory: return "UIRawMemory";
	case UIRendering: return "UIRendering";
	case UIRuntime: return "UIRuntime";
	case Video: return "Video";
	default:
		return "Unknown";
	};
}

#if SEOUL_ENABLE_MEMORY_TOOLING
#if SEOUL_ENABLE_STACK_TRACES
static const UInt32 kMaxCallStackCapture = 20u;
#endif // /SEOUL_ENABLE_STACK_TRACES

struct MemoryManagerBasicTrackingData SEOUL_SEALED
{
	MemoryBudgets::Type m_eType;
};

struct MemoryManagerVerboseTrackingData SEOUL_SEALED
{
	size_t m_zActualSizeInBytes;
	size_t m_zRequestedSizeInBytes;
	MemoryBudgets::Type m_eType;
#if !SEOUL_ENABLE_STACK_TRACES
	UInt32 m_uLineNumber;
	Byte const* m_psCallFilename;
#else
	size_t m_aCallStack[kMaxCallStackCapture];
#endif // /SEOUL_ENABLE_STACK_TRACES
	MemoryManagerVerboseTrackingData* m_pPrev;
	MemoryManagerVerboseTrackingData* m_pNext;
};

class MemoryManagerToolingDataWrapper SEOUL_SEALED
{
public:
	static MemoryManagerToolingDataWrapper CreateFromBasicTrackingData(MemoryBudgets::Type eType)
	{
		MemoryManagerToolingDataWrapper ret;
		ret.m_u = (((size_t)((UInt16)eType)) << 1) | (size_t)0x1;
		return ret;
	}

	static MemoryManagerToolingDataWrapper CreateFromVerboseTrackingData(MemoryManagerVerboseTrackingData* p)
	{
		MemoryManagerToolingDataWrapper ret;
		ret.m_p = p;
		return ret;
	}

	MemoryManagerToolingDataWrapper()
		: m_p(nullptr)
	{
	}

	explicit MemoryManagerToolingDataWrapper(void* p)
		: m_p(p)
	{
	}

	/* Lowest bit of the pointer will be 1 if inline basic tracking data. */
	Bool IsBasicTrackingData() const
	{
		return (0 != (m_u & 0x1));
	}

	/* Null is null. */
	Bool IsNull() const
	{
		return (nullptr == m_p);
	}

	/* Lowest bit of the pointer will be 0 if a pointer to tracking data. */
	Bool IsVerboseTrackingData() const
	{
		return (0 == (m_u & 0x1));
	}

	MemoryManagerBasicTrackingData AsBasicTrackingData() const
	{
		MemoryManagerBasicTrackingData data;

		if (IsVerboseTrackingData())
		{
			auto p = AsVerboseTrackingData();
			if (nullptr == p)
			{
				data.m_eType = MemoryBudgets::TBD;
			}
			else
			{
				data.m_eType = p->m_eType;
			}
		}
		else
		{
			data.m_eType = (MemoryBudgets::Type)(m_u >> 1);
		}

		return data;
	}

	MemoryManagerVerboseTrackingData* AsVerboseTrackingData() const
	{
		if (IsVerboseTrackingData())
		{
			return (MemoryManagerVerboseTrackingData*)m_p;
		}

		return nullptr;
	}

	void* ToVoidp() const { return m_p; }

private:
	union
	{
		void* m_p;
		size_t m_u;
	};
};
SEOUL_STATIC_ASSERT(sizeof(MemoryManagerToolingDataWrapper) == sizeof(void*));

/**
 * Internal data used by MemoryManager for
 * memory leak detection.
 */
class MemoryManagerToolingImpl SEOUL_SEALED
{
public:
	static MemoryManagerToolingImpl& GetOrLazyConstruct();
	static void AtExit();

	void ChangeBudget(void* pAddress, MemoryBudgets::Type eNewType)
	{
		if (pAddress == nullptr)
		{
			return;
		}

		MemoryManagerToolingDataWrapper wrapper(MemoryManagerDetail::GetUserData(pAddress));
		if (!wrapper.IsNull())
		{
			auto const zSize = MemoryManagerDetail::GetAllocationSizeInBytes(pAddress);
			auto const eType = wrapper.AsBasicTrackingData().m_eType;

			m_aAllocations[eType]--;
			m_aUsageInBytes[eType] -= (Atomic32Type)zSize;

			if (wrapper.IsBasicTrackingData())
			{
				wrapper = MemoryManagerToolingDataWrapper::CreateFromBasicTrackingData(eNewType);
				MemoryManagerDetail::SetUserData(pAddress, wrapper.ToVoidp());
			}
			else
			{
				wrapper.AsVerboseTrackingData()->m_eType = eNewType;
			}

			m_aUsageInBytes[eNewType] += (Atomic32Type)zSize;
			m_aAllocations[eNewType]++;
		}
	}

	Bool IsLeakingMemory() const;

	void MemoryLeakDetection(FILE* output);

	MemoryManagerVerboseTrackingData* GetHeadPerAllocationData() const
	{
		return m_pHeadPerAllocationData;
	}

	Int32 GetAllocations(MemoryBudgets::Type eType)
	{
		return (Int32)m_aAllocations[eType];
	}

	Int64 GetTotalUsageInBytes()
	{
		Int64 iReturn = 0;
		for (auto i : m_aUsageInBytes)
		{
			iReturn += i;
		}
		return iReturn;
	}

	Int32 GetUsageInBytes(MemoryBudgets::Type eType)
	{
		return (Int32)m_aUsageInBytes[eType];
	}

	void PrintMemoryDetails(
		MemoryBudgets::Type eType,
		MemoryManager::PrintfLike printfLike,
		void* userData,
		Bool bRaw);

	void SetMemoryLeaksFilename(const String& sFilename);

	void AssignToolingData(
		void* pAddress,
		size_t zSizeInBytes,
		MemoryBudgets::Type eType,
		UInt32 uLineNumber,
		Byte const* psCallFilename)
	{
		SEOUL_ASSERT_NO_LOG(nullptr != pAddress); // Sanity.

		auto const zActualSizeInBytes = MemoryManagerDetail::GetAllocationSizeInBytes(pAddress);
		if (s_bEnableVerboseMemoryLeakDetection)
		{
			MemoryManagerVerboseTrackingData* pData = CreateVerboseTrackingData(
				zActualSizeInBytes,
				zSizeInBytes,
				eType,
				uLineNumber,
				psCallFilename);
			MemoryManagerDetail::SetUserDataWithSize(zActualSizeInBytes, pAddress, pData);
		}
		else
		{
			MemoryManagerToolingDataWrapper wrapper = CreateBasicTrackingData(
				zActualSizeInBytes,
				zSizeInBytes,
				eType,
				uLineNumber,
				psCallFilename);
			MemoryManagerDetail::SetUserDataWithSize(zActualSizeInBytes, pAddress, wrapper.ToVoidp());
		}
	}

	void RemoveToolingData(void* pAddressToDeallocate)
	{
		SEOUL_ASSERT_NO_LOG(nullptr != pAddressToDeallocate); // Sanity.

		// Acquire and remove.
		auto const pair = MemoryManagerDetail::SetUserData(pAddressToDeallocate, nullptr);
		MemoryManagerToolingDataWrapper wrapper(pair.First);

		// Cleanup.
		if (auto pData = wrapper.AsVerboseTrackingData())
		{
			DestroyVerboseTrackingData(pData);
			wrapper = MemoryManagerToolingDataWrapper();
		}
		else if (wrapper.IsBasicTrackingData())
		{
			// Track the allocation.
			auto const eType = wrapper.AsBasicTrackingData().m_eType;
			m_aUsageInBytes[eType] -= (Atomic32Type)pair.Second;
			m_aAllocations[eType]--;
		}
	}

private:
	MemoryManagerToolingImpl();
	~MemoryManagerToolingImpl();

	MemoryManagerToolingDataWrapper CreateBasicTrackingData(
		size_t zActualSizeInBytes,
		size_t zRequestedSizeInBytes,
		MemoryBudgets::Type eType,
		UInt32 uLineNumber,
		Byte const* psCallFilename);

	MemoryManagerVerboseTrackingData* CreateVerboseTrackingData(
		size_t zActualSizeInBytes,
		size_t zRequestedSizeInBytes,
		MemoryBudgets::Type eType,
		UInt32 uLineNumber,
		Byte const* psCallFilename);

	void DestroyVerboseTrackingData(MemoryManagerVerboseTrackingData* pData);

	MemoryManagerVerboseTrackingData* m_pHeadPerAllocationData;
	Mutex m_PerAllocationListMutex;
	Byte m_aMemoryLeaksFilenameBuffer[1024];
	Atomic32 m_aAllocations[MemoryBudgets::TYPE_COUNT];
	Atomic32 m_aUsageInBytes[MemoryBudgets::TYPE_COUNT];
}; // class MemoryManagerToolingImpl

static volatile MemoryManagerToolingImpl* s_pMemoryManagerLeakDetectionImpl = nullptr;

MemoryManagerToolingImpl& MemoryManagerToolingImpl::GetOrLazyConstruct()
{
	if (nullptr == s_pMemoryManagerLeakDetectionImpl)
	{
		MemoryManagerToolingImpl* pImpl = (MemoryManagerToolingImpl*)MemoryManagerDetail::AllocateAligned(
			sizeof(MemoryManagerToolingImpl),
			SEOUL_ALIGN_OF(MemoryManagerToolingImpl));
		new (pImpl) MemoryManagerToolingImpl;

		if (nullptr != AtomicPointerCommon::AtomicSetIfEqual(
			(void**)(&s_pMemoryManagerLeakDetectionImpl),
			pImpl,
			nullptr))
		{
			pImpl->~MemoryManagerToolingImpl();
			MemoryManagerDetail::Deallocate(pImpl);
			pImpl = nullptr;
		}
	}

	return *((MemoryManagerToolingImpl*)s_pMemoryManagerLeakDetectionImpl);
}

void MemoryManagerToolingImpl::AtExit()
{
	while (nullptr != s_pMemoryManagerLeakDetectionImpl)
	{
		MemoryManagerToolingImpl* pImpl = (MemoryManagerToolingImpl*)s_pMemoryManagerLeakDetectionImpl;

		if (pImpl == AtomicPointerCommon::AtomicSetIfEqual(
			(void**)(&s_pMemoryManagerLeakDetectionImpl),
			nullptr,
			pImpl))
		{
			pImpl->~MemoryManagerToolingImpl();
			MemoryManagerDetail::Deallocate(pImpl);
			pImpl = nullptr;
		}
	}
}

/**
 * @return True if memory leaks were detected, false otherwise.
 */
Bool MemoryManagerToolingImpl::IsLeakingMemory() const
{
	for (Int i = 0; i < MemoryBudgets::TYPE_COUNT; ++i)
	{
		if (MemoryBudgets::Debug == i)
		{
			continue;
		}

		if (m_aAllocations[i] > 0)
		{
			return true;
		}
	}

	return false;
}

/**
 * If enabled, report memory leaks per still allocated block,
 * including requested size, type, filename, line number, and
 * a stack trace at the type of allocation, if available.
 */
void MemoryManagerToolingImpl::MemoryLeakDetection(FILE* output)
{
	SEOUL_ASSERT_NO_LOG(output);

#if SEOUL_ENABLE_STACK_TRACES
	static Byte s_aBuffer[4096u];
#endif // /SEOUL_ENABLE_STACK_TRACES

	// Basic data.
	fprintf(output, "---- Memory Leaks ----\n");
	for (Int i = 0; i < MemoryBudgets::TYPE_COUNT; ++i)
	{
		if (MemoryBudgets::Debug == i)
		{
			continue;
		}

		if (0 == m_aAllocations[i])
		{
			continue;
		}

		fprintf(output, "%s: %zu(%u)\n", MemoryBudgets::ToString((MemoryBudgets::Type)i), (size_t)m_aUsageInBytes[i], (UInt32)m_aAllocations[i]);
	}

	size_t uMemoryLeakCount = 0u;
	size_t uMemoryLeakTotalBytes = 0u;
	size_t uLeaksElided = 0u;
	size_t uBytesElided = 0u;
	for (auto pData = m_pHeadPerAllocationData; nullptr != pData; pData = pData->m_pNext)
	{
		if (MemoryBudgets::Debug != pData->m_eType)
		{
			// If this is the first leak, write out a header string.
			if (0 == uMemoryLeakCount)
			{
				fprintf(output, "\n---- Memory Leaks (verbose) ----\n");
			}

			uMemoryLeakCount++;
			uMemoryLeakTotalBytes += pData->m_zActualSizeInBytes;

			if (uMemoryLeakCount <= 500)
			{
				fprintf(output, "Memory Leak:\n");
				fprintf(output, "\tSize (Actual): %zu\n", pData->m_zActualSizeInBytes);
				fprintf(output, "\tSize (Requested): %zu\n", pData->m_zRequestedSizeInBytes);
				fprintf(output, "\tType: %s\n", MemoryBudgets::ToString(pData->m_eType));
#if !SEOUL_ENABLE_STACK_TRACES
				fprintf(output, "\tFile: %s\n", pData->m_psCallFilename);
				fprintf(output, "\tLine: %u\n", pData->m_uLineNumber);
#endif

#if SEOUL_ENABLE_STACK_TRACES
				if (Core::GetMapFile())
				{
					Core::PrintStackTraceToBuffer(s_aBuffer, sizeof(s_aBuffer), "\t\t", pData->m_aCallStack, kMaxCallStackCapture);
					fprintf(output, "\tStack Trace:\n");
					fprintf(output, "%s", s_aBuffer);
				}
				else
				{
					fprintf(output, "\tStack Trace: <no map file>\n");
				}
#else
				fprintf(output, "\tStack Trace: <disabled in this build>\n");
#endif // /!SEOUL_ENABLE_STACK_TRACES

				fprintf(output, "\n");
				MemoryManagerDetail::FlushFileStream(output);
			}
			else
			{
				// Only print the first 500 leaks, otherwise non-graceful
				// exits will take forever just logging all of the leaks
				uLeaksElided++;
				uBytesElided += pData->m_zActualSizeInBytes;
			}
		}
	}

	if (uLeaksElided > 0)
	{
		fprintf(output, "...\n(%zu verbose leaks of %zu bytes elided)\n...\n", uLeaksElided, uBytesElided);
	}

	fprintf(output, "\nTotal Verbose Memory Leak Count: %zu, Total Bytes: %zu.\n", uMemoryLeakCount, uMemoryLeakTotalBytes);
	MemoryManagerDetail::FlushFileStream(output);
}

void MemoryManagerToolingImpl::SetMemoryLeaksFilename(const String& sFilename)
{
	size_t const zStringLength = sFilename.GetSize();
	if (zStringLength + 1u <= sizeof(m_aMemoryLeaksFilenameBuffer))
	{
		STRNCPY(m_aMemoryLeaksFilenameBuffer, sFilename.CStr(), sizeof(m_aMemoryLeaksFilenameBuffer));
	}
	else
	{
		SEOUL_WARN("Memory leaks filename \"%s\" is too long.\n", sFilename.CStr());
	}
}

/** The fallback to carry basic data (inline) when verbose tracking is not enabled. */
MemoryManagerToolingDataWrapper MemoryManagerToolingImpl::CreateBasicTrackingData(
	size_t zActualSizeInBytes,
	size_t zRequestedSizeInBytes,
	MemoryBudgets::Type eType,
	UInt32 uLineNumber,
	Byte const* psCallFilename)
{
	auto const ret = MemoryManagerToolingDataWrapper::CreateFromBasicTrackingData(eType);

	// Track the allocation.
	m_aAllocations[eType]++;
	m_aUsageInBytes[eType] += (Atomic32Type)zActualSizeInBytes;

	return ret;
}

/**
 * When verbose memory leak tracking is enabled, this function
 * instantiates a new allocation data object to help with
 * tracking allocations. It is allocated on the same
 * heap used by MemoryManager.
 */
MemoryManagerVerboseTrackingData* MemoryManagerToolingImpl::CreateVerboseTrackingData(
	size_t zActualSizeInBytes,
	size_t zRequestedSizeInBytes,
	MemoryBudgets::Type eType,
	UInt32 uLineNumber,
	Byte const* psCallFilename)
{
	void* pMemoryArea = MemoryManagerDetail::AllocateAligned(
		sizeof(MemoryManagerVerboseTrackingData),
		SEOUL_ALIGN_OF(MemoryManagerVerboseTrackingData));

	MemoryManagerVerboseTrackingData* pData = reinterpret_cast<MemoryManagerVerboseTrackingData*>(pMemoryArea);
	pData->m_zActualSizeInBytes = zActualSizeInBytes;
	pData->m_zRequestedSizeInBytes = zRequestedSizeInBytes;
	pData->m_eType = eType;

#if !SEOUL_ENABLE_STACK_TRACES
	pData->m_uLineNumber = uLineNumber;
	pData->m_psCallFilename = psCallFilename;
#endif

#if SEOUL_ENABLE_STACK_TRACES
	memset(pData->m_aCallStack, 0, sizeof(pData->m_aCallStack));
	(void)Core::GetCurrentCallStack(
		0u,
		kMaxCallStackCapture,
		pData->m_aCallStack);
#endif

	{
		Lock lock(m_PerAllocationListMutex);
		pData->m_pNext = m_pHeadPerAllocationData;
		pData->m_pPrev = nullptr;
		if (pData->m_pNext)
		{
			pData->m_pNext->m_pPrev = pData;
		}
		m_pHeadPerAllocationData = pData;
	}

	// Track the allocation.
	m_aAllocations[eType]++;
	m_aUsageInBytes[eType] += (Atomic32Type)zActualSizeInBytes;

	return pData;
}

/**
 * Clean up an allocation made for memory tracking purposes.
 */
void MemoryManagerToolingImpl::DestroyVerboseTrackingData(MemoryManagerVerboseTrackingData* pData)
{
	// Early out for null values.
	if (nullptr == pData)
	{
		return;
	}

	// Track the allocation.
	m_aUsageInBytes[pData->m_eType] -= (Atomic32Type)(pData->m_zActualSizeInBytes);
	m_aAllocations[pData->m_eType]--;

	{
		Lock lock(m_PerAllocationListMutex);
		if (pData->m_pNext)
		{
			pData->m_pNext->m_pPrev = pData->m_pPrev;
		}

		if (pData->m_pPrev)
		{
			pData->m_pPrev->m_pNext = pData->m_pNext;
		}

		if (m_pHeadPerAllocationData == pData)
		{
			m_pHeadPerAllocationData = pData->m_pNext;
		}

		pData->m_pNext = nullptr;
		pData->m_pPrev = nullptr;
	}

	MemoryManagerDetail::Deallocate(pData);
}

MemoryManagerToolingImpl::MemoryManagerToolingImpl()
	: m_pHeadPerAllocationData(nullptr)
	, m_PerAllocationListMutex()
{
	static Byte const* const ksDefaultMemoryLeaksFilename = "memory_leaks.txt";

	STRNCPY(m_aMemoryLeaksFilenameBuffer, ksDefaultMemoryLeaksFilename, sizeof(m_aMemoryLeaksFilenameBuffer));
}

MemoryManagerToolingImpl::~MemoryManagerToolingImpl()
{
	// If memory is leaking, write out a memory leak report.
	if (IsLeakingMemory())
	{
		// Open the file stream used by either detection method. If
		// the filename is empty, just use stdout.
		FILE* output = nullptr;
		if (m_aMemoryLeaksFilenameBuffer[0] == '\0' ||
			!MemoryManagerDetail::OpenFileStream(m_aMemoryLeaksFilenameBuffer, output))
		{
			output = stdout;
		}

		MemoryLeakDetection(output);

		// Close the output stream if it's not stdout.
		if (stdout != output)
		{
			MemoryManagerDetail::CloseFileStream(output);
		}
		output = nullptr;

		// Yell about the memory leaks.
		PlatformPrint ::PrintDebugString(
			PlatformPrint::Type::kError,
			"--------------------------------------------------------\n"
			"MEMORY LEAKS DETECTED, SEE MEMORYLEAKS FILE FOR DETAILS.\n"
			"--------------------------------------------------------\n");
	}

	// The map file, if defined, is destroyed here. This allows
	// it to be used for memory leak reporting. Do this with
	// custom handling, so we can avoid needlessly recreating
	// the global MemoryManagerToolingImpl() with
	// a call to MemoryManagerToolingImpl::GetOrLazyConstruct().
#if SEOUL_ENABLE_STACK_TRACES
	IMapFile* pMapFile = Core::GetMapFile();
	Core::SetMapFile(nullptr);
	if (nullptr != pMapFile)
	{
		// Destruct the object.
		pMapFile->~IMapFile();

		// Remove leak tracking user data.
		RemoveToolingData(pMapFile);

		// Deallocate the memory.
		MemoryManagerDetail::Deallocate(pMapFile);
		pMapFile = nullptr;
	}
#endif // /SEOUL_ENABLE_STACK_TRACES
}

/**
 * Changes the memory budget of the given piece of memory
 *
 * @param[in] pAddress Address of memory to change the memory budget for
 * @param[in] eNewType New memory budget for the given piece of memory
 */
void MemoryManager::ChangeBudget(void* pAddress, MemoryBudgets::Type eNewType)
{
	MemoryManagerToolingImpl& impl = MemoryManagerToolingImpl::GetOrLazyConstruct();
	impl.ChangeBudget(pAddress, eNewType);
}

/** @return true if verbose memory leak detection is currently enabled. */
Bool MemoryManager::GetVerboseMemoryLeakDetectionEnabled()
{
	return s_bEnableVerboseMemoryLeakDetection;
}

/**
 * Runtime control of verbose memory leak detection. Useful
 * in tools and other scenarios where we want a developer
 * build (with logging, assertions, etc. enabled) but don't
 * want the overhead of verbose memory leak tracking.
 */
void MemoryManager::SetVerboseMemoryLeakDetectionEnabled(Bool bEnabled)
{
	s_bEnableVerboseMemoryLeakDetection = bEnabled;
}

/** Current memory allocation count of memory of type eType. */
Int32 MemoryManager::GetAllocations(MemoryBudgets::Type eType)
{
	MemoryManagerToolingImpl& impl = MemoryManagerToolingImpl::GetOrLazyConstruct();
	return impl.GetAllocations(eType);
}

/**
 * Current memory used by memory of type eType. This includes
 * the actual memory size, including oversizing/overhead of the
 * memory allocator.
 */
Int32 MemoryManager::GetUsageInBytes(MemoryBudgets::Type eType)
{
	MemoryManagerToolingImpl& impl = MemoryManagerToolingImpl::GetOrLazyConstruct();
	return impl.GetUsageInBytes(eType);
}

/** Current memory used in total. */
Int64 MemoryManager::GetTotalUsageInBytes()
{
	MemoryManagerToolingImpl& impl = MemoryManagerToolingImpl::GetOrLazyConstruct();
	return impl.GetTotalUsageInBytes();
}

/**
 * Sets the filename that will be used to write memory leaks on program
 * exit. If not specified, a file called "memory_leaks.txt" will be
 * written to the current directory.
 */
void MemoryManager::SetMemoryLeaksFilename(const String& sFilename)
{
	MemoryManagerToolingImpl& impl = MemoryManagerToolingImpl::GetOrLazyConstruct();
	impl.SetMemoryLeaksFilename(sFilename);
}

/**
 * If called and memory allocator debug information is available,
 * prints memory detail to printfLike. If bRaw is true, logs every tracked
 * memory block for the given MemoryBudget. Otherwise, prints a summarized
 * view, keyed on an indicative stack frame stored with the blocks.
 *
 * \remarks Pass MemoryBudgets::Unknown to include all memory budgets.
 */
void MemoryManager::PrintMemoryDetails(
	MemoryBudgets::Type eType,
	PrintfLike printfLike,
	void* userData,
	Bool bRaw /*= false*/)
{
	MemoryManagerToolingImpl::GetOrLazyConstruct().PrintMemoryDetails(eType, printfLike, userData, bRaw);
}

namespace
{

// For summarized memory bucket print, entry used to gather
// into tracking table.
struct SummaryEntry
{
	size_t m_zTotalSize;
	size_t m_zTotalCount;
	MemoryBudgets::Type m_eType;
};
// For summarized memory bucket print, full entry stored
// in flat vector.
struct FullEntry : SummaryEntry
{
	size_t m_zAddress;

	Bool operator<(const FullEntry& b) const
	{
		return m_zTotalSize > b.m_zTotalSize;
	}
};

// For summarized memory bucket print, prefixes used
// to filter frames in a callstack as "not indicative" - generally
// container frames with a few special cases (e.g. operator new).
static const String kasFilteredTypes[] =
{
	"operator new",
	"Seoul::Allocator",
	"Seoul::AtomicRingBuffer",
	"Seoul::DataStore",
	"Seoul::HashSet",
	"Seoul::HashTable",
	"Seoul::List",
	"Seoul::MemoryManager",
	"Seoul::Queue",
	"Seoul::StdContainerAllocator",
	"Seoul::StreamBuffer",
	"Seoul::String",
	"Seoul::Vector",
	"Seoul::ZSTDCompress",
	"Seoul::ZSTDDecompress",
	"std::_List_alloc",
	"std::_List_buy",
	"std::list",
};

static const String ksSeoulPrefix("Seoul::");

} // namespace anonymous

void MemoryManagerToolingImpl::PrintMemoryDetails(
	MemoryBudgets::Type eType,
	MemoryManager::PrintfLike printfLike,
	void* userData,
	Bool bRaw)
{
#if SEOUL_ENABLE_STACK_TRACES
	static Byte s_aBuffer[4096u];
#endif // /SEOUL_ENABLE_STACK_TRACES

	// Exclusive access by this thread to the tracking list - relies
	// on Seoul::Mutex implementing re-entrancy for a Mutex from a given
	// thread (since this body can heap allocate in the bRaw = false case).
	Lock lock(m_PerAllocationListMutex);

	// Raw must be forced if stack traces are not enabled or
	// if we don't have a map file to perform resolves.
#if SEOUL_ENABLE_STACK_TRACES
	if (nullptr == Core::GetMapFile())
	{
		bRaw = true;
	}
#else
	bRaw = true;
#endif

	// Raw, just log the buckets in allocation order.
	if (bRaw)
	{
		size_t nMemoryAllocationCount = 0u;
		size_t nMemoryAllocationTotalBytes = 0u;
		for (MemoryManagerVerboseTrackingData* pData = GetHeadPerAllocationData(); nullptr != pData; pData = pData->m_pNext)
		{
			if (MemoryBudgets::Unknown == eType ||
				eType == pData->m_eType)
			{
				nMemoryAllocationCount++;
				nMemoryAllocationTotalBytes += pData->m_zActualSizeInBytes;

				printfLike(userData, "Memory Allocation:\n");
				printfLike(userData, "\tSize (Actual): %zu\n", pData->m_zActualSizeInBytes);
				printfLike(userData, "\tSize (Requested): %zu\n", pData->m_zRequestedSizeInBytes);
				printfLike(userData, "\tSize: %zu\n", pData->m_zRequestedSizeInBytes);
				printfLike(userData, "\tType: %s\n", MemoryBudgets::ToString(pData->m_eType));
#if !SEOUL_ENABLE_STACK_TRACES
				printfLike(userData, "\tFile: %s\n", pData->m_psCallFilename);
				printfLike(userData, "\tLine: %u\n", pData->m_uLineNumber);
#endif

#if SEOUL_ENABLE_STACK_TRACES
				if (Core::GetMapFile())
				{
					Core::PrintStackTraceToBuffer(s_aBuffer, sizeof(s_aBuffer), "\t\t", pData->m_aCallStack, kMaxCallStackCapture);
					printfLike(userData, "\tStack Trace:\n");
					printfLike(userData, "%s", s_aBuffer);
				}
				else
				{
					printfLike(userData, "\tStack Trace: <no map file>\n");
				}
#else
				printfLike(userData, "\tStack Trace: <disabled in this build>\n");
#endif // /!SEOUL_ENABLE_STACK_TRACES

				printfLike(userData, "\n");
			}
		}

		printfLike(userData, "Memory Allocation Count: %zu, Total Bytes: %zu", nMemoryAllocationCount, nMemoryAllocationTotalBytes);
	}
#if SEOUL_ENABLE_STACK_TRACES
	// bRaw = false implies "human friendly" - in this case, we gather allocations based on a filtered indicative
	// function on the stack, and then print allocations in largest (total) to smallest.
	else
	{
		// NOTE: Use the Debug memory budget for all temporary containers
		// so it can track successfully.

		// Emit - sanity first, should have been enforced by check at head of function.
		auto pMapFile = Core::GetMapFile();
		SEOUL_ASSERT(nullptr != pMapFile);
		pMapFile->WaitUntilLoaded();

		// Gather
		UInt64 uOverallSize = 0;
		Vector<FullEntry, MemoryBudgets::Debug> v;
		{
			// true if a function should not be used as indicative (part of a core container
			// and a few other cases, such as operator new), false otherwise.
			auto const filteredFuncName = [&]()
			{
				auto const sFuncName = s_aBuffer;
				for (auto const& s : kasFilteredTypes)
				{
					if (0 == strncmp(s.CStr(), sFuncName, s.GetSize()))
					{
						return true;
					}
				}

				return false;
			};

			// Used for filtering certain stack frames that are effectively
			// noise for identifying allocation points (e.g. methods on various Seoul
			// engine containers).
			HashTable<size_t, Bool, MemoryBudgets::Debug> tFiltered;
			auto const getIndicativeAddress = [&](size_t aCallStack[kMaxCallStackCapture])
			{
				for (auto i = 0u; i < kMaxCallStackCapture; ++i)
				{
					auto const zAddress = aCallStack[i];

					// 0 indicates end of capture, stop iterating.
					if (0 == zAddress)
					{
						break;
					}

					// Check if we've recorded filtering for this address already,
					// otherwise, query and store.
					Bool bFiltered = false;
					if (!tFiltered.GetValue(zAddress, bFiltered))
					{
						// If we fail to query, assume filtered.
						if (!pMapFile->QueryFunctionName(zAddress, s_aBuffer, sizeof(s_aBuffer)))
						{
							bFiltered = true;
						}
						else
						{
							// Filter based on name.
							bFiltered = filteredFuncName();
						}

						// Track.
						tFiltered.Insert(zAddress, bFiltered);
					}

					// First address that is not filter, use as indicative.
					if (!bFiltered)
					{
						return zAddress;
					}
				}

				// Fallback to stack top - happens if the capture is not deep enough to
				// include an indicative function.
				return aCallStack[0];
			};

			// Gather.
			HashTable<size_t, SummaryEntry, MemoryBudgets::Debug> t;
			for (auto pData = GetHeadPerAllocationData(); nullptr != pData; pData = pData->m_pNext)
			{
				// Budget of the desired type, or we're gathering all, record.
				if (MemoryBudgets::Unknown == eType || eType == pData->m_eType)
				{
					// Address to use to represent.
					auto const zAddress = getIndicativeAddress(pData->m_aCallStack);
					// Track and sum - only inserts if not already inserted.
					auto const e = t.Insert(zAddress, SummaryEntry{});
					// Sum.
					auto& r = e.First->Second;
					r.m_zTotalCount++;
					r.m_zTotalSize += pData->m_zActualSizeInBytes;
					// Use last budget - arbitrary.
					r.m_eType = pData->m_eType;
					// Also accumulate into total size.
					uOverallSize += pData->m_zActualSizeInBytes;
				}
			}

			// Final gather for sort.
			v.Reserve(t.GetSize());
			for (auto const& pair : t)
			{
				FullEntry e;
				e.m_zAddress = pair.First;
				e.m_zTotalCount = pair.Second.m_zTotalCount;
				e.m_zTotalSize = pair.Second.m_zTotalSize;
				e.m_eType = pair.Second.m_eType;
				v.PushBack(e);
			}
		}

		// Sort.
		QuickSort(v.Begin(), v.End());

		// Formatting constants.
		static const UInt32 kuFuncNameColumnWidth = 82;
		static const UInt32 kuFileColumnWidth = 40;
		static const UInt32 kuLineColumnWidth = 7;
		static const UInt32 kuSizeColumnWidth = 8;
		static const UInt32 kuCountColumnWidth = 11;
		static const UInt32 kuAvgColumnWidth = 10;
		static const UInt32 kuBucketColumnWidth = 22;
		static const UInt32 kuColumnMargin = 2u;

		// Help messaging and general info.
		if (MemoryBudgets::Unknown == eType /* Total */)
		{
			printfLike(userData, "All: %" PRIu64 " MBs\n", uOverallSize / (1024 * 1024));
		}
		else
		{
			printfLike(userData, "%s: %" PRIu64 " MBs\n", MemoryBudgets::ToString(eType), uOverallSize / (1024 * 1024));
		}
		printfLike(userData, "NOTE: All values are the current snapshot (no historical data).\n");
		printfLike(userData, "NOTE: 'Size' is the sum total size of all allocations from a code location.\n");
		printfLike(userData, "NOTE: 'Count' is the total number of single allocations from a code location.\n");
		printfLike(userData, "NOTE: 'Avg.' is the average size of a single allocation from a code location.\n\n");

		// Column headers.
		printfLike(userData, "%*s", kuFuncNameColumnWidth, "Name");
		printfLike(userData, "%*s", kuFileColumnWidth, "File");
		printfLike(userData, ":%-*s", kuLineColumnWidth, "Line");
		printfLike(userData, "%*s", kuSizeColumnWidth, "Size");
		printfLike(userData, "%*s", kuCountColumnWidth, "Count");
		printfLike(userData, "%*s", kuAvgColumnWidth, "Avg.");
		if (MemoryBudgets::Unknown == eType)
		{
			printfLike(userData, "%*s", kuBucketColumnWidth, "Bucket");
		}
		printfLike(userData, "\n");

		// Utility - remove "Seoul::" from all places
		// in the value of s_aBuffer. Used to shorten
		// function names.
		auto const stripSeoul = [&]()
		{
			auto sIn = s_aBuffer;
			auto sOut = s_aBuffer;
			while (*sIn)
			{
				if (0 == strncmp(sIn, ksSeoulPrefix.CStr(), ksSeoulPrefix.GetSize()))
				{
					sIn += ksSeoulPrefix.GetSize();
					continue;
				}

				*sOut = *sIn;
				++sIn;
				++sOut;
			}
			// Terminate.
			*sOut = *sIn;
		};

		// Return a shortened pointer to s,
		// right-aligned, to zLength.
		auto const truncateString = [&](Byte const* s, size_t zLength)
		{
			auto const zSize = strlen(s);
			if (zSize > zLength)
			{
				return s + zSize - zLength;
			}
			else
			{
				return s;
			}
		};

		// Equivalent to truncateString, except also
		// limits the filename to the last part (the base filename).
		auto const truncateFileName = [&](Byte const* s, size_t zLength)
		{
			// Find the base filename part.
			auto sReturn = strrchr(s, '\\');
			if (nullptr == sReturn)
			{
				sReturn = s;
			}
			else
			{
				sReturn++;
			}

			// Shorten.
			return truncateString(sReturn, zLength);
		};

		// Common utility, print nice size.
		auto const printSize = [&](size_t z, UInt32 uColumnWidth)
		{
			if (z > 1024 * 1024)
			{
				printfLike(userData, "%*zu MBs", (uColumnWidth - 4), z / (1024 * 1024));
			}
			else if (z > 1024)
			{
				printfLike(userData, "%*zu KBs", (uColumnWidth - 4), z / 1024);
			}
			else
			{
				printfLike(userData, "%*zu  Bs", (uColumnWidth - 4), z);
			}
		};

		// Entries, print.
		for (auto const& e : v)
		{
			// Name
			if (pMapFile->QueryFunctionName(e.m_zAddress, s_aBuffer, sizeof(s_aBuffer)))
			{
				stripSeoul();
				printfLike(userData, "%*s", kuFuncNameColumnWidth, truncateString(s_aBuffer, kuFuncNameColumnWidth));
			}
			else
			{
				printfLike(userData, "%*s", kuFuncNameColumnWidth, "<unknown>");
			}

			// File:Line
			UInt32 uLine = 0u;
			if (pMapFile->QueryLineInfo(e.m_zAddress, s_aBuffer, sizeof(s_aBuffer), &uLine))
			{
				printfLike(userData, "%*s", kuFileColumnWidth, truncateFileName(s_aBuffer, kuFileColumnWidth - kuColumnMargin));
				printfLike(userData, ":%-*u", kuLineColumnWidth, uLine);
			}
			else
			{
				printfLike(userData, "%*s", kuFileColumnWidth, "<unknown>");
				printfLike(userData, ":%-*s", kuLineColumnWidth, "-1");
			}

			// Size
			printSize(e.m_zTotalSize, kuSizeColumnWidth);
			// Count
			printfLike(userData, "%*zu", kuCountColumnWidth, e.m_zTotalCount);
			// Avg.
			printSize(e.m_zTotalCount > 0 ? (e.m_zTotalSize / e.m_zTotalCount) : 0, kuAvgColumnWidth);
			// (Optional) Bucket (if printing all buckets)
			if (MemoryBudgets::Unknown == eType)
			{
				printfLike(userData, "%*s", kuBucketColumnWidth, MemoryBudgets::ToString(e.m_eType));
			}
			printfLike(userData, "\n");
		}
#endif // /#if SEOUL_ENABLE_STACK_TRACES
	}
}
#endif // /SEOUL_ENABLE_MEMORY_TOOLING

#if SEOUL_ENABLE_MEMORY_TOOLING
// Under MSVC, insert the AtExit hook late, right before C pre-termination
// handlers, by inserting an initialization hook right before C++
// constructions.
// See also:
// - http://www.codeguru.com/cpp/misc/misc/applicationcontrol/article.php/c6945/Running-Code-Before-and-After-Main.htm
// - https://social.msdn.microsoft.com/Forums/en-US/7737e56f-c813-4c96-a632-5db7a26ef2fc/linker-bug-datasegcrtxiu-not-merged-into-other-crt-segments?forum=vcgeneral
#ifdef _MSC_VER
#	pragma section(".CRT$XIU", long, read)
static void MsvcLeakDetectionAtExitHookInsert();
__declspec(allocate(".CRT$XIU")) void (__cdecl* g_MsvcLeakDetectionAtExitHookInsert)() = MsvcLeakDetectionAtExitHookInsert;
static void MsvcLeakDetectionAtExitHookInsert()
{
	(void)atexit(MemoryManagerToolingImpl::AtExit);
}
#endif // /_MSC_VER
#endif // /SEOUL_ENABLE_MEMORY_TOOLING

/**
 * Handles all memory allocation requests.
 */
void* MemoryManager::InternalAllocate(
	size_t zSizeInBytes,
	size_t zAlignmentInBytes,
	MemoryBudgets::Type eType,
	UInt32 uLineNumber,
	Byte const* psCallFilename)
{
	void* pReturn = MemoryManagerDetail::AllocateAligned(zSizeInBytes, zAlignmentInBytes);

#if SEOUL_ENABLE_MEMORY_TOOLING
	if (nullptr != pReturn)
	{
		MemoryManagerToolingImpl& impl = MemoryManagerToolingImpl::GetOrLazyConstruct();
		impl.AssignToolingData(pReturn, zSizeInBytes, eType, uLineNumber, psCallFilename);
	}
#endif // /SEOUL_ENABLE_MEMORY_TOOLING

	return pReturn;
}

/**
 * Handles all memory reallocation requests.
 */
void* MemoryManager::InternalReallocate(
	void* pAddressToReallocate,
	size_t zSizeInBytes,
	size_t zAlignmentInBytes,
	MemoryBudgets::Type eType,
	UInt32 uLineNumber,
	Byte const* psCallFilename)
{
#if SEOUL_ENABLE_MEMORY_TOOLING
	if (nullptr != pAddressToReallocate)
	{
		MemoryManagerToolingImpl& impl = MemoryManagerToolingImpl::GetOrLazyConstruct();
		impl.RemoveToolingData(pAddressToReallocate);
	}
#endif

	void* pReturn = MemoryManagerDetail::ReallocateAligned(pAddressToReallocate, zSizeInBytes, zAlignmentInBytes);

#if SEOUL_ENABLE_MEMORY_TOOLING
	if (nullptr != pReturn)
	{
		MemoryManagerToolingImpl& impl = MemoryManagerToolingImpl::GetOrLazyConstruct();
		impl.AssignToolingData(pReturn, zSizeInBytes, eType, uLineNumber, psCallFilename);
	}
#endif // /SEOUL_ENABLE_MEMORY_TOOLING

	return pReturn;
}

/**
 * Handles all memory deallocation requests.
 */
void MemoryManager::InternalDeallocate(
	void* pAddressToDeallocate)
{
#if SEOUL_ENABLE_MEMORY_TOOLING
	if (nullptr != pAddressToDeallocate)
	{
		MemoryManagerToolingImpl& impl = MemoryManagerToolingImpl::GetOrLazyConstruct();
		impl.RemoveToolingData(pAddressToDeallocate);
	}
#endif // /SEOUL_ENABLE_MEMORY_TOOLING

	MemoryManagerDetail::Deallocate(pAddressToDeallocate);
}

/**
 * Handles memory block size requests.
 */
size_t MemoryManager::InternalGetAllocationSizeInBytes(
	void* pAllocatedAddress)
{
	return MemoryManagerDetail::GetAllocationSizeInBytes(pAllocatedAddress);
}

} // namespace Seoul

#if SEOUL_OVERRIDE_GLOBAL_NEW

/**
 * Overrides for global new and delete operators - note that overrides
 * of alignment versions of new are non-standard GCC extensions.
 */
//////// non-debug new operators ////////
// Throwing, standard new
void* operator new(size_t zRequestedMemory) SEOUL_THROWS(std::bad_alloc)
{
	return Seoul::MemoryManager::Allocate(
		zRequestedMemory,
		Seoul::MemoryBudgets::OperatorNew);
}

// Throwing, standard new with a memory type argument
void* operator new (
	size_t zRequestedMemory,
	Seoul::MemoryBudgets::Type eType) SEOUL_THROWS(std::bad_alloc)
{
	return Seoul::MemoryManager::Allocate(
		zRequestedMemory,
		eType);
}

// No throw, standard new
void* operator new(
	size_t zRequestedMemory,
	const std::nothrow_t&) SEOUL_THROW0()
{
	return Seoul::MemoryManager::Allocate(
		zRequestedMemory,
		Seoul::MemoryBudgets::OperatorNew);
}

// Throwing, alignment new
void* operator new(
	size_t zRequestedMemory,
	size_t zRequestedAlignment) SEOUL_THROWS(std::bad_alloc)
{
	return Seoul::MemoryManager::AllocateAligned(
		zRequestedMemory,
		Seoul::MemoryBudgets::OperatorNew,
		zRequestedAlignment);
}

// No throw, alignment new
void* operator new(
	size_t zRequestedMemory,
	size_t zRequestedAlignment,
	const std::nothrow_t&) SEOUL_THROW0()
{
	return Seoul::MemoryManager::AllocateAligned(
		zRequestedMemory,
		Seoul::MemoryBudgets::OperatorNew,
		zRequestedAlignment);
}

//////// non-debug new array operators ////////
// Throwing, standard new array
void* operator new[](
	size_t zRequestedMemory) SEOUL_THROWS(std::bad_alloc)
{
	return Seoul::MemoryManager::Allocate(
		zRequestedMemory,
		Seoul::MemoryBudgets::OperatorNewArray);
}

// Throwing, standard array new with a memory type argument
void* operator new[](
	size_t zRequestedMemory,
	Seoul::MemoryBudgets::Type eType) SEOUL_THROWS(std::bad_alloc)
{
	return Seoul::MemoryManager::Allocate(
		zRequestedMemory,
		eType);
}

// No throw, standard new array
void* operator new[](
	size_t zRequestedMemory,
	const std::nothrow_t&) SEOUL_THROW0()
{
	return Seoul::MemoryManager::Allocate(
		zRequestedMemory,
		Seoul::MemoryBudgets::OperatorNewArray);
}

// Throwing, alignment new array
void* operator new[](
	size_t zRequestedMemory,
	size_t zRequestedAlignment) SEOUL_THROWS(std::bad_alloc)
{
	return Seoul::MemoryManager::AllocateAligned(
		zRequestedMemory,
		Seoul::MemoryBudgets::OperatorNewArray,
		zRequestedAlignment);
}

// No throw, alignment new array
void* operator new[](
	size_t zRequestedMemory,
	size_t zRequestedAlignment,
	const std::nothrow_t&) SEOUL_THROW0()
{
	return Seoul::MemoryManager::AllocateAligned(
		zRequestedMemory,
		Seoul::MemoryBudgets::OperatorNewArray,
		zRequestedAlignment);
}

//////// debug new operators ////////
void* operator new (
	size_t zRequestedMemory,
	char const* sCallerFilename,
	int iCallerLine) SEOUL_THROWS(std::bad_alloc)
{
	return Seoul::MemoryManager::Allocate(
		zRequestedMemory,
		Seoul::MemoryBudgets::OperatorNew,
		sCallerFilename,
		iCallerLine);
}

void* operator new (
	size_t zRequestedMemory,
	char const* sCallerFilename,
	int iCallerLine,
	Seoul::MemoryBudgets::Type eType) SEOUL_THROWS(std::bad_alloc)
{
	return Seoul::MemoryManager::Allocate(
		zRequestedMemory,
		eType,
		sCallerFilename,
		iCallerLine);
}

void* operator new[](
	size_t zRequestedMemory,
	char const* sCallerFilename,
	int iCallerLine) SEOUL_THROWS(std::bad_alloc)
{
	return Seoul::MemoryManager::Allocate(
		zRequestedMemory,
		Seoul::MemoryBudgets::OperatorNewArray,
		sCallerFilename,
		iCallerLine);
}

void* operator new[](
	size_t zRequestedMemory,
	char const* sCallerFilename,
	int iCallerLine,
	Seoul::MemoryBudgets::Type eType) SEOUL_THROWS(std::bad_alloc)
{
	return Seoul::MemoryManager::Allocate(
		zRequestedMemory,
		eType,
		sCallerFilename,
		iCallerLine);
}

//////// delete operators ////////
void operator delete (
	void* pAddressToDeallocate,
	Seoul::MemoryBudgets::Type /*eType*/) SEOUL_THROW0()
{
	Seoul::MemoryManager::Deallocate(pAddressToDeallocate);
}

void operator delete[](
	void* pAddressToDeallocate,
	Seoul::MemoryBudgets::Type /*eType*/) SEOUL_THROWS(std::bad_alloc)
{
	Seoul::MemoryManager::Deallocate(pAddressToDeallocate);
}

void operator delete (
	void* pAddressToDeallocate,
	char const* /*sCallerFilename*/,
	int /*iCallerLine*/) SEOUL_THROWS(std::bad_alloc)
{
	Seoul::MemoryManager::Deallocate(pAddressToDeallocate);
}

void operator delete (
	void* pAddressToDeallocate,
	char const* /*sCallerFilename*/,
	int /*iCallerLine*/,
	Seoul::MemoryBudgets::Type /*eType*/) SEOUL_THROWS(std::bad_alloc)
{
	Seoul::MemoryManager::Deallocate(pAddressToDeallocate);
}

void operator delete[](
	void* pAddressToDeallocate,
	char const* /*sCallerFilename*/,
	int /*iCallerLine*/) SEOUL_THROWS(std::bad_alloc)
{
	Seoul::MemoryManager::Deallocate(pAddressToDeallocate);
}

void operator delete[](
	void* pAddressToDeallocate,
	char const* /*sCallerFilename*/,
	int /*iCallerLine*/,
	Seoul::MemoryBudgets::Type eType) SEOUL_THROWS(std::bad_alloc)
{
	Seoul::MemoryManager::Deallocate(pAddressToDeallocate);
}

void operator delete (void* pAddressToDeallocate) SEOUL_THROW0()
{
	Seoul::MemoryManager::Deallocate(pAddressToDeallocate);
}

void operator delete (
	void* pAddressToDeallocate,
	size_t /*zRequestedAlignment*/) SEOUL_THROW0()
{
	Seoul::MemoryManager::Deallocate(pAddressToDeallocate);
}

void operator delete (void* pAddressToDeallocate, const std::nothrow_t&) SEOUL_THROW0()
{
	Seoul::MemoryManager::Deallocate(pAddressToDeallocate);
}

void operator delete (
	void* pAddressToDeallocate,
	size_t /*zRequestedAlignment*/,
	const std::nothrow_t&) SEOUL_THROW0()
{
	Seoul::MemoryManager::Deallocate(pAddressToDeallocate);
}

void operator delete[] (void* pAddressToDeallocate) SEOUL_THROW0()
{
	Seoul::MemoryManager::Deallocate(pAddressToDeallocate);
}

void operator delete[] (
	void* pAddressToDeallocate,
	size_t /*zRequestedAlignment*/) SEOUL_THROW0()
{
	Seoul::MemoryManager::Deallocate(pAddressToDeallocate);
}

void operator delete[] (void* pAddressToDeallocate, const std::nothrow_t&) SEOUL_THROW0()
{
	Seoul::MemoryManager::Deallocate(pAddressToDeallocate);
}

void operator delete[] (
	void* pAddressToDeallocate,
	size_t /*zRequestedAlignment*/,
	const std::nothrow_t&) SEOUL_THROW0()
{
	Seoul::MemoryManager::Deallocate(pAddressToDeallocate);
}

#endif // /SEOUL_OVERRIDE_GLOBAL_NEW
