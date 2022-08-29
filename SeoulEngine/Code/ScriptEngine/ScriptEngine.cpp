/**
 * \file ScriptEngine.cpp
 * \brief Binder instance for exposing the global
 * Engine singleton instance into a script VM.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Engine.h"
#include "FileManager.h"
#include "GameAutomation.h" // TODO: Up reference, not meant for the ScriptEngine project.
#include "HTTPManager.h"
#include "PlatformData.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"
#include "SeoulProcess.h"
#include "ScriptEngine.h"
#include "ScriptFunctionInterface.h"
#include "GameClient.h" // TODO: Up reference, not meant for the ScriptEngine project.

namespace Seoul
{

SEOUL_BEGIN_TYPE(ScriptEngine, TypeFlags::kDisableCopy)
	SEOUL_METHOD(GetGameTimeInTicks)
	SEOUL_METHOD(GetGameTimeInSeconds)
	SEOUL_METHOD(GetTimeInSecondsSinceFrameStart)
	SEOUL_METHOD(GetPlatformUUID)
	SEOUL_METHOD(GetPlatformData)
	SEOUL_METHOD(GetSecondsInTick)
	SEOUL_METHOD(GetThisProcessId)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "double?")
	SEOUL_METHOD(HasNativeBackButtonHandling)
	SEOUL_METHOD(NetworkPrefetch)
	SEOUL_METHOD(OpenURL)
	SEOUL_METHOD(PostNativeQuitMessage)
	SEOUL_METHOD(HasEnabledRemoteNotifications)
	SEOUL_METHOD(CanRequestRemoteNotificationsWithoutPrompt)
	SEOUL_METHOD(SetCanRequestRemoteNotificationsWithoutPrompt)
	SEOUL_METHOD(URLEncode)
	SEOUL_METHOD(IsAutomationOrUnitTestRunning)
	SEOUL_METHOD(ScheduleLocalNotification)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "void", "double iNotificationID, WorldTime fireDate, string sLocalizedMessage, string sLocalizedAction")
	SEOUL_METHOD(CancelLocalNotification)
	SEOUL_METHOD(CancelAllLocalNotifications)
	SEOUL_METHOD(GetCurrentServerTime)
	SEOUL_METHOD(ShowAppStoreToRateThisApp)
	SEOUL_METHOD(DoesSupportInAppRateMe)
	SEOUL_METHOD(WriteToClipboard)
	SEOUL_METHOD(GetGameSecondsSinceStartup)
SEOUL_END_TYPE()

ScriptEngine::ScriptEngine()
{
}

ScriptEngine::~ScriptEngine()
{
}

/** @return the current game time in ticks. */
Int64 ScriptEngine::GetGameTimeInTicks() const
{
	return SeoulTime::GetGameTimeInTicks();
}

/** @return the current game time in seconds. */
Double ScriptEngine::GetGameTimeInSeconds() const
{
	return SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks());
}

/** @return the game time in seconds since the start of the frame. */
Double ScriptEngine::GetTimeInSecondsSinceFrameStart() const
{
	return SeoulTime::ConvertTicksToSeconds(Max(
		SeoulTime::GetGameTimeInTicks() - Engine::Get()->FrameStartTicks(),
		(Int64)0));
}

/** @return the current UUID, platform specific. */
String ScriptEngine::GetPlatformUUID() const
{
	return Engine::Get()->GetPlatformUUID();
}

/** @return the current device's platform data: platform specific. */
PlatformData ScriptEngine::GetPlatformData() const
{
	PlatformData data;
	Engine::Get()->GetPlatformData(data);
	return data;
}

/** @return the current frame's delta time in seconds. */
Float ScriptEngine::GetSecondsInTick() const
{
	return Engine::Get()->GetSecondsInTick();
}

Bool ScriptEngine::IsAutomationOrUnitTestRunning() const
{
	return g_bRunningAutomatedTests ||
		g_bRunningUnitTests ||
		g_bHeadless ||
		Game::Automation::Get().IsValid(); // TODO: Up reference, not meant for the ScriptEngine project.
}

void ScriptEngine::ShowAppStoreToRateThisApp()
{
	Engine::Get()->ShowAppStoreToRateThisApp();
}

Bool ScriptEngine::DoesSupportInAppRateMe() const
{
	return Engine::Get()->DoesSupportInAppRateMe();
}

Bool ScriptEngine::WriteToClipboard(const String& sArgumentName) const
{
	return Engine::Get()->WriteToClipboard(sArgumentName);
}

/** @return The current process ID. */
void ScriptEngine::GetThisProcessId(Script::FunctionInterface* pInterface) const
{
	Int32 iReturn = 0;
	if (Process::GetThisProcessId(iReturn))
	{
		pInterface->PushReturnInteger(iReturn);
	}
	else
	{
		pInterface->PushReturnNil();
	}
}

/**
 * Get whether the current platform
 * has native/default back button handling.
 */
