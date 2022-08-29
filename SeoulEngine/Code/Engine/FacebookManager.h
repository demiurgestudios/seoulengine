/**
 * \file FacebookManager.h
 * \brief Platform-agnostic Facebook SDK interface.
 *
 * Wraps the low-level, platform-specific (typically middleware
 * SDK) hooks of talking to Facebook.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FACEBOOK_MANAGER_H
#define FACEBOOK_MANAGER_H

#include "Delegate.h"
#include "SeoulString.h"
#include "Singleton.h"
#include "Vector.h"

namespace Seoul
{

enum class FacebookManagerType
{
	kAndroid,
	kDebugPC,
	kIOS,
	kNull,
};

static const HString FacebookStatusChangedEventId("FBStatusChangedEventId");

/**
 * Platform-agnostic interface to the various platform-specific Facebook SDKs.
 * If Facebook is not supported on the current platform, or if
 * SEOUL_WITH_FACEBOOK is 0, then this interface is still valid, but the only
 * valid subclass will be the NullFacebookManager.
 */
class FacebookManager SEOUL_ABSTRACT : public Singleton<FacebookManager>
{
public:
	SEOUL_DELEGATE_TARGET(FacebookManager);

	FacebookManager();

	/** Virtual destructor */
	virtual ~FacebookManager();

	virtual FacebookManagerType GetType() const = 0;
	
	/**
	 * Begins the Facebook login flow.  The user is asked if he wants to allow
	 * the app to have the given permissions.
	 */
	virtual void Login(const Vector<String>& vRequestedPermissions) = 0;
	
	/**
	 * Refresh the Access Token. Refreshing may update
	 * the current Facebook user ID. If the ID changed (for instance, if the
	 * user logs out), then the Facebook user ID becomes blank.
	 */
	virtual void RefreshAccessToken() = 0;

	/**
	 * Closes the current session but does not delete the user's current
	 * authentication token.
	 */
	virtual void CloseSession() = 0;

	/**
	 * Closes the current session and deletes the user's current authentication
	 * token.
	 */
	virtual void LogOff() = 0;

	/**
	 * Tests if the user is currently logged into Facebook
	 */
	virtual Bool IsLoggedIn() = 0;

	/**
	 * Gets the current Facebook authentication token, or the empty string if
	 * we don't have a token.
	 */
	virtual String GetAuthToken() = 0;

	virtual Bool FacebookLoginSupported() const = 0;

	virtual void GetFriendsList(const String& sFields, Int iUserLimit) = 0;

	virtual void SendRequest(const String& sMessage, const String& sTitle, const String& sRecipients, const String& sSuggestedFriends, const String& sData) = 0;

	virtual void DeleteRequest(const String& sRequestID) = 0;

	virtual void SendPurchaseEvent(Double fLocalPrice, const String& sCurrencyCode) = 0;

	virtual void TrackEvent(const String& sEvent) = 0;

	/** Post a link. */
	virtual void ShareLink(const String& sContentURL) = 0;

	const String& GetFacebookID() const
	{
		return m_sMyFacebookID;
	}

	virtual void RequestBatchUserInfo(const Vector<String>& vUserIDs) = 0;

	// Called once the user has a player guid reported.
	virtual void SetUserID(const String& sUserID) = 0;

protected:
	// Called on the main thread when our Facebook login status has changed
	void OnFacebookLoginStatusChanged();

	void OnReturnFriendsList(String sFriendsListStr);

	void OnSentRequest(String sRequestID, String sRecipients, String sData);

	void OnReceiveBatchUserInfo(const String& sID, const String& sName);

	void OnReceiveBatchUserInfoFailed(const String& sID);

	void SetFacebookID(const String& sID);

protected:
	String m_sMyFacebookID;

	/**
	 * List of pending requests to delete, in case we were asked to delete a
	 * request before a session was started
	 */
	Vector<String> m_vRequestsToDelete;
	
private:
	SEOUL_DISABLE_COPY(FacebookManager);
};

/**
 * Null interface for FacebookManager.  All methods fail or return default
 * values.
 */
class NullFacebookManager SEOUL_SEALED : public FacebookManager
{
public:
	NullFacebookManager()
	{
	}

	~NullFacebookManager()
	{
	}

	virtual FacebookManagerType GetType() const SEOUL_OVERRIDE { return FacebookManagerType::kNull; }

	virtual void Login(const Vector<String>& vRequestedPermissions) SEOUL_OVERRIDE
	{
		// No-op
	}

	virtual void RefreshAccessToken() SEOUL_OVERRIDE
	{
		// No-op
	}

	virtual void CloseSession() SEOUL_OVERRIDE
	{
		// No-op
	}

	virtual void LogOff() SEOUL_OVERRIDE
	{
		// No-op
	}

	virtual Bool IsLoggedIn() SEOUL_OVERRIDE
	{
		return false;
	}

	virtual String GetAuthToken() SEOUL_OVERRIDE
	{
		return String();
	}

	virtual Bool FacebookLoginSupported() const SEOUL_OVERRIDE
	{
		return false;
	}

