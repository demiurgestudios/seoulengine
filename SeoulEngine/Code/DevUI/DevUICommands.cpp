/**
 * \file DevUICommands.cpp
 * \brief Cheat commands for DevUI functionality.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DevUIConfig.h"
#include "DevUIRoot.h"
#include "DevUIUtil.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "JobsFunction.h"
#include "LocManager.h"
#include "ReflectionDataStoreTableUtil.h"
#include "ReflectionDefine.h"
#include "RenderDevice.h"
#include "SettingsManager.h"

#if SEOUL_ENABLE_DEV_UI

namespace Seoul
{

namespace DevUI
{
	
static Config s_DefaultConfig{};

#if SEOUL_ENABLE_CHEATS

#if !SEOUL_SHIP // Only save/load in non-ship builds.
static inline FilePath GetDevUIConfigFilePath()
{
	return FilePath::CreateSaveFilePath("devui_config.json");
}

static inline FilePath GetDeprecatedScreenConfigFilePath()
{
	return FilePath::CreateSaveFilePath("screenshot_config.json");
}

static void DoValidateSettings()
{
	auto pDevUI(Root::Get());
	Int32 iId = 0;
	if (pDevUI)
	{
		pDevUI->DisplayTrackedNotification("Validating JSON files...", iId);
	}

	UInt32 uNumChecked = 0u;
	auto const bReturn = SettingsManager::Get()->ValidateSettings("UnitTests/*", true, uNumChecked);

	if (pDevUI)
	{
		pDevUI->DisplayNotification(String::Printf("JSON (%u files): %s", uNumChecked, (bReturn ? "SUCCESS" : "FAILURE")));
		pDevUI->KillNotification(iId);
	}
}

static void DoValidateLocTokens()
{
	auto pDevUI(Root::Get());
	Int32 iId = 0;
	if (pDevUI)
	{
		pDevUI->DisplayTrackedNotification("Validating Loc Tokens...", iId);
	}

	UInt32 uNumChecked = 0u;
	auto const bReturn = LocManager::Get()->ValidateTokens(uNumChecked);

	if (pDevUI)
	{
		pDevUI->DisplayNotification(String::Printf("TOKENS (%u): %s", uNumChecked, (bReturn ? "SUCCESS" : "FAILURE")));
		pDevUI->KillNotification(iId);
	}
}
#endif // /!SEOUL_SHIP

/**
 * Cheat commands for DevUI functionality.
 */
class Commands SEOUL_SEALED : public Singleton<Commands>
{
public:
	Commands()
	{
		LoadDevUIConfig();
	}

	~Commands()
	{
	}

	void LoadDevUIConfig()
	{
		// Settings only loaded in developer builds - support exists for DevUI in non-developer
		// builds only to support cheats (deprecated usage of the Profiling build for cheats)
		// and the miniature FPS overlay).
#if !SEOUL_SHIP
		auto const filePath = GetDevUIConfigFilePath();
		auto bSuccess = FileManager::Get()->Exists(filePath);

		// Backwards compatibility handling.
		if (!bSuccess)
		{
			auto const oldFilePath = GetDeprecatedScreenConfigFilePath();
			bSuccess = FileManager::Get()->Exists(oldFilePath);
			if (bSuccess)
			{
				m_DevUIConfig = Config{};
				bSuccess = SettingsManager::Get()->DeserializeObject(oldFilePath, &m_DevUIConfig.m_ScreenshotConfig);
				if (bSuccess)
				{
					if (SaveDevUIConfig())
					{
						FileManager::Get()->Delete(oldFilePath);
					}
				}
			}
		}
		else
		{
			bSuccess = SettingsManager::Get()->DeserializeObject(filePath, &m_DevUIConfig);
		}

		if (!bSuccess)
		{
			m_DevUIConfig = Config{};
		}

		// Apply some settings to the environment.
		if (m_DevUIConfig.m_GlobalConfig.m_bAutoHotLoad)
		{
			Content::LoadManager::Get()->SetHoadLoadMode(Content::LoadManagerHotLoadMode::kPermanentAccept);
		}
		else
		{
			Content::LoadManager::Get()->SetHoadLoadMode(Content::LoadManagerHotLoadMode::kNoAction);
		}
#else // SEOUL_SHIP
		m_DevUIConfig = Config{};
#endif // /SEOUL_SHIP
	}

