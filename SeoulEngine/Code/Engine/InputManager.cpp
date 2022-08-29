/**
 * \file InputManager.cpp
 * \brief InputManager is the global singleton that manages InputDevice
 * instances for SeoulEngine. It also handles key remap and binding pressed
 * events.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Core.h"
#include "Engine.h"
#include "EventsManager.h"
#include "GamePaths.h"
#include "InputManager.h"
#include "Path.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDataStoreTableUtil.h"
#include "ReflectionDefine.h"
#include "SettingsManager.h"
#include "StringUtil.h"

namespace Seoul
{

/** Constants used to lookup and parse input.json. */
static const HString ksLeftStickDeadZone("LeftStickDeadZone");
static const HString ksInputAxisBindings("InputAxisBindings");
static const HString ksInputAxisToButtonBindings("InputAxisToButtonBindings");
static const HString ksInputButtonBindings("InputButtonBindings");
static const HString ksInputSettings("InputSettings");
static const HString ksRightStickDeadZone("RightStickDeadZone");
static const HString ksTriggerDeadZone("TriggerDeadZone");

static inline FilePath GetInputConfigFilePath()
{
	return FilePath::CreateSaveFilePath("input_config.json");
}

SEOUL_BEGIN_ENUM(ButtonEventType)
	SEOUL_ENUM(ButtonEventType::ButtonPressed)
	SEOUL_ENUM(ButtonEventType::ButtonReleased)
	SEOUL_ENUM(ButtonEventType::ButtonRepeat)
SEOUL_END_ENUM()

SEOUL_BEGIN_ENUM(InputAxis)
	// Mouse axes
	SEOUL_ENUM(InputAxis::MouseX)
	SEOUL_ENUM(InputAxis::MouseY)
	SEOUL_ENUM(InputAxis::MouseWheel)

	// Touch axes
	SEOUL_ENUM(InputAxis::Touch1X) SEOUL_ENUM(InputAxis::Touch1Y)
	SEOUL_ENUM(InputAxis::Touch2X) SEOUL_ENUM(InputAxis::Touch2Y)
	SEOUL_ENUM(InputAxis::Touch3X) SEOUL_ENUM(InputAxis::Touch3Y)
	SEOUL_ENUM(InputAxis::Touch4X) SEOUL_ENUM(InputAxis::Touch4Y)
	SEOUL_ENUM(InputAxis::Touch5X) SEOUL_ENUM(InputAxis::Touch5Y)

	// Generic game pad axes
	SEOUL_ENUM(InputAxis::GamepadLeftThumbstickX)
	SEOUL_ENUM(InputAxis::GamepadLeftThumbstickY)
	SEOUL_ENUM(InputAxis::GamepadRightThumbstickX)
	SEOUL_ENUM(InputAxis::GamepadRightThumbstickY)
	SEOUL_ENUM(InputAxis::GamepadAxis5)
	SEOUL_ENUM(InputAxis::GamepadAxis6)
	SEOUL_ENUM(InputAxis::GamepadAxis7)
	SEOUL_ENUM(InputAxis::GamepadAxis8)

	// Xbox 360 controller axes
	SEOUL_ENUM(InputAxis::XboxLeftThumbstickX)
	SEOUL_ENUM(InputAxis::GamepadLeftThumbstickX)
	SEOUL_ENUM(InputAxis::XboxLeftThumbstickY)
	SEOUL_ENUM(InputAxis::GamepadLeftThumbstickY)
	SEOUL_ENUM(InputAxis::XboxRightThumbstickX)
	SEOUL_ENUM(InputAxis::GamepadRightThumbstickX)
	SEOUL_ENUM(InputAxis::XboxRightThumbstickY)
	SEOUL_ENUM(InputAxis::GamepadRightThumbstickY)
	SEOUL_ENUM(InputAxis::XboxLeftTriggerZ)
	SEOUL_ENUM(InputAxis::GamepadAxis5)
	SEOUL_ENUM(InputAxis::XboxRightTriggerZ)
	SEOUL_ENUM(InputAxis::GamepadAxis6)

	// PlayStation 3 controller axes
	SEOUL_ENUM(InputAxis::PS3LeftThumbstickX)
	SEOUL_ENUM(InputAxis::GamepadLeftThumbstickX)
	SEOUL_ENUM(InputAxis::PS3LeftThumbstickY)
	SEOUL_ENUM(InputAxis::GamepadLeftThumbstickY)
	SEOUL_ENUM(InputAxis::PS3RightThumbstickX)
	SEOUL_ENUM(InputAxis::GamepadRightThumbstickX)
	SEOUL_ENUM(InputAxis::PS3RightThumbstickY)
	SEOUL_ENUM(InputAxis::GamepadRightThumbstickY)

	// Wiimote axes
	SEOUL_ENUM(InputAxis::WiiInfrared1X)
	SEOUL_ENUM(InputAxis::WiiInfrared1Y)
	SEOUL_ENUM(InputAxis::WiiInfrared2X)
	SEOUL_ENUM(InputAxis::WiiInfrared2Y)
	SEOUL_ENUM(InputAxis::WiiAccelerationX)
	SEOUL_ENUM(InputAxis::WiiAccelerationY)
	SEOUL_ENUM(InputAxis::WiiAccelerationZ)

	// Wiimote Nunchuck axes
	SEOUL_ENUM(InputAxis::WiiNunchuckThumbstickX)
	SEOUL_ENUM(InputAxis::GamepadLeftThumbstickX)
	SEOUL_ENUM(InputAxis::WiiNunchuckThumbstickY)
	SEOUL_ENUM(InputAxis::GamepadLeftThumbstickY)
	SEOUL_ENUM(InputAxis::WiiNunchuckAccelerationX)
	SEOUL_ENUM(InputAxis::WiiNunchuckAccelerationY)
	SEOUL_ENUM(InputAxis::WiiNunchuckAccelerationZ)

	// Unknown axis
	SEOUL_ENUM(InputAxis::AxisUnknown)
SEOUL_END_ENUM()

