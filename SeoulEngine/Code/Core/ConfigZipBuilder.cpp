/**
 * \file ConfigZipBuilder.h
 * \brief Tool for collecting all config .json files under the config root
 * and writing them in .zip format to a SyncFile.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ConfigZipBuilder.h"
#include "ZipFile.h"
#include "FileManager.h"
#include "Logger.h"
#include "ScopedAction.h"
#include "SeoulTime.h"
#include "DataStore.h"
#include "DataStoreParser.h"

namespace Seoul
{

namespace ConfigZipBuilder
{

SharedPtr<DataStore> CommandFileIncludeResolver(const String& sFileName, Bool bResolveCommands)
{
	auto const filePath(FilePath::CreateConfigFilePath(sFileName));

	DataStore dataStore;
	if (!DataStoreParser::FromFile(filePath, dataStore))
	{
		SEOUL_LOG("Can't parse file %s", sFileName.CStr());
		return SharedPtr<DataStore>();
	}

	if (bResolveCommands && DataStoreParser::IsJsonCommandFile(dataStore))
	{
		// Resolve the command file and replace the DataStore with the resolved content.
		if (!DataStoreParser::ResolveCommandFile(
			SEOUL_BIND_DELEGATE(&CommandFileIncludeResolver),
			filePath.GetAbsoluteFilename(),
			dataStore,
			dataStore,
			DataStoreParserFlags::kLogParseErrors))
		{
			SEOUL_LOG("Can't resolve commands for file %s", sFileName.CStr());
			return SharedPtr<DataStore>();
		}
	}

	SharedPtr<DataStore> pDataStore(SEOUL_NEW(MemoryBudgets::DataStore) DataStore);
	pDataStore->Swap(dataStore);
	return pDataStore;
}


/**
 * Fills a .zip file with all config .json files in the Config directory.
 */
Bool WriteAllJson(SyncFile& zipFile)
{
	// List the config directory
	auto pFileManager(FileManager::Get());
	if (!pFileManager)
	{
		SEOUL_WARN("Not writing config: FileManager not initialized");
		return false;
	}
	FilePath configDir;
	configDir.SetDirectory(GameDirectory::kConfig);

	Vector<String> vConfigFiles;
	if (!pFileManager->GetDirectoryListing(
			configDir,
			vConfigFiles,
			false, // bIncludeDirectories
			true, // bRecursive
			".json")) // sFileExtension
	{
		SEOUL_WARN("Not writing config: could not list directory");
		return false;
	}

	ZipFileWriter zip;
	if (!zip.Init(zipFile))
	{
		SEOUL_WARN("Not writing config: failed to initialize zip.");
		return false;
	}

	// Collect the files into a zip
	for (auto const& sConfigPath : vConfigFiles)
	{
		String const sRelativeFilename = FilePath::CreateConfigFilePath(sConfigPath).GetRelativeFilename();
		if (sRelativeFilename.StartsWith("UnitTests\\"))
		{
			continue;
		}

		String sConfigJson;
		{
			SharedPtr<DataStore> pDataStore = CommandFileIncludeResolver(sConfigPath, true);
			if (!pDataStore.IsValid())
			{
				SEOUL_WARN("Not writing config: failed to read %s", sConfigPath.CStr());
				return false;
			}

			pDataStore->ToString(pDataStore->GetRootNode(), sConfigJson, false, 0, false);
		}

		if (!zip.AddFileString(sRelativeFilename, sConfigJson, ZlibCompressionLevel::kNone))
		{
			SEOUL_WARN("Not writing config: failed to add \"%s\" to zip.", sConfigPath.CStr());
			return false;
		}
	}

	// Finish writing the zip
	if (!zip.Finalize())
	{
		SEOUL_WARN("Not writing config: failed to finalize zip.");
		return false;
	}

	return true;
}

} // namespace ConfigZipBuilder
} // namespace Seoul
