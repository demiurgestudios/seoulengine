/**
 * \file EngineVirtuals.cpp
 * \brief Psuedo-vtable global that encapsulates miscellaneous up references
 * from Engine into App-specific handlers and gameplay code.
 *
 * TODO: Stop-gap until this bit bubbles up high enough in priority
 * to warrant a better design. Ideally, we refactor relevant functionality
 * so that no up references are needed (or so that those up references are
 * injected in a more typical/expected way - e.g. polymorphic children).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EngineVirtuals.h"
#include "ReflectionDefine.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(PurchaseReceiptData)
	SEOUL_PROPERTY_N("Payload", m_sPayload)
	SEOUL_PROPERTY_N("Payload2", m_sPayload2)
	SEOUL_PROPERTY_N("Store", m_sStore)
	SEOUL_PROPERTY_N("TransactionID", m_sTransactionID)
SEOUL_END_TYPE()

// EngineVirtuals hookup.
static Bool DefaultCanHandlePurchasedItems() { return false; }
static void DefaultOnEnterBackground() {}
static void DefaultOnLeaveBackground() {}
static void DefaultOnSessionStart(const WorldTime& timestamp) {}
static void DefaultOnSessionEnd(const WorldTime& timestamp) {}
static void DefaultOnFacebookLoginStatusChanged() {}
static void DefaultOnFacebookFriendsListReturned(const String& sFriendsListJSON) {}
static void DefaultOnFacebookSentRequest(const String& sRequestID, const String& sRecipients, const String& sData) {}
static void DefaultOnFacebookGetBatchUserInfo(const String& sID, const String& sName) {}
static void DefaultOnFacebookGetBatchUserInfoFailed(const String& sID) {}
static void DefaultOnItemPurchased(HString itemID, CommerceManager::EPurchaseResult eResult, PurchaseReceiptData const* pReceiptData) {}
static void DefaultOnItemInfoRefreshed(CommerceManager::ERefreshResult eResult) {}
static void DefaultOnSubscriptionsReceived() {}
static Bool DefaultOnOpenURL(const String& sURL) { return false; }
static void DefaultOnReceivedOSNotification(Bool bRemoteNotification, Bool bWasRunning, Bool bWasInForeground, const DataStore& userInfo, const String& sAlertBody) {}
static EngineVirtuals const s_DefaultEngineVirtuals =
{
	DefaultCanHandlePurchasedItems,
	DefaultOnEnterBackground,
	DefaultOnLeaveBackground,
	DefaultOnSessionStart,
	DefaultOnSessionEnd,
	DefaultOnFacebookLoginStatusChanged,
	DefaultOnFacebookFriendsListReturned,
	DefaultOnFacebookSentRequest,
	DefaultOnFacebookGetBatchUserInfo,
	DefaultOnFacebookGetBatchUserInfoFailed,
	DefaultOnItemPurchased,
	DefaultOnItemInfoRefreshed,
	DefaultOnSubscriptionsReceived,
	DefaultOnOpenURL,
	DefaultOnReceivedOSNotification,
};
EngineVirtuals const* g_pEngineVirtuals = &s_DefaultEngineVirtuals;

} // namespace Seoul