SEOUL_BEGIN_ENUM(InputButton)
	SEOUL_ENUM(InputButton::KeyLeftShift)
	SEOUL_ENUM(InputButton::KeyRightShift)
	SEOUL_ENUM(InputButton::KeyLeftControl)
	SEOUL_ENUM(InputButton::KeyRightControl)
	SEOUL_ENUM(InputButton::KeyLeftAlt)
	SEOUL_ENUM(InputButton::KeyRightAlt)
	SEOUL_ENUM(InputButton::KeyA)
	SEOUL_ENUM(InputButton::KeyB)
	SEOUL_ENUM(InputButton::KeyC)
	SEOUL_ENUM(InputButton::KeyD)
	SEOUL_ENUM(InputButton::KeyE)
	SEOUL_ENUM(InputButton::KeyF)
	SEOUL_ENUM(InputButton::KeyG)
	SEOUL_ENUM(InputButton::KeyH)
	SEOUL_ENUM(InputButton::KeyI)
	SEOUL_ENUM(InputButton::KeyJ)
	SEOUL_ENUM(InputButton::KeyK)
	SEOUL_ENUM(InputButton::KeyL)
	SEOUL_ENUM(InputButton::KeyM)
	SEOUL_ENUM(InputButton::KeyN)
	SEOUL_ENUM(InputButton::KeyO)
	SEOUL_ENUM(InputButton::KeyP)
	SEOUL_ENUM(InputButton::KeyQ)
	SEOUL_ENUM(InputButton::KeyR)
	SEOUL_ENUM(InputButton::KeyS)
	SEOUL_ENUM(InputButton::KeyT)
	SEOUL_ENUM(InputButton::KeyU)
	SEOUL_ENUM(InputButton::KeyV)
	SEOUL_ENUM(InputButton::KeyW)
	SEOUL_ENUM(InputButton::KeyX)
	SEOUL_ENUM(InputButton::KeyY)
	SEOUL_ENUM(InputButton::KeyZ)
	SEOUL_ENUM(InputButton::Key0)
	SEOUL_ENUM(InputButton::Key1)
	SEOUL_ENUM(InputButton::Key2)
	SEOUL_ENUM(InputButton::Key3)
	SEOUL_ENUM(InputButton::Key4)
	SEOUL_ENUM(InputButton::Key5)
	SEOUL_ENUM(InputButton::Key6)
	SEOUL_ENUM(InputButton::Key7)
	SEOUL_ENUM(InputButton::Key8)
	SEOUL_ENUM(InputButton::Key9)
	SEOUL_ENUM(InputButton::KeySpace)
	SEOUL_ENUM(InputButton::KeySpaceBar)
	SEOUL_ENUM(InputButton::KeyOEM_3)
	SEOUL_ENUM(InputButton::KeyGrave)
	SEOUL_ENUM(InputButton::KeyTilde)
	SEOUL_ENUM(InputButton::KeyBackQuote)
	SEOUL_ENUM(InputButton::KeyOEM_Minus)
	SEOUL_ENUM(InputButton::KeyMinus)
	SEOUL_ENUM(InputButton::KeyUnderscore)
	SEOUL_ENUM(InputButton::KeyOEM_Plus)
	SEOUL_ENUM(InputButton::KeyEquals)
	SEOUL_ENUM(InputButton::KeyPlus)
	SEOUL_ENUM(InputButton::KeyOEM_4)
	SEOUL_ENUM(InputButton::KeyLeftBracket)
	SEOUL_ENUM(InputButton::KeyLeftBrace)
	SEOUL_ENUM(InputButton::KeyOEM_6)
	SEOUL_ENUM(InputButton::KeyRightBracket)
	SEOUL_ENUM(InputButton::KeyRightBrace)
	SEOUL_ENUM(InputButton::KeyOEM_5)
	SEOUL_ENUM(InputButton::KeyBackslash)
	SEOUL_ENUM(InputButton::KeyPipe)
	SEOUL_ENUM(InputButton::KeyOEM_1)
	SEOUL_ENUM(InputButton::KeySemicolon)
	SEOUL_ENUM(InputButton::KeyColon)
	SEOUL_ENUM(InputButton::KeyOEM_7)
	SEOUL_ENUM(InputButton::KeyApostrophe)
	SEOUL_ENUM(InputButton::KeyQuote)
	SEOUL_ENUM(InputButton::KeySingleQuote)
	SEOUL_ENUM(InputButton::KeyDoubleQuote)
	SEOUL_ENUM(InputButton::KeyOEM_Comma)
	SEOUL_ENUM(InputButton::KeyComma)
	SEOUL_ENUM(InputButton::KeyLessThan)
	SEOUL_ENUM(InputButton::KeyOEM_Period)
	SEOUL_ENUM(InputButton::KeyPeriod)
	SEOUL_ENUM(InputButton::KeyGreaterThan)
	SEOUL_ENUM(InputButton::KeyOEM_2)
	SEOUL_ENUM(InputButton::KeySlash)
	SEOUL_ENUM(InputButton::KeyForwardSlash)
	SEOUL_ENUM(InputButton::KeyQuestionMark)
	SEOUL_ENUM(InputButton::KeyOEM_102)
	SEOUL_ENUM(InputButton::KeyOEM_8)
	SEOUL_ENUM(InputButton::KeyF1)
	SEOUL_ENUM(InputButton::KeyF2)
	SEOUL_ENUM(InputButton::KeyF3)
	SEOUL_ENUM(InputButton::KeyF4)
	SEOUL_ENUM(InputButton::KeyF5)
	SEOUL_ENUM(InputButton::KeyF6)
	SEOUL_ENUM(InputButton::KeyF7)
	SEOUL_ENUM(InputButton::KeyF8)
	SEOUL_ENUM(InputButton::KeyF9)
	SEOUL_ENUM(InputButton::KeyF10)
	SEOUL_ENUM(InputButton::KeyF11)
	SEOUL_ENUM(InputButton::KeyF12)
	SEOUL_ENUM(InputButton::KeyF13)
	SEOUL_ENUM(InputButton::KeyF14)
	SEOUL_ENUM(InputButton::KeyF15)
	SEOUL_ENUM(InputButton::KeyF16)
	SEOUL_ENUM(InputButton::KeyF17)
	SEOUL_ENUM(InputButton::KeyF18)
	SEOUL_ENUM(InputButton::KeyF19)
	SEOUL_ENUM(InputButton::KeyF20)
	SEOUL_ENUM(InputButton::KeyF21)
	SEOUL_ENUM(InputButton::KeyF22)
	SEOUL_ENUM(InputButton::KeyF23)
	SEOUL_ENUM(InputButton::KeyF24)
	SEOUL_ENUM(InputButton::KeyEscape)
	SEOUL_ENUM(InputButton::KeyTab)
	SEOUL_ENUM(InputButton::KeyCapsLock)
	SEOUL_ENUM(InputButton::KeyBackspace)
	SEOUL_ENUM(InputButton::KeyEnter)
	SEOUL_ENUM(InputButton::KeyReturn)
	SEOUL_ENUM(InputButton::KeyLeftWindows)
	SEOUL_ENUM(InputButton::KeyRightWindows)
	SEOUL_ENUM(InputButton::KeyAppMenu)
	SEOUL_ENUM(InputButton::KeyInsert)
	SEOUL_ENUM(InputButton::KeyDelete)
	SEOUL_ENUM(InputButton::KeyHome)
	SEOUL_ENUM(InputButton::KeyEnd)
	SEOUL_ENUM(InputButton::KeyPageUp)
	SEOUL_ENUM(InputButton::KeyPageDown)
	SEOUL_ENUM(InputButton::KeyUp)
	SEOUL_ENUM(InputButton::KeyDown)
	SEOUL_ENUM(InputButton::KeyLeft)
	SEOUL_ENUM(InputButton::KeyRight)
	SEOUL_ENUM(InputButton::KeyPrintScreen)
	SEOUL_ENUM(InputButton::KeySystemRequest)
	SEOUL_ENUM(InputButton::KeyScrollLock)
	SEOUL_ENUM(InputButton::KeyPause)
	SEOUL_ENUM(InputButton::KeyBreak)
	SEOUL_ENUM(InputButton::KeyNumLock)
	SEOUL_ENUM(InputButton::KeyNumpad0)
	SEOUL_ENUM(InputButton::KeyNumpadInsert)
	SEOUL_ENUM(InputButton::KeyNumpad1)
	SEOUL_ENUM(InputButton::KeyNumpadEnd)
	SEOUL_ENUM(InputButton::KeyNumpad2)
	SEOUL_ENUM(InputButton::KeyNumpadDown)
	SEOUL_ENUM(InputButton::KeyNumpad3)
	SEOUL_ENUM(InputButton::KeyNumpadPageDown)
	SEOUL_ENUM(InputButton::KeyNumpad4)
	SEOUL_ENUM(InputButton::KeyNumpadLeft)
	SEOUL_ENUM(InputButton::KeyNumpad5)
	SEOUL_ENUM(InputButton::KeyNumpad6)
	SEOUL_ENUM(InputButton::KeyNumpadRight)
	SEOUL_ENUM(InputButton::KeyNumpad7)
	SEOUL_ENUM(InputButton::KeyNumpadHome)
	SEOUL_ENUM(InputButton::KeyNumpad8)
	SEOUL_ENUM(InputButton::KeyNumpadUp)
	SEOUL_ENUM(InputButton::KeyNumpad9)
	SEOUL_ENUM(InputButton::KeyNumpadPageUp)
	SEOUL_ENUM(InputButton::KeyNumpadPlus)
	SEOUL_ENUM(InputButton::KeyNumpadMinus)
	SEOUL_ENUM(InputButton::KeyNumpadTimes)
	SEOUL_ENUM(InputButton::KeyNumpadDivide)
	SEOUL_ENUM(InputButton::KeyNumpadEnter)
	SEOUL_ENUM(InputButton::KeyNumpadPeriod)
	SEOUL_ENUM(InputButton::KeyNumpadDelete)
	SEOUL_ENUM(InputButton::KeyBrowserBack)
	SEOUL_ENUM(InputButton::KeyBrowserForward)
	SEOUL_ENUM(InputButton::KeyVolumeDown)
	SEOUL_ENUM(InputButton::KeyVolumeUp)
	SEOUL_ENUM(InputButton::MouseButton1)
	SEOUL_ENUM(InputButton::MouseLeftButton)
	SEOUL_ENUM(InputButton::MouseButton2)
	SEOUL_ENUM(InputButton::MouseRightButton)
	SEOUL_ENUM(InputButton::MouseButton3)
	SEOUL_ENUM(InputButton::MouseMiddleButton)
	SEOUL_ENUM(InputButton::MouseButton4)
	SEOUL_ENUM(InputButton::MouseButton5)
	SEOUL_ENUM(InputButton::MouseButton6)
	SEOUL_ENUM(InputButton::MouseButton7)
	SEOUL_ENUM(InputButton::MouseButton8)
	SEOUL_ENUM(InputButton::TouchButton1)
	SEOUL_ENUM(InputButton::TouchButton2)
	SEOUL_ENUM(InputButton::TouchButton3)
	SEOUL_ENUM(InputButton::TouchButton4)
	SEOUL_ENUM(InputButton::TouchButton5)
	SEOUL_ENUM(InputButton::XboxSectionStart)
	SEOUL_ENUM(InputButton::XboxA)
	SEOUL_ENUM(InputButton::XboxB)
	SEOUL_ENUM(InputButton::XboxX)
	SEOUL_ENUM(InputButton::XboxY)
	SEOUL_ENUM(InputButton::XboxLeftBumper)
	SEOUL_ENUM(InputButton::XboxRightBumper)
	SEOUL_ENUM(InputButton::XboxBack)
	SEOUL_ENUM(InputButton::XboxStart)
	SEOUL_ENUM(InputButton::XboxLeftThumbstickButton)
	SEOUL_ENUM(InputButton::XboxRightThumbstickButton)
	SEOUL_ENUM(InputButton::XboxLeftTrigger)
	SEOUL_ENUM(InputButton::XboxRightTrigger)
	SEOUL_ENUM(InputButton::XboxGuide)
	SEOUL_ENUM(InputButton::XboxDpadUp)
	SEOUL_ENUM(InputButton::XboxDpadDown)
	SEOUL_ENUM(InputButton::XboxDpadLeft)
	SEOUL_ENUM(InputButton::XboxDpadRight)
	SEOUL_ENUM(InputButton::XboxSectionEnd)
	SEOUL_ENUM(InputButton::PS3SectionStart)
	SEOUL_ENUM(InputButton::PS3X)
	SEOUL_ENUM(InputButton::PS3Square)
	SEOUL_ENUM(InputButton::PS3Circle)
	SEOUL_ENUM(InputButton::PS3Triangle)
	SEOUL_ENUM(InputButton::PS3L1)
	SEOUL_ENUM(InputButton::PS3R1)
	SEOUL_ENUM(InputButton::PS3L2)
	SEOUL_ENUM(InputButton::PS3R2)
	SEOUL_ENUM(InputButton::PS3L3)
	SEOUL_ENUM(InputButton::PS3R3)
	SEOUL_ENUM(InputButton::PS3Start)
	SEOUL_ENUM(InputButton::PS3Select)
	SEOUL_ENUM(InputButton::PS3PS)
	SEOUL_ENUM(InputButton::PS3DpadUp)
	SEOUL_ENUM(InputButton::PS3DpadDown)
	SEOUL_ENUM(InputButton::PS3DpadLeft)
	SEOUL_ENUM(InputButton::PS3DpadRight)
	SEOUL_ENUM(InputButton::PS3SectionEnd)
	SEOUL_ENUM(InputButton::WiiSectionStart)
	SEOUL_ENUM(InputButton::WiiA)
	SEOUL_ENUM(InputButton::WiiB)
	SEOUL_ENUM(InputButton::Wii1)
	SEOUL_ENUM(InputButton::Wii2)
	SEOUL_ENUM(InputButton::WiiPlus)
	SEOUL_ENUM(InputButton::WiiMinus)
	SEOUL_ENUM(InputButton::WiiHome)
	SEOUL_ENUM(InputButton::WiiDpadUp)
	SEOUL_ENUM(InputButton::WiiDpadDown)
	SEOUL_ENUM(InputButton::WiiDpadLeft)
	SEOUL_ENUM(InputButton::WiiDpadRight)
	SEOUL_ENUM(InputButton::WiiNunchuckC)
	SEOUL_ENUM(InputButton::WiiNunchuckZ)
	SEOUL_ENUM(InputButton::WiiSectionEnd)
	SEOUL_ENUM(InputButton::GamepadButton1)
	SEOUL_ENUM(InputButton::GamepadButton2)
	SEOUL_ENUM(InputButton::GamepadButton3)
	SEOUL_ENUM(InputButton::GamepadButton4)
	SEOUL_ENUM(InputButton::GamepadButton5)
	SEOUL_ENUM(InputButton::GamepadButton6)
	SEOUL_ENUM(InputButton::GamepadButton7)
	SEOUL_ENUM(InputButton::GamepadButton8)
	SEOUL_ENUM(InputButton::GamepadButton9)
	SEOUL_ENUM(InputButton::GamepadButton10)
	SEOUL_ENUM(InputButton::GamepadButton11)
	SEOUL_ENUM(InputButton::GamepadButton12)
	SEOUL_ENUM(InputButton::GamepadButton13)
	SEOUL_ENUM(InputButton::GamepadButton14)
	SEOUL_ENUM(InputButton::GamepadButton15)
	SEOUL_ENUM(InputButton::GamepadButton16)
	SEOUL_ENUM(InputButton::GamepadButton17)
	SEOUL_ENUM(InputButton::GamepadButton18)
	SEOUL_ENUM(InputButton::GamepadButton19)
	SEOUL_ENUM(InputButton::GamepadButton20)
	SEOUL_ENUM(InputButton::ButtonUnknown)
SEOUL_END_ENUM()

/**
 * Map of virtual key codes (VKs) to SeoulEngine button ids.
 */
