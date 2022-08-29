/**
 * \file GameMain.cpp
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

#include "AnalyticsManager.h"
#include "AnimationNetworkDefinitionManager.h"
#if SEOUL_WITH_ANIMATION_2D
#include "Animation2DManager.h"
#endif
#include "Animation3DManager.h"
#include "ApplicationJson.h"
#include "BuildDistro.h"
#include "CommerceManager.h"
#include "Engine.h"
#include "EngineVirtuals.h"
#include "FileManager.h"
#include "FxManager.h"
#if SEOUL_WITH_FX_STUDIO
#include "FxStudioManager.h"
#endif // /#if SEOUL_WITH_FX_STUDIO
#include "GameAnalytics.h"
#include "GameAuthManager.h"
#include "GameAutomation.h"
#include "GameClient.h"
#include "GameConfigManager.h"
#include "GameDevUIViewGameUI.h"
#include "EventsManager.h"
#include "GameMain.h"
#include "GamePaths.h"
#include "GameScriptManager.h"
#include "GameScriptManagerSettings.h"
#include "HTTPManager.h"
#include "InputManager.h"
#include "LocManager.h"
#include "PackageFileSystem.h"
#include "PatchablePackageFileSystem.h"
#include "Path.h"
#include "PlatformData.h"
#include "PlatformSignInManager.h"
#include "ReflectionDataStoreTableUtil.h"
#include "ReflectionDefine.h"
#include "RenderDevice.h"
#include "Renderer.h"
#include "ScenePrefabManager.h"
#include "ScriptFunctionInterface.h"
#include "ScriptManager.h"
#include "SeoulProfiler.h"
#include "SeoulUUID.h"
#include "SettingsManager.h"
#include "SoundManager.h"
#include "ToString.h"
#include "TrackingManager.h"
#include "UIManager.h"
#include "VmStats.h"

#if SEOUL_ENABLE_DEV_UI
#include "GameDevUIRoot.h"
#endif

namespace Seoul
{

#if (SEOUL_WITH_SERVER_BROWSER && SEOUL_ENABLE_DEV_UI && !SEOUL_SHIP)
SEOUL_LINK_ME_NS(class, Game, DevUIViewServerBrowser);
#endif // /(SEOUL_WITH_SERVER_BROWSER && SEOUL_ENABLE_DEV_UI && !SEOUL_SHIP)

SEOUL_LINK_ME_NS(class, Game, FxPreview)
SEOUL_LINK_ME_NS(class, Game, Patcher)
SEOUL_LINK_ME_NS(class, Game, PatcherStatus)
#if SEOUL_WITH_SCENE
SEOUL_LINK_ME_NS(class, Game, SceneMovie);
#endif // /#if SEOUL_WITH_SCENE
SEOUL_LINK_ME_NS(class, Game, ScriptManagerProxy);
SEOUL_LINK_ME_NS(class, Game, ScriptMain);
#if (!SEOUL_SHIP || defined(SEOUL_PROFILING_BUILD))
SEOUL_LINK_ME_NS(class, Game, VideoCapture);
#endif

#if SEOUL_WITH_ANIMATION_2D
SEOUL_LINK_ME(class, ScriptAnimation2DManager);
SEOUL_LINK_ME(class, ScriptAnimation2DQuery);
#endif
SEOUL_LINK_ME(class, ScriptEngine);
SEOUL_LINK_ME(class, ScriptEngineAchievementManager);
SEOUL_LINK_ME(class, ScriptEngineAnalyticsManager);
SEOUL_LINK_ME(class, ScriptEngineCamera);
SEOUL_LINK_ME(class, ScriptEngineCommerceManager);
SEOUL_LINK_ME(class, ScriptEngineCore);
SEOUL_LINK_ME(class, ScriptEngineFileManager);
SEOUL_LINK_ME(class, ScriptEngineHTTP);
SEOUL_LINK_ME(class, ScriptEngineHTTPRequest);
SEOUL_LINK_ME(class, ScriptEngineInputManager);
SEOUL_LINK_ME(class, ScriptEngineJobManager);
SEOUL_LINK_ME(class, ScriptEngineLocManager);
SEOUL_LINK_ME(class, ScriptEnginePath);
SEOUL_LINK_ME(class, ScriptEnginePlatformSignInManager);
SEOUL_LINK_ME(class, ScriptEngineProcess);
SEOUL_LINK_ME(class, ScriptEngineRenderer);
SEOUL_LINK_ME(class, ScriptEngineSettingsManager);
SEOUL_LINK_ME(class, ScriptEngineSoundManager);
SEOUL_LINK_ME(class, ScriptEngineTrackingManager);
#if SEOUL_WITH_SCENE
SEOUL_LINK_ME(class, ScriptMotion);
SEOUL_LINK_ME(class, ScriptMotionApproach);
SEOUL_LINK_ME(class, ScriptMotionPointToMove);
#endif // /#if SEOUL_WITH_SCENE
#if SEOUL_WITH_NETWORK
SEOUL_LINK_ME(class, ScriptNetworkExtrapolator);
SEOUL_LINK_ME(class, ScriptNetworkManager);
SEOUL_LINK_ME(class, ScriptNetworkMessenger);
#endif // /#if SEOUL_WITH_NETWORK
SEOUL_LINK_ME(class, ScriptUIManager);
SEOUL_LINK_ME(class, ScriptEngineWordFilter);
SEOUL_LINK_ME_NS(class, Script, PseudoRandom);

namespace Game
{

/** Miscellaneous HString constants used for utilities and queries. */
static const HString ksEnableLetterboxImage("EnableLetterboxImage");
static const HString ksHTTPMaxResendTimeInSeconds("HTTPMaxResendTimeInSeconds");
static const HString ksHTTPRandomResendTimeInSeconds("HTTPRandomResendTimeInSeconds");

