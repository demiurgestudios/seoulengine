-- Access to the native AnalyticsManager singleton.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.



local AnalyticsManager = class_static 'AnalyticsManager'

local udAnalyticsManager = nil

function AnalyticsManager.cctor()

	udAnalyticsManager = CoreUtilities.NewNativeUserData(ScriptEngineAnalyticsManager)
	if nil == udAnalyticsManager then

		error 'Failed instantiating ScriptEngineAnalyticsManager.'
	end
end

ProfileUpdateOp = {

--- <summary>
--- Unknown or invalid.
--- </summary>
Unknown = 0,

--- <summary>
--- Add a numeric value to an existing numeric value.
--- </summary>
Add = 1,

--- <summary>
--- Adds values to a list.
--- </summary>
Append = 2,

--- <summary>
--- Remove a value from an existing list.
--- </summary>
Remove = 3,

--- <summary>
--- Set a value to a named property, always.
--- </summary>
Set = 4,

--- <summary>
--- Set a value to a named property only if it is not already set.
--- </summary>
SetOnce = 5,

--- <summary>
--- Merge a list of values with an existing list of values, deduped.
--- </summary>
Union = 6,

--- <summary>
--- Permanently delete the list of named properties from the profile.
--- </summary>
Unset = 7
}

function AnalyticsManager.Flush()

	udAnalyticsManager:Flush()
end

function AnalyticsManager.GetAnalyticsSandboxed()

	return udAnalyticsManager:GetAnalyticsSandboxed()
end

function AnalyticsManager.GetStateProperties()

	return udAnalyticsManager:GetStateProperties()
end

function AnalyticsManager.TrackEvent(sEventName, tProperties)

	udAnalyticsManager:TrackEvent(sEventName, tProperties)
end

function AnalyticsManager.UpdateProfile(eOp, tUpdates)

	udAnalyticsManager:UpdateProfile(eOp, tUpdates)
end

function AnalyticsManager.GetSessionCount()

	return udAnalyticsManager:GetSessionCount()
end
return AnalyticsManager
