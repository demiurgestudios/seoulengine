/**
* \file ScriptVmSettings.h
* \brief Settings used by the script VMs.
*
* Copyright (c) Demiurge Studios, Inc.
* 
* This source code is licensed under the MIT license.
* Full license details can be found in the LICENSE file
* in the root directory of this source tree.
*/

#pragma once
#ifndef SCRIPT_VM_SETTINGS_H
#define SCRIPT_VM_SETTINGS_H

#include "CrashManager.h"
#include "Delegate.h"

namespace Seoul::Script
{

/** Utility structure, fully configures a Script::Vm instance. */
struct VmSettings SEOUL_SEALED
{
	typedef Vector<String, MemoryBudgets::Scripting> BasePaths;

	/** Callback type registered with a scripting VM to capture error events. */
	typedef Delegate<void(const CustomCrashErrorState&)> ErrorHandler;

	/** Callback type registered with a scripting VM to capture output to the script's standard output. */
	typedef Delegate<void(Byte const*)> StandardOutput;

	VmSettings()
		: m_fTargetIncrementalGcTimeInMilliseconds(0.5)
		, m_vBasePaths()
		, m_CustomMemoryAllocatorHook(nullptr)
		, m_ErrorHandler(SEOUL_BIND_DELEGATE(&CrashManager::DefaultErrorHandler))
		, m_StandardOutput()
		, m_uMinGcStepSize(32u)
		, m_uMaxGcStepSize(1024u)
		, m_uInitialGcStepSize(512u)
		, m_pPreCollectionHook(nullptr)
		, m_sVmName("Script")
		, m_bProfilingEnabled(true)
#if SEOUL_ENABLE_DEBUGGER_CLIENT
		, m_bEnableDebuggerHooks(false)
#endif // /SEOUL_ENABLE_DEBUGGER_CLIENT
#if SEOUL_ENABLE_MEMORY_TOOLING
		, m_bEnableMemoryProfiling(false)
#endif // /SEOUL_ENABLE_MEMORY_TOOLING
	{
	}

	/** Target time that the incremental GC step will spend each frame. */
	Double m_fTargetIncrementalGcTimeInMilliseconds;

	/** Convenience function, populates m_vBasePaths with the standard SeoulEngine set. */
	void SetStandardBasePaths();

	/** Path(s) used as the root of all script files to be loaded into the VM, required. */
	BasePaths m_vBasePaths;

	/** (Optional) Currently used for address sanitizer workaround (see ScriptTest.cpp). */
	typedef void* (*CustomMemoryAllocatorHook)(void* pUserData, void* pMemory, size_t zOldSize, size_t zNewSize);
	CustomMemoryAllocatorHook m_CustomMemoryAllocatorHook;

	/** Callback that will be invoked on errors, optional. */
	ErrorHandler m_ErrorHandler;

	/** Callback that will be invoked on print/output, optional. */
	StandardOutput m_StandardOutput;

	/**
	 * Minimum GC step size - for lua, this is how much we want to shave
	 * off the total Lua heap size, in kilobytes. Larger values make the
	 * GC step more aggressive.
	 */
	UInt32 m_uMinGcStepSize;

	/**
	 * Maximum GC step size - for lua, this is how much we want to shave
	 * off the total Lua heap size, in kilobytes. Larger values make the
	 * GC step more aggressive.
	 */
	UInt32 m_uMaxGcStepSize;

	/**
	 * Initial value for the GC step size - this value will be increased
	 * and decreased from the initial to hit the target GC step time.
	 */
	UInt32 m_uInitialGcStepSize;

	/**
	 * (Optional) Low-level hook, use with caution. This "pre-collection" hook
	 * is a Demiurge modification to lua. It is invoked just before a userdata's
	 * entry will be removed from a weak table. It can be used to "rescue" that userdata
	 * from garbage collection.
	 *
	 * An example use case is for script binding wrappers for the UI system
	 * (e.g. ScriptUIMovieClipInstance). We want these only to be garbage collected
	 * once the Falcon UI instance that they bind will also be destroyed on the
	 * garbage collection event (the SharedPtr<> owned by the ScriptUIMovieClipInstance
	 * is the unique owner of the Falcon instance). In this way, we guarantee persistence
	 * lifespan of the script data associated with the instance.
	 *
	 * p is the raw memory block of the user data. uData will be 1-based index into
	 * the reflection registry to identify the type of p (e.g. if uData is 0, the
	 * collection hook should usually return immediately with no further action,
	 * since there is no type information for the raw block). Note that uData will
	 * only be valid if the userdata has a user defined destructor. Perhaps an
	 * unintuitive requirement but again, this is a low-level, specialized hook, exposed
	 * solely for runtime performance reasons).
	 *
	 * This function should return 1 to tell Lua to go ahead and garbage collect the
	 * user data, otherwise it should return 0.
	 */
	typedef Int (*PreCollectionHook)(void* p, UInt32 uData);
	PreCollectionHook m_pPreCollectionHook;

	/** Debug name used to isolate profiling data from this VM. */
	String m_sVmName;

	/** Default to true - enables profiling for the VM. Only enabled in non-Ship builds. */
	Bool m_bProfilingEnabled;

#if SEOUL_ENABLE_DEBUGGER_CLIENT
	/** Runtime control of debugger integration - not enabled in builds that don't support it (Ship), even when true. */
	Bool m_bEnableDebuggerHooks;
#endif // /SEOUL_ENABLE_DEBUGGER_CLIENT

#if SEOUL_ENABLE_MEMORY_TOOLING
	/** Opt-in to memory profiling - has both memory and runtime overhead. */
	Bool m_bEnableMemoryProfiling;
#endif // /SEOUL_ENABLE_MEMORY_TOOLING
}; // struct Script::VmSettings

} // namespace Seoul::Script

#endif // include guard
