/**
 * \file InputDevice.cpp
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

#include "Engine.h"
#include "EventsManager.h"
#include "InputManager.h"
#include "InputDevice.h"
#include "Platform.h"
#include "ReflectionDefine.h"

namespace Seoul
{

SEOUL_TYPE(InputDevice, TypeFlags::kDisableNew);

SEOUL_TYPE(InputDevice::Axis)
SEOUL_TYPE(InputDevice::Button)
SEOUL_BEGIN_ENUM(InputDevice::Type)
	SEOUL_ENUM_N("Keyboard", InputDevice::Keyboard)
	SEOUL_ENUM_N("Mouse", InputDevice::Mouse)
	SEOUL_ENUM_N("Xbox360Controller", InputDevice::Xbox360Controller)
	SEOUL_ENUM_N("PS3Controller", InputDevice::PS3Controller)
	SEOUL_ENUM_N("PS3NavController", InputDevice::PS3NavController)
	SEOUL_ENUM_N("WiiRemote", InputDevice::WiiRemote)
	SEOUL_ENUM_N("GameController", InputDevice::GameController)
	SEOUL_ENUM_N("Unknown", InputDevice::Unknown)
SEOUL_END_ENUM()

/**
 * Default repeat delay for input buttons (500 milliseconds)
 */
const Float InputDevice::Button::s_fDefaultRepeatDelay = 0.500f;

/**
 * Default repeat rate for input buttons (30 milliseconds)
 */
const Float InputDevice::Button::s_fDefaultRepeatRate = 0.030f;

/**
 * Default Button constructor
 *
 * Default Button constructor.  Constructs a Button object with an unknown ID
 * and default properties.
 */
InputDevice::Button::Button(void)
	: m_ID(InputButton::ButtonUnknown),
	  m_BitFlag(0),
	  m_bPressed(false),
	  m_bPrevPressed(false),
	  m_bDoubleTogglePressed(false),
	  m_bUpdatedSinceLastCheck(false),
	  m_bHandled(false),
	  m_fRepeatDelay(s_fDefaultRepeatDelay),
	  m_fRepeatRate(s_fDefaultRepeatRate),
	  m_fTimeUntilRepeat(0.0f)
{
}

/**
 * Button constructor
 *
 * Button constructor.  Constructs a Button object with a given ID and default properties.
 */
InputDevice::Button::Button(InputButton buttonID)
	: m_ID(buttonID),
	  m_BitFlag(0),
	  m_bPressed(false),
	  m_bPrevPressed(false),
	  m_bDoubleTogglePressed(false),
	  m_bUpdatedSinceLastCheck(false),
	  m_bHandled(false),
	  m_fRepeatDelay(s_fDefaultRepeatDelay),
	  m_fRepeatRate(s_fDefaultRepeatRate),
	  m_fTimeUntilRepeat(0.0f)
{
}

/**
* Button constructor
*
* Button constructor.  Constructs a Button object with a given ID, the API's bitfield to test, and default properties.
*/
InputDevice::Button::Button(InputButton buttonID, UInt32 flag)
	: m_ID(buttonID),
	  m_BitFlag(flag),
	  m_bPressed(false),
	  m_bPrevPressed(false),
	  m_bDoubleTogglePressed(false),
	  m_bUpdatedSinceLastCheck(false),
	  m_bHandled(false),
	  m_fRepeatDelay(s_fDefaultRepeatDelay),
	  m_fRepeatRate(s_fDefaultRepeatRate),
	  m_fTimeUntilRepeat(0.0f)
{
}

/**
 * Updates the button's state
 *
 * Updates the button's state.  Its previous state is saved in m_bPrevState,
 * and if the button has newly become pressed, its repeat delay is reset.
 *
 * @param[in] bPressed The button's new pressed state
 */
void InputDevice::Button::UpdateState(Bool bPressed)
{
	// If this is the first event we've gotten this tick, save off the last state
	if(!m_bUpdatedSinceLastCheck)
	{
		m_bPrevPressed = m_bPressed;
	}
	// Otherwise, look to see if we've toggled back to our original state before having a chance
	// to fire off button events.
	else
	{
		m_bDoubleTogglePressed = (bPressed == m_bPrevPressed) && (bPressed != m_bPressed);
	}

	// Save off current state
	m_bPressed = bPressed;

	// Set flag that we've updated this button state at least once this tick.
	m_bUpdatedSinceLastCheck = true;

	// If we're just pressed now (regardless of if we've double toggled), start repeat timer.
	if(!m_bPrevPressed && bPressed)
	{
		m_fTimeUntilRepeat = m_fRepeatDelay;
	}
}

