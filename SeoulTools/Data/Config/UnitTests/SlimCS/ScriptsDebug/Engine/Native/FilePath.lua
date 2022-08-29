--[[
	FilePath.lua
	AUTO GENERATED - DO NOT MODIFY
	API FOR NATIVE CLASS INSTANCE

	Run GenerateScriptBindings.bat in the Utilities folder to re-generate bindings.

	Copyright (c) Demiurge Studios, Inc.
	
	This source code is licensed under the MIT license.
	Full license details can be found in the LICENSE file
	in the root directory of this source tree.
]]





local FilePath = class('FilePath', nil, 'Native.FilePath')

interface('IStatic00', 'Native.FilePath+IStatic', 'IStatic')





local s_udStaticApi = nil

function FilePath.cctor()

	s_udStaticApi = CoreUtilities.DescribeNativeUserData 'FilePath'
end













function FilePath.CreateConfigFilePath(a0)

	return s_udStaticApi:CreateConfigFilePath(a0)
end



function FilePath.CreateContentFilePath(a0)

	return s_udStaticApi:CreateContentFilePath(a0)
end
return FilePath

