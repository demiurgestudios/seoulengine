/**
 * \file GenericInput.h
 * \brief Platform independent mouse and keyboard devices that require
 * platform dependent key and mouse button/move injection. The keyboard
 * device is based on VK_* style key codes, which are actually use
 * on multiple platforms despite being tied to Win32.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GENERIC_INPUT_H
#define GENERIC_INPUT_H

#include "AtomicRingBuffer.h"
#include "FixedArray.h"
#include "InputManager.h"
#include "InputKeys.h"

namespace Seoul
{

/**
 * Specialization of InputDevice for handling input
 * from a generic joystick or gamepad.
 *
 * The current platform must inject button and axis change events
 * for this instance to function as expected.
 */
class GenericController SEOUL_SEALED : public InputDevice
{
public:
	GenericController();
	~GenericController();

	virtual void OnLostFocus() SEOUL_OVERRIDE;

	virtual void Poll() SEOUL_OVERRIDE;

	void QueueAxisEvent(InputAxis eAxis, Int iAxisValue);
	void QueueButtonEvent(InputButton eButton, Bool bPressed);

private:
	struct InputEvent
	{
		union
		{
			InputAxis m_eAxis;
			InputButton m_eButton;
		};
		union
		{
			Int m_iAxisValue;
			Bool m_bPressed;
		};
		Bool m_bAxisEvent;
	};

	typedef AtomicRingBuffer<InputEvent*, MemoryBudgets::Input> EventBuffer;

	EventBuffer m_FreeBuffer;
	EventBuffer m_InputBuffer;
	Atomic32Value<Bool> m_bLostFocus;

	SEOUL_DISABLE_COPY(GenericController);
}; // class GenericController

/**
 * Specialization of InputDevice for handling input from a keyboard.
 * The current platform must inject VK_ style key press and release events via
 * QueueKeyEvent for this instance to function as expected.
 */
class GenericKeyboard SEOUL_SEALED : public InputDevice
{
public:
	GenericKeyboard();
	~GenericKeyboard();

	virtual void OnLostFocus() SEOUL_OVERRIDE;

	virtual void Tick(Float fTime) SEOUL_OVERRIDE;
	virtual void Poll() SEOUL_OVERRIDE;

	virtual void QueueKeyEvent(UInt nKey, Bool bPressed) SEOUL_OVERRIDE;

private:
	/** Event queues for each of the (up to) 256 buttons on the keyboard */
	FixedArray< AtomicRingBuffer<void*, MemoryBudgets::Input>, 256u> m_aEventQueues;

	Bool m_bLostFocus;
}; // class GenericKeyboard

/**
 * Specialization of InputDevice for handling input from a mouse.
 * Mouse move and button events must be injected via QueueMouseButtonEvent()
 * and QueueMouseMoveEvent() for this instance to function as expected.
 */
class GenericMouse SEOUL_SEALED : public MouseDevice
{
public:
	GenericMouse();
	~GenericMouse();

	virtual void OnLostFocus() SEOUL_OVERRIDE;

	virtual void Poll() SEOUL_OVERRIDE;

	virtual Point2DInt GetMousePosition() const SEOUL_OVERRIDE
	{
		return m_MousePosition;
	}

	virtual void QueueMouseButtonEvent(InputButton eMouseButton, Bool bPressed) SEOUL_OVERRIDE;
	virtual void QueueMouseMoveEvent(const Point2DInt& location) SEOUL_OVERRIDE;
	virtual void QueueMouseWheelEvent(Int iDelta) SEOUL_OVERRIDE;

private:
	Point2DInt m_MousePosition;

	/** Types of events that we cache in our ring buffer. */
	enum MouseInputEventType
	{
		/** Any key/button down event. */
		kButtonPressed,

		/** Any key/button up event. */
		kButtonReleased,

		/** A mouse move event. */
		kMove,

		/** A mouse wheel event. */
		kWheel,
	};

	struct InputEvent
	{
		Point2DInt m_LocationOrDelta;
		InputButton m_eMouseButton;
		MouseInputEventType m_eEventType;
	};

	typedef AtomicRingBuffer<InputEvent*, MemoryBudgets::Input> EventBuffer;

	EventBuffer m_FreeBuffer;
	EventBuffer m_InputBuffer;
	Atomic32Value<Bool> m_bLostFocus;

private:
	SEOUL_DISABLE_COPY(GenericMouse);
}; // class GenericMouse

/**
 * Specialization of InputDevice for handling input from a multi-touch device.
 */
class GenericMultiTouch SEOUL_SEALED : public MultiTouchDevice
{
public:
	typedef FixedArray<Point2DInt, ((int)InputButton::TouchButtonLast - (int)InputButton::TouchButtonFirst + 1)> Positions;

	GenericMultiTouch();
	~GenericMultiTouch();

	virtual void OnLostFocus() SEOUL_OVERRIDE;

	virtual void Poll() SEOUL_OVERRIDE;

	virtual UInt32 GetTouchCount() const SEOUL_OVERRIDE;
	virtual Point2DInt GetTouchPosition(UInt32 u) const SEOUL_OVERRIDE
	{
		// Touch query is out of range, return 0, 0.
		if (u >= m_aPositions.GetSize())
		{
			return Point2DInt();
		}

		// Return last tracked position.
		return m_aPositions[u];
	}

	virtual Point2DInt GetMousePosition() const SEOUL_OVERRIDE
	{
		// Index 0 is the mouse primary.
		return m_aPositions.Front();
	}

	virtual void QueueMouseButtonEvent(InputButton eMouseButton, Bool bPressed) SEOUL_OVERRIDE
	{
		if (InputButton::MouseLeftButton == eMouseButton)
		{
			QueueTouchButtonEvent(InputButton::TouchButton1, bPressed);
		}
	}
	virtual void QueueMouseMoveEvent(const Point2DInt& location) SEOUL_OVERRIDE
	{
		QueueTouchMoveEvent(InputButton::TouchButton1, location);
	}
	virtual void QueueTouchButtonEvent(InputButton eTouchButton, Bool bPressed) SEOUL_OVERRIDE;
	virtual void QueueTouchMoveEvent(InputButton eTouch, const Point2DInt& location) SEOUL_OVERRIDE;

private:
	Positions m_aPositions;

	/** Types of events that we cache in our ring buffer. */
	enum InputEventType
	{
		/** Any press event. */
		kPressed,

		/** Any release event. */
		kReleased,

		/** A touch move event. */
		kMove,
	};

	struct InputEvent SEOUL_SEALED
	{
		InputEvent()
			: m_LocationOrDelta()
			, m_eTouch(InputButton::ButtonUnknown)
			, m_eEventType(kPressed)
		{
		}

		Point2DInt m_LocationOrDelta;
		InputButton m_eTouch;
		InputEventType m_eEventType;
	}; // struct InputEvent

	typedef AtomicRingBuffer<InputEvent*, MemoryBudgets::Input> EventBuffer;

	EventBuffer m_FreeBuffer;
	EventBuffer m_InputBuffer;
	Atomic32Value<Bool> m_bLostFocus;

	SEOUL_DISABLE_COPY(GenericMultiTouch);
}; // class GenericMultiTouch

} // namespace Seoul

#endif // include guard
