-- Access to the native CommerceManager singleton.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.



local CommerceManager = class_static 'CommerceManager'

--- <summary>
--- Enumeration of result codes for PurchaseItem()
--- </summary>
--- <remarks>
--- NOTE: This must be kept in sync with the EPurchaseResult
--- enum in Java land and in CPP
--- </remarks>
EPurchaseResult = {

--[[ Purchase succeeded ]]
kResultSuccess = 0,

--[[ Unspecified failure ]]
kResultFailure = 1,

--[[ Purchase was canceled by the user ]]
kResultCanceled = 2,

--[[ Error communicating with server ]]
kResultNetworkError = 3,

--[[ User is not signed into an online profile ]]
kResultNotSignedInOnline = 4,

--[[ Steam is not running (PC-only) ]]
kResultSteamNotRunning = 5,

--[[ Steam settings are disabling the overlay (PC-only) ]]
kResultSteamOverlayDisabled = 6,

--[[ User is not authorized to make payments ]]
kResultCantMakePayments = 7,

--[[ Requested product is not available for purchase ]]
kResultProductUnavailable = 8,

--[[ Failure to consume since item is not owned ]]
kResultItemNotOwned = 9,

--[[
			On iOS: Indicates that the client is not allowed to perform the attempted action.
			On Google Play: Indicates a response code of FEATURE_NOT_SUPPORTED
			On Amazon: Indicates a response code of NOT_SUPPORTED
		]]
kResultClientInvalid = 10,

--[[
			On iOS: Indicates that one of the payment parameters was not recognized by the Apple App Store.
			On Google Play: Indicates a response code of DEVELOPER_ERROR
			On Amazon: Indicates a response code of ALREADY_PURCHASED
		]]
kResultPaymentInvalid = 11,

--[[ Raised by client handling if we already have a record of the given transaction id ]]
kResultDuplicate = 12,

--[[ Raised by the engine if there is already a purchase in flight ]]
kSeoulPurchaseInProgress = 13,

--[[ Raised by the engine if the item is not recognized maybe idk ]]
kSeoulUnknownItem = 14,

--[[ Raised by the engine if we somehow fail to record the new item ]]
kSeoulFailedToSetItem = 15,

--[[ Raised by platform-specific code when the 1st party API returns an error ]]
kInternalPlatformError = 16,

--[[
			Raised by platform-specific code when initialization was not completed
			On Android: no commerce manager was created
			On Steam: steam not initialized
		]]
kPlatformNotInitialized = 17,

--[[
			Platform specific, see notes/usage
			On iOS: Failed to find product in DoPurchaseItem
			On Android: Exception in purchase
			On Steam: HTTTP failure in purchase
			On developer: Product not found
		]]
kPlatformSpecificError1 = 18,

--[[
			Platform specific, see notes/usage
			On Android: Exception in purchase runnable
			On Steam: Failure to parse purchase response
		]]
kPlatformSpecificError2 = 19,

--[[
			Platform specific, see notes/usage
			On Android: Exception in consume
			On Steam: HTTP failure in settle
		]]
kPlatformSpecificError3 = 20,

--[[
			Platform specific, see notes/usage
			On Android: Exception in consume runnable
			On Steam: Failure to parse settle response
		]]
kPlatformSpecificError4 = 21,

--[[
			Platform specific, see notes/usage
			On Android: No purchase data in OnPurchaseComplete
		]]
kPlatformSpecificError5 = 22,

--[[
			Game specific, see notes/usage
		]]
kGameSpecificError1 = 23,

--[[
			Game specific, see notes/usage
		]]
kGameSpecificError2 = 24,

--[[
			Game specific, see notes/usage
		]]
kGameSpecificError3 = 25,

--[[
			Game specific, see notes/usage
		]]
kGameSpecificError4 = 26
}