static const HString ksHTTPResendMinTimeInSeconds("HTTPResendMinTimeInSeconds");
static const HString ksHTTPResendMaxTimeInSeconds("HTTPResendMaxTimeInSeconds");
static const HString ksHTTPResendBaseMultiplier("HTTPResendBaseMultiplier");
static const HString ksHTTPResendRandomMultiplier("HTTPResendRandomMultiplier");

static const HString ksHTTPDomainRequestBudgetInitial("HTTPDomainRequestBudgetInitial");
static const HString ksHTTPDomainRequestBudgetSecondsPerIncrease("HTTPDomainRequestBudgetSecondsPerIncrease");

static const HString ksRenderConfigDefault("DefaultConfig");

static const HString kUIConditionGDPRCheckAccepted("GDPRCheckAccepted");

/**
 * HString constant for leave background UI event. We don't
 * dispatch an event for enter background, since on mobile
 * platforms we won't have enough time to actually deliver
 * it before the app stops running (the event wouldn't actually
 * be delivered until the same frame as LeaveBackground).
 */
static const HString kLeaveBackground("LeaveBackground");

/**
 * @return FilePath to gui.json.
 */
inline static FilePath GetGuiFilePath()
{
	return FilePath::CreateConfigFilePath("gui.json");
}

static void WaitForHTTPRequests(Double fWaitTimeInSeconds)
{
	// Busy wait for up to fWaitTimeInSeconds for HTTP
	// operations to complete.

	Int64 const iStartingTick = SeoulTime::GetGameTimeInTicks();
	Double fElapsedTime = 0.0f;
	while (HTTP::Manager::Get()->HasRequests() && fElapsedTime < fWaitTimeInSeconds)
	{
		HTTP::Manager::Get()->Tick();

		Int64 const iCurrentTick = SeoulTime::GetGameTimeInTicks();
		fElapsedTime = SeoulTime::ConvertTicksToSeconds(Max(iCurrentTick - iStartingTick, (Int64)0));
	}
}

namespace
{

HTTP::Request* SaveLoadManagerCreateRequest(
	const String& sURL,
	const HTTP::ResponseDelegate& callback,
	HString sMethod,
	Bool bResendOnFailure,
	Bool bSuppressErrorMail)
{
	ClientLifespanLock lock;
	if (Client::Get())
	{
		return &Client::Get()->CreateRequest(
			sURL,
			callback,
			sMethod,
			bResendOnFailure,
			bSuppressErrorMail);
	}
	else
	{
		// SaveLoadManager cannot fulfill requests
		// if we have no Game::Client, since
		// the server expects various parts of
		// the Game authentication to be
		// available.
		return nullptr;
	}
}

} // namespace anonymous

SaveLoadManagerSettings Main::GetSaveLoadManagerSettings(
	MainSettings::AutomatedTesting eAutomatedTesting)
{
	SaveLoadManagerSettings settings;
	settings.m_CreateRequest = SEOUL_BIND_DELEGATE(&SaveLoadManagerCreateRequest);
#if SEOUL_UNIT_TESTS
	settings.m_bEnableFirstTimeTests = (MainSettings::kAutomatedTesting == eAutomatedTesting);
	settings.m_bEnableValidation = (MainSettings::kAutomatedTesting == eAutomatedTesting || MainSettings::kPersistentAutomatedTesting == eAutomatedTesting);
#endif // /#if SEOUL_UNIT_TESTS
	return settings;
}

static inline UI::StackFilter GetStackFilter(const MainSettings& settings)
{
#if SEOUL_ENABLE_DEV_UI
	// If automation is enabled, use the kDevAndAutomation levels,
	// otherwise use the kDevOnly levels.
	if (settings.m_eAutomatedTesting == MainSettings::kOff)
	{
		return UI::StackFilter::kDevOnly;
	}
	else
	{
		return UI::StackFilter::kDevAndAutomation;
	}
#else
	// In Ship builds, only kAlways.
	return UI::StackFilter::kAlways;
#endif
}

#if SEOUL_ENABLE_DEV_UI
static inline UI::Manager* DevUICreateUIManager(const MainSettings& settings)
{
	if (settings.m_eAutomatedTesting == MainSettings::kOff && !g_bRunningAutomatedTests && !g_bRunningUnitTests && !g_bHeadless)
	{
		return DevUIRoot::InstantiateUIManagerInGameDevUI(GetGuiFilePath(), GetStackFilter(settings));
	}
	else
	{
		return SEOUL_NEW(MemoryBudgets::UIRuntime) UI::Manager(GetGuiFilePath(), GetStackFilter(settings));
	}
}
#endif

Main::Main(const MainSettings& settings)
	: m_Settings(settings)
#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)
	, m_pAnimationNetworkDefinitionManager(SEOUL_NEW(MemoryBudgets::Animation) Animation::NetworkDefinitionManager)
#endif // /#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)
#if SEOUL_WITH_ANIMATION_2D
	, m_pAnimation2DManager(SEOUL_NEW(MemoryBudgets::Animation2D) Animation2D::Manager)
