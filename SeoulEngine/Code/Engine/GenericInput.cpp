/**
 * \file GenericInput.cpp
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

#include "EventsManager.h"
#include "GenericInput.h"
#include "Engine.h"
#include "RenderDevice.h"
#include "Thread.h"

namespace Seoul
{

static const UInt32 kInitialEventBufferSize = 1024u;

GenericController::GenericController()
	: InputDevice(Xbox360Controller) // TODO: Don't pretend to be an Xbox 360 controller.
	, m_FreeBuffer()
	, m_InputBuffer()
	, m_bLostFocus(false)
{
	// Initialize the free events buffer.
	for (size_t i = 0; i < kInitialEventBufferSize; ++i)
	{
		m_FreeBuffer.Push(SEOUL_NEW(MemoryBudgets::Input) InputEvent);
	}

	// Support buttons and axes for other controller types.
	for (Int i = (Int)InputButton::XboxSectionStart + 1; i < (Int)InputButton::XboxSectionEnd; ++i)
	{
		m_aButtons.PushBack(Button((InputButton)i));
	}

	// Setup Xbox 360 style axes.
	m_aAxes.PushBack(InputAxis::XboxLeftThumbstickX);
	m_aAxes.PushBack(InputAxis::XboxLeftThumbstickY);
	m_aAxes.PushBack(InputAxis::XboxRightThumbstickX);
	m_aAxes.PushBack(InputAxis::XboxRightThumbstickY);
	m_aAxes.PushBack(InputAxis::XboxLeftTriggerZ);
	m_aAxes.Back().SetRange(0, 255);
	m_aAxes.PushBack(InputAxis::XboxRightTriggerZ);
	m_aAxes.Back().SetRange(0, 255);

	m_bConnected = true;
}

GenericController::~GenericController()
{
	InputEvent* p = m_InputBuffer.Pop();
	while (nullptr != p)
	{
		SafeDelete(p);
		p = m_InputBuffer.Pop();
	}

	p = m_FreeBuffer.Pop();
	while (nullptr != p)
	{
		SafeDelete(p);
		p = m_FreeBuffer.Pop();
	}
}

/**
 * Called by InputManager when the game's focus is lost - allows
 * the mouse to release buttons and act as if it not in use.
 */
void GenericController::OnLostFocus()
{
	m_bLostFocus = true;
}

void GenericController::Poll()
{
	// Cache the connected state.
	m_bWasConnected = m_bConnected;

	// All button states are initially equal to the current state.
	Vector<Bool> vbNewState(m_aButtons.GetSize());
	Vector<Bool> vbUpdatedState(m_aButtons.GetSize());
	for (UInt32 i = 0u; i < vbNewState.GetSize(); ++i)
	{
		vbNewState[i] = m_aButtons[i].m_bPressed;
	}

	// All axis states are initially equal to the current state.
	Vector<Int> viNewAxisState(m_aAxes.GetSize());
	for (UInt32 i = 0u; i < viNewAxisState.GetSize(); ++i)
	{
		viNewAxisState[i] = m_aAxes[i].GetRawState();
	}

	InputEvent* p = m_InputBuffer.Pop();
	while (nullptr != p)
	{
		// If a button event, update the button state.
		if (!p->m_bAxisEvent)
		{
			Int nButton = (Int)p->m_eButton - ((Int)InputButton::XboxSectionStart + 1);
			m_aButtons[nButton].UpdateState(p->m_bPressed);
			vbUpdatedState[nButton] = true;
		}
		// If an axis event, compute the value.
		else
		{
			viNewAxisState[(Int)p->m_eAxis - (Int)InputAxis::XboxLeftThumbstickX] = p->m_iAxisValue;
		}

		m_FreeBuffer.Push(p);

		p = m_InputBuffer.Pop();
	}

	// If the controller isn't active, just clear everything we processed.
	if (m_bLostFocus)
	{
		vbNewState.Assign(false);
		viNewAxisState.Assign((Int)0);
		m_bLostFocus = false;
	}

	// Update state changes.
	for (UInt32 i = 0u; i < vbNewState.GetSize(); ++i)
	{
		if(!vbUpdatedState[i])
		{
			m_aButtons[i].UpdateState(vbNewState[i]);
		}
	}

	// Update thes axes.
	for (UInt32 i = 0u; i < m_aAxes.GetSize(); ++i)
	{
		m_aAxes[i].UpdateState(viNewAxisState[i]);
	}
}

