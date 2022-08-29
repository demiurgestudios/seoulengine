/**
 * \file ScriptManager.cpp
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

#include "CompilerSettings.h"
#include "ContentLoadManager.h"
#include "CookManager.h"
#include "FileManager.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ScriptDebuggerClient.h"
#include "ScriptManager.h"

namespace Seoul
{

namespace Script
{

class ProjectContentLoader SEOUL_SEALED : public Content::LoaderBase
{
public:
	ProjectContentLoader(
		FilePath filePath,
		const Content::Handle<Project>& hEntry)
		: Content::LoaderBase(filePath)
		, m_hEntry(hEntry)
	{
		m_hEntry.GetContentEntry()->IncrementLoaderCount();
	}

	~ProjectContentLoader()
	{
		// Block until this Content::Loaded is in a non-loading state.
		WaitUntilContentIsNotLoading();

		// Release the content populate entry if it is still valid.
		InternalReleaseEntry();
	}

private:
	SEOUL_DISABLE_COPY(ProjectContentLoader);
	SEOUL_REFERENCE_COUNTED_SUBCLASS(ProjectContentLoader);

	Content::Handle<Project> m_hEntry;

	Content::LoadState InternalExecuteContentLoadOp()
	{
#if !SEOUL_SHIP
		// Conditionally cook if the cooked file is not up to date with the source file.
		(void)CookManager::Get()->CookIfOutOfDate(GetFilePath());
#endif

		// Nothing to load, complete immediately. Script::Project
		// currently just exists to track hot loading and load
		// state of SlimCS execution.
		SharedPtr<Project> p(SEOUL_NEW(MemoryBudgets::Scripting) Project);
		auto pEntry(m_hEntry.GetContentEntry());
		if (!pEntry.IsValid())
		{
			return Content::LoadState::kError;
		}

		pEntry->AtomicReplace(p);
		InternalReleaseEntry();
		return Content::LoadState::kLoaded;
	}

	/**
	 * Release the loader's reference on its content entry - doing this as
	 * soon as loading completes allows anything waiting for the load to react
	 * as soon as possible.
	 */
	void InternalReleaseEntry()
	{
		if (!m_hEntry.IsInternalPtrValid())
		{
			return;
		}

		// NOTE: We need to release our reference before decrementing the loader count.
		// This is safe, because a Content::Entry's Content::Store always maintains 1 reference,
		// and does not release it until the content is done loading.
		auto pEntry = m_hEntry.GetContentEntry().GetPtr();
		m_hEntry.Reset();
		pEntry->DecrementLoaderCount();
	}
}; // class Script::ProjectContentLoader

} // namespace Script

SharedPtr<Script::Project> Content::Traits<Script::Project>::GetPlaceholder(const KeyType& key)
{
	return SharedPtr<Script::Project>();
}

Bool Content::Traits<Script::Project>::FileChange(const KeyType& key, const Content::Handle<Script::Project>& pEntry)
{
	// Only react to FileChange events if the key is a Proto type file.
	if (FileType::kScriptProject == key.GetType())
	{
		Load(key, pEntry);
		return true;
	}

	return false;
}

void Content::Traits<Script::Project>::Load(const KeyType& key, const Content::Handle<Script::Project>& pEntry)
{
	// Only load if the key a Proto type file.
	if (FileType::kScriptProject == key.GetType())
	{
		Content::LoadManager::Get()->Queue(
			SharedPtr<Content::LoaderBase>(SEOUL_NEW(MemoryBudgets::Content) Script::ProjectContentLoader(key, pEntry)));
	}
}

Bool Content::Traits<Script::Project>::PrepareDelete(
	const KeyType& key,
	Content::Entry<Script::Project, KeyType>& entry)
{
	return true;
}

namespace Script
{

Manager::Manager()
	: m_Projects()
	, m_Pbs()
	, m_Scripts()
	, m_ScriptHotLoad(0)
	, m_AppScriptProjectPath(CompilerSettings::GetApplicationScriptProjectFilePath())
	, m_hAppScriptProject()
#if SEOUL_ENABLE_DEBUGGER_CLIENT
	, m_pDebugger(SEOUL_NEW(MemoryBudgets::Scripting) DebuggerClient(m_AppScriptProjectPath, "127.0.0.1" /* TODO: */))
#endif // /SEOUL_ENABLE_DEBUGGER_CLIENT
{
	// Load up the app's script project if one exists.
	if (m_AppScriptProjectPath.IsValid() &&
		FileManager::Get()->Exists(m_AppScriptProjectPath))
	{
		m_hAppScriptProject = m_Projects.GetContent(m_AppScriptProjectPath);
	}
}

Manager::~Manager()
{
}

/**
 * Utility method - equivalent to GetPb(), except this method
 * will busy wait until the pb has completed loading, and then
 * return the pointer (which may still be nullptr if the loading failed).
 */
SharedPtr<Protobuf> Manager::WaitForPb(FilePath filePath)
{
	Content::Handle<Protobuf> hPb = GetPb(filePath);
	Content::LoadManager::Get()->WaitUntilLoadIsFinished(hPb);
	return hPb.GetPtr();
}

/**
 * Utility method - equivalent to GetScript(), except this method
 * will busy wait until the script has completed loading, and then
 * return the pointer (which may still be nullptr if the loading failed).
 */
SharedPtr<FileBody> Manager::WaitForScript(FilePath filePath)
{
	Content::Handle<FileBody> hScript = m_Scripts.GetContent(filePath, true);
	Content::LoadManager::Get()->WaitUntilLoadIsFinished(hScript);
	return hScript.GetPtr();
}

} // namespace Script

} // namespace Seoul
