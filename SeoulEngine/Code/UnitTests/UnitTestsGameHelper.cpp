/**
 * \file UnitTestsGameHelper.cpp
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Directory.h"
#include "FileManager.h"
#include "GameConfigManager.h"
#include "GameMain.h"
#include "GamePaths.h"
#include "GamePersistenceManager.h"
#include "NullPlatformEngine.h"
#include "PatchablePackageFileSystem.h"
#include "ReflectionDefine.h"
#include "UnitTestsEngineHelper.h"
#include "UnitTestsGameHelper.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

struct UnitTestsConfigData
{
	UnitTestsConfigData()
	{
	}
};

struct UnitTestsPersistenceData
{
	UnitTestsPersistenceData()
		: m_iPlaceholder(0)
	{
	}

	Int32 m_iPlaceholder;
};

class UnitTestsConfigManager SEOUL_SEALED : public Game::ConfigManager
{
public:
	static CheckedPtr<Game::ConfigManager> CreateConfigManager(const Reflection::WeakAny& pConfigData)
	{
		return SEOUL_NEW(MemoryBudgets::Config) UnitTestsConfigManager(pConfigData.Cast<UnitTestsConfigData*>());
	}

private:
	UnitTestsConfigManager(UnitTestsConfigData* pData)
		: m_pData(pData)
	{
	}

	ScopedPtr<UnitTestsConfigData> m_pData;

	SEOUL_DISABLE_COPY(UnitTestsConfigManager);
};

#if SEOUL_WITH_GAME_PERSISTENCE
class UnitTestsPersistenceManager SEOUL_SEALED : public Game::PersistenceManager
{
public:
	static CheckedPtr<Game::PersistenceManager> CreatePersistenceManager(
		const Game::PersistenceSettings& settings,
		Bool bDisableSaving,
		const Reflection::WeakAny& pPersistenceData)
	{
		return SEOUL_NEW(MemoryBudgets::Persistence) UnitTestsPersistenceManager(
			settings,
			bDisableSaving,
			pPersistenceData.Cast<UnitTestsPersistenceData*>());
	}

	static Bool PersistencePostLoad(
		const Game::PersistenceSettings& /*settings*/,
		const Reflection::WeakAny& /*pPersistenceData*/,
		Bool /*bNew*/)
	{
		return true;
	}

	virtual void QueueSave(Bool bForceCloudSave, const SharedPtr<ISaveLoadOnComplete>& pSaveComplete = SharedPtr<ISaveLoadOnComplete>()) SEOUL_OVERRIDE
	{
		// Nop
	}

	virtual void Update() SEOUL_OVERRIDE
	{
	}

	virtual void GetSoundSettings(Sound::Settings&) const SEOUL_OVERRIDE
	{
	}

private:
	UnitTestsPersistenceManager(
		const Game::PersistenceSettings&,
		Bool bDisableSaving,
		UnitTestsPersistenceData* pData)
		: m_pData(pData)
	{
	}

	ScopedPtr<UnitTestsPersistenceData> m_pData;

	SEOUL_DISABLE_COPY(UnitTestsPersistenceManager);
}; // class UnittestsPersistenceManager
#endif // /#if SEOUL_WITH_GAME_PERSISTENCE

SEOUL_TYPE(UnitTestsConfigData)

#if SEOUL_WITH_GAME_PERSISTENCE
SEOUL_BEGIN_TYPE(UnitTestsPersistenceData)
	SEOUL_PROPERTY_N("Placeholder", m_iPlaceholder)
SEOUL_END_TYPE()
#endif // /#if SEOUL_WITH_GAME_PERSISTENCE

SEOUL_BEGIN_TYPE(UnitTestsConfigManager, TypeFlags::kDisableNew)
	SEOUL_PARENT(Game::ConfigManager)
	SEOUL_ATTRIBUTE(RootConfigDataType, "UnitTestsConfigData")
	SEOUL_ATTRIBUTE(CreateConfigManager, UnitTestsConfigManager::CreateConfigManager)
SEOUL_END_TYPE()

