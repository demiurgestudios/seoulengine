/**
 * \file DevUIViewEngineStats.cpp
 * \brief A developer UI view component that displays
 * miscellaneous Engine stats (like object draw count).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DevUIImGui.h"
#include "DevUIUtil.h"
#include "DevUIViewEngineStats.h"
#include "ReflectionDefine.h"
#include "Renderer.h"
#include "VmStats.h"

#if (SEOUL_ENABLE_DEV_UI && !SEOUL_SHIP)

namespace Seoul
{

SEOUL_BEGIN_TYPE(DevUI::ViewEngineStats, TypeFlags::kDisableCopy)
	SEOUL_ATTRIBUTE(DisplayName, "Engine Stats")
	SEOUL_PARENT(DevUI::View)
SEOUL_END_TYPE()

namespace DevUI
{

ViewEngineStats::ViewEngineStats()
{
}

ViewEngineStats::~ViewEngineStats()
{
}

void ViewEngineStats::DoPrePose(Controller& rController, RenderPass& rPass)
{
	using namespace Util;
	using namespace ImGui;

	// Delegates section
	if (TreeNodeEx("Delegates", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ValueText("Allocated Count", "%d",
			(Int)DelegateMemberBindHandleTable::GetAllocatedCount());

		TreePop();
	}

	// Rendering section
	if (TreeNodeEx("Rendering", ImGuiTreeNodeFlags_DefaultOpen))
	{
		auto stats = Renderer::Get()->GetRenderStats();
		DataStore dataStore;
		if (Reflection::SerializeToDataStore(&stats, dataStore))
		{
			TextDataStore(dataStore, dataStore.GetRootNode());
		}

		TreePop();
	}

	// Strings section
	if (TreeNodeEx("Strings", ImGuiTreeNodeFlags_DefaultOpen))
	{
		auto stats = HString::GetHStringStats();
		DataStore dataStore;
		if (Reflection::SerializeToDataStore(&stats, dataStore))
		{
			TextDataStore(dataStore, dataStore.GetRootNode());
		}

		TreePop();
	}

	// Vm section
	if (TreeNodeEx("Vm", ImGuiTreeNodeFlags_DefaultOpen))
	{
		auto stats = g_VmStats;
		DataStore dataStore;
		if (Reflection::SerializeToDataStore(&stats, dataStore))
		{
			TextDataStore(dataStore, dataStore.GetRootNode());
		}

		TreePop();
	}
}

} // namespace DevUI

} // namespace Seoul

#endif // /(SEOUL_ENABLE_DEV_UI && !SEOUL_SHIP)
