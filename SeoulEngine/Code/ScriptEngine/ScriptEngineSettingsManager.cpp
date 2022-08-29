/**
 * \file ScriptEngineSettingsManager.cpp
 * \brief Binder instance for exposing the global SettingsManager
 * to script.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Compress.h"
#include "CookManager.h"
#include "FileManager.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "ScopedAction.h"
#include "ScriptEngineSettingsManager.h"
#include "ScriptFunctionInterface.h"
#include "ScriptVm.h"
#include "SeoulFileReaders.h"
#include "SettingsManager.h"
#include "SeoulWildcard.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(ScriptEngineSettingsManager, TypeFlags::kDisableCopy)
	SEOUL_METHOD(Exists)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "bool", "object filePathOrFileNameString")
	SEOUL_METHOD(GetSettings)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(SlimCS.Table, FilePath)", "object filePathOrFileNameString")
	SEOUL_METHOD(SetSettings)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "void", "FilePath path, SlimCS.Table data")
	SEOUL_METHOD(GetSettingsAsJsonString)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(string, FilePath)", "object filePathOrFileNameString")
	SEOUL_METHOD(GetSettingsInDirectory)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(SlimCS.Table, FilePath)", "object filePathOrFileNameString, bool bRecursive = false")
#if !SEOUL_SHIP
	SEOUL_METHOD(ValidateSettings)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "bool", "string sExcludeWildcard, bool bCheckDependencies")
#endif // /#if !SEOUL_SHIP
SEOUL_END_TYPE()

/** Shared utility, get a FilePath from the Script interface. */
static inline Bool CommonGetFilePath(Script::FunctionInterface* pInterface, FilePath& rFilePath, Bool bDirectory = false)
{
	// Support argument as string or as a raw FilePath.
	String sFilename;
	FilePath filePath;
	if (pInterface->GetString(1u, sFilename))
	{
		filePath = FilePath::CreateConfigFilePath(sFilename);
	}
	// Error if argument one is not a FilePath after not being a string.
	else if (!pInterface->GetFilePath(1u, filePath))
	{
		pInterface->RaiseError(1, "expected string or FilePath file identifier.");
		return false;
	}

	// Fixup type if not a directory query.
	if (!bDirectory)
	{
		// Convenience, allow the lack of an extension on the lua side.
		if (filePath.GetType() == FileType::kUnknown)
		{
			filePath.SetType(FileType::kJson);
		}
	}

	rFilePath = filePath;
	return true;
}

ScriptEngineSettingsManager::ScriptEngineSettingsManager()
{
}

ScriptEngineSettingsManager::~ScriptEngineSettingsManager()
{
}

void ScriptEngineSettingsManager::Exists(Script::FunctionInterface* pInterface) const
{
	FilePath filePath;
	if (!CommonGetFilePath(pInterface, filePath))
	{
		return;
	}

	pInterface->PushReturnBoolean(FileManager::Get()->Exists(filePath));
}

void ScriptEngineSettingsManager::SetSettings(Script::FunctionInterface* pInterface) const
{
	FilePath filePath;
	if (!CommonGetFilePath(pInterface, filePath))
	{
		pInterface->RaiseError(1, "failed serializing filepath.");
		return;
	}

	SharedPtr<DataStore> pDataStore(SEOUL_NEW(MemoryBudgets::DataStore) DataStore);
	if (!pInterface->GetTable(2, *pDataStore))
	{
		pInterface->RaiseError(2, "failed serializing settings table to DataStore.");
		return;
	}
	
	SettingsManager::Get()->SetSettings(filePath, pDataStore);
}

void ScriptEngineSettingsManager::GetSettings(Script::FunctionInterface* pInterface) const
{
	FilePath filePath;
	if (!CommonGetFilePath(pInterface, filePath))
	{
		return;
	}

	SharedPtr<DataStore> pDataStore(SettingsManager::Get()->WaitForSettings(filePath));
	if (!pDataStore.IsValid())
	{
		pInterface->PushReturnNil();
		return;
	}

	if (!pInterface->PushReturnDataNode(*pDataStore, pDataStore->GetRootNode(), false, true))
	{
		pInterface->RaiseError(-1, "failed serializing JSON data to Lua, check for large integers or other incompatible elements.");
		return;
	}

#if SEOUL_HOT_LOADING
	// In builds with hot loading, make sure we tag this filePath as a dependency of the script VM.
	pInterface->GetScriptVm()->AddDataDependency(filePath);
#endif // /#if SEOUL_HOT_LOADING

	pInterface->PushReturnFilePath(filePath);
}

