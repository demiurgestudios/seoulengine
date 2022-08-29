-- Access to the native FileManager singleton.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.



local FileManager = class_static 'FileManager'

local udFileManager = nil

function FileManager.cctor()

	udFileManager = CoreUtilities.NewNativeUserData(ScriptEngineFileManager)
	if nil == udFileManager then

		error 'Failed instantiating ScriptEngineFileManager.'
	end
end

function FileManager.FileExists(filePathOrFileName)

	local bRet = udFileManager:FileExists(filePathOrFileName)
	return bRet
end

function FileManager.GetDirectoryListing(sPath, bRecursive, sFileExtension)

	return udFileManager:GetDirectoryListing(sPath, bRecursive, sFileExtension)
end

function FileManager.GetSourceDir()

	local sRet = udFileManager:GetSourceDir()
	return sRet
end

function FileManager.GetVideosDir()

	local sRet = udFileManager:GetVideosDir()
	return sRet
end
return FileManager
