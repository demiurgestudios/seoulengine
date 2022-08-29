/**
 * \file IOSGameCenterPlatformSignInManager.h
 * \brief IOS-specific platform sign-in manager. Implemented
 * with GameCenter API.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef IOS_GAME_CENTER_PLATFORM_SIGN_IN_MANAGER_H
#define IOS_GAME_CENTER_PLATFORM_SIGN_IN_MANAGER_H

#include "Atomic32.h"
#include "PlatformSignInManager.h"
#include "Mutex.h"
#include "Prereqs.h"

#if SEOUL_WITH_GAMECENTER

#ifdef __OBJC__
@class UIViewController;
#endif

namespace Seoul
{

/**
 * IOS-specific sign in using Google Play Games.
 */
class IOSGameCenterPlatformSignInManager SEOUL_SEALED : public PlatformSignInManager
{
public:
	/**
	 * Convenience function to get the IOSGameCenterPlatformSignInManager singleton pointer
	 */
	static CheckedPtr<IOSGameCenterPlatformSignInManager> Get()
	{
		if (PlatformSignInManager::Get() && PlatformSignInManager::Get()->GetType() == PlatformSignInManagerType::kIOSGameCenter)
		{
			return (IOSGameCenterPlatformSignInManager*)PlatformSignInManager::Get().Get();
		}
		else
		{
			return CheckedPtr<IOSGameCenterPlatformSignInManager>();
		}
	}

	IOSGameCenterPlatformSignInManager();
	~IOSGameCenterPlatformSignInManager();

	void Initialize(void* pRootViewController);

	virtual Atomic32Type GetStateChangeCount() const SEOUL_OVERRIDE
	{
		return m_ChangeCount;
	}

	virtual PlatformSignInManagerType GetType() const SEOUL_OVERRIDE {
		return PlatformSignInManagerType::kIOSGameCenter;
	}

	virtual Bool HaveAnyCancellationsOccurred() const SEOUL_OVERRIDE
	{
		return (0 != m_CancellationCount);
	}

	virtual Bool IsSignedIn() const SEOUL_OVERRIDE
	{
		return m_bSignedIn;
	}

	virtual Bool IsSigningIn() const SEOUL_OVERRIDE
	{
		return m_bSigningIn;
	}

	virtual Bool IsSignInSupported() const SEOUL_OVERRIDE
	{
		return m_bSignInSupported;
	}

	virtual void SignIn() SEOUL_OVERRIDE;
	virtual void SignOut() SEOUL_OVERRIDE;

	virtual void GetIDToken(const OnTokenReceivedDelegate& delegate) SEOUL_OVERRIDE;
	virtual void StartWithIdToken(HTTP::Request& rRequest) SEOUL_OVERRIDE;

	virtual void OnSessionStart() SEOUL_OVERRIDE;
	virtual void OnSessionEnd() SEOUL_OVERRIDE;

	// Hooks invoked by handlers.
	static void HandlePlatformChange(Bool bSignedIn, const String& sId);
	static void HandlePlatformSignInViewChange(Bool bShowing);
	static void HandlePlatformUserCancellation();

	// IOS hook - true if the GameCenter sign-in view is active.
	Bool IsShowingSignInView() const
	{
		return m_bShowingSignInView;
	}

private:
#ifdef __OBJC__
	UIViewController* m_pRootViewController;
#else
	void* m_pRootViewController;
#endif
	Atomic32 m_ChangeCount;
	Atomic32 m_CancellationCount;
	Mutex m_Mutex;
	String m_sId;
	Atomic32Value<Bool> m_bSignInSupported;
	Atomic32Value<Bool> m_bInitialized;
	Atomic32Value<Bool> m_bSignedIn;
	Atomic32Value<Bool> m_bShowingSignInView;
	Atomic32Value<Bool> m_bSigningIn;
	Atomic32Value<Bool> m_bGameCenterHookRegistered;

	void InternalPerformGameCenterSignIn();

	void OnPlatformChange(Bool bSignedIn, const String& sId);
	void OnPlatformSignInViewChange(Bool bShowing);
	void OnPlatformUserCancellation();

	SEOUL_DISABLE_COPY(IOSGameCenterPlatformSignInManager);
}; // class IOSGameCenterPlatformSignInManager

} // namespace Seoul

#endif 	// SEOUL_WITH_GAMECENTER

#endif  // include guard