static const InputButton g_aVirtualKeyMap[256] =
{
	InputButton::ButtonUnknown,    // 0x00
	InputButton::ButtonUnknown,    // 0x01 VK_LBUTTON. Does not handle mouse input
	InputButton::ButtonUnknown,    // 0x02 VK_RBUTTON. Does not handle mouse input
	InputButton::ButtonUnknown,    // 0x03 VK_CANCEL. Not handled
	InputButton::ButtonUnknown,    // 0x04 VK_MBUTTON. Does not handle mouse input
	InputButton::ButtonUnknown,    // 0x05 VK_XBUTTON1. Does not handle mouse input
	InputButton::ButtonUnknown,    // 0x06 VK_XBUTTON2. Does not handle mouse input
	InputButton::ButtonUnknown,    // 0x07
	InputButton::KeyBackspace,     // 0x08 VK_BACK
	InputButton::KeyTab,           // 0x09 VK_TAB
	InputButton::ButtonUnknown,	  // 0x0A Reserved
	InputButton::ButtonUnknown,    // 0x0B Reserved
	InputButton::ButtonUnknown,    // 0x0C KV_CLEAR. Not handled
	InputButton::KeyEnter,         // 0x0D DIK_RETURN
	InputButton::ButtonUnknown,	  // 0x0E
	InputButton::ButtonUnknown,    // 0x0F
	InputButton::ButtonUnknown,    // 0x10 VK_SHIFT. Does not distinguish between left or right, ignore as the left and right versions will be checked instead.
	InputButton::ButtonUnknown,    // 0x11 VK_CTRL. Does not distinguish between left or right, ignore as the left and right versions will be checked instead.
	InputButton::ButtonUnknown,    // 0x12 VK_MENU. Does not distinguish between left or right, ignore as the left and right versions will be checked instead.
	InputButton::KeyPause,		  // 0x13 VK_PAUSE
	InputButton::KeyCapsLock,	  // 0x14 VK_CAPITAL
	InputButton::ButtonUnknown,    // 0x15 VK_KANA, VK_HANGUEL, VK_HANGUL. Not handled
	InputButton::ButtonUnknown,    // 0x16
	InputButton::ButtonUnknown,    // 0x17 VK_JUNJA. Not handled
	InputButton::ButtonUnknown,    // 0x18 VK_FINAL. Not handled
	InputButton::ButtonUnknown,    // 0x19 VK_HANJA, VK_KANJI. Not handled
	InputButton::ButtonUnknown,    // 0x1A
	InputButton::KeyEscape,		  // 0x1B VK_ESCAPE
	InputButton::ButtonUnknown,    // 0x1C VK_CONVERT. Not handled
	InputButton::ButtonUnknown,    // 0x1D VK_NONCONVERT. Not handled
	InputButton::ButtonUnknown,    // 0x1E VK_ACCEPT. Not handled
	InputButton::ButtonUnknown,    // 0x1F VK_MODECHANGE. Not handled
	InputButton::KeySpace,         // 0x20 VK_SPACE
	InputButton::KeyPageUp,        // 0x21 VK_PRIOR
	InputButton::KeyPageDown,      // 0x22 VK_NEXT
	InputButton::KeyEnd,			  // 0x23 VK_END
	InputButton::KeyHome,		  // 0x24 VK_HOME
	InputButton::KeyLeft,		  // 0x25 VK_LEFT
	InputButton::KeyUp,			  // 0x26 VK_UP
	InputButton::KeyRight,		  // 0x27 VK_RIGHT
	InputButton::KeyDown,		  // 0x28 VK_DOWN
	InputButton::ButtonUnknown,	  // 0x29 VK_SELECT. Not handled
	InputButton::ButtonUnknown,	  // 0x2A VK_PRINT. Not handled
	InputButton::ButtonUnknown,	  // 0x2B VK_EXECUTE. Not handled
	InputButton::KeyPrintScreen,   // 0x2C VK_SNAPSHOT
	InputButton::KeyInsert,		  // 0x2D VK_INSERT
	InputButton::KeyDelete,		  // 0x2E VK_DELETE
	InputButton::ButtonUnknown,	  // 0x2F VK_HELP. Not handled
	InputButton::Key0,             // 0x30 '0'
	InputButton::Key1,             // 0x31 '1'
	InputButton::Key2,             // 0x32 '2'
	InputButton::Key3,             // 0x33 '3'
	InputButton::Key4,             // 0x34 '4'
	InputButton::Key5,             // 0x35 '5'
	InputButton::Key6,             // 0x36 '6'
	InputButton::Key7,             // 0x37 '7'
	InputButton::Key8,             // 0x38 '8'
	InputButton::Key9,             // 0x39 '9'
	InputButton::ButtonUnknown,    // 0x3A
	InputButton::ButtonUnknown,    // 0x3B
	InputButton::ButtonUnknown,    // 0x3C
	InputButton::ButtonUnknown,    // 0x3D
	InputButton::ButtonUnknown,    // 0x3E
	InputButton::ButtonUnknown,    // 0x3F
	InputButton::ButtonUnknown,    // 0x40
	InputButton::KeyA,             // 0x41 'A'
	InputButton::KeyB,             // 0x42 'B'
	InputButton::KeyC,             // 0x43 'C'
	InputButton::KeyD,             // 0x44 'D'
	InputButton::KeyE,             // 0x45 'E'
	InputButton::KeyF,             // 0x46 'F'
	InputButton::KeyG,			  // 0x47 'G'
	InputButton::KeyH,             // 0x48 'H'
	InputButton::KeyI,             // 0x49 'I'
	InputButton::KeyJ,             // 0x4A 'J'
	InputButton::KeyK,			  // 0x4B 'K'
	InputButton::KeyL,			  // 0x4C 'L'
	InputButton::KeyM,			  // 0x4D 'M'
	InputButton::KeyN,			  // 0x4E 'N'
	InputButton::KeyO,			  // 0x4F 'O'
	InputButton::KeyP,			  // 0x50 'P'
	InputButton::KeyQ,			  // 0x51 'Q'
	InputButton::KeyR,			  // 0x52 'R'
	InputButton::KeyS,			  // 0x53 'S'
	InputButton::KeyT,			  // 0x54 'T'
	InputButton::KeyU,			  // 0x55 'U'
	InputButton::KeyV,			  // 0x56 'V'
	InputButton::KeyW,             // 0x57 'W'
	InputButton::KeyX,             // 0x58 'X'
	InputButton::KeyY,			  // 0x59 'Y'
	InputButton::KeyZ,			  // 0x5A 'Z'
	InputButton::KeyLeftWindows,   // 0x5B VK_LWIN
	InputButton::KeyRightWindows,  // 0x5C VK_RWIN
	InputButton::KeyAppMenu,       // 0x5D VK_APPS
	InputButton::ButtonUnknown,    // 0x5E Reserved
	InputButton::ButtonUnknown,    // 0x5F VK_SLEEP
	InputButton::KeyNumpad0,       // 0x60 VK_NUMPAD0
	InputButton::KeyNumpad1,       // 0x61 VK_NUMPAD1
	InputButton::KeyNumpad2,       // 0x62 VK_NUMPAD2
	InputButton::KeyNumpad3,       // 0x63 VK_NUMPAD3
	InputButton::KeyNumpad4,       // 0x64 VK_NUMPAD4
	InputButton::KeyNumpad5,       // 0x65 VK_NUMPAD5
	InputButton::KeyNumpad6,       // 0x66 VK_NUMPAD6
	InputButton::KeyNumpad7,		  // 0x67 VK_NUMPAD7
	InputButton::KeyNumpad8,       // 0x68 VK_NUMPAD8
	InputButton::KeyNumpad9,       // 0x69 VK_NUMPAD9
	InputButton::KeyNumpadTimes,   // 0x6A VK_MULTIPLY
	InputButton::KeyNumpadPlus,    // 0x6B VK_ADD
	InputButton::KeyNumpadEnter,   // 0x6C VK_SEPARATOR
	InputButton::KeyNumpadMinus,   // 0x6D VK_SUBTRACT
	InputButton::KeyNumpadPeriod,  // 0x6E VK_DECIMAL
	InputButton::KeyNumpadDivide,  // 0x6F VK_DIVIDE
	InputButton::KeyF1,			  // 0x70 VK_F1
	InputButton::KeyF2,			  // 0x71 VK_F2
	InputButton::KeyF3,			  // 0x72 VK_F3
	InputButton::KeyF4,			  // 0x73 VK_F4
	InputButton::KeyF5,			  // 0x74 VK_F5
	InputButton::KeyF6,			  // 0x75 VK_F6
	InputButton::KeyF7,			  // 0x76 VK_F7
	InputButton::KeyF8,			  // 0x77 VK_F8
	InputButton::KeyF9,			  // 0x78 VK_F9
	InputButton::KeyF10,			  // 0x79 VK_F10
	InputButton::KeyF11,			  // 0x7A VK_F11
	InputButton::KeyF12,			  // 0x7B VK_F12
	InputButton::KeyF13,			  // 0x7C VK_F13
	InputButton::KeyF14,			  // 0x7D VK_F14
	InputButton::KeyF15,			  // 0x7E VK_F15
	InputButton::KeyF16,			  // 0x7F VK_F16
	InputButton::KeyF17,			  // 0x80 VK_F17
	InputButton::KeyF18,			  // 0x81 VK_F18
	InputButton::KeyF19,			  // 0x82 VK_F19
	InputButton::KeyF20,			  // 0x83 VK_F20
	InputButton::KeyF21,			  // 0x84 VK_F21
	InputButton::KeyF22,			  // 0x85 VK_F22
	InputButton::KeyF23,			  // 0x86 VK_F23
	InputButton::KeyF24,			  // 0x87 VK_F24
	InputButton::ButtonUnknown,    // 0x88
	InputButton::ButtonUnknown,    // 0x89
	InputButton::ButtonUnknown,    // 0x8A
	InputButton::ButtonUnknown,    // 0x8B
	InputButton::ButtonUnknown,    // 0x8C
	InputButton::ButtonUnknown,    // 0x8D
	InputButton::ButtonUnknown,    // 0x8E
	InputButton::ButtonUnknown,    // 0x8F
	InputButton::KeyNumLock,       // 0x90 VK_NUMLOCK
	InputButton::KeyScrollLock,    // 0x91 VK_SCROLL
	InputButton::ButtonUnknown,    // 0x92 VK_OEM_FJ_JISHO. Not handled
	InputButton::ButtonUnknown,    // 0x93 VK_OEM_FJ_MASSHOU. Not handled
	InputButton::ButtonUnknown,    // 0x94 VK_OEM_FJ_TOUROKU. Not handled
	InputButton::ButtonUnknown,    // 0x95 VK_OEM_FJ_LOYA. Not handled
	InputButton::ButtonUnknown,    // 0x96 VK_OEM_FJ_ROYA. Not handled
	InputButton::ButtonUnknown,    // 0x97
	InputButton::ButtonUnknown,    // 0x98
	InputButton::ButtonUnknown,    // 0x99
	InputButton::ButtonUnknown,    // 0x9A
	InputButton::ButtonUnknown,    // 0x9B
	InputButton::ButtonUnknown,    // 0x9C
	InputButton::ButtonUnknown,    // 0x9D
	InputButton::ButtonUnknown,    // 0x9E
	InputButton::ButtonUnknown,    // 0x9F
	InputButton::KeyLeftShift,     // 0xA0 VK_LSHIFT
	InputButton::KeyRightShift,    // 0xA1 VK_RSHIFT
	InputButton::KeyLeftControl,   // 0xA2 VK_LCONTROL
	InputButton::KeyRightControl,  // 0xA3 VK_RCONTROL
	InputButton::KeyLeftAlt,       // 0xA4 VK_LMENU
	InputButton::KeyRightAlt,      // 0xA5 VK_RMENU
	InputButton::KeyBrowserBack,   // 0xA6 VK_BROWSER_BACK, Not handled
	InputButton::KeyBrowserForward,// 0xA7 VK_BROWSER_FORWARD. Not handled
	InputButton::ButtonUnknown,    // 0xA8 VK_BROWSER_REFRESH. Not handled
	InputButton::ButtonUnknown,    // 0xA9 VK_BROWSER_STOP. Not handled
	InputButton::ButtonUnknown,    // 0xAA VK_BROWSER_SEARCH. Not handled
	InputButton::ButtonUnknown,    // 0xAB VK_BROWSER_FAVORITES. Not handled
	InputButton::ButtonUnknown,    // 0xAC VK_BROWSER_HOME. Not handled
	InputButton::ButtonUnknown,    // 0xAD VK_VOLUME_MUTE
	InputButton::KeyVolumeDown,    // 0xAE VK_VOLUME_DOWN
	InputButton::KeyVolumeUp,      // 0xAF VK_VOLUME_UP
	InputButton::ButtonUnknown,    // 0xB0 VK_MEDIA_NEXT_TRACK
	InputButton::ButtonUnknown,    // 0xB1 VK_MEDIA_PREV_TRACK
	InputButton::ButtonUnknown,    // 0xB2 VK_MEDIA_STOP
	InputButton::ButtonUnknown,    // 0xB3 VK_MEDIA_PLAY_PAUSE
	InputButton::ButtonUnknown,    // 0xB4 VK_LAUNCH_MAIL
	InputButton::ButtonUnknown,    // 0xB5 VK_LAUNCH_MEDIA_SELECT
	InputButton::ButtonUnknown,    // 0xB6 VK_LAUNCH_APP1
	InputButton::ButtonUnknown,    // 0xB7 VK_LAUNCH_APP2
	InputButton::ButtonUnknown,    // 0xB8 Reserved
	InputButton::ButtonUnknown,    // 0xB9 Reserved
	InputButton::KeySemicolon,     // 0xBA VK_OEM_1 (;:)
	InputButton::KeyOEM_Plus,      // 0xBB VK_OEM_PLUS (=+)
	InputButton::KeyOEM_Comma,     // 0xBC VK_OEM_COMMA (,<)
	InputButton::KeyOEM_Minus,     // 0xBD VK_OEM_MINUS (-_)
	InputButton::KeyOEM_Period,    // 0xBE VK_OEM_PERIOD (.>)
	InputButton::KeyOEM_2,	      // 0xBF VK_OEM_2 (/?)
	InputButton::KeyOEM_3,         // 0xC0 VK_OEM_3 (`~)
	InputButton::ButtonUnknown,    // 0xC1 Reserved
	InputButton::ButtonUnknown,    // 0xC2 Reserved
	InputButton::ButtonUnknown,    // 0xC3 Reserved
	InputButton::ButtonUnknown,    // 0xC4 Reserved
	InputButton::ButtonUnknown,    // 0xC5 Reserved
	InputButton::ButtonUnknown,    // 0xC6 Reserved
	InputButton::ButtonUnknown,    // 0xC7 Reserved
	InputButton::ButtonUnknown,    // 0xC8 Reserved
	InputButton::ButtonUnknown,    // 0xC9 Reserved
	InputButton::ButtonUnknown,    // 0xCA Reserved
	InputButton::ButtonUnknown,    // 0xCB Reserved
	InputButton::ButtonUnknown,    // 0xCC Reserved
	InputButton::ButtonUnknown,    // 0xCD Reserved
	InputButton::ButtonUnknown,    // 0xCE Reserved
	InputButton::ButtonUnknown,    // 0xCF Reserved
	InputButton::ButtonUnknown,    // 0xD0 Reserved
	InputButton::ButtonUnknown,    // 0xD1 Reserved
	InputButton::ButtonUnknown,    // 0xD2 Reserved
	InputButton::ButtonUnknown,    // 0xD3 Reserved
	InputButton::ButtonUnknown,    // 0xD4 Reserved
	InputButton::ButtonUnknown,    // 0xD5 Reserved
	InputButton::ButtonUnknown,    // 0xD6 Reserved
	InputButton::ButtonUnknown,    // 0xD7 Reserved
	InputButton::ButtonUnknown,    // 0xD8
	InputButton::ButtonUnknown,    // 0xD9
	InputButton::ButtonUnknown,    // 0xDA
	InputButton::KeyOEM_4,		  // 0xDB VK_OEM_4 ([{)
	InputButton::KeyOEM_5,         // 0xDC VK_OEM_5 (\|)
	InputButton::KeyOEM_6,		  // 0xDD VK_OEM_6 (]})
	InputButton::KeyOEM_7,		  // 0xDE VK_OEM_7 ('")
	InputButton::KeyOEM_8,	      // 0xDF VK_OEM_8 (squiggly thing, !)
	InputButton::ButtonUnknown,    // 0xE0 Reserved
	InputButton::ButtonUnknown,    // 0xE1 VK_OEM_AX. Not handled
	InputButton::KeyOEM_102,       // 0xE2 VK_OEM_102 (><)
	InputButton::ButtonUnknown,    // 0xE3 VK_ICO_HELP
	InputButton::ButtonUnknown,    // 0xE4 VK_ICO_00
	InputButton::ButtonUnknown,    // 0xE5 VK_PROCESSKEY
	InputButton::ButtonUnknown,    // 0xE6 VK_ICO_CLEAR
	InputButton::ButtonUnknown,    // 0xE7 VK_PACKET
	InputButton::ButtonUnknown,    // 0xE8
	InputButton::ButtonUnknown,    // 0xE9
	InputButton::ButtonUnknown,    // 0xEA
	InputButton::ButtonUnknown,    // 0xEB
	InputButton::ButtonUnknown,    // 0xEC
	InputButton::ButtonUnknown,    // 0xED
	InputButton::ButtonUnknown,    // 0xEE
	InputButton::ButtonUnknown,    // 0xEF
	InputButton::ButtonUnknown,    // 0xF0
	InputButton::ButtonUnknown,    // 0xF1
	InputButton::ButtonUnknown,    // 0xF2
	InputButton::ButtonUnknown,    // 0xF3
	InputButton::ButtonUnknown,    // 0xF4
	InputButton::ButtonUnknown,    // 0xF5
	InputButton::ButtonUnknown,    // 0xF6 VK_ATTN
	InputButton::ButtonUnknown,    // 0xF7 VK_CRSEL
	InputButton::ButtonUnknown,    // 0xF8 VK_EXSEL
	InputButton::ButtonUnknown,    // 0xF9 VK_EREOF
	InputButton::ButtonUnknown,    // 0xFA VK_PLAY
	InputButton::ButtonUnknown,    // 0xFB VK_ZOOM
	InputButton::ButtonUnknown,    // 0xFC VK_NONAME
	InputButton::ButtonUnknown,    // 0xFD VK_PA1
	InputButton::ButtonUnknown,    // 0xFE VK_OEM_CLEAR
	InputButton::ButtonUnknown,    // 0xFF
};