/**
 * Insert an axis event into the pending axis queue.
 */
void GenericController::QueueAxisEvent(InputAxis eAxis, Int iAxisValue)
{
	InputEvent* p = m_FreeBuffer.Pop();

	// If we've run out of buffer, ignore the event, as waiting for
	// the buffer to free up, we can hit a deadlock state if the user
	// has a message box displayed
	if (nullptr == p)
	{
		return;
	}

	memset(p, 0, sizeof(*p));
	p->m_bAxisEvent = true;
	p->m_eAxis = eAxis;
	p->m_iAxisValue = iAxisValue;

	m_InputBuffer.Push(p);
}

/**
 * Insert a button event into the pending button queue.
 */
void GenericController::QueueButtonEvent(InputButton eButton, Bool bPressed)
{
	InputEvent* p = m_FreeBuffer.Pop();

	// If we've run out of buffer, ignore the event, as waiting for
	// the buffer to free up, we can hit a deadlock state if the user
	// has a message box displayed
	if (nullptr == p)
	{
		return;
	}

	memset(p, 0, sizeof(*p));
	p->m_bAxisEvent = false;
	p->m_bPressed = bPressed;
	p->m_eButton = eButton;

	m_InputBuffer.Push(p);
}

GenericKeyboard::GenericKeyboard()
	: InputDevice(InputDevice::Keyboard)
	, m_bLostFocus(false)
{
	// Initialize the button array
	m_aButtons.Reserve(256);
	for(Int nKey = 0; nKey < 256; nKey++)
	{
		m_aButtons.PushBack(Button(InputManager::GetInputButtonForVKCode(nKey)));
	}

	// Mark as initially connected.
	m_bConnected = true;
}

GenericKeyboard::~GenericKeyboard()
{
}

/**
 * When focus has been lost, mark it, use this to release
 * all keyboard keys.
 */
void GenericKeyboard::OnLostFocus()
{
	m_bLostFocus = true;
}

void GenericKeyboard::Tick(Float fTime)
{
	InputDevice::Tick(fTime);
}

void GenericKeyboard::Poll()
{
	// Cache the connected state.
	m_bWasConnected = m_bConnected;

	// Only check input if the application window is active and isn't be
	// dragged/resized/etc. in a modal message loop
	if (!m_bLostFocus &&
		RenderDevice::Get()->IsActive() &&
	   !Engine::Get()->IsInModalWindowsLoop())
	{
		// Update the Seoul keyboard state from the windows messages
		for(UInt nKey = 0; nKey < 256; nKey++)
		{
			// Don't want to change the state if we didn't receive a key event
			Bool bPressed = m_aButtons[nKey].m_bPressed;
			if(!m_aEventQueues[nKey].IsEmpty())
			{
				void* p = m_aEventQueues[nKey].Pop();
				while (nullptr != p)
				{
					Bool const bValue = (((void*)1) == p);
					bPressed = bValue;
					m_aButtons[nKey].UpdateState(bPressed);
					p = m_aEventQueues[nKey].Pop();
				}
			}
			else
			{
				m_aButtons[nKey].UpdateState(bPressed);
			}
		}
	}
	else // When the window is not active make sure buttons act as if they are not pressed
	{
		for(UInt nKey = 0; nKey < 256; nKey++)
		{
			m_aButtons[nKey].UpdateState(false);
		}
		m_bLostFocus = false;
	}
}

void GenericKeyboard::QueueKeyEvent(UInt nKey, Bool bPressed)
{
	// Can't use 0 for false, because AtomicRingBuffer uses nullptr
	// as a placeholder for "no value".
	void* pValue = (bPressed ? (void*)1 : (void*)(size_t)0xFEEDFACE);

	// Push the event into the receive buffer.
	m_aEventQueues[nKey].Push(pValue);
}

