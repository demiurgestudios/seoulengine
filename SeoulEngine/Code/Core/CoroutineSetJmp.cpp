/**
 * \file CoroutineSetJmp.cpp
 * \brief A Coroutine is a concept that allows for the implementation of
 * cooperative multi-tasking - switching context between 2 coroutines stores
 * the current stack and replaces it with a different stack.
 *
 * CoroutineSetJmp implements coroutines by using _setjmp() and modifying
 * the jmp_buf structure (in a hacky, non platform independent way) to
 * point the stack at the heap allocated area allocated for each coroutine.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Coroutine.h"
#include "FixedArray.h"
#include "PerThreadStorage.h"
#include "SeoulMath.h"

// SetJmp API is not used on Windows.
#if !SEOUL_PLATFORM_WINDOWS

// TODO: Address sanitizer hinting of
// fiber state is only supported on Linux
// right now.
#if SEOUL_ADDRESS_SANITIZER && SEOUL_PLATFORM_LINUX
#define SEOUL_ASAN_FIBER_HINT 1
#else
#define SEOUL_ASAN_FIBER_HINT 0
#endif

// When address sanitizer hinting is enabled, we need
// to hint it about the stack switch to avoid spurious
// errors. This functionality is incredibly new,
// so we need to declare these functions for ourself.
#if SEOUL_ASAN_FIBER_HINT
extern "C"
{

void __sanitizer_start_switch_fiber(void** ppStackSave, void const* pBottom, size_t zSize);
void __sanitizer_finish_switch_fiber(void* pStackSave, void const** ppBottomOld, size_t* pzSizeOld);

} // extern "C"

#endif // /SEOUL_ASAN_FIBER_HINT

#include <setjmp.h>
#include <stddef.h>

#if SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_LINUX
#	include <sys/mman.h>
#	include <unistd.h>
#endif

// Platform dependent macros used to define Coroutine behavior.
#if SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_LINUX
#	define SEOUL_LONGJMP _longjmp
#	define SEOUL_SETJMP _setjmp
#	define SEOUL_JMP_BUF jmp_buf
#	define SEOUL_STACK_POINTER(p, size, offset) ((((Byte*)(p)) + (size_t)(size)) - (size_t)(offset))
#else
#	error "Define for this platform."
#endif

// Architecture dependent macros that handle saving and restoring
// the stack pointer.
#if defined(__i386__)
#	define SEOUL_GET_SP(p) asm volatile("mov %%esp, %0" : "=r"(p))
#	define SEOUL_SET_SP(p) asm volatile("mov %0, %%esp" : : "r"(p))
#elif defined(__x86_64__)
#	define SEOUL_GET_SP(p) asm volatile("movq %%rsp, %0" : "=r"(p))
#	define SEOUL_SET_SP(p) asm volatile("movq %0, %%rsp" : : "r"(p))
#elif defined(__arm64__) || defined(__aarch64__)
#	define SEOUL_GET_SP(p) asm volatile("mov %0, sp" : "=r"(p))
#	define SEOUL_SET_SP(p) asm volatile("mov sp, %0" : : "r"(p))
#elif defined(__arm__)
#	define SEOUL_GET_SP(p) asm volatile("mov %0, sp" : "=r"(p))
#	define SEOUL_SET_SP(p) asm volatile("mov sp, %0" : : "r"(p))
#else
#	error "Define for this platform."
#endif

namespace Seoul
{

// Sanity checks.
SEOUL_STATIC_ASSERT(0u == (sizeof(SEOUL_JMP_BUF) % sizeof(UInt32)));
SEOUL_STATIC_ASSERT(sizeof(Byte const*) == sizeof(size_t));

/**
 * Represents a single Coroutine.
 */
struct Coroutine
{
	SEOUL_JMP_BUF m_JumpBuffer;
	CoroutineEntryPoint m_pEntryPoint;
	Byte* m_pStack;
	size_t m_zStackSizeInBytes;
	void* m_pUserData;
	void* m_pOriginalStackPointer;
};

/**
 * Global data used by the Coroutine system that must be stored per-thread.
 */