// TODO: Eliminate usages of globals here, move them into InputManager.

/**
 * Mapping from Seoul button codes to virtual key codes, i.e. the inverse of
 * the map above.  This is initialized at runtime.
 */
static UInt s_aInverseVirtualKeyMap[(Int)InputButton::ButtonUnknown + 1];

/** Name of the json section where we store input bindings */
const String InputManager::s_kInputBindingsJsonSection = "InputBindings";

/** "Button pressed" event ID */
const HString g_EventButtonEvent("Input.ButtonEvent");

/** "Axis changed" event ID */
const HString g_EventAxisEvent("Input.AxisEvent");

/** "Mouse moved" event ID */
const HString g_MouseMoveEvent("Input.MouseEvent");

/**
 * Structure for mapping button IDs to names and human-readable names
 */
struct InputButtonName
{
	InputButton m_ID;
	char const* m_sName;
	char const* m_sHumanName;
};

// Macro to simplify these definitions
#define KEY(key, humanName) {InputButton::Key ## key, #key, humanName}
#define BUTTON(button, humanName) {InputButton::button, #button, humanName}

// Note: this list must be kept in sync with the InputButton enum in InputKeys.h

/**
 * List of all known input buttons
 *
 * List of all known input buttons.  This array maps input button IDs to
 * human-readable names.
 */
const InputButtonName g_aInputButtonNames[] =
{
	// Keyboard keys
	KEY(LeftShift,    "LeftShift"),
	KEY(RightShift,   "RightShift"),
	KEY(LeftControl,  "LeftControl"),
	KEY(RightControl, "RightControl"),
	KEY(LeftAlt,      "LeftAlt"),
	KEY(RightAlt,     "RightAlt"),

	KEY(A, "A"),
	KEY(B, "B"),
	KEY(C, "C"),
	KEY(D, "D"),
	KEY(E, "E"),
	KEY(F, "F"),
	KEY(G, "G"),
	KEY(H, "H"),
	KEY(I, "I"),
	KEY(J, "J"),
	KEY(K, "K"),
	KEY(L, "L"),
	KEY(M, "M"),
	KEY(N, "N"),
	KEY(O, "O"),
	KEY(P, "P"),
	KEY(Q, "Q"),
	KEY(R, "R"),
	KEY(S, "S"),
	KEY(T, "T"),
	KEY(U, "U"),
	KEY(V, "V"),
	KEY(W, "W"),
	KEY(X, "X"),
	KEY(Y, "Y"),
	KEY(Z, "Z"),

	KEY(0, "0"),
	KEY(1, "1"),
	KEY(2, "2"),
	KEY(3, "3"),
	KEY(4, "4"),
	KEY(5, "5"),
	KEY(6, "6"),
	KEY(7, "7"),
	KEY(8, "8"),
	KEY(9, "9"),

	KEY(Space,        "Space"),

	KEY(Grave,        "Grave"),
	KEY(Minus,        "Minus"),
	KEY(Equals,       "Equals"),
	KEY(LeftBracket,  "LeftBracket"),
	KEY(RightBracket, "RightBracket"),
	KEY(Backslash,    "Backslash"),
	KEY(Semicolon,    "Semicolon"),
	KEY(Quote,        "Quote"),
	KEY(Comma,        "Comma"),
	KEY(Period,       "Period"),
	KEY(Slash,        "Slash"),
	KEY(OEM_102,	  "LessThan"),
	KEY(OEM_8,		  "Exlamation"),

	KEY(F1,  "F1"),
	KEY(F2,  "F2"),
	KEY(F3,  "F3"),
	KEY(F4,  "F4"),
	KEY(F5,  "F5"),
	KEY(F6,  "F6"),
	KEY(F7,  "F7"),
	KEY(F8,  "F8"),
	KEY(F9,  "F9"),
	KEY(F10, "F10"),
	KEY(F11, "F11"),
	KEY(F12, "F12"),
	KEY(F13, "F13"),
	KEY(F14, "F14"),
	KEY(F15, "F15"),
	KEY(F16, "F16"),
	KEY(F17, "F17"),
	KEY(F18, "F18"),
	KEY(F19, "F19"),
	KEY(F20, "F20"),
	KEY(F21, "F21"),
	KEY(F22, "F22"),
	KEY(F23, "F23"),
	KEY(F24, "F24"),

	KEY(Escape,       "Escape"),
	KEY(Tab,          "Tab"),
	KEY(CapsLock,     "CapsLock"),
	KEY(Backspace,    "Backspace"),
	KEY(Enter,        "Enter"),
	KEY(LeftWindows,  "LeftWindows"),
	KEY(RightWindows, "RightWindows"),
	KEY(AppMenu,      "ApplicationMenu"),

	KEY(Insert,   "Insert"),
	KEY(Delete,   "Delete"),
	KEY(Home,     "Home"),
	KEY(End,      "End"),
	KEY(PageUp,   "PageUp"),
	KEY(PageDown, "PageDown"),

	KEY(Up,    "Up"),    // U+2191 = Upwards arrow
	KEY(Down,  "Down"),  // U+2193 = Downwards arrow
	KEY(Left,  "Left"),  // U+2190 = Leftwards arrow
	KEY(Right, "Right"), // U+2192 = Rightwards arrow

	KEY(PrintScreen, "PrintScreen"),
	KEY(ScrollLock,  "ScrollLock"),
	KEY(Pause,       "Pause"),

	KEY(NumLock, "NumLock"),
	KEY(Numpad0, "Numpad0"),
	KEY(Numpad1, "Numpad1"),
	KEY(Numpad2, "Numpad2"),
	KEY(Numpad3, "Numpad3"),
	KEY(Numpad4, "Numpad4"),
	KEY(Numpad5, "Numpad5"),
	KEY(Numpad6, "Numpad6"),
	KEY(Numpad7, "Numpad7"),
	KEY(Numpad8, "Numpad8"),
	KEY(Numpad9, "Numpad9"),

	KEY(NumpadPlus,   "NumpadPlus"),
	KEY(NumpadMinus,  "NumpadMinus"),
	KEY(NumpadTimes,  "NumpadTimes"),
	KEY(NumpadDivide, "NumpadDivide"),
	KEY(NumpadEnter,  "NumpadEnter"),
	KEY(NumpadDelete, "NumpadDelete"),

	KEY(BrowserBack,    "BrowserBack"),
	KEY(BrowserForward, "BrowserForward"),

	KEY(VolumeDown, "VolumeDown"),
	KEY(VolumeUp,   "VolumeUp"),

	// Mouse buttons
	BUTTON(MouseLeftButton,   "MouseLeftButton"),
	BUTTON(MouseRightButton,  "MouseRightButton"),
	BUTTON(MouseMiddleButton, "MouseMiddleButton"),
	BUTTON(MouseButton4,      "MouseButton4"),
	BUTTON(MouseButton5,      "MouseButton5"),
	BUTTON(MouseButton6,      "MouseButton6"),
	BUTTON(MouseButton7,      "MouseButton7"),
	BUTTON(MouseButton8,      "MouseButton8"),

	// Touch inputs
	BUTTON(TouchButton1, "TouchButton1"),
	BUTTON(TouchButton2, "TouchButton2"),
	BUTTON(TouchButton3, "TouchButton3"),
	BUTTON(TouchButton4, "TouchButton4"),
	BUTTON(TouchButton5, "TouchButton5"),

	// Xbox 360 controller buttons
	BUTTON(XboxSectionStart,		  ""),
	BUTTON(XboxA,                     "A"),
	BUTTON(XboxB,                     "B"),
	BUTTON(XboxX,                     "X"),
	BUTTON(XboxY,                     "Y"),
	BUTTON(XboxLeftBumper,            "LeftBumper"),
	BUTTON(XboxRightBumper,           "RightBumper"),
	BUTTON(XboxLeftTrigger,           "LeftTrigger"),
	BUTTON(XboxRightTrigger,          "RightTrigger"),
	BUTTON(XboxStart,                 "Start"),
	BUTTON(XboxBack,                  "Back"),
	BUTTON(XboxGuide,                 "Guide"),
	BUTTON(XboxLeftThumbstickButton,  "LeftThumbstickClick"),
	BUTTON(XboxRightThumbstickButton, "RightThumbstickClick"),
	BUTTON(XboxDpadUp,                "Up"),
	BUTTON(XboxDpadDown,              "Down"),
	BUTTON(XboxDpadLeft,              "Left"),
	BUTTON(XboxDpadRight,             "Right"),
	BUTTON(XboxSectionEnd,			  ""),

	// PlayStation 3 controller buttons
	BUTTON(PS3SectionStart, ""),
	BUTTON(PS3X,         "X"),         // U+2715 = Multiplication X
	BUTTON(PS3Square,    "Square"),    // U+25A1 = White square, U+2B1C = White large square
	BUTTON(PS3Circle,    "Circle"),    // U+25CB = White circle, U+25EF = Large circle
	BUTTON(PS3Triangle,  "Triangle"),  // U+25B3 = White up-pointing triangle
	BUTTON(PS3L1,        "L1"),
	BUTTON(PS3R1,        "R1"),
	BUTTON(PS3L2,        "L2"),
	BUTTON(PS3R2,        "R2"),
	BUTTON(PS3L3,        "L3"),
	BUTTON(PS3R3,        "R3"),
	BUTTON(PS3Start,     "Start"),
	BUTTON(PS3Select,    "Select"),
	BUTTON(PS3PS,        "PS"),
	BUTTON(PS3DpadUp,    "Up"),
	BUTTON(PS3DpadDown,  "Down"),
	BUTTON(PS3DpadLeft,  "Left"),
	BUTTON(PS3DpadRight, "Right"),
	BUTTON(PS3SectionEnd, ""),

	// Wiimote & Nunchuck buttons
	BUTTON(WiiSectionStart, ""),
	BUTTON(WiiA,         "A"),
	BUTTON(WiiB,         "B"),
	BUTTON(Wii1,         "1"),
	BUTTON(Wii2,         "2"),
	BUTTON(WiiPlus,      "Plus"),
	BUTTON(WiiMinus,     "Minus"),
	BUTTON(WiiHome,      "Home"),
	BUTTON(WiiDpadUp,    "Up"),
	BUTTON(WiiDpadDown,  "Down"),
	BUTTON(WiiDpadLeft,  "Left"),
	BUTTON(WiiDpadRight, "Right"),
	BUTTON(WiiNunchuckC, "C"),
	BUTTON(WiiNunchuckZ, "Z"),
	BUTTON(WiiSectionEnd, ""),

	// Generic gamepad buttons
	BUTTON(GamepadButton1,  "Gamepad1"),
	BUTTON(GamepadButton2,  "Gamepad2"),
	BUTTON(GamepadButton3,  "Gamepad3"),
	BUTTON(GamepadButton4,  "Gamepad4"),
	BUTTON(GamepadButton5,  "Gamepad5"),
	BUTTON(GamepadButton6,  "Gamepad6"),
	BUTTON(GamepadButton7,  "Gamepad7"),
	BUTTON(GamepadButton8,  "Gamepad8"),
	BUTTON(GamepadButton9,  "Gamepad9"),
	BUTTON(GamepadButton10, "Gamepad10"),
	BUTTON(GamepadButton11, "Gamepad11"),
	BUTTON(GamepadButton12, "Gamepad12"),
	BUTTON(GamepadButton13, "Gamepad13"),
	BUTTON(GamepadButton14, "Gamepad14"),
	BUTTON(GamepadButton15, "Gamepad15"),
	BUTTON(GamepadButton16, "Gamepad16"),
	BUTTON(GamepadButton17, "Gamepad17"),
	BUTTON(GamepadButton18, "Gamepad18"),
	BUTTON(GamepadButton19, "Gamepad19"),
	BUTTON(GamepadButton20, "Gamepad20"),

	// Unknown button - this must be the last element in this array
	BUTTON(ButtonUnknown, "<Unknown Button>"),
};

