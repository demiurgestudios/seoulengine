-- Core Globals, requiring this file causes global side effects
-- and adds functions and values to global tables.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.



local Globals = class_static 'Globals'

--- <summary>
--- Global dispose functions will be called
--- once just before the script environment
--- is destroyed.
--- </summary>
local s_aGlobalDispose;function Globals.RegisterGlobalDispose(func)

	table.insert(s_aGlobalDispose, func)
end

local s_aMainThreadInit;function Globals.RegisterGlobalMainThreadInit(func)

	table.insert(s_aMainThreadInit, func)
end

local s_aOnHotLoad;function Globals.RegisterOnHotLoad(func)

	table.insert(s_aOnHotLoad, func)
end

local s_aPostHotLoad;function Globals.RegisterPostHotLoad(func)

	table.insert(s_aPostHotLoad, func)
end

local s_aScriptInit;function Globals.RegisterOnScriptInitializeComplete(func)

	table.insert(s_aScriptInit, func)
end

s_aGlobalDispose = {}
s_aMainThreadInit = {}
s_aOnHotLoad = {}
s_aPostHotLoad = {}
s_aScriptInit = {}

local function Run(a)

	for _, func in ipairs(a) do

		pcall(func)
	end
end

function Globals.cctor()

	-- Global hook invoked by native at a point where
	-- this script VM is pending release.
	_G.SeoulDispose = function()

		-- Iterate over registered functions.
		local a = s_aGlobalDispose
		s_aGlobalDispose = {}

		Run(a)
	end

	_G.SeoulMainThreadInit = function()

		-- Iterate over registered functions.
		local a = s_aMainThreadInit
		s_aMainThreadInit = {}

		Run(a)
	end

	-- Global hook invoked by native when a hotload is about to occur.
	_G.SeoulOnHotload = function()

		-- Iterate over registered functions - unlike
		-- global dipose and init, we leave the table unmodified
		-- (this callback can be fired multiple times in the same
		-- VM instance).
		local a = s_aOnHotLoad

		Run(a)
	end

	-- Global hook invoked by native on the new VM when a hotload complete.
	_G.SeoulPostHotload = function()

		-- Iterate over registered functions.
		local a = s_aPostHotLoad
		s_aPostHotLoad = {}

		Run(a)
	end

	_G.ScriptInitializeComplete = function()

		-- Iterate over registered functions.
		local a = s_aScriptInit
		s_aScriptInit = {}

		Run(a)
	end
end
return Globals