#endif // /#if SEOUL_WITH_ANIMATION_2D
#if SEOUL_WITH_ANIMATION_3D
	, m_pAnimation3DManager(SEOUL_NEW(MemoryBudgets::Animation3D) Animation3D::Manager)
#endif // /#if SEOUL_WITH_ANIMATION_3D
#if SEOUL_WITH_FX_STUDIO
	, m_pFxManager(SEOUL_NEW(MemoryBudgets::Fx) FxStudio::Manager)
#else
	, m_pFxManager(SEOUL_NEW(MemoryBudgets::Fx) NullFxManager)
#endif // /#if SEOUL_WITH_FX_STUDIO
#if SEOUL_ENABLE_DEV_UI
	, m_pUIManager(DevUICreateUIManager(settings))
	, m_pDevUI(DevUI::Root::Get())
#else
	, m_pUIManager(SEOUL_NEW(MemoryBudgets::UIRuntime) UI::Manager(GetGuiFilePath(), GetStackFilter(settings)))
#endif
	, m_pGameClient(SEOUL_NEW(MemoryBudgets::Network) Client)
	, m_pGameAuthManager(SEOUL_NEW(MemoryBudgets::Network) AuthManager)
#if SEOUL_WITH_SCENE
	, m_pScenePrefabManager(SEOUL_NEW(MemoryBudgets::Scene) Scene::PrefabManager)
#endif // /#if SEOUL_WITH_SCENE
	, m_pScriptManager(SEOUL_NEW(MemoryBudgets::Scripting) Script::Manager)
	, m_pAutomation()
	// All singletons below this point are "game tier" and are not constructed until the patcher
	// flow completes.
	, m_pGameConfigManager()
#if SEOUL_WITH_GAME_PERSISTENCE
	, m_pGamePersistenceManager()
#endif // /#if SEOUL_WITH_GAME_PERSISTENCE
	, m_pGameScriptManager()
	, m_LastUptimeWithNoResends(Engine::Get()->GetUptime())
	, m_bIsConnectedToNetwork(true)
	, m_bInBackground(false)
	, m_bInSession(false)
	, m_bQuit(false)
	, m_bGDPRAcceptedCache(false)
{
	// Push GDPR state.
	RefreshGDPR();

	// Setup the renderer.
	HString const sRendererConfig(ksRenderConfigDefault);
	Renderer::Get()->ReadConfiguration(
		FilePath::CreateConfigFilePath(String::Printf("Renderer/Renderer%s.json", GetCurrentPlatformName())),
		sRendererConfig);

	// Create the game automation instance if specified.
	if (!m_Settings.m_sAutomationScriptMain.IsEmpty())
	{
		AutomationSettings settings;
		settings.m_sMainScriptFileName = m_Settings.m_sAutomationScriptMain;
		settings.m_bAutomatedTesting = (
			MainSettings::kAutomatedTesting == m_Settings.m_eAutomatedTesting ||
			MainSettings::kPersistentAutomatedTesting == m_Settings.m_eAutomatedTesting);
		m_pAutomation.Reset(SEOUL_NEW(MemoryBudgets::Developer) Automation(settings));
	}

	// Initial session start.
	OnSessionStart();

	// Send the OnInstall() event if this is the first run of the Engine.
	{
		PlatformData data;
		Engine::Get()->GetPlatformData(data);
		if (data.m_bFirstRunAfterInstallation)
		{
			Analytics::OnInstall();
		}
	}

	if (m_Settings.m_bTrackAppLaunch)
	{
		// Send the OnAppLaunch() event.
		Analytics::OnAppLaunch();
	}

	// Flush analytics immediately after startup events.
	AnalyticsManager::Get()->Flush();

	// Update HTTP::Manager resend controls.
	UpdateHTTPResendControls();

	// Set the crash and content contexts as running.
	CrashManager::Get()->SetCrashContext(CrashContext::kRun);
	Content::LoadManager::Get()->SetLoadContext(Content::LoadContext::kRun);
}

