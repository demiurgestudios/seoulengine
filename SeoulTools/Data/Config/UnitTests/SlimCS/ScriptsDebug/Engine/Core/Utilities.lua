-- Miscellaneous core engine utilities.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.





local CoreUtilities = class_static 'CoreUtilities'











--- <summary>
--- Debug only - raise an exception if b is not true.
--- </summary>
--- <param name="b">Condition to check.</param>
--- <param name="msg">Message to include with the raised exception.</param>
function CoreUtilities.Assert(
	b, msg)

	assert(b, msg)
end

--- <summary>
--- Returns a new table with the given capacities.
--- </summary>
--- <param name="iArrayCapacity">Size of the array used to store values in the table.</param>
--- <param name="iRecordCapacity">Size of the array used to store keys in the table.</param>
--- <returns>A new Table with the given capacities.</returns>
--- <remarks>
--- A Table used like an array should pass null for the record capacity. The Table implementation
--- only needs a record array if it is sparse or uses keys that are not numbers.
--- </remarks>
function CoreUtilities.CreateTable(iArrayCapacity, iRecordCapacity)

	return _G.SeoulCreateTable(iArrayCapacity, iRecordCapacity)
end

-- Wrap the native SeoulDescribeNativeEnum.
function CoreUtilities.DescribeNativeEnum(sName)

	return _G.SeoulDescribeNativeEnum(sName)
end

-- Wrap the native SeoulDescribeNativeUserData.
function CoreUtilities.DescribeNativeUserData(sName)

	return _G.SeoulDescribeNativeUserData(sName)
end

-- Wrap the native SeoulNativeNewNativeUserData with a wrapped version that
-- automagically invokes Construct(), if the native type exposes it.
function CoreUtilities.NewNativeUserDataByName(sName, ...)

	-- Instantiate the type.
	local ud = _G.SeoulNativeNewNativeUserData(sName)

	-- If constructed and has a Construct() method,
	-- invoke it.
	if nil ~= ud then

		local construct = ud.Construct
		if nil ~= construct then

			construct(ud, ...)
		end
	end

	return ud
end

function CoreUtilities.NewNativeUserData(T, ...)

	-- Instantiate the type.
	local ud = _G.SeoulNativeNewNativeUserData(TypeOf(T):get_Name())

	-- If constructed and has a Construct() method,
	-- invoke it.
	if nil ~= ud then

		local construct = ud.Construct
		if nil ~= construct then

			construct(ud, ...)
		end
	end

	return ud
end

-- If value is nil, returns default; otherwise returns value
function CoreUtilities.WithDefault(T, value, def)

	if value == nil then

		return def
	end

	return __cast__(value, T)
end

-- Iterate over the elements in the table and return a count of them
function CoreUtilities.CountTableElements(tValue)

	local iCount = 0
	for _, _ in pairs(tValue) do

		iCount = iCount + 1
	end
	return iCount
end

--- <summary>
--- Log an object to the given logger channel.
--- </summary>
--- <param name="sChannel">Channel to log to.</param>
--- <param name="value">object to log.</param>
--- <param name="fOptionalMaxDepth">Optional argument to limit how much of the tree is printed.</param>
function CoreUtilities.LogTableOrTostring(
	sChannel, value, fOptionalMaxDepth)

	if type(value) == 'table' then

		CoreUtilities.LogTable(sChannel, value, fOptionalMaxDepth)

	else

		CoreNative.LogChannel(sChannel, tostring(value))
	end
end

--- <summary>
--- Searches through a table and returns the index of the string found... else -1;
--- </summary>
--- <typeparam name="T"></typeparam>
--- <param name="table"></param>
--- <param name="toFind"></param>
--- <returns></returns>
function CoreUtilities.FindStringInTable(table0, toFind)

	for index, value in ipairs(table0) do

		if value == toFind then

			return index
		end
	end

	return -1
end

