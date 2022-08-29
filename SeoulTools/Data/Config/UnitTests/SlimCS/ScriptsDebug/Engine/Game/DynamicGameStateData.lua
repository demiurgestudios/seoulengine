-- Keeper of dynamic game state data, global information store that persists
-- (within the lifespan of a single script VM - it is not saved to disk
-- nor does it persist across a game restart).
--
-- "Dynamic" here indicates that keys can be set to any value without
-- type checking (e.g. a key could be set to a string or int at different
-- times). Generally, exploiting this is not recommended as it can be generally
-- confusing and create bugs.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.



local DynamicGameStateData = class_static 'DynamicGameStateData'; local tRootMovieClipRegistry

local udScriptManagerProxy = nil

local tGameStateData = {}

local s_DataTable = {}

-- For each value in tDataTable, contains a list of {path to data table element, metatable class name}
-- pairs to allow us to store objects with a metatable through the ReceiveState/RestoreState pipeline
local s_DataMetatableTable = {}
local tPreviousValueTable = {}





local function IsStorableValue(value, key, sKeyPath)

	if type(value) == 'function' then

		-- We can't store a function
		table.insert(sKeyPath, key)
		local sPath = ''
		for _, sPathKey in ipairs(sKeyPath) do

			sPath = tostring(sPath) ..tostring(sPathKey) .. '/'
		end
		CoreNative.Warn("IsStorableValue can't store function, path=" .. tostring(sPath))
		CoreNative.Warn('-----------value---------------------------------------')
		CoreUtilities.LogTableOrTostring('Warning'--[[LoggerChannel.Warning]], value)
		CoreNative.Warn('-------------------------------------------------------')
		table.remove(sKeyPath, #sKeyPath)
		return false

	elseif not (type(value) == 'table') then

		-- If its not a function and not a table, its fine to store
		return true
	end

	-- At this point, we know its a table

	table.insert(sKeyPath, key)
	do local res, ret = tryfinally(function()

		-- If it has a non-class metatable, we can't store it
		local metatable = getmetatable(value)
		if nil ~= metatable and SlimCSStringLib.IsNullOrEmpty(metatable.m_sClassName) then

			local sPath = ''
			for _, sPathKey in ipairs(sKeyPath) do

				sPath = tostring(sPath) ..tostring(sPathKey) .. '/'
			end
			CoreNative.Warn("IsStorableValue can't store a table value with a non-class metatable, path=" .. tostring(sPath))
			CoreNative.Warn('-----------value---------------------------------------')
			CoreUtilities.LogTableOrTostring('Warning'--[[LoggerChannel.Warning]], value)
			CoreNative.Warn('-----------metatable-----------------------------------')
			CoreUtilities.LogTable('Warning'--[[LoggerChannel.Warning]], metatable)
			CoreNative.Warn('-------------------------------------------------------')
			return 2, false
		end

		-- Check if any values in the table aren't storable
		for sKey, v in pairs(value) do

			if not IsStorableValue(sKey, sKey, sKeyPath) or not IsStorableValue(v, sKey, sKeyPath) then

				return 2, false
			end
		end
	end, function()


		table.remove(sKeyPath, #sKeyPath)
	end) if 2 == res then return ret end end
	return true
end

local AccumulateMetatables;local function InternalSetValue(sKey, value)

	s_DataTable[sKey] = value

	if nil == value then

		s_DataMetatableTable[sKey] = nil

	else

		local metadataTable = {}
		local prefix = {}
		AccumulateMetatables(value, prefix, metadataTable)
		s_DataMetatableTable[sKey] = metadataTable
	end
end

function AccumulateMetatables(value, prefix, results)

	if type(value) ~= 'table' then

		return
	end

	local metatable = getmetatable(value)
	if metatable ~= nil then

		local sClassName = metatable.m_sClassName

		if not SlimCSStringLib.IsNullOrEmpty(sClassName) then

			local p = CoreUtilities.ShallowCopyTable1_E365A2FF(Double,String, prefix)
			table.insert(
				results,

				{
					Prefix = p,
					ClassName = sClassName
				})
		end
	end

	for key, v in pairs(value) do

		table.insert(prefix, key)
		AccumulateMetatables(v, prefix, results)
		table.remove(prefix)
	end
end

local function ReattachMetatables()

	for sKey, value in pairs(s_DataTable) do

		local metatables = s_DataMetatableTable[sKey]

		for _, metatableMapping in ipairs(metatables) do

			local aPrefix = metatableMapping.Prefix
			local sClassName = metatableMapping.ClassName

			local v = value
			for _, key in ipairs(aPrefix) do

				v = v[key]
			end

			setmetatable(v, _G[sClassName])
		end
	end
end

local OnHotLoad;function DynamicGameStateData.cctor() tRootMovieClipRegistry = CoreUtilities.MakeWeakTable();

	udScriptManagerProxy = CoreUtilities.NewNativeUserData(Game_ScriptManagerProxy)
	if nil == udScriptManagerProxy then

		error 'Failed instantiating Game_ScriptManagerProxy.'
	end

	local function SetValueWithCheck(sKey, v, sName)

		if IsStorableValue(v, sKey, {}) then

			if v == nil and DynamicGameStateData.GetValue(Object, sKey) ~= nil then

				local oldVal = DynamicGameStateData.GetValue(Object, sKey)

				if oldVal == nil then

					tPreviousValueTable[sKey] = nil

				else

					local val = {}
					val.value = oldVal
					val.screen = sName
					tPreviousValueTable[sKey] = val
				end
			end

			InternalSetValue(sKey, v)

		else

			CoreNative.Warn('Trying to store Unstorable value in DynamicGameStateData, sKey: ' ..(sKey or '<none>') .. ' sName: ' ..(sName or '<none>'))
		end
	end

	local function SetValueNoCheck(sKey, v, _)

		InternalSetValue(sKey, v)
	end

	-- The check is expensive and only used to make hotloading safe
	-- we don't need it in ship.



	rawset(DynamicGameStateData, 'SetValue', SetValueWithCheck)


	_G.RestoreDynamicGameStateData = function()

		local tRestoreData = nil
		local tRestoreMetatableData = nil
		try(function()

			tRestoreData, tRestoreMetatableData = udScriptManagerProxy:RestoreState()
			if nil ~= tRestoreData and nil ~= tRestoreMetatableData then

				s_DataTable = tRestoreData
				s_DataMetatableTable = tRestoreMetatableData

				ReattachMetatables()

			else

				CoreNative.Warn 'Error restoring DynamicGameStateData, no data returned'
			end
		end,
		function() return true end, function()

			CoreNative.Warn('Error restoring DynamicGameStateData: ' .. tostring(tRestoreData))
		end)
	end

	Globals.RegisterOnHotLoad(OnHotLoad)
end

function DynamicGameStateData.GetValue(T, sKey)

	return s_DataTable[sKey]
end

function DynamicGameStateData.SetValueWithCheck(sKey, v, sName)

	local _ = sKey, v, sName
	error 'Failed to replace placeholder'
end

function DynamicGameStateData.UnsetValue(sKey, sOptionalName)

	DynamicGameStateData.SetValue(sKey, nil, sOptionalName)
end

function OnHotLoad()

	local movieTypeSet = {}

	for _, v in pairs(tRootMovieClipRegistry) do

		movieTypeSet[v] = true
	end

	for sKey, v in pairs(tPreviousValueTable) do

		if movieTypeSet[v.screen] ~= nil then

			InternalSetValue(sKey, v.value)
		end
	end

	udScriptManagerProxy:ReceiveState(s_DataTable, s_DataMetatableTable)
end

function DynamicGameStateData.RegisterRootMovieClip(rootMovieClip, sMovieTypeName)

	tRootMovieClipRegistry[rootMovieClip] = sMovieTypeName
end

function DynamicGameStateData.UnregisterRootMovieClip(rootMovieClip)

	tRootMovieClipRegistry[rootMovieClip] = nil
end

function DynamicGameStateData.SetValue(_, _, _)

	error 'Placeholder was not replaced.'
end
return DynamicGameStateData