struct CoroutinePerThreadData
{
	CoroutinePerThreadData()
		: m_ThreadCoroutine()
		, m_pCurrentCoroutine(nullptr)
#if SEOUL_ASAN_FIBER_HINT
		, m_pThreadStack(nullptr)
		, m_zThreadStack(0u)
#endif // /#if SEOUL_ASAN_FIBER_HINT
	{
		memset(&m_ThreadCoroutine, 0, sizeof(m_ThreadCoroutine));

		// TODO: This should not be necessary - instead,
		// we should be using the data from
		// __sanitizer_finish_switch_fiber, but this it not working.
		//
		// See comment in EndSwitch.

		// TODO: If we leave this, wrap as a general implementation
		// as a static function of the Thread class.

		// When hinting is enabled, capture the thread stack attributes.
#if SEOUL_ASAN_FIBER_HINT
		pthread_attr_t attributes;
		memset(&attributes, 0, sizeof(attributes));
		SEOUL_VERIFY_NO_LOG(0 == pthread_attr_init(&attributes));
		SEOUL_VERIFY_NO_LOG(0 == pthread_getattr_np(pthread_self(), &attributes));
		SEOUL_VERIFY_NO_LOG(0 == pthread_attr_getstack(&attributes, (void**)&(m_pThreadStack), (size_t*)&(m_zThreadStack)));
		SEOUL_VERIFY_NO_LOG(0 == pthread_attr_destroy(&attributes));
#endif // /#if SEOUL_ASAN_FIBER_HINT
	}

	Coroutine m_ThreadCoroutine;
	volatile Coroutine* m_pCurrentCoroutine;
#if SEOUL_ASAN_FIBER_HINT
	volatile void* m_pThreadStack;
	volatile size_t m_zThreadStack;
#endif // /#if SEOUL_ASAN_FIBER_HINT
};

/** Global per-thread data used by the Coroutine system. */
static PerThreadStorage s_PerThreadCoroutineData;

// When address sanitizer hinting is enabled, we need to track coroutine start and end. */
#if SEOUL_ASAN_FIBER_HINT

/**
 * Wrapped in SEOUL_SWITCH_START(). Invoked carefully to
 * mark entry into a new coroutine for address sanitizer.
 */
static SEOUL_NO_INLINE void StartSwitch(Coroutine volatile* pCoroutine)
{
	// Get the per thread coroutine data.
	CoroutinePerThreadData* pData = (CoroutinePerThreadData*)s_PerThreadCoroutineData.GetPerThreadStorage();
	SEOUL_ASSERT(nullptr != pData);

	// Stack values for tracking.
	void* pStack = nullptr;
	size_t zStack = 0u;

	// Gather - if we're switching to a thread coroutine,
	// use the previously acquired thread stack.
	if (nullptr == pCoroutine->m_pStack)
	{
		// Sanity check that we've captured the thread stack.
		SEOUL_ASSERT(nullptr != pData->m_pThreadStack);
		SEOUL_ASSERT(0u != pData->m_zThreadStack);

		// Assign.
		pStack = (void*)pData->m_pThreadStack;
		zStack = (size_t)pData->m_zThreadStack;
	}
	// Otherwise, use the coroutine's stack.
	else
	{
		pStack = (void*)pCoroutine->m_pStack;
		zStack = (size_t)pCoroutine->m_zStackSizeInBytes;
	}

	// TODO: Track stack save.

	__sanitizer_start_switch_fiber(nullptr, pStack, zStack);
}

/**
 * Wrapped in SEOUL_SWITCH_End(). Invoked carefully to
 * mark exit from a coroutine for address sanitizer.
 */
static SEOUL_NO_INLINE void EndSwitch()
{
	// TODO: Track stack save.

	// TODO: Examples on the Internet imply that we're supposed to use
	// __sanitizer_finish_switch_fiber to get the thread's stack size and pointer.
	// However, for some reason this function is never populating the out values
	// (they are always null/0u/left unmodified). Might just be making an API
	// mistake or it may be a bug in clang 3.9's implementation of these
	// API.

	__sanitizer_finish_switch_fiber(nullptr, nullptr, nullptr);
}

#define SEOUL_SWITCH_START(...) StartSwitch(__VA_ARGS__)
#define SEOUL_SWITCH_END(...) EndSwitch(__VA_ARGS__)

#else // !SEOUL_ASAN_FIBER_HINT

#define SEOUL_SWITCH_START(...) ((void)0)
#define SEOUL_SWITCH_END(...) ((void)0)

#endif // /!SEOUL_ASAN_FIBER_HINT

