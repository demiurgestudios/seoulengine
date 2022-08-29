/**
 * \file GameMain.h
 * \brief Root singleton that handles startup of Singletons that
 * depend on Engine, and organized control of startup logic in light
 * of in-app patching and soft restarts.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GAME_MAIN_H
#define GAME_MAIN_H

#include "ContentHandle.h"
#include "CrashManager.h"
#include "Delegate.h"
#include "GamePersistenceManager.h"
#include "SaveLoadManagerSettings.h"
#include "ScopedPtr.h"
#include "SeoulUtil.h"
#include "Settings.h"
#include "Singleton.h"
#include "ThreadId.h"
#include "Vector.h"
namespace Seoul { struct CustomCrashErrorState; }
namespace Seoul { namespace DevUI { class Root; } }
namespace Seoul { class FxManager; }
namespace Seoul { namespace Game { class AuthManager; } }
namespace Seoul { namespace Game { class Automation; } }
namespace Seoul { namespace Game { class ConfigManager; } }
namespace Seoul { namespace Game { class Client; } }
namespace Seoul { namespace Game { class PersistenceManager; } }
namespace Seoul { namespace Game { class ScriptManager; } }
namespace Seoul { namespace Game { struct ScriptManagerSettings; } }
namespace Seoul { class PatchablePackageFileSystem; }
namespace Seoul { namespace Scene { class PrefabManager; } }
namespace Seoul { namespace Script { class Manager; } }
namespace Seoul { namespace Script { class Vm; } }
namespace Seoul { namespace UI { class Manager; } }
namespace Seoul { namespace UI { class Movie; } }
namespace Seoul { class WorldTime; }
namespace Seoul { namespace Animation { class NetworkDefinitionManager; } }
namespace Seoul { namespace Animation2D { class Manager; } }
namespace Seoul { namespace Animation3D { class Manager; } }
namespace Seoul { namespace Reflection { class Type; } }
namespace Seoul { namespace Reflection { class WeakAny; } }

namespace Seoul::Game
{

static const HString MainTickEventID("GameMainTickEvent");

struct MainSettings SEOUL_SEALED
{
	typedef Delegate<void(const Reflection::WeakAny& pConfigData)> CreateConfigManager;
	typedef Delegate<UI::Movie*(HString typeName)> CustomUIMovieInstantiator;
	typedef Delegate<void(const CustomCrashErrorState&)> ScriptErrorHandler;

	enum AutomatedTesting
	{
		/** Testing is disabled. */
		kOff,

		/** Testing is on, not persistent. */
		kAutomatedTesting,

		/** Testing is on, persistent. Session is not assumed to be a clean slate. */
		kPersistentAutomatedTesting,

		/** Unit testing hook, does not enabled automation but tweaks some other config. */
		kUnitTesting,
	};

	MainSettings(
		const Reflection::Type& configManagerType
#if SEOUL_WITH_GAME_PERSISTENCE
		, const PersistenceSettings& gamePersistenceSettings
#endif
		)
		: m_sServerBaseURL()
		, m_ConfigManagerType(configManagerType)
#if SEOUL_WITH_GAME_PERSISTENCE
		, m_PersistenceManagerSettings(gamePersistenceSettings)
#endif
		, m_ScriptErrorHandler(SEOUL_BIND_DELEGATE(&CrashManager::DefaultErrorHandler))
		, m_InstantiatorOverride()
		, m_pConfigUpdatePackageFileSystem(nullptr)
		, m_pContentUpdatePackageFileSystem(nullptr)
		, m_sAutomationScriptMain()
		, m_eAutomatedTesting(kOff)
	{
	}

	/** URL for our Game::Server, will be used for all Game::Client communications. */
	String m_sServerBaseURL;

	/** (Required) Concrete type of the App's ConfigManager. */
	const Reflection::Type& m_ConfigManagerType;

#if SEOUL_WITH_GAME_PERSISTENCE
	/** (Required) Configuration of the App's PersistenceManager. */
	PersistenceSettings m_PersistenceManagerSettings;
#endif // /#if SEOUL_WITH_GAME_PERSISTENCE

	/** (Optional) If defined, Lua script erros will be handled by this implementation. */
	ScriptErrorHandler m_ScriptErrorHandler;

	/** (Optional) If defined, Native UI Movies will default to instantiation via this function. */
	CustomUIMovieInstantiator m_InstantiatorOverride;

	/** (Optional) Patchable config data for the current application. */
	CheckedPtr<PatchablePackageFileSystem> m_pConfigUpdatePackageFileSystem;

	/** (Optional) Patchable content data for the current application. */
	CheckedPtr<PatchablePackageFileSystem> m_pContentUpdatePackageFileSystem;

	/** (Optional) If defined, a Game::Automation singleton will be created on this main script file. Path relative to the Scripts folder. */
	String m_sAutomationScriptMain;

	/**
	 * (Optional) If defined, the Game::Automation singleton will be placed in auto testing mode, which
	 * modifies some engine behavior (frame clamping and ticking) to prepare to run in headless
	 * execution mode.
	 */
	AutomatedTesting m_eAutomatedTesting;

	/** (Optional) If false, analytics track of app launch is suppressed. true by default. */
	Bool m_bTrackAppLaunch = true;
}; // struct Game::MainSettings

class Main SEOUL_SEALED : public Singleton<Main>
{
public:
	static SaveLoadManagerSettings GetSaveLoadManagerSettings(
		MainSettings::AutomatedTesting eAutomatedTesting);

	Main(const MainSettings& settings);
	~Main();

