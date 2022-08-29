/**
 * \file DevUIViewMemoryUsage.cpp
 * \brief A developer UI view component that displays
 * the runtime's current memory usage info.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DevUIImGui.h"
#include "DevUIUtil.h"
#include "EventsManager.h"
#include "DevUIViewMemoryUsage.h"
#include "ReflectionDefine.h"
#include "TextureManager.h"
#include "ToString.h"

#if (SEOUL_ENABLE_DEV_UI && !SEOUL_SHIP)

namespace Seoul
{

SEOUL_BEGIN_TYPE(DevUI::ViewMemoryUsage, TypeFlags::kDisableCopy)
	SEOUL_ATTRIBUTE(DisplayName, "Memory Usage")
	SEOUL_PARENT(DevUI::View)
SEOUL_END_TYPE()

namespace DevUI
{

#if SEOUL_ENABLE_MEMORY_TOOLING
static inline String GetMemoryUsageString(size_t zSizeInBytes)
{
	// Handling for difference thresholds.
	if (zSizeInBytes > 1024 * 1024) { return ToString(zSizeInBytes / (1024 * 1024)) + " MBs"; }
	else if (zSizeInBytes > 1024) { return ToString(zSizeInBytes / 1024) + " KBs"; }
	else { return ToString(zSizeInBytes) + " Bs"; }
}
#endif // /#if SEOUL_ENABLE_MEMORY_TOOLING

ViewMemoryUsage::ViewMemoryUsage()
{
}

ViewMemoryUsage::~ViewMemoryUsage()
{
}

void ViewMemoryUsage::DoPrePose(Controller& rController, RenderPass& rPass)
{
#if SEOUL_ENABLE_MEMORY_TOOLING
	using namespace Util;
	using namespace ImGui;

	// Get and cache computed values.
	UInt32 uTextureMemoryUsage = 0u;
	(void)TextureManager::Get()->GetTextureMemoryUsageInBytes(uTextureMemoryUsage);

	size_t const zTotalUsageInBytes = MemoryManager::GetTotalUsageInBytes();
	size_t const zTotalSecondaryInBytes = (size_t)uTextureMemoryUsage;

	// Text - total usage.
	Text("Total Usage: %s", GetMemoryUsageString(zTotalUsageInBytes + zTotalSecondaryInBytes).CStr());

	// Main memory usage.
	Text("Main Memory Usage: %s", GetMemoryUsageString(zTotalUsageInBytes).CStr());

	for (Int i = MemoryBudgets::FirstType; i <= MemoryBudgets::LastType; ++i)
	{
		ValueText(MemoryBudgets::ToString((MemoryBudgets::Type)i), "%s(%d)",
			GetMemoryUsageString(
				(size_t)MemoryManager::GetUsageInBytes((MemoryBudgets::Type)i)).CStr(),
				(Int)MemoryManager::GetAllocations((MemoryBudgets::Type)i));
	}

	// Secondary memory usage.
	Text("Secondary Memory Usage: %s", GetMemoryUsageString(
		zTotalSecondaryInBytes).CStr());

	// Textures
	ValueText("Textures", "%s", GetMemoryUsageString(
		(size_t)uTextureMemoryUsage).CStr());
#endif // /#if SEOUL_ENABLE_MEMORY_TOOLING
}

} // namespace DevUI

} // namespace Seoul

#endif // /(SEOUL_ENABLE_DEV_UI && !SEOUL_SHIP)
