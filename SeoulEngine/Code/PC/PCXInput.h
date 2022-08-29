/**
 * \file PCXInput.h
 * \brief Integration of controllers via the XInput API on the PC platform.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef PC_X_INPUT_H
#define PC_X_INPUT_H

#include "InputManager.h"
#include "SeoulTypes.h"

namespace Seoul
{

class PCXInputDeviceEnumerator : public InputDeviceEnumerator
{
public:
	virtual void EnumerateDevices(InputDevices & aDevices);

}; // class PCXInputDeviceEnumerator

} // namespace Seoul

#endif // include guard
