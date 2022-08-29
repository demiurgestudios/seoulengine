/**
 * \file PlatformSignInManager.h
 * \brief Platform agnostic wrapper around platform specific
 * authentication (currently, Google Play Games on Android
 * and GameCenter on iOS).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef PLATFORM_SIGN_IN_MANAGER_H
#define PLATFORM_SIGN_IN_MANAGER_H

#include "Atomic32.h"
#include "Delegate.h"
#include "SeoulString.h"
#include "Singleton.h"
namespace Seoul { namespace HTTP { class Request; } }

namespace Seoul
{

/**
 * Used for basic type checking on cast, given our lack of RTTI,
 * without the full Reflection setup.
 */
enum class PlatformSignInManagerType
{
	kAndroid,
	kDeveloper,
	kIOSGameCenter,
	kIOSApple,
	kNull,
};

/**
 * Events::Manager event ID that can be used to receive
 * callbacks that fire when any significant event in the
 * auth system occurs. (For example, an attempted sign-in
 * fails.)
 */
static const HString PlatformSignInEventId("SignInEventId");

class PlatformSignInManager SEOUL_ABSTRACT : public Singleton<PlatformSignInManager>
{
public:
	/** Virtual destructor */
	virtual ~PlatformSignInManager();

	/**
	 * Return the atomic count of state changes. This value will
	 * incremented whenever sign-in or token status has changed
	 * and can be used by interested parties to detect changes
	 * in sign-in state.
	 */
	virtual Atomic32Type GetStateChangeCount() const = 0;

	/** Specific type of this sign-in manager. Used for explicit casts. */
	virtual PlatformSignInManagerType GetType() const = 0;

	/**
	 * True if the user has cancelled an explicit sign-in flow during the
	 * current session, false otherwise.
	 */
	virtual Bool HaveAnyCancellationsOccurred() const = 0;

	/**
	 * Tests if the user is currently authenticated with platform services.
	 */
	virtual Bool IsSignedIn() const = 0;

	/**
	 * True if the sign-in manager is actively trying to sign-in or sign-out.
	 * Typically, client code should wait for this to return false before
	 * making decisions based on the state of the sign-in manager.
	 */
	virtual Bool IsSigningIn() const = 0;

	/**
	 * At runtime, check for sign-in support. Can be conditionally disabled
	 * or never supported (e.g. NullPlatformSignInManager).
	 */
	virtual Bool IsSignInSupported() const = 0;

	/**
	 * Request a sign-in - GetStateChangeCount() can be polled to detect
	 * a change to sign-in status.
	 */
	virtual void SignIn() = 0;

	/**
	 * Signs the user out of platform services.
	 */
	virtual void SignOut() = 0;

	typedef Delegate<void(const String&, Bool)> OnTokenReceivedDelegate;

	// Retrieve the ID token for the current platform, invoke delegate
	// on completion.
	virtual void GetIDToken(const OnTokenReceivedDelegate& delegate) = 0;

	// Sets the ID token for the current platform. Default implementation
	// is nop, but for some platforms (Apple) a new token is received from the
	// server. The associated old token is meant to be the token which the new
	// token should replace. This is used to tie token changes to a certain state.
	virtual void SetIDToken(const String& sToken,
							const String& sAssociatedOldToken) {}

	/**
	 * Convenience utility - trigger an explicit refresh of
	 * the id token for the current platform. Meant to be used
	 * to "prime" the token so that a future request for the token
	 * itself will return quickly.
	 */
	void RefreshIdToken()
	{
		GetIDToken(SEOUL_BIND_DELEGATE(Nop));
	}

	/**
	 * Asynchronously request the platform's id token
	 * and include it with the request, then start the
	 * request. Request is always sent - if any error
	 * requesting the token or if the platform auth is
	 * not signed in, the request will be sent with no
	 * token.
	 */
	virtual void StartWithIdToken(HTTP::Request& rRequest) = 0;

	// Hooks for tracking stop/start events for the app.
	virtual void OnSessionStart() = 0;
	virtual void OnSessionEnd() = 0;

#if SEOUL_ENABLE_CHEATS
	virtual void DevOnlyAddSignInCancellation() {}
	virtual Bool DevOnlyEnabled() const { return true; }
	virtual void DevOnlySetEnabled(Bool b) {}
	virtual void DevOnlySetType(PlatformSignInManagerType eType) {}
#endif // /#if SEOUL_ENABLE_CHEATS

protected:
	PlatformSignInManager();

	static void TriggerSignInEvent();

private:
	SEOUL_DISABLE_COPY(PlatformSignInManager);

	static void Nop(const String&, Bool) {}
}; // class PlatformSignInManager

/**
 * Null interface for PlatformSignInManager.  All methods fail or return default
 * values.
 */
class NullPlatformSignInManager SEOUL_SEALED : public PlatformSignInManager
{
public:
	NullPlatformSignInManager()
	{
	}

	~NullPlatformSignInManager()
	{
	}

