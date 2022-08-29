-- Extension methods for Table classes
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.



local TableExtensions = class_static 'TableExtensions'

--- <summary>
--- Returns whether the given table is empty. Also returns true if the table is null.
--- </summary>
function TableExtensions.IsNullOrEmpty(table0)

	if nil == table0 then

		return true
	end

	return nil == next(table0)
end

--- <summary>
--- Get a value from the table, returning <paramref name="def"/> if the value is null/not found
--- </summary>
--- <param name="table">Table on which to perform the lookup.</param>
--- <param name="key">Name of item to lookup in the table.</param>
--- <param name="def">The value to return if <paramref name="key"/> is null/not found</param>
function TableExtensions.Get(T, table0, key, def)

	local val = table0[key]
	if val == nil then

		return def
	end

	return val
end

--- <summary>
--- Get a value from the table, returning <paramref name="default"/> if the value is null/not found
--- </summary>
--- <param name="default">The value to return if <paramref name="key"/> is null/not found</param>
function TableExtensions.Get3_5B109F3D(K, V, table0, key, default)


	local val = table0[key]
	if val == nil then

		return default
	end

	return val
end

--- <summary>
--- Get a value from the table, returning <paramref name="default"/> if the value is not found
--- </summary>
--- <param name="default">The value to return if <paramref name="key"/> is not found</param>
function TableExtensions.Get3_3E1B9F62(K, V, table0, key, default)


	local val = table0[key]
	if val == nil then

		return default
	end

	return val
end
return TableExtensions
