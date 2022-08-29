/**
 * \file PlatformSignInManager.cpp
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

#include "EventsManager.h"
#include "HTTPManager.h"
#include "HTTPRequest.h"
#include "HTTPRequestCancellationToken.h"
#include "PlatformSignInManager.h"
#include "ReflectionDefine.h"
#include "ScopedAction.h"

namespace Seoul
{

SEOUL_BEGIN_ENUM(PlatformSignInManagerType)
	SEOUL_ENUM_N("Android", PlatformSignInManagerType::kAndroid)
	SEOUL_ENUM_N("Developer", PlatformSignInManagerType::kDeveloper)
	SEOUL_ENUM_N("IOSGameCenter", PlatformSignInManagerType::kIOSGameCenter)
	SEOUL_ENUM_N("IOSApple", PlatformSignInManagerType::kIOSApple)
	SEOUL_ENUM_N("Null", PlatformSignInManagerType::kNull)
SEOUL_END_ENUM()

PlatformSignInManager::PlatformSignInManager()
{
}

PlatformSignInManager::~PlatformSignInManager()
{
}

void PlatformSignInManager::TriggerSignInEvent()
{
	SEOUL_ASSERT(IsMainThread());
	if (Events::Manager::Get() && PlatformSignInManager::Get())
	{
		Events::Manager::Get()->TriggerEvent(PlatformSignInEventId);
	}
}

void NullPlatformSignInManager::GetIDToken(const OnTokenReceivedDelegate& delegate)
{
	delegate(String(), false);
}

/**
 * Asynchronously request the platform's id token
 * and include it with the request, then start the
 * request. Request is always sent - if any error
 * requesting the token or if the platform auth is
 * not signed in, the request will be sent with no
 * token.
 */
void NullPlatformSignInManager::StartWithIdToken(HTTP::Request& rRequest)
{
	rRequest.Start();
}

#if SEOUL_ENABLE_CHEATS

struct DeveloperPlatformSignInManagerCommands SEOUL_SEALED
{
	void AddSignInCancellation()
	{
		PlatformSignInManager::Get()->DevOnlyAddSignInCancellation();
	}

	void ToggleFakeSignIn()
	{
		PlatformSignInManager::Get()->DevOnlySetEnabled(
			!PlatformSignInManager::Get()->DevOnlyEnabled());
	}

	void SetPretendSignInManagerType(PlatformSignInManagerType eType)
	{
		PlatformSignInManager::Get()->DevOnlySetType(eType);
	}
};

SEOUL_BEGIN_TYPE(DeveloperPlatformSignInManagerCommands, TypeFlags::kDisableCopy)
	SEOUL_ATTRIBUTE(CommandsInstance)
	SEOUL_METHOD(AddSignInCancellation)
		SEOUL_ATTRIBUTE(Category, "Server")
		SEOUL_ATTRIBUTE(Description,
			"Add a cancellation count to debug cancellation tracking.")
		SEOUL_ATTRIBUTE(DisplayName, "Add Sign In Cancellation")
	SEOUL_METHOD(ToggleFakeSignIn)
		SEOUL_ATTRIBUTE(Category, "Server")
		SEOUL_ATTRIBUTE(Description,
			"Enable/disable the developer only sign-in\n"
			"simulator for debugging auth UI on dev.\n"
			"builds.")
		SEOUL_ATTRIBUTE(DisplayName, "Toggle Fake Sign In")
	SEOUL_METHOD(SetPretendSignInManagerType)
		SEOUL_ATTRIBUTE(Category, "Server")
		SEOUL_ATTRIBUTE(Description,
			"Set the type that the developer only sign-in\n"
			"simulator should pretend to be.\n"
			"Does not enable real sign-in on the chosen platform.")
		SEOUL_ATTRIBUTE(DisplayName, "Set Pretend Sign In Type")
		SEOUL_ATTRIBUTE(CommandNeedsButton)
SEOUL_END_TYPE()

DeveloperPlatformSignInManager::DeveloperPlatformSignInManager()
	: m_ChangeCount(0)
	, m_bSignedIn(true)
	, m_bEnabled(false)
	, m_ePretendManagerType(PlatformSignInManagerType::kDeveloper)
{
}

DeveloperPlatformSignInManager::~DeveloperPlatformSignInManager()
{
}

Atomic32Type DeveloperPlatformSignInManager::GetStateChangeCount() const
{
	return m_ChangeCount;
}

Bool DeveloperPlatformSignInManager::HaveAnyCancellationsOccurred() const
{
	return (0 != m_CancellationCount);
}

Bool DeveloperPlatformSignInManager::IsSignedIn() const
{
	return (m_bEnabled && m_bSignedIn);
}

Bool DeveloperPlatformSignInManager::IsSigningIn() const
{
	return false;
}

Bool DeveloperPlatformSignInManager::IsSignInSupported() const
{
	return m_bEnabled;
}

void DeveloperPlatformSignInManager::GetIDToken(const OnTokenReceivedDelegate& delegate)
{
	delegate(String(), false);
}

void DeveloperPlatformSignInManager::StartWithIdToken(HTTP::Request& rRequest)
{
	rRequest.Start();
}

void DeveloperPlatformSignInManager::SignIn()
{
	// Always run the trigger sign in event when this function exits
	auto const signInEvent(MakeDeferredAction(
		[]() { TriggerSignInEvent(); }));

	if (m_bSignedIn) { return; }

	m_bSignedIn = true;
	++m_ChangeCount;
}

void DeveloperPlatformSignInManager::SignOut()
{
	// Always run the trigger sign in event when this function exits
	auto const signInEvent(MakeDeferredAction(
		[]() { TriggerSignInEvent(); }));

	if (!m_bSignedIn) { return; }

	m_bSignedIn = false;
	++m_ChangeCount;
}

void DeveloperPlatformSignInManager::OnSessionStart()
{
	// Nop
}

void DeveloperPlatformSignInManager::OnSessionEnd()
{
	// Nop
}

#endif // /#if SEOUL_ENABLE_CHEATS

} // namespace Seoul
