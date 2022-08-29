-- Access to the native NetworkManager
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.




local NetworkManager = class_static 'NetworkManager'

local udNetworkManager = nil

function NetworkManager.cctor()

	udNetworkManager = CoreUtilities.NewNativeUserData(ScriptNetworkManager)
	if nil == udNetworkManager then

		error 'Failed instantiating ScriptNetworkManager.'
	end
end

function NetworkManager.NewMessenger(hostname, port, encryptionKeyBase32)

	return udNetworkManager:NewMessenger(hostname, port, encryptionKeyBase32)
end
return NetworkManager
-- /#if SEOUL_WITH_ANIMATION_2D