void ScriptEngineSettingsManager::GetSettingsAsJsonString(Script::FunctionInterface* pInterface) const
{
	FilePath filePath;
	if (!CommonGetFilePath(pInterface, filePath))
	{
		return;
	}

	Bool bMultiline = false;
	if (!pInterface->IsNilOrNone(2u))
	{
		if (!pInterface->GetBoolean(2u, bMultiline))
		{
			pInterface->RaiseError(2, "expected optional boolean bMultiline.");
			return;
		}
	}

	Bool bSortTableKeysAlphabetical = false;
	if (!pInterface->IsNilOrNone(3u))
	{
		if (!pInterface->GetBoolean(3u, bSortTableKeysAlphabetical))
		{
			pInterface->RaiseError(3, "expected optional boolean bSortTableKeysAlphabetical.");
			return;
		}
	}

	SharedPtr<DataStore> pDataStore(SettingsManager::Get()->WaitForSettings(filePath));
	if (!pDataStore.IsValid())
	{
		pInterface->PushReturnNil();
		return;
	}

#if SEOUL_HOT_LOADING
	// In builds with hot loading, make sure we tag this filePath as a dependency of the script VM.
	pInterface->GetScriptVm()->AddDataDependency(filePath);
#endif // /#if SEOUL_HOT_LOADING

	String s;
	pDataStore->ToString(pDataStore->GetRootNode(), s, bMultiline, 0, bSortTableKeysAlphabetical);

	pInterface->PushReturnString(s);
	pInterface->PushReturnFilePath(filePath);
}

void ScriptEngineSettingsManager::GetSettingsInDirectory(Script::FunctionInterface* pInterface) const
{
	FilePath dirPath;
	if (!CommonGetFilePath(pInterface, dirPath, true))
	{
		return;
	}

	Bool bRecursive = false;
	if (!pInterface->IsNilOrNone(2))
	{
		pInterface->GetBoolean(2, bRecursive);
	}

	Vector<String> vs;
	if (!FileManager::Get()->GetDirectoryListing(
		dirPath,
		vs,
		false,
		bRecursive,
		FileTypeToSourceExtension(FileType::kJson)))
	{
		pInterface->RaiseError(1, "failed directory listing, possibly invalid directory.");
		return;
	}

	// Now return all files.
	for (auto i = vs.Begin(); vs.End() != i; ++i)
	{
		FilePath const filePath(FilePath::CreateConfigFilePath(*i));
		SharedPtr<DataStore> pDataStore(SettingsManager::Get()->WaitForSettings(filePath));
		if (!pDataStore.IsValid())
		{
			pInterface->RaiseError(-1, "failed loading settings \"%s\"", filePath.CStr());
			return;
		}

		if (!pInterface->PushReturnDataNode(*pDataStore, pDataStore->GetRootNode(), false, true))
		{
			pInterface->RaiseError(-1, "failed serializing JSON data to Lua, check for large integers or other incompatible elements.");
			return;
		}

#if SEOUL_HOT_LOADING
		// In builds with hot loading, make sure we tag this filePath as a dependency of the script VM.
		pInterface->GetScriptVm()->AddDataDependency(filePath);
#endif // /#if SEOUL_HOT_LOADING

		pInterface->PushReturnFilePath(filePath);
	}
}

#if !SEOUL_SHIP
void ScriptEngineSettingsManager::ValidateSettings(Script::FunctionInterface* pInterface) const
{
	String sWildcard;
	(void)pInterface->GetString(1, sWildcard);
	Bool bCheckDependencies = true;
	(void)pInterface->GetBoolean(2, bCheckDependencies);

	UInt32 uUnusedNumChecked = 0u;
	auto const bReturn = SettingsManager::Get()->ValidateSettings(sWildcard, bCheckDependencies, uUnusedNumChecked);

	pInterface->PushReturnBoolean(bReturn);
}
#endif // /#if !SEOUL_SHIP

} // namespace Seoul
