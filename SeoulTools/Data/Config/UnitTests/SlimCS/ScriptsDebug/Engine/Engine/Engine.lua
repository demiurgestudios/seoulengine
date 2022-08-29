-- Access to the native Engine singleton.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.





local Engine = class_static 'Engine'

local udEngine = nil

function Engine.cctor()

	udEngine = CoreUtilities.NewNativeUserData(ScriptEngine)
	if nil == udEngine then

		error 'Failed instantiating ScriptEngine.'
	end
end

function Engine.GetGameTimeInTicks()


	local iRet = udEngine:GetGameTimeInTicks()
	return iRet
end

function Engine.GetGameTimeInSeconds()


	local iRet = udEngine:GetGameTimeInSeconds()
	return iRet
end

function Engine.GetTimeInSecondsSinceFrameStart()


	local iRet = udEngine:GetTimeInSecondsSinceFrameStart()
	return iRet
end


function Engine.GetPlatformUUID()

	local sRet = udEngine:GetPlatformUUID()
	return sRet
end

function Engine.GetPlatformData()

	local sRet = udEngine:GetPlatformData()
	return sRet
end

function Engine.GetSecondsInTick()

	local fRet = udEngine:GetSecondsInTick()
	return fRet
end

function Engine.IsAutomationOrUnitTestRunning()

	local bRet = udEngine:IsAutomationOrUnitTestRunning()
	return bRet
end

function Engine.GetThisProcessId()

	local iRet = udEngine:GetThisProcessId()
	return iRet
end

function Engine.HasNativeBackButtonHandling()

	local bRet = udEngine:HasNativeBackButtonHandling()
	return bRet
end

function Engine.OpenURL(sURL)

	local bRet = udEngine:OpenURL(sURL)
	return bRet
end

function Engine.NetworkPrefetch(filePath)

	local bRet = udEngine:NetworkPrefetch(filePath)
	return bRet
end

function Engine.PostNativeQuitMessage()

	local bRet = udEngine:PostNativeQuitMessage()
	return bRet
end

function Engine.ScheduleLocalNotification(iNotificationID, fireDate, sLocalizedMessage, sLocalizedAction)

	udEngine:ScheduleLocalNotification(iNotificationID, fireDate, sLocalizedMessage, sLocalizedAction)
end

function Engine.CancelLocalNotification(iNotificationID)

	udEngine:CancelLocalNotification(iNotificationID)
end

function Engine.CancelAllLocalNotifications()

	udEngine:CancelAllLocalNotifications()
end

function Engine.HasEnabledRemoteNotifications()

	return udEngine:HasEnabledRemoteNotifications()
end

function Engine.CanRequestRemoteNotificationsWithoutPrompt()

	return udEngine:CanRequestRemoteNotificationsWithoutPrompt()
end

function Engine.SetCanRequestRemoteNotificationsWithoutPrompt(canRequest)

	udEngine:SetCanRequestRemoteNotificationsWithoutPrompt(canRequest)
end

function Engine.GetCurrentServerTime()

	return udEngine:GetCurrentServerTime()
end

function Engine.ShowAppStoreToRateThisApp()

	udEngine:ShowAppStoreToRateThisApp()
end

function Engine.DoesSupportInAppRateMe()

	return udEngine:DoesSupportInAppRateMe()
end

function Engine.URLEncode(sURL)

	local sRet = udEngine:URLEncode(sURL)
	return sRet
end

function Engine.WriteToClipboard(toWrite)

	local bRet = udEngine:WriteToClipboard(toWrite)
	return bRet
end

function Engine.GetGameSecondsSinceStartup()

	return udEngine:GetGameSecondsSinceStartup()
end
return Engine
