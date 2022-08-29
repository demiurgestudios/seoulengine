/**
 * \file PCXInput.cpp
 * \brief Integration of controllers via the XInput API on the PC platform.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ControllerVibrationManager.h"
#include "Core.h"
#include "Engine.h"
#include "Platform.h"
#include "PCXInput.h"
#include "SeoulMath.h"

#include <XInput.h>

namespace Seoul
{

/** Fixed pad index for Xbox controller input. */
static const UInt32 kuPadNumber = 0u;

/**
 * Generic input device using XInput
 */
class PCXInputDevice SEOUL_SEALED : public InputDevice
{
public:
	PCXInputDevice();
	~PCXInputDevice();

	virtual void Tick(Float fTime) SEOUL_OVERRIDE;
	virtual void Poll() SEOUL_OVERRIDE;

private:
	XINPUT_STATE m_State;
};

/**
 * PCXInputDevice constructor. Creates an Xbox360 controller device.
 *
 * @param[in] ID The new controller's ID
 */
PCXInputDevice::PCXInputDevice()
	: InputDevice(Xbox360Controller)
{
	memset(&m_State, 0, sizeof(m_State));
	SEOUL_ASSERT(m_aButtons.IsEmpty());

	m_aButtons.Reserve(14u);
	m_aButtons.PushBack(Button(InputButton::XboxA, XINPUT_GAMEPAD_A));
	m_aButtons.PushBack(Button(InputButton::XboxB, XINPUT_GAMEPAD_B));
	m_aButtons.PushBack(Button(InputButton::XboxX, XINPUT_GAMEPAD_X));
	m_aButtons.PushBack(Button(InputButton::XboxY, XINPUT_GAMEPAD_Y));
	m_aButtons.PushBack(Button(InputButton::XboxLeftBumper, XINPUT_GAMEPAD_LEFT_SHOULDER));
	m_aButtons.PushBack(Button(InputButton::XboxRightBumper, XINPUT_GAMEPAD_RIGHT_SHOULDER));
	m_aButtons.PushBack(Button(InputButton::XboxBack, XINPUT_GAMEPAD_BACK));
	m_aButtons.PushBack(Button(InputButton::XboxStart, XINPUT_GAMEPAD_START));
	m_aButtons.PushBack(Button(InputButton::XboxLeftThumbstickButton, XINPUT_GAMEPAD_LEFT_THUMB));
	m_aButtons.PushBack(Button(InputButton::XboxRightThumbstickButton, XINPUT_GAMEPAD_RIGHT_THUMB));
	m_aButtons.PushBack(Button(InputButton::XboxLeftTrigger, 0));
	m_aButtons.PushBack(Button(InputButton::XboxRightTrigger, 0));
	m_aButtons.PushBack(Button(InputButton::XboxDpadUp, XINPUT_GAMEPAD_DPAD_UP));
	m_aButtons.PushBack(Button(InputButton::XboxDpadDown, XINPUT_GAMEPAD_DPAD_DOWN));
	m_aButtons.PushBack(Button(InputButton::XboxDpadLeft, XINPUT_GAMEPAD_DPAD_LEFT));
	m_aButtons.PushBack(Button(InputButton::XboxDpadRight, XINPUT_GAMEPAD_DPAD_RIGHT));

	SEOUL_ASSERT(m_aAxes.IsEmpty());
	m_aAxes.Reserve(6u);
	m_aAxes.PushBack(Axis(InputAxis::XboxLeftThumbstickX));
	m_aAxes.PushBack(Axis(InputAxis::XboxLeftThumbstickY));
	m_aAxes.PushBack(Axis(InputAxis::XboxRightThumbstickX));
	m_aAxes.PushBack(Axis(InputAxis::XboxRightThumbstickY));
	m_aAxes.PushBack(Axis(InputAxis::XboxLeftTriggerZ));
	m_aAxes.PushBack(Axis(InputAxis::XboxRightTriggerZ));

	m_aAxes[4].SetRange(0, 255);
	m_aAxes[5].SetRange(0, 255);

	DWORD uResult = XInputGetState(kuPadNumber, &m_State);

	m_bConnected = (ERROR_SUCCESS == uResult);
	m_bWasConnected = m_bConnected;
}


/**
 * PCXInputDevice destructor
 */
PCXInputDevice::~PCXInputDevice(void)
{
	m_aButtons.Clear();
	m_aAxes.Clear();
}


/**
 * Poll an xinput device and update the buttons & axes
 */