/**
 *  Default Axis constructor
 *
 * Default Axis constructor.  Constructs an Axis object with an unknown ID and
 * default properties.
 */
InputDevice::Axis::Axis(void)
	: m_ID(InputAxis::AxisUnknown)
	, m_nRawState(0)
	, m_nMinValue(-32768)
	, m_nMaxValue(32767)
	, m_eDeadZoneType(DeadZoneNone)
	, m_nDeadZoneMin(0)
	, m_nDeadZoneMax(0)
	, m_fDeadZoneSize(0.0f)
	, m_pDeadZoneSiblingAxis(nullptr)
	, m_fState(0.0f)
	, m_fPrevState(0.0f)
	, m_bHandled(false)
{
}

/**
 * Axis constructor
 *
 * Axis constructor.  Constructs an Axis object with a given ID and default
 * properties.
 */
InputDevice::Axis::Axis(InputAxis axisID)
	: m_ID(axisID)
	, m_nRawState(0)
	, m_nMinValue(-32768)
	, m_nMaxValue(32767)
	, m_eDeadZoneType(DeadZoneNone)
	, m_nDeadZoneMin(0)
	, m_nDeadZoneMax(0)
	, m_fDeadZoneSize(0.0f)
	, m_pDeadZoneSiblingAxis(nullptr)
	, m_fState(0.0f)
	, m_fPrevState(0.0f)
	, m_bHandled(false)
{
}

/**
 * Updates the axis' state given a particular raw state value
 *
 * Updates the axis' state given a particular raw state value.  The raw state
 * is clamped to the axis' minimum and maximum values.  If the state lies within
 * the axis' predefined dead zone, the value is snapped to the value
 * m_fDeadZoneValue.  Otherwise, the raw state is converted into a floating
 * point value between -1 and +1, where the minimum value maps to -1 and the
 * maximum value maps to +1.
 *
 * @param nRawState New raw state to apply
 */
void InputDevice::Axis::UpdateState(Int nRawState)
{
	m_nRawState = Clamp(nRawState, m_nMinValue, m_nMaxValue);
	m_fPrevState = m_fState;

	// Convert raw state into range [-1, 1] based on min and max values
	m_fState = (Float)(nRawState - m_nMinValue) / (m_nMaxValue - m_nMinValue) * 2.0f - 1.0f;

	Float fOtherState;
	switch(m_eDeadZoneType)
	{
	case DeadZoneNone:
		break;

	case DeadZoneSingleCentered:
	case DeadZoneSingleZeroBased:
		if(nRawState >= m_nDeadZoneMin && nRawState <= m_nDeadZoneMax)
		{
			m_fState = 0.0f;
		}
		break;

	case DeadZoneDualCircular:
	case DeadZoneDualSquare:
		fOtherState = (Float)(m_pDeadZoneSiblingAxis->m_nRawState - m_pDeadZoneSiblingAxis->m_nMinValue) / (m_pDeadZoneSiblingAxis->m_nMaxValue - m_pDeadZoneSiblingAxis->m_nMinValue) * 2.0f - 1.0f;
		if(m_eDeadZoneType == DeadZoneDualCircular)
		{
			Float fDistanceSquared = m_fState * m_fState + fOtherState * fOtherState;
			if(fDistanceSquared <= m_fDeadZoneSize)
			{
				m_fState = 0.0f;
				m_pDeadZoneSiblingAxis->m_fState = 0.0f;
			}
		}
		else  // DeadZoneDualSquare
		{
			if(Abs(m_fState) <= m_fDeadZoneSize && Abs(m_pDeadZoneSiblingAxis->m_fState) <= m_pDeadZoneSiblingAxis->m_fDeadZoneSize)
			{
				m_fState = 0.0f;
				m_pDeadZoneSiblingAxis->m_fState = 0.0f;
			}
		}
		break;

	default:
		SEOUL_FAIL("Invalid dead zone type");
		break;
	}
}

/**
 * Sets the range of the axis
 *
 * The raw state is clamped to the range set here, and then normalized by
 * dividing by the size of the range to get a value between -1 and 1.
 *
 * @param[in] nMin Minimum value of the axis range
 * @param[in] nMax Maximum value of the axis range
 */
