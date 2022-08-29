/**
 * \file CoreSettings.h
 * \brief Utility structure used to configure the Core singleton.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef CORE_SETTINGS_H
#define CORE_SETTINGS_H

#include "GamePathsSettings.h"
#include "SeoulString.h"

namespace Seoul
{

struct CoreSettings
{
	CoreSettings()
		: m_sLogName()
		, m_GamePathsSettings()
		, m_bOpenLogFile(true)
		, m_bLoadLoggerConfigurationFile(true)
	{
	}

	String m_sLogName;
	GamePathsSettings m_GamePathsSettings;
	Bool m_bOpenLogFile;
	Bool m_bLoadLoggerConfigurationFile;
}; // struct CoreSettings

} // namespace Seoul

#endif // include guard