Main::~Main()
{
	static const Double kfHTTPShutdownTime = 1.0;

	// Set the crash and content contexts as shutdown.
	CrashManager::Get()->SetCrashContext(CrashContext::kShutdown);
	Content::LoadManager::Get()->SetLoadContext(Content::LoadContext::kShutdown);

	// Make sure we are no longer in the background prior to teardown.
	OnLeaveBackground();

	// Last session end.
	OnSessionEnd(m_pGameClient->GetCurrentServerTime()); SEOUL_TEARDOWN_TRACE();

	// Give HTTP requests some time to complete prior to
	// tearing most systems down.
	WaitForHTTPRequests(kfHTTPShutdownTime); SEOUL_TEARDOWN_TRACE();

	// TODO: Need to bottle up thread safety of UIManager. It's
	// not a problem in general due to most structures being populate once
	// and then being left populated, but it can still be surprising in cases like
	// shutdown.
	UI::Manager::Get()->ShutdownPrep(); SEOUL_TEARDOWN_TRACE();

	// Disable network file IO before further processing, we don't want
	// calls to WaitUntilAllLoadsAreFinished() to content manager with
	// network file IO still active.
	FileManager::Get()->DisableNetworkFileIO(); SEOUL_TEARDOWN_TRACE();

	// Allow any in progress content operations to complete - this
	// avoids crashes due to mutations from within UI::Manager::Get()->Clear()
	//
	// Also, wait for content loads to finish, make sure
	// content references are free before shutdown.
	Content::LoadManager::Get()->WaitUntilAllLoadsAreFinished(); SEOUL_TEARDOWN_TRACE();

	// Clear the UI system.
	UI::Manager::Get()->ShutdownComplete(); SEOUL_TEARDOWN_TRACE();

	// Tell game scripting we're about to shutdown.
	if (m_pGameScriptManager.IsValid())
	{
		m_pGameScriptManager->PreShutdown(); SEOUL_TEARDOWN_TRACE();
	}

	// If automation is active, tell it we're about to shut down
	// (this happens before script termination to allow leak
	// checks and other niceness).
	if (m_pAutomation.IsValid())
	{
		m_pAutomation->PreShutdown(); SEOUL_TEARDOWN_TRACE();
	}

	// Wait for content loads to finish, make sure
	// content references are free before shutdown.
	Content::LoadManager::Get()->WaitUntilAllLoadsAreFinished(); SEOUL_TEARDOWN_TRACE();

	// Give HTTP requests some time to complete now that we've finished
	// tearing most systems down.
	WaitForHTTPRequests(kfHTTPShutdownTime); SEOUL_TEARDOWN_TRACE();

	// Shutdown the renderer.
	Renderer::Get()->ClearConfiguration(); SEOUL_TEARDOWN_TRACE();

	// Cleanup global singletons - cancel any game requests
	// prior to destruction of systems.
	m_pGameClient->CancelPendingRequests(); SEOUL_TEARDOWN_TRACE();
	m_pGameScriptManager.Reset(); SEOUL_TEARDOWN_TRACE();
#if SEOUL_WITH_GAME_PERSISTENCE
	m_pGamePersistenceManager.Reset(); SEOUL_TEARDOWN_TRACE();
#endif // /#if SEOUL_WITH_GAME_PERSISTENCE
	m_pGameConfigManager.Reset(); SEOUL_TEARDOWN_TRACE();
	m_pAutomation.Reset(); SEOUL_TEARDOWN_TRACE();
	m_pScriptManager.Reset(); SEOUL_TEARDOWN_TRACE();
#if SEOUL_WITH_SCENE
	m_pScenePrefabManager.Reset(); SEOUL_TEARDOWN_TRACE();
#endif // /#if SEOUL_WITH_SCENE

	// Wait for content loads to finish, make sure
	// content references are free before shutdown.
	Content::LoadManager::Get()->WaitUntilAllLoadsAreFinished(); SEOUL_TEARDOWN_TRACE();

	m_pGameAuthManager.Reset(); SEOUL_TEARDOWN_TRACE();
	m_pGameClient.Reset(); SEOUL_TEARDOWN_TRACE();
#if SEOUL_ENABLE_DEV_UI
	m_pDevUI.Reset(); SEOUL_TEARDOWN_TRACE();
#endif // /SEOUL_ENABLE_DEV_UI
	m_pUIManager.Reset(); SEOUL_TEARDOWN_TRACE();
	m_pFxManager.Reset(); SEOUL_TEARDOWN_TRACE();
#if SEOUL_WITH_ANIMATION_3D
	m_pAnimation3DManager.Reset(); SEOUL_TEARDOWN_TRACE();
#endif // /#if SEOUL_WITH_ANIMATION_3D
#if SEOUL_WITH_ANIMATION_2D
	m_pAnimation2DManager.Reset(); SEOUL_TEARDOWN_TRACE();
#endif // /#if SEOUL_WITH_ANIMATION_2D
#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)
	m_pAnimationNetworkDefinitionManager.Reset(); SEOUL_TEARDOWN_TRACE();
#endif // /#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)
}

/**
 * Called on the main thread when we enter and leave the background
 * (iOS and Android only)
 * NOTE: Apple has a unspecified time limit for the app to give
 * up the foreground so don't do anything complicated here
 */
void Main::OnEnterBackground()
{
	SEOUL_ASSERT(IsMainThread());

	// Filter redundant calls.
	if (m_bInBackground)
	{
		return;
	}

	// Log for testing and debug tracking.
	SEOUL_LOG("GameMain::OnEnterBackground(): Start");

	// Dispatch to App code.
	g_pEngineVirtuals->OnEnterBackground();

	// Common engine handling.
	{
		// TODO: Push OnLeaveBackground() engine-level handling into Engine.

		Engine::Get()->PauseTickTimer();

		// Inform sound.
		if (Sound::Manager::Get())
		{
			Sound::Manager::Get()->OnEnterBackground();
		}

		// Inform rendering.
		if (RenderDevice::Get())
		{
			RenderDevice::Get()->OnEnterBackground();
		}

		// Let Analytics go to sleep.
		if (AnalyticsManager::Get())
		{
			AnalyticsManager::Get()->OnEnterBackground();
		}

		// Let the HTTP::Manager go to sleep.
		if (HTTP::Manager::Get())
		{
			HTTP::Manager::Get()->OnEnterBackground();
		}

		// Let the Jobs::Manager go to sleep.
		if (Jobs::Manager::Get())
		{
			Jobs::Manager::Get()->OnEnterBackground();
		}
	}

	// Log for testing and debug tracking.
	SEOUL_LOG("GameMain::OnEnterBackground(): Done");

	// Now in the background.
	m_bInBackground = true;
}