void InputDevice::Axis::SetRange(Int nMin, Int nMax)
{
	SEOUL_ASSERT(nMin < nMax);
	m_nMinValue = nMin;
	m_nMaxValue = nMax;

	// should we call UpdateState() here?
}

/**
 * Updates the axis' state given a particular raw state value
 *
 * Updates the axis' state given a particular raw state value.  The raw state
 * is clamped to the axis' minimum and maximum values.  If the state lies within
 * the axis' predefined dead zone, the value is snapped to the value
 * m_fDeadZoneValue.  Otherwise, the raw state is converted into a floating
 * point value between 0 and 1, where the minimum value maps to 0 and the
 * maximum value maps to +1.
 *
 * @param nRawState New raw state to apply
 */
void InputDevice::Axis::UpdateZeroBasedState(Int nRawState)
{
	m_nRawState = Clamp(nRawState, m_nMinValue, m_nMaxValue);

	SEOUL_ASSERT(m_eDeadZoneType == DeadZoneSingleZeroBased);
	if(nRawState >= m_nDeadZoneMin && nRawState <= m_nDeadZoneMax)
	{
		m_fState = 0.0f;
	}
	else
	{
		// Convert raw state into range [0, 1] based on min and max values
		m_fState = (Float)(nRawState - m_nMinValue) / (Float)(m_nMaxValue - m_nMinValue);
	}
}

/**
 * Sets the axis' dead zone size
 *
 * Sets the axis' dead zone size, as a percentage of its range.  The dead zone
 * is centered about the center of the range.  For example, if the axis' range
 * is [0, 10000], and fDeadZoneSize is 0.25, then the dead zone will be
 * [2500, 7500].
 *
 * @param[in] fDeadZoneSize Size of the dead zone, expressed as a fraction of
 *            the axis' range.  Must be between 0 and 1.
 */
void InputDevice::Axis::SetDeadZone(Float fDeadZoneSize)
{
	SEOUL_ASSERT(fDeadZoneSize >= 0.0f && fDeadZoneSize <= 1.0f);
	m_eDeadZoneType = DeadZoneSingleCentered;

	Float fCenter = (Float)(m_nMinValue + m_nMaxValue) / 2.0f;
	Float fRange = (Float)(m_nMaxValue - m_nMinValue);

	m_nDeadZoneMin = (Int)(fCenter - fDeadZoneSize * fRange / 2.0f);
	m_nDeadZoneMax = (Int)(fCenter + fDeadZoneSize * fRange / 2.0f);
}

/**
 * Sets the axis' dead zone size
 *
 * Sets the axis' dead zone size, as a percentage of its range.  The dead zone
 * is at the bottom of the range.  For example, if the axis' range is [0, 10000],
 * and fDeadZoneSize is 0.25, then the dead zone will be [0, 2500].
 *
 * @param[in] fDeadZoneSize Size of the dead zone, expressed as a fraction of
 *            the axis' range.  Must be between 0 and 1.
 */
void InputDevice::Axis::SetZeroBasedDeadZone(Float fDeadZoneSize)
{
	SEOUL_ASSERT(fDeadZoneSize >= 0.0f && fDeadZoneSize <= 1.0f);
	m_eDeadZoneType = DeadZoneSingleZeroBased;

	Float fRange = (Float)(m_nMaxValue - m_nMinValue);

	m_nDeadZoneMin = 0;
	m_nDeadZoneMax = (Int)(fDeadZoneSize * fRange);
}

/**
 * Sets the axis' dead zone to a circular region, when combined with
 *        another axis
 *
 * This dead zone for the pair of axes defined by this and pOtherAxis is
 * set to a circular region centered at (0, 0).  The diameter of the dead zone
 * is specified by the fDeadZoneDiamater parameter.  The two axes are
 * presumed to return normalized values in the range [-1, 1]; the diameter is
 * measured as a fraction of this range, so a diameter of 1 specifies that all
 * pairs of normalized values (x, y) for the two axes are clamped to (0, 0) if
 * x<sup>2</sup> + y<sup>2</sup> <= 1.
 *
 * @param[in] pOtherAxis Pointer to the other axis that is associated with this
 *                       axis for the purposes of defining the dead zone.  For
 *                       example, this might be XboxThumbstickX, and
 *                       pOtherAxis might be XboxThumbstickY.
 * @param[in] fDeadZoneDiamater Diameter of the deadzone; see comments above for
 *                              units.  Must be in the range [0, 1].
 */