#if SEOUL_WITH_GAME_PERSISTENCE
SEOUL_BEGIN_TYPE(UnitTestsPersistenceManager, TypeFlags::kDisableNew)
	SEOUL_PARENT(Game::PersistenceManager)
	SEOUL_ATTRIBUTE(CreatePersistenceManager, UnitTestsPersistenceManager::CreatePersistenceManager)
	SEOUL_ATTRIBUTE(PersistencePostLoad, UnitTestsPersistenceManager::PersistencePostLoad)
	SEOUL_ATTRIBUTE(RootPersistenceDataType, "UnitTestsPersistenceData")
SEOUL_END_TYPE()
#endif // /#if SEOUL_WITH_GAME_PERSISTENCE

namespace
{

static String s_sRelativeConfigUpdatePath;
static String s_sRelativeContentPath;
static String s_sRelativeContentUpdatePath;
static CheckedPtr<PatchablePackageFileSystem> s_pConfigUpdate;
static CheckedPtr<PackageFileSystem> s_pContent;
static CheckedPtr<PatchablePackageFileSystem> s_pContentUpdate;

static void RegisterFileSystems()
{
	s_pConfigUpdate = FileManager::Get()->RegisterFileSystem<PatchablePackageFileSystem>(
		Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests", s_sRelativeConfigUpdatePath), // read-only builtin
		Path::Combine(GetUnitTestingSaveDir(), "Data/GamePatcherTest_ConfigUpdate.sar")); // updateable path
	s_pContent = FileManager::Get()->RegisterFileSystem<PackageFileSystem>(
		Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests", s_sRelativeContentPath)); // Content without updates.
	s_pContentUpdate = FileManager::Get()->RegisterFileSystem<PatchablePackageFileSystem>(
		Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests", s_sRelativeContentUpdatePath), // read-only builtin
		Path::Combine(GetUnitTestingSaveDir(), "Data/GamePatcherTest_ContentUpdate.sar")); // updateable path
}

static Game::MainSettings GetSettings(const String& sServerBaseURL)
{
#if SEOUL_WITH_GAME_PERSISTENCE
	Game::PersistenceSettings persistenceSettings;
	persistenceSettings.m_FilePath = FilePath::CreateSaveFilePath("unit-tests-save.dat");
	persistenceSettings.m_iVersion = 1;
	persistenceSettings.m_pPersistenceManagerType = &TypeOf<UnitTestsPersistenceManager>();
	Game::MainSettings settings(TypeOf<UnitTestsConfigManager>(), persistenceSettings);
#else
	Game::MainSettings settings(TypeOf<UnitTestsConfigManager>());
#endif
	settings.m_sServerBaseURL = sServerBaseURL;
	settings.m_pConfigUpdatePackageFileSystem = s_pConfigUpdate;
	settings.m_pContentUpdatePackageFileSystem = s_pContentUpdate;

	return settings;
}

} // namespace anonymous

UnitTestsGameHelper::UnitTestsGameHelper(
	const String& sServerBaseURL,
	const String& sRelativeConfigUpdatePath,
	const String& sRelativeContentPath,
	const String& sRelativeContentUpdatePath,
	Sound::Manager* (*soundManagerCreate)() /*= nullptr*/)
	: m_pEngineHelper()
	, m_pGameMain()
{
	s_sRelativeConfigUpdatePath = sRelativeConfigUpdatePath;
	s_sRelativeContentPath = sRelativeContentPath;
	s_sRelativeContentUpdatePath = sRelativeContentUpdatePath;

	NullPlatformEngineSettings settings;
	settings.m_CreateSoundManager = soundManagerCreate;
	m_pEngineHelper.Reset(SEOUL_NEW(MemoryBudgets::Developer) UnitTestsEngineHelper(RegisterFileSystems, settings));
	m_pGameMain.Reset(SEOUL_NEW(MemoryBudgets::Developer) Game::Main(GetSettings(sServerBaseURL)));
}

UnitTestsGameHelper::~UnitTestsGameHelper()
{
	// On shutdown, delete any files in the user's save folder.
	auto const sSave(GetUnitTestingSaveDir());

	m_pGameMain.Reset();
	m_pEngineHelper.Reset();

	s_pContentUpdate.Reset();
	s_pContent.Reset();
	s_pConfigUpdate.Reset();

	// Now delete files in the save directory.
	Directory::Delete(sSave, true);
}

void UnitTestsGameHelper::Tick()
{
	m_pGameMain->Tick();
}

#endif // /#if SEOUL_UNIT_TESTS

} // namespace Seoul
