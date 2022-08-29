/**
 * \file IOSInput.h
 * \brief Handling of mouse and keyboard input for Google Native Client.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef IOS_INPUT_H
#define IOS_INPUT_H

#include "AtomicRingBuffer.h"
#include "InputManager.h"
#include "Platform.h"
#include "SeoulTypes.h"

namespace Seoul
{

class IOSInputDeviceEnumerator SEOUL_SEALED : public InputDeviceEnumerator
{
public:
	IOSInputDeviceEnumerator()
	{
	}

	~IOSInputDeviceEnumerator()
	{
	}

	virtual void EnumerateDevices(InputDevices& rvDevices) SEOUL_OVERRIDE;

private:
	SEOUL_DISABLE_COPY(IOSInputDeviceEnumerator);
}; // class IOSInputDeviceEnumerator

} // namespace Seoul

#endif // include guard
