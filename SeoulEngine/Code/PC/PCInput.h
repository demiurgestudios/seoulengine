/**
 * \file PCInput.h
 * \brief Handling of mouse and keyboard input for PC.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef PC_INPUT_H
#define PC_INPUT_H

#include "AtomicRingBuffer.h"
#include "FixedArray.h"
#include "InputManager.h"
#include "InputKeys.h"

namespace Seoul
{

class PCInputDeviceEnumerator SEOUL_SEALED : public InputDeviceEnumerator
{
public:
	PCInputDeviceEnumerator()
	{
	}

	~PCInputDeviceEnumerator()
	{
	}

	virtual void EnumerateDevices(InputDevices& rvDevices) SEOUL_OVERRIDE;

private:
	SEOUL_DISABLE_COPY(PCInputDeviceEnumerator);
}; // class PCInputDeviceEnumerator

} // namespace Seoul

#endif // include guard
