-- Access to the native InputManager singleton.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.



-- Note: this must be kept in sync with InputKeys.h
--- <summary>
--- Input button IDs
--- </summary>
InputButton = {

-- Keyboard keys

-- IMPORTANT: Special keys must come first - this ensures that they
-- are updated first, so any subsequent key input events can
-- check InputManager::IsSpecialPressed() and get correct results.
KeyLeftShift = 0,
KeyRightShift = 1,
KeyLeftControl = 2,
KeyRightControl = 3,
KeyLeftAlt = 4,
KeyRightAlt = 5,
KeyA = 6,
KeyB = 7,
KeyC = 8,
KeyD = 9,
KeyE = 10,
KeyF = 11,
KeyG = 12,
KeyH = 13,
KeyI = 14,
KeyJ = 15,
KeyK = 16,
KeyL = 17,
KeyM = 18,
KeyN = 19,
KeyO = 20,
KeyP = 21,
KeyQ = 22,
KeyR = 23,
KeyS = 24,
KeyT = 25,
KeyU = 26,
KeyV = 27,
KeyW = 28,
KeyX = 29,
KeyY = 30,
KeyZ = 31,
Key0 = 32,
Key1 = 33,
Key2 = 34,
Key3 = 35,
Key4 = 36,
Key5 = 37,
Key6 = 38,
Key7 = 39,
Key8 = 40,
Key9 = 41,
KeySpace = 42,
KeySpaceBar = 42,
KeyOEM_3 = 43,
KeyGrave = 43,
KeyTilde = 43,
KeyBackQuote = 43,
KeyOEM_Minus = 44,
KeyMinus = 44,
KeyUnderscore = 44,
KeyOEM_Plus = 45,
KeyEquals = 45,
KeyPlus = 45,
KeyOEM_4 = 46,
KeyLeftBracket = 46,
KeyLeftBrace = 46,
KeyOEM_6 = 47,
KeyRightBracket = 47,
KeyRightBrace = 47,
KeyOEM_5 = 48,
KeyBackslash = 48,
KeyPipe = 48,
KeyOEM_1 = 49,
KeySemicolon = 49,
KeyColon = 49,
KeyOEM_7 = 50,
KeyApostrophe = 50,
KeyQuote = 50,
KeySingleQuote = 50,
KeyDoubleQuote = 50,
KeyOEM_Comma = 51,
KeyComma = 51,
KeyLessThan = 51,
KeyOEM_Period = 52,
KeyPeriod = 52,
KeyGreaterThan = 52,
KeyOEM_2 = 53,
KeySlash = 53,
KeyForwardSlash = 53,
KeyQuestionMark = 53,
KeyOEM_102 = 54,
KeyOEM_8 = 55,
KeyF1 = 56,
KeyF2 = 57,
KeyF3 = 58,
KeyF4 = 59,
KeyF5 = 60,
KeyF6 = 61,
KeyF7 = 62,
KeyF8 = 63,
KeyF9 = 64,
KeyF10 = 65,
KeyF11 = 66,
KeyF12 = 67,
KeyF13 = 68,
KeyF14 = 69,
KeyF15 = 70,
KeyF16 = 71,
KeyF17 = 72,
KeyF18 = 73,
KeyF19 = 74,
KeyF20 = 75,
KeyF21 = 76,
KeyF22 = 77,
KeyF23 = 78,
KeyF24 = 79,
KeyEscape = 80,
KeyTab = 81,
KeyCapsLock = 82,
KeyBackspace = 83,
KeyEnter = 84,
KeyReturn = 84,
KeyLeftWindows = 85,
KeyRightWindows = 86,
KeyAppMenu = 87,
KeyInsert = 88,
KeyDelete = 89,
KeyHome = 90,
KeyEnd = 91,
KeyPageUp = 92,
KeyPageDown = 93,
KeyUp = 94,
KeyDown = 95,
KeyLeft = 96,
KeyRight = 97,
KeyPrintScreen = 98,
KeySystemRequest = 98,
KeyScrollLock = 99,
KeyPause = 100,
KeyBreak = 100,
KeyNumLock = 101,
KeyNumpad0 = 102,
KeyNumpadInsert = 102,
KeyNumpad1 = 103,
KeyNumpadEnd = 103,
KeyNumpad2 = 104,
KeyNumpadDown = 104,
KeyNumpad3 = 105,
KeyNumpadPageDown = 105,
KeyNumpad4 = 106,
KeyNumpadLeft = 106,
KeyNumpad5 = 107,
KeyNumpad6 = 108,
KeyNumpadRight = 108,
KeyNumpad7 = 109,
KeyNumpadHome = 109,
KeyNumpad8 = 110,
KeyNumpadUp = 110,
KeyNumpad9 = 111,
KeyNumpadPageUp = 111,
KeyNumpadPlus = 112,
KeyNumpadMinus = 113,
KeyNumpadTimes = 114,
KeyNumpadDivide = 115,
KeyNumpadEnter = 116,
KeyNumpadPeriod = 117,
KeyNumpadDelete = 117,
KeyBrowserBack = 118,
KeyBrowserForward = 119,
KeyVolumeDown = 120,
KeyVolumeUp = 121,

-- Mouse buttons
MouseButton1 = 122,
MouseLeftButton = 122,
MouseButton2 = 123,
MouseRightButton = 123,
MouseButton3 = 124,
MouseMiddleButton = 124,
MouseButton4 = 125,
MouseButton5 = 126,
MouseButton6 = 127,
MouseButton7 = 128,
MouseButton8 = 129,

-- Touch inputs,
TouchButton1 = 130,
TouchButton2 = 131,
TouchButton3 = 132,
TouchButton4 = 133,
TouchButton5 = 134,
TouchButtonFirst = 130,
TouchButtonLast = 134,

-- Xbox 360 controller buttons
XboxSectionStart = 135,
XboxA = 136,
XboxB = 137,
XboxX = 138,
XboxY = 139,
XboxLeftBumper = 140,
XboxRightBumper = 141,
XboxBack = 142,
XboxStart = 143,
XboxLeftThumbstickButton = 144,
XboxRightThumbstickButton = 145,
XboxLeftTrigger = 146,
XboxRightTrigger = 147,
XboxGuide = 148,
XboxDpadUp = 149,
XboxDpadDown = 150,
XboxDpadLeft = 151,
XboxDpadRight = 152,
XboxSectionEnd = 153,

-- PlayStation 3 controller buttons
PS3SectionStart = 154,
PS3X = 155,
PS3Square = 156,
PS3Circle = 157,
PS3Triangle = 158,
PS3L1 = 159,
PS3R1 = 160,
PS3L2 = 161,
PS3R2 = 162,
PS3L3 = 163,
PS3R3 = 164,
PS3Start = 165,
PS3Select = 166,
PS3PS = 167,
PS3DpadUp = 168,
PS3DpadDown = 169,
PS3DpadLeft = 170,
PS3DpadRight = 171,
PS3SectionEnd = 172,

-- Wiimote buttons
WiiSectionStart = 173,
WiiA = 174,
WiiB = 175,
Wii1 = 176,
Wii2 = 177,
WiiPlus = 178,
WiiMinus = 179,
WiiHome = 180,
WiiDpadUp = 181,
WiiDpadDown = 182,
WiiDpadLeft = 183,
WiiDpadRight = 184,

-- Wiimote Nunchuck buttons
WiiNunchuckC = 185,
WiiNunchuckZ = 186,
WiiSectionEnd = 187,

-- Generic Gamepad buttons
GamepadButton1 = 188,
GamepadButton2 = 189,
GamepadButton3 = 190,
GamepadButton4 = 191,
GamepadButton5 = 192,
GamepadButton6 = 193,
GamepadButton7 = 194,
GamepadButton8 = 195,
GamepadButton9 = 196,
GamepadButton10 = 197,
GamepadButton11 = 198,
GamepadButton12 = 199,
GamepadButton13 = 200,
GamepadButton14 = 201,
GamepadButton15 = 202,
GamepadButton16 = 203,
GamepadButton17 = 204,
GamepadButton18 = 205,
GamepadButton19 = 206,
GamepadButton20 = 207,

-- Unknown button - this must be the last element in this array
ButtonUnknown = 208
}

local InputManager = class_static 'InputManager'

local udInputManager = nil

function InputManager.cctor()

	udInputManager = CoreUtilities.NewNativeUserData(ScriptEngineInputManager)
	if nil == udInputManager then

		error 'Failed instantiating ScriptEngineInputManager.'
	end
end

function InputManager.GetMousePosition()

	local fX, fY = udInputManager:GetMousePosition()
	return fX, fY
end

function InputManager.HasSystemBindingLock()

	return udInputManager:HasSystemBindingLock()
end

function InputManager.IsBindingDown(sName)

	local bRet = udInputManager:IsBindingDown(sName)
	return bRet
end

function InputManager.WasBindingPressed(sName)

	local bRet = udInputManager:WasBindingPressed(sName)
	return bRet
end

function InputManager.WasBindingReleased(sName)

	local bRet = udInputManager:WasBindingReleased(sName)
	return bRet
end
return InputManager
