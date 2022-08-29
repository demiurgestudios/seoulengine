/**
 * \file GameDevUIViewLocalization.cpp
 * \brief A developer UI view component with localization
 * tools for editing and debugging text.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DevUIImGui.h"
#include "DevUIUtil.h"
#include "FalconEditTextInstance.h"
#include "FalconMovieClipInstance.h"
#include "GameDevUIViewGameUI.h"
#include "GameDevUIViewLocalization.h"
#include "LocManager.h"
#include "ReflectionDefine.h"
#include "Renderer.h"
#include "UIManager.h"
#include "UIMovie.h"
#include "VmStats.h"

#if (SEOUL_ENABLE_DEV_UI && !SEOUL_SHIP)

namespace Seoul
{

SEOUL_BEGIN_TYPE(Game::DevUIViewLocalization, TypeFlags::kDisableCopy)
	SEOUL_ATTRIBUTE(DisplayName, "Localization")
	SEOUL_PARENT(DevUI::View)
SEOUL_END_TYPE()

namespace Game
{

DevUIViewLocalization::DevUIViewLocalization()
{
}

DevUIViewLocalization::~DevUIViewLocalization()
{
}

void DevUIViewLocalization::GetTokens(String const& sString, Vector<HString>& rvTokens) const
{
	auto pLocManager = LocManager::Get();
	pLocManager->DebugGetAllMatchingTokens(sString, rvTokens);
}

void DevUIViewLocalization::PoseEditText(CheckedPtr<Falcon::EditTextInstance const> pInstance, HashSet<HString>& foundTokens)
{
	using namespace ImGui;

	auto const& sXhtmlText = pInstance->GetXhtmlText();
	if (sXhtmlText.IsEmpty())
	{
		return;
	}

	Vector<HString> vTokens;
	GetTokens(sXhtmlText, vTokens);
	for (auto const& sToken : vTokens)
	{
		foundTokens.Insert(sToken);
	}
}

void DevUIViewLocalization::PoseMovieClip(CheckedPtr<Falcon::MovieClipInstance const> pInstance, HashSet<HString>& foundTokens)
{
	UInt32 const uChildren = pInstance->GetChildCount();
	for (UInt32 i = 0u; i < uChildren; ++i)
	{
		SharedPtr<Falcon::Instance> pChild;
		SEOUL_VERIFY(pInstance->GetChildAt((Int)i, pChild));
		PoseInstance(pChild.GetPtr(), foundTokens);
	}
}

void DevUIViewLocalization::PoseInstance(CheckedPtr<Falcon::Instance const> pInstance, HashSet<HString>& foundTokens)
{
	if (!pInstance.IsValid() || !pInstance->GetVisibleAndNotAlphaZero())
	{
		return;
	}

	switch (pInstance->GetType())
	{
	case Falcon::InstanceType::kMovieClip:
	{
		auto ptr(static_cast<Falcon::MovieClipInstance const*>(pInstance.Get()));
		PoseMovieClip(ptr, foundTokens);
		break;
	}
	case Falcon::InstanceType::kEditText:
	{
		auto ptr(static_cast<Falcon::EditTextInstance const*>(pInstance.Get()));
		PoseEditText(ptr, foundTokens);
		break;
	}
	default:
		break;
	};
}

void DevUIViewLocalization::DoPrePose(DevUI::Controller& rController, RenderPass& rPass)
{
	using namespace DevUI::Util;
	using namespace ImGui;

	CheckedPtr<Falcon::Instance> pInstance;

	auto pGameUi = DevUIViewGameUI::Get();
	if (pGameUi.IsValid())
	{
		pInstance = pGameUi->GetSelectedInstance().GetPtr();
	}

	if (pInstance.IsValid())
	{
		HashSet<HString> uniqueTokens;
		PoseInstance(pInstance, uniqueTokens);

		Text("Selection: %s", pInstance->GetName().CStr());
		NewLine();

		Text("Listed tokens are based on text search; not necessarily correct.");
		NewLine();

		if (uniqueTokens.IsEmpty())
		{
			Text("(no tokens found)");
		}
		else
		{
			Columns(2);
			Text("Token");
			NextColumn();
			Text("Text");
			NextColumn();
			Separator();

			auto pLocManager = LocManager::Get();
			Vector<HString> vSortedTokens;
			for (auto const& sToken : uniqueTokens)
			{
				vSortedTokens.PushBack(sToken);
			}
			Sort(vSortedTokens.Begin(), vSortedTokens.End(), [](HString a, HString b){ return strcmp(a.CStr(), b.CStr()) < 0; });

			for (auto const& sToken : vSortedTokens)
			{
				Text("%s", sToken.CStr());
				NextColumn();

				String sLocalizedToken;
				if (pLocManager.IsValid())
				{
					sLocalizedToken = pLocManager->Localize(sToken);
				}
				Text("%s", sLocalizedToken.CStr());
				NextColumn();
				Separator();
			}

			Columns();
		}
	}
	else
	{
		Text("Select a Movie Clip in the UI explorer (right click).");
	}
}

} // namespace Game

} // namespace Seoul

#endif // /(SEOUL_ENABLE_DEV_UI && !SEOUL_SHIP)
