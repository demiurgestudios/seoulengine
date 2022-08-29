/**
 * \file PCInput.cpp
 * \brief Handling of mouse and keyboard input for PC.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Engine.h"
#include "GenericInput.h"
#include "PCInput.h"
#include "RenderDevice.h"
#include "Thread.h"

namespace Seoul
{

/**
 * Enumerates input devices
 *
 * Enumerates input devices.  All input devices which are found are stored in
 * the aDevices parameter.
 *
 * @param[out] aDevices Vector in which to store pointers to the new input
 *                      devices.  The calling function is responsible for
 *                      deleting the device pointers.
 */
void PCInputDeviceEnumerator::EnumerateDevices(InputDevices& rvDevices)
{
	rvDevices.PushBack(SEOUL_NEW(MemoryBudgets::Input) GenericKeyboard());
	rvDevices.PushBack(SEOUL_NEW(MemoryBudgets::Input) GenericMouse());
}

} // namespace Seoul
