/**
 * \file IOSInput.cpp
 * \brief Handling of mouse and keyboard input for Apple iOS.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "GenericInput.h"
#include "IOSInput.h"

namespace Seoul
{

/*
 * Add input devices for local users
 */
void IOSInputDeviceEnumerator::EnumerateDevices(InputDevices & rvDevices)
{
	rvDevices.PushBack(SEOUL_NEW(MemoryBudgets::Input) GenericKeyboard());
	rvDevices.PushBack(SEOUL_NEW(MemoryBudgets::Input) GenericMultiTouch());
}

} // namespace Seoul