void Main::OnLeaveBackground()
{
	SEOUL_ASSERT(IsMainThread());

	// Filter redundant calls.
	if (!m_bInBackground)
	{
		return;
	}

	// Log for testing and debug tracking.
	SEOUL_LOG("GameMain::OnLeaveBackground(): Start");

	// No longer in the background.
	m_bInBackground = false;

	// Common engine handling.
	{
		// TODO: Push OnLeaveBackground() engine-level handling into Engine.

		// Wake up the Jobs::Manager.
		if (Jobs::Manager::Get())
		{
			Jobs::Manager::Get()->OnLeaveBackground();
		}

		// Wake up the HTTP::Manager.
		if (HTTP::Manager::Get())
		{
			HTTP::Manager::Get()->OnLeaveBackground();
		}

		// Wake up analytics.
		if (AnalyticsManager::Get())
		{
			AnalyticsManager::Get()->OnLeaveBackground();
		}

		// Wake up rendering.
		if (RenderDevice::Get())
		{
			RenderDevice::Get()->OnLeaveBackground();
		}

		// Wake up sound.
		if (Sound::Manager::Get())
		{
			Sound::Manager::Get()->OnLeaveBackground();
		}

		Engine::Get()->UnPauseTickTimer();
	}

	// Dispatch to App code.
	g_pEngineVirtuals->OnLeaveBackground();

	// Let the UI system know we just resumed from sleep.
	UI::Manager::Get()->TriggerTransition(kLeaveBackground);

	// Log for testing and debug tracking.
	SEOUL_LOG("GameMain::OnLeaveBackground(): Done");
}

/**
 * Same as above, including the note, but this is called only when the app is no longer visible.
 * For example a system dialog box will cause EnterBackground to be called, but not SessionEnd,
 * where as pressing the home button will cause both to be called.
 */
void Main::OnSessionStart()
{
	// Filter redundant calls.
	if (m_bInSession)
	{
		return;
	}

	// Log for testing and debug tracking.
	SEOUL_LOG("GameMain::OnSessionStart(): Start");

	// Make sure we refresh uptime for code inside SessionStart() handlers.
	Engine::Get()->RefreshUptime();

	if (AnalyticsManager::Get())
	{
		AnalyticsManager::Get()->TrackSessionStart();
	}

	PlatformSignInManager::Get()->OnSessionStart();

	// Refresh changing auth data.
	AuthManager::Get()->Refresh();

	g_pEngineVirtuals->OnSessionStart(Client::StaticGetCurrentServerTime());

	// Now in a session.
	m_bInSession = true;

	// Log for testing and debug tracking.
	SEOUL_LOG("GameMain::OnSessionStart(): Done");
}

void Main::OnSessionEnd(const WorldTime& timeStamp)
{
	// Filter redundant calls.
	if (!m_bInSession)
	{
		return;
	}

	// Log for testing and debug tracking.
	SEOUL_LOG("GameMain::OnSessionEnd(): Start");

	// No longer in a session.
	m_bInSession = false;

	g_pEngineVirtuals->OnSessionEnd(timeStamp);

	PlatformSignInManager::Get()->OnSessionEnd();

	if (AnalyticsManager::Get())
	{
		AnalyticsManager::Get()->TrackSessionEnd(timeStamp);
	}

	// Log for testing and debug tracking.
	SEOUL_LOG("GameMain::OnSessionEnd(): Done");
}

Bool Main::Tick()
{
	// Early out if we have previously quit.
	if (m_bQuit)
	{
		return false;
	}

	// Run the actual Tick handling.
	m_bQuit = !DoTick();

	// True if we're still running, false otherwise.
	return !m_bQuit;
}

