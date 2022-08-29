--[[
	Game_Patcher.lua
	AUTO GENERATED - DO NOT MODIFY
	API FOR NATIVE CLASS INSTANCE

	Run GenerateScriptBindings.bat in the Utilities folder to re-generate bindings.

	Copyright (c) Demiurge Studios, Inc.
	
	This source code is licensed under the MIT license.
	Full license details can be found in the LICENSE file
	in the root directory of this source tree.
]]



local Game_Patcher = class('Game_Patcher', 'UI_Movie', 'Native.Game_Patcher')

interface('IStatic000', 'Native.Game_Patcher+IStatic', 'IStatic')




local s_udStaticApi = nil

function Game_Patcher.cctor()

	s_udStaticApi = CoreUtilities.DescribeNativeUserData 'Game_Patcher'
end



function Game_Patcher.StayOnLoadingScreen()

	return s_udStaticApi:StayOnLoadingScreen()
end
return Game_Patcher