void InputDevice::Axis::SetCircularDeadZoneWithAxis(Axis *pOtherAxis, Float fDeadZoneDiamater)
{
	SEOUL_ASSERT(pOtherAxis != nullptr && fDeadZoneDiamater >= 0.0f && fDeadZoneDiamater <= 1.0f);

	m_eDeadZoneType = DeadZoneDualCircular;
	m_fDeadZoneSize = fDeadZoneDiamater * fDeadZoneDiamater;  // square here to avoid squaring in UpdateState()
	m_pDeadZoneSiblingAxis = pOtherAxis;

	pOtherAxis->m_eDeadZoneType = DeadZoneDualCircular;
	pOtherAxis->m_fDeadZoneSize = m_fDeadZoneSize;
	pOtherAxis->m_pDeadZoneSiblingAxis = this;
}

/**
 * Sets the axis' dead zone to a square region, when combined with
 *        another axis
 *
 * This dead zone for the pair of axes defined by this and pOtherAxis is
 * set to a square region centered at (0, 0).  The width of the dead zone
 * is specified by the fDeadZoneDiamater parameter.  The two axes are
 * presumed to return normalized values in the range [-1, 1]; the width is
 * measured as a fraction of this range, so a width of 0.5 specifies that all
 * pairs of normalized values (x, y) for the two axes are clamped to (0, 0) if
 * -0.5 <= x <= 0.5 and -0.5 <= y <= 0.5
 *
 * @param[in] pOtherAxis Pointer to the other axis that is associated with this
 *                       axis for the purposes of defining the dead zone.  For
 *                       example, this might be XboxThumbstickX, and
 *                       pOtherAxis might be XboxThumbstickY.
 * @param[in] fDeadZoneDiamater Size of the deadzone; see comments above for
 *                              units.  Must be in the range [0, 1].
 */
void InputDevice::Axis::SetSquareDeadZoneWithAxis(Axis *pOtherAxis, Float fDeadZoneSize)
{
	SEOUL_ASSERT(pOtherAxis != nullptr && fDeadZoneSize >= 0.0f && fDeadZoneSize <= 1.0f);

	m_eDeadZoneType = DeadZoneDualSquare;
	m_fDeadZoneSize = fDeadZoneSize;
	m_pDeadZoneSiblingAxis = pOtherAxis;

	pOtherAxis->m_eDeadZoneType = DeadZoneDualSquare;
	pOtherAxis->m_fDeadZoneSize = fDeadZoneSize;
	pOtherAxis->m_pDeadZoneSiblingAxis = this;
}

/**
 * InputDevice constructor
 *
 * InputDevice constructor.  Initializes the input device to a given device type
 * with no buttons or axes. Adds this InputDevice to the list of InputDevices.
 */
InputDevice::InputDevice(Type eDeviceType)
	: m_eDeviceType(eDeviceType)
	, m_bConnected(true)
	, m_bWasConnected(true)
	, m_bVibrationEnabled(true)
{
}

/**
 * InputDevice destructor
 *
 * Removes this InputDevice from the list of InputDevices.
 */
InputDevice::~InputDevice(void)
{
}

/**
 * Gets a button by ID
 *
 * Gets a button by ID.  If no button with the given ID is present on this
 * device, nullptr is returned.
 *
 * @param buttonID ID of the button to find
 *
 * @return A pointer to the button with the given ID, or nullptr if no such button
 *         is present
 */
InputDevice::Button *InputDevice::GetButton(InputButton buttonID)
{
	for(UInt nButton = 0; nButton < m_aButtons.GetSize(); nButton++)
	{
		if(m_aButtons[nButton].m_ID == buttonID)
		{
			return &m_aButtons[nButton];
		}
	}

	return nullptr;
}

/**
 * Gets an axis by ID
 *
 * Gets an axis by ID.  If no axis with the given ID is present on this device,
 * nullptr is returned.
 *
 * @param axisID ID of the axis to find
 *
 * @return A pointer to the axis with the given ID, or nullptr if no such button
 *         is present
 */
