/**
 * \file EditorUIViewCommandHistory.cpp
 * \brief An editor view that displays
 * a tool for working with hierarchical data.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DevUIController.h"
#include "DevUICommand.h"
#include "DevUIImGui.h"
#include "DevUIImGuiRenderer.h"
#include "EditorUIRoot.h"
#include "EditorUIIcons.h"
#include "EditorUIViewCommandHistory.h"

namespace Seoul::EditorUI
{

// Move to a shared location.
static inline String ToMemoryString(UInt64 uSizeInBytes)
{
	if (uSizeInBytes > 1024 * 1024)
	{
		UInt32 const uSize((UInt32)(uSizeInBytes / (1024 * 1024)));
		return String::Printf("%u MB%s",
			uSize,
			(uSize == 1 ? "" : "s"));
	}
	else if (uSizeInBytes > 1024)
	{
		UInt32 const uSize((UInt32)(uSizeInBytes / 1024));
		return String::Printf("%u KB%s",
			uSize,
			(uSize == 1 ? "" : "s"));
	}
	else
	{
		UInt32 const uSize((UInt32)uSizeInBytes);
		return String::Printf("%u B%s",
			uSize,
			(uSize == 1 ? "" : "s"));
	}
}

ViewCommandHistory::ViewCommandHistory()
{
}

ViewCommandHistory::~ViewCommandHistory()
{
}

void ViewCommandHistory::DoPrePose(DevUI::Controller& rController, RenderPass& rPass)
{
	using namespace ImGui;

	DevUI::Command const* const pCurrentHead = rController.GetHeadCommand();
	DevUI::Command const* pRealHead = pCurrentHead;

	// Find the real head - which is the portion of the history
	// that we are behind (redo is possible).
	{
		while (nullptr != pRealHead)
		{
			if (pRealHead->GetNextCommand() == nullptr)
			{
				break;
			}
			pRealHead = pRealHead->GetNextCommand();
		}
	}

	// Now count all nodes.
	Int iCount = 0;
	{
		DevUI::Command const* p = pRealHead;
		while (nullptr != p && !p->GetDescription().IsEmpty())
		{
			++iCount;
			p = p->GetPrevCommand();
		}
	}

	// Tracking for clear confirmation.
	Bool bDisplayClearConfirm = false;

	// Display the list.
	if (iCount > 0)
	{
		DevUI::Command const* pNewHead = pCurrentHead;
		Bool bUndo = false;

		// Display summary info.
		Text("%d Command%s (%s)",
			iCount,
			(iCount == 1 ? "" : "s"),
			ToMemoryString(rController.GetCommandHistoryTotalSizeInBytes()).CStr());

		// Display the clear button.
		SameLine();

		// Align right.
		SetCursorPosX(GetCursorPosX() + GetContentRegionAvail().x - GetFontSize() - 2 * GetStyle().FramePadding.x);
		if (ImageButtonEx(
			Root::Get()->GetRenderer().ResolveTexture(Root::Get()->GetIcons().m_Trash),
			ImVec2(GetFontSize(), GetFontSize()),
			false,
			true))
		{
			bDisplayClearConfirm = true;
		}

		BeginChild("##List");
		{
			ImGuiListClipper clipper;
			clipper.Begin(iCount, GetFontSize());
			if (clipper.Step())
			{
				Int iOffset = 0;
				DevUI::Command const* p = pRealHead;
				Bool bSelected = false;
				while (nullptr != p && !p->GetDescription().IsEmpty())
				{
					// Once we hit the current head, all remaining nodes
					// are selected/active.
					if (pCurrentHead == p)
					{
						bSelected = true;
					}

					if (iOffset >= clipper.DisplayStart && iOffset < clipper.DisplayEnd)
					{
						String const sDescription(String::Printf("%d. %s",
							(iCount - iOffset),
							p->GetDescription().CStr()));
						if (Selectable(sDescription.CStr(), bSelected))
						{
							pNewHead = p;
							bUndo = bSelected;
						}
					}

					p = p->GetPrevCommand();
					++iOffset;
				}
			}
			clipper.End();
		}
		EndChild();

		// Undo/redo as necessary to set the new head, if specified.
		if (pNewHead != pCurrentHead)
		{
			if (bUndo)
			{
				while (rController.CanUndo() && rController.GetHeadCommand() != pNewHead)
				{
					rController.Undo();
				}
			}
			else
			{
				while (rController.CanRedo() && rController.GetHeadCommand() != pNewHead)
				{
					rController.Redo();
				}
			}
		}
	}

	// Clear confirmation handling.
	if (bDisplayClearConfirm)
	{
		OpenPopup("Clear History?");
	}
	if (BeginPopupModal("Clear History?", nullptr, ImGuiWindowFlags_NoResize))
	{
		Text("Clear the command history? This can't be undone!");

		if (Button("Yes"))
		{
			rController.ClearHistory();
			CloseCurrentPopup();
		}
		SameLine();
		if (Button("No"))
		{
			CloseCurrentPopup();
		}

		EndPopup();
	}
}

} // namespace Seoul::EditorUI
