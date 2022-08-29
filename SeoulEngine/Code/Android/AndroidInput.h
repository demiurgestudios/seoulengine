/**
 * \file AndroidInput.h
 * \brief Handling of mouse and keyboard input for Android.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANDROID_INPUT_H
#define ANDROID_INPUT_H

#include "AtomicRingBuffer.h"
#include "InputManager.h"
#include "Platform.h"
#include "SeoulTypes.h"

namespace Seoul
{

class AndroidInputDeviceEnumerator SEOUL_SEALED : public InputDeviceEnumerator
{
public:
	AndroidInputDeviceEnumerator()
	{
	}

	~AndroidInputDeviceEnumerator()
	{
	}

	virtual void EnumerateDevices(InputDevices& rvDevices) SEOUL_OVERRIDE;

private:
	SEOUL_DISABLE_COPY(AndroidInputDeviceEnumerator);
}; // class AndroidInputDeviceEnumerator

} // namespace Seoul

#endif // include guard
