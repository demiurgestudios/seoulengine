/**
 * \file InputManager.h
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

#pragma once
#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include "Atomic32.h"
#include "Delegate.h"
#include "FilePath.h"
#include "HashTable.h"
#include "FixedArray.h"
#include "InputDevice.h"
#include "InputKeys.h"
#include "SeoulString.h"
#include "SeoulTypes.h"
#include "Singleton.h"
#include "Vector.h"

namespace Seoul
{
/**
 * Callback function signature for callbacks called when the user has
 * confirmed or not confirmed a selection
 */
typedef void (*DeviceConnectionStatusChangedCallback)(InputDevice*);

typedef Pair<InputButton,String> InputBindingPair;

class InputManager SEOUL_SEALED : public Singleton<InputManager>
{
public:
	SEOUL_DELEGATE_TARGET(InputManager);

	/**
	 * Helper structure, used to store a button plus a special key
	 * bitvector to define a total key binding.
	 */
	struct InputButtonPlusModifier
	{
		static InputButtonPlusModifier Create(InputButton eButton, UInt32 specialKeys)
		{
			InputButtonPlusModifier ret;
			ret.m_eButton = eButton;
			ret.m_SpecialKeyFlags = specialKeys;
			return ret;
		}

		InputButton m_eButton;
		UInt32 m_SpecialKeyFlags;
	};

	typedef Vector<InputButtonPlusModifier, MemoryBudgets::Input> ButtonVector;

	InputManager();
	~InputManager();

	void Initialize();

	void EnumerateInputDevices(InputDeviceEnumerator *pEnumerator);

	InputDevice* GetDevice(UInt uInputDeviceID) const { return m_vInputDevices[uInputDeviceID]; }
	UInt GetNumDevices() const { return m_vInputDevices.GetSize(); }
	UInt GetNumDevices(InputDevice::Type eDeviceType) const;
	void TriggerRescan();
	Bool ShouldRescan() const { return m_bForceRescan; }

	// Should be called whenever the game is no longer the main application, giving
	// input devices a chance to clear state.
	void OnLostFocus();

	void Tick(Float fTime);

	static InputButton GetInputButtonForVKCode(UInt uVKCode);
	static UInt GetVKCodeForInputButton(InputButton eButton);

	InputButton GetButtonID(const Vector<String>& vButtonTokens);
	UInt32 GetSpecialKeys(const Vector<String>& vButtonTokens);
	InputAxis GetAxisID(const String & sAxisName);

	static Byte const* InputButtonToString(InputButton b);

	String BindingToString(HString bindingName) const;
	String ButtonToString(const InputButtonPlusModifier& button) const;

	/**
	 * Returns the InputButtons assigned to the given binding
	 */
	ButtonVector const* GetButtonsFromBinding(HString bindingName) const
	{
		ButtonVector* pReturn = nullptr;
		(void)m_tBindingButtonMap.GetValue(bindingName, pReturn);
		return pReturn;
	}

	/**
	 * Equivalent to OverrideButtonForBinding(), but completely erases
	 * the specified button binding.
	 */
	void ClearButtonForBinding(HString bindingName, Bool bSave = true);

	/**
	 * Set a new user specific button for the given binding. If
	 * bSave is true (the default), will immediately commit the new input
	 * state to disk.
	 */
	void OverrideButtonForBinding(
		HString bindingName,
		InputButton eButton,
		UInt32 uSpecialKeys,
		Bool bSave = true);

	// Trigger a manual save of input settings to the user's config. Only settings
	// that differ from the base (default) input settings are saved.
	void SaveBindingsToUserConfig();

	/**
	 * @return True if a system binding lock is currently set, false otherwise.
	 * This is expected to be set when the current platform's system
	 * UI is visible.
	 */
	Bool HasSystemBindingLock() const
	{
		return (m_nSystemBindingLock > 0);
	}

