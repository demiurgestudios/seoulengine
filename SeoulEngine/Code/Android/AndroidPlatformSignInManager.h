/**
 * \file AndroidPlatformSignInManager.h
 * \brief Android-specific platform sign-in manager. Implemented
 * with Google Play Games API.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANDROID_PLATFORM_SIGN_IN_MANAGER_H
#define ANDROID_PLATFORM_SIGN_IN_MANAGER_H

#include "Atomic32.h"
#include "PlatformSignInManager.h"
#include "Mutex.h"
#include "Prereqs.h"

#if SEOUL_WITH_GOOGLEPLAYGAMES

namespace Seoul
{

/** Configuration for the Android sign-in manager. */
struct AndroidPlatformSignInManagerSettings SEOUL_SEALED
{
	// See: https://stackoverflow.com/questions/40997205/unregistered-on-api-console-while-getting-oauth2-token-on-android
	String m_sOauthClientId;
}; // struct AndroidPlatformSignInManagerSettings

/**
 * Android-specific sign in using Google Play Games.
 */
class AndroidPlatformSignInManager SEOUL_SEALED : public PlatformSignInManager
{
public:
	// Runtime check for availability - must be called prior to instantiating a
	// AndroidPlatformSignInManager instance.
	static Bool IsAvailable();

	/**
	 * Convenience function to get the AndroidPlatformSignInManager singleton pointer
	 */
	static CheckedPtr<AndroidPlatformSignInManager> Get()
	{
		if (PlatformSignInManager::Get() && PlatformSignInManager::Get()->GetType() == PlatformSignInManagerType::kAndroid)
		{
			return (AndroidPlatformSignInManager*)PlatformSignInManager::Get().Get();
		}
		else
		{
			return CheckedPtr<AndroidPlatformSignInManager>();
		}
	}

	AndroidPlatformSignInManager(const AndroidPlatformSignInManagerSettings& settings);
	~AndroidPlatformSignInManager();

	virtual Atomic32Type GetStateChangeCount() const SEOUL_OVERRIDE
	{
		return m_ChangeCount;
	}

	virtual PlatformSignInManagerType GetType() const SEOUL_OVERRIDE { return PlatformSignInManagerType::kAndroid; }

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
		return true;
	}

	/**
	 * This is an Android only hook used by CommerceManager.
	 * If a purchase is attempted and product data failed to
	 * be retrieved, and if the PlatformSignInManager is capable
	 * to sign in but is not signed in, this method can be called
	 * to sign in at the platform level *only*. This does not
	 * switch the game's state from signed in to not signed in.
	 *
	 * It is an attempt to escalate Google Play state sufficiently
	 * for IAPs to function.
	 */
	typedef Delegate<void(Bool bSuccess)> PlatformSignInDelegate;
	void PlatformSignInOnly(PlatformSignInDelegate delegate);

	virtual void SignIn() SEOUL_OVERRIDE;
	virtual void SignOut() SEOUL_OVERRIDE;

	// Retrieve the ID token for the current platform, invoke delegate
	// on completion.
	virtual void GetIDToken(const OnTokenReceivedDelegate& delegate) SEOUL_OVERRIDE;

	// Retrieve the ID token for the current platform, dispatch
	// the request with the token assigned to an appropriate post field
	// on completion. If token retrieval fails, the request will be invoked
	// with the corresponding post field left empty.
	virtual void StartWithIdToken(HTTP::Request& rRequest) SEOUL_OVERRIDE;

	virtual void OnSessionStart() SEOUL_OVERRIDE;
	virtual void OnSessionEnd() SEOUL_OVERRIDE;

	// Hooks to invoke from JNI
	static void HandleCancelled();
	static void HandleChange(Bool bSignedIn);

private:
	AndroidPlatformSignInManagerSettings const m_Settings;
	Atomic32 m_ChangeCount;
	Atomic32 m_CancellationCount;
	Mutex m_Mutex;
	Atomic32Value<Bool> m_bSignedIn;
	Atomic32Value<Bool> m_bSigningIn;

	void OnCancelled();
	void OnChange(Bool bSignedIn);

	SEOUL_DISABLE_COPY(AndroidPlatformSignInManager);
}; // class AndroidPlatformSignInManager

} // namespace Seoul

#endif 	// SEOUL_WITH_GOOGLEPLAYGAMES

#endif  // include guard