Bool Main::DoTick()
{
	SEOUL_PROF("FrameTotal");

	// Time in seconds that resend requests are allowed to be pending before we consider
	// the game disconnected/unable to connect.
	static Float const kfPendingResendRequestMaxTimeInSeconds = 15.0f;

	// Propagate connection status to the UI system.
	static const HString ksIsConnectedToNetwork("IsConnectedToNetwork");

	SEOUL_ASSERT(IsMainThread());

	// Hot load tracking.
	{
		static const HString ksCancelHotLoad("CancelHotLoad");
		static const HString ksHotLoad("HotLoad");

		if (InputManager::Get()->WasBindingPressed(ksHotLoad))
		{
			Content::LoadManager::Get()->SetHoadLoadMode(Content::LoadManagerHotLoadMode::kAccept);
		}
		else if (InputManager::Get()->WasBindingPressed(ksCancelHotLoad))
		{
			Content::LoadManager::Get()->SetHoadLoadMode(Content::LoadManagerHotLoadMode::kReject);
		}
	}

	// Pre tick automation if defined.
	if (Automation::Get())
	{
		SEOUL_PROF("GameAutomation.PreTick");

		// Quit immediately if automation returned false.
		if (!Automation::Get()->PreTick())
		{
			return false;
		}
	}

	// Return value from possible branches below.
	Bool bTickResult = false;

	// We time the frame as the portion of the total
	// frame that excludes automation pre and post.
	{
		SEOUL_PROF("Frame");

		// Tick if we're not shutting down.
		Bool bEngineTick = false;
		{
			SEOUL_PROF("Engine.Tick");
			bEngineTick = Engine::Get()->Tick();
		}
		if (bEngineTick)
		{
			// TODO: As this is being simplified back down, probably makes
			// sense to push these back into Engine. They're here from legacy
			// engine code. At that time the tick loop was threaded and more complex
			// and was handled outside Engine so that different applications could
			// coordinate the threads uniquely.
			Float const fDeltaTimeInSeconds = Engine::Get()->GetSecondsInTick();

			// Tick auth.
			if (m_pGameAuthManager.IsValid())
			{
				SEOUL_PROF("GameAuth.Update");
				m_pGameAuthManager->Update();
			}

#if SEOUL_WITH_GAME_PERSISTENCE
			// Tick persistence.
			if (m_pGamePersistenceManager.IsValid())
			{
				SEOUL_PROF("GamePersistence.Update");

				// Conditional - if loading on the patcher, don't want to force a lock here.
				PersistenceTryLock tryLock;
				if (tryLock.IsLocked())
				{
					m_pGamePersistenceManager->Update();
				}
			}
#endif // /#if SEOUL_WITH_GAME_PERSISTENCE

			// Refresh GPDR state.
			RefreshGDPR();

			// Tick event.
			Events::Manager::Get()->TriggerEvent(MainTickEventID, fDeltaTimeInSeconds);

			// Tick scripting.
			if (m_pGameScriptManager.IsValid())
			{
				SEOUL_PROF("GameScript.Tick");

#if SEOUL_WITH_GAME_PERSISTENCE
				// Required persistence lock here.
				PersistenceLock lock;
#endif // /#if SEOUL_WITH_GAME_PERSISTENCE
				m_pGameScriptManager->Tick();
			}

			// Update the connected to network flag
			auto const uptime = Engine::Get()->GetUptime();
			if (!(HTTP::Manager::Get()->GetNetworkFailureActiveResendRequests() > 0))
			{
				m_LastUptimeWithNoResends = uptime;
			}

			auto const fTimeWithResendsInSeconds = (uptime - m_LastUptimeWithNoResends).GetSecondsAsDouble();
			m_bIsConnectedToNetwork = (fTimeWithResendsInSeconds <= kfPendingResendRequestMaxTimeInSeconds);
			UI::Manager::Get()->SetCondition(ksIsConnectedToNetwork, m_bIsConnectedToNetwork);

#if SEOUL_WITH_ANIMATION_2D
			{
				SEOUL_PROF("Animation2D.Tick");
				Animation2D::Manager::Get()->Tick(fDeltaTimeInSeconds);
			}
#endif // /#if SEOUL_WITH_ANIMATION_2D

			{
				SEOUL_PROF("FxManager.Tick");
				FxManager::Get()->Tick(fDeltaTimeInSeconds);
			}

			{
				SEOUL_PROF("Renderer.Pose");
				// Required persistence lock if scripting exists.
				if (m_pGameScriptManager.IsValid())
				{
#if SEOUL_WITH_GAME_PERSISTENCE
					PersistenceLock lock;
#endif // /#if SEOUL_WITH_GAME_PERSISTENCE
					Renderer::Get()->Pose(fDeltaTimeInSeconds);
				}
				// Otherwise, we don't lock, to avoid contention
				// with asynchronous script creation.
				else
				{
					Renderer::Get()->Pose(fDeltaTimeInSeconds);
				}
			}

			{
				SEOUL_PROF("Sound.Tick");
				Sound::Manager::Get()->Tick(fDeltaTimeInSeconds);
			}

			{
				SEOUL_PROF("Renderer.Render");
				Renderer::Get()->Render(fDeltaTimeInSeconds);
			}

			bTickResult = true;
		}
		else
		{
			bTickResult = false;
		}
	}

	// If still ticking and Game::Automation is valid, perform
	// a post tick operation.
	if (bTickResult && Automation::Get())
	{
		SEOUL_PROF("GameAutomation.PostTick");

		// Quit immediately if automation returned false.
		if (!Automation::Get()->PostTick())
		{
			bTickResult = false;
		}
	}

	return bTickResult;
}

/**
 * Call to propagate GDPR state to the UIManager.
 */
void Main::RefreshGDPR()
{
	// Once GDPR is accepted, just cache this as true
	if (!m_bGDPRAcceptedCache)
	{
		m_bGDPRAcceptedCache = Engine::Get()->GetGDPRAccepted();
	}
	UI::Manager::Get()->SetCondition(kUIConditionGDPRCheckAccepted, m_bGDPRAcceptedCache);
}

/**
 * Push configurable HTTP resend controls to HTTP::Manager.
 */
void Main::UpdateHTTPResendControls()
{
	Float fHTTPResendMinTimeInSeconds = 0.0f;
	Float fHTTPResendMaxTimeInSeconds = 0.0f;
	Float fHTTPResendBaseMultiplier = 0.0f;
	Float fHTTPResendRandomMultiplier = 0.0f;

	if (GetApplicationJsonValue(ksHTTPResendMinTimeInSeconds, fHTTPResendMinTimeInSeconds) &&
		GetApplicationJsonValue(ksHTTPResendMaxTimeInSeconds, fHTTPResendMaxTimeInSeconds) &&
		GetApplicationJsonValue(ksHTTPResendBaseMultiplier, fHTTPResendBaseMultiplier) &&
		GetApplicationJsonValue(ksHTTPResendRandomMultiplier, fHTTPResendRandomMultiplier))
	{
		HTTP::Manager::Get()->SetResendSettings(fHTTPResendMinTimeInSeconds, fHTTPResendMaxTimeInSeconds, fHTTPResendBaseMultiplier, fHTTPResendRandomMultiplier);
	}

	Int iInitialBudget = 0;
	Int iSecondsPerIncrease = 0;
	if (GetApplicationJsonValue(ksHTTPDomainRequestBudgetInitial, iInitialBudget) &&
		GetApplicationJsonValue(ksHTTPDomainRequestBudgetSecondsPerIncrease, iSecondsPerIncrease))
	{
		HTTP::Manager::Get()->SetDomainRequestBudgetSettings(iInitialBudget, iSecondsPerIncrease);
	}
}

