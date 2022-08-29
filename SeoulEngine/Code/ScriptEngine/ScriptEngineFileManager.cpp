/**
 * \file ScriptEngineFileManager.cpp
 * \brief Binder instance for exposing the FileManager Singleton into
 * a script VM.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FileManager.h"
#include "GamePaths.h"
#include "Path.h"
#include "ReflectionDefine.h"
#include "ScriptEngineFileManager.h"
#include "ScriptFunctionInterface.h"
#include "SeoulString.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(ScriptEngineFileManager, TypeFlags::kDisableCopy)
	SEOUL_METHOD(Delete)
	SEOUL_METHOD(FileExists)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "bool", "object filePathOrFileNameString")
	SEOUL_METHOD(GetDirectoryListing)
	SEOUL_METHOD(GetSourceDir)
	SEOUL_METHOD(GetVideosDir)
	SEOUL_METHOD(RenameFile)
SEOUL_END_TYPE()

static inline void NormalizePath(String& rs)
{
	if (!Path::IsRooted(rs))
	{
		(void)Path::CombineAndSimplify(
			GamePaths::Get()->GetBaseDir(),
			rs,
			rs);
	}
	else
	{
		(void)Path::CombineAndSimplify(
			String(),
			rs,
			rs);
	}
}

ScriptEngineFileManager::ScriptEngineFileManager()
{
}

ScriptEngineFileManager::~ScriptEngineFileManager()
{
}

Bool ScriptEngineFileManager::Delete(const String& sFileName)
{
	return FileManager::Get()->Delete(sFileName);
}

void ScriptEngineFileManager::FileExists(Script::FunctionInterface* pInterface) const
{
	String s;
	FilePath filePath;
	if (pInterface->GetString(1u, s))
	{
		NormalizePath(s);
		pInterface->PushReturnBoolean(FileManager::Get()->Exists(s));
	}
	else if (pInterface->GetFilePath(1u, filePath))
	{
		pInterface->PushReturnBoolean(FileManager::Get()->Exists(filePath));
	}
	else
	{
		pInterface->PushReturnBoolean(false);
	}
}

Vector<String> ScriptEngineFileManager::GetDirectoryListing(const String& sPath, Bool bRecursive, const String& sFileExtension) const
{
	Vector<String> vsReturn;
	(void)FileManager::Get()->GetDirectoryListing(
		sPath,
		vsReturn,
		false,
		bRecursive,
		sFileExtension);
	return vsReturn;
}

Bool ScriptEngineFileManager::RenameFile(const String& sFrom, const String& sTo)
{
	return FileManager::Get()->Rename(sFrom, sTo);
}

String ScriptEngineFileManager::GetSourceDir() const
{
	return GamePaths::Get()->GetSourceDir();
}

String ScriptEngineFileManager::GetVideosDir() const
{
	return GamePaths::Get()->GetVideosDir();
}

} // namespace Seoul
