/**
 * \file InputDevice.h
 * \brief Base class of concrete subclasses which implement input
 * capture from an arbitrary set of hardware input devices supported
 * by SeoulEngine (e.g. Keyboard, Mouse, Controller).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef INPUT_DEVICE_H
#define INPUT_DEVICE_H

#include "Geometry.h"
#include "HashTable.h"
#include "InputKeys.h"
#include "SeoulTypes.h"
#include "Vector.h"

namespace Seoul
{
class InputDevice;

typedef Vector<InputDevice*, MemoryBudgets::Input> InputDevices;

static const UInt InvalidInputDeviceID = UINT_MAX;

/**
 * Represents a single input device
 *
 * Represents a single input device, such as a keyboard, mouse, or game
 * controller.  An input device consists of a number of buttons and axes.
 * Buttons are binary inputs (either pressed or not pressed), and have actions
 * corresponding to being pressed, being released, and being held down for a
 * long enough period of time (i.e. a repeat event).  Axes are analog inputs,
 * and generate an input event every frame with their new values.  Mouse
 * cursors and gamepad analog sticks are examples of axes (and in those inputs
 * actually consist of a pair of axes, one for each of the X- and Y-directions).
 */
class InputDevice SEOUL_ABSTRACT
{
public:
	/**
	 * Input device type constants
	 */
	enum Type
	{
		Keyboard,
		Mouse,
		Xbox360Controller,
		PS3Controller,
		PS3NavController,
		WiiRemote,
		GameController,
		Unknown
	};

	/**
	 * Represents a button on an input device
	 *
	 * Represents a button on an input device.  Each button has an identifier
	 * which is unique within the device (but may not be unique between
	 * multiple devices).
	 */
	struct Button
	{
		Button();
		Button(InputButton buttonID);
		Button(InputButton buttonID, UInt32 flag);

		void UpdateState(Bool bPressed);

		/** Button identifier, unique within the device */
		InputButton m_ID;

		/** Button bit flag, unique within the api/device */
		UInt32 m_BitFlag;

		/** Whether or not the button is currently pressed */
		Bool m_bPressed;

		/** Whether or not the button was pressed on the previous frame */
		Bool m_bPrevPressed;

		/** If this is true, the user toggle the button and even number of times since the last tick. */
		Bool m_bDoubleTogglePressed;

		/** This is true if we have already updated the pressed state at least once this tick. */
		Bool m_bUpdatedSinceLastCheck;

		/** True if the last event for this button was handled by an input handler the last time it was dispatched. */
		Bool m_bHandled;

		/**
		 * Time (in seconds) between when the button is first pressed, and when
		 * the first repeat event is generated
		 */
		Float m_fRepeatDelay;

		/** Time (in seconds) between successive repeat events */
		Float m_fRepeatRate;

		/** Time (in seconds) until the next repeat event will occur */
		Float m_fTimeUntilRepeat;

		// Default repeat values
		static const Float s_fDefaultRepeatDelay;
		static const Float s_fDefaultRepeatRate;
	};

	/**
	 * Represents an analog input on an input device
	 *
	 * Represents an analog input on an input device, such as a mouse, an analog
	 * stick, or a pressure-sensitive trigger.
	 */
	class Axis
	{
	public:
		// Types of dead zones
		enum DeadZoneType
		{
			DeadZoneNone,			// No dead zone
			DeadZoneSingleCentered,	// Single-axis dead zone, centered at the center of the axis
			DeadZoneSingleZeroBased,// Single-axis dead zone, starting from zero
			DeadZoneDualCircular,	// Dual-axis dead zone, circular in shape
			DeadZoneDualSquare		// Dual-axis dead zone, square in shape
		};

		Axis();
		Axis(InputAxis axisID);

		void UpdateState(Int nRawState);
		void UpdateZeroBasedState(Int nRawState);

		void RemoveDeadZone();
		void SetDeadZone(Float fDeadZoneSize);
		void SetZeroBasedDeadZone(Float fDeadZoneSize);
		void SetCircularDeadZoneWithAxis(Axis *pOtherAxis, Float fDeadZoneDiamater);
		void SetSquareDeadZoneWithAxis(Axis *pOtherAxis, Float fDeadZoneSize);

		/**
		 * Gets the normalized state of the axis
		 *
		 * @return The normalized state of the axis, between -1 and 1 inclusive
		 */
		Float GetState() const
		{
			return m_fState;
		}

		/**
		 * Gets the normalized state of the axis from the previous frame
		 *
		 * @return The previous normalized state of the axis, between -1 and 1
		 *         inclusive
		 */
		Float GetPrevState() const
		{
			return m_fPrevState;
		}

		/**
		 * Gets the raw state of the axis
		 *
		 * @return The raw state of the axis
		 */
		Int GetRawState() const
		{
			return m_nRawState;
		}

		/**
		 * @return True if this Axis was handled the last time it
		 * was dispatched.
		 */
		Bool Handled() const
		{
			return m_bHandled;
		}

		/**
		 * Update whether this Axis was handled or not.
		 */
		void SetHandled(Bool bHandled)
		{
			m_bHandled = bHandled;
		}

		/**
		 * Gets the identifier of the axis
		 *
		 * @return The identifier of the axis
		 */
		InputAxis GetID() const
		{
			return m_ID;
		}

		void SetRange(Int nMin, Int nMax);

	private:
		/** Axis identifier, unique within the device */
		InputAxis m_ID;