/**
 * Called by the patcher to release "game tier" members. This includes
 * configuration, persistence, and scripting.
 */
void Main::PatcherFriend_ShutdownGame()
{
	// Script VM release, followed by persistence, and then config. Need
	// to release suspended as well since we're about to discard their VM.
	UI::Manager::Get()->ClearSuspended();
	m_pGameScriptManager.Reset();
#if SEOUL_WITH_GAME_PERSISTENCE
	m_pGamePersistenceManager.Reset();
#endif // /#if SEOUL_WITH_GAME_PERSISTENCE
	m_pGameConfigManager.Reset();

	m_pGameAuthManager->Main_ResetAuth();
}

/**
 * Called by the patcher to pass through the instantiated config manager.
 */
void Main::PatcherFriend_AcquireConfigManager(ScopedPtr<ConfigManager>& rpGameConfigManager)
{
	m_pGameConfigManager.Swap(rpGameConfigManager);
}

#if SEOUL_WITH_GAME_PERSISTENCE
/**
 * Called by the patcher to pass through the instantiated persistence manager.
 */
void Main::PatcherFriend_AcquirePersistenceManager(ScopedPtr<PersistenceManager>& rpGamePersistenceManager)
{
	m_pGamePersistenceManager.Swap(rpGamePersistenceManager);
}
#endif // /#if SEOUL_WITH_GAME_PERSISTENCE

/**
 * Called by the patcher to pass through the instantiated script vm for the game.
 */
void Main::PatcherFriend_AcquireScriptManagerVm(
	const ScriptManagerSettings& settings,
	const SharedPtr<Script::Vm>& pVm)
{
	m_pGameScriptManager.Reset(SEOUL_NEW(MemoryBudgets::Scripting) ScriptManager(settings, pVm));
}

/**
 * Called by the patcher on initialize, just before Game::Script is constructed.
 */
void Main::PatcherFriend_PreInitializeScript()
{
	SEOUL_ASSERT(IsMainThread());

	// NOTE: Operations in this body must be repeatable.
	// The method can be invoked more than once
	// (without corresponding PostInitializeScript() or
	// Game::Shutdown()) depending on patcher state
	// traversal.

	{
		// Update the additional version string used by analytics to the new data config version.
		String const sSubVersionString((GetConfigUpdatePackageFileSystem().IsValid() ? ToString(GetConfigUpdatePackageFileSystem()->GetBuildChangelist()) : String()) +
			((GetConfigUpdatePackageFileSystem().IsValid() && GetContentUpdatePackageFileSystem().IsValid()) ? "." : String()) +
			(GetContentUpdatePackageFileSystem().IsValid() ? ToString(GetContentUpdatePackageFileSystem()->GetBuildChangelist()) : String()));

		AnalyticsManager::Get()->SetSubVersionString(sSubVersionString);
	}

	// Suspend settings unloading while we're re-initializing the game layer.
	SettingsManager::Get()->BeginUnloadSuppress();

	// Update HTTP::Manager resend timeouts.
	UpdateHTTPResendControls();

	// Reinitialize commerce
	CommerceManager::Get()->ReloadItemInfoTable();

	// Resume unloading.
	SettingsManager::Get()->EndUnloadSuppress();

#if SEOUL_WITH_GAME_PERSISTENCE
	// Apply sound settings from persistence
	Sound::Settings soundSettings;
	m_pGamePersistenceManager->GetSoundSettings(soundSettings);
	Sound::Manager::Get()->ApplySoundSettings(soundSettings);
#endif // /#if SEOUL_WITH_GAME_PERSISTENCE
}

/**
 * Called by the patcher on initialize, just after Game::Script is constructed.
 */
void Main::PatcherFriend_PostInitializeScript()
{
	ScriptManager::Get()->OnScriptInitializeComplete();
}

void Main::ServerBrowserFriend_SetServerBaseURL(const String& sServerBaseUrl)
{
#if SEOUL_WITH_GAME_PERSISTENCE
	{
		auto& s = m_Settings.m_PersistenceManagerSettings;
		s.m_sCloudLoadURL = s.m_sCloudLoadURL.ReplaceAll(m_Settings.m_sServerBaseURL, sServerBaseUrl);
		s.m_sCloudResetURL = s.m_sCloudResetURL.ReplaceAll(m_Settings.m_sServerBaseURL, sServerBaseUrl);
		s.m_sCloudSaveURL = s.m_sCloudSaveURL.ReplaceAll(m_Settings.m_sServerBaseURL, sServerBaseUrl);
	}
#endif

	auto pGameClient = Client::Get();
	if (pGameClient.IsValid())
	{
		pGameClient->SetAuthToken(String());
	}

	m_Settings.m_sServerBaseURL = sServerBaseUrl;
}

} // namespace Game

// Developer only functionality.
#if SEOUL_ENABLE_CHEATS
namespace Game
{

class MainCommandInstance SEOUL_SEALED
{
public:
	MainCommandInstance()
	{
	}

	~MainCommandInstance()
	{
	}

	void RefreshAuth()
	{
		if (AuthManager::Get())
		{
			AuthManager::Get()->Refresh();
		}
	}

#if SEOUL_WITH_GAME_PERSISTENCE
	void ResetSave()
	{
		// Trigger this first, as it enqueues a deletion on next load.
		SaveLoadManager::Get()->QueueSaveReset(
			Main::Get()->GetSettings().m_PersistenceManagerSettings.m_FilePath,
			Main::Get()->GetSettings().m_PersistenceManagerSettings.m_sCloudResetURL,
			false);

		// Trigger a restart - will occur immediately, force.
		UI::Manager::Get()->TriggerRestart(true);
	}

