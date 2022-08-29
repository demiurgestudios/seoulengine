/**
 * \file FalconGlobalConfig.cpp
 * \brief Global accessors and up references for Falcon.
 *
 * The GlobalConfig must be set once before any Falcon functionality
 * is used and cannot be destroyed until all Falcon instances are
 * released.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FalconGlobalConfig.h"

namespace Seoul::Falcon
{

GlobalConfig g_Config = GlobalConfig();

GlobalConfig::GlobalConfig()
	: m_pGetFCNFile(nullptr)
	, m_pGetFont(nullptr)
	, m_pGetStage3DSettings(nullptr)
	, m_pGetTextEffectSettings(nullptr)
	, m_pResolveImageSource(nullptr)
{
}

} // namespace Seoul::Falcon
