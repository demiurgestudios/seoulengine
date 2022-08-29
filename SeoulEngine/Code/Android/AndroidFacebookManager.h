/**
 * \file AndroidFacebookManager.h
 * \brief Specialization of FacebookManager for Android. Wraps
 * the Android Facebook (Java) SDK.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#ifndef ANDROID_FACEBOOK_MANAGER_H
#define ANDROID_FACEBOOK_MANAGER_H

#include "FacebookManager.h"

#if SEOUL_WITH_FACEBOOK

namespace Seoul
{

/**
 * iOS-specific interface to the Facebook SDK
 */
class AndroidFacebookManager SEOUL_SEALED : public FacebookManager
{
public:
	AndroidFacebookManager();
	~AndroidFacebookManager();

	/**
	 * Convenience function to get the AndroidFacebookManager singleton pointer
	 */
	static CheckedPtr<AndroidFacebookManager> Get()
	{
		if (FacebookManager::Get() && FacebookManager::Get()->GetType() == FacebookManagerType::kAndroid)
		{
			return (AndroidFacebookManager*)FacebookManager::Get().Get();
		}
		else
		{
			return CheckedPtr<AndroidFacebookManager>();
		}
	}

	virtual FacebookManagerType GetType() const SEOUL_OVERRIDE { return FacebookManagerType::kAndroid; }
	
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

	/**
	 * Public helper function to call through to OnFacebookLoginStatusChanged()
	 */
	void PublicOnFacebookLoginStatusChanged()
	{
		OnFacebookLoginStatusChanged();
	}

	virtual Bool FacebookLoginSupported() const SEOUL_OVERRIDE
	{
		return true;
	}

	virtual void GetFriendsList(const String& sFields, Int iUserLimit) SEOUL_OVERRIDE;

	virtual void SendRequest(const String& sMessage, const String& sTitle, const String& sRecipients, const String& sSuggestedFriends, const String& sData) SEOUL_OVERRIDE;

	virtual void DeleteRequest(const String& sRequestID) SEOUL_OVERRIDE;
	
	virtual void SendPurchaseEvent(Double fLocalPrice, const String& sCurrencyCode) SEOUL_OVERRIDE;

	virtual void TrackEvent(const String& sEventID) SEOUL_OVERRIDE;

	virtual void ShareLink(const String& sContentURL) SEOUL_OVERRIDE;

	virtual void SetUserID(const String& sUserID) SEOUL_OVERRIDE;

	virtual void RequestBatchUserInfo(const Vector<String>& vUserIDs) SEOUL_OVERRIDE;

	void PublicOnReturnFriendsList(const String& sStr)
	{
		OnReturnFriendsList(sStr);
	}

	void PublicOnSentRequest(const String& sRequestID, const String& sRecipients, String sData)
	{
		OnSentRequest(sRequestID, sRecipients, sData);
	}

	void PublicSetFacebookID(const String& sID)
	{
		SetFacebookID(sID);
	}

	void PublicOnGetBatchUserInfo(const String& sID, const String& sName)
	{
		OnReceiveBatchUserInfo(sID,sName);
	}

	void PublicOnGetBatchUserInfoFailed(const String& sID)
	{
		OnReceiveBatchUserInfoFailed(sID);
	}

private:
	SEOUL_DISABLE_COPY(AndroidFacebookManager);

	Atomic32Value<Bool> m_bInitialized;
};

} // namespace Seoul

#endif  // SEOUL_WITH_FACEBOOK

#endif // include guard
