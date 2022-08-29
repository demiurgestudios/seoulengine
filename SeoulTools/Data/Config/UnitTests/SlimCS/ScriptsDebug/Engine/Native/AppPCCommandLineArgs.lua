--[[
	AppPCCommandLineArgs.lua
	AUTO GENERATED - DO NOT MODIFY
	API FOR NATIVE CLASS INSTANCE

	Run GenerateScriptBindings.bat in the Utilities folder to re-generate bindings.

	Copyright (c) Demiurge Studios, Inc.
	
	This source code is licensed under the MIT license.
	Full license details can be found in the LICENSE file
	in the root directory of this source tree.
]]




local AppPCCommandLineArgs = class('AppPCCommandLineArgs', nil, 'Native.AppPCCommandLineArgs')

interface('IStatic', 'Native.AppPCCommandLineArgs+IStatic')


















local s_udStaticApi = nil

function AppPCCommandLineArgs.cctor()

	s_udStaticApi = CoreUtilities.DescribeNativeUserData 'AppPCCommandLineArgs'
end

function AppPCCommandLineArgs.AcceptGDPR()

	return s_udStaticApi:AcceptGDPR()
end

function AppPCCommandLineArgs.D3D11Headless()

	return s_udStaticApi:D3D11Headless()
end

function AppPCCommandLineArgs.EnableWarnings()

	return s_udStaticApi:EnableWarnings()
end

function AppPCCommandLineArgs.FMODHeadless()

	return s_udStaticApi:FMODHeadless()
end

function AppPCCommandLineArgs.FreeDiskAccess()

	return s_udStaticApi:FreeDiskAccess()
end

function AppPCCommandLineArgs.RunScript()

	return s_udStaticApi:RunScript()
end

function AppPCCommandLineArgs.DownloadablePackageFileSystemsEnabled()

	return s_udStaticApi:DownloadablePackageFileSystemsEnabled()
end

function AppPCCommandLineArgs.PersistentTest()

	return s_udStaticApi:PersistentTest()
end

function AppPCCommandLineArgs.RunUnitTests()

	return s_udStaticApi:RunUnitTests()
end

function AppPCCommandLineArgs.RunAutomatedTest()

	return s_udStaticApi:RunAutomatedTest()
end

function AppPCCommandLineArgs.GenerateScriptBindings()

	return s_udStaticApi:GenerateScriptBindings()
end

function AppPCCommandLineArgs.TrainerFile()

	return s_udStaticApi:TrainerFile()
end

function AppPCCommandLineArgs.UIManager()

	return s_udStaticApi:UIManager()
end

function AppPCCommandLineArgs.VerboseMemoryTooling()

	return s_udStaticApi:VerboseMemoryTooling()
end

function AppPCCommandLineArgs.VideoDir()

	return s_udStaticApi:VideoDir()
end
return AppPCCommandLineArgs