	Bool SaveDevUIConfig()
	{
		// Settings only loaded in developer builds - support exists for DevUI in non-developer
		// builds only to support cheats (deprecated usage of the Profiling build for cheats)
		// and the miniature FPS overlay).
#if !SEOUL_SHIP
		DataStore dataStore;

		SEOUL_VERIFY(Reflection::SerializeToDataStore(&m_DevUIConfig, dataStore));

		auto const filePath = GetDevUIConfigFilePath();
		Content::LoadManager::Get()->TempSuppressSpecificHotLoad(filePath);
		return Reflection::SaveDataStore(dataStore, dataStore.GetRootNode(), filePath);
#else // SEOUL_SHIP
		return false;
#endif // /SEOUL_SHIP
	}

	// Global config.
	void SetAutoHotLoad(Bool bEnable)
	{
		if (bEnable != m_DevUIConfig.m_GlobalConfig.m_bAutoHotLoad)
		{
			m_DevUIConfig.m_GlobalConfig.m_bAutoHotLoad = bEnable;
			SaveDevUIConfig();

			// Apply some settings to the environment.
			if (m_DevUIConfig.m_GlobalConfig.m_bAutoHotLoad)
			{
				Content::LoadManager::Get()->SetHoadLoadMode(Content::LoadManagerHotLoadMode::kPermanentAccept);
			}
			else
			{
				Content::LoadManager::Get()->SetHoadLoadMode(Content::LoadManagerHotLoadMode::kNoAction);
			}
		}
	}

	void ResetConfiguredInverseWindowScale()
	{
		if (Root::Get())
		{
			Root::Get()->ResetConfiguredInverseWindowScale();
		}
	}

	void SetConfiguredInverseWindowScale(Float fScale)
	{
		if (Root::Get())
		{
			Root::Get()->SetConfiguredInverseWindowScale(fScale);
		}
	}

	void SetUniqueLayoutForBranches(Bool bUnique)
	{
		if (bUnique != m_DevUIConfig.m_GlobalConfig.m_bUniqueLayoutForBranches)
		{
			m_DevUIConfig.m_GlobalConfig.m_bUniqueLayoutForBranches = bUnique;
			SaveDevUIConfig();
		}
	}

	// Screenshots.
	void SetDedupScreenshots(Bool bDedup)
	{
		if (bDedup != m_DevUIConfig.m_ScreenshotConfig.m_bDedup)
		{
			m_DevUIConfig.m_ScreenshotConfig.m_bDedup = bDedup;
			SaveDevUIConfig();
		}
	}

	void SetTargetHeight(Int32 iTargetHeight)
	{
		if (iTargetHeight != m_DevUIConfig.m_ScreenshotConfig.m_iTargetHeight)
		{
			m_DevUIConfig.m_ScreenshotConfig.m_iTargetHeight = iTargetHeight;
			SaveDevUIConfig();
		}
	}

	void ValidateLocTokens()
	{
#if !SEOUL_SHIP
		Jobs::AsyncFunction(&DoValidateLocTokens);
#endif
	}

	void ValidateSettings()
	{
#if !SEOUL_SHIP
		Jobs::AsyncFunction(&DoValidateSettings);
#endif
	}

	Config m_DevUIConfig;

private:
	SEOUL_DISABLE_COPY(Commands);
}; // class DevUI::Commands

static Reflection::Any GetAutoHotLoad()
{
	return Commands::Get()->m_DevUIConfig.m_GlobalConfig.m_bAutoHotLoad;
}

static Reflection::Any GetCurrentConfiguredInverseWindowScale()
{
	return Root::Get() ? Root::Get()->GetConfiguredInverseWindowScale() : 1.0f;
}

static Reflection::Any GetCurrentScreenshotDedup()
{
	return Commands::Get()->m_DevUIConfig.m_ScreenshotConfig.m_bDedup;
}

static Reflection::Any GetCurrentTargetHeight()
{
	return Commands::Get()->m_DevUIConfig.m_ScreenshotConfig.m_iTargetHeight;
}

static Reflection::Any GetCurrentUniqueLayoutForBranches()
{
	return Commands::Get()->m_DevUIConfig.m_GlobalConfig.m_bUniqueLayoutForBranches;
}

} // namespace DevUI

