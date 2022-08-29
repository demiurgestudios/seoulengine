--[[
	RunContentChecks.lua
	Checks various types of SeoulEngine content in the App,
	including cooked UI files and .json data files.

	Copyright (c) Demiurge Studios, Inc.
	
	This source code is licensed under the MIT license.
	Full license details can be found in the LICENSE file
	in the root directory of this source tree.
--]]

-- Manually initialize some SlimCS managed dependencies.
require 'SlimCSMain'
require 'Engine/Native/ScriptEnginePath'
require 'Engine/Core/Globals'
require 'Engine/Core/Native'
require 'Engine/Core/Path'
require 'Engine/Core/Utilities'
require 'Engine/Native/ScriptEngine'
require 'Engine/Native/ScriptEngineCore'
require 'Engine/Engine/Engine'
require 'Engine/Engine/FileManager'
require 'Engine/Native/ScriptEngineInputManager'
require 'Engine/Engine/InputManager'
require 'Engine/Engine/BigNumberFormatter'
require 'Engine/Native/ScriptEngineLocManager'
require 'Engine/Engine/LocManager'
require 'Engine/Native/ScriptEngineFileManager'
require 'Engine/Native/ScriptEngineSettingsManager'
require 'Engine/Engine/SettingsManager'
require 'Engine/Native/ScriptUIManager'
require 'Engine/UI/UIManager'
FinishClassTableInit(CoreNative)
FinishClassTableInit(CoreUtilities)
FinishClassTableInit(Engine)
FinishClassTableInit(FileManager)
FinishClassTableInit(Globals)
FinishClassTableInit(InputManager)
FinishClassTableInit(BigNumberFormatter)
FinishClassTableInit(LocManager)
FinishClassTableInit(Path)
FinishClassTableInit(SettingsManager)
FinishClassTableInit(UIManager)
CoreNative.cctor()
Engine.cctor()
FileManager.cctor()
Globals.cctor()
InputManager.cctor()
LocManager.cctor()
Path.cctor()
SettingsManager.cctor()
UIManager.cctor()
CoreUtilities.OnGlobalMainThreadInit()

-- Run JSON file checks - just requiring the file runs the check.
local bReturn = require 'DevOnly/ContentChecks/CheckJson'

-- Run Loc token checks.
bReturn = LocManager.ValidateTokens() and bReturn

-- Run FLA (Adobe Animate) and FCN (Falcon, cooked SWF) file checks
bReturn = require 'DevOnly/ContentChecks/ValidateUiFiles' and bReturn

-- Run specific checks for A/B test file configs.
bReturn = require 'DevOnly/ContentChecks/CheckABTestConfigs' and bReturn

-- Run specific checks for A/B test file remaps.
bReturn = require 'DevOnly/ContentChecks/CheckABTestRemaps' and bReturn

-- Report error
if not bReturn then
	error('One or more errors, see log.')
end
