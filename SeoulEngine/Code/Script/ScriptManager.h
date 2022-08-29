/**
 * \file ScriptManager.h
 * \brief Singleton manager of Lua cooked script files, as well as
 * other miscellaneous global Lua controls (such as whether debug
 * or release scripts are executed).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_MANAGER_H
#define SCRIPT_MANAGER_H

#include "Atomic32.h"
#include "ContentStore.h"
#include "ScriptFileBody.h"
#include "ScriptProtobuf.h"
#include "Singleton.h"
namespace Seoul { namespace Script { class DebuggerClient; } }

namespace Seoul
{

namespace Script
{

class Project SEOUL_SEALED
{
public:
	Project()
	{
	}

	~Project()
	{
	}

private:
	SEOUL_DISABLE_COPY(Project);
	SEOUL_REFERENCE_COUNTED(Project);
}; // class Script::Project

} // namespace Script

namespace Content
{

template <>
struct Traits<Script::Project>
{
	typedef FilePath KeyType;
	static const Bool CanSyncLoad = false;

	static SharedPtr<Script::Project> GetPlaceholder(const KeyType& key);
	static Bool FileChange(const KeyType& key, const Handle<Script::Project>& hEntry);
	static void Load(const KeyType& key, const Handle<Script::Project>& hEntry);
	static Bool PrepareDelete(const KeyType& key, Entry<Script::Project, KeyType>& entry);
	static void SyncLoad(FilePath filePath, const Handle<Script::Project>& hEntry) {}
	static UInt32 GetMemoryUsage(const SharedPtr<Script::Project>& p) { return 0u; }
}; // struct Content::Traits<ScriptProtobuf>

} // namespace Content

namespace Script
{

class Manager SEOUL_SEALED : public Singleton<Manager>
{
public:
	Manager();
	~Manager();

	/** @return The core project for the application. */
	const Content::Handle<Project>& GetAppScriptProject() const
	{
		return m_hAppScriptProject;
	}

	/** @return The protoc compiled Protocol Buffer data. */
	Content::Handle<Protobuf> GetPb(FilePath filePath)
	{
		return m_Pbs.GetContent(filePath);
	}

	/** @return The Lua bytecode data associated with filePath. */
	Content::Handle<FileBody> GetScript(FilePath filePath)
	{
		return m_Scripts.GetContent(filePath);
	}

	// Utility method - equivalent to GetPb(), except this method
	// will busy wait until the pb has completed loading, and then
	// return the pointer (which may still be nullptr if the loading failed).
	SharedPtr<Protobuf> WaitForPb(FilePath filePath);

	// Utility method - equivalent to GetScript(), except this method
	// will busy wait until the script has completed loading, and then
	// return the pointer (which may still be nullptr if the loading failed).
	SharedPtr<FileBody> WaitForScript(FilePath filePath);

	// Used for tracking normally untracked script loading tasks. Script::Vm's
	// are typically gameplay tied, so this is used to expose loading
	// status to Engine systems.
	void BeginAppScriptHotLoad() { ++m_ScriptHotLoad; }
	void EndAppScriptHotLoad() { --m_ScriptHotLoad; }
	Bool IsInAppScriptHotLoad() const
	{
		return (0 != m_ScriptHotLoad);
	}

private:
	Content::Store<Project> m_Projects;
	Content::Store<Protobuf> m_Pbs;
	Content::Store<FileBody> m_Scripts;
	Atomic32 m_ScriptHotLoad;
	FilePath const m_AppScriptProjectPath;
	Content::Handle<Project> m_hAppScriptProject;

#if SEOUL_ENABLE_DEBUGGER_CLIENT
	ScopedPtr<DebuggerClient> m_pDebugger;
#endif // /SEOUL_ENABLE_DEBUGGER_CLIENT

	SEOUL_DISABLE_COPY(Manager);
}; // class Script::Manager

} // namespace Script

} // namespace Seoul

#endif // include guard