/** Platform dependent allocation of stack memory, uses virtual memory allocations. */
static void* AllocateStack(size_t zCommitSize, size_t& rzReservedSize)
{
#if SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_LINUX
	// Get configuration for the current platform.
	size_t const zPageSize = (size_t)getpagesize();
	size_t const zGuardSize = zPageSize;
	size_t const zGuardSize2x = 2 * zGuardSize;

	// Adjust the stack size to the page size.
	rzReservedSize = RoundUpToAlignment(rzReservedSize, zPageSize);

	// Compute the total allocation size (requested plus guard area).
	size_t const zAllocSize = rzReservedSize + zGuardSize2x;

	// Allocate the block, then set the first and last pages to no access, so that stack overflows
	// will trigger a SEGV_ACCERR (access error).
#if SEOUL_PLATFORM_IOS
	Byte* pMemory = (Byte*)mmap(nullptr, zAllocSize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_NORESERVE | MAP_PRIVATE, -1, 0);
#else
	Byte* pMemory = (Byte*)mmap(nullptr, zAllocSize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_NORESERVE | MAP_PRIVATE | MAP_STACK, -1, 0);
#endif
	SEOUL_VERIFY(0 == mprotect(pMemory, zGuardSize, PROT_NONE));
	SEOUL_VERIFY(0 == mprotect(pMemory + zGuardSize + rzReservedSize, zGuardSize, PROT_NONE));

	// If commit size is greater than zero, perform the commit request.
	if (zCommitSize > 0u)
	{
		// TODO: Unclear - some Android devices return EBADF for madvise(..., MADV_WILLNEED),
		// so that apparently is only consistently applicable to memory mapped files, not anonymous
		// allocations. So we just use a memset here to make the memory area resident.
		memset(pMemory + zGuardSize, 0, zCommitSize);
	}

	// Done, stack is at the memory address + the guard size.
	return (pMemory + zGuardSize);
#else
#	error "Define for this platform."
#endif
}

/** Platform dependent allocation of stack memory, uses virtual memory allocations. */
static void DeallocateStack(void* pStack, size_t zStackSizeInBytes)
{
#if SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_LINUX
	size_t const zPageSize = (size_t)getpagesize();
	size_t const zGuardSize = zPageSize;
	size_t const zGuardSize2x = 2 * zGuardSize;

	// Find the memory head - this is one guard page length prior to
	// the passed in pointer.
	Byte* pMemory = (Byte*)pStack - zGuardSize;

	// Release the block.
	SEOUL_VERIFY(0 == munmap(pMemory, zGuardSize2x + zStackSizeInBytes));
#else
#	error "Define for this platform."
#endif
}

/**
 * Platform dependent decommit of a region of previously reserved stack memory.
 */
static void DecommitStackRegion(
	Byte* pStack,
	size_t zStackSize,
	size_t zSizeToLeaveCommitted)
{
#if SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_LINUX
	size_t const zPageSize = (size_t)getpagesize();

	// Sanity check.
	SEOUL_ASSERT(zStackSize % zPageSize == 0);

	// Round up zSizeToLeaveCommitted to at least a page.
	zSizeToLeaveCommitted = RoundUpToAlignment(zSizeToLeaveCommitted, zPageSize);

	// Early out if the entire stack will now be left committed.
	if (zSizeToLeaveCommitted >= zStackSize)
	{
		return;
	}

	// Get the pointer to the start and end of the stack region we want to decommit.
	Byte* pDecommitBegin = SEOUL_STACK_POINTER(pStack, zStackSize, zSizeToLeaveCommitted);
	Byte* pDecommitEnd = SEOUL_STACK_POINTER(pStack, zStackSize, zStackSize);

	// Adjust - on many platforms, stacks grow toward lower memory addresses, so begin
	// may be greater than end.
	if (pDecommitBegin > pDecommitEnd) { Swap(pDecommitBegin, pDecommitEnd); }

	// Finally, decommit the pages.
	SEOUL_VERIFY(0 == madvise(pDecommitBegin, (size_t)(pDecommitEnd - pDecommitBegin), MADV_DONTNEED));
#else
#	error "Define for this platform."
#endif
}

/**
 * Invoke the current coroutine's entry point, used by InternalStaticInvokeCoroutine(),
 * necessary to wrap s_PerThreadCoroutineData acceses across longjmp contexts.
 */
static SEOUL_NO_RETURN void InternalStaticInvokeCurrent()
{
	// Get the per thread coroutine data.
	CoroutinePerThreadData* pData = (CoroutinePerThreadData*)s_PerThreadCoroutineData.GetPerThreadStorage();
	SEOUL_ASSERT(nullptr != pData);

	Coroutine* pToInvoke = (Coroutine*)(pData->m_pCurrentCoroutine);
	SEOUL_ASSERT(nullptr != pToInvoke);
	pToInvoke->m_pEntryPoint(pToInvoke->m_pUserData);

	// The entry point should never return
	SEOUL_FAIL("A Coroutine entry point should never return.");

#if SEOUL_ASSERTIONS_DISABLED
	// Trigger a break explicitly even in Ship.
	SEOUL_DEBUG_BREAK();
#endif // /#if SEOUL_ASSERTIONS_DISABLED
}