InputDevice::Axis const* InputDevice::GetAxis(InputAxis axisID) const
{
	for(UInt nAxis = 0; nAxis < m_aAxes.GetSize(); nAxis++)
	{
		if(m_aAxes[nAxis].GetID() == axisID)
		{
			return &m_aAxes[nAxis];
		}
	}

	return nullptr;
}

/**
 * Gets an axis by ID
 *
 * Gets an axis by ID.  If no axis with the given ID is present on this device,
 * nullptr is returned.
 *
 * @param axisID ID of the axis to find
 *
 * @return A pointer to the axis with the given ID, or nullptr if no such button
 *         is present
 */
InputDevice::Axis* InputDevice::GetAxis(InputAxis axisID)
{
	for(UInt nAxis = 0; nAxis < m_aAxes.GetSize(); nAxis++)
	{
		if(m_aAxes[nAxis].GetID() == axisID)
		{
			return &m_aAxes[nAxis];
		}
	}

	return nullptr;
}


/**
 * Sets the axis' dead zone for the specified axis in this input device
 *
 * @param[in] eAxis Axis enum
 * @param[in] fDeadZoneSize Size of the dead zone, expressed as a fraction of
 *            the axis' range.  Must be between 0 and 1.
 */
void InputDevice::SetAxisDeadZone(InputAxis eAxis, Float fDeadZoneSize)
{
	InputDevice::Axis *pAxis;

	pAxis = GetAxis(eAxis);
	SEOUL_ASSERT_MESSAGE(pAxis, "ERROR: unknown axis name for this input device");
	if(pAxis)
	{
		pAxis->SetDeadZone(fDeadZoneSize);
	}
}

/**
 * Sets the axis' dead zone for the specified axis in this input device
 *
 * @param[in] eAxis Axis enum
 * @param[in] fDeadZoneSize Size of the dead zone, expressed as a fraction of
 *            the axis' range.  Must be between 0 and 1.
 */
void InputDevice::SetZeroBasedAxisDeadZone(InputAxis eAxis, Float fDeadZoneSize)
{
	InputDevice::Axis *pAxis;

	pAxis = GetAxis(eAxis);
	SEOUL_ASSERT_MESSAGE(pAxis, "ERROR: unknown axis name for this input device");
	if(pAxis)
	{
		pAxis->SetZeroBasedDeadZone(fDeadZoneSize);
	}
}

void InputDevice::SetTwoAxisDeadZoneCircular(InputAxis eAxis1, InputAxis eAxis2, Float fDeadZoneDiameter)
{
	InputDevice::Axis *pAxis1, *pAxis2;

	pAxis1 = GetAxis(eAxis1);
	pAxis2 = GetAxis(eAxis2);
	SEOUL_ASSERT_MESSAGE(pAxis1 != nullptr && pAxis2 != nullptr, "ERROR: unknown axis name(s) for this input device");
	if(pAxis1 != nullptr && pAxis2 != nullptr)
	{
		pAxis1->SetCircularDeadZoneWithAxis(pAxis2, fDeadZoneDiameter);
	}
}

void InputDevice::SetTwoAxisDeadZoneSquare(InputAxis eAxis1, InputAxis eAxis2, Float fDeadZoneDiameter)
{
	InputDevice::Axis *pAxis1, *pAxis2;

	pAxis1 = GetAxis(eAxis1);
	pAxis2 = GetAxis(eAxis2);
	SEOUL_ASSERT_MESSAGE(pAxis1 != nullptr && pAxis2 != nullptr, "ERROR: unknown axis name(s) for this input device");
	if(pAxis1 != nullptr && pAxis2 != nullptr)
	{
		pAxis1->SetSquareDeadZoneWithAxis(pAxis2, fDeadZoneDiameter);
	}
}

/**
 * Ticks this input device
 *
 * Ticks this input device.  This tests all of the input buttons for changes; if
 * their states have changed, this generates button pressed/released events.
 * This also generates button repeated events, for buttons which have been
 * pressed for a sufficiently long time.  This also generates exactly one event
 * per input axis.
 *
 * @param fTime Time to tick the input system by
 */