		/** Raw state of the axis */
		Int m_nRawState;

		/** Minimum raw value of the axis */
		Int m_nMinValue;

		/** Maximum raw value of the axis */
		Int m_nMaxValue;

		/** Type of dead zone used by the axis */
		DeadZoneType m_eDeadZoneType;

		/** Minimum raw value of the axis' dead zone */
		Int m_nDeadZoneMin;

		/** Maximum raw value of the axis' dead zone */
		Int m_nDeadZoneMax;

		/** Radius squared/half-width of dual-axis dead zone (ignored for other types of dead zones) */
		Float m_fDeadZoneSize;

		/** Other axis used for computing dual-axis dead zones */
		Axis *m_pDeadZoneSiblingAxis;

		/** Normalized state of the axis */
		Float m_fState;

		/** Normalized state from the previous frame */
		Float m_fPrevState;

		/** True if an axis handler callback captured this axis state during the last dispatch. */
		Bool m_bHandled;
	};

	InputDevice(Type eDeviceType);
	virtual ~InputDevice();

	/**
	 * Gets the device type of this device
	 *
	 * @return The device type of this device
	 */
	Type GetDeviceType() const
	{
		return m_eDeviceType;
	}

	Button *GetButton(InputButton buttonID);
	Axis const* GetAxis(InputAxis axisID) const;
	Axis* GetAxis(InputAxis axisID);

	void SetAxisDeadZone(InputAxis eAxis, Float fDeadZoneSize);
	void SetZeroBasedAxisDeadZone(InputAxis eAxis, Float fDeadZoneSize);

	void SetTwoAxisDeadZoneCircular(InputAxis eAxis1, InputAxis eAxis2, Float fDeadZoneDiameter);
	void SetTwoAxisDeadZoneSquare(InputAxis eAxis1, InputAxis eAxis2, Float fDeadZoneSize);

	virtual void Tick(Float fTime);
	virtual void Poll() = 0;

	// Allows interested devices to react to a lost of application focus.
	virtual void OnLostFocus() {}

	// Used on some platforms to pass input events from a main event loop to the appropriate devices.
	virtual void QueueKeyEvent(UInt nKey, Bool bPressed) {}
	virtual void QueueMouseButtonEvent(InputButton eMouseButton, Bool bPressed) {}
	virtual void QueueMouseMoveEvent(const Point2DInt& location) {}
	virtual void QueueMouseWheelEvent(Int iDelta) {}
	virtual void QueueTouchButtonEvent(InputButton eTouchButton, Bool bPressed) {}
	virtual void QueueTouchMoveEvent(InputButton eTouch, const Point2DInt& location) {}

	virtual Bool IsButtonDown(InputButton buttonID, Bool bReturnFalseIfHandled = false) const;
	Bool WasButtonPressed(InputButton buttonID, Bool bReturnFalseIfHandled = false) const;
	Bool WasButtonReleased(InputButton buttonID, Bool bReturnFalseIfHandled = false) const;
	Bool GetLastButtonPressed(InputButton& button) const;

	Bool IsConnected() const { return m_bConnected; }
	virtual Bool IsMultiTouchDevice() const { return false; }
	Bool WasConnected() const { return m_bWasConnected; }

	Bool VibrationEnabled() const;

protected:
	Type m_eDeviceType;

	Vector<Button> m_aButtons;
	Vector<Axis> m_aAxes;

	Bool m_bConnected;
	Bool m_bWasConnected;
	Bool m_bVibrationEnabled;

private:
	SEOUL_DISABLE_COPY(InputDevice);
}; // class InputDevice

/**
 * Device subtype for mice
 *
 * Interface for mouse devices.
 */
class MouseDevice : public InputDevice
{
public:
	static const Int kMinWheelDelta = -127;
	static const Int kMaxWheelDelta = 127;

	MouseDevice();
	virtual ~MouseDevice();

	virtual Point2DInt GetMousePosition() const = 0;

private:
	SEOUL_DISABLE_COPY(MouseDevice);
}; // class MouseDevice

/**
 * Device subtype for multi-touch input devices (e.g. touch screens).
 */
class MultiTouchDevice : public MouseDevice
{
public:
	MultiTouchDevice();
	virtual ~MultiTouchDevice();

	virtual UInt32 GetTouchCount() const = 0;
	virtual Point2DInt GetTouchPosition(UInt32 u) const = 0;

	virtual Bool IsMultiTouchDevice() const SEOUL_OVERRIDE SEOUL_SEALED { return true; }

private:
	SEOUL_DISABLE_COPY(MultiTouchDevice);
}; // class MultiTouchDevice

/**
 * Interface for enumerating input devices
 *
 * Interface for enumerating input devices.  Any platform-specific code that
 * enumerates input devices for the platform (e.g. finds connected controllers)
 * should derive from this class and implement the EnumerateDevices() method.
 * The purpose of this class is to keep the platform-specific code separate from
 * the generic input-handling code.
 */
class InputDeviceEnumerator
{
public:
	virtual ~InputDeviceEnumerator()
	{
	}

	/**
	 * Enumerates input devices
	 *
	 * Enumerates input devices in a platform-specific manner.  The list of
	 * devices found is returned through the vDevices parameter.
	 *
	 * @param[out] aDevices Receives the list of input devices found
	 */
	virtual void EnumerateDevices(InputDevices & aDevices) = 0;
};

} // namespace Seoul

#endif // include guard