	/**
	 * Subtract one from the system binding lock count - the binding is considered
	 * locked when the count > 0, unlocked otherwise.
	 */
	void DecrementSystemBindingLock()
	{
		--m_nSystemBindingLock;
	}

	/**
	 * Add one to the system binding lock count - the binding is considered
	 * locked when the count > 0, unlocked otherwise.
	 */
	void IncrementSystemBindingLock()
	{
		++m_nSystemBindingLock;
	}

	Bool IsBindingDown(
		HString bindingName,
		Bool bIgnoreExtraModifiers = false,
		InputDevice const** pWhichDevice = nullptr) const;
	Bool WasBindingPressed(
		HString bindingName,
		Bool bIgnoreExtraModifiers = false,
		InputDevice const** pWhichDevice = nullptr) const;
	Bool WasBindingReleased(
		HString bindingName,
		InputDevice const** pWhichDevice = nullptr) const;

	Bool IsBindingDown(
		const String& sBindingName,
		Bool bIgnoreExtraModifiers = false,
		InputDevice const** pWhichDevice = nullptr) const
	{
		return IsBindingDown(HString(sBindingName), bIgnoreExtraModifiers, pWhichDevice);
	}
	Bool WasBindingPressed(
		const String& sBindingName,
		Bool bIgnoreExtraModifiers = false,
		InputDevice const** pWhichDevice = nullptr) const
	{
		return WasBindingPressed(HString(sBindingName), bIgnoreExtraModifiers, pWhichDevice);
	}

	Bool WasBindingReleased(
		const String& sBindingName,
		InputDevice const** pWhichDevice = nullptr) const
	{
		return WasBindingReleased(HString(sBindingName), pWhichDevice);
	}

	Bool IsBindingDown(
		Byte const* sBindingName,
		Bool bIgnoreExtraModifiers = false,
		InputDevice const** pWhichDevice = nullptr) const
	{
		return IsBindingDown(HString(sBindingName), bIgnoreExtraModifiers, pWhichDevice);
	}
	Bool WasBindingPressed(
		Byte const* sBindingName,
		Bool bIgnoreExtraModifiers = false,
		InputDevice const** pWhichDevice = nullptr) const
	{
		return WasBindingPressed(HString(sBindingName), bIgnoreExtraModifiers, pWhichDevice);
	}

	Bool WasBindingReleased(
		Byte const* sBindingName,
		InputDevice const** pWhichDevice = nullptr) const
	{
		return WasBindingReleased(HString(sBindingName), pWhichDevice);
	}

	// Manually inject a binding event - will cause WasBindingReleased and WasBindingPressed
	// to return true, once, for the specified event.
	void ManuallyInjectBindingEvent(HString bindingName);

	MouseDevice* FindFirstMouseDevice() const;

	Float GetAxisState(
		HString bindingName,
		InputDevice const** pWhichDevice = nullptr) const;
	const Point2DInt& GetMousePosition() const
	{
		return m_MousePosition;
	}

	// Injects input events
	void QueueKeyboardEvent(UInt32 uVirtualKeyCode, Bool bPressed);
	void QueueMouseButtonEvent(InputButton eMouseButton, Bool bPressed);
	void QueueMouseMoveEvent(const Point2DInt& location);
	void QueueMouseWheelEvent(Int iDelta);
	void QueueTouchButtonEvent(InputButton eTouchButton, Bool bPressed);
	void QueueTouchMoveEvent(InputButton eTouch, const Point2DInt& location);

	void UpdateDeadZonesForCurrentControllers();

	Bool HasConnectedDevice(InputDevice::Type eDeviceType) const;

	void SetDeviceConnectionStatusChangedCallback(DeviceConnectionStatusChangedCallback c)
	{
		m_pDeviceConnectionChangedCallback = c;
	}