--- <summary>
--- Log a table to the given logger channel.
--- </summary>
--- <param name="sChannel">Channel to log to.</param>
--- <param name="value">Table to log.</param>
--- <param name="fOptionalMaxDepth">Optional argument to limit how much of the tree is printed.</param>
function CoreUtilities.LogTable(
	sChannel, value, fOptionalMaxDepth)

	local tCache = {}

	local function nestedPrint(t, indent, remainingDepth)

		if remainingDepth == 0 then

			return
		end

		if nil ~= tCache[tostring(t)] then

			CoreNative.LogChannel(sChannel, tostring(indent) .. 'ref ' .. tostring(t))

		else

			tCache[tostring(t)] = true
			CoreNative.LogChannel(sChannel, tostring(indent) .. '{')
			indent = tostring(indent) .. '  '
			for k, v in pairs(t) do

				if 'table' == type(v) then

					CoreNative.LogChannel(sChannel, tostring(indent) .. tostring(k) .. ' =')
					nestedPrint(v, indent, remainingDepth - 1)

				else

					CoreNative.LogChannel(sChannel, tostring(indent) .. tostring(k) .. ' = ' .. tostring(v))
				end
			end
			CoreNative.LogChannel(sChannel, tostring(indent) .. '}')
		end
	end

	if fOptionalMaxDepth == nil then

		fOptionalMaxDepth = -1
	end

	if 'table' == type(value) then

		nestedPrint(value, ''--[[String.Empty]], fOptionalMaxDepth)

	else

		CoreNative.LogChannel(sChannel, tostring(value))
	end
end

CoreUtilities.kWeakMetatable = 
{
	__mode = 'kv' -- weak table
}

function CoreUtilities.MakeWeakTable()

	local ret = {}
	setmetatable(ret, CoreUtilities.kWeakMetatable)
	return ret
end

-- Returns a random number drawn from a gaussian (aka normal) distribution with the given mean and standard deviation.
-- There is the optional ability to limit the max number of standard devitions away from the mean. Using this param
-- results in the returned values no longer being from a truely a normal distrubution (for example this will change the
-- standard deviation of the returned values) but the shape of the clipped curve will be normal between the max values.
-- This function is relatively slow due to the use of math.exp, math.sqrt, math.log, math.cos, and two calls to math.random.
function CoreUtilities.SlowRandomGaussian(fMean, fStandardDeviation, fOptionalMaxStandardDeviation)

	-- When we clip the gaussian with the fOptionalMaxStandardDeviation param we need to figure out
	-- what to do with the 'extra' area outside of that given range. Below we explore two
	-- ways to do this.

	-- Method 1  - Fill in extra area evenly
	-- In this method, if a value is generated outside the given fOptionalMaxStandardDeviation
	-- range, we just pick a new uniformly distributed random number and return that. This
	-- essentially means that we're lifting the whole gaussian curve 'up' to account for
	-- the 'extra' area we clipped off the edges.
	--
	-- Calculate a normal with mean 0 and std dev 1
	local p = math.sqrt(-2 * math.log(1 - math.random()))
	local rand = p * math.cos(6.28318530717959--[[2 * math.pi]] * math.random())

	-- If it's outside the desired range (if that is specified and non-zero), pick a random
	-- (uniformly distributed value) value in the desired range.
	if nil ~= fOptionalMaxStandardDeviation then

		local d = asnum(fOptionalMaxStandardDeviation)
		if rand > d or rand < -d then

			rand = 2 * (math.random() - 0.5) * d
		end
	end

	-- Method 2 - Edges approach probability of 0
	-- This method keeps the shape of the curve between +/- fOptionalMaxStandardDeviation by scaling
	-- up that curve segment and filling in the 'extra' area mainly in the middle of the graph.
	-- The edges apporach a probabilty of 0 even if fOptionalMaxStandardDeviation is something small like
	-- 0.5. For current applications this method is not desired, but there might be some future
	-- use for it (and the math was tricky to figure out) so the code is left in as an example.
	--
	---- Optionally limit the range of the returned values
	--local fMaxStdDevFactor = 1
	--if fOptionalMaxStandardDeviation then
	--	fMaxStdDevFactor = 1 - math.exp(fOptionalMaxStandardDeviation * fOptionalMaxStandardDeviation / -2)
	--end
	---- Calculate a normal with mean 0 and std dev 1
	--local p = math.sqrt(-2 * math.log(1 - fMaxStdDevFactor * math.random()))
	--local rand = p * math.cos(2 * math.pi * math.random())

	-- Return the value with the requested mean and std dev.
	return asnum(fMean + rand * fStandardDeviation)
end

local InternalShallowCopyTable;function CoreUtilities.ShallowCopyTable(tTable)

	return InternalShallowCopyTable(tTable)
end

function CoreUtilities.ShallowCopyTable1_E365A2FF(K, V, tTable)


	return InternalShallowCopyTable(tTable)
