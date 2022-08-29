/**
 * \file EngineVirtuals.h
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

#pragma once
#ifndef ENGINE_VIRTUALS_H
#define ENGINE_VIRTUALS_H

#include "Prereqs.h"
#include "CommerceManager.h"
namespace Seoul { class DataStore; }
namespace Seoul { class HString; }
namespace Seoul { class String; }
namespace Seoul { class WorldTime; }

namespace Seoul
{

struct PurchaseReceiptData SEOUL_SEALED
{
	// Payload of receipt and other first-party data.
	String m_sPayload{};
	// Additional payload for platforms with multiple receipts.
	String m_sPayload2{};
	// String name of store used to identify source of purchase.
	String m_sStore{};
	// Identifier for this transaction.
	String m_sTransactionID{};
	// Token to identify this purchase with the first party in the future.
	// This is not the same as the TransactionID in some platforms.
	String m_sPurchaseToken{};
}; // struct PurchaseReceiptData

// TODO: Stop-gap after removing GameApp.
//
// "Up references" from Engine into the running application. This is a miscellaneous
// bucket of handlers that are very application specific but need to be triggered
// or accessed by Engine code.
struct EngineVirtuals SEOUL_SEALED
{
	Bool (*CanHandlePurchasedItems)();

	// Called on (currently, Apple and iOS only) when the App
	// enters the background/loses focus and regains focus/leaves
	// the background.
	//
	// NOTE: Apple has a unspecified time limit for the app to give
	// up the foreground so don't do anything complicated here
	void (*OnEnterBackground)();
	void (*OnLeaveBackground)();

	// Equivalent to OnLeaveBackground/OnEnterBackground, but this is called when the app is
	// is no longer visible.
	//
	// For example a system dialog box will cause EnterBackground to be called, but not SessionEnd,
	// where as pressing the home button will cause both to be called.
	void (*OnSessionStart)(const WorldTime& timestamp);
	void (*OnSessionEnd)(const WorldTime& timestamp);

	void (*OnFacebookLoginStatusChanged)();
	void (*OnFacebookFriendsListReturned)(const String& sFriendsListJSON);
	void (*OnFacebookSentRequest)(const String& sRequestID, const String& sRecipients, const String& sData);
	void (*OnFacebookGetBatchUserInfo)(const String& sID, const String& sName);
	void (*OnFacebookGetBatchUserInfoFailed)(const String& sID);

	void (*OnItemPurchased)(HString itemID, CommerceManager::EPurchaseResult eResult, PurchaseReceiptData const* pReceiptData);
	void (*OnItemInfoRefreshed)(CommerceManager::ERefreshResult eResult);
	void (*OnSubscriptionsReceived)();

	Bool (*OnOpenURL)(const String& sURL);
	void (*OnReceivedOSNotification)(
		Bool bRemoteNotification,
		Bool bWasRunning,
		Bool bWasInForeground,
		const DataStore& userInfo,
		const String& sAlertBody);
}; // struct EngineVirtuals
extern EngineVirtuals const* g_pEngineVirtuals;

} // namespace Seoul

#endif // include guard
