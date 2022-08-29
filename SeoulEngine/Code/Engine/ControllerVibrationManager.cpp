/**
 * \file ControllerVibrationManager.cpp
 * \brief Manage the various sources of controller vibration
 * that can be in effect.  We do this so we don't have to store
 * vibration information in Users/Input Devices.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ControllerVibrationManager.h"

namespace Seoul
{

ControllerVibrationManager::ControllerVibrationManager()
	: m_bNeedsShutdown(false)
	, m_bEnabled(true)
{
}

ControllerVibrationManager::~ControllerVibrationManager()
{
	SEOUL_ASSERT(!m_bNeedsShutdown);
}

/**
 * Override in subclasses to do actual initialization work
 */
void ControllerVibrationManager::Initialize(void)
{
	SEOUL_ASSERT(!m_bNeedsShutdown);
	m_bNeedsShutdown=true;
}

/**
 * Override in subclasses to do actual shutdown work.  If Shutdown must
 *          be called if Initialize is called
 */
void ControllerVibrationManager::Shutdown(void)
{
	SEOUL_ASSERT(m_bNeedsShutdown);
	m_bNeedsShutdown=false;
}

/**
 * Base implementation - passed in vibration settings get populated
 *          with the desired controller rumble
 *
 * @param[out] vibrationSettings Device vibration settings to be populated with
 *                                   rumble parameters for the current frame
 * @param[in] uLocalID The local user ID that the settings should be computed for
 */
void ControllerVibrationManager::GetControllerVibration(
	VibrationSettings& vibrationSettings) const
{
	vibrationSettings.m_fLowFrequency=0.f;
	vibrationSettings.m_fHighFrequency=0.f;
}

} // namespace Seoul
