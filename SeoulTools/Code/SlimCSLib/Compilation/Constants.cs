// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

namespace SlimCSLib.Compilation
{
	public static class Constants
	{
		public const string ksFileLibName = "SlimCSFiles.lua";
		public const string ksMainLibNameLua = "SlimCSMain.lua";
		public const string ksMainLibNameCS = "SlimCSMain.cs";

		public const string ksMainLibBody =
@"--[[
	SlimCSMain.lua
	Glue required by SlimCS. Be careful with modifications - this
	file defines much of the backend support functionality needed
	by Lua. As a result, using certain concepts in this file
	(e.g. a cast) will generate infinite recursion or circular
	dependencies.

	Copyright (c) Demiurge Studios, Inc.
	
	This source code is licensed under the MIT license.
	Full license details can be found in the LICENSE file
	in the root directory of this source tree.
]]













local function InvokeConstructor(tClass, sCtorName, oInstance, ...)

	if nil == tClass then

		return
	end

	local ctor = tClass[sCtorName]
	if nil ~= ctor then

		ctor(oInstance, ...)
	end
end

-- The default __tostring just returns the fully qualified name.
local function DefaultToString(tClass)

	return tClass.m_sFullClassName
end

-- Clone all members (except 'cctor'
-- (static constructor), and '__cinit' (our internal
-- 'class initializer').
local tDoNotCopy =
	{
		cctor = true,
		__cinit = true
	}

_G.Activator = {}
_G.Activator.CreateInstance = function(type0)

	return type0.m_Class:New(nil)
end

_G.FinishClassTableInit = function(t)

	-- Check if the class has already been finished.
	if nil ~= rawget(t, '^table-setup') then

		return
	end

	-- Done with table setup, one way or another.
	rawset(t, '^table-setup', true)

	-- Run the (hacky, would like to remove) class 'initalizer'
	-- if one is defined.
	local cinit = rawget(t, '__cinit')
	if nil ~= cinit then

		cinit(t)
	end

	local tSuper = t.m_tSuper
	if nil ~= tSuper then

		_G.FinishClassTableInit(tSuper)

		-- Merge super members.
		for k, v in pairs(tSuper) do

			if true ~= tDoNotCopy[k] and nil == rawget(t, k) then

				rawset(t, k, v)
			end
		end

		local tIs = t.m_tIs
		for k, _ in pairs(tSuper.m_tIs) do

			rawset(tIs, k, true)
		end
	end

	-- After propagation has completed,
	-- check for __init and __tostring.
	-- If not explicitly defined, give
	-- defaults.

	-- Default __tostring just returns the name of the class.
	if nil == rawget(t, '__tostring') then

		rawset(t, '__tostring', DefaultToString)
	end

	-- Default __index handler is just the class table itself.
	local index = rawget(t, '__index')
	if nil == index or index == tSuper then

		rawset(t, '__index', t)
	end
end

local function OverrideNew(tClass, sCtorName, ...)

	local oInstance = {}
	setmetatable(oInstance, tClass)
	InvokeConstructor(tClass, sCtorName, oInstance, ...)
	return oInstance
end
local function DefaultNew(tClass, ...)

	return OverrideNew(tClass, 'Constructor', ...)
end

_G.class = function(sName, sSuper, sFullyQualifiedName, sShortName, ...)

	local tClass = _G[sName]
	if nil == tClass then

		tClass = {}
		_G[sName] = tClass
	end

	-- This is partial class support. If
	-- tClass already has a valid m_sClassName
	-- member, return. We've already
	-- setup the class table.
	if nil ~= rawget(tClass, 'm_sClassName') then

		return tClass
	end

	local tSuper = nil
	if nil == sSuper then

		if tClass ~= _G.object then

			tSuper = _G.object
		end

	else

		-- Handling for generics - the class table will have been passed in.
		if type(sSuper) == 'table' then

			tSuper = sSuper

		else

			tSuper = _G[sSuper]
			if nil == tSuper then

				tSuper = {}
				_G[sSuper] = tSuper
			end
		end
	end

	-- Sanitize.
	sFullyQualifiedName = sFullyQualifiedName or sName
	sShortName = sShortName or sName

	-- Set the class, full name, and super fields.
	tClass.m_sFullClassName = sFullyQualifiedName
	tClass.m_sShortClassName = sShortName
	tClass.m_sClassName = sName
	tClass.m_tSuper = tSuper

	-- Build a lookup table for 'as' and 'is'
	-- queries. Interfaces have string keys,
	-- parent classes use the parent table as the key.
	local tIs = {}
	local args = {...}
	for _, v in pairs(args) do

		tIs[v] = true
	end
	tIs[tClass] = true

	tClass.m_tIs = tIs

	-- Setup handlers.
	tClass.New = DefaultNew
	tClass.ONew = OverrideNew

	return tClass
end

-- TODO: A static class may need to be more than just a table?
_G.class_static = function(sName)

	local tClass = _G[sName]
	if nil == tClass then

		tClass = {}
		_G[sName] = tClass
	end

	return tClass
end

_G.interface = function(sName, sFullyQualifiedName, sShortName, ...)

	local tInterface = _G[sName]
	if nil == tInterface then

		tInterface = {}
		_G[sName] = tInterface
	end

	-- Sanitize.
	sFullyQualifiedName = sFullyQualifiedName or sName
	sShortName = sShortName or sName

	-- Set the class, full name, and super fields.
	tInterface.m_sFullClassName = sFullyQualifiedName
	tInterface.m_sShortClassName = sShortName
	tInterface.m_sClassName = sName

	-- Build a lookup table for 'as' and 'is'
	-- queries. Interfaces have string keys,
	-- parent classes use the parent table as the key.
	local tIs = {}
	local tmp = {...}
	local args = tmp
	for _, v in pairs(args) do

		tIs[v] = true
	end
	tIs[tInterface] = true
	tIs[sName] = true

	tInterface.m_tIs = tIs

	return tInterface
end

-- builtin string library extensions
string.align = function(s, i)

	s = tostring(s)
	local l = string.len(s)
	local pad = math.abs(i) - l
	if pad <= 0 then

		return s
	end

	if i < 0 then

		return tostring(s) .. tostring(string.rep(' ', pad))

	elseif i > 0 then

		return tostring(string.rep(' ', pad)) .. tostring(s)

	else

		return s
	end
end

string.find_last = function(s, pattern, index, plain)

	local find = function(s0, pattern0, init, plain0) if plain0 == nil then plain0 = false end return string.find(s0, pattern0, init, plain0) end
	local i, j = nil, nil
	local lastI, lastJ = nil, nil
	repeat

		lastI, lastJ = i, j
		i, j = find(s, pattern, index, plain)
		if nil ~= j then

			index = j + 1
		end
	until not(nil ~= i)

	if nil == lastI then

		return nil, nil

	else

		return lastI, lastJ
	end
end

-- TODO: Possibly temp.
_G.Table = {} -- TODO: Filter this out at compile time?
local function final_class(sName, sSuper, sFullyQualifiedName, sShortName, ...)

	local t = _G.class(sName, sSuper, sFullyQualifiedName, sShortName, ...)
	_G.FinishClassTableInit(t)
	return t
end

final_class 'object'
_G.Object = _G.object -- TODO: Alias is the way to go?
_G.Object.m_sFullClassName = 'System.Object'
final_class 'char'
final_class('bool', nil, nil, 'Boolean')
_G.bool.__default_value = false
_G.bool.__arr_read_placeholder = false
_G.Boolean = _G.bool -- TODO: Alias is the way to go?
final_class('float', null, null, 'Single')
_G['float']['__default_value'] = 0
_G['Single'] = _G['float'] -- TODO: Alias is the way to go?
final_class('double', nil, nil, 'Double')
_G.double.__default_value = 0
_G.Double = _G.double -- TODO: Alias is the way to go?
final_class('sbyte', null, null, 'SByte')
_G['sbyte']['__default_value'] = 0
final_class('short', null, null, 'Int16')
_G['short']['__default_value'] = 0
final_class('int', null, null, 'Int32')
_G['int']['__default_value'] = 0
final_class('long', null, null, 'Int64')
_G['long']['__default_value'] = 0
final_class('byte', null, null, 'Byte')
_G['byte']['__default_value'] = 0
final_class('ushort', null, null, 'UShort')
_G['ushort']['__default_value'] = 0
final_class('uint', null, null, 'UInt32')
_G['uint']['__default_value'] = 0
final_class('ulong', null, null, 'UInt64')
_G['ulong']['__default_value'] = 0
final_class('String', nil, 'string')
_G.String.m_sClassName = 'string' -- TODO:
final_class 'Action' -- TODO: Don't handle these specially?
final_class('NullableBoolean', nil, nil, 'Nullable`1') -- TODO: Don't handle these specially?
final_class('NullableDouble', nil, nil, 'Nullable`1') -- TODO: Don't handle these specially?
final_class('NullableInt32', nil, nil, 'Nullable`1') -- TODO: Don't handle these specially?
final_class 'Type' -- TODO: Don't handle these specially?
_G.Type.__tostring = function(t)

	return t.m_Class.m_sFullClassName
end

_G.Char = _G.char -- TODO: Alias is the way to go?
_G.Int32 = _G.int -- TODO: Alias is the way to go?

-- TODO: This approache loses specific type checking
-- on delegate casts.
final_class 'Delegate'

final_class 'Attribute'
_G.Attribute.m_sClassName = 'System.Attribute' -- TODO:

final_class 'Exception'
_G.Exception.m_sClassName = 'System.Exception' -- TODO:
_G.Exception.Constructor = function(self, msg)

	msg = msg or ""Exception of type '"" .. tostring(self.m_sClassName) .. ""' was thrown.""
	self.Message = msg
end
_G.Exception.get_Message = function(self)

	return self.Message
end
_G.Exception.__tostring = function(self)

	return
		tostring(self.m_sClassName) ..
		': ' ..tostring(
		self.Message)
end
final_class('ArgumentException', 'Exception')
_G.ArgumentException.Constructor = function(self, msg)

	self.Message = msg
end

final_class('NullReferenceException', 'Exception')
_G.NullReferenceException.Constructor = function(self, msg)

	msg = msg or 'Object reference not set to an instance of an object.'
	self.Message = msg
end

local tWrappers =
	{
		['nil'] = function(err)

			return err
		end,
		string = function(err)

			return _G.Exception:New(err)
		end,
		table = function(err)

			local cls = getmetatable(err)
			if nil == cls or nil == cls.get_Message then

				return _G.Exception:New(tostring(err))

			else

				return err
			end
		end
	}

local function wrapError(err)

	local sType = type(err)
	local f = tWrappers[sType]
	return f(err)
end

_G.try = function(tryblock, ...)

	-- TODO: This breaks for userland functions that
	-- return multiple return values. But I'd rather not
	-- add the overhead for an uncommon case, so we'll
	-- need another pair (tryfinallymultiple and trymultiple)
	local bSuccess, res, ret = pcall(tryblock)
	if not bSuccess then

		res = wrapError(res)

		local tmp = {...}
		local args = tmp
		for i = 1, #args, 2 do

			local test = args[i]

			-- Invoke the test to see if this catch should be invoked.
			if test(res) then

				local catch = args[i + 1]
				local bNewSuccess, newRes, newRet = pcall(catch, res)

				bSuccess = bNewSuccess
				if not bSuccess then

					newRes = wrapError(newRes)
					if nil ~= newRes then

						res = newRes
					end

				else

					res = newRes
				end
				ret = newRet

				-- Done.
				break
			end
		end
	end

	if not bSuccess then

		error(res)
	end

	return res, ret
end

_G.tryfinally = function(tryblock, ...)

	local res, ret = _G.try(tryblock, ...)

	local tmp = {...}
	local args = tmp
	local finallyFunc = args[#args]
	finallyFunc()

	return res, ret
end

_G.using = function(...)

	local tmp = {...}
	local args = tmp
	local iArgs = #args
	local block = args[iArgs]
	local bSuccess, res, ret = pcall(block, ...)

	for i = iArgs - 1, 1, -1 do

		if args[i] ~= nil then

			args[i]:Dispose()
		end
	end

	if not bSuccess then

		error(wrapError(res))
	end

	return res, ret
end

local tLookups =
	{
		boolean = function(_, b)

			return b == _G.bool or b == _G.object or b == _G.NullableBoolean
		end,
		['function'] = function(_, b)

			return b == _G.Delegate or b == _G.object
		end,
		['nil'] = function(_, _)

			return false
		end,
		number = function(_, b)

			return b == _G.double or b == _G.int or b == _G.object or b == _G.NullableDouble or b == _G.NullableInt32
		end, -- TODO: Temp, allow int or double.
		string = function(_, b)

			return b == _G.String or b == _G.object
		end,
		table = function(a, b)

			return b == _G.Table or true == (a and (a.m_tIs and a.m_tIs[b])) or false
		end,
		thread = function(_, _)

			return false
		end,
		userdata = function(_, _)

			return false
		end -- TODO: should actually evaluate, not just return false.
	}
local tTypeLookup =
	{
		boolean = function(_)

			return _G.bool
		end,
		['function'] = function(_)

			return _G.Delegate
		end,
		number = function(_)

			return _G.double
		end,
		string = function(_)

			return _G.String
		end,
		table = function(a)

			return getmetatable(a)
		end
	}

local s_tTypes = {}
_G.TypeOf = function(o)

	local ret = s_tTypes[o]
	if nil == ret then

		ret = {}

		-- Fill out our minimal System.Reflection.Type table.
		local dyn = o
		ret.m_Class = dyn
		ret.get_Name = function(t)

			return t.m_Class.m_sShortClassName
		end

		-- ToString() support.
		setmetatable(ret, _G.Type)

		s_tTypes[o] = ret
	end
	return ret
end

_G.GetType = function(o)

	return _G.TypeOf(tTypeLookup[type(o)](o))
end

_G.ToString = function(o)

	return tostring(o)
end

_G.__bind_delegate__ = function(self, func)

	local ret = self[func]
	if nil == ret then

		ret = function(...)

			return func(self, ...)
		end
		self[func] = ret
	end
	return ret
end

_G.arraycreate = function(size, val)

	-- Apply our filtering behavior - we use
	-- false as a placeholder for nil.
	if val == nil then

		val = false
	end

	-- TODO: Do this in native with
	-- a lua extension. Note that this
	-- is quite a bit faster than table.insert().
	local a = {}
	for i = 1, size do

		a[i] = val
	end

	return a
end

_G.genericlookup = function(sType, sId, ...)

	local genericType = _G[sId]
	if nil == genericType then

		genericType = _G.class(sId, sType, sId, sId)

		local tmp = {...}
		local args = tmp
		for i = 1, #args, 2 do

			rawset(genericType, args[i], args[i + 1])
		end

		-- Finalize.
		if nil ~= rawget(_G[sType], '^table-setup') then

			_G.FinishClassTableInit(genericType)
			if nil ~= genericType.cctor then

				genericType.cctor()
			end
		end

		_G[sId] = genericType
	end
	return genericType
end

_G.initarr = function(inst, ...)

	local tmp = {...}
	local args = tmp
	local i = 1
	local add_ = inst.Add
	while args[i] ~= nil do

		local v = args[i]
		add_(inst, v)
		i = i + 1
	end

	return inst
end

_G.initlist = function(inst, ...)

	local tmp = {...}
	local args = tmp
	local i = 1
	while args[i] ~= nil do

		local bProp = args[i]
		local k = args[i + 1]
		local v = args[i + 2]

		if bProp then

			local f = inst[k]
			f(inst, v)

		else

			rawset(inst, k, v)
		end

		i = (i) +(3)
	end

	return inst
end

_G.is = function(a, b)

	local f = tLookups[type(a)]
	return f(a, b)
end

_G.as = function(a, b)

	local f = tLookups[type(a)]
	if f(a, b) then

		return a

	else

		return nil
	end
end

_G.asnum = function(a)

	return a or 0
end

_G.__cast__ = function(a, b)

	local ret = _G.as(a, b)
	if ret == nil and a ~= nil then

		local classA = tTypeLookup[type(a)](a)
		local classB = b
		error(
			_G.Exception:New(
			""Cannot convert type '"" .. tostring(classA.m_sFullClassName) .. ""' to '"" .. tostring(classB.m_sFullClassName) .. ""'""),
			1)
	end
	return ret
end

local __i32truncate__ = _G.math.i32truncate
_G.__castint__ = function(a)

	local typeA = type(a)
	if typeA == 'number' then

		return __i32truncate__(a)

	else

		if nil == a then

			error(_G.Exception:New('Object reference not set to an instance of an object.'), 1)

		else

			local classA = tTypeLookup[typeA](a)
			error(_G.Exception:New(""Cannot convert type '"" .. tostring(classA.m_sFullClassName) .. ""' to 'int'""), 1)
		end
		return 0
	end
end

_G.__bband__ = function(a, b)

	return a and b
end
_G.__bbor__ = function(a, b)

	return a or b
end
_G.__bbxor__ = function(a, b)

	return a ~= b
end
_G.__i32mod__ = _G.math.i32mod
_G.__i32mul__ = _G.math.i32mul
_G.__i32narrow__ = _G.bit.tobit
_G.__i32truncate__ = __i32truncate__







































































interface 'IDisposable'



";
	}
}