/**
 * Called at the end of CreateCoroutine() to configure a coroutine setjmp.
 *
 * Behavior:
 * - grabs sp (the stack pointer) and stores it in p.
 * - copies a measured size block of the stack, which is big enough to
 *   include p.
 * - sets sp to a new, heap allocated stack pointer.
 * - calls setjmp to capture a jmp_buf with the new, heap allocated stack.
 * - on the return branch from setjmp, restores the original stack.
 * - on the invoke branch from setjmp, uses the heap allocated
 *   stack, which never returns.
 *
 * NOTE: We must disable AddressSanitizer for this function, as
 * it doesn't interact nicely with our low-level assembly manipulations
 * of the stack
 */
static SEOUL_DISABLE_ADDRESS_SANITIZER void InternalStaticInvokeCoroutine(Coroutine* p, void* pFrameReference)
{
	// WARNING: After SEOUL_SET_SP(), and before the
	// body of the set jmp, the previous stack
	// is no longer the stack. It has been replaced with
	// our heap allocated stack. This is why we copy
	// the old stack to new, but care must still be taken
	// when updating the body of this function.

	// First, backup the current stack pointer into temp storage in p.
	SEOUL_GET_SP(p->m_pOriginalStackPointer);

	// Measure initial frame size.
	size_t const zFrameSizeInBytes = (size_t)Abs((Int64)((ptrdiff_t)pFrameReference - (ptrdiff_t)p->m_pOriginalStackPointer));

	// Sanity checks of the computed frame size - must be:
	// - at least the size of a pointer.
	// - no bigger than our reasonable maximum - our worst case
	//   (64-bit debug build with address sanitizer enabled)
	//   should be well below 512 bytes (expected ~300 bytes).
	// - less than the total stack size.
	// - is a multiple of a pointer size.
	SEOUL_ASSERT(zFrameSizeInBytes >= sizeof(Coroutine*));
	SEOUL_ASSERT(zFrameSizeInBytes <= 512);
	SEOUL_ASSERT(zFrameSizeInBytes <= p->m_zStackSizeInBytes);
	SEOUL_ASSERT(zFrameSizeInBytes % sizeof(void*) == 0);

	// Copy frame from old stack into heap allocated stack.
	memcpy(SEOUL_STACK_POINTER(p->m_pStack, p->m_zStackSizeInBytes, zFrameSizeInBytes), p->m_pOriginalStackPointer, zFrameSizeInBytes);

	// Now set the stack pointer register to use the new stack area.
	SEOUL_SET_SP(SEOUL_STACK_POINTER(p->m_pStack, p->m_zStackSizeInBytes, zFrameSizeInBytes));

	// Perform the setjmp to capture context with the dynamic stack - a value
	// of 0 indicates a set, while a non-zero value indicates
	// arriving at the context via a longjmp, which means we want to fall through
	// and invoke the coroutine's function.
	if (0 == SEOUL_SETJMP(p->m_JumpBuffer))
	{
		// Restore the original stack pointer from the temp storage
		// and return.
		SEOUL_SET_SP(p->m_pOriginalStackPointer);
		return;
	}

	// Tell address sanitizer about post jump.
	SEOUL_SWITCH_END();

	// If we get here, it means the coroutine has been invoked (arrived at
	// via a longjmp). This function must never return, so we're done at
	// this point.
	InternalStaticInvokeCurrent();

	// See the fail at the end of InternalStaticInvokeCurrent() - it is invalid
	// to return from this point, as the stack pointer references the new heap,
	// while the frame pointer will be the original frame pointer of the coroutine
	// creation. As a result, returning from this function will return to the
	// stack context of the original capture, and insanity will ensue.
}

/**
 * @return A Coroutine for the current Thread - this function must be called
 * before calling SwitchToCoroutine() on a Coroutine created with CreateCoroutine().
 *
 * \warning Do NOT delete the return value from this function with DeleteCoroutine().
 */