#undef KEY
#undef BUTTON

/** Helper struct that defines a special key bit vector to
 *  string name mapping.
 */
struct SpecialKeyName
{
	UInt32 m_SpecialKeyFlags;
	char const* m_sName;
};

/** Array of special key to name mapping entries.
 *  This array is used by InputManager to match
 *  string values in JSON files to special key bit flags.
 */
static const SpecialKeyName g_aSpecialKeyNames[] =
{
	{ InputManager::kLeftShift, "Left Shift" },
	{ InputManager::kRightShift, "Right Shift" },
	{ InputManager::kLeftControl, "Left Ctrl" },
	{ InputManager::kRightControl, "Right Ctrl" },
	{ InputManager::kLeftControl, "Left Control" },
	{ InputManager::kRightControl, "Right Control" },
	{ InputManager::kLeftAlt, "Left Alt" },
	{ InputManager::kRightAlt, "Right Alt" },

	// Some extras for simplicity sake when specifying bindings.
	{ InputManager::kLeftShift | InputManager::kRightShift, "Shift" },
	{ InputManager::kLeftControl | InputManager::kRightControl, "Control" },
	{ InputManager::kLeftControl | InputManager::kRightControl, "Ctrl" },
	{ InputManager::kLeftAlt | InputManager::kRightAlt, "Alt" },
};

#define AXIS(axis, humanName) {InputAxis::axis, #axis, humanName}

/**
 * Structure mapping axis IDs to names and human-readable names
 */
struct InputAxisName
{
	InputAxis m_ID;
	char const* m_sName;
	char const* m_sHumanName;
};

/**
 * List of all known input axes
 *
 * List of all known input axes.  This array maps input axis IDs to
 * human-readable names.
 */
const InputAxisName g_aInputAxisNames[] =
{
	// Mouse axes
	AXIS(MouseX, "Mouse"),
	AXIS(MouseY, "Mouse"),
	AXIS(MouseWheel, "MouseWheel"),

	// Touch axes
	AXIS(Touch1X, "Touch1X"), AXIS(Touch1Y, "Touch1Y"),
	AXIS(Touch2X, "Touch2X"), AXIS(Touch2Y, "Touch2Y"),
	AXIS(Touch3X, "Touch3X"), AXIS(Touch3Y, "Touch3Y"),
	AXIS(Touch4X, "Touch4X"), AXIS(Touch4Y, "Touch4Y"),
	AXIS(Touch5X, "Touch5X"), AXIS(Touch5Y, "Touch5Y"),

	// Xbox 360 controller axes
	AXIS(XboxLeftThumbstickX,  "Left Thumbstick"),
	AXIS(XboxLeftThumbstickY,  "Left Thumbstick"),
	AXIS(XboxRightThumbstickX,  "Right Thumbstick"),
	AXIS(XboxRightThumbstickY, "Right Thumbstick"),

	// PlayStation 3 controller axes
	AXIS(PS3LeftThumbstickX,  "Left Thumbstick"),
	AXIS(PS3LeftThumbstickY,  "Left Thumbstick"),
	AXIS(PS3RightThumbstickX, "Right Thumbstick"),
	AXIS(PS3RightThumbstickY, "Right Thumbstick"),

	// Wiimote & Nunchuck axes
	AXIS(WiiAccelerationX,         "Wiimote Accelerometer"),
	AXIS(WiiAccelerationY,         "Wiimote Accelerometer"),
	AXIS(WiiAccelerationZ,         "Wiimote Accelerometer"),
	AXIS(WiiNunchuckThumbstickX,   "Nunchuck Thumbstick"),
	AXIS(WiiNunchuckThumbstickY,   "Nunchuck Thumbstick"),
	AXIS(WiiNunchuckAccelerationX, "Nunchuck Accelerometer"),
	AXIS(WiiNunchuckAccelerationY, "Nunchuck Accelerometer"),
	AXIS(WiiNunchuckAccelerationZ, "Nunchuck Accelerometer"),

	// Generic game pad axes
	AXIS(GamepadLeftThumbstickX,  "Left Thumbstick"),
	AXIS(GamepadLeftThumbstickY,  "Left Thumbstick"),
	AXIS(GamepadRightThumbstickX, "Right Thumbstick"),
	AXIS(GamepadRightThumbstickY, "Right Thumbstick"),
	AXIS(GamepadAxis5, "Gamepad Axis 5"),
	AXIS(GamepadAxis6, "Gamepad Axis 6"),
	AXIS(GamepadAxis7, "Gamepad Axis 7"),
	AXIS(GamepadAxis8, "Gamepad Axis 8"),

	// Unknown axis - this must be the last element in this array
	AXIS(AxisUnknown, "<Unknown Axis>"),
};

#undef AXIS

/** @return True if text editing is currently active, false otherwise. */
static inline Bool HasInputBindingLock()
{
	return (Engine::Get() && nullptr != Engine::Get()->GetTextEditable());
}

/**
 * Input system constructor
 */
InputManager::InputManager()
	: m_bInitialized(false)
	, m_nPendingForceRescan(1)
	, m_bForceRescan(false)
	, m_PreviousMousePosition(0, 0)
	, m_MousePosition(0, 0)
	, m_tManualBindingEvents()
	, m_fLeftStickDeadZone(0.f)
	, m_fRightStickDeadZone(0.f)
	, m_fTriggerDeadZone(0.f)
	, m_nSystemBindingLock(0)
	, m_pDeviceConnectionChangedCallback(nullptr)
	, m_SpecialKeyFlags(0)
{
}

/**
* Input instance destructor
*
* Input system destructor.  Shuts down the input system if it has not yet been
* shut down.
*/
InputManager::~InputManager()
{
	SEOUL_ASSERT(m_bInitialized);

	if (m_bInitialized)
	{
		ClearBindings();
		SafeDeleteVector(m_vInputDevices);

		memset(s_aInverseVirtualKeyMap, 0, sizeof(s_aInverseVirtualKeyMap));

		m_bInitialized = false;
	}
}

/**
 * Registers input events/callbacks and loads input bindings.
 */
void InputManager::Initialize()
{
	SEOUL_ASSERT(!m_bInitialized);

	// Initialize inverse virtual key map
	for (UInt i = 0; i < SEOUL_ARRAY_COUNT(g_aVirtualKeyMap); i++)
	{
		InputButton eButton = g_aVirtualKeyMap[i];
		if (eButton != InputButton::ButtonUnknown)
		{
			// Check that the mapping is valid and that we don't have any
			// duplicate mappings
			SEOUL_ASSERT((UInt)eButton < SEOUL_ARRAY_COUNT(s_aInverseVirtualKeyMap));
			SEOUL_ASSERT(s_aInverseVirtualKeyMap[(Int)eButton] == 0);
			s_aInverseVirtualKeyMap[(Int)eButton] = i;
		}
	}

	SEOUL_ASSERT(Events::Manager::Get().IsValid());
	Events::Manager::Get()->RegisterCallback(g_EventButtonEvent, SEOUL_BIND_DELEGATE(&InputManager::InternalHandleButtonEvent, this));
	LoadBindingsFromJson();

	m_bInitialized = true;
}

/**
* Ticks all input devices
*
* Ticks all input devices and updates their internal states.  This mostly does
* things like generate button repeat events for buttons which have been held
* down.
*
* @param fTime Amount of time to tick devices by
*/
void InputManager::Tick(Float fTime)
{
	// Clear manual binding events from the previous tick.
	m_tManualBindingEvents.Clear();

	// Thread safe update of the m_bForceRescan member.
	Atomic32Type currentValue = m_nPendingForceRescan;
	m_bForceRescan = (0 != currentValue);
	while (currentValue != m_nPendingForceRescan.CompareAndSet(0, currentValue))
	{
		// Either we saw a 1, which changed to a 0 before we could clear it, or we
		// saw a 0, which changed to a 1 before we could clear it. Either way we want
		// to force a rescan.
		m_bForceRescan = true;
		currentValue = m_nPendingForceRescan;
	}

	for (UInt nDevice = 0; nDevice < m_vInputDevices.GetSize(); nDevice++)
	{
		m_vInputDevices[nDevice]->Poll();

		if (m_pDeviceConnectionChangedCallback &&
			m_vInputDevices[nDevice]->IsConnected() != m_vInputDevices[nDevice]->WasConnected())
		{
			m_pDeviceConnectionChangedCallback(m_vInputDevices[nDevice]);
		}
	}

	// If the Input system binding lock is enabled, do not dispatch mouse move events.
	if (!InputManager::Get()->HasSystemBindingLock())
	{
		m_PreviousMousePosition = m_MousePosition;
		auto pMouse = FindFirstMouseDevice();
		if (nullptr != pMouse)
		{
			m_MousePosition = pMouse->GetMousePosition();
		}
		else
		{
			m_MousePosition = Point2DInt(0, 0);
		}

		if (m_MousePosition != m_PreviousMousePosition)
		{
			Events::Manager::Get()->TriggerEvent(
				g_MouseMoveEvent,
				m_MousePosition.X,
				m_MousePosition.Y);
		}
	}

	for (UInt nDevice = 0; nDevice < m_vInputDevices.GetSize(); nDevice++)
	{
		m_vInputDevices[nDevice]->Tick(fTime);
	}
}

/**
 * Converts a virtual key code into a Seoul button enum
 */
InputButton InputManager::GetInputButtonForVKCode(UInt uVKCode)
{
	SEOUL_ASSERT(uVKCode < SEOUL_ARRAY_COUNT(g_aVirtualKeyMap));
	return g_aVirtualKeyMap[uVKCode];
}

/**
 * Converts a Seoul button enum into a virtual key code
 */
UInt InputManager::GetVKCodeForInputButton(InputButton eButton)
{
	SEOUL_ASSERT((UInt)eButton < SEOUL_ARRAY_COUNT(s_aInverseVirtualKeyMap));
	return s_aInverseVirtualKeyMap[(Int)eButton];
}

/**
 * Loads input bindings from an JSON file
 *
 * Loads input bindings from an JSON file.
 */
void InputManager::LoadBindingsFromJson()
{
	// Flush existing state prior to load.
	ClearBindings();

	// Apply defaults.
	if (!InternalLoadBindingsFromJson(GamePaths::Get()->GetInputJsonFilePath()))
	{
		SEOUL_WARN("Error loading \"%s\".\n", GamePaths::Get()->GetInputJsonFilePath().CStr());
	}

	// Now apply user configuration.
	(void)InternalLoadBindingsFromJson(GetInputConfigFilePath());
}