	enum SpecialKeyBits
	{
		kLeftAlt = (1 << 0),
		kRightAlt = (1 << 1),
		kLeftShift = (1 << 2),
		kRightShift = (1 << 3),
		kLeftControl = (1 << 4),
		kRightControl = (1 << 5)
	};

	static inline Bool IsSpecial(InputButton eButton)
	{
		switch (eButton)
		{
		case InputButton::KeyLeftAlt: // fall-through
		case InputButton::KeyLeftControl: // fall-through
		case InputButton::KeyLeftShift: // fall-through
		case InputButton::KeyRightAlt: // fall-through
		case InputButton::KeyRightControl: // fall-through
		case InputButton::KeyRightShift: // fall-through
			return true;
		default:
			return false;
		};
	}

	/**
	 * Returns true if one of the special keys (alt, shift, or ctrl) is
	 * currently pressed.  If bIgnoreExtraModifiers is false, then exactly
	 * those keys must be pressed; if it is true, then extra modifiers may be
	 * present.
	 */
	Bool IsSpecialPressed(UInt32 specialKeys, Bool bIgnoreExtraModifiers = false) const
	{
		Bool bReturn = true;

		// We only return true if each special key (ALT, SHIFT, CTRL)
		// of the input state matches the current state. Meaning, an input
		// of ALT+ENTER will return false if the current state is CTRL+ALT+ENTER
		// and vice versa.
		bReturn = bReturn && InternalCheckSpecial(specialKeys, kLeftAlt | kRightAlt, bIgnoreExtraModifiers);
		bReturn = bReturn && InternalCheckSpecial(specialKeys, kLeftShift | kRightShift, bIgnoreExtraModifiers);
		bReturn = bReturn && InternalCheckSpecial(specialKeys, kLeftControl | kRightControl, bIgnoreExtraModifiers);

		return bReturn;
	}

	/**
	 * Determine if the specified button should function on the current
	 * platform.
	 *
	 * @param[in] b		the button to test
	 * @return true if the button should affect the game on the current platform
	 */
	static Bool IsButtonForThisPlatform(const InputButton b)
	{
#if SEOUL_PLATFORM_WINDOWS || SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
		// any of the buttons that are not specific to console controllers
		return !IsButtonForXbox(b) && !IsButtonForPS3(b) && !IsButtonForWii(b);
#else
#		error "Define for this platform."

		// default is to not function on this platform
		return false;
#endif
	}

	/**
	 * Determine if the specified button functions on Xbox
	 * @param[in] b		the button to test
	 * @return true if button matters on xbox
	 */
	static Bool IsButtonForXbox(const InputButton b)
	{
		return b > InputButton::XboxSectionStart && b < InputButton::XboxSectionEnd;
	}

	/**
	 * Determine if the specified button functions on PS3
	 * @param[in] b the button to test
	 * @return true if button matters on PS3
	 */
	static Bool IsButtonForPS3(const InputButton b)
	{
		return b > InputButton::PS3SectionStart && b < InputButton::PS3SectionEnd;
	}

	/**
	 * Determine if the specified button functions on Wii
	 * @param[in] b		the button to test
	 * @return true if button matters on Wii
	 */
	static Bool IsButtonForWii(const InputButton b)
	{
		return b > InputButton::WiiSectionStart && b < InputButton::WiiSectionEnd;
	}

	void LoadBindingsFromJson();

private:
	Bool InternalLoadBindingsFromJson(FilePath filePath);

	/** Helper function, used to check a single special key
	 *  (i.e. ALT) in both its left-right variations against an input
	 *  special key bit-vector.
	 *
	 *  This function returns true if either the masked portion
	 *  of the bitvector of both the current and input state is equal,
	 *  or if the input state has both left and right keys marked and
	 *  the current state has one or the other key as pressed.
	 */
	Bool InternalCheckSpecial(UInt32 specialKeyFlags, UInt32 mask, Bool bIgnoreExtraModifiers) const
	{
		UInt32 in = (mask & specialKeyFlags);
		UInt32 cur = (mask & m_SpecialKeyFlags);

		return (in == cur) || ((in == mask || bIgnoreExtraModifiers) && cur != 0);
	}