end

function CoreUtilities.ShallowCopyTable1_D14D2F67(K, V, tTable)


	return InternalShallowCopyTable(tTable)
end

function InternalShallowCopyTable(tTable)

	if tTable == nil then

		return nil
	end

	local tType = type(tTable)
	if tType ~= 'table' then

		error("Can't shallow copy non-table (" .. tostring(tType) .. ')')
		return nil
	end

	local tCopy = {}
	for key, value in pairs(tTable) do

		tCopy[key] = value
	end

	return tCopy
end

-- Creates a new table with the elements of the original,
-- and adds any elements from the parent not in the original
function CoreUtilities.ShallowCombineTable(tOriginal, tParent)

	local tOriginalType = type(tOriginal)
	if tOriginalType ~= 'table' then

		error("Can't shallow combine non-table original (" .. tostring(tOriginalType) .. ')')
		return nil
	end

	local tRet = CoreUtilities.ShallowCopyTable(tOriginal)

	if nil ~= tParent then

		local tParentType = type(tParent)
		if tParentType ~= 'table' then

			error("Can't shallow combine non-table parent (" .. tostring(tParentType) .. ')')
			return nil
		end

		-- Add any values we didn't override from the parent
		for key, value in pairs(tParent) do

			if nil == tRet[key] then

				tRet[key] = value
			end
		end
	end

	return tRet
end

--- <summary>
--- Variation of ShallowCombineTable that merges tParent directly into tOriginal.
--- </summary>
--- <param name="tOriginal">The target table to merge into.</param>
--- <param name="tParent">The source table. Values are merged if not already defined in tOriginal.</param>
function CoreUtilities.ShallowMergeTable(tOriginal, tParent)

	for key, value in pairs(tParent) do

		if nil == tOriginal[key] then

			tOriginal[key] = value
		end
	end
end

--- <summary>
--- Pass to VisitTables to traverse. Return false to stop traversal.
--- </summary>


--- <summary>
--- Utility function, visits all table objects globally accessible
--- (via global objects and closures).
--- </summary>
local InternalVisitHelper;function CoreUtilities.VisitTables(func)

	local tVisitedTable = {}
	local aPath = {}

	InternalVisitHelper(func, _G, tVisitedTable, aPath)
	InternalVisitHelper(func, debug.getregistry(), tVisitedTable, aPath)
end

function InternalVisitHelper(
	func,
	objToSearch,
	tVisitedTable,
	aPath)

	if nil == objToSearch then

		return
	end

	local sType = type(objToSearch)

	-- Do the visit here if it is a table
	if 'table' == sType and not func(objToSearch, aPath) then

		return
	end

	if (tVisitedTable[objToSearch] == nil and { false } or { tVisitedTable[objToSearch] })[1] then

		return
	end
	tVisitedTable[objToSearch] = true

	if 'table' == sType then

		local tableToSearch = objToSearch
		for key, val in pairs(tableToSearch) do

			table.insert(aPath, '<key>')
			InternalVisitHelper(func, key, tVisitedTable, aPath)
			table.remove(aPath)

			table.insert(aPath, ToString(key))
			InternalVisitHelper(func, val, tVisitedTable, aPath)
			table.remove(aPath)
		end

		table.insert(aPath, '<metatable>')
		InternalVisitHelper(func, getmetatable(objToSearch), tVisitedTable, aPath)
		table.remove(aPath)

	elseif 'function' == sType then

		table.insert(aPath, '<fenv>')
		InternalVisitHelper(func, debug.getuservalue(objToSearch), tVisitedTable, aPath)
		table.remove(aPath)

		-- 60 is maximum up-value limit for luajit.
		for i = 1, 60 do

			local n, v = debug.getupvalue(objToSearch, i)
			if nil == n then

				break
			end

			table.insert(aPath, '<upvalue>:' ..tostring(n))
			InternalVisitHelper(func, v, tVisitedTable, aPath)
			table.remove(aPath)
		end

	elseif 'userdata' == sType then

		table.insert(aPath, '<metatable>')
		InternalVisitHelper(func, getmetatable(objToSearch), tVisitedTable, aPath)
		table.remove(aPath)

		table.insert(aPath, '<user-value>')
		InternalVisitHelper(func, debug.getuservalue(objToSearch), tVisitedTable, aPath)
		table.remove(aPath)
	end
end