Bool InputManager::InternalLoadBindingsFromJson(FilePath filePath)
{
	SharedPtr<DataStore> pDataStore(SettingsManager::Get()->WaitForSettings(filePath));
	if (!pDataStore.IsValid())
	{
		return false;
	}

	DataStoreTableUtil settingsSection(*pDataStore, ksInputSettings);

	// Read overall configuration values.
	(void)settingsSection.GetValue<Float>(ksLeftStickDeadZone, m_fLeftStickDeadZone);
	(void)settingsSection.GetValue<Float>(ksRightStickDeadZone, m_fRightStickDeadZone);
	(void)settingsSection.GetValue<Float>(ksTriggerDeadZone, m_fTriggerDeadZone);

	// Load all of the button bindings
	{
		typedef Vector<String> StringVector;

		DataStoreTableUtil bindingsSection(*pDataStore, ksInputButtonBindings);
		auto const iBegin = bindingsSection.Begin();
		auto const iEnd = bindingsSection.End();

		// First enumerate, and remove any entries that exist in the current config
		// that we added in the previous config. We want to override them completely,
		// not append.
		for (auto i = iBegin; iEnd != i; ++i)
		{
			// Skip null entries, these are removes generated by the diff.
			if (i->Second.IsNull())
			{
				continue;
			}

			HString const sButton(i->First);
			StringVector vsBindings;
			(void)bindingsSection.GetValue(sButton, vsBindings);

			auto const iBindingsBegin = vsBindings.Begin();
			auto const iBindingsEnd = vsBindings.End();
			for (auto i = iBindingsBegin; iBindingsEnd != i; ++i)
			{
				HString const key(*i);
				ButtonVector* p = nullptr;
				(void)m_tBindingButtonMap.GetValue(key, p);
				(void)m_tBindingButtonMap.Erase(key);
				SafeDelete(p);
			}
		}

		// Vector to cache tokens parsed out of a key binding name
		// entry.
		Vector<String> vsButtonTokens;

		// For each line of the InputBindings section of input.json
		for (auto i = iBegin; iEnd != i; ++i)
		{
			// Skip null entries, these are removes generated by the diff.
			if (i->Second.IsNull())
			{
				continue;
			}

			HString const sButton(i->First);
			StringVector vsBindings;
			if (!bindingsSection.GetValue(sButton, vsBindings))
			{
				SEOUL_WARN("Bad input bindings value for binding '%s'\n", i->First.CStr());
				continue;
			}

			// Split the button string on the + delimiter. Bindings with
			// modifiers will have several entries, regular bindings will
			// only have one.
			vsButtonTokens.Clear();
			SplitString(String(i->First), '+', vsButtonTokens);

			// Ensure there's at least one button specified
			if (vsButtonTokens.IsEmpty())
			{
				SEOUL_WARN("Bad input binding: empty\n");
				continue;
			}

			// Key the buttonID and any special key modifiers.
			// If the binding has no special keys, specialKeys
			// will be 0.
			InputButton buttonID = GetButtonID(vsButtonTokens);
			UInt32 specialKeys = GetSpecialKeys(vsButtonTokens);

			// Check that a valid button was specified
			if (buttonID == InputButton::ButtonUnknown)
			{
				SEOUL_WARN("Found binding for unknown button: %s = %s\n",
					sButton.CStr(), vsBindings[0].CStr());
				continue;
			}

			// If the button itself is a modifier key, then make sure that we
			// include itself as a modifier so that button presses register
			// properly
			if (buttonID == InputButton::KeyLeftShift || buttonID == InputButton::KeyRightShift)
			{
				specialKeys |= kLeftShift | kRightShift;
			}
			else if (buttonID == InputButton::KeyLeftControl || buttonID == InputButton::KeyRightControl)
			{
				specialKeys |= kLeftControl | kRightControl;
			}
			else if (buttonID == InputButton::KeyLeftAlt || buttonID == InputButton::KeyRightAlt)
			{
				specialKeys |= kLeftAlt | kRightAlt;
			}

			auto const iBindingsBegin = vsBindings.Begin();
			auto const iBindingsEnd = vsBindings.End();
			for (auto i = iBindingsBegin; iBindingsEnd != i; ++i)
			{
				HString const binding = HString(*i);
				if (!binding.IsEmpty())
				{
					// Each binding has a list of buttons; append this button to the list
					ButtonVector* pButtons = nullptr;
					if (!m_tBindingButtonMap.GetValue(binding, pButtons))
					{
						pButtons = SEOUL_NEW(MemoryBudgets::Input) ButtonVector;
					}

					pButtons->PushBack(InputButtonPlusModifier::Create(buttonID, specialKeys));
					m_tBindingButtonMap.Insert(binding, pButtons);
				}
			}
		}
	}

	{
		typedef Vector<String> StringVector;

		// Load all of the axis bindings
		DataStoreTableUtil bindingsSection(*pDataStore, ksInputAxisBindings);
		auto const iBegin = bindingsSection.Begin();
		auto const iEnd = bindingsSection.End();

		// First enumerate, and remove any entries that exist in the current config
		// that we added in the previous config. We want to override them completely,
		// not append.
		for (auto i = iBegin; iEnd != i; ++i)
		{
			HString const sAxis(i->First);
			StringVector vsBindings;
			(void)bindingsSection.GetValue(sAxis, vsBindings);

			auto const iBindingsBegin = vsBindings.Begin();
			auto const iBindingsEnd = vsBindings.End();
			for (auto i = iBindingsBegin; iBindingsEnd != i; ++i)
			{
				HString const key(*i);
				InputAxes* p = nullptr;
				(void)m_tBindingAxisMap.GetValue(key, p);
				(void)m_tBindingAxisMap.Erase(key);
				SafeDelete(p);
			}
		}

		// For each line of the InputBindings section of input.json
		for (auto i = iBegin; iEnd != i; ++i)
		{
			String const sAxis(i->First);
			StringVector vsBindings;
			if (!bindingsSection.GetValue(i->First, vsBindings))
			{
				SEOUL_WARN("Bad input axis bindings value for binding '%s'\n", i->First.CStr());
				continue;
			}

			InputAxis axisID = GetAxisID(sAxis);

			// Check that a valid axis was specified
			if (axisID == InputAxis::AxisUnknown)
			{
				SEOUL_WARN("Found binding for unknown axis: %s = %s\n",
					sAxis.CStr(), vsBindings[0].CStr());
				continue;
			}

			auto const iBindingsBegin = vsBindings.Begin();
			auto const iBindingsEnd = vsBindings.End();
			for (auto i = iBindingsBegin; iBindingsEnd != i; ++i)
			{
				HString const binding = HString(*i);
				if (!binding.IsEmpty())
				{
					// Each binding has a list of axes; append this axis to the
					// list
					InputAxes* pAxes = nullptr;
					if (!m_tBindingAxisMap.GetValue(binding, pAxes))
					{
						pAxes = SEOUL_NEW(MemoryBudgets::Input) InputAxes;
					}

					pAxes->PushBack(axisID);
					m_tBindingAxisMap.Insert(binding, pAxes);
				}
			}
		}
	}

	{
		// Load all of the axis-to-button bindings
		DataStoreTableUtil bindingsSection(*pDataStore, ksInputAxisToButtonBindings);
		auto const iBegin = bindingsSection.Begin();
		auto const iEnd = bindingsSection.End();

		// For each line of the InputBindings section of input.json
		for (auto i = iBegin; iEnd != i; ++i)
		{
			// Parse the key into an axis name and a direction (up for positive,
			// down for negative)
			String const sAxisAndDir(i->First);
			AxisAndDirection axisAndDir;
			if (sAxisAndDir.EndsWith("_Up"))
			{
				axisAndDir.m_eAxis = GetAxisID(sAxisAndDir.Substring(0, sAxisAndDir.GetSize() - 3));
				axisAndDir.m_bPositive = true;
			}
			else if (sAxisAndDir.EndsWith("_Down"))
			{
				axisAndDir.m_eAxis = GetAxisID(sAxisAndDir.Substring(0, sAxisAndDir.GetSize() - 5));
				axisAndDir.m_bPositive = false;
			}
			else
			{
				SEOUL_WARN("Unknown axis-to-button key \"%s\".  Valid keys must end in \"_Up\" or \"_Down\"", sAxisAndDir.CStr());
				continue;
			}

			if (axisAndDir.m_eAxis == InputAxis::AxisUnknown)
			{
				SEOUL_WARN("Unknown axis for axis-to-button binding: %s", sAxisAndDir.CStr());
				continue;
			}

			// Parse the value into an array of binding names
			BindingVector vsBindings;
			if (!bindingsSection.GetValue(i->First, vsBindings))
			{
				SEOUL_WARN("Bad input axis to buttonbindings value for binding '%s'\n", i->First.CStr());
				continue;
			}

			// Insert the bindings into our hash table.
			SEOUL_VERIFY(m_BindingAxisToButtonMap.Overwrite(axisAndDir, vsBindings).Second);
		}
	}

	return true;
}

/**
* Enumerates over all input devices
*
* Enumerates over all input devices using the given platform-specific device enumerator.
*
* @param[in] pEnumerator Platform-specific input device enumerator
*/
void InputManager::EnumerateInputDevices(InputDeviceEnumerator *pEnumerator)
{
	pEnumerator->EnumerateDevices(m_vInputDevices);
}

/**
 * Clears out the input event binding map
 *
 * Clears out the input event binding map and deletes all internal pointers
 * associated with it.
 */
void InputManager::ClearBindings()
{
	// Clear buttons
	SafeDeleteTable(m_tBindingButtonMap);

	// Clear axes
	SafeDeleteTable(m_tBindingAxisMap);

	// Clear axes-to-buttons
	m_BindingAxisToButtonMap.Clear();
}

/**
 * Finds the number of devices of the specified type.
 *
 * @param eDeviceType The type of device.
 *
 * @return The number of devices of the specified type.
 */
UInt InputManager::GetNumDevices(InputDevice::Type eDeviceType) const
{
	UInt result = 0;
	auto iter = m_vInputDevices.Begin();
	for ( ; iter != m_vInputDevices.End(); ++iter )
	{
		InputDevice* pInputDevice = *iter;
		if (pInputDevice->GetDeviceType() == eDeviceType && pInputDevice->IsConnected())
		{
			result++;
		}
	}

	return result;
}

/** On the next tick, instructs us to search for newly connected devices.
 *
 * Needed on some platforms (PC) because scanning every frame is too expensive.
 * (We scan when we get an OS notification that system hardware changed.)
 *
 * On other platforms (Xbox), it is cheap and we automatically scan every frame,
 * so no need to call this.
 */
void InputManager::TriggerRescan()
{
	Atomic32Type currentValue = m_nPendingForceRescan;
	while (currentValue != m_nPendingForceRescan.CompareAndSet(1, currentValue))
	{
		currentValue = m_nPendingForceRescan;
	}
}

/**
 * When called, informs all registered input devices
 * that the game has lost focus. Typically, devices use
 * this to reset buttons to the "up" state.
 */
void InputManager::OnLostFocus()
{
	UInt32 const zSize = m_vInputDevices.GetSize();
	for (UInt32 i = 0u; i < zSize; ++i)
	{
		InputDevice* pDevice = m_vInputDevices[i];
		if (nullptr != pDevice)
		{
			pDevice->OnLostFocus();
		}
	}
}

void InputManager::UpdateDeadZonesForCurrentControllers()
{
	// update the dead zones for all currently connected controllers
	for (auto i = m_vInputDevices.Begin(); m_vInputDevices.End() != i; ++i)
	{
		auto pDevice = *i;
		if (pDevice->GetDeviceType() == InputDevice::Xbox360Controller)
		{
			pDevice->SetTwoAxisDeadZoneCircular(InputAxis::XboxLeftThumbstickX,  InputAxis::XboxLeftThumbstickY,  m_fLeftStickDeadZone);
			pDevice->SetTwoAxisDeadZoneCircular(InputAxis::XboxRightThumbstickX, InputAxis::XboxRightThumbstickY, m_fRightStickDeadZone);
			pDevice->SetZeroBasedAxisDeadZone(InputAxis::XboxLeftTriggerZ, m_fTriggerDeadZone);
			pDevice->SetZeroBasedAxisDeadZone(InputAxis::XboxRightTriggerZ, m_fTriggerDeadZone);
		}
		else if (pDevice->GetDeviceType() == InputDevice::GameController)
		{
			pDevice->SetTwoAxisDeadZoneCircular(InputAxis::GamepadLeftThumbstickX, InputAxis::GamepadLeftThumbstickY,  m_fLeftStickDeadZone);
			pDevice->SetTwoAxisDeadZoneCircular(InputAxis::GamepadRightThumbstickX, InputAxis::GamepadRightThumbstickY, m_fRightStickDeadZone);
			pDevice->SetAxisDeadZone(InputAxis::GamepadAxis5, m_fTriggerDeadZone);
			pDevice->SetAxisDeadZone(InputAxis::GamepadAxis6, m_fTriggerDeadZone);
		}
		else if (pDevice->GetDeviceType() == InputDevice::PS3Controller)
		{
			pDevice->SetTwoAxisDeadZoneCircular(InputAxis::PS3LeftThumbstickX,  InputAxis::PS3LeftThumbstickY,  m_fLeftStickDeadZone);
			pDevice->SetTwoAxisDeadZoneCircular(InputAxis::PS3RightThumbstickX, InputAxis::PS3RightThumbstickY, m_fRightStickDeadZone);
		}
		else if (pDevice->GetDeviceType() == InputDevice::PS3NavController)
		{
			pDevice->SetTwoAxisDeadZoneCircular(InputAxis::PS3LeftThumbstickX, InputAxis::PS3LeftThumbstickY,  m_fLeftStickDeadZone);
		}
	}
}