GenericMouse::GenericMouse()
	: MouseDevice()
	, m_MousePosition(0, 0)
	, m_bLostFocus(false)
{
	for (size_t i = 0; i < kInitialEventBufferSize; ++i)
	{
		m_FreeBuffer.Push(SEOUL_NEW(MemoryBudgets::Input) InputEvent);
	}

	m_aButtons.Reserve(10);
	m_aButtons.PushBack(Button(InputButton::MouseButton1));
	m_aButtons.PushBack(Button(InputButton::MouseButton2));
	m_aButtons.PushBack(Button(InputButton::MouseButton3));
	m_aButtons.PushBack(Button(InputButton::MouseButton4));
	m_aButtons.PushBack(Button(InputButton::MouseButton5));
	m_aButtons.PushBack(Button(InputButton::MouseButton6));
	m_aButtons.PushBack(Button(InputButton::MouseButton7));
	m_aButtons.PushBack(Button(InputButton::MouseButton8));

	m_aAxes.Reserve(3);
	m_aAxes.PushBack(Axis(InputAxis::MouseX));
	m_aAxes.PushBack(Axis(InputAxis::MouseY));
	m_aAxes.PushBack(Axis(InputAxis::MouseWheel));

	// Range set so that the full supported mouse wheel values can fall in the range
	// of a byte [-127, 127].
	m_aAxes.Back().SetRange(kMinWheelDelta, kMaxWheelDelta);

	m_bConnected = true;
}

GenericMouse::~GenericMouse()
{
	InputEvent* p = m_InputBuffer.Pop();
	while (nullptr != p)
	{
		SafeDelete(p);
		p = m_InputBuffer.Pop();
	}

	p = m_FreeBuffer.Pop();
	while (nullptr != p)
	{
		SafeDelete(p);
		p = m_FreeBuffer.Pop();
	}
}

/**
 * Called by InputManager when the game's focus is lost - allows
 * the mouse to release buttons and act as if it not in use.
 */
void GenericMouse::OnLostFocus()
{
	m_bLostFocus = true;
}

void GenericMouse::Poll()
{
	// Cache the connected state.
	m_bWasConnected = m_bConnected;

	// All button states are initially equal to the current state.
	Vector<Bool> vbNewState(m_aButtons.GetSize());
	Vector<Bool> vbUpdatedState(m_aButtons.GetSize());
	for (UInt32 i = 0u; i < vbNewState.GetSize(); ++i)
	{
		vbNewState[i] = m_aButtons[i].m_bPressed;
	}

	// The mouse wheel always resets, so start with a delta of 0.
	Int iMouseWheelDelta = 0;

	// Set the delta to the current axis values initially.
	Point2DInt newDelta(m_aAxes[0].GetRawState(), m_aAxes[1].GetRawState());

	InputEvent* p = m_InputBuffer.Pop();
	while (nullptr != p)
	{
		switch (p->m_eEventType)
		{
		case kButtonPressed: // fall-through
		case kButtonReleased:
			// If a button event, update the button state.
			{
				Int nButton = (Int)p->m_eMouseButton - (Int)InputButton::MouseButton1;
				m_aButtons[nButton].UpdateState(kButtonPressed == p->m_eEventType);
				vbUpdatedState[nButton] = true;
			}
			break;

		case kMove:
			// If a mouse move event, compute the new delta and position.
			{
				newDelta = Point2DInt(p->m_LocationOrDelta.X - m_MousePosition.X, p->m_LocationOrDelta.Y - m_MousePosition.Y);
				m_MousePosition = p->m_LocationOrDelta;
			}
			break;

		case kWheel:
			// If a wheel event, capture the wheel change.
			{
				iMouseWheelDelta += p->m_LocationOrDelta.X;
			}
			break;
		};

		m_FreeBuffer.Push(p);

		p = m_InputBuffer.Pop();
	}

	// If the mouse isn't active, just clear everything we processed.
	if (m_bLostFocus)
	{
		vbNewState.Assign(false);
		m_bLostFocus = false;
	}

	// Update state changes.
	for (UInt32 i = 0u; i < vbNewState.GetSize(); ++i)
	{
		if(!vbUpdatedState[i])
		{
			m_aButtons[i].UpdateState(vbNewState[i]);
		}
	}

	// Set the new mouse location.
	m_aAxes[0].UpdateState(newDelta.X);
	m_aAxes[1].UpdateState(newDelta.Y);

	// Set the new mouse wheel delta.
	m_aAxes[2].UpdateState(iMouseWheelDelta);
}

/**
 * Insert a mouse button event into the pending mouse button queue.
 */
