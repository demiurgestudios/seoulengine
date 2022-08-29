/**
 * \file DevUIConfig.cpp
 * \brief Persistent configuration of parts of the developer
 * UI.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DevUIConfig.h"
#include "ReflectionDefine.h"

#if SEOUL_ENABLE_DEV_UI

namespace Seoul
{

SEOUL_BEGIN_TYPE(DevUI::Config::FxPreviewConfig)
	SEOUL_ATTRIBUTE(NotRequired)
	SEOUL_PROPERTY_N("ShowGame", m_bShowGame)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(DevUI::Config::GlobalConfig)
	SEOUL_ATTRIBUTE(NotRequired)
	SEOUL_PROPERTY_N("AutoHotLoad", m_bAutoHotLoad)
	SEOUL_PROPERTY_N("UniqueLayoutForBranches", m_bUniqueLayoutForBranches)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(DevUI::Config::ScreenshotConfig)
	SEOUL_ATTRIBUTE(NotRequired)
	SEOUL_PROPERTY_N("TargetHeight", m_iTargetHeight)
	SEOUL_PROPERTY_N("Dedup", m_bDedup)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(DevUI::Config)
	SEOUL_ATTRIBUTE(NotRequired)
	SEOUL_PROPERTY_N("FxPreviewConfig", m_FxPreviewConfig)
	SEOUL_PROPERTY_N("GlobalConfig", m_GlobalConfig)
	SEOUL_PROPERTY_N("ScreenshotConfig", m_ScreenshotConfig)
SEOUL_END_TYPE()

} // namespace Seoul

#endif // /SEOUL_ENABLE_DEV_UI