UnsafeHandle ConvertThreadToCoroutine(void* pUserData /* = nullptr */)
{
	// Sanity check.
	SEOUL_ASSERT(nullptr == s_PerThreadCoroutineData.GetPerThreadStorage());

	// Instantiate a per-thread data object.
	CoroutinePerThreadData* pData = SEOUL_NEW(MemoryBudgets::Coroutines) CoroutinePerThreadData;

	// Initialize its internal coroutine member.
	pData->m_ThreadCoroutine.m_pUserData = pUserData;

	// Set the initial current coroutine to the thread coroutine.
	pData->m_pCurrentCoroutine = &(pData->m_ThreadCoroutine);

	// Set the new per-thread object to per-thread storage.
	s_PerThreadCoroutineData.SetPerThreadStorage(pData);

	// Return the result.
	return pData->m_pCurrentCoroutine;
}

/**
 * Convert the coroutine of the current thread back to a standard thread. This
 * is the "delete" function for a corresponding call to ConvertThreadToCoroutine().
 */
void ConvertCoroutineToThread()
{
	// Get the local variable and unset the per-thread data.
	CoroutinePerThreadData* pData = (CoroutinePerThreadData*)s_PerThreadCoroutineData.GetPerThreadStorage();
	SEOUL_ASSERT(nullptr != pData);
	s_PerThreadCoroutineData.SetPerThreadStorage(nullptr);

	// Sanity checks.
	SEOUL_ASSERT(
		nullptr != pData->m_pCurrentCoroutine &&
		&(pData->m_ThreadCoroutine) == pData->m_pCurrentCoroutine);

	// Free the data.
	SafeDelete(pData);
}

/**
 * @return A new Coroutine with main function pEntryPoint and stack size zStackSize.
 *
 * pCoroutineData will be passed to the main function when the coroutine is invoked, and can
 * also be accessed through GetCoroutineData() when the Coroutine is the currently active
 * Coroutine.
 */
UnsafeHandle CreateCoroutine(
	size_t zStackCommitSize,
	size_t zStackReservedSize,
	CoroutineEntryPoint pEntryPoint,
	void* pUserData)
{
	// Sanity check.
	SEOUL_ASSERT(zStackCommitSize <= zStackReservedSize);

	// Allocate and initialize the Coroutine members.
	Coroutine* p = (Coroutine*)MemoryManager::Allocate(sizeof(Coroutine), MemoryBudgets::Coroutines);
	memset(p, 0, sizeof(*p));

	// Set initial stack size - may be adjusted by AllocateStack().
	p->m_zStackSizeInBytes = zStackReservedSize;

	// Initialize the stack - stack reserved size may be adjusted for
	// alignment by AllocateStack.
	p->m_pStack = (Byte*)AllocateStack(zStackCommitSize, p->m_zStackSizeInBytes);

	// Setup remaining Coroutine members.
	p->m_pEntryPoint = pEntryPoint;
	p->m_pUserData = pUserData;

	// Capture the current sp into pFrameReference. This
	// is used by InternalStaticInvokeCoroutine() to compute
	// the entire frame size of InternalStaticInvokeCoroutine()
	// for copy into the initial frame of the heap allocated
	// stack.
	void* pFrameReference = nullptr;
	SEOUL_GET_SP(pFrameReference);

	// This function sets up the Coroutine's SEOUL_JMP_BUF to use the heap allocated stack, and
	// also sets up the necessary hooks so that SwitchToCoroutine() behaves as expected.
	InternalStaticInvokeCoroutine(p, pFrameReference);

	// Return the coroutine object.
	return (Coroutine volatile*)p;
}

/**
 * Cleanup the memory associated with rhCoroutine.
 *
 * \warning rhCoroutine must NOT be the currently active Coroutine on
 * any thread.
 *
 * \warning rhCoroutine must NOT be a thread Coroutine, created with
 * ConvertThreadToCoroutine().
 */
void DeleteCoroutine(UnsafeHandle& rhCoroutine)
{
	// If Coroutine is nullptr, nothing to do.
	if (!rhCoroutine.IsValid())
	{
		return;
	}

	// Get and reset the handle.
	Coroutine volatile* p = StaticCast<Coroutine volatile*>(rhCoroutine);
	rhCoroutine.Reset();

	// This will only be nullptr if rhCoroutine is a thread Coroutine.
	SEOUL_ASSERT(nullptr != p->m_pStack);

	// Cannot delete the currently active Coroutine.
	SEOUL_ASSERT(((CoroutinePerThreadData*)s_PerThreadCoroutineData.GetPerThreadStorage())->m_pCurrentCoroutine != p);

	// Deallocate the stack area and the coroutine memory.
	DeallocateStack(p->m_pStack, p->m_zStackSizeInBytes);
	MemoryManager::Deallocate((Coroutine*)p);
}

