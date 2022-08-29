/**
 * \file UIContext.cpp
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "RenderDevice.h"
#include "UIContext.h"
#include "UIManager.h"

namespace Seoul::UI
{

/* Developer hookage. */
static void DefaultDisplayNotification(const String& sMessage)
{
	// Nop
}

static void DefaultDisplayTrackedNotification(const String& sMessage, Int32& riId)
{
	riId = 0;
}

static void DefaultKillNotification(Int32 iId)
{
	// Nop
}

/** Default implementation returns the entire back buffer viewport. */
static Viewport DefaultGetRootViewport()
{
	return RenderDevice::Get()->GetBackBufferViewport();
}

/**
 * @return A root poseable that can be used to pose and render UI screens - in this
 * case, this always returns the global UI::Manager singleton.
 */
static IPoseable* DefaultSpawnUIManager(const DataStoreTableUtil&, Bool& rbRenderPassOwnsPoseableObject)
{
	rbRenderPassOwnsPoseableObject = false;
	return Manager::Get();
}

Context g_UIContext =
{
	&DefaultDisplayNotification,
	&DefaultDisplayTrackedNotification,
	&DefaultKillNotification,
	&DefaultGetRootViewport,
	&DefaultSpawnUIManager,
};

} // namespace Seoul::UI
