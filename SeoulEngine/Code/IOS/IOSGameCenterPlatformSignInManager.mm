/**
 * \file IOSGameCenterPlatformSignInManager.cpp
 * \brief IOS-specific platform sign-in manager. Implemented
 * with GameCenter API.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "IOSGameCenterPlatformSignInManager.h"

#if SEOUL_WITH_GAMECENTER

#include "HTTPManager.h"
#include "HTTPRequest.h"
#include "HTTPRequestCancellationToken.h"
#include "IOSEngine.h"
#include "JobsFunction.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "ScopedAction.h"
#import <GameKit/GameKit.h>

namespace Seoul
{

// The string value does not match the name ("GameCenter") for backwards compatibility
static NSString* const ksIOSGameCenterPlatformSignInManager_SignedOut = @"IOSPlatformSignInManager_SignedOut";
static inline Bool GetSavedSignedInValue()
{
	NSUserDefaults* pDefaults = [NSUserDefaults standardUserDefaults];

	auto bSignedOut = [pDefaults boolForKey:ksIOSGameCenterPlatformSignInManager_SignedOut];
	return !bSignedOut;
}
static inline void SetSavedSignedInValue(Bool bValue)
{
	NSUserDefaults* pDefaults = [NSUserDefaults standardUserDefaults];

	// We save "signed out" so the unset default is false, so we will
	// auto sign in.
	auto const bSignedOut = !bValue;

	[pDefaults setBool:bSignedOut forKey:ksIOSGameCenterPlatformSignInManager_SignedOut];
	[pDefaults synchronize];
}

IOSGameCenterPlatformSignInManager::IOSGameCenterPlatformSignInManager()
	: m_pRootViewController(nil)
	, m_ChangeCount(0)
	, m_CancellationCount(0)
	, m_Mutex()
	, m_sId()
	, m_bSignInSupported(true)
	, m_bInitialized(false)
	, m_bSignedIn(false)
	, m_bShowingSignInView(false)
	// True by default, false once we get any sort of sign-in status update.
	, m_bSigningIn(true)
	, m_bGameCenterHookRegistered(false)
{
}

void IOSGameCenterPlatformSignInManager::Initialize(void* pRootViewController)
{
	// Early out.
	if (m_bInitialized)
	{
		return;
	}

	// Cache controller for future use.
	m_pRootViewController = (UIViewController*)pRootViewController;
	[m_pRootViewController retain];

	// If the saved signed in value is false, immediately
	// return without performing the setup.
	if (!GetSavedSignedInValue())
	{
		m_bSignedIn = false;
		m_bSigningIn = false;
	}
	else
	{
		InternalPerformGameCenterSignIn();
	}

	// Initialized.
	m_bInitialized = true;
}

IOSGameCenterPlatformSignInManager::~IOSGameCenterPlatformSignInManager()
{
	// Early out.
	if (!m_bInitialized)
	{
		return;
	}

	// Deinitialize.
	m_bInitialized = false;

	// Release the view controller.
	[m_pRootViewController release];
	m_pRootViewController = nil;
}

void IOSGameCenterPlatformSignInManager::SignIn()
{
	// Update that we want to be auto signed in going forward.
	SetSavedSignedInValue(true);

	// Nop if already signed in, in the process of signing in,
	// or not allowed to sign in (platform connection not
	// made if m_bSignInSupported is false).
	if (m_bSigningIn)
	{
		return;
	}
	if (!m_bSignInSupported || m_bSignedIn)
	{
		Jobs::AsyncFunction(
			GetMainThreadId(),
			&PlatformSignInManager::TriggerSignInEvent);
		return;
	}

	// Register the hook if needed. This will handle the remainder of
	// sign in handling.
	if (!m_bGameCenterHookRegistered)
	{
		InternalPerformGameCenterSignIn();
	}
	// Otherwise, apply immediately.
	else
	{
		// Set signed in to true - on iOS, there does not seem
		// to be a way to sign out of game center (at the platform
		// level) so we also can't re-sign in at the platform level.
		// So we simulate it.
		m_bSignedIn = true;

		// Signal changes.
		SeoulMemoryBarrier();

		// Changes
		++m_ChangeCount;

		Jobs::AsyncFunction(
			GetMainThreadId(),
			&PlatformSignInManager::TriggerSignInEvent);
	}
}

void IOSGameCenterPlatformSignInManager::SignOut()
{
	// Set that we want to not sign in by default in the future.
	SetSavedSignedInValue(false);

	// If signed out or signing out, nop.
	if (m_bSigningIn)
	{
		return;
	}
	if (!m_bSignedIn)
	{
		Jobs::AsyncFunction(
			GetMainThreadId(),
			&PlatformSignInManager::TriggerSignInEvent);
		return;
	}

	// Set signed in to false - on iOS, there does not seem
	// to be a way to sign out of game center (at the platform
	// level). So we only simulate it.
	m_bSignedIn = false;

	// Signal changes.
	SeoulMemoryBarrier();

	// Changes
	++m_ChangeCount;

	Jobs::AsyncFunction(
		GetMainThreadId(),
		&PlatformSignInManager::TriggerSignInEvent);
}

struct IOSPlatformSignInToken SEOUL_SEALED
{
	String m_sBundleId;
	String m_sPlayerId;
	String m_sPublicKeyUrl;
	String m_sSalt;
	String m_sSignature;
	UInt64 m_uTimestamp;
};
SEOUL_BEGIN_TYPE(IOSPlatformSignInToken)
	SEOUL_PROPERTY_N("bundleId", m_sBundleId)
	SEOUL_PROPERTY_N("playerId", m_sPlayerId)
	SEOUL_PROPERTY_N("publicKeyUrl", m_sPublicKeyUrl)
	SEOUL_PROPERTY_N("salt", m_sSalt)
	SEOUL_PROPERTY_N("signature", m_sSignature)
	SEOUL_PROPERTY_N("timestamp", m_uTimestamp)
SEOUL_END_TYPE()

static Bool EncodeToken(const IOSPlatformSignInToken& tok, String& rsOut)
{
	if (!Reflection::SerializeToString(&tok, rsOut))
	{
		return false;
	}

	rsOut = Base64Encode(rsOut.CStr(), rsOut.GetSize());
	return true;
}

void IOSGameCenterPlatformSignInManager::GetIDToken(const OnTokenReceivedDelegate& del)
{
	// Local value cache for block.
	auto const delegate = del;

	// Immediate complete with no id if not signed in.
	if (!m_bSignedIn || m_bSigningIn)
	{
		delegate(String(), false);
		return;
	}

	// Dispatch to get validation data, generate a token
	// for the server, then send the request.
	dispatch_async(dispatch_get_main_queue(), ^()
	{
		GKLocalPlayer* pLocalPlayer = [GKLocalPlayer localPlayer];
		[pLocalPlayer generateIdentityVerificationSignatureWithCompletionHandler:^(NSURL * _Nullable publicKeyUrl, NSData * _Nullable signature, NSData * _Nullable salt, uint64_t timestamp, NSError * _Nullable error)
		{
			// On error, send the request immediately with no payload.
			if (nil != error)
			{
				delegate(String(), false);
				return;
			}

			// Fill out the token members.
			IOSPlatformSignInToken tok;
			tok.m_sBundleId = String([[NSBundle mainBundle] bundleIdentifier]);
			tok.m_sPlayerId = String(pLocalPlayer.playerID);
			tok.m_sPublicKeyUrl = String(publicKeyUrl.absoluteString);
			tok.m_sSalt = Base64Encode(salt.bytes, (UInt32)salt.length);
			tok.m_sSignature = Base64Encode(signature.bytes, (UInt32)signature.length);
			tok.m_uTimestamp = timestamp;

			// Serialize the token.
			String sEncodedToken;
			if (!EncodeToken(tok, sEncodedToken))
			{
				delegate(String(), false);
				return;
			}

			SEOUL_LOG_AUTH("Reporting successful token to callback: %s",
				sEncodedToken.CStr());

			delegate(sEncodedToken, true);
		}];
	});
}

void IOSGameCenterPlatformSignInManager::StartWithIdToken(HTTPRequest& rRequest)
{
	// Immediate complete with no id if not signed in.
	if (!m_bSignedIn || m_bSigningIn)
	{
		rRequest.Start();
		return;
	}

	// Dispatch to get validation data, generate a token
	// for the server, then send the request.
	dispatch_async(dispatch_get_main_queue(), ^()
	{
		GKLocalPlayer* pLocalPlayer = [GKLocalPlayer localPlayer];
		[pLocalPlayer generateIdentityVerificationSignatureWithCompletionHandler:^(NSURL * _Nullable publicKeyUrl, NSData * _Nullable signature, NSData * _Nullable salt, uint64_t timestamp, NSError * _Nullable error)
		{
			// On error, send the request immediately with no payload.
			if (nil != error)
			{
				rRequest.Start();
				return;
			}

			// Fill out the token members.
			IOSPlatformSignInToken tok;
			tok.m_sBundleId = String([[NSBundle mainBundle] bundleIdentifier]);
			tok.m_sPlayerId = String(pLocalPlayer.playerID);
			tok.m_sPublicKeyUrl = String(publicKeyUrl.absoluteString);
			tok.m_sSalt = Base64Encode(salt.bytes, (UInt32)salt.length);
			tok.m_sSignature = Base64Encode(signature.bytes, (UInt32)signature.length);
			tok.m_uTimestamp = timestamp;

			// Serialize the token.
			String sEncodedToken;
			if (!EncodeToken(tok, sEncodedToken))
			{
				rRequest.Start();
				return;
			}

			SEOUL_LOG_AUTH("Sending HTTP request with id token: %s",
				sEncodedToken.CStr());

			rRequest.AddPostData("GameCenterToken", sEncodedToken);
			rRequest.Start();
		}];
	});
}

void IOSGameCenterPlatformSignInManager::OnSessionStart()
{
	// Nop
}

void IOSGameCenterPlatformSignInManager::OnSessionEnd()
{
	// Nop
}

void IOSGameCenterPlatformSignInManager::HandlePlatformChange(Bool bSignedIn, const String& sId)
{
	auto pMan = IOSGameCenterPlatformSignInManager::Get();
	if (!pMan)
	{
		return;
	}

	pMan->OnPlatformChange(bSignedIn, sId);
}

void IOSGameCenterPlatformSignInManager::HandlePlatformSignInViewChange(Bool bShowing)
{
	auto pMan = IOSGameCenterPlatformSignInManager::Get();
	if (!pMan)
	{
		return;
	}

	pMan->OnPlatformSignInViewChange(bShowing);
}

void IOSGameCenterPlatformSignInManager::HandlePlatformUserCancellation()
{
	auto pMan = IOSGameCenterPlatformSignInManager::Get();
	if (!pMan)
	{
		return;
	}

	pMan->OnPlatformUserCancellation();
}

void IOSGameCenterPlatformSignInManager::InternalPerformGameCenterSignIn()
{
	// Early out if the hook is already setup.
	if (m_bGameCenterHookRegistered)
	{
		return;
	}

	// Now signing in.
	m_bSigningIn = true;

	// Now hooked.
	m_bGameCenterHookRegistered = true;

	dispatch_async(dispatch_get_main_queue(), ^()
	{
		UIViewController* pRootViewControllerIOS = m_pRootViewController;
		GKLocalPlayer* pLocalPlayer = [GKLocalPlayer localPlayer];
		pLocalPlayer.authenticateHandler = ^(UIViewController* pViewController, NSError* pError)
		{
			if (nil != pViewController)
			{
				// Display the sign-in view controller.
				HandlePlatformSignInViewChange(true);
				[pRootViewControllerIOS presentViewController:pViewController animated:YES completion:nil];
				return;
			}

			// Specially identify cancellation.
			if (nil != pError)
			{
				NSInteger iCode = [pError code];
				if (GKErrorCancelled == iCode ||
					GKErrorUserDenied == iCode)
				{
					HandlePlatformUserCancellation();
				}
			}

			// If we get here, view controller is never visible.
			HandlePlatformSignInViewChange(false);

			// Handling.
			if (pLocalPlayer.isAuthenticated)
			{
				HandlePlatformChange(true, String(pLocalPlayer.playerID));
			}
			else
			{
				HandlePlatformChange(false, String());
			}
		};
	});
}

void IOSGameCenterPlatformSignInManager::OnPlatformChange(
	Bool bSignedIn,
	const String& sId)
{
	// Always run the trigger sign in event when this function exits
	auto const signInEvent(MakeDeferredAction([]()
	{
		Jobs::AsyncFunction(
			GetMainThreadId(),
			&PlatformSignInManager::TriggerSignInEvent);
	}));

	// Sanitize.
	if (sId.IsEmpty())
	{
		bSignedIn = false;
	}

	{
		Lock lock(m_Mutex);

		// Now that we're inside the lock, early out on
		// no change.
		if (bSignedIn == m_bSignedIn &&
			sId == m_sId &&
			m_bSignInSupported == m_bSignedIn)
		{
			// Clear before return.
			m_bSigningIn = false;
			return;
		}

		// Update.
		if (bSignedIn)
		{
			m_sId = sId;
		}
		else
		{
			m_sId = String();
		}

		// Updated signed in state.
		m_bSignedIn = bSignedIn;

		// Once the platform hook is registered, we must be authenticated
		// with platform services to support "sign in". It's a bit convoluted -
		// we always advertised that we support sign in until we've connected
		// to GameCenter. Once we've registered for the Game Center hook, we
		// can no longer sign in or out deliberately with platform services.
		//
		// As such, if the user cancels the Game Center sign in flow, we need
		// to advertise that we don't support sign in (so, for example, the
		// settings screen no longer displays the "sign in" button - since
		// we can't actually fulfill that request).
		m_bSignInSupported = m_bSignedIn;
	}

	// Update status changed.
	SeoulMemoryBarrier();

	// Changes
	++m_ChangeCount;

	// Finally, update signing in status.
	SeoulMemoryBarrier();
	m_bSigningIn = false;
}

void IOSGameCenterPlatformSignInManager::OnPlatformSignInViewChange(Bool bShowing)
{
	// Early out if already in the requested state.
	if (m_bShowingSignInView == bShowing)
	{
		return;
	}

	m_bShowingSignInView = bShowing;
	if (bShowing)
	{
		Engine::Get()->PauseTickTimer();
	}
	else
	{
		Engine::Get()->UnPauseTickTimer();
	}
}

void IOSGameCenterPlatformSignInManager::OnPlatformUserCancellation()
{
	// Tracking.
	auto const count = ++m_CancellationCount;
	(void)count;

	SEOUL_LOG_AUTH("Sign-in cancelled, cancellation count is now '%d'",
		(Int)count);
}

} // namespace Seoul

#endif // SEOUL_WITH_GAMECENTER
