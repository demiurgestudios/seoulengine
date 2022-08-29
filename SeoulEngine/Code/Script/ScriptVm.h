/**
 * \file ScriptVm.h
 * \brief SeoulEngine wrapper around a Lua scripting language virtual machine.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_VM_H
#define SCRIPT_VM_H

#include "Atomic32.h"
#include "ContentHandle.h"
#include "Delegate.h"
#include "HashTable.h"
#include "SharedPtr.h"
#include "Mutex.h"
#include "Prereqs.h"
#include "ReflectionMethod.h"
#include "ReflectionWeakAny.h"
#include "ScriptFileBody.h"
#include "ScriptVmHandle.h"
#include "ScriptVmSettings.h"
#include "Settings.h"

// Forward declarations
extern "C"
{
	struct lua_State;
	typedef int (*lua_CFunction) (lua_State *L);
} // extern "C"

namespace Seoul
{

// Forward declarations
namespace Content { struct ChangeEvent; }
struct CustomCrashErrorState;
namespace Script { class Vm; }
namespace Script { class VmObject; }
namespace Reflection { class Type; }

namespace Script
{

/**
 * Wraps a Lua script VM instance.
 *
 * In developer builds, supports hot loading
 * of scripts that have been executed in the
 * virtual machine.
 */
class Vm SEOUL_SEALED
{
public:
	SEOUL_DELEGATE_TARGET(Vm);

	Vm(const VmSettings& VmSettings);
	~Vm();

	/**
	 * Instantiate an instance of type T in Lua and if successful,
	 * output its binding handle.
	 */
	template <typename T>
	Bool BindStrongInstance(SharedPtr<VmObject>& rpBinding)
	{
		void* pUnusedInstance = nullptr;
		return InternalBindStrongInstance(TypeOf<T>(), rpBinding, pUnusedInstance);
	}

	/**
	 * Instantiate an instance of type T in Lua and if successful,
	 * output its data pointer and binding handle.
	 */
	template <typename T>
	Bool BindStrongInstance(
		SharedPtr<VmObject>& rpBinding,
		T*& rpInstance)
	{
		void* pInstance = nullptr;
		if (InternalBindStrongInstance(TypeOf<T>(), rpBinding, pInstance))
		{
			rpInstance = (T*)pInstance;
			return true;
		}

		return false;
	}

	// Register a type that can later be used to fulfill
	// user data instantiation requests to the script environment.
	void BindType(const Reflection::Type& type);

	// Bind a purely native instance into the VM. IMPORTANT: It is the
	// responsibility of the caller/environment to guarantee the lifespan
	// of nativeInstance until Lua has released its reference to it.
	Bool BindWeakInstance(
		const Reflection::WeakAny& nativeInstance,
		SharedPtr<VmObject>& rpBinding);

	// Run a full garbage collection cycle. Typically used
	// at transition points when a hitch is acceptable, or
	// on shutdown.
	void GcFull();

	/**
	 * @return Indirect handle reference to this Script::Vm.
	 */
	const VmHandle& GetHandle() const
	{
		return m_hThis;
	}

	/** @return The settings used to configure this VM. */
	const VmSettings& GetSettings() const
	{
		return m_Settings;
	}

	// Construct a Lua table from dataStore at tableNode and
	// bind as a strong instance, assigned to rpBinding.
	Bool BindStrongTable(
		SharedPtr<VmObject>& rpBinding,
		const DataStore& dataStore,
		const DataNode& tableNode);

	// Convenience, return true if a global member is not nil.
	Bool HasGlobal(HString name) const;

	// Execute inline script code inside this Lua VM.
	Bool RunCode(const String& sCode);

	// Execute a script inside this Lua VM. Typically used to run the application's main script.
	Bool RunScript(const String& sRelativeFilename, Bool bAddToHotLoadSet = true);

	// Run one incremental step of the VM's garbage collector.
	void StepGarbageCollector();

	// Attempt to set rpObject to an existing object in the global table.
	Bool TryGetGlobal(HString name, SharedPtr<VmObject>& rpObject);

	// Attempt to set a Script::VmObject to the global table.
	Bool TrySetGlobal(HString name, const SharedPtr<VmObject>& pObject);

	// Init trackers - expected to be called from script to track initialization progress.
	void InitGetProgress(Atomic32Type& rTotalSteps, Atomic32Type& rProgress) const
	{
		rTotalSteps = m_InitTotalSteps;
		rProgress = m_InitProgress;
	}
	void InitIncreaseProgressTotal(Atomic32Type total)
	{
		m_InitTotalSteps += total;
	}
	void InitOnProgress()
	{
		++m_InitProgress;
	}

	// Utility - public for script hook utilities, not meant to be used by client code.
	Bool ResolveFilePathFromRelativeFilename(const String& sRelativeFilename, FilePath& rFilePath) const;

#if SEOUL_HOT_LOADING
	struct HotLoadData
	{
		HotLoadData()
			: m_tData()
			, m_tScripts()
			, m_tDataToMonitor()
			, m_tScriptsToMonitor()
			, m_bRegisteredForHotLoading(false)
			, m_ScriptProjectLoadCount()
			, m_bOutOfDate(false)
		{
		}