function CoreUtilities.GenerateTwoWayLookupTables(EnumType)


	local stringToEnumTable = EnumType

	local enumToStringTable = {}
	for s, e in pairs(stringToEnumTable) do

		enumToStringTable[e] = s
	end

	return stringToEnumTable, enumToStringTable
end

function CoreUtilities.Shuffle(array)

	local count = #array

	for i = 1, count - 1 do

		local j = math.random(i, count)
		local temp = array[i]
		array[i] = array[j]
		array[j] = temp
	end
end


-- takes a numerically indexed table and creates the "inverted" lookup table.
-- Note, this assumes you are okay with duplicate entries being removed.
function CoreUtilities.CreateLookUp(T, table0)

	local newTable = {}

	for key, entry in pairs(table0) do

		newTable[entry] = key
	end

	return newTable
end

function CoreUtilities.AppendToTableAsArray(originalTable, newElements)

	local output = {}
	if not TableExtensions.IsNullOrEmpty(originalTable) then

		for _, entry in ipairs(originalTable) do

			table.insert(output, entry)
		end
	end
	if not TableExtensions.IsNullOrEmpty(newElements) then

		for _, entry in ipairs(newElements) do

			table.insert(output, entry)
		end
	end
	return output
end

local RestoreEntry = class('RestoreEntry', nil, 'CoreUtilities+RestoreEntry')





local s_aToRestore = {}

--- <summary>
--- Utility for storing methods that are unsafe outside of the main thread context.
--- </summary>
--- <param name="t">Table of methods to store.</param>
--- <param name="asMethods">List of method names to store.</param>
--- <remarks>
--- Part of our main thread protection. Methods are removed from t and replaced
--- with a function that raises an exception on access. Automatically restores
--- functions once main thread has been entered.
--- </remarks>
function CoreUtilities.MainThreadOnly(t, ...)

	local tRestore = {}
	for _, name in ipairs(({...})) do name = name or nil;

		tRestore[name] = t[name]
		t[name] = function()

			error(tostring(name) .. ' can only be called from the main thread.')
		end
	end

	-- Add an eytr.
	table.insert(
		s_aToRestore,
		initlist(RestoreEntry:New(),

		false, 'm_Target', t,
		false, 'm_ToRestore', tRestore))

end

function CoreUtilities.OnGlobalMainThreadInit()

	for _, entry in ipairs(s_aToRestore) do

		local target = entry.m_Target
		for sName, func in pairs(entry.m_ToRestore) do

			target[sName] = func
		end
	end

	-- Clear.
	s_aToRestore = {}
end

function CoreUtilities.cctor()

	Globals.RegisterGlobalMainThreadInit(CoreUtilities.OnGlobalMainThreadInit)
end

function CoreUtilities.LessThan(a, b)

	--  This is kinda bad. Just a workaround so get c# to stop complaining so we can let LUA handle string compare.
	return a < b
end

function CoreUtilities.FillNumArray(min, max)

	local output = {}
	local index = 1
	for i = min, max do

		table.insert(output, index, i)
		index = index + 1
	end
	return output
end



function CoreUtilities.ApplyTransformToStringArray(array, transform)

	local output = {}

	for _, entry in ipairs(array) do

		table.insert(output, transform(entry))
	end

	return output
end

function CoreUtilities.NumArrayToString(array, locFormatNumber)

	local output = {}
	local count = CoreUtilities.CountTableElements(array)
	for i = 1, count do

		if locFormatNumber then

			table.insert(output, LocManager.FormatNumber(asnum(array[i]), 0))

		else

			table.insert(output, array[i])
		end
	end
	return output
end

--- <summary>
--- Returns a PseudoRandom instance seeded from a secure random source
--- </summary>
function CoreUtilities.NewRandom()

	return CoreUtilities.NewNativeUserData(Script_PseudoRandom)
end

--- <summary>
--- Returns a PseudoRandom instance seeded deterministically, from a hash of the given string
--- </summary>
function CoreUtilities.NewRandomFromSeedString(stringToHash)

	return CoreUtilities.NewNativeUserData(Script_PseudoRandom, stringToHash)
end



function CoreUtilities.OpenAssociatedJsonFile(filePath)

	local sFilePath = string.gsub(filePath:GetAbsoluteFilename(), '\\', '/')
	Engine.OpenURL('file:///' ..tostring(sFilePath))
end
--DEBUG
return CoreUtilities