	/**
	 * Return the atomic count of state changes. This value will
	 * incremented whenever sign-in or token status has changed
	 * and can be used by interested parties to detect changes
	 * in sign-in state.
	 */
	virtual Atomic32Type GetStateChangeCount() const SEOUL_OVERRIDE
	{
		return 0;
	}

	virtual PlatformSignInManagerType GetType() const SEOUL_OVERRIDE { return PlatformSignInManagerType::kNull; }

	/**
	 * True if the user has cancelled an explicit sign-in flow during the
	 * current session, false otherwise.
	 */
	virtual Bool HaveAnyCancellationsOccurred() const SEOUL_OVERRIDE { return false; }

	/**
	 * Tests if the user is currently logged into platform services.
	 */
	virtual Bool IsSignedIn() const SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * True if the sign-in manager is actively trying to sign-in or sign-out.
	 * Typically, client code should wait for this to return false before
	 * making decisions based on the state of the sign-in manager.
	 */
	virtual Bool IsSigningIn() const SEOUL_OVERRIDE
	{
		return false;
	}

	/**
	 * At runtime, check for sign-in support. Can be conditionally disabled
	 * or never supported (e.g. NullPlatformSignInManager).
	 */
	virtual Bool IsSignInSupported() const SEOUL_OVERRIDE
	{
		return false;
	}

	// Retrieve the ID token for the current platform, invoke delegate
	// on completion.
	virtual void GetIDToken(const OnTokenReceivedDelegate& delegate) SEOUL_OVERRIDE;

	/**
	 * Attempt to add the necessary data to a post
	 * request to identify the user.
	 *
	 * @return false if no add occurred, true other if data
	 * was added.
	 */
	virtual void StartWithIdToken(HTTP::Request& rRequest) SEOUL_OVERRIDE;

	/**
	 * Request a sign-in - GetStateChangeCount() can be polled to detect
	 * a change to sign-in status.
	 */
	virtual void SignIn() SEOUL_OVERRIDE
	{
	}

	/**
	 * Signs the user out of platform services.
	 */
	virtual void SignOut() SEOUL_OVERRIDE
	{
	}

	virtual void OnSessionStart() SEOUL_OVERRIDE
	{
	}

	virtual void OnSessionEnd() SEOUL_OVERRIDE
	{
	}

private:
	SEOUL_DISABLE_COPY(NullPlatformSignInManager);
}; // class NullPlatformSignInManager

#if SEOUL_ENABLE_CHEATS

/**
 * Dev-only sign-in manager available on limited platforms
 * when cheats are enabled, used for testing various
 * configurations.
 */
class DeveloperPlatformSignInManager SEOUL_SEALED : public PlatformSignInManager
{
public:
	DeveloperPlatformSignInManager();
	~DeveloperPlatformSignInManager();

	virtual Atomic32Type GetStateChangeCount() const SEOUL_OVERRIDE;
	virtual PlatformSignInManagerType GetType() const SEOUL_OVERRIDE {
		return m_ePretendManagerType;
	}
	virtual Bool HaveAnyCancellationsOccurred() const SEOUL_OVERRIDE;
	virtual Bool IsSignedIn() const SEOUL_OVERRIDE;
	virtual Bool IsSigningIn() const SEOUL_OVERRIDE;
	virtual Bool IsSignInSupported() const SEOUL_OVERRIDE;
	virtual void GetIDToken(const OnTokenReceivedDelegate& delegate) SEOUL_OVERRIDE;
	virtual void StartWithIdToken(HTTP::Request& rRequest) SEOUL_OVERRIDE;
	virtual void SignIn() SEOUL_OVERRIDE;
	virtual void SignOut() SEOUL_OVERRIDE;
	virtual void OnSessionStart() SEOUL_OVERRIDE;
	virtual void OnSessionEnd() SEOUL_OVERRIDE;

	virtual void DevOnlyAddSignInCancellation() SEOUL_OVERRIDE { ++m_CancellationCount; }
	virtual Bool DevOnlyEnabled() const SEOUL_OVERRIDE { return m_bEnabled; }
	virtual void DevOnlySetEnabled(Bool b) SEOUL_OVERRIDE
	{
		if (b != m_bEnabled)
		{
			m_bEnabled = b;
			++m_ChangeCount;
		}
	}
	virtual void DevOnlySetType(PlatformSignInManagerType eType) SEOUL_OVERRIDE {
		m_ePretendManagerType = eType;
	}

private:
	SEOUL_DISABLE_COPY(DeveloperPlatformSignInManager);

	Atomic32 m_CancellationCount;
	Atomic32 m_ChangeCount;
	Atomic32Value<Bool> m_bSignedIn;
	Atomic32Value<Bool> m_bEnabled;
	Atomic32Value<PlatformSignInManagerType> m_ePretendManagerType;
}; // class DeveloperPlatformSignInManager

#endif // /#if SEOUL_ENABLE_CHEATS

} // namespace Seoul

#endif  // Include guard
