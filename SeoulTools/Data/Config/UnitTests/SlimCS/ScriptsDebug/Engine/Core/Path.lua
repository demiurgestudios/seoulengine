-- SeoulEngine path manipulation and sanitizer functions.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.



local Path = class_static 'Path'

local udPath = nil

function Path.cctor()

	udPath = CoreUtilities.NewNativeUserData(ScriptEnginePath)
	if nil == udPath then

		error 'Failed instantiating ScriptEnginePath.'
	end
end

function Path.Combine(...)

	local sRet = udPath:Combine(...)
	return sRet
end

function Path.CombineAndSimplify(...)

	local sRet = udPath:CombineAndSimplify(...)
	return sRet
end

function Path.GetDirectoryName(sPath)

	local sRet = udPath:GetDirectoryName(sPath)
	return sRet
end

function Path.GetExactPathName(sPath)

	local sRet = udPath:GetExactPathName(sPath)
	return sRet
end

function Path.GetExtension(sPath)

	local sRet = udPath:GetExtension(sPath)
	return sRet
end

function Path.GetFileName(sPath)

	local sRet = udPath:GetFileName(sPath)
	return sRet
end

function Path.GetFileNameWithoutExtension(sPath)

	local sRet = udPath:GetFileNameWithoutExtension(sPath)
	return sRet
end

function Path.GetPathWithoutExtension(sPath)

	local sRet = udPath:GetPathWithoutExtension(sPath)
	return sRet
end

function Path.Normalize(sPath)

	local sRet = udPath:Normalize(sPath)
	return sRet
end
return Path
