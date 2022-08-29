--[[
	EngineCommandLineArgs.lua
	AUTO GENERATED - DO NOT MODIFY
	API FOR NATIVE CLASS INSTANCE

	Run GenerateScriptBindings.bat in the Utilities folder to re-generate bindings.

	Copyright (c) Demiurge Studios, Inc.
	
	This source code is licensed under the MIT license.
	Full license details can be found in the LICENSE file
	in the root directory of this source tree.
]]



local EngineCommandLineArgs = class('EngineCommandLineArgs', nil, 'Native.EngineCommandLineArgs')

interface('IStatic0', 'Native.EngineCommandLineArgs+IStatic', 'IStatic')







local s_udStaticApi = nil

function EngineCommandLineArgs.cctor()

	s_udStaticApi = CoreUtilities.DescribeNativeUserData 'EngineCommandLineArgs'
end

function EngineCommandLineArgs.AutomationScript()

	return s_udStaticApi:AutomationScript()
end

function EngineCommandLineArgs.MoriartyServer()

	return s_udStaticApi:MoriartyServer()
end

function EngineCommandLineArgs.NoCooking()

	return s_udStaticApi:NoCooking()
end

function EngineCommandLineArgs.PreferUsePackageFiles()

	return s_udStaticApi:PreferUsePackageFiles()
end
return EngineCommandLineArgs