void GenericMouse::QueueMouseButtonEvent(InputButton eMouseButton, Bool bPressed)
{
	InputEvent* p = m_FreeBuffer.Pop();

	// If we've run out of buffer, ignore the event, as waiting for
	// the buffer to free up, we can hit a deadlock state if the user
	// has a message box displayed
	if (nullptr == p)
	{
		return;
	}

	p->m_LocationOrDelta = Point2DInt(0, 0);
	p->m_eMouseButton = eMouseButton;
	p->m_eEventType = (bPressed ? kButtonPressed : kButtonReleased);

	m_InputBuffer.Push(p);
}

/**
 * Insert a mouse move event into the pending mouse movement queue.
 */
void GenericMouse::QueueMouseMoveEvent(const Point2DInt& location)
{
	InputEvent* p = m_FreeBuffer.Pop();

	// If we've run out of buffer, ignore the event, as waiting for
	// the buffer to free up, we can hit a deadlock state if the user
	// has a message box displayed
	if (nullptr == p)
	{
		return;
	}

	p->m_LocationOrDelta = location;
	p->m_eMouseButton = InputButton::ButtonUnknown;
	p->m_eEventType = kMove;

	m_InputBuffer.Push(p);
}

/**
 * Insert a mouse wheel event into the pending mouse queue.
 */
void GenericMouse::QueueMouseWheelEvent(Int iDelta)
{
	InputEvent* p = m_FreeBuffer.Pop();

	// If we've run out of buffer, ignore the event, as waiting for
	// the buffer to free up, we can hit a deadlock state if the user
	// has a message box displayed
	if (nullptr == p)
	{
		return;
	}

	p->m_LocationOrDelta.X = iDelta;
	p->m_eMouseButton = InputButton::ButtonUnknown;
	p->m_eEventType = kWheel;

	m_InputBuffer.Push(p);
}

GenericMultiTouch::GenericMultiTouch()
	: MultiTouchDevice()
	, m_aPositions()
	, m_FreeBuffer()
	, m_InputBuffer()
	, m_bLostFocus(false)
{
	for (size_t i = 0; i < kInitialEventBufferSize; ++i)
	{
		m_FreeBuffer.Push(SEOUL_NEW(MemoryBudgets::Input) InputEvent);
	}

	m_aButtons.Reserve(5);
	m_aButtons.PushBack(Button(InputButton::TouchButton1));
	m_aButtons.PushBack(Button(InputButton::TouchButton2));
	m_aButtons.PushBack(Button(InputButton::TouchButton3));
	m_aButtons.PushBack(Button(InputButton::TouchButton4));
	m_aButtons.PushBack(Button(InputButton::TouchButton5));
	m_aButtons.PushBack(Button(InputButton::MouseButton1)); // Last entry echoes button 1 of a mouse.

	m_aAxes.Reserve(10);
	m_aAxes.PushBack(Axis(InputAxis::Touch1X));
	m_aAxes.PushBack(Axis(InputAxis::Touch1Y));
	m_aAxes.PushBack(Axis(InputAxis::Touch2X));
	m_aAxes.PushBack(Axis(InputAxis::Touch2Y));
	m_aAxes.PushBack(Axis(InputAxis::Touch3X));
	m_aAxes.PushBack(Axis(InputAxis::Touch3Y));
	m_aAxes.PushBack(Axis(InputAxis::Touch4X));
	m_aAxes.PushBack(Axis(InputAxis::Touch4Y));
	m_aAxes.PushBack(Axis(InputAxis::Touch5X));
	m_aAxes.PushBack(Axis(InputAxis::Touch5Y));

	m_bConnected = true;
}

GenericMultiTouch::~GenericMultiTouch()
{
	InputEvent* p = m_InputBuffer.Pop();
	while (nullptr != p)
	{
		SafeDelete(p);
		p = m_InputBuffer.Pop();
	}

	p = m_FreeBuffer.Pop();
	while (nullptr != p)
	{
		SafeDelete(p);
		p = m_FreeBuffer.Pop();
	}
}

void GenericMultiTouch::OnLostFocus()
{
	m_bLostFocus = true;
}