	// Called on the main thread when we enter and leave the background
	// (iOS and Android only)
	// NOTE: Apple has a unspecified time limit for the app to give
	// up the foreground so don't do anything complicated here
	void OnEnterBackground();
	void OnLeaveBackground();

	// Same as above, including the note, but this is called only when the app is no longer visible.
	// For example a system dialog box will cause EnterBackground to be called, but not SessionEnd,
	// where as pressing the home button will cause both to be called.
	void OnSessionStart();
	void OnSessionEnd(const WorldTime& timeStamp);

	Bool IsInBackground() const
	{
		return m_bInBackground;
	}

	/**
	 * @return A read-only reference to the global *_Config.sar PackageFileSystem
	 * that supports runtime updates.
	 */
	CheckedPtr<PatchablePackageFileSystem> GetConfigUpdatePackageFileSystem() const
	{
		return m_Settings.m_pConfigUpdatePackageFileSystem;
	}

	/**
	 * @return A read-only reference to the global *_ContentUpdate.sar PackageFileSystem
	 * that supports runtime updates.
	 */
	CheckedPtr<PatchablePackageFileSystem> GetContentUpdatePackageFileSystem() const
	{
		return m_Settings.m_pContentUpdatePackageFileSystem;
	}

	// Call to run 1 frame of the game loop on the main thread. Returns
	// true if the game has not been shutdown, false otherwise.
	Bool Tick();

	/**
	 * Convenience function for platforms that use a traditional game poll
	 * loop.
	 */
	void Run()
	{
		SEOUL_ASSERT(IsMainThread());

		while (Tick()) ;
	}

	/**
	 * @return True if Game::Main thinks we're connected to the server, false otherwise.
	 */
	Bool IsConnectedToNetwork() const
	{
		return m_bIsConnectedToNetwork;
	}

	/** @return The HTTP server used by this game. */
	const String& GetServerBaseURL() const
	{
		return m_Settings.m_sServerBaseURL;
	}

	/** @return The settings used to configured GameMain. */
	const MainSettings& GetSettings() const
	{
		return m_Settings;
	}

	/*
	 * @return a non-empty string from the server to display after a 503 response
	 */
	String GetServerDownMessage() const
	{
		return m_sServerDownMessage;
	}

	void SetServerDownMessage(String sMessage)
	{
		m_sServerDownMessage = sMessage;
	}

private:
	Bool DoTick();

	MainSettings m_Settings;
#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)
	ScopedPtr<Animation::NetworkDefinitionManager> m_pAnimationNetworkDefinitionManager;
#endif // /#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)
#if SEOUL_WITH_ANIMATION_2D
	ScopedPtr<Animation2D::Manager> m_pAnimation2DManager;
#endif // /#if SEOUL_WITH_ANIMATION_2D
#if SEOUL_WITH_ANIMATION_3D
	ScopedPtr<Animation3D::Manager> m_pAnimation3DManager;
#endif // /#if SEOUL_WITH_ANIMATION_3D
	ScopedPtr<FxManager> m_pFxManager;
	ScopedPtr<UI::Manager> m_pUIManager;
#if SEOUL_ENABLE_DEV_UI
	ScopedPtr<DevUI::Root> m_pDevUI;
#endif // /SEOUL_ENABLE_DEV_UI
	ScopedPtr<Client> m_pGameClient;
	ScopedPtr<AuthManager> m_pGameAuthManager;
#if SEOUL_WITH_SCENE
	ScopedPtr<Scene::PrefabManager> m_pScenePrefabManager;
#endif // /SEOUL_WITH_SCENE
	ScopedPtr<Script::Manager> m_pScriptManager;

	// These members are "game tier". They will be destroyed and created during application
	// lifespan to apply hot patches. Members above this are "engine tier", and are created
	// once for the lifespan of the application.
	ScopedPtr<Automation> m_pAutomation;
	ScopedPtr<ConfigManager> m_pGameConfigManager;
#if SEOUL_WITH_GAME_PERSISTENCE
	ScopedPtr<PersistenceManager> m_pGamePersistenceManager;
#endif // /#if SEOUL_WITH_GAME_PERSISTENCE
	ScopedPtr<ScriptManager> m_pGameScriptManager;
	TimeInterval m_LastUptimeWithNoResends;
	Bool m_bIsConnectedToNetwork;
	Bool m_bInBackground;
	Bool m_bInSession;
	Bool m_bQuit;
	Bool m_bGDPRAcceptedCache;
	String m_sServerDownMessage;

	void RefreshGDPR();
	void UpdateHTTPResendControls();

	// Part of patcher startup flow. These are ordered in the order they are expected
	// to be called by the patcher.
	friend class Patcher;
	void PatcherFriend_ShutdownGame();
	void PatcherFriend_AcquireConfigManager(ScopedPtr<ConfigManager>& rpGameConfigManager);
#if SEOUL_WITH_GAME_PERSISTENCE
	void PatcherFriend_AcquirePersistenceManager(ScopedPtr<PersistenceManager>& rpGamePersistenceManager);
#endif // /#if SEOUL_WITH_GAME_PERSISTENCE
	void PatcherFriend_AcquireScriptManagerVm(
		const ScriptManagerSettings& settings,
		const SharedPtr<Script::Vm>& pVm);
	void PatcherFriend_PreInitializeScript();
	void PatcherFriend_PostInitializeScript();

	friend class DevUIViewServerBrowser;
	void ServerBrowserFriend_SetServerBaseURL(const String& sServerBaseUrl);

	SEOUL_DISABLE_COPY(Main);
}; // class Game::Main

} // namespace Seoul::Game

#endif // include guard