Bool ScriptEngine::HasNativeBackButtonHandling() const
{
	return Engine::Get()->HasNativeBackButtonHandling();
}

/**
 * Tell the FileManager to start downloading filePath,
 * if it will be network serviced.
 */
Bool ScriptEngine::NetworkPrefetch(FilePath filePath)
{
	// We use medium priority, so that explicitly requested prefetch operations
	// are lower priority than kDefault (the priority of any fetch that occurs
	// as part of normal file IO) but is higher priority than kLow (the priority
	// of audio prefetch operations).
	return FileManager::Get()->NetworkPrefetch(filePath, NetworkFetchPriority::kMedium);
}

/** Script hook to call Engine::Get()->OpenURL(). */
Bool ScriptEngine::OpenURL(const String& sURL)
{
	return Engine::Get()->OpenURL(sURL);
}

/** Script hook to call Engine::Get()->ScheduleLocalNotification()
 *  And parse function interface into parameters
	Int iNotificationID,
	const WorldTime& fireDate,
	Bool bIsWallClockTime,
	const String& sLocalizedMessage,
	Bool bHasActionButton,
	const String& sLocalizedActionButtonText,
	const String& sLaunchImageFilePath,
	const String& sSoundFilePath,
	Int nIconBadgeNumber,
	const DataStore& userInfo
 */
void ScriptEngine::ScheduleLocalNotification(Script::FunctionInterface* pInterface)
{
	Int iNotificationID;
	if (!pInterface->GetInteger(1, iNotificationID))
	{
		pInterface->RaiseError(1, "Expected iNotificationID to be of type Int");
		return;
	}

	WorldTime* pFireData = pInterface->GetUserData<WorldTime>(2);
	if (nullptr == pFireData)
	{
		pInterface->RaiseError(2, "Expected worldTime to be a userdata WorldTime");
		return;
	}

	String sLocalizedMessage;
	if (!pInterface->GetString(3, sLocalizedMessage))
	{
		pInterface->RaiseError(3, "Expected sLocalizedMessage to be of type String");
		return;
	}

	String sLocalizedAction;
	if (!pInterface->GetString(4, sLocalizedAction))
	{
		pInterface->RaiseError(4, "Expected sLocalizedAction to be of type String");
		return;
	}

	DataStore emptyDataStore;
	Engine::Get()->ScheduleLocalNotification(iNotificationID,
		*pFireData,
		false,
		sLocalizedMessage,
		true,
		sLocalizedAction,
		"",
		"",
		0,
		emptyDataStore);
}

/** Script hook to call Engine::Get()->CancelLocalNotification() */
void ScriptEngine::CancelLocalNotification(Int iNotificationID)
{
	Engine::Get()->CancelLocalNotification(iNotificationID);
}

/** Script hook to call Engine::Get()->CancelAllLocalNotifications() */
void ScriptEngine::CancelAllLocalNotifications()
{
	Engine::Get()->CancelAllLocalNotifications();
}

WorldTime ScriptEngine::GetCurrentServerTime() const
{
	if (Game::Client::Get())
	{
		return Game::Client::Get()->GetCurrentServerTime(); // TODO: Up reference, not meant for the ScriptEngine project.
	}
	else
	{
		return WorldTime::GetUTCTime();
	}
}

/**
 * Trigger native "back button" handling for the
 * current platform. Typically, this causes
 * the App to exit.
 */
Bool ScriptEngine::PostNativeQuitMessage()
{
	return Engine::Get()->PostNativeQuitMessage();
}

Bool ScriptEngine::HasEnabledRemoteNotifications()
{
#if SEOUL_WITH_REMOTE_NOTIFICATIONS
	return Engine::Get()->HasEnabledRemoteNotifications();
#else
	return false;
#endif // /#if SEOUL_WITH_REMOTE_NOTIFICATIONS
}

Bool ScriptEngine::CanRequestRemoteNotificationsWithoutPrompt()
{
#if SEOUL_WITH_REMOTE_NOTIFICATIONS
	return Engine::Get()->CanRequestRemoteNotificationsWithoutPrompt();
#else
	// Returning true means the game doesn't try to tell you about the notification dialog
	return true;
#endif // /SEOUL_WITH_REMOTE_NOTIFICATIONS
}

void ScriptEngine::SetCanRequestRemoteNotificationsWithoutPrompt(Bool canRequest)
{
#if SEOUL_WITH_REMOTE_NOTIFICATIONS
	Engine::Get()->SetCanRequestRemoteNotificationsWithoutPrompt(canRequest);
	Game::Client::Get()->RequestRemoteNotificationsIfSilent();
#endif
}

/** Clean an input string to be URL safe. */
String ScriptEngine::URLEncode(const String& s) const
{
	return HTTP::Manager::URLEncode(s);
}

Double ScriptEngine::GetGameSecondsSinceStartup() const
{
	return Engine::Get()->GetGameSecondsSinceStartup();
}

} // namespace Seoul