--- <summary>
--- Enumeration of result codes for OnItemInfoRefreshed()
--- </summary>
--- <remarks>
--- NOTE: This must be kept in sync with the ERefreshResult
--- enum in CPP
--- </remarks>
ERefreshResult = {

--- <summary>
--- &amp;lt; Refresh succeeded with at least one valid product
--- </summary>
kRefreshSuccess = 0,

--- <summary>
--- &amp;lt; Unspecified failure
--- </summary>
kRefreshFailure = 1,

--- <summary>
--- &amp;lt; Refresh succeeded but no products are available
--- </summary>
kRefreshNoProducts = 2
}



function CommerceManager.GetErrorDisplayText(eResult)

	repeat local _ = eResult; if _ == 3 then goto case elseif _ == 4 then goto case0 elseif _ == 7 then goto case1 elseif _ == 8 then goto case2 elseif _ == 9 then goto case3 elseif _ == 10 then goto case4 else goto case5 end

		::case::
			do return 'IAPError_NetworkError_Title', 'IAPError_NetworkError_Body' end

		::case0::
			do return 'IAPError_NotSignedInOnline_Title', 'IAPError_NotSignedInOnline_Body' end

		::case1::
			do return 'IAPError_CantMakePayments_Title', 'IAPError_CantMakePayments_Body' end

		::case2::
			do return 'IAPError_ProductUnavailable_Title', 'IAPError_ProductUnavailable_Body' end

		::case3::
			do return 'IAPError_ItemNotOwned_Title', 'IAPError_ItemNotOwned_Body' end

		::case4::
			do return 'IAPError_ClientInvalid_Title', 'IAPError_ClientInvalid_Body' end

		::case5::
			do return 'IAPError_Failure_Title', 'IAPError_Failure_Body' end
	until true
end

local udCommerceManager = nil



function CommerceManager.cctor()

	udCommerceManager = CoreUtilities.NewNativeUserData(ScriptEngineCommerceManager)
	if nil == udCommerceManager then

		error 'Failed instantiating ScriptEngineCommerceManager.'
	end

	-- Native to script invoke when the list of purchaseable IAPs via CommerceManager
	-- has been updated.
	_G.HANDLER_GlobalOnItemInfoRefreshed = function(eResult)

		repeat local _ = eResult; if _ == 0 then goto case elseif _ == 1 then goto case0 elseif _ == 2 then goto case1 else goto case2 end

			::case::
				--[[CoreNative.LogChannel(LoggerChannel.Commerce, 'Refreshed IAP item info, inform UIs')]]
				UIManager.BroadcastEvent 'HANDLER_OnItemInfoRefreshed'
				break

			::case0::
				--[[CoreNative.LogChannel(LoggerChannel.Commerce, 'Failed to refresh IAP item info')]]
				break

			::case1::
				--[[CoreNative.LogChannel(LoggerChannel.Commerce, 'Refresh of IAP item info found no products')]]
				break

			::case2::
				--[[CoreNative.LogChannel(
					LoggerChannel.Commerce,
					'Received invalid result from item info refresh ' ..tostring(
					eResult) ..
					', did you update CommerceManager ERefreshResult in Native and fail to update script?')]]
				break
		until true
	end
end

function CommerceManager.GetItemPrice(sItemID)

	local sRet = udCommerceManager:GetItemPrice(sItemID)
	return sRet
end

function CommerceManager.FormatPrice(amount, currencyString)

	local sRet, dRet = udCommerceManager:FormatPrice(amount, currencyString)
	return sRet, dRet
end

function CommerceManager.HasAllItemInfo()

	local bRet = udCommerceManager:HasAllItemInfo()
	return bRet
end

function CommerceManager.PurchaseItem(sItemID)

	udCommerceManager:PurchaseItem(sItemID)
end

function CommerceManager.OnItemPurchaseFinalized(sItemID, sFirstPartyTransactionID)

	udCommerceManager:OnItemPurchaseFinalized(sItemID, sFirstPartyTransactionID)
end

function CommerceManager.GetCommercePlatformId()

	return udCommerceManager:GetCommercePlatformId()
end

function CommerceManager.GetProductInfo(sItemID)

	return udCommerceManager:GetProductInfo(sItemID)
end
return CommerceManager
