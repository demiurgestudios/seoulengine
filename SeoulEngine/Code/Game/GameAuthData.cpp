/**
 * \file GameAuthData.cpp
 * \brief Data requested from the server that is required for
 * server authentication and client configuration.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "GameAuthData.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(Game::AuthDataRefresh)
	SEOUL_PROPERTY_N("VariationString", m_sVariationString)
		SEOUL_ATTRIBUTE(NotRequired)
	SEOUL_PROPERTY_N("ConfigUpdateUrl", m_sConfigUpdateUrl)
	SEOUL_PROPERTY_N("ContentUpdateUrl", m_sContentUpdateUrl)
	SEOUL_PROPERTY_N("ABTests", m_tABTests)
		SEOUL_ATTRIBUTE(NotRequired)
	SEOUL_PROPERTY_N("RemapConfigs", m_vRemapConfigs)
		SEOUL_ATTRIBUTE(NotRequired)
	SEOUL_PROPERTY_N("VersionRecommended", m_VersionRecommended)
		SEOUL_ATTRIBUTE(NotRequired)
	SEOUL_PROPERTY_N("VersionRequired", m_VersionRequired)
		SEOUL_ATTRIBUTE(NotRequired)
	SEOUL_PROPERTY_N("AnalyticsSandboxed", m_bAnalyticsSandboxed)
		SEOUL_ATTRIBUTE(NotRequired)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Game::AuthData)
	SEOUL_PROPERTY_N("AuthToken", m_sAuthToken)
	SEOUL_PROPERTY_N("AnalyticsGuid", m_sAnalyticsGuid)
	SEOUL_PROPERTY_N("RefreshData", m_RefreshData)
SEOUL_END_TYPE()

} // namespace Seoul