void InputDevice::Tick(Float fTime)
{
	// If the Input system binding lock is enabled, do not dispatch input events.
	if (InputManager::Get()->HasSystemBindingLock())
	{
		return;
	}

	// Button events
	for(UInt nButton = 0; nButton < m_aButtons.GetSize(); nButton++)
	{
		// Reset the handled flag to false.
		m_aButtons[nButton].m_bHandled = false;

		// Reset update check
		m_aButtons[nButton].m_bUpdatedSinceLastCheck = false;

		// Check for state change - either button pressed or button released
		if(m_aButtons[nButton].m_bPressed != m_aButtons[nButton].m_bPrevPressed)
		{
			m_aButtons[nButton].m_bHandled = Events::Manager::Get()->TriggerEvent(g_EventButtonEvent, this, m_aButtons[nButton].m_ID, m_aButtons[nButton].m_bPressed ? ButtonEventType::ButtonPressed : ButtonEventType::ButtonReleased);
		}
		// If our button is in the same state it was before but we double toggled it during the last tick, make sure that event is not lost.
		else if(m_aButtons[nButton].m_bDoubleTogglePressed)
		{
			if(m_aButtons[nButton].m_bPrevPressed)
			{
				m_aButtons[nButton].m_bHandled = Events::Manager::Get()->TriggerEvent(g_EventButtonEvent, this, m_aButtons[nButton].m_ID, ButtonEventType::ButtonReleased);
				m_aButtons[nButton].m_bHandled = Events::Manager::Get()->TriggerEvent(g_EventButtonEvent, this, m_aButtons[nButton].m_ID, ButtonEventType::ButtonPressed);
			}
			else
			{
				m_aButtons[nButton].m_bHandled = Events::Manager::Get()->TriggerEvent(g_EventButtonEvent, this, m_aButtons[nButton].m_ID, ButtonEventType::ButtonPressed);
				m_aButtons[nButton].m_bHandled = Events::Manager::Get()->TriggerEvent(g_EventButtonEvent, this, m_aButtons[nButton].m_ID, ButtonEventType::ButtonReleased);
			}

			m_aButtons[nButton].m_bDoubleTogglePressed = false;
		}
		// Check for ButtonRepeat events. Only repeat keypresses.
		else if(m_aButtons[nButton].m_bPressed &&
			GetDeviceType() == Keyboard)
		{
			m_aButtons[nButton].m_fTimeUntilRepeat -= fTime;

			while(m_aButtons[nButton].m_fTimeUntilRepeat <= 0.0f)
			{
				m_aButtons[nButton].m_fTimeUntilRepeat += m_aButtons[nButton].m_fRepeatRate;
				m_aButtons[nButton].m_bHandled = Events::Manager::Get()->TriggerEvent(g_EventButtonEvent, this, m_aButtons[nButton].m_ID, ButtonEventType::ButtonRepeat) || m_aButtons[nButton].m_bHandled;
			}
		}
	}

	// Axis events
	for(UInt nAxis = 0; nAxis < m_aAxes.GetSize(); nAxis++)
	{
		m_aAxes[nAxis].SetHandled(Events::Manager::Get()->TriggerEvent(g_EventAxisEvent, this, &m_aAxes[nAxis]));
	}
}

/**
 * Tests if the given button is currently pressed
 *
 * Tests if the given button is currently pressed.
 * NOTE: Think twice about calling this directly from game code. You should be going through the InputManager...
 *
 * @param[in] buttonID Button to test
 *
 * @return True if buttonID is currently pressed, or false of buttonID is
 *         not currently pressed
 */
Bool InputDevice::IsButtonDown(InputButton buttonID, Bool bReturnFalseIfHandled /*= false*/) const
{
	for(UInt nButton = 0; nButton < m_aButtons.GetSize(); nButton++)
	{
		if(m_aButtons[nButton].m_ID == buttonID)
		{
			// If we treat this button as not done when it was handled during input dispatch,
			// return false.
			if (bReturnFalseIfHandled && m_aButtons[nButton].m_bHandled)
			{
				return false;
			}

			return m_aButtons[nButton].m_bPressed;
		}
	}
	return false;
}

/**
 * Tests if the given button was pressed during the last tick
 *
 * Tests if the given button was pressed during the last tick
 * NOTE: Think twice about calling this directly from game code. You should be going through the InputManager...
 *
 * @param[in] buttonID Button to test
 *
 * @return True if buttonID was pressed during the last tick, or false
 *         otherwise
 */
