/**
 * \file DevUIConfig.h
 * \brief Persistent configuration of parts of the developer
 * UI.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef DEV_UI_CONFIG_H
#define DEV_UI_CONFIG_H

#include "Prereqs.h"

#if SEOUL_ENABLE_DEV_UI

namespace Seoul::DevUI
{

// TODO: There are up values in this structure. Need
// to generalize the UI config in a way that allows specializations
// of DevUI::Root to specify members without the base DevUI::Root expressing
// them explicitly.
struct Config SEOUL_SEALED
{
	struct FxPreviewConfig
	{
		Bool m_bShowGame = false;
	};

	struct GlobalConfig
	{
		Bool m_bAutoHotLoad = false;
		Bool m_bUniqueLayoutForBranches = false;
	};

	struct ScreenshotConfig
	{
		Int32 m_iTargetHeight = -1;
		Bool m_bDedup = true;
	};

	FxPreviewConfig m_FxPreviewConfig;
	GlobalConfig m_GlobalConfig;
	ScreenshotConfig m_ScreenshotConfig;
}; // struct DevUI::Config

// Defined in DevUICommands.cpp
Config& GetDevUIConfig();
Bool SaveDevUIConfig();

} // namespace Seoul::DevUI

#endif // /SEOUL_ENABLE_DEV_UI

#endif // include guard
