-- Access to the native TrackingManager singleton.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.



local TrackingManager = class_static 'TrackingManager'

local udTrackingManager = nil

function TrackingManager.cctor()

	udTrackingManager = CoreUtilities.NewNativeUserData(ScriptEngineTrackingManager)
	if nil == udTrackingManager then

		error 'Failed instantiating ScriptEngineTrackingManager.'
	end
end



function TrackingManager.get_ExternalTrackingUserID()

	return udTrackingManager:GetExternalTrackingUserID()
end


function TrackingManager.TrackEvent(sEventName)

	udTrackingManager:TrackEvent(sEventName)
end
return TrackingManager