Bool InputManager::HasConnectedDevice(InputDevice::Type eDeviceType) const
{
	for (auto i = m_vInputDevices.Begin(); m_vInputDevices.End() != i; ++i)
	{
		auto pDevice = *i;
		if (pDevice->GetDeviceType() == eDeviceType && pDevice->IsConnected())
		{
			return true;
		}
	}

	return false;
}


/**
 * Find the first mouse in the device list for the given local user
 *
 * If no mouse is currently available, the result is nullptr.
 *
 * @return The first mouse device in the registered with the input manager
 */
MouseDevice* InputManager::FindFirstMouseDevice() const
{
	for (auto i = m_vInputDevices.Begin(); m_vInputDevices.End() != i; ++i)
	{
		auto pDevice = *i;
		if (pDevice->GetDeviceType() == InputDevice::Mouse)
		{
			return static_cast<MouseDevice*>(pDevice);
		}
	}

	return nullptr;
}

/**
 * Injects a keyboard button event into all attached keyboard devices
 */
void InputManager::QueueKeyboardEvent(UInt32 uVirtualKeyCode, Bool bPressed)
{
	for (auto i = m_vInputDevices.Begin(); m_vInputDevices.End() != i; ++i)
	{
		auto pDevice = *i;
		if (pDevice->GetDeviceType() == InputDevice::Keyboard)
		{
			pDevice->QueueKeyEvent(uVirtualKeyCode, bPressed);
		}
	}
}

/**
 * Injects a mouse button event into all attached mouse devices
 */
void InputManager::QueueMouseButtonEvent(InputButton eMouseButton, Bool bPressed)
{
	for (auto i = m_vInputDevices.Begin(); m_vInputDevices.End() != i; ++i)
	{
		auto pDevice = *i;
		if (pDevice->GetDeviceType() == InputDevice::Mouse)
		{
			pDevice->QueueMouseButtonEvent(eMouseButton, bPressed);
		}
	}
}

/**
 * Injects a mouse move event into all attached mouse devices
 */
void InputManager::QueueMouseMoveEvent(const Point2DInt& location)
{
	for (auto i = m_vInputDevices.Begin(); m_vInputDevices.End() != i; ++i)
	{
		auto pDevice = *i;
		if (pDevice->GetDeviceType() == InputDevice::Mouse)
		{
			pDevice->QueueMouseMoveEvent(location);
		}
	}
}

/**
 * Injects a mouse wheel event into all attached mouse devices
 */
void InputManager::QueueMouseWheelEvent(Int iDelta)
{
	for (auto i = m_vInputDevices.Begin(); m_vInputDevices.End() != i; ++i)
	{
		auto pDevice = *i;
		if (pDevice->GetDeviceType() == InputDevice::Mouse)
		{
			pDevice->QueueMouseWheelEvent(iDelta);
		}
	}
}

/**
 * Injects a touch press/release event into all attached touch devices
 */
void InputManager::QueueTouchButtonEvent(InputButton eTouchButton, Bool bPressed)
{
	for (auto i = m_vInputDevices.Begin(); m_vInputDevices.End() != i; ++i)
	{
		auto pDevice = *i;
		if (pDevice->GetDeviceType() == InputDevice::Mouse &&
			pDevice->IsMultiTouchDevice())
		{
			pDevice->QueueTouchButtonEvent(eTouchButton, bPressed);
		}
	}
}

/**
 * Injects a touch move event into all attached touch devices
 */
void InputManager::QueueTouchMoveEvent(InputButton eTouch, const Point2DInt& location)
{
	for (auto i = m_vInputDevices.Begin(); m_vInputDevices.End() != i; ++i)
	{
		auto pDevice = *i;
		if (pDevice->GetDeviceType() == InputDevice::Mouse &&
			pDevice->IsMultiTouchDevice())
		{
			pDevice->QueueTouchMoveEvent(eTouch, location);
		}
	}
}

/**
 * Maps a button name to a button ID.
 *
 * @param[in] sButtonName Name of the button to look up.
 *
 * @return ID of the corresponding button, or ButtonUnknown if no such button exists.
 */
InputButton InputManager::GetButtonID(const Vector<String>& vButtonTokens)
{
	SEOUL_ASSERT(!vButtonTokens.IsEmpty());
	for (UInt i = 0; i < SEOUL_ARRAY_COUNT(g_aInputButtonNames) - 1; i++)
	{
		// The button name must be the last token in a binding name.
		if (vButtonTokens.Back().CompareASCIICaseInsensitive(g_aInputButtonNames[i].m_sName) == 0
			|| vButtonTokens.Back().CompareASCIICaseInsensitive(g_aInputButtonNames[i].m_sHumanName) == 0)
		{
			return g_aInputButtonNames[i].m_ID;
		}
	}

	return InputButton::ButtonUnknown;
}

/**
 * Given a button string, extracts any special key modifiers in
 * that string and combines them into a single bitvector representing
 * those modifiers.
 *
 * Special keys are the keys ALT, CTRL, and SHIFT.
 */
UInt32 InputManager::GetSpecialKeys(const Vector<String>& vButtonTokens)
{
	UInt32 ret = 0u;

	// There needs to be at least two tokens for there to be any modifiers.
	if (vButtonTokens.GetSize() > 1u)
	{
		// We iterate through the entire array here, unlike GetButton, because
		// there is no invalid special key value.
		for (UInt i = 0; i < SEOUL_ARRAY_COUNT(g_aSpecialKeyNames); i++)
		{
			// The last token must always be the button, so we don't check it
			// as a modifier.
			for (UInt j = 0; j < vButtonTokens.GetSize() - 1; j++)
			{
				if (vButtonTokens[j].CompareASCIICaseInsensitive(g_aSpecialKeyNames[i].m_sName) == 0)
				{
					ret |= g_aSpecialKeyNames[i].m_SpecialKeyFlags;
				}
			}
		}
	}

	return ret;
}

 /**
 * Maps an axis name to a axis ID.
 *
 * @param[in] sAxisName Name of the axis to look up.
 *
 * @return ID of the corresponding axis, or AxisUnknown if no such button exists.
 */
InputAxis InputManager::GetAxisID(const String & sAxisName)
{
	for (UInt i = 0; i < SEOUL_ARRAY_COUNT(g_aInputAxisNames) - 1; i++)
	{
		if (sAxisName.CompareASCIICaseInsensitive(g_aInputAxisNames[i].m_sName) == 0)
		{
			return g_aInputAxisNames[i].m_ID;
		}
	}

	return InputAxis::AxisUnknown;
}

/**
 * Returns the String representation of the specified button.
 */
Byte const* InputManager::InputButtonToString(InputButton b)
{
	return g_aInputButtonNames[(Int)b].m_sHumanName;
}

/**
 * Helper function to determine if a binding is down
 *
 * @param[in] pDevice Pointer to the input device to check
 * @param[in] InputButton Button to be checked
 * @param[in] bIgnoreExtraModifiers Whether to ignore extra modifier keys that
 *            are being pressed
 *
 * @return True if the button is down, false otherwise
 */
Seoul::Bool CheckBindingIsDown(
	const InputDevice& device,
	const InputManager::InputButtonPlusModifier& button,
	Bool bIgnoreExtraModifiers)
{
	return
		device.IsButtonDown(button.m_eButton, true) &&
		InputManager::Get()->IsSpecialPressed(button.m_SpecialKeyFlags, bIgnoreExtraModifiers);
}

/**
 * Helper function to determine if a binding was pressed
 *
 * @param[in] pDevice Pointer to the input device to check
 * @param[in] InputButton Button to be checked
 * @param[in] bIgnoreExtraModifiers Whether to ignore extra modifier keys that
 *            are being pressed
 *
 * @return True if the button was pressed, false otherwise
 */
Bool CheckBindingWasPressed(
	const InputDevice& device,
	const InputManager::InputButtonPlusModifier& button,
	Bool bIgnoreExtraModifiers)
{
	return
		device.WasButtonPressed(button.m_eButton, true) &&
		InputManager::Get()->IsSpecialPressed(button.m_SpecialKeyFlags, bIgnoreExtraModifiers);
}

/**
 * Helper function to determine if a binding was released
 *
 * @param[in] pDevice Pointer to the input device to check
 * @param[in] InputButton Button to be checked
 * @param[in] bIgnoreExtraModifiers (Ignored)
 *
 * @return True if the button was released, false otherwise
 */
Bool CheckBindingWasReleased(
	const InputDevice& device,
	const InputManager::InputButtonPlusModifier& button,
	Bool bIgnoreExtraModifiers)
{
	// We do not check the modifier here. It only matters if the button was
	// released, not the modifier.
	return device.WasButtonReleased(button.m_eButton, true);
}

// Helper enum for the different types of input checks
enum BindingCheckType
{
	IsDown = 0,
	WasPressed,
	WasReleased
};
// Array of function pointers for the different types of input checks
typedef Bool (*BindingCheckFunctionPtr)(const InputDevice&, const InputManager::InputButtonPlusModifier&, Bool);
static const BindingCheckFunctionPtr FuncPtrs[] = {CheckBindingIsDown, CheckBindingWasPressed, CheckBindingWasReleased};

/**
 * Generalized state check of one or more keys or buttons bound to a
 * name
 *
 * This function takes an enum which specifies the type of check to perform for
 * the given binding & user id.  If none of the keys or buttons bound to the
 * name were pressed/released/held/etc. then this function returns false.
 *
 * @param[in] sBindingName A name, bound to one or more Keyboard keys or
 *            buttons, to test
 * @param[in] bIgnoreExtraModifiers Whether to ignore extra modifier keys that
 *            are being pressed
 * @param[in] uUserID ID of which user is being checked, or kAnyDevice.
 * @param[in] iCheckType Specifies which check to perform based on
 *            BindingCheckType. Used as an index into the list of function
 *            pointers.
 * @param[out] pWhichDevice If this function returns true and pWhichDevice is
 *             non-null, *pWhichDevice will be updated to point to the device
 *             which triggered the binding.
 *
 * @return True if one or more of the keys or buttons bound to bindingName
 * is currently pressed/released/held.
 */
Bool InputManager::CheckBinding(
		HString bindingName,
		Bool bIgnoreExtraModifiers,
		UInt iCheckType,
		InputDevice const** pWhichDevice) const
{
	ButtonVector* pButtons = nullptr;

	// get the button for the input string
	if (!m_tBindingButtonMap.GetValue(bindingName, pButtons))
	{
		return false;
	}

	// Cache buttons begin and end.
	auto const iButtonsBegin = pButtons->Begin();
	auto const iButtonsEnd = pButtons->End();

	// now check all of the buttons in all of the input devices in the list
	auto const iBegin = m_vInputDevices.Begin();
	auto const iEnd = m_vInputDevices.End();
	for (auto deviceIter = iBegin; iEnd != deviceIter; ++deviceIter)
	{
		const InputDevice& device = *(*deviceIter);
		for (auto buttonIter = iButtonsBegin;
			iButtonsEnd != buttonIter;
			++buttonIter)
		{
			if ((*FuncPtrs[iCheckType])(device, *buttonIter, bIgnoreExtraModifiers))
			{
				if (pWhichDevice)
				{
					*pWhichDevice = &device;
				}

				return true;
			}
		}
	}

	return false;
}

/**
 * @return A human readable string of the binding sBindingName, or the empty
 * string if sBindingName is not a valid binding name.
 */
String InputManager::BindingToString(HString bindingName) const
{
	auto pButtons = GetButtonsFromBinding(bindingName);
	if (nullptr == pButtons)
	{
		return String();
	}

	String sReturn;
	for (auto i = pButtons->Begin(); pButtons->End() != i; ++i)
	{
		if (pButtons->Begin() != i)
		{
			sReturn.Append(", ");
		}

		sReturn.Append(ButtonToString(*i));
	}

	return sReturn;
}

/**
 * @return A human readable string of the button+modifier button.
 */
String InputManager::ButtonToString(const InputButtonPlusModifier& button) const
{
	Bool const bAlt =
		kLeftAlt == (kLeftAlt & button.m_SpecialKeyFlags) ||
		kRightAlt == (kRightAlt & button.m_SpecialKeyFlags);
	Bool const bControl =
		kLeftControl == (kLeftControl & button.m_SpecialKeyFlags) ||
		kRightControl == (kRightControl & button.m_SpecialKeyFlags);
	Bool const bShift =
		kLeftShift == (kLeftShift & button.m_SpecialKeyFlags) ||
		kRightShift == (kRightShift & button.m_SpecialKeyFlags);

	String sReturn;
	if (bAlt) { sReturn.Append("ALT+"); }
	if (bControl) { sReturn.Append("CTRL+"); }
	if (bShift) { sReturn.Append("SHIFT+"); }
	sReturn.Append(InputButtonToString(button.m_eButton));
	return sReturn;
}

