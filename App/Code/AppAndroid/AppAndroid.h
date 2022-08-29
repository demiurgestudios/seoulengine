/**
 * \file AppAndroid.h
 * \brief Singleton class unique to Android - hosts entry points
 * from the Java host code into SeoulEngine and the App C++ code.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef APP_ANDROID_H
#define APP_ANDROID_H

#include "ScopedPtr.h"
#include "SharedPtr.h"
#include "Singleton.h"

// Forward declarations
struct ANativeWindow;

namespace Seoul
{

// Forward declarations
class AndroidCrashManager;
class AndroidEngine;
namespace Game { class Main; }

class AppAndroidEngine SEOUL_SEALED : public Singleton<AppAndroidEngine>
{
public:
	AppAndroidEngine(
		const String& sAutomationScript,
		ANativeWindow* pMainWindow,
		const String& sBaseDirectoryPath);
	~AppAndroidEngine();

private:
	ScopedPtr<AndroidCrashManager> m_pCrashManager;
	ScopedPtr<AndroidEngine> m_pEngine;

	SEOUL_DISABLE_COPY(AppAndroidEngine);
}; // class AppAndroidEngine

class AppAndroid SEOUL_SEALED : public Singleton<AppAndroid>
{
public:
	AppAndroid(const String& sAutomationScript);
	~AppAndroid();

	void Tick();

	void Initialize();
	void Shutdown();

private:
	ScopedPtr<Game::Main> m_pGameMain;

	SEOUL_DISABLE_COPY(AppAndroid);
}; // class AppAndroid

} // namespace Seoul

#endif // include guard
