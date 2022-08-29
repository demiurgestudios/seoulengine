/**
 * \file GamePathsSettings.h
 * \brief Utility structure used to configure GamePaths.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GAME_PATHS_SETTINGS_H
#define GAME_PATHS_SETTINGS_H

#include "SeoulString.h"

namespace Seoul
{

struct GamePathsSettings SEOUL_SEALED
{
	GamePathsSettings()
		: m_sBaseDirectoryPath()
	{
	}

	String m_sBaseDirectoryPath;
}; // struct GamePathsSettings

} // namespace Seoul

#endif // include guard