void GenericMultiTouch::Poll()
{
	// Cache the connected state.
	m_bWasConnected = m_bConnected;

	// All button states are initially equal to the current state.
	Vector<Bool> vbNewState(m_aButtons.GetSize());
	Vector<Bool> vbUpdatedState(m_aButtons.GetSize());
	for (UInt32 i = 0u; i < vbNewState.GetSize(); ++i)
	{
		vbNewState[i] = m_aButtons[i].m_bPressed;
	}

	// Set the delta to the current axis values initially.
	Vector<Int> viNewDelta(m_aAxes.GetSize());
	for (UInt32 i = 0u; i < viNewDelta.GetSize(); ++i)
	{
		viNewDelta[i] = m_aAxes[i].GetRawState();
	}

	InputEvent* p = m_InputBuffer.Pop();
	while (nullptr != p)
	{
		switch (p->m_eEventType)
		{
		case kPressed: // fall-through
		case kReleased:
			// If a button event, update the button state.
			{
				// Main update.
				{
					auto const iButton = (Int)p->m_eTouch - (Int)InputButton::TouchButton1;
					m_aButtons[iButton].UpdateState(kPressed == p->m_eEventType);
					vbUpdatedState[iButton] = true;
				}

				// Also update MouseButton1 if this is TouchButton1.
				if (InputButton::TouchButton1 == p->m_eTouch)
				{
					auto const iButton = (Int)vbUpdatedState.GetSize() - 1;
					m_aButtons[iButton].UpdateState(kPressed == p->m_eEventType);
					vbUpdatedState[iButton] = true;
				}
			}
			break;

		case kMove:
			// If a touch move event, compute the new delta and position.
			{
				auto const iButton = (Int)p->m_eTouch - (Int)InputButton::TouchButton1;
				auto const iAxis = (iButton * 2);
				viNewDelta[iAxis + 0] = p->m_LocationOrDelta.X - m_aPositions[iButton].X;
				viNewDelta[iAxis + 1] = p->m_LocationOrDelta.Y - m_aPositions[iButton].Y;
				m_aPositions[iButton] = p->m_LocationOrDelta;
			}
			break;

		default:
			// Nop
			break;
		};

		m_FreeBuffer.Push(p);

		p = m_InputBuffer.Pop();
	}

	// If the device isn't active, just clear everything we processed.
	if (m_bLostFocus)
	{
		vbNewState.Assign(false);
		m_bLostFocus = false;
	}

	// Update state changes.
	for (UInt32 i = 0u; i < vbNewState.GetSize(); ++i)
	{
		if (!vbUpdatedState[i])
		{
			m_aButtons[i].UpdateState(vbNewState[i]);
		}
	}

	// Set the new touch locations.
	UInt32 const uAxes = m_aAxes.GetSize();
	for (UInt32 i = 0u; i < uAxes; i += 2u)
	{
		m_aAxes[i+0].UpdateState(viNewDelta[i+0]);
		m_aAxes[1+1].UpdateState(viNewDelta[i+1]);
	}
}

UInt32 GenericMultiTouch::GetTouchCount() const
{
	// Button array starts with touches, then
	// has regular mouse buttons. Only check touches.
	auto const iSize = (Int)InputButton::TouchButtonLast - (Int)InputButton::TouchButtonFirst + 1;

	UInt32 uReturn = 0u;
	for (Int32 i = 0u; i < iSize; ++i)
	{
		if (m_aButtons[i].m_bPressed)
		{
			++uReturn;
		}
	}

	return uReturn;
}

/**
 * Insert a touch press/release event into the pending touch queue.
 */
void GenericMultiTouch::QueueTouchButtonEvent(InputButton eTouchButton, Bool bPressed)
{
	InputEvent* p = m_FreeBuffer.Pop();

	// If we've run out of buffer, ignore the event, as waiting for
	// the buffer to free up, we can hit a deadlock state if the user
	// has a message box displayed
	if (nullptr == p)
	{
		return;
	}

	p->m_LocationOrDelta = Point2DInt(0, 0);
	p->m_eTouch = eTouchButton;
	p->m_eEventType = (bPressed ? kPressed : kReleased);

	m_InputBuffer.Push(p);
}

/**
 * Insert a touch move event into the pending touch queue.
 */
void GenericMultiTouch::QueueTouchMoveEvent(InputButton eTouch, const Point2DInt& location)
{
	InputEvent* p = m_FreeBuffer.Pop();

	// If we've run out of buffer, ignore the event, as waiting for
	// the buffer to free up, we can hit a deadlock state if the user
	// has a message box displayed
	if (nullptr == p)
	{
		return;
	}

	p->m_LocationOrDelta = location;
	p->m_eTouch = eTouch;
	p->m_eEventType = kMove;

	m_InputBuffer.Push(p);
}

} // namespace Seoul
