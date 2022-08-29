/**
 * \file InputKeys.h
 * \brief Defines the (fairly large) number of enum
 * constants that correspond to various button types
 * for various input devices across SeoulEngine platforms.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef INPUT_KEYS_H
#define INPUT_KEYS_H

namespace Seoul
{

// Note: this must be kept in sync with the g_aInputButtonNames[] array in
// InputManager.cpp
// It should also be kept in sync with the copy of this enum in InputManager.cs
// in the script project.
/**
 * Input button IDs
 */
enum class InputButton
{
	// Keyboard keys

	// IMPORTANT: Special keys must come first - this ensures that they
	// are updated first, so any subsequent key input events can
	// check InputManager::IsSpecialPressed() and get correct results.
	KeyLeftShift,
	KeyRightShift,
	KeyLeftControl,
	KeyRightControl,
	KeyLeftAlt,
	KeyRightAlt,

	KeyA,
	KeyB,
	KeyC,
	KeyD,
	KeyE,
	KeyF,
	KeyG,
	KeyH,
	KeyI,
	KeyJ,
	KeyK,
	KeyL,
	KeyM,
	KeyN,
	KeyO,
	KeyP,
	KeyQ,
	KeyR,
	KeyS,
	KeyT,
	KeyU,
	KeyV,
	KeyW,
	KeyX,
	KeyY,
	KeyZ,

	Key0,
	Key1,
	Key2,
	Key3,
	Key4,
	Key5,
	Key6,
	Key7,
	Key8,
	Key9,

	KeySpace,        KeySpaceBar     = KeySpace,

	KeyOEM_3,		 KeyGrave		 = KeyOEM_3,        KeyTilde        = KeyOEM_3,			KeyBackQuote    = KeyOEM_3,
	KeyOEM_Minus,	 KeyMinus		 = KeyOEM_Minus,    KeyUnderscore   = KeyOEM_Minus,
	KeyOEM_Plus,	 KeyEquals		 = KeyOEM_Plus,     KeyPlus         = KeyOEM_Plus,
	KeyOEM_4,		 KeyLeftBracket  = KeyOEM_4,		KeyLeftBrace    = KeyOEM_4,
	KeyOEM_6,		 KeyRightBracket = KeyOEM_6,		KeyRightBrace   = KeyOEM_6,
	KeyOEM_5,		 KeyBackslash	 = KeyOEM_5,		KeyPipe         = KeyOEM_5,
	KeyOEM_1,		 KeySemicolon	 = KeyOEM_1,		KeyColon        = KeyOEM_1,
	KeyOEM_7,		 KeyApostrophe   = KeyOEM_7,		KeyQuote        = KeyOEM_7,			KeySingleQuote  = KeyOEM_7,		KeyDoubleQuote  = KeyOEM_7,
	KeyOEM_Comma,	 KeyComma		 = KeyOEM_Comma,    KeyLessThan     = KeyOEM_Comma,
	KeyOEM_Period,   KeyPeriod		 = KeyOEM_Period,   KeyGreaterThan  = KeyOEM_Period,
	KeyOEM_2,		 KeySlash		 = KeyOEM_2,        KeyForwardSlash = KeyOEM_2,         KeyQuestionMark = KeyOEM_2,
	KeyOEM_102,
	KeyOEM_8,

	KeyF1,
	KeyF2,
	KeyF3,
	KeyF4,
	KeyF5,
	KeyF6,
	KeyF7,
	KeyF8,
	KeyF9,
	KeyF10,
	KeyF11,
	KeyF12,
	KeyF13,
	KeyF14,
	KeyF15,
	KeyF16,
	KeyF17,
	KeyF18,
	KeyF19,
	KeyF20,
	KeyF21,
	KeyF22,
	KeyF23,
	KeyF24,

	KeyEscape,
	KeyTab,
	KeyCapsLock,
	KeyBackspace,
	KeyEnter,         KeyReturn = KeyEnter,
	KeyLeftWindows,
	KeyRightWindows,
	KeyAppMenu,

	KeyInsert,
	KeyDelete,
	KeyHome,
	KeyEnd,
	KeyPageUp,
	KeyPageDown,

	KeyUp,
	KeyDown,
	KeyLeft,
	KeyRight,

	KeyPrintScreen, KeySystemRequest = KeyPrintScreen,
	KeyScrollLock,
	KeyPause, KeyBreak = KeyPause,

	KeyNumLock,
	KeyNumpad0, KeyNumpadInsert   = KeyNumpad0,
	KeyNumpad1, KeyNumpadEnd      = KeyNumpad1,
	KeyNumpad2, KeyNumpadDown     = KeyNumpad2,
	KeyNumpad3, KeyNumpadPageDown = KeyNumpad3,
	KeyNumpad4, KeyNumpadLeft     = KeyNumpad4,
	KeyNumpad5,
	KeyNumpad6, KeyNumpadRight    = KeyNumpad6,
	KeyNumpad7, KeyNumpadHome     = KeyNumpad7,
	KeyNumpad8, KeyNumpadUp       = KeyNumpad8,
	KeyNumpad9, KeyNumpadPageUp   = KeyNumpad9,

	KeyNumpadPlus,
	KeyNumpadMinus,
	KeyNumpadTimes,
	KeyNumpadDivide,
	KeyNumpadEnter,
	KeyNumpadPeriod, KeyNumpadDelete = KeyNumpadPeriod,

