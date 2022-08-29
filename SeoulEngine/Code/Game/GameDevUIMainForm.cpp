/**
 * \file GameDevUIMainForm.cpp
 * \brief Specialization of DevUIMainForm for GameDevUI.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DevUIController.h"
#include "DevUIImGui.h"
#include "DevUIRoot.h"
#include "DevUIViewCommands.h"
#include "GameDevUIMainForm.h"
#include "ReflectionAttributes.h"
#include "ReflectionRegistry.h"
#include "ReflectionType.h"

namespace Seoul::Game
{

static DevUI::MainForm::Views CollectViewTypes()
{
	using namespace Reflection;

	DevUI::MainForm::Views vViews;
	UInt32 const uCount = Registry::GetRegistry().GetTypeCount();
	for (UInt32 i = 0u; i < uCount; ++i)
	{
		auto pType = Registry::GetRegistry().GetType(i);
		if (pType->IsSubclassOf(TypeOf<DevUI::View>()))
		{
			HString name = pType->GetName();
			if (auto pName = pType->GetAttribute<Attributes::DisplayName>())
			{
				name = pName->m_DisplayName;
			}

			DevUI::MainForm::ViewEntry entry;
			entry.m_sPrunedName = String(name).ReplaceAll("&", String());
			entry.m_Name = name;
			entry.m_pView.Reset(pType->New<DevUI::View>(MemoryBudgets::DevUI));
			SEOUL_ASSERT(entry.m_pView.IsValid());
			vViews.PushBack(entry);
		}
	}

	// Now sort lexographically.
	QuickSort(vViews.Begin(), vViews.End());

	// Assign.
	return vViews;
}

DevUIMainForm::DevUIMainForm()
	: Seoul::DevUI::MainForm(CollectViewTypes())
	, m_pController(SEOUL_NEW(MemoryBudgets::DevUI) DevUI::NullController)
{
}

DevUIMainForm::~DevUIMainForm()
{
}

void DevUIMainForm::PrePoseMainMenu()
{
	InternalPrePoseViewsMenu();
}

void DevUIMainForm::ImGuiPrepForLoadSettings()
{
	for (auto& e : GetViews())
	{
		// All views not open by default.
		e.m_pView->SetOpen(false);

#if (SEOUL_ENABLE_CHEATS)
		// On mobile, commands is open by default
		// (won't be visible until overall dev UI is made visible).
		if (DevUI::Root::Get()->IsMobile())
		{
			if (e.m_pView->GetId() == DevUI::ViewCommands::StaticGetId())
			{
				e.m_pView->SetOpen(true);
			}
		}
#endif // /(SEOUL_ENABLE_CHEATS)
	}
}

void DevUIMainForm::InternalPrePoseViewsMenu()
{
	using namespace ImGui;

	auto const bMobile = DevUI::Root::Get()->IsMobile();
	auto const& vViews = GetViews();
	Bool const bEnabled = !vViews.IsEmpty();

	Bool bBegin = false;

	// On mobile, place the title in the menu slot,
	// since users can't see the title bar of a window.
	if (bMobile)
	{
		// Prefix, hamburger placeholder using uppercase Greek Xi, followed by a space.
		static UInt8 const aPrefix[] = { 0xCE, 0x9E, 0x20, 0 };

		auto openId = InternalFindFirstOpened();
		if (openId.IsEmpty())
		{
			bBegin = BeginMenu(String::Printf("%sViews###Views", (Byte const*)aPrefix).CStr(), bEnabled);
		}
		else
		{
			bBegin = BeginMenu(String::Printf("%s%s###Views", (Byte const*)aPrefix, openId.CStr()).CStr(), bEnabled);
		}
	}
	else
	{
		bBegin = BeginMenu("Views", bEnabled);
	}

	if (bBegin)
	{
		ViewEntry const* pOpened = nullptr;
		for (auto const& e : vViews)
		{
			// Skip views that are always open.
			if (e.m_pView->IsAlwaysOpen())
			{
				continue;
			}

			Bool bSelected = (e.m_pView->IsOpen());
			if (MenuItem(
				e.m_Name.CStr(),
				nullptr,
				&bSelected,
				true))
			{
				// On mobile, only allowed to toggle, not close.
				if (!bMobile || bSelected)
				{
					e.m_pView->SetOpen(bSelected);
					if (bSelected)
					{
						pOpened = &e;
					}
				}
			}
		}

		// On mobile, if one view was opened, all the rest
		// must be closed.
		if (bMobile && nullptr != pOpened)
		{
			for (auto const& e : vViews)
			{
				// Skip views that area always open.
				if (e.m_pView->IsAlwaysOpen())
				{
					continue;
				}

				// Skip the opened view.
				if (&e == pOpened)
				{
					continue;
				}

				// Otherwise, close.
				e.m_pView->SetOpen(false);
			}
		}

		EndMenu();
	}
}

} // namespace Seoul::Game