Bool InputDevice::WasButtonPressed(InputButton buttonID, Bool bReturnFalseIfHandled /*= false*/) const
{
	for(UInt nButton = 0; nButton < m_aButtons.GetSize(); nButton++)
	{
		if(m_aButtons[nButton].m_ID == buttonID)
		{
			// If we treat this button as not done when it was handled during input dispatch,
			// return false.
			if (bReturnFalseIfHandled && m_aButtons[nButton].m_bHandled)
			{
				return false;
			}

			return m_aButtons[nButton].m_bPressed && !m_aButtons[nButton].m_bPrevPressed;
		}
	}
	return false;
}

/**
 * Tests if the given button was released during the last tick
 *
 * Tests if the given button was released during the last tick
 * NOTE: Think twice about calling this directly from game code. You should be going through the InputManager...
 *
 * @param[in] buttonID Button to test
 *
 * @return True if buttonID was released during the last tick, or false
 *         otherwise
 */
Bool InputDevice::WasButtonReleased(InputButton buttonID, Bool bReturnFalseIfHandled /*= false*/) const
{
	for(UInt nButton = 0; nButton < m_aButtons.GetSize(); nButton++)
	{
		if(m_aButtons[nButton].m_ID == buttonID)
		{
			// If we treat this button as not done when it was handled during input dispatch,
			// return false.
			if (bReturnFalseIfHandled && m_aButtons[nButton].m_bHandled)
			{
				return false;
			}

			return !m_aButtons[nButton].m_bPressed && m_aButtons[nButton].m_bPrevPressed;
		}
	}
	return false;
}

/**
 * Determine which button was pressed most recently. Used in the controls
 * configuration screen.
 *
 * @return the InputButton pressed most recently on this device
 */
Bool InputDevice::GetLastButtonPressed(InputButton& button) const
{
#if SEOUL_PLATFORM_WINDOWS
	// Special case for the European left alt key which acts as
	// LCtrl+RAlt. Don't let them bind anything to this key.
	Bool bIgnoreAltGr = false;
	if(m_aButtons.GetSize() > VK_RMENU && m_aButtons.GetSize() > VK_LCONTROL
		&& m_aButtons[VK_RMENU].m_bPressed && m_aButtons[VK_LCONTROL].m_bPressed)
	{
		bIgnoreAltGr = true;
	}

	// Special case for the ^2 key on French keyboards. The font cant render it
	// so ignore it.
	Bool bIgnoreOEM_7 = false;
	char sName[KL_NAMELENGTH];
	static const String sFrenchLayoutName("0000040C");
	if(GetKeyboardLayoutName(sName) && sName == sFrenchLayoutName)
	{
		bIgnoreOEM_7 = true;
	}
#endif

	for(UInt nButton = 0; nButton < m_aButtons.GetSize(); nButton++)
	{
		// Only check valid buttons
		if (m_aButtons[nButton].m_ID != InputButton::ButtonUnknown && m_aButtons[nButton].m_bPressed)
		{
#if SEOUL_PLATFORM_WINDOWS
			// Check special cases
			if((!bIgnoreAltGr || (nButton != VK_RMENU && nButton != VK_LCONTROL))
				&& (!bIgnoreOEM_7 || nButton != VK_OEM_7))
			{
				button = m_aButtons[nButton].m_ID;
				return true;
			}
#else
			button = m_aButtons[nButton].m_ID;
			return true;
#endif
		}
	}

	button = InputButton::ButtonUnknown;
	return false;
}

/**
 * Returns whether or not vibration is enabled for this device
 *
 * Checks both m_bVibrationEnabled and the device type to see if vibration
 *   should be computed for this device
 *
 */
Bool InputDevice::VibrationEnabled() const
{
	if(!m_bVibrationEnabled)
	{
		return false;
	}

	switch (m_eDeviceType)
	{
	case Xbox360Controller: // Pass through
	case PS3Controller: // Pass through
	case WiiRemote:
		return true;
	case Keyboard: // Pass through
	case Mouse: // Pass through
	case GameController: // Pass through
	case Unknown: // Pass through
	default:
		return false;
	}
}

/** MouseDevice constructor */
MouseDevice::MouseDevice()
	: InputDevice(InputDevice::Mouse)
{
}

/** MouseDevice destructor */
MouseDevice::~MouseDevice()
{
}

MultiTouchDevice::MultiTouchDevice()
	: MouseDevice()
{
}

MultiTouchDevice::~MultiTouchDevice()
{
}

} // namespace Seoul
