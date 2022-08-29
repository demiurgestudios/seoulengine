/**
 * \file ScriptEngine.h
 * \brief Binder instance for exposing the global
 * Engine singleton instance into a script VM.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_ENGINE_H
#define SCRIPT_ENGINE_H

#include "FilePath.h"
#include "PlatformData.h"
namespace Seoul { namespace Script { class FunctiofnInterface; } }

namespace Seoul
{

class ScriptEngine SEOUL_SEALED
{
public:
	ScriptEngine();
	~ScriptEngine();

	// Return the current game time in ticks.
	Int64 GetGameTimeInTicks() const;

	// Return the current game time in seconds.
	Double GetGameTimeInSeconds() const;

	// Return the game time in seconds since the start of the frame
	Double GetTimeInSecondsSinceFrameStart() const;

	// Return the current UUID, platform specific.
	String GetPlatformUUID() const;

	// Returns the current device's platform data: platform specific.
	PlatformData GetPlatformData() const;

	// Return the current frame's delta time in seconds.
	Float GetSecondsInTick() const;

	// Returns true if we are in an automated or unit (testing) environment.
	Bool IsAutomationOrUnitTestRunning() const;

	void ShowAppStoreToRateThisApp();

	Bool DoesSupportInAppRateMe() const;

	Bool WriteToClipboard(const String& sArgumentName) const;

	// Return the current process id.
	void GetThisProcessId(Script::FunctionInterface* pInterface) const;

	// Get whether the current platform
	// has native/default back button handling.
	Bool HasNativeBackButtonHandling() const;

	// Pass a URL to the system's default URL handling
	// (usually the default web browser).
	Bool OpenURL(const String& sURL);

	// Tell the FileManager to start downloading filePath,
	// if it will be network serviced.
	Bool NetworkPrefetch(FilePath filePath);

	// Trigger native "back button" handling for the
	// current platform. Typically, this causes
	// the App to exit.
	Bool PostNativeQuitMessage();

	// Are Remote Notifications enabled?
	Bool HasEnabledRemoteNotifications();

	// Can we register for remote notifications without an OS prompt?
	Bool CanRequestRemoteNotificationsWithoutPrompt();

	// Mark this device as able to register for remote notifications without an OS prompt?
	void SetCanRequestRemoteNotificationsWithoutPrompt(Bool);

	// Schedule a local notification.
	// See Engine.h for expected arguments
	void ScheduleLocalNotification(Script::FunctionInterface* pInterface);

	// Cancel a scheduled local notification
	void CancelLocalNotification(Int iNotificationID);

	// Cancel all scheduled local notifications
	void CancelAllLocalNotifications();

	// Returns worldtime in seconds since epoch
	WorldTime GetCurrentServerTime() const;

	// Clean an input string to be URL safe.
	String URLEncode(const String& s) const;

	Double GetGameSecondsSinceStartup() const;

private:
	SEOUL_DISABLE_COPY(ScriptEngine);
}; // class ScriptEngine

} // namespace Seoul

#endif // include guard
