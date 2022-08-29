/**
 * \file ControllerVibrationManager.h
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

#pragma once
#ifndef CONTROLLER_VIBRATION_MANAGER_H
#define CONTROLLER_VIBRATION_MANAGER_H

#include "Prereqs.h"
#include "Singleton.h"

namespace Seoul
{

class ControllerVibrationManager SEOUL_ABSTRACT : public Singleton<ControllerVibrationManager>
{
public:
	/**
	 * Simple struct to use to pass vibrations settings around
	 */
	struct VibrationSettings
	{
		Float m_fLowFrequency;
		Float m_fHighFrequency;
	};

	ControllerVibrationManager(void);
	virtual ~ControllerVibrationManager(void);
	virtual void Initialize(void);
	virtual void Shutdown(void);
	virtual void GetControllerVibration(
		VibrationSettings& rVibrationSettings) const;
	virtual void Disable() {m_bEnabled=false;}
	virtual void Enable() {m_bEnabled=true;}
	virtual Bool IsEnabled() const {return m_bEnabled;}

protected:
	/** Whether the ControllerVibrationManager needs to be shutdown
	 *    checked in the destructor to ensure that matching initialize
	 *    and shutdown calls get made
	 */
	Bool m_bNeedsShutdown;

	/**
	 * Used to disable all controller vibrations
	 */
	Bool m_bEnabled;

private:
	SEOUL_DISABLE_COPY(ControllerVibrationManager);
}; // class ControllerVibrationManager

} // namespace Seoul

#endif // include guard