		typedef HashTable<FilePath, Bool, MemoryBudgets::Scripting> Data;
		Data m_tData;
		Data m_tGeneral;
		typedef HashTable<FilePath, Bool, MemoryBudgets::Scripting> Scripts;
		Scripts m_tScripts;
		typedef HashTable<FilePath, SettingsContentHandle, MemoryBudgets::Scripting> DataToMonitor;
		DataToMonitor m_tDataToMonitor;
		typedef HashTable<FilePath, Content::Handle<FileBody>, MemoryBudgets::Scripting> ScriptsToMonitor;
		ScriptsToMonitor m_tScriptsToMonitor;
		Bool m_bRegisteredForHotLoading;
		Atomic32Type m_ScriptProjectLoadCount;
		Atomic32Value<Bool> m_bOutOfDate;
	}; // struct HotLoadData

	/**
	 * Declare a FilePath as a dependency of the VM. A file change event to this
	 * dependency will trigger a hot reload of the VM.
	 */
	void AddDataDependency(FilePath filePath)
	{
		(void)m_HotLoadData.m_tData.Insert(filePath, true);
	}

	/**
	* Declare a FilePath as a dependency of the VM. A file change event to this
	* dependency will trigger a hot reload of the VM.
	*/
	void AddGeneralDependency(FilePath filePath)
	{
		(void)m_HotLoadData.m_tGeneral.Insert(filePath, true);
	}

	/**
	 * Unset the out-of-date flag. Used for hot loading management.
	 */
	void ClearOutOfDate()
	{
		m_HotLoadData.m_bOutOfDate = false;
	}

	/**
	 * @return True if this VM is out-of-date with files on disk,
	 * false otherwise.
	 */
	Bool IsOutOfDate() const
	{
		return m_HotLoadData.m_bOutOfDate;
	}

	// Call to register this Script::Vm for hot loading. Unregister
	// must be called before this Vm is destroyed. Both methods must be called
	// on the main thread.
	void RegisterForHotLoading();

	// Special fixed script hook - called when about to
	// do a hot load so the script can clean things up.
	Bool TryInvokeGlobalOnHotload();

	// Special fixed script hook - called after a hotload
	// is complete so the new VM can set things up.
	Bool TryInvokeGlobalPostHotload();

	// Special fixed script hook - to push saved off DynamicGameStateData
	// back into the script.
	Bool TryInvokeGlobalRestoreDynamicGameStateData();

	// Call to unregister this Script::Vm from hot loading.
	void UnregisterFromHotLoading();
#endif // /#if SEOUL_HOT_LOADING

#if !SEOUL_ASSERTIONS_DISABLED
	// Debugging only hook used to avoid certain checking
	// when a VM is in its destroyed body.
	static Bool DebugIsInVmDestroy() { return (0 != s_InVmDestroy); }
#endif // /#if !SEOUL_ASSERTIONS_DISABLED

	/** @return true if shutdown interrupt has been raised for this VM. */
	Bool Interrupted() const
	{
		return m_bInterrupted;
	}

	/**
	 * Interrupt for long running script functions (e.g. startup).
	 * sets a flag that will raise an error when the script next
	 * interacts with native (does not guarantee an interrupt
	 * if the script remains fully script code).
	 *
	 * Once interrupted, a VM must be shutdown.
	 */
	void RaiseInterrupt()
	{
		m_bInterrupted = true;
	}

#if SEOUL_ENABLE_MEMORY_TOOLING
	/**
	 * When enabled, call the given delegate with memory profiling data.
	 * Entries will be reported in largest-to-smallest order. 0 sized
	 * entries will not be reported.
	 */
	typedef Delegate<void(Byte const* sName, ptrdiff_t iSize, int iStartLine)> MemoryCallback;
	void QueryMemoryProfilingData(const MemoryCallback& callback);
#endif // /SEOUL_ENABLE_MEMORY_TOOLING

private:
	SEOUL_REFERENCE_COUNTED(Vm);

#if !SEOUL_ASSERTIONS_DISABLED
	static Atomic32 s_InVmDestroy;
#endif // /#if !SEOUL_ASSERTIONS_DISABLED

	void InsideLockBindBuiltins();
	void InsideLockBindType(const Reflection::Type& type, Bool bWeak);
	void InsideLockCreateVM();
#if SEOUL_ENABLE_DEBUGGER_CLIENT
	void InsideLockSetDebuggerHooks();
#endif // /SEOUL_ENABLE_DEBUGGER_CLIENT
	void InsideLockDestroyVM();
	Bool InsideLockRunCode(const String& sCode);
	Bool InsideLockRunScript(const String& sRelativeFilename, Bool bAddToHotLoadSet = true);

	Bool InternalBindStrongInstance(
		const Reflection::Type& type,
		SharedPtr<VmObject>& rpBinding,
		void*& rpInstance);

