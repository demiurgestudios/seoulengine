/**
 * \file TrackingManager.h
 * \brief Manager for user tracking. Complement to AnalyticsManager,
 * focused on user acquisition middleware.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef TRACKING_MANAGER_H
#define TRACKING_MANAGER_H

#include "CommerceManager.h"            // for CommerceManager::ItemInfo
#include "DataStore.h"                  // for DataStore
#include "Delegate.h"
#include "Prereqs.h"                    // for SEOUL_ABSTRACT
#include "Singleton.h"                  // for Singleton
namespace Seoul { struct AnalyticsSessionChangeEvent; }

namespace Seoul
{

enum class TrackingManagerType
{
	kAndroid,
	kIOS,
	kNull,
	kTesting,
};

/** Common base class for tracking. */
class TrackingManager SEOUL_ABSTRACT : public Singleton<TrackingManager>
{
public:
	SEOUL_DELEGATE_TARGET(TrackingManager);

	virtual ~TrackingManager();

	virtual TrackingManagerType GetType() const = 0;

	// Return the user ID used by external tracking middleware, or the
	// empty string if not defined. This is equivalent to the user ID
	// set by SetTrackingUserID for middleware that does not allow us
	// to override the value.
	virtual String GetExternalTrackingUserID() const = 0;

	// For certain special URL domains (e.g. helpshift://), route the
	// URL through the third party handling.
	virtual Bool OpenThirdPartyURL(const String& sURL) const = 0;

	// Commit our app's user ID to TrackingManager. Typically,
	// tracking is not enabled until this ID is set/made available.
	virtual void SetTrackingUserID(const String& sUserName, const String& sUserID) = 0;
	
	// Some third party SDKs need to show a Help landing page.
	// TODO: Separate implementations for Aquisition and CS SDKs?
	virtual Bool ShowHelp() const = 0;

	// Track analytic events with third-party SDKs that are interested in these events.
	virtual void TrackEvent(const String& sEventName) = 0;

	// Push update user data to SDKs (e.g. HelpShift) that use this for tracking
	// purposes.
	void UpdateUserData(const DataStore& customData, const DataStore& metaData)
	{
		Lock lock(m_UserDataMutex);
		m_UserCustomData.CopyFrom(customData);
		m_UserMetaData.CopyFrom(metaData);
	}

protected:
	TrackingManager();

	virtual void OnSessionChange(const AnalyticsSessionChangeEvent& evt)
	{
		// Nop by default.
	}

	Mutex m_UserDataMutex;
	DataStore m_UserCustomData;
	DataStore m_UserMetaData;

private:
	SEOUL_DISABLE_COPY(TrackingManager);
}; // class TrackingManager

/** Null tracking for platforms which do not use tracking. */
class NullTrackingManager SEOUL_SEALED : public TrackingManager
{
public:
	NullTrackingManager()
	{
	}

	~NullTrackingManager()
	{
	}

	virtual TrackingManagerType GetType() const SEOUL_OVERRIDE { return TrackingManagerType::kNull; }

	virtual String GetExternalTrackingUserID() const SEOUL_OVERRIDE
	{
		return String();
	}

	virtual Bool OpenThirdPartyURL(const String& sURL) const SEOUL_OVERRIDE
	{
		// Nop
		return false;
	}
	
	virtual Bool ShowHelp() const SEOUL_OVERRIDE
	{
		// Nop
		return false;
	}

	virtual void SetTrackingUserID(const String& sUserName, const String& sUserID) SEOUL_OVERRIDE
	{
		// Nop
	}

	virtual void TrackEvent(const String& sEventName) SEOUL_OVERRIDE
	{
		// Nop
	}

private:
	SEOUL_DISABLE_COPY(NullTrackingManager);
}; // class NullTrackingManager

} //namespace Seoul

#endif // include guard