	virtual void GetFriendsList(const String& sFields, Int iUserLimit) SEOUL_OVERRIDE
	{
		// No-op
	}

	virtual void SendRequest(const String& sMessage, const String& sTitle, const String& sRecipients, const String& sSuggestedFriends, const String& sData) SEOUL_OVERRIDE
	{
		// No-op
	}

	virtual void DeleteRequest(const String& sRequestID) SEOUL_OVERRIDE
	{
		// No-op
	}

	virtual void RequestBatchUserInfo(const Vector<String>& vUserIDs) SEOUL_OVERRIDE
	{
		// No-op
	}

	virtual void SendPurchaseEvent(Double fLocalPrice, const String& sCurrencyCode) SEOUL_OVERRIDE
	{
		// No-op
	}

	virtual void TrackEvent(const String& sEventID) SEOUL_OVERRIDE
	{
		// No-op
	}

	virtual void ShareLink(const String& sContentURL) SEOUL_OVERRIDE
	{
		// No-op
	}

	// Called once the user has a player guid reported.
	virtual void SetUserID(const String& sUserID) SEOUL_OVERRIDE
	{
		// No-op
	}

private:
	SEOUL_DISABLE_COPY(NullFacebookManager);
};

#if SEOUL_ENABLE_CHEATS && SEOUL_PLATFORM_WINDOWS
class DebugPCFacebookManager SEOUL_SEALED : public FacebookManager
{
public:
	DebugPCFacebookManager()
	{
	}

	~DebugPCFacebookManager()
	{
	}

	virtual FacebookManagerType GetType() const SEOUL_OVERRIDE { return FacebookManagerType::kDebugPC; }

	virtual void Login(const Vector<String>& vRequestedPermissions) SEOUL_OVERRIDE
	{
		// No-op
	}

	virtual void RefreshAccessToken() SEOUL_OVERRIDE
	{
		// No-op
	}

	virtual void CloseSession() SEOUL_OVERRIDE
	{
		// No-op
	}

	virtual void LogOff() SEOUL_OVERRIDE
	{
		// No-op
	}

	virtual Bool IsLoggedIn() SEOUL_OVERRIDE
	{
		return true;
	}

	virtual String GetAuthToken() SEOUL_OVERRIDE
	{
		return String();
	}

	virtual Bool FacebookLoginSupported() const SEOUL_OVERRIDE
	{
		return true;
	}