/**
 * Unset any button associated with the specified binding.
 *
 * @param[in] bindingName The name of the binding to clear.
 * @param[in] bSave Whether to immediately commit the changes to disk or not.
 */
void InputManager::ClearButtonForBinding(HString bindingName, Bool bSave /* = true */)
{
	ButtonVector* p = nullptr;
	(void)m_tBindingButtonMap.GetValue(bindingName, p);
	m_tBindingButtonMap.Erase(bindingName);
	SafeDelete(p);

	if (bSave)
	{
		SaveBindingsToUserConfig();
	}
}

/**
 * Override the specified button to the given binding name.
 * @param[in] bindingName  The name of the binding to modify.
 * @param[in] eButton  The button to assign.
 * @param[in] eSpecialKeys Modifier keys to associate.
 * @param[in] bSave Whether to immediately commit the changes to disk or not.
 */
void InputManager::OverrideButtonForBinding(HString bindingName, InputButton eButton, UInt32 uSpecialKeys, Bool bSave /* = true */)
{
	ButtonVector* pButtons = SEOUL_NEW(MemoryBudgets::Input) ButtonVector;
	pButtons->PushBack(InputButtonPlusModifier::Create(eButton, uSpecialKeys));

	ButtonVector* pOldButtons = nullptr;
	if (m_tBindingButtonMap.GetValue(bindingName, pOldButtons))
	{
		m_tBindingButtonMap.Erase(bindingName);
		SafeDelete(pOldButtons);
	}

	SEOUL_VERIFY(m_tBindingButtonMap.Insert(bindingName, pButtons).Second);

	if (bSave)
	{
		SaveBindingsToUserConfig();
	}
}

/**
 * Immediately commit the current input binding state to the user's config file.
 * Only values that different from the base state are committed.
 */
void InputManager::SaveBindingsToUserConfig()
{
	// TODO: Bindings other than button bindings.

	DataStore dataStore;
	dataStore.MakeTable();

	DataNode const root = dataStore.GetRootNode();

	// Button bindings.
	{
		// We need to reassembly these into a mapping from input to array of bindings.
		typedef Vector<HString, MemoryBudgets::Input> InputVector;
		typedef HashTable<String, InputVector, MemoryBudgets::Input> InputTable;
		InputTable t;

		auto const iBegin = m_tBindingButtonMap.Begin();
		auto const iEnd = m_tBindingButtonMap.End();

		for (auto i = iBegin; iEnd != i; ++i)
		{
			HString const binding = i->First;
			auto const iButtonBegin = i->Second->Begin();
			auto const iButtonEnd = i->Second->End();
			for (auto iButton = iButtonBegin; iButtonEnd != iButton; ++iButton)
			{
				String const sButton(ButtonToString(*iButton));

				auto const e = t.Insert(sButton, Vector<HString, MemoryBudgets::Input>());
				e.First->Second.PushBack(binding);
			}
		}

		// Serialize the table.
		if (!Reflection::SerializeObjectToTable(
			ContentKey(),
			dataStore,
			root,
			ksInputButtonBindings,
			&t))
		{
			SEOUL_WARN("Failed saving input bindings, failed serializing button bindings table.");
			return;
		}
	}

	SharedPtr<DataStore> pInputDataStore(SettingsManager::Get()->WaitForSettings(
		GamePaths::Get()->GetInputJsonFilePath()));
	if (pInputDataStore.IsValid())
	{
		// Generate a diff between the constructed DataStore and the default,
		// and replace dataStore with it.
		DataStore diff;
		if (ComputeDiff(*pInputDataStore, dataStore, diff))
		{
			dataStore.Swap(diff);
		}
	}

	// Save the file.
	FilePath const filePath(GetInputConfigFilePath());
	Content::LoadManager::Get()->TempSuppressSpecificHotLoad(filePath);
	if (!Reflection::SaveDataStore(dataStore, dataStore.GetRootNode(), filePath))
	{
		SEOUL_WARN("Failed saving input config. Check that \"%s\" is not read-only "
			"(checked out from source control).",
			filePath.GetAbsoluteFilenameInSource().CStr());
	}
}

/**
 * Tests if one or more keys or buttons bound to a name are currently
 * pressed
 *
 * Tests if one or more keys or buttons bound to a name are currently
 * pressed.  If none of the keys or buttons bound to the name are currently
 * pressed, the result is false.
 *
 * @param[in] sBindingName A name, bound to one or more Keyboard keys or
 *            buttons, to test
 * @param[in] bIgnoreExtraModifiers Whether extra modifier keys should be
 *            ignored.  If false, the exact set of modifiers required must be
 *            pressed.
 * @param[in] uUserID ID of which user is being checked, or kAnyDevice.
 * @param[out] pWhichDevice If this function returns true and pWhichDevice is
 *             non-null, *pWhichDevice will be updated to point to the device
 *             which triggered the binding.
 *
 * @return True if one or more of the keys or buttons bound to bindingName
 *         is currently pressed.  Always returns false if a binding lock is set
 *         and sBindingName is not equal to the locked binding.
 */
Bool InputManager::IsBindingDown(
	HString bindingName,
	Bool bIgnoreExtraModifiers,
	InputDevice const** pWhichDevice) const
{
	if (!HasSystemBindingLock() && !HasInputBindingLock())
	{
		return CheckBinding(
			bindingName,
			bIgnoreExtraModifiers,
			(UInt)(IsDown),
			pWhichDevice);
	}
	else
	{
		return false;
	}
}

/**
 * Tests if one or more keys or buttons bound to a name was pressed
 *        during the last tick
 *
 * Tests if one or more keys or buttons bound to a name was pressed during the
 * last tick. If none of the keys or buttons bound to the name was pressed, the
 * result is false.
 *
 * @param[in] sBindingName A name, bound to one or more Keyboard keys or
 *            buttons, to test
 * @param[in] bIgnoreExtraModifiers Whether extra modifier keys should be
 *            ignored.  If false, the exact set of modifiers required must be
 *            pressed.
 * @param[in] uUserID ID of which user is being checked, or kAnyDevice.
 * @param[out] pWhichDevice If this function returns true and pWhichDevice is
 *             non-null, *pWhichDevice will be updated to point to the device
 *             which triggered the binding.
 *
 * @return True if one or more of the keys or buttons bound to bindingName
 *         was pressed during the last tick.  Always returns false if a binding
 *         lock is set and sBindingName is not equal to the locked binding.
 */
Bool InputManager::WasBindingPressed(
	HString bindingName,
	Bool bIgnoreExtraModifiers,
	InputDevice const** pWhichDevice) const
{
	if (!HasSystemBindingLock() && !HasInputBindingLock())
	{
		// If a manual binding event is present, activate the binding immediately.
		if (m_tManualBindingEvents.HasValue(bindingName))
		{
			return true;
		}

		return CheckBinding(
			bindingName,
			bIgnoreExtraModifiers,
			(UInt)(WasPressed),
			pWhichDevice);
	}
	else
	{
		return false;
	}
}

/**
 * Tests if one or more keys or buttons bound to a name was released
 *        during the last tick
 *
 * Tests if one or more keys or buttons bound to a name was released during the
 * last tick. If none of the keys or buttons bound to the name was released, the
 * result is false.
 *
 * @param[in] sBindingName A name, bound to one or more Keyboard keys or
 *            buttons, to test
 * @param[in] uUserID ID of which user is being checked, or kAnyDevice.
 * @param[out] pWhichDevice If this function returns true and pWhichDevice is
 *             non-null, *pWhichDevice will be updated to point to the device
 *             which triggered the binding.
 *
 * @return True if one or more of the keys or buttons bound to bindingName
 *         was released during the last tick.  Always returns false if a
 *         binding lock is set and sBindingName is not equal to the locked
 *         binding.
 */
Bool InputManager::WasBindingReleased(
	HString bindingName,
	InputDevice const** pWhichDevice) const
{
	if (!HasSystemBindingLock() && !HasInputBindingLock())
	{
		// If a manual binding event is present, activate the binding immediately.
		if (m_tManualBindingEvents.HasValue(bindingName))
		{
			return true;
		}

		return CheckBinding(
			bindingName,
			false,
			(UInt)(WasReleased),
			pWhichDevice);
	}
	else
	{
		return false;
	}
}

/**
 * Manually inject a binding event - will cause WasBindingReleased and WasBindingPressed
 * to return true, once, for the specified event.
 */
void InputManager::ManuallyInjectBindingEvent(HString bindingName)
{
	SEOUL_ASSERT(IsMainThread());

	SEOUL_VERIFY(m_tManualBindingEvents.Overwrite(bindingName, true).Second);
}

/**
 * Returns the requested axis state for the specified user
 *
 * @param[in] sBindingName requested binding
 * @param[in] uUserID user ID, or kAnyDevice.
 * @param[out] pWhichDevice If this function returns nonzero and pWhichDevice is non-null,
 *  *pWhichDevice will be updated to point to the device which triggered the binding.
 *  If this function returns zero, *pWhichDevice will be updated to point to an arbitrary
 *  device with the given local user and binding name.
 *
 * @return Axis state
 */
Float InputManager::GetAxisState(
	HString bindingName,
	InputDevice const** pWhichDevice) const
{
	if (HasSystemBindingLock())
	{
		// Locked bindings don't work.
		return 0.f;
	}

	// get the axes for the binding
	InputAxes* pAxes = nullptr;
	if (!m_tBindingAxisMap.GetValue(bindingName, pAxes))
	{
#if !SEOUL_PLATFORM_WINDOWS
		SEOUL_WARN("No axes for binding \"%s\".\n", bindingName.CStr());
#endif
		return 0.f;
	}

	Bool bFoundBinding = false;

	// return the state from the first device that has this axis
	for (auto deviceIter = m_vInputDevices.Begin();
		m_vInputDevices.End() != deviceIter;
		++deviceIter)
	{
		const InputDevice& device = *(*deviceIter);
		for (auto axesIter = pAxes->Begin(); pAxes->End() != axesIter; ++axesIter)
		{
			InputDevice::Axis const* pAxis = device.GetAxis(*axesIter);
			if ( pAxis )
			{
				bFoundBinding = true;

				// If this axis was captured during input handling, return its state as 0.0
				if (pAxis->Handled())
				{
					if (pWhichDevice)
					{
						*pWhichDevice = nullptr;
					}
					return 0.0f;
				}

				if (pWhichDevice)
				{
					// Update pWhichDevice even if axis isn't zero - in
					// case no axis is nonzero, we still want to update it
					*pWhichDevice = &device;
				}
				if (!IsZero(pAxis->GetState()))
				{
					return pAxis->GetState();
				}
			}
		}
	}

	if (!bFoundBinding)
	{
		SEOUL_WARN("No device with binding: %s\n", bindingName.CStr());
	}

	// If they weren't valid, then no user had an axis that was nonzero,
	// which is okay.
	return 0.f;
}

Bool InputManager::InternalHandleButtonEvent(InputDevice *pDevice, InputButton buttonID, ButtonEventType eventType)
{
	// Update pressed key state. Used by GUIScreen to handle
	// key processing.
	if (eventType == ButtonEventType::ButtonReleased)
	{
		switch (buttonID)
		{
		case InputButton::KeyLeftShift: m_SpecialKeyFlags &= ~kLeftShift; break;
		case InputButton::KeyRightShift: m_SpecialKeyFlags &= ~kRightShift; break;
		case InputButton::KeyLeftAlt: m_SpecialKeyFlags &= ~kLeftAlt; break;
		case InputButton::KeyRightAlt: m_SpecialKeyFlags &= ~kRightAlt; break;
		case InputButton::KeyLeftControl: m_SpecialKeyFlags &= ~kLeftControl; break;
		case InputButton::KeyRightControl: m_SpecialKeyFlags &= ~kRightControl; break;
		default:
			break;
		}
	}
	else
	{
		switch (buttonID)
		{
		case InputButton::KeyLeftShift: m_SpecialKeyFlags |= kLeftShift; break;
		case InputButton::KeyRightShift: m_SpecialKeyFlags |= kRightShift; break;
		case InputButton::KeyLeftAlt: m_SpecialKeyFlags |= kLeftAlt; break;
		case InputButton::KeyRightAlt: m_SpecialKeyFlags |= kRightAlt; break;
		case InputButton::KeyLeftControl: m_SpecialKeyFlags |= kLeftControl; break;
		case InputButton::KeyRightControl: m_SpecialKeyFlags |= kRightControl; break;
		default:
			break;
		}
	}

	// Return false - indicates to Events::Manager that the event has not been
	// handled and should continue to be dispatched.
	return false;
}

/**
 * Hashes an AxisAndDirection structure
 */
UInt32 GetHash(const InputManager::AxisAndDirection& axisAndDir)
{
	return MixHashes(
		GetHash((int)axisAndDir.m_eAxis), axisAndDir.m_bPositive ? 1 : 0);
}

} // namespace Seoul