/**
 * Switch the execution context from currently active Coroutine to
 * hCoroutine.
 */
void SwitchToCoroutine(UnsafeHandle hCoroutine)
{
	// Get the per thread coroutine data.
	CoroutinePerThreadData* pData = (CoroutinePerThreadData*)s_PerThreadCoroutineData.GetPerThreadStorage();
	SEOUL_ASSERT(nullptr != pData);

	// Get a pointer to the coroutine object.
	Coroutine volatile* pCoroutine = StaticCast<Coroutine volatile*>(hCoroutine);

	// Sanity checks.
	if (nullptr != pCoroutine &&
		nullptr != pData->m_pCurrentCoroutine &&
		pCoroutine != pData->m_pCurrentCoroutine)
	{
		// A 0 value indicates that we've just called the _setjmp and we should perform
		// the jump. Otherwise, it will be != 0, which means we've jumped back, so
		// we should return from SwitchToCoroutine.
		if (0 == SEOUL_SETJMP(((Coroutine*)(pData->m_pCurrentCoroutine))->m_JumpBuffer))
		{
			// Tell address sanitizer about pre jump.
			SEOUL_SWITCH_START(pCoroutine);

			// Cache the coroutine we're about to jump to.
			pData->m_pCurrentCoroutine = pCoroutine;

			// Jump to the new coroutine context.
			SEOUL_LONGJMP(((Coroutine*)(pData->m_pCurrentCoroutine))->m_JumpBuffer, 1);
		}

		// Tell address sanitizer about post jump.
		SEOUL_SWITCH_END();
	}
}

/**
 * @return A UnsafeHandle to the currently active Coroutine, or an Invalid() UnsafeHandle
 * if there is no currently active Coroutine.
 */
UnsafeHandle GetCurrentCoroutine()
{
	// Get the per thread coroutine data.
	CoroutinePerThreadData* pData = (CoroutinePerThreadData*)s_PerThreadCoroutineData.GetPerThreadStorage();
	SEOUL_ASSERT(nullptr != pData);

	return pData->m_pCurrentCoroutine;
}

/**
 * @return The userdata associated with the currently active Coroutine, or nullptr
 * if there is no currently active Coroutine.
 */
void* GetCoroutineUserData()
{
	// Get the per thread coroutine data.
	CoroutinePerThreadData* pData = (CoroutinePerThreadData*)s_PerThreadCoroutineData.GetPerThreadStorage();
	SEOUL_ASSERT(nullptr != pData);

	if (nullptr != pData->m_pCurrentCoroutine)
	{
		return pData->m_pCurrentCoroutine->m_pUserData;
	}

	return nullptr;
}

Bool IsInOriginCoroutine()
{
	if (nullptr == s_PerThreadCoroutineData.GetPerThreadStorage())
	{
		return true;
	}

	// Get the per thread coroutine data.
	CoroutinePerThreadData* pData = (CoroutinePerThreadData*)s_PerThreadCoroutineData.GetPerThreadStorage();
	SEOUL_ASSERT(nullptr != pData);

	return (&(pData->m_ThreadCoroutine) == pData->m_pCurrentCoroutine);
}

void PartialDecommitCoroutineStack(
	UnsafeHandle hCoroutine,
	size_t zStackSizeToLeaveCommitted)
{
	// Get the per thread coroutine data.
	CoroutinePerThreadData* pData = (CoroutinePerThreadData*)s_PerThreadCoroutineData.GetPerThreadStorage();
	SEOUL_ASSERT(nullptr != pData);

	// Get a pointer to the coroutine object.
	Coroutine volatile* pCoroutine = StaticCast<Coroutine volatile*>(hCoroutine);

	// Sanity checks - coroutine must be valid and cannot be equal to the current
	// coroutine. The zStackSizeToLeaveCommited must also be < the entire stack
	// size or this function is a nop.
	if (nullptr != pCoroutine &&
		nullptr != pData->m_pCurrentCoroutine &&
		pCoroutine != pData->m_pCurrentCoroutine &&
		zStackSizeToLeaveCommitted < pCoroutine->m_zStackSizeInBytes)
	{
		DecommitStackRegion(
			pCoroutine->m_pStack,
			pCoroutine->m_zStackSizeInBytes,
			zStackSizeToLeaveCommitted);
	}
}

} // namespace Seoul

#endif // /!SEOUL_PLATFORM_WINDOWS