	void ClearBindings();
	Bool CheckBinding(
		HString bindingName,
		Bool bIgnoreExtraModifiers,
		UInt iCheckType,
		InputDevice const** pWhichDevice) const;

	Bool InternalHandleButtonEvent(InputDevice *pDevice, InputButton buttonID, ButtonEventType eventType);

public:
	// Name of the json section where we store input bindings
	static const String s_kInputBindingsJsonSection;

private:
	Bool m_bInitialized;

	// Should rescan for expensive-to-scan input devices next poll?
	Atomic32 m_nPendingForceRescan;
	Atomic32Value<Bool> m_bForceRescan;

	// a list of all of the input devices currently connected
	InputDevices m_vInputDevices;

	Point2DInt m_PreviousMousePosition;
	Point2DInt m_MousePosition;

	/** Maps a name to one or more InputButtons */
	typedef HashTable<HString, ButtonVector*, MemoryBudgets::Input> BindingButtonMap;
	BindingButtonMap m_tBindingButtonMap;

	/** Maps a name to one or more InputAxis */
	typedef Vector<InputAxis, MemoryBudgets::Input> InputAxes;
	typedef HashTable<HString, InputAxes*, MemoryBudgets::Input> BindingAxisMap;
	BindingAxisMap m_tBindingAxisMap;

	/**
	 * Structure representing an axis and a direction for that axis
	 * (positive or negative)
	 */
	struct AxisAndDirection
	{
		AxisAndDirection()
			: m_eAxis(InputAxis::AxisUnknown)
			, m_bPositive(false)
		{
		}

		Bool operator == (const AxisAndDirection& other) const
		{
			return (m_eAxis == other.m_eAxis && m_bPositive == other.m_bPositive);
		}

		Bool operator != (const AxisAndDirection& other) const
		{
			return !(*this == other);
		}

		InputAxis m_eAxis;
		Bool m_bPositive;
	};

	/**
	 * HashTable traits for AxisAndDirection so that it can be used as a
	 * key in tables
	 */
	struct AxisAndDirectionHashTableKeyTraits
	{
		inline static Float GetLoadFactor()
		{
			return 0.75f;
		}

		inline static InputManager::AxisAndDirection GetNullKey()
		{
			return InputManager::AxisAndDirection();
		}

		static const Bool kCheckHashBeforeEquals = false;
	};

	friend UInt32 GetHash(const AxisAndDirection& axisAndDir);

	/**
	 * Maps a directional axis to a list of binding names for converting axis
	 * input to button input
	 */
	typedef Vector<HString, MemoryBudgets::Input> BindingVector;
	typedef HashTable<AxisAndDirection, BindingVector, MemoryBudgets::Input, AxisAndDirectionHashTableKeyTraits> BindingAxisToButtonMap;
	BindingAxisToButtonMap m_BindingAxisToButtonMap;

	typedef HashTable<HString, Bool, MemoryBudgets::Input> ManualBindingEvents;
	ManualBindingEvents m_tManualBindingEvents;

	// need to keep these around for controller re-init if they're hotswapped
	Float m_fLeftStickDeadZone;
	Float m_fRightStickDeadZone;
	Float m_fTriggerDeadZone;

	Atomic32 m_nSystemBindingLock;

	DeviceConnectionStatusChangedCallback m_pDeviceConnectionChangedCallback;

	UInt32 m_SpecialKeyFlags;
};

// Input event IDs
extern const HString g_EventButtonEvent;
extern const HString g_EventAxisEvent;
extern const HString g_MouseMoveEvent;

// Hashes an AxisAndDirection structure
UInt32 GetHash(const InputManager::AxisAndDirection& axisAndDir);

} // namespace Seoul

#endif // include guard
