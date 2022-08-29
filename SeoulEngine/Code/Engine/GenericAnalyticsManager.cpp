/**
 * \file GenericAnalyticsManager.cpp
 * \brief Shared data between Generic (HTTP based)
 * analytics managers.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AnalyticsManager.h"
#include "GenericAnalyticsManager.h"
#include "MixpanelAnalyticsManager.h"

namespace Seoul
{

AnalyticsManager* CreateGenericAnalyticsManager(const GenericAnalyticsManagerSettings& settings)
{
	auto const sApiKey(settings.m_GetApiKeyDelegate.IsValid() ? settings.m_GetApiKeyDelegate() : String());
	auto const bSendAnalytics(
		!settings.m_ShouldSendAnalyticsDelegate.IsValid() ||
		settings.m_ShouldSendAnalyticsDelegate());

	// Enable a "real" analytics system if we both have an API key
	// and the separate "should send" query returned true.
	if (bSendAnalytics && !sApiKey.IsEmpty())
	{
		switch (settings.m_eType)
		{
		case GenericAnalyticsManagerType::kMixpanel:
			return SEOUL_NEW(MemoryBudgets::Analytics) MixpanelAnalyticsManager(settings);
		case GenericAnalyticsManagerType::kNone: // fall-through
		default:
			return SEOUL_NEW(MemoryBudgets::Analytics) NullAnalyticsManager;
		};
	}
	else
	{
		return SEOUL_NEW(MemoryBudgets::Analytics) NullAnalyticsManager;
	}
}

} // namespace Seoul
