/**
 * \file FacebookManager.cpp
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

#include "EngineVirtuals.h"
#include "FacebookManager.h"
#include "FacebookImageManager.h"
#include "EventsManager.h"

namespace Seoul
{

FacebookManager::FacebookManager()
{
	SEOUL_NEW(MemoryBudgets::TBD) FacebookImageManager;
}

FacebookManager::~FacebookManager()
{
	SEOUL_DELETE FacebookImageManager::Get();
}

/**
 * Called on the main thread when our Facebook login status has changed
 */
void FacebookManager::OnFacebookLoginStatusChanged()
{
	g_pEngineVirtuals->OnFacebookLoginStatusChanged();

	Events::Manager::Get()->TriggerEvent(FacebookStatusChangedEventId);

	// If we just finished logging in and we have pending requests to delete,
	// process those now
	if (!m_vRequestsToDelete.IsEmpty() && IsLoggedIn())
	{
		Vector<String> vRequestsToDelete;
		vRequestsToDelete.Swap(m_vRequestsToDelete);

		for (UInt i = 0; i < vRequestsToDelete.GetSize(); i++)
		{
			DeleteRequest(vRequestsToDelete[i]);
		}
	}
}

void FacebookManager::OnReturnFriendsList(String sFriendsListStr)
{
	g_pEngineVirtuals->OnFacebookFriendsListReturned(sFriendsListStr);
}

void FacebookManager::OnSentRequest(String sRequestID, String sRecipients, String sData)
{
	g_pEngineVirtuals->OnFacebookSentRequest(sRequestID, sRecipients, sData);
}

void FacebookManager::SetFacebookID(const String& sID)
{
	m_sMyFacebookID = sID;
}

void FacebookManager::OnReceiveBatchUserInfo(const String& sID, const String& sName)
{
	g_pEngineVirtuals->OnFacebookGetBatchUserInfo(sID, sName);
}

void FacebookManager::OnReceiveBatchUserInfoFailed(const String& sID)
{
	g_pEngineVirtuals->OnFacebookGetBatchUserInfoFailed(sID);
}

} // namespace Seoul
