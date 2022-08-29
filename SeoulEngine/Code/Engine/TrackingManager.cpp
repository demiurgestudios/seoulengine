/**
 * \file TrackingManager.cpp
 * \brief Manager for user tracking. Complement to AnalyticsManager,
 * focused on user acquisition middleware.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AnalyticsManager.h"
#include "EventsManager.h"
#include "TrackingManager.h"

namespace Seoul
{

TrackingManager::~TrackingManager()
{
	SEOUL_ASSERT(IsMainThread());

	// Remove callback.
	Events::Manager::Get()->UnregisterCallback(
		AnalyticsSessionGameEventId,
		SEOUL_BIND_DELEGATE(&TrackingManager::OnSessionChange, this));
}

TrackingManager::TrackingManager()
{
	SEOUL_ASSERT(IsMainThread());

	// Configure session change callbacks now.
	Events::Manager::Get()->RegisterCallback(
		AnalyticsSessionGameEventId,
		SEOUL_BIND_DELEGATE(&TrackingManager::OnSessionChange, this));
}

} //namespace Seoul

