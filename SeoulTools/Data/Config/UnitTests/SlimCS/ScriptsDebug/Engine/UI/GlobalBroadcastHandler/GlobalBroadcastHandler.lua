-- Root MovieClip that allows non movie clips to register for ticks and
-- register broadcast handlers. Registering must happen at the class
-- level rather than the instance level
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.



local GlobalBroadcastHandler = class('GlobalBroadcastHandler', 'RootMovieClip')





local BroadcastList = {}
local TickFunction = nil

function GlobalBroadcastHandler:Constructor() RootMovieClip.Constructor(self)

	-- Take the list of registered handlers and set them to be functions
	--  on this instance
	for sKey, oFn in pairs(BroadcastList) do

		self[sKey] = function(_, ...)

			oFn(...)
		end
	end

	-- Call registered tick functions
	self:AddEventListener(
		'tick'--[[Event.TICK]],
		function(_, ...)

			if nil ~= TickFunction then

				TickFunction()
			end
		end)
end

-- Register broadcast event handlers
function GlobalBroadcastHandler.RegisterGlobalBroadcastEventHandler(sKey, oFunction)

	if nil ~= sKey then

		if nil ~= BroadcastList[sKey] then

			local oOriginalFunction = BroadcastList[sKey]
			BroadcastList[sKey] = function(...)

				oOriginalFunction(...)
				oFunction(...)
			end

		else

			BroadcastList[sKey] = oFunction
		end
	end
end

-- Register tick functions
function GlobalBroadcastHandler.RegisterGlobalTickEventHandler(oFunction)

	if nil ~= TickFunction then

		local oOriginalFunction = TickFunction
		TickFunction = function()

			oOriginalFunction()
			oFunction()
		end

	else

		TickFunction = oFunction
	end
end
return GlobalBroadcastHandler