SEOUL_BEGIN_TYPE(DevUI::Commands, TypeFlags::kDisableCopy)
	SEOUL_ATTRIBUTE(CommandsInstance)

	// Global config.
	SEOUL_METHOD(SetAutoHotLoad)
		SEOUL_ATTRIBUTE(Category, "Dev Settings")
		SEOUL_ATTRIBUTE(Description,
			"When true, hot loading is not prompt. Assets reload\n"
			"as soon as possible after the change on disk is detected.")
		SEOUL_ATTRIBUTE(DisplayName, "Auto Hot Load")
		SEOUL_ARG_ATTRIBUTE(0, GetCurrentValue, DevUI::GetAutoHotLoad)
	SEOUL_METHOD(ResetConfiguredInverseWindowScale)
		SEOUL_ATTRIBUTE(Category, "Dev Settings")
		SEOUL_ATTRIBUTE(Description,
			"Reset developer UI scale. The scaling value will\n"
			"revert to the platform default as determined by system DPI.")
		SEOUL_ATTRIBUTE(DisplayName, "Reset Window Scale")
	SEOUL_METHOD(SetConfiguredInverseWindowScale)
		SEOUL_ATTRIBUTE(Category, "Dev Settings")
		SEOUL_ATTRIBUTE(Description,
			"Adjust the relative size of the developer UI.\n"
			"This scales padding, fonts, and all elements of the developer UI.")
		SEOUL_ATTRIBUTE(DisplayName, "Window Scale")
		SEOUL_ARG_ATTRIBUTE(0, GetCurrentValue, DevUI::GetCurrentConfiguredInverseWindowScale)
		SEOUL_ARG_ATTRIBUTE(0, Range, DevUI::Util::kfMinInverseWindowScale, DevUI::Util::kfMaxInverseWindowScale)
	SEOUL_METHOD(SetUniqueLayoutForBranches)
		SEOUL_ATTRIBUTE(Category, "Dev Settings")
		SEOUL_ATTRIBUTE(Description,
			"True will save a unique developer UI layout file for branch builds\n"
			"vs. the head build.")
		SEOUL_ATTRIBUTE(DisplayName, "Unique Branch Layout")
		SEOUL_ARG_ATTRIBUTE(0, GetCurrentValue, DevUI::GetCurrentUniqueLayoutForBranches)

	// Screenshot related.
	SEOUL_METHOD(SetDedupScreenshots)
		SEOUL_ATTRIBUTE(Category, "Screenshot")
		SEOUL_ATTRIBUTE(Description,
			"True will prevent overwrite when writing a screenshot. Otherwise, overwrite\n"
			"will occur (same filename will be used for all screenshots).")
		SEOUL_ATTRIBUTE(DisplayName, "Dedup")
		SEOUL_ARG_ATTRIBUTE(0, GetCurrentValue, DevUI::GetCurrentScreenshotDedup)

	SEOUL_METHOD(SetTargetHeight)
		SEOUL_ATTRIBUTE(Category, "Screenshot")
		SEOUL_ATTRIBUTE(Description,
			"Set the height of a captured screenshot. Otherwise, <= 0, screenshot will match\n"
			"viewport height. NOTE: Upscaling will be fuzzy, this is not super-sampled.")
		SEOUL_ATTRIBUTE(DisplayName, "Target Height")
		SEOUL_ARG_ATTRIBUTE(0, GetCurrentValue, DevUI::GetCurrentTargetHeight)

	SEOUL_METHOD(ValidateLocTokens)
		SEOUL_ATTRIBUTE(Category, "Localization")
		SEOUL_ATTRIBUTE(Description,
			"Runs validation on all loc tokens.")
		SEOUL_ATTRIBUTE(DisplayName, "Validate")
	SEOUL_METHOD(ValidateSettings)
		SEOUL_ATTRIBUTE(Category, "JSON")
		SEOUL_ATTRIBUTE(Description,
			"Runs validation on all .json files, including a subset\n"
			"of user authored, nested content.")
		SEOUL_ATTRIBUTE(DisplayName, "Validate")
SEOUL_END_TYPE()

namespace DevUI
{

	Config& GetDevUIConfig()
	{
		return Commands::Get().IsValid() ? Commands::Get()->m_DevUIConfig : s_DefaultConfig;
	}

	Bool SaveDevUIConfig()
	{
		if (Commands::Get())
		{
			return Commands::Get()->SaveDevUIConfig();
		}

		return false;
	}

#else // !SEOUL_ENABLE_CHEATS

Config& GetDevUIConfig()
{
	return s_DefaultConfig;
}

Bool SaveDevUIConfig()
{
	return false;
}

#endif // /!SEOUL_ENABLE_CHEATS

} // namespace DevUI

} // namespace Seoul

#endif // /SEOUL_ENABLE_DEV_UI
