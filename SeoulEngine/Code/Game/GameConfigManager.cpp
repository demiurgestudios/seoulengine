/**
 * \file GameConfigManager.cpp
 * \brief Game::ConfigManager is (roughly) the concrete equivalent to
 * SettingsManager.
 *
 * While all data in SettingsManager is in a dynamic DataStore
 * structure, ConfigManager is expected to contain a concrete,
 * C++ hierarchy of data objects.
 *
 * As such, an App must define a concrete subclass of GameConfigManager.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FileManager.h"
#include "GameConfigManager.h"
#include "Logger.h"
#include "Path.h"
#include "ReflectionDefine.h"
#include "ReflectionDeserialize.h"
#include "ReflectionRegistry.h"
#include "ReflectionTable.h"
#include "SettingsManager.h"

namespace Seoul
{

using namespace Reflection;

SEOUL_TYPE(Game::ConfigManager, TypeFlags::kDisableNew);

namespace Game
{

ConfigManager::ConfigManager()
{
}

ConfigManager::~ConfigManager()
{
}

ConfigManagerLoadJob::ConfigManagerLoadJob(const Type& configManagerType)
	: m_ConfigManagerType(configManagerType)
	, m_pConfigManager()
{
}

ConfigManagerLoadJob::~ConfigManagerLoadJob()
{
}

static Bool LoadManyToOneProperty(const WeakAny& pConfigData, HString const configName, const Property& prop)
{
	// property must be viewable as a table.
	const TypeInfo& propertyTypeInfo = prop.GetMemberTypeInfo();
	const Table* propertyTable = propertyTypeInfo.GetType().TryGetTable();
	if (nullptr == propertyTable)
	{
		SEOUL_LOG("ManyToOne property \"%s\" must be a table type, is incompatible type \"%s\".",
			prop.GetName().CStr(),
			propertyTypeInfo.GetType().GetName().CStr());
		return false;
	}

	// Retrieve a pointer to the member. If this fails, we cannot load it.
	WeakAny member;
	if (!prop.TryGetPtr(pConfigData, member))
	{
		SEOUL_LOG("Failed getting member \"%s\" as pointer while loading config data.", prop.GetName().CStr());
		return false;
	}

	// Construct a FilePath with no type, references a directory.
	FilePath directory;
	directory.SetDirectory(GameDirectory::kConfig);
	directory.SetRelativeFilenameWithoutExtension(FilePathRelativeFilename(configName.CStr(), configName.GetSizeInBytes()));
	directory.SetType(FileType::kUnknown);

	// List the contents of the directory.
	Vector<String> vsResults;
	if (!FileManager::Get()->GetDirectoryListing(
		directory,
		vsResults,
		false,
		false,
		FileTypeToCookedExtension(FileType::kJson)))
	{
		SEOUL_LOG("ManyToOne property \"%s\" deserialization failed, could not list expected directory \"%s\"",
			prop.GetName().CStr(),
			directory.ToString().CStr());
		return false;
	}

	// Now enumerate the results, and add each to the table
	for (auto i = vsResults.Begin(); vsResults.End() != i; ++i)
	{
		// Get the base name of the file to use as the key.
		HString const key(Path::GetFileNameWithoutExtension(*i));

		// Get a pointer to the value we want to deserialize into. Pass
		// true to construct a default value if it doesn't already exist.
		WeakAny valuePtr;
		if (!propertyTable->TryGetValuePtr(member, key, valuePtr, true))
		{
			SEOUL_LOG("Failed getting pointer to value \"%s\" for insert into "
				"property table \"%s\" for ManyToOne ConfigData load.",
				key.CStr(),
				prop.GetName().CStr());
			return false;
		}

		// Get a FilePath to the individual file.
		FilePath const filePath(FilePath::CreateConfigFilePath(*i));

		// Now deserialize into the member.
		if (!SettingsManager::Get()->DeserializeObject(filePath, valuePtr))
		{
			SEOUL_LOG("Failed deserializing member \"%s\" property table \"%s\" "
				"for ManyToOne ConfigData load.",
				key.CStr(),
				prop.GetName().CStr());
			return false;
		}
	}

	return true;
}

static Bool LoadOneToOneProperty(const WeakAny& pConfigData, HString const configName, const Property& prop)
{
	// Retrieve a pointer to the member. If this fails, we cannot load it.
	WeakAny member;
	if (!prop.TryGetPtr(pConfigData, member))
	{
		SEOUL_LOG("Failed getting member \"%s\" as pointer while loading config data.", prop.GetName().CStr());
		return false;
	}

	// Create a FilePath to the expected file type.
	FilePath filePath = FilePath::CreateConfigFilePath(String(configName) + ".json");

	// Now deserialize into the member.
	return SettingsManager::Get()->DeserializeObject(filePath, member);
}

static Bool LoadConfigData(const WeakAny& configData)
{
	const Type& type = configData.GetType();
	UInt32 const uProperties = type.GetPropertyCount();
	for (UInt32 i = 0u; i < uProperties; ++i)
	{
		const Property& prop = *type.GetProperty(i);

		// Skip properties that we cannot set.
		if (!prop.CanSet())
		{
			continue;
		}

		// Config name is initially equal to property.Name
		HString configName = prop.GetName();

		// Can be overriden by the ConfigName attribute.
		{
			auto pAttribute = prop.GetAttributes().GetAttribute<Attributes::ConfigName>();
			if (nullptr != pAttribute)
			{
				configName = pAttribute->m_Name;
			}
		}

		// Keyed handling for ManyToOne vs. OneToOne properties.
		if (prop.GetAttributes().HasAttribute<Attributes::ManyToOne>())
		{
			// Load settings for the property.
			if (!LoadManyToOneProperty(configData, configName, prop))
			{
				return false;
			}
		}
		else
		{
			if (!LoadOneToOneProperty(configData, configName, prop))
			{
				return false;
			}
		}
	}

	return true;
}

void ConfigManagerLoadJob::InternalExecuteJob(Jobs::State& reNextState, ThreadId& rNextThreadId)
{
	// Get the attribute which defines our config manager instantiator.
	auto pCreateConfigManager = m_ConfigManagerType.GetAttribute<Attributes::CreateConfigManager>();
	if (nullptr == pCreateConfigManager)
	{
		SEOUL_LOG("Failed loading config manager, no CreateConfigManager attribute.");
		reNextState = Jobs::State::kError;
		return;
	}

	// Get the attribute which defines the root config data type.
	auto pRootConfigDataType = m_ConfigManagerType.GetAttribute<Attributes::RootConfigDataType>();
	if (nullptr == pRootConfigDataType)
	{
		SEOUL_LOG("Failed loading config manager, no RootConfigDataType attribute.");
		reNextState = Jobs::State::kError;
		return;
	}

	// Now acquire the config data type from the registry.
	auto pConfigDataType = Registry::GetRegistry().GetType(pRootConfigDataType->m_Name);
	if (nullptr == pConfigDataType)
	{
		SEOUL_LOG("Failed loading config manager, \"%s\" is not a valid config data type.", pRootConfigDataType->m_Name.CStr());
		reNextState = Jobs::State::kError;
		return;
	}

	// Instantiate the config data object.
	auto pConfigData = pConfigDataType->New(MemoryBudgets::Config);
	if (!pConfigData.IsValid())
	{
		SEOUL_LOG("Failed instantiating an instance of config data type \"%s\".", pConfigDataType->GetName().CStr());
		reNextState = Jobs::State::kError;
		return;
	}

	// Deserialize the root object.
	if (!LoadConfigData(pConfigData))
	{
		pConfigDataType->Delete(pConfigData);
		SEOUL_LOG("Failed deserializing config data into config data of type \"%s\".", pConfigDataType->GetName().CStr());
		reNextState = Jobs::State::kError;
		return;
	}

	// Now instantiate the config manager.
	m_pConfigManager.Reset(pCreateConfigManager->m_pCreateConfigManager(pConfigData));
	if (!m_pConfigManager.IsValid())
	{
		pConfigDataType->Delete(pConfigData);
		SEOUL_LOG("Failed instantiating concrete config manager of type \"%s\".", m_ConfigManagerType.GetName().CStr());
		reNextState = Jobs::State::kError;
		return;
	}

	reNextState = Jobs::State::kComplete;
}

} // namespace Game

SEOUL_TYPE(Game::NullConfigData, TypeFlags::kDisableCopy)

namespace Game
{

NullConfigData::NullConfigData()
{
}

NullConfigData::~NullConfigData()
{
}

} // namespace Game

SEOUL_BEGIN_TYPE(Game::NullConfigManager, TypeFlags::kDisableNew)
	SEOUL_PARENT(Game::ConfigManager)
	SEOUL_ATTRIBUTE(RootConfigDataType, "NullConfigData")
	SEOUL_ATTRIBUTE(CreateConfigManager, Game::NullConfigManager::CreateNullConfigManager)
SEOUL_END_TYPE()

namespace Game
{

CheckedPtr<ConfigManager> NullConfigManager::CreateNullConfigManager(const Reflection::WeakAny& pConfigData)
{
	return SEOUL_NEW(MemoryBudgets::Config) NullConfigManager(pConfigData.Cast<NullConfigData*>());
}

/**
 * Construct the NullConfigManager with already loaded/valid
 * config data.
 *
 * NullConfigManager takes ownership of the data and will SafeDelete()
 * it on destruction.
 */
NullConfigManager::NullConfigManager(CheckedPtr<NullConfigData> pConfigData)
	: m_pConfigData(pConfigData)
{
}

NullConfigManager::~NullConfigManager()
{
}

} // namespace Game

} // namespace Seoul