void PCXInputDevice::Poll()
{
	m_bWasConnected = m_bConnected;

	// Initially the controller is not connected.
	DWORD res = ERROR_DEVICE_NOT_CONNECTED;

	// If we're rescanning for controllers, or if the controller was connected
	// the last time we checked, get the controller's state.
	if(InputManager::Get()->ShouldRescan() || m_bConnected)
	{
		// get the state
		res = XInputGetState(kuPadNumber, &m_State);
	}

	// Success indicates the controller is connected
	if(res == ERROR_SUCCESS)
	{
		// connected
		m_bConnected = true;

		// Only update the state if my game window was focus
		if(Engine::Get()->HasFocus())
		{
			// update axes state
			m_aAxes[0].UpdateState(m_State.Gamepad.sThumbLX);
			m_aAxes[1].UpdateState(m_State.Gamepad.sThumbLY);
			m_aAxes[2].UpdateState(m_State.Gamepad.sThumbRX);
			m_aAxes[3].UpdateState(m_State.Gamepad.sThumbRY);
			m_aAxes[4].UpdateZeroBasedState(m_State.Gamepad.bLeftTrigger);
			m_aAxes[5].UpdateZeroBasedState(m_State.Gamepad.bRightTrigger);

			// update button state
			for(UInt i = 0; i < m_aButtons.GetSize(); i++)
			{
				if( m_aButtons[i].m_ID == InputButton::XboxLeftTrigger)
				{
					m_aButtons[i].UpdateState(m_aAxes[4].GetState() > 0);
				}
				else if(m_aButtons[i].m_ID == InputButton::XboxRightTrigger)
				{
					m_aButtons[i].UpdateState(m_aAxes[5].GetState() > 0);
				}
				if( m_aButtons[i].m_BitFlag )
				{
					m_aButtons[i].UpdateState( 0 != (m_State.Gamepad.wButtons & m_aButtons[i].m_BitFlag) );
				}
			}

			// Successfully polled - return. Falling through indicates a disconnected
			// or out-of-focus window, which resets the controller state.
			return;
		}
	}
	// According to the XInput example code, any other value indicates the controller
	// is not connected.
	else
	{
		// not connected
		m_bConnected = false;
	}

	// Reset controller state when the window loses focus or if it is not connected.

	// update axes state
	m_aAxes[0].UpdateState(0);
	m_aAxes[1].UpdateState(0);
	m_aAxes[2].UpdateState(0);
	m_aAxes[3].UpdateState(0);
	m_aAxes[4].UpdateZeroBasedState(0);
	m_aAxes[5].UpdateZeroBasedState(0);

	// update button state
	for(UInt i = 0; i < m_aButtons.GetSize(); i++)
	{
		m_aButtons[i].UpdateState(false);
	}
}

/**
 * Update controller vibration for this controller's user if it is
 *         connected and assigned
 *
 * \sa InputDevice::Tick
 */
void PCXInputDevice::Tick(Float fTime)
{
	InputDevice::Tick(fTime);

	// Only set vibration state if the controller is currently connected - all
	// XInput related functions are otherwise very, very expensive, so we
	// need to minimize when they are called on a disconnected controller.
	if (!m_bConnected)
	{
		return;
	}

	// XINPUT vibration takes a WORD, so we need to multiply by 0xFFFF
	//   because our vibration parameters are percentages of full vibration
	static const WORD kMaxVibration = 65535;
	XINPUT_VIBRATION vibration;
	vibration.wLeftMotorSpeed = 0;
	vibration.wRightMotorSpeed = 0;

	// If we are connected, assigned to a user and vibration is enabled
	//   update the vibration percentage
	if(VibrationEnabled() && ControllerVibrationManager::Get() && InputManager::Get())
	{
		ControllerVibrationManager::VibrationSettings vibrationSettings;

		ControllerVibrationManager::Get()->GetControllerVibration(vibrationSettings);

		vibration.wLeftMotorSpeed = (WORD)(vibrationSettings.m_fLowFrequency * kMaxVibration);
		vibration.wRightMotorSpeed = (WORD)(vibrationSettings.m_fHighFrequency * kMaxVibration);
	}

	// Always reset vibration, even if it's 0.  This will ensure
	//    that we don't accidentally leave the controller vibrating
	//    during pauses/level transitions etc. (which would fail cert)
	XInputSetState(kuPadNumber, &vibration);
}

/**
* Create all of the allowable xinput devices allowed and add them to the input'd device list
*
* Also makes sure that the device is not already in the list
*
* @param[in/out] aDevices List of input devices to be updates
*/
void PCXInputDeviceEnumerator::EnumerateDevices(InputDevices& rvDevices)
{
	rvDevices.PushBack(SEOUL_NEW(MemoryBudgets::Input) PCXInputDevice());
}

} // namespace Seoul
