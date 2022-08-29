/**
 * \file IOSApplePlatformSignInManager.mm
 * \brief IOS-specific platform sign-in manager. Implemented
 * with GameCenter API.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "IOSApplePlatformSignInManager.h"

#if SEOUL_WITH_APPLESIGNIN

#include "HTTPManager.h"
#include "HTTPRequest.h"
#include "HTTPRequestCancellationToken.h"
#include "IOSEngine.h"
#include "JobsFunction.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "ScopedAction.h"

#import "IOSKeychain.h"

#import  <AuthenticationServices/AuthenticationServices.h>

@interface IOSApplePlatformSignInDelegate : NSObject<ASAuthorizationControllerDelegate>;
@end

@implementation IOSApplePlatformSignInDelegate

- (void) authorizationController:(ASAuthorizationController*) controller
	didCompleteWithAuthorization:(ASAuthorization*) authorization API_AVAILABLE(ios(13.0))
{
	using namespace Seoul;

	if ([authorization.credential isKindOfClass:[ASAuthorizationAppleIDCredential class]])
	{
		ASAuthorizationAppleIDCredential* pAppleIDCredential = (ASAuthorizationAppleIDCredential*)authorization.credential;

		String sUser = String(pAppleIDCredential.user);
		String sIdToken = String((Seoul::Byte*)pAppleIDCredential.identityToken.bytes,
							   // Identity Token should not be longer than a UInt
							   (UInt)pAppleIDCredential.identityToken.length);
		String sAuthCode = String((Seoul::Byte*)pAppleIDCredential.authorizationCode.bytes,
								  // Auth Code should not be longer than a UInt
								  (UInt)pAppleIDCredential.authorizationCode.length);

		SEOUL_LOG_AUTH("IOS Apple Sign-in: Successful login, User is: %s, Auth Code is: %s, ID Token is: %s",
					   sUser.CStr(),
					   sAuthCode.CStr(),
					   sIdToken.CStr());

		IOSApplePlatformSignInManager::HandlePlatformChange(true,
															sUser,
															sIdToken,
															sAuthCode);
	}
	else
	{
		SEOUL_WARN("IOSApplePlatformSignInDelegate: Received credential of unknown type."
				   " Could not complete Apple sign-in.");
		IOSApplePlatformSignInManager::HandlePlatformChange(false, String(), String(), String());
	}
}

- (void) authorizationController:(ASAuthorizationController*) controller
			didCompleteWithError:(NSError*) error API_AVAILABLE(ios(13.0))
{
	using namespace Seoul;

	if (error != nil && error.code == ASAuthorizationErrorCanceled)
	{
		SEOUL_LOG_AUTH("IOS Apple Sign-in: user canceled login.");
		IOSApplePlatformSignInManager::HandlePlatformUserCancellation();
	}
	// We treat "ErrorUnknown" like a cancellation, because Apple throws this error
	// at us if the user is not signed in with Apple on their device. In this case,
	// we want to treat our prompt to sign in with Apple as if it got canceled to
	// avoid continued prompting of the user to sign in.
	else if (error != nil && error.code == ASAuthorizationErrorUnknown)
	{
		SEOUL_LOG_AUTH("IOS Apple Sign-in: unknown sign-in error. This can happen if the user"
			" was not signed in to their Apple ID on the device, or if they did not enable two"
			" factor authentication.");
		IOSApplePlatformSignInManager::HandlePlatformUserCancellation();
	}

	SEOUL_LOG_AUTH("IOS Apple Sign-in: there was an error while logging in.");
	IOSApplePlatformSignInManager::HandlePlatformChange(false, String(), String(), String());
}

@end

@interface IOSApplePlatformSignInContextProvider : NSObject<ASAuthorizationControllerPresentationContextProviding>;
@end

@implementation IOSApplePlatformSignInContextProvider

- (nonnull ASPresentationAnchor)presentationAnchorForAuthorizationController:
	(nonnull ASAuthorizationController *)controller API_AVAILABLE(ios(13.0))
{
	using namespace Seoul;

	UIViewController* pRootViewController = IOSApplePlatformSignInManager::Get()->GetRootViewController();
	// This is required to be nonnull, so just return it; if it's null we have to crash anyways
	return pRootViewController.view.window;
}

@end

namespace Seoul
{

static NSString* const ksIOSApplePlatformSignInManager_UserIDKey = @"AppleUserIdentifier";
static NSString* const ksIOSApplePlatformSignInManager_SessionTokenKey = @"AppleSessionToken";
static inline Bool GetSavedCredentials(String& sUserID, String& sSessionToken)
{
	NSString* pUser = [IOSKeychain
					   getValueForKey:ksIOSApplePlatformSignInManager_UserIDKey
					   searchCloud:false];
	NSString* pToken = [IOSKeychain
						getValueForKey:ksIOSApplePlatformSignInManager_SessionTokenKey
						searchCloud:false];

	if (pUser != nil && pToken != nil)
	{
		sUserID = String(pUser);
		sSessionToken = String(pToken);
		return true;
	}
	return false;
}
static inline void SetSavedCredentials(const String& sUserID,
									   const String& sSessionToken)
{
	[IOSKeychain
	 setKey:ksIOSApplePlatformSignInManager_UserIDKey
	 toValue:sUserID.ToNSString()
	 isInvisible:true
	 saveToCloud:false];

	[IOSKeychain
	setKey:ksIOSApplePlatformSignInManager_SessionTokenKey
	toValue:sSessionToken.ToNSString()
	isInvisible:true
	saveToCloud:false];
}
static inline void RemoveSavedCredentials()
{
	(void)[IOSKeychain
	 removeKey:ksIOSApplePlatformSignInManager_UserIDKey
	 isCloud:false];

	(void)[IOSKeychain
	removeKey:ksIOSApplePlatformSignInManager_SessionTokenKey
	isCloud:false];
}

IOSApplePlatformSignInManager::IOSApplePlatformSignInManager()
	: m_pRootViewController(nil)
	, m_pAppleSignInDelegate(nil)
	, m_pAppleSignInContextProvider(nil)
	, m_CancellationCount(0)
	, m_Mutex()
	, m_ChangeCount(0)
	, m_bInitialized(false)
	, m_bSignedIn(false)
	, m_bSigningIn(false)
	, m_sUserID()
	, m_sIdentityToken()
	, m_sAuthCode()
	, m_sSessionToken()
{
}

void IOSApplePlatformSignInManager::Initialize(void* pRootViewController)
{
	Lock lock(m_Mutex);

	// Early out.
	if (m_bInitialized)
	{
		return;
	}
	// Initialized.
	m_bInitialized = true;

	// Cache controller for future use.
	m_pRootViewController = (UIViewController*)pRootViewController;
	[m_pRootViewController retain];

	m_pAppleSignInDelegate = [[IOSApplePlatformSignInDelegate alloc] init];
	m_pAppleSignInContextProvider = [[IOSApplePlatformSignInContextProvider alloc] init];

	String sUserID;
	String sSessionToken;
	if (GetSavedCredentials(sUserID, sSessionToken))
	{
		if (!sUserID.IsEmpty() && !sSessionToken.IsEmpty())
		{
			SEOUL_LOG_AUTH("IOS Apple Sign-in: Read credentials on startup. "
						   "User is %s, Session Token is %s",
						   sUserID.CStr(),
						   sSessionToken.CStr());

			m_sUserID = sUserID;
			m_sSessionToken = sSessionToken;
		}
	}
}

IOSApplePlatformSignInManager::~IOSApplePlatformSignInManager()
{
	Lock lock(m_Mutex);

	// Early out.
	if (!m_bInitialized)
	{
		return;
	}

	// Deinitialize.
	m_bInitialized = false;

	[m_pAppleSignInDelegate release];
	m_pAppleSignInDelegate = nil;

	[m_pAppleSignInContextProvider release];
	m_pAppleSignInContextProvider = nil;

	// Release the view controller.
	[m_pRootViewController release];
	m_pRootViewController = nil;
}

Bool IOSApplePlatformSignInManager::IsSignInSupported() const
{
	// Apple Sign-in requires iOS 13+.
	// Apple's linter literally requires this to be in an "if" statement
	// and not just a "return".
	if (@available(iOS 13.0, *))
	{
		return true;
	}
	return false;
}

void IOSApplePlatformSignInManager::SignIn()
{
	// These functions are essentially synchronized.
	// It is not expected that we want more than one
	// in flight at once.
	Lock lock(m_Mutex);

	// Nop if signing in.
	if (m_bSigningIn)
	{
		return;
	}
	// Indicate sign in completion if we can't
	// or already have.
	if (m_bSignedIn || !IsSignInSupported())
	{
		Jobs::AsyncFunction(
			GetMainThreadId(),
			&PlatformSignInManager::TriggerSignInEvent);
		return;
	}

	SEOUL_LOG_AUTH("IOS Apple Sign-in: starting login process.");
	InternalSignIn();
}

void IOSApplePlatformSignInManager::SignOut()
{
	// These functions are essentially synchronized.
	// It is not expected that we want more than one
	// in flight at once.
	Lock lock(m_Mutex);

	// If signed out or signing out, nop.
	if (!m_bSignedIn || m_bSigningIn)
	{
		return;
	}

	SEOUL_LOG_AUTH("IOS Apple Sign-in: logging out.");
	HandlePlatformChange(false, String(), String(), String());
}

struct IOSAppleSignInCredentials SEOUL_SEALED
{
	String m_sIdentityToken;
	String m_sAuthCode;
	String m_sSessionToken;
};
SEOUL_BEGIN_TYPE(IOSAppleSignInCredentials)
	SEOUL_PROPERTY_N("id_token", m_sIdentityToken)
	SEOUL_PROPERTY_N("auth_code", m_sAuthCode)
	SEOUL_PROPERTY_N("session_token", m_sSessionToken)
SEOUL_END_TYPE()
void IOSApplePlatformSignInManager::GetIDToken(const OnTokenReceivedDelegate& del)
{
	// Local value cache for block.
	auto const delegate = del;

	IOSAppleSignInCredentials credentials;
	{
		Lock lock(m_Mutex);
		// Immediate complete with no id if not signed in.
		if (!m_bSignedIn || m_bSigningIn)
		{
			delegate(String(), false);
			return;
		}

		if (!m_sSessionToken.IsEmpty())
		{
			credentials.m_sSessionToken = m_sSessionToken;
		}
		else
		{
			credentials.m_sIdentityToken = m_sIdentityToken;
			credentials.m_sAuthCode = m_sAuthCode;
		}
	}

	String sSerialized;
	if (!Reflection::SerializeToString(&credentials, sSerialized))
	{
		delegate(String(), false);
		return;
	}

	SEOUL_LOG_AUTH("IOS Apple Sign-in: Reporting successful credentials to callback: %s", sSerialized.CStr());

	delegate(sSerialized, true);
}

void IOSApplePlatformSignInManager::StartWithIdToken(HTTPRequest& rRequest)
{
	IOSAppleSignInCredentials credentials;
	{
		Lock lock(m_Mutex);

		// Immediate complete with no id if not signed in.
		if (!m_bSignedIn || m_bSigningIn)
		{
			rRequest.Start();
			return;
		}

		if (!m_sSessionToken.IsEmpty())
		{
			credentials.m_sSessionToken = m_sSessionToken;
		}
		else
		{
			credentials.m_sIdentityToken = m_sIdentityToken;
			credentials.m_sAuthCode = m_sAuthCode;
		}
	}

	String sSerialized;
	if (!Reflection::SerializeToString(&credentials, sSerialized))
	{
		rRequest.Start();
		return;
	}

	SEOUL_LOG_AUTH("Sending HTTP request with id token: %s", sSerialized.CStr());

	rRequest.AddPostData("AppleSignInCredentials", sSerialized);
	rRequest.Start();
}

void IOSApplePlatformSignInManager::SetIDToken(const String& sToken,
											   const String& sAssociatedOldToken)
{
	Lock lock(m_Mutex);

	if (!m_bSignedIn || m_bSigningIn)
	{
		return;
	}

	IOSAppleSignInCredentials credentials;
	if (!m_sSessionToken.IsEmpty())
	{
		credentials.m_sSessionToken = m_sSessionToken;
	}
	else
	{
		credentials.m_sIdentityToken = m_sIdentityToken;
		credentials.m_sAuthCode = m_sAuthCode;
	}

	String sSerializedOldToken;
	if (!Reflection::SerializeToString(&credentials, sSerializedOldToken))
	{
		sSerializedOldToken = String();
	}

	if (sAssociatedOldToken != sSerializedOldToken)
	{
		SEOUL_LOG_AUTH("IOS Apple Sign-in: did not set new token because old token was mismatched.");
		return;
	}

	m_sSessionToken = sToken;

	SetSavedCredentials(m_sUserID, m_sSessionToken);
}

void IOSApplePlatformSignInManager::HandlePlatformChange(Bool bSignedIn,
														 const String& sUser,
														 const String& sIdentityToken,
														 const String& sAuthCode)
{
	auto pMan = IOSApplePlatformSignInManager::Get();
	if (!pMan)
	{
		return;
	}

	pMan->OnPlatformChange(bSignedIn, sUser, sIdentityToken, sAuthCode);
}

void IOSApplePlatformSignInManager::HandlePlatformUserCancellation()
{
	auto pMan = IOSApplePlatformSignInManager::Get();
	if (!pMan)
	{
		return;
	}

	pMan->OnPlatformUserCancellation();
}

void IOSApplePlatformSignInManager::InternalSignIn()
{
	// iOS 13 only
	if (@available(iOS 13.0, *))
	{
		String sUserID;
		String sSessionToken;
		{
			Lock lock(m_Mutex);

			// Now signing in.
			m_bSigningIn = true;

			// Quickly cache current user credentials (with lock)
			sUserID = m_sUserID;
			sSessionToken = m_sSessionToken;
		}

		if (!sUserID.IsEmpty() && !sSessionToken.IsEmpty())
		{
			SEOUL_LOG_AUTH("IOS Apple Sign-in: checking saved credentials for user ID %s.", sUserID.CStr());

			ASAuthorizationAppleIDProvider* pAuthorizationProvider = [[ASAuthorizationAppleIDProvider alloc] init];

			// This is a non-network SDK call
			[pAuthorizationProvider
			 getCredentialStateForUserID:sUserID.ToNSString()
			 completion:^(ASAuthorizationAppleIDProviderCredentialState credentialState,
						  NSError * _Nullable error) {
				if (error != nil)
				{
					InternalPerformSignInRequest(sUserID);
					return; // inner block return
				}

				switch (credentialState)
				{
					case ASAuthorizationAppleIDProviderCredentialAuthorized:
					{
						SEOUL_LOG_AUTH("IOS Apple Sign-in: locally verified saved credentials.");
						{
							Lock lock(m_Mutex);

							// Our cached credentials are good, ensure they are saved and mark us signed in.
							SetSavedCredentials(sUserID, sSessionToken);
							m_bSignedIn = true;
							++m_ChangeCount;
							m_bSigningIn = false;
						}

						Jobs::AsyncFunction(
							GetMainThreadId(),
							&PlatformSignInManager::TriggerSignInEvent);
						return; // inner block return
					}
					case ASAuthorizationAppleIDProviderCredentialRevoked:
					case ASAuthorizationAppleIDProviderCredentialNotFound:
					case ASAuthorizationAppleIDProviderCredentialTransferred:
					default:
						InternalPerformSignInRequest(sUserID);
						return; // inner block return
				}
			}];

			[pAuthorizationProvider release];
			return;
		}

		// We had no saved credentials, go straight to sign-in
		InternalPerformSignInRequest(sUserID);
	}
	else
	{
		SEOUL_WARN("IOSApplePlatformSignInManager: Attempted sign-in when iOS version was less than 13!");
	}
}

void IOSApplePlatformSignInManager::InternalPerformSignInRequest(const String& sUser)
{
	// iOS 13 only
	if (@available(iOS 13.0, *))
	{
		SEOUL_LOG_AUTH("IOS Apple Sign-in: there were no locally saved credentials, or they could not be verified."
					   " Starting initial sign-in request.");

		// Create an authorization provider, which we use to create an authorization request that can
		// be used to request Apple sign-in
		ASAuthorizationAppleIDProvider* pAuthorizationProvider = [[ASAuthorizationAppleIDProvider alloc] init];

		ASAuthorizationAppleIDRequest* pAuthorizationRequest = [pAuthorizationProvider createRequest];
		if (!sUser.IsEmpty())
		{
			pAuthorizationRequest.user = sUser.ToNSString();
		}

		// Create an authorization controller we can use to execute our requests
		NSArray* requestArray = @[pAuthorizationRequest];
		ASAuthorizationController* pAuthorizationController = [[ASAuthorizationController alloc]
															   initWithAuthorizationRequests:requestArray];
		pAuthorizationController.presentationContextProvider = m_pAppleSignInContextProvider;
		pAuthorizationController.delegate = m_pAppleSignInDelegate;
		[pAuthorizationController performRequests];

		// Cleanup
		[pAuthorizationProvider release];
		[pAuthorizationController release];
	}
	else
	{
		SEOUL_WARN("IOSApplePlatformSignInManager: Attempted sign-in when iOS version was less than 13!");
	}
}

void IOSApplePlatformSignInManager::OnPlatformChange(
	Bool bSignedIn,
	const String& sUser,
	const String& sIdentityToken,
	const String& sAuthCode)
{
	// Always run the trigger sign in event when this function exits.
	auto const signInEvent(MakeDeferredAction([]()
	{
		Jobs::AsyncFunction(
			GetMainThreadId(),
			&PlatformSignInManager::TriggerSignInEvent);
	}));

	// Sanitize.
	if (sUser.IsEmpty() || sIdentityToken.IsEmpty() || sAuthCode.IsEmpty())
	{
		SEOUL_LOG_AUTH("IOS Apple Sign-in: some credentials were empty, so sign-in is being set to false.");
		bSignedIn = false;
	}

	{
		Lock lock(m_Mutex);

		// Now that we're inside the lock, early out on
		// no change.
		if (bSignedIn == m_bSignedIn &&
			sUser == m_sUserID &&
			sIdentityToken == m_sIdentityToken &&
			sAuthCode == m_sAuthCode)
		{
			// Clear before return.
			m_bSigningIn = false;
			return;
		}

		// Update.
		if (bSignedIn)
		{
			m_sUserID = sUser;
			m_sIdentityToken = sIdentityToken;
			m_sAuthCode = sAuthCode;
		}
		else
		{
			m_sUserID = String();
			m_sIdentityToken = String();
			m_sAuthCode = String();
		}

		// Updated signed in state.
		m_bSignedIn = bSignedIn;

		// If we are changing the state of our sign-in,
		// remove the saved token, as we are restarting
		// the flow.
		m_sSessionToken = String();

		// If we are changing the state of our sign-in,
		// remove the saved token, as we are restarting
		// the flow.
		RemoveSavedCredentials();

		// Update status changed.
		++m_ChangeCount;

		// Finally, update signing in status.
		m_bSigningIn = false;
	}
}

void IOSApplePlatformSignInManager::OnPlatformUserCancellation()
{
	// Tracking.
	auto const count = ++m_CancellationCount;
	(void)count;

	SEOUL_LOG_AUTH("Sign-in cancelled, cancellation count is now '%d'",
		(Int)count);
}

} // namespace Seoul

#endif // SEOUL_WITH_APPLESIGNIN