	virtual void GetFriendsList(const String& sFields, Int iUserLimit) SEOUL_OVERRIDE
	{
		static const String ksFriendListStr = "{\"data\":[{\"picture\":{\"data\":{\"is_silhouette\":true,\"url\":\"https://fbcdn-profile-a.akamaihd.net/static-ak/rsrc.php/v2/yo/r/UlIqmHJn-SK.gif\"}},\"id\":\"100006439434800\",\"first_name\":\"Richard\",\"name\":\"Richard Amfdcidcdhk Chaiwitz\"},{\"picture\":{\"data\":{\"is_silhouette\":true,\"url\":\"https://fbcdn-profile-a.akamaihd.net/static-ak/rsrc.php/v2/y9/r/IB7NOFmPw2a.gif\"}},\"id\":\"100006465260072\",\"first_name\":\"Maria\",\"name\":\"Maria Amfdfebfkgb Bowerssen\"},{\"picture\":{\"data\":{\"is_silhouette\":true,\"url\":\"https://fbcdn-profile-a.akamaihd.net/static-ak/rsrc.php/v2/y9/r/IB7NOFmPw2a.gif\"}},\"id\":\"100006469130060\",\"first_name\":\"Sandra\",\"name\":\"Sandra Amfdfiackfj Chaiwitz\"},{\"picture\":{\"data\":{\"is_silhouette\":true,\"url\":\"https://fbcdn-profile-a.akamaihd.net/static-ak/rsrc.php/v2/y9/r/IB7NOFmPw2a.gif\"}},\"id\":\"100006471288909\",\"first_name\":\"Jennifer\",\"name\":\"Jennifer Amfdgabhhiji Riceescubergwitzmanskysensonstein\"},{\"picture\":{\"data\":{\"is_silhouette\":true,\"url\":\"https://fbcdn-profile-a.akamaihd.net/static-ak/rsrc.php/v2/yo/r/UlIqmHJn-SK.gif\"}},\"id\":\"100006480448916\",\"first_name\":\"Harry\",\"name\":\"Harry Amfdhjddhiaf Adeagboman\"},{\"picture\":{\"data\":{\"is_silhouette\":true,\"url\":\"https://fbcdn-profile-a.akamaihd.net/static-ak/rsrc.php/v2/yo/r/UlIqmHJn-SK.gif\"}},\"id\":\"100006481769384\",\"first_name\":\"John\",\"name\":\"John Amfdhagfichd Laverdetberg\"},{\"picture\":{\"data\":{\"is_silhouette\":true,\"url\":\"https://fbcdn-profile-a.akamaihd.net/static-ak/rsrc.php/v2/y9/r/IB7NOFmPw2a.gif\"}},\"id\":\"100006488174361\",\"first_name\":\"Helen\",\"name\":\"Helen Amfdhhagdcfa Vijayvergiyaescu\"},{\"picture\":{\"data\":{\"is_silhouette\":true,\"url\":\"https://fbcdn-profile-a.akamaihd.net/static-ak/rsrc.php/v2/yo/r/UlIqmHJn-SK.gif\"}},\"id\":\"100006493544715\",\"first_name\":\"Rick\",\"installed\":true,\"name\":\"Rick Amfdiceddgae Sharpewitz\"},{\"picture\":{\"data\":{\"is_silhouette\":true,\"url\":\"https://fbcdn-profile-a.akamaihd.net/static-ak/rsrc.php/v2/yo/r/UlIqmHJn-SK.gif\"}},\"id\":\"100006509412256\",\"first_name\":\"John\",\"name\":\"John Amfejidabbef Narayanansen\"},{\"picture\":{\"data\":{\"is_silhouette\":true,\"url\":\"https://fbcdn-profile-a.akamaihd.net/static-ak/rsrc.php/v2/yo/r/UlIqmHJn-SK.gif\"}},\"id\":\"100006529210422\",\"first_name\":\"John\",\"name\":\"John Amfebibajdbb Occhinowitz\"},{\"picture\":{\"data\":{\"is_silhouette\":true,\"url\":\"https://fbcdn-profile-a.akamaihd.net/static-ak/rsrc.php/v2/y9/r/IB7NOFmPw2a.gif\"}},\"id\":\"100006537667126\",\"first_name\":\"Jennifer\",\"name\":\"Jennifer Amfecgffgabf Greeneman\"},{\"picture\":{\"data\":{\"is_silhouette\":true,\"url\":\"https://fbcdn-profile-a.akamaihd.net/static-ak/rsrc.php/v2/yo/r/UlIqmHJn-SK.gif\"}},\"id\":\"100006546667148\",\"first_name\":\"Will\",\"name\":\"Will Amfedfffgadh Moidusenskyescusteinwitzmanbergson\"},{\"picture\":{\"data\":{\"is_silhouette\":true,\"url\":\"https://fbcdn-profile-a.akamaihd.net/static-ak/rsrc.php/v2/y9/r/IB7NOFmPw2a.gif\"}},\"id\":\"100006566046488\",\"first_name\":\"Mary\",\"name\":\"Mary Amfeffjdfdhh Bowerssen\"},{\"picture\":{\"data\":{\"is_silhouette\":true,\"url\":\"https://fbcdn-profile-a.akamaihd.net/static-ak/rsrc.php/v2/yo/r/UlIqmHJn-SK.gif\"}},\"id\":\"100006580713150\",\"first_name\":\"David\",\"name\":\"David Amfehjgacaej Fergiesen\"},{\"picture\":{\"data\":{\"is_silhouette\":true,\"url\":\"https://fbcdn-profile-a.akamaihd.net/static-ak/rsrc.php/v2/yo/r/UlIqmHJn-SK.gif\"}},\"id\":\"100006600062777\",\"first_name\":\"Charlie\",\"name\":\"Charlie Amfflfbggg Ricestein\"},{\"picture\":{\"data\":{\"is_silhouette\":true,\"url\":\"https://fbcdn-profile-a.akamaihd.net/static-ak/rsrc.php/v2/y9/r/IB7NOFmPw2a.gif\"}},\"id\":\"100006611462571\",\"first_name\":\"Jennifer\",\"name\":\"Jennifer Amffaadfbega Bowersson\"},{\"picture\":{\"data\":{\"is_silhouette\":true,\"url\":\"https://fbcdn-profile-a.akamaihd.net/static-ak/rsrc.php/v2/yo/r/UlIqmHJn-SK.gif\"}},\"id\":\"100006618181957\",\"first_name\":\"Dave\",\"name\":\"Dave Amffahahaieg Smithskysteinwitzsensonbergmanescu\"}]}";
		OnReturnFriendsList(ksFriendListStr);
	}

	virtual void SendRequest(const String& sMessage, const String& sTitle, const String& sRecipients, const String& sSuggestedFriends, const String& sData) SEOUL_OVERRIDE
	{
		// No-op
	}

	virtual void DeleteRequest(const String& sRequestID)
	{
		// No-op
	}

	virtual void RequestBatchUserInfo(const Vector<String>& vUserIDs) SEOUL_OVERRIDE
	{
		// No-op
	}
	
	virtual void SendPurchaseEvent(Double fLocalPrice, const String& sCurrencyCode) SEOUL_OVERRIDE
	{
		// No-op
	}

	virtual void TrackEvent(const String& sEventID) SEOUL_OVERRIDE
	{
		// No-op
	}
	
	virtual void ShareLink(const String& sContentURL) SEOUL_OVERRIDE
	{
		// No-op
	}

	// Called once the user has a player guid reported.
	virtual void SetUserID(const String& sUserID) SEOUL_OVERRIDE
	{
		// No-op
	}

private:
	SEOUL_DISABLE_COPY(DebugPCFacebookManager);
};
#endif // #if SEOUL_ENABLE_CHEATS && SEOUL_PLATFORM_WINDOWS

} // namespace Seoul

#endif // include guard
