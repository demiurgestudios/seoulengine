/**
 * \file IOSFacebookManager.h
 * \brief iOS-specific Facebook SDK implementation. Wraps the
 * iOS Objective-C Facebook SDK.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef IOS_FACEBOOK_MANAGER_H
#define IOS_FACEBOOK_MANAGER_H

#include "FacebookManager.h"

#if SEOUL_WITH_FACEBOOK

@class NSURL;
@class FBSDKLoginManager;

namespace Seoul
{

/**
 * iOS-specific interface to the Facebook SDK
 */
class IOSFacebookManager SEOUL_SEALED : public FacebookManager
{
public:
	IOSFacebookManager();
	~IOSFacebookManager();

	/**
	 * @return The global singleton instance. Will be nullptr if that instance has not
	 * yet been created.
	 */
	static CheckedPtr<IOSFacebookManager> Get()
	{
		if (FacebookManager::Get() && FacebookManager::Get()->GetType() == FacebookManagerType::kIOS)
		{
			return (IOSFacebookManager*)FacebookManager::Get().Get();
		}
		else
		{
			return CheckedPtr<IOSFacebookManager>();
		}
	}

	virtual FacebookManagerType GetType() const SEOUL_OVERRIDE { return FacebookManagerType::kIOS; }
	
	// Begins the Facebook login flow.  The user is asked if he wants to allow
	// the app to have the given permissions.
	virtual void Login(const Vector<String>& vRequestedPermissions) SEOUL_OVERRIDE;
	
	// Refresh the current Facebook identity.
	virtual void RefreshAccessToken() SEOUL_OVERRIDE;

	// Closes the current session but does not delete the user's current
	// authentication token.
	virtual void CloseSession() SEOUL_OVERRIDE;

	// Closes the current session and deletes the user's current authentication
	// token.
	virtual void LogOff() SEOUL_OVERRIDE;

	// Tests if the user is currently logged into Facebook
	virtual Bool IsLoggedIn() SEOUL_OVERRIDE;

	// Gets the current Facebook authentication token, or the empty string if
	// we don't have a token.
	virtual String GetAuthToken() SEOUL_OVERRIDE;

	virtual Bool FacebookLoginSupported() const SEOUL_OVERRIDE
	{
		return true;
	}

	// Facebook Delegate Callback to apply gifts.
	virtual void ApplyGiftRequest(NSString* rid, NSString* recipients, NSString* data);

	virtual void GetFriendsList(const String& sFields, Int iUserLimit) SEOUL_OVERRIDE;

	virtual void SendRequest(const String& sMessage, const String& sTitle, const String& sRecipients, const String& sSuggestedFriends, const String& sData) SEOUL_OVERRIDE;

	virtual void DeleteRequest(const String& sRequestID) SEOUL_OVERRIDE;
	
	virtual void SendPurchaseEvent(Double fLocalPrice, const String& sCurrencyCode) SEOUL_OVERRIDE;

	virtual void TrackEvent(const String& sEventID) SEOUL_OVERRIDE;

	virtual void ShareLink(const String& sContentURL) SEOUL_OVERRIDE;

	// Called once the user has a player guid reported.
	virtual void SetUserID(const String& sUserID) SEOUL_OVERRIDE;

	virtual void RequestBatchUserInfo(const Vector<String>& vUserIDs) SEOUL_OVERRIDE;

	// IOS Hooks.
	void SetLaunchOptions(void* pApplication, void* pLaunchOptions);

	// Called from the AppDelegate to handle opening a Facebook URL, whose scheme
	// should be "fb<APPID>".
	static Bool HandleOpenURL(
		void* pInApplication,
		NSURL* url,
		NSString* sourceApplication,
		void* pInOptions,
		id annotation);

private:
	SEOUL_DISABLE_COPY(IOSFacebookManager);

	FBSDKLoginManager* m_FacebookLogin;

	Atomic32Value<Bool> m_bInitialized;
	void* m_pApplication = nullptr;
	void* m_pLaunchOptions = nullptr;
};

} // namespace Seoul

#endif  // SEOUL_WITH_FACEBOOK

#endif  // Include guard
