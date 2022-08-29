/**
 * \file AndroidInput.cpp
 * \brief Handling of mouse and keyboard input for Android.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AndroidInput.h"
#include "GenericInput.h"

namespace Seoul
{

/*
 * Add input devices for local users
 */
void AndroidInputDeviceEnumerator::EnumerateDevices(InputDevices & rvDevices)
{
	rvDevices.PushBack(SEOUL_NEW(MemoryBudgets::Input) GenericKeyboard());
	rvDevices.PushBack(SEOUL_NEW(MemoryBudgets::Input) GenericMultiTouch());
}

} // namespace Seoul