	friend class FunctionInterface;
	friend class FunctionInvoker;
	friend class VmObject;
	VmSettings const m_Settings;
	VmHandle m_hThis;
	CheckedPtr<lua_State> m_pLuaVm;
	lua_CFunction m_pDefaultAtPanic;
	Mutex m_Mutex;
	UInt32 m_uGcStepSize;
	Atomic32 m_InitProgress;
	Atomic32 m_InitTotalSteps;
	Atomic32Value<Bool> m_bInterrupted;

#if SEOUL_ENABLE_MEMORY_TOOLING
	typedef HashTable<void*, ptrdiff_t, MemoryBudgets::Scripting> MemoryTracking;
	static void* LuaMemoryAllocWithTooling(void* ud, void* ptr, size_t osize, size_t nsize);
	MemoryTracking m_tMemory;
#endif // /SEOUL_ENABLE_MEMORY_TOOLING
	static void* LuaMemoryAlloc(void* ud, void* ptr, size_t osize, size_t nsize);

#if SEOUL_HOT_LOADING
	HotLoadData m_HotLoadData;
	Bool OnFileChange(Content::ChangeEvent* pFileChangeEvent);
	Bool OnIsFileLoaded(FilePath filePath);
	Bool OnFileLoadComplete(FilePath filePath);
#endif // /#if SEOUL_HOT_LOADING

	SEOUL_DISABLE_COPY(Vm);
}; // class Script::Vm

/**
 * Wraps a Lua instance and creates a hard
 * reference to it. Prevents it from garbage
 * collection and allows invocation and other
 * operations on it.
 */
class VmObject SEOUL_SEALED
{
public:
	VmObject(const VmHandle& hVm, Int32 iObject);
	~VmObject();

	/**
	 * @return The owner VM of this Script::VmObject.
	 *
	 * May be invalid, Script::VmObject is a weak
	 * reference to its owner Vm.
	 */
	const VmHandle& GetVm() const
	{
		return m_hVm;
	}

	/** @return True if the ref of this Vm Object is currently nil. */
	Bool IsNil() const
	{
		return (-1 == m_iRef);
	}

	// Push onto the VM stack the referenced script object. Will push
	// nil if invalid.
	void PushOntoVmStack(lua_State* pVm);

	// Sets the reference state of this VM object to nil.
	void ReleaseRef();

	// Used for management of Script::VmObject's created with
	// Script::Vm::BindWeakInstance(). Calling this method sets
	// the internal binding to nil, which is useful if the
	// bound native instance is destroyed prior to Lua's ownership
	// of the binding.
	void SetWeakBindingToNil();

	// If a compatible type, serializes the script object
	// to rDataStore.
	Bool TryToDataStore(DataStore& rDataStore) const;

private:
	SEOUL_REFERENCE_COUNTED(VmObject);

	VmHandle m_hVm;
	Int32 m_iRef;
	void InternalGetRef(lua_State* pVm) const;
	void InternalRef(lua_State* pVm);
	void InternalUnref(lua_State* pVm);

	SEOUL_DISABLE_COPY(VmObject);
}; // class Script::VmObject

} // namespace Script

template <>
struct DataNodeHandler< SharedPtr<Script::VmObject>, false>
{
	static const Bool Value = true;
	static Bool FromDataNode(Reflection::SerializeContext&, const DataStore& dataStore, const DataNode& dataNode, SharedPtr<Script::VmObject>& rp);
	static Bool ToArray(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, const SharedPtr<Script::VmObject>& p);
	static Bool ToTable(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& table, HString key, const SharedPtr<Script::VmObject>& p);
	static void FromScript(lua_State* pVm, Int iOffset, SharedPtr<Script::VmObject>& rp);
	static void ToScript(lua_State* pVm, const SharedPtr<Script::VmObject>& p);
}; // struct DataNodeHandler< SharedPtr<Script::VmObject>, false>

namespace Script
{

/**
 * Utility wrapper, allows raw strings or arbitrary byte buffers
 * to be passed to script as string data without additional copying. Data
 * will only be copied and wrapped into a script string before being
 * passed to the script VM.
 */
struct ByteBuffer SEOUL_SEALED
{
	ByteBuffer()
		: m_pData(nullptr)
		, m_zDataSizeInBytes(0u)
	{
	}

	void* m_pData;
	UInt32 m_zDataSizeInBytes;
}; // struct Script::ByteBuffer

} // namespace Script

template <>
struct DataNodeHandler< Script::ByteBuffer, false>
{
	static const Bool Value = true;
	static Bool FromDataNode(Reflection::SerializeContext&, const DataStore& dataStore, const DataNode& dataNode, Script::ByteBuffer& rv);
	static Bool ToArray(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, const Script::ByteBuffer& v);
	static Bool ToTable(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& table, HString key, const Script::ByteBuffer& v);
	static void FromScript(lua_State* pVm, Int iOffset, Script::ByteBuffer& rv);
	static void ToScript(lua_State* pVm, const Script::ByteBuffer& v);
}; // struct DataNodeHandler< Script::ByteBuffer, false>

} // namespace Seoul

#endif // include guard