	void ResetPlayer()
	{
		// Tell the server to reset all the player's data
		auto const sURL(String::Printf("%s/v1/cheat/reset_player", Main::Get()->GetServerBaseURL().CStr()));
		auto& r = Client::Get()->CreateRequest(
			sURL,
			SEOUL_BIND_DELEGATE(OnResetPlayer),
			HTTP::Method::ksPost,
			true);
		r.Start();
	}

	static HTTP::CallbackResult OnResetPlayer(HTTP::Result eResult, HTTP::Response* pResponse)
	{
		if (eResult != HTTP::Result::kSuccess || pResponse->GetStatus() >= 500) {
			SEOUL_WARN("Error resetting player, see log");
			return HTTP::CallbackResult::kNeedsResend;
		}
		else if (pResponse->GetStatus() >= 400)
		{
			SEOUL_WARN("Error resetting player, see log");
			return HTTP::CallbackResult::kSuccess;
		}

		// Trigger this first, as it enqueues a deletion on next load.
		SaveLoadManager::Get()->QueueSaveReset(
			Main::Get()->GetSettings().m_PersistenceManagerSettings.m_FilePath,
			Main::Get()->GetSettings().m_PersistenceManagerSettings.m_sCloudResetURL,
			true);

		// Trigger a restart - will occur immediately, force.
		UI::Manager::Get()->TriggerRestart(true);

		return HTTP::CallbackResult::kSuccess;
	}
#endif // /#if SEOUL_WITH_GAME_PERSISTENCE

	void TakeScreenshot()
	{
		if (DevUIViewGameUI::Get())
		{
			DevUIViewGameUI::Get()->TakeScreenshot(DevUI::GetDevUIConfig().m_ScreenshotConfig);
		}
	}

	void TestAuthChoosePlayer()
	{
		AuthManager::Get()->DevOnlyFakeAuthConflict();
	}

	void TestRecommendedUpdate()
	{
		AuthManager::Get()->DevOnlyFakeRecommendedUpdate();
	}

	void ToggleRequiredUpdate()
	{
		AuthManager::Get()->DevOnlyToggleFakeRequiredUpdate();

		// Trigger a restart - will occur "eventually", but can be delayed
		// by UI state (passing true forces an immediate restart).
		UI::Manager::Get()->TriggerRestart(false);
	}

private:
	SEOUL_DISABLE_COPY(MainCommandInstance);
}; // class Game::MainCommandInstance

} // namespace Game

SEOUL_BEGIN_TYPE(Game::MainCommandInstance, TypeFlags::kDisableCopy)
	SEOUL_ATTRIBUTE(CommandsInstance)
	SEOUL_METHOD(RefreshAuth)
		SEOUL_ATTRIBUTE(Category, "Server")
		SEOUL_ATTRIBUTE(Description,
			"Ping the server to refresh changing auth data.")
		SEOUL_ATTRIBUTE(DisplayName, "Refresh Auth")
	SEOUL_METHOD(TakeScreenshot)
		SEOUL_ATTRIBUTE(Category, "Screenshot")
		SEOUL_ATTRIBUTE(DisplayName, "Take")
	SEOUL_METHOD(TestAuthChoosePlayer)
		SEOUL_ATTRIBUTE(Category, "Server")
		SEOUL_ATTRIBUTE(Description,
			"Generate a fake auth conflict. This will present the user flow\n"
			"that asks the user to choose between a player associated with\n"
			"platform auth (e.g. Game Center or Google Play) and a local\n"
			"player saved on/associated with the current device.")
		SEOUL_ATTRIBUTE(DisplayName, "Test Auth Choose Player")
	SEOUL_METHOD(TestRecommendedUpdate)
		SEOUL_ATTRIBUTE(Category, "Server")
		SEOUL_ATTRIBUTE(Description,
			"Fake a recommended update to display recommended upate flow.")
		SEOUL_ATTRIBUTE(DisplayName, "Test Recommended Update")
	SEOUL_METHOD(ToggleRequiredUpdate)
		SEOUL_ATTRIBUTE(Category, "Server")
		SEOUL_ATTRIBUTE(Description,
			"Fake a required update to display required upate flow.")
		SEOUL_ATTRIBUTE(DisplayName, "Toggle Required Update")
#if SEOUL_WITH_GAME_PERSISTENCE
	SEOUL_METHOD(ResetSave)
		SEOUL_ATTRIBUTE(Category, "Saving")
		SEOUL_ATTRIBUTE(Description,
			"Reset the save to its default state. Use with caution, cannot undo.")
		SEOUL_ATTRIBUTE(DisplayName, "Reset Local Save")
	SEOUL_METHOD(ResetPlayer)
		SEOUL_ATTRIBUTE(Category, "Saving")
		SEOUL_ATTRIBUTE(Description,
			"Reset the save to its default state, then disconnects from the server player data. Use with caution, cannot undo.")
		SEOUL_ATTRIBUTE(DisplayName, "Reset Local Save and Server Data")
#endif // /#if SEOUL_WITH_GAME_PERSISTENCE
SEOUL_END_TYPE()

#endif // /#if SEOUL_ENABLE_CHEATS

} // namespace Seoul