	KeyBrowserBack,
	KeyBrowserForward,
	KeyVolumeDown,
	KeyVolumeUp,

	// Mouse buttons
	MouseButton1, MouseLeftButton   = MouseButton1,
	MouseButton2, MouseRightButton  = MouseButton2,
	MouseButton3, MouseMiddleButton = MouseButton3,
	MouseButton4,
	MouseButton5,
	MouseButton6,
	MouseButton7,
	MouseButton8,

	// Touch inputs,
	TouchButton1,
	TouchButton2,
	TouchButton3,
	TouchButton4,
	TouchButton5, TouchButtonFirst = TouchButton1, TouchButtonLast = TouchButton5,

	// Xbox 360 controller buttons
	XboxSectionStart,
	XboxA,
	XboxB,
	XboxX,
	XboxY,
	XboxLeftBumper,
	XboxRightBumper,
	XboxBack,
	XboxStart,
	XboxLeftThumbstickButton,
	XboxRightThumbstickButton,
	XboxLeftTrigger,
	XboxRightTrigger,
	XboxGuide,
	XboxDpadUp,
	XboxDpadDown,
	XboxDpadLeft,
	XboxDpadRight,
	XboxSectionEnd,

	// PlayStation 3 controller buttons
	PS3SectionStart,
	PS3X,
	PS3Square,
	PS3Circle,
	PS3Triangle,
	PS3L1,
	PS3R1,
	PS3L2,
	PS3R2,
	PS3L3,
	PS3R3,
	PS3Start,
	PS3Select,
	PS3PS,
	PS3DpadUp,
	PS3DpadDown,
	PS3DpadLeft,
	PS3DpadRight,
	PS3SectionEnd,

	// Wiimote buttons
	WiiSectionStart,
	WiiA,
	WiiB,
	Wii1,
	Wii2,
	WiiPlus,
	WiiMinus,
	WiiHome,
	WiiDpadUp,
	WiiDpadDown,
	WiiDpadLeft,
	WiiDpadRight,

	// Wiimote Nunchuck buttons
	WiiNunchuckC,
	WiiNunchuckZ,
	WiiSectionEnd,

	// Generic Gamepad buttons
	GamepadButton1,
	GamepadButton2,
	GamepadButton3,
	GamepadButton4,
	GamepadButton5,
	GamepadButton6,
	GamepadButton7,
	GamepadButton8,
	GamepadButton9,
	GamepadButton10,
	GamepadButton11,
	GamepadButton12,
	GamepadButton13,
	GamepadButton14,
	GamepadButton15,
	GamepadButton16,
	GamepadButton17,
	GamepadButton18,
	GamepadButton19,
	GamepadButton20,

	// Unknown button - this must be the last element in this array
	ButtonUnknown,

	// These are not real values but are instead set for
	// query purposes.
	INPUT_BUTTON_COUNT,
	KEY_FIRST = KeyLeftShift,
	KEY_LAST = KeyVolumeUp,
	KEY_COUNT = (KEY_LAST - KEY_FIRST) + 1,
	MOUSE_FIRST = MouseButton1,
	MOUSE_LAST = MouseButton8,
	MOUSE_COUNT = (MOUSE_LAST - MOUSE_FIRST) + 1,
};

/**
 * Input axis IDs
 */
enum class InputAxis
{
	// Mouse axes
	MouseX,
	MouseY,
	MouseWheel,

	// Touch axes
	Touch1X, Touch1Y,
	Touch2X, Touch2Y,
	Touch3X, Touch3Y,
	Touch4X, Touch4Y,
	Touch5X, Touch5Y,

	// Generic game pad axes
	GamepadLeftThumbstickX,
	GamepadLeftThumbstickY,
	GamepadRightThumbstickX,
	GamepadRightThumbstickY,
	GamepadAxis5,
	GamepadAxis6,
	GamepadAxis7,
	GamepadAxis8,

	// Xbox 360 controller axes
	XboxLeftThumbstickX = GamepadLeftThumbstickX,
	XboxLeftThumbstickY = GamepadLeftThumbstickY,
	XboxRightThumbstickX = GamepadRightThumbstickX,
	XboxRightThumbstickY = GamepadRightThumbstickY,
	XboxLeftTriggerZ = GamepadAxis5,
	XboxRightTriggerZ = GamepadAxis6,

	// PlayStation 3 controller axes
	PS3LeftThumbstickX = GamepadLeftThumbstickX,
	PS3LeftThumbstickY = GamepadLeftThumbstickY,
	PS3RightThumbstickX = GamepadRightThumbstickX,
	PS3RightThumbstickY = GamepadRightThumbstickY,

	// Wiimote axes
	WiiInfrared1X,
	WiiInfrared1Y,
	WiiInfrared2X,
	WiiInfrared2Y,
	WiiAccelerationX,
	WiiAccelerationY,
	WiiAccelerationZ,

	// Wiimote Nunchuck axes
	WiiNunchuckThumbstickX = GamepadLeftThumbstickX,
	WiiNunchuckThumbstickY = GamepadLeftThumbstickY,
	WiiNunchuckAccelerationX,
	WiiNunchuckAccelerationY,
	WiiNunchuckAccelerationZ,

	// Unknown axis
	AxisUnknown,
};

/**
 * Possible events that can be generated by an input button
 */
enum class ButtonEventType
{
	ButtonPressed,
	ButtonReleased,
	ButtonRepeat
};

} // namespace Seoul

#endif // include guard
