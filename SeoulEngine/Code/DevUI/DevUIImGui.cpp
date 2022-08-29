/**
 * \file DevUIImGui.cpp
 * \brief DevUI imgui.h inclusion and overrides.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DevUIRoot.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "DevUIImGui.h"
#include "RenderCommandStreamBuilder.h"

#if SEOUL_ENABLE_DEV_UI

#ifndef IMGUI_DISABLE

#include "imgui_internal.h"

namespace ImGui
{

bool IsSpecificItemHovered(ImGuiID id)
{
	return GImGui->HoveredId == id;
}

bool DragUInt(const char* label, unsigned int* v, float v_speed, unsigned int v_min, unsigned int v_max, const char* display_format)
{
	if (!display_format)
		display_format = "%.0f";
	float v_f = (float)*v;
	bool value_changed = DragFloat(label, &v_f, v_speed, (float)v_min, (float)v_max, display_format);
	*v = (unsigned int)v_f;
	return value_changed;
}

static bool IsOverControlSurfaces(const ImVec2& pos, ImGuiWindow const* p)
{
	ImGuiContext& g = *GImGui;

	if (p->TitleBarRect().Contains(pos))
	{
		return true;
	}

	if (ImGuiWindowFlags_MenuBar == (ImGuiWindowFlags_MenuBar & p->Flags) &&
		p->MenuBarRect().Contains(pos))
	{
		return true;
	}

	// Feels like a cheat but the simplest way to the most accurate information
	// about whether we're hovering a resize control surface.
	if (g.MouseCursor > ImGuiMouseCursor_Arrow)
	{
		return true;
	}

	return false;
}

bool WillWantCaptureMousePos(const ImVec2& pos, const char** pname, bool* pb_client_area)
{
	ImGuiContext& g = *GImGui;

	// Easy cases.
	if (!g.OpenPopupStack.empty())
	{
		if (nullptr != pname) { *pname = nullptr; }
		if (nullptr != pb_client_area) { *pb_client_area = true; }
		return true;
	}
	if (g.MovingWindow && !(g.MovingWindow->Flags & ImGuiWindowFlags_NoMouseInputs))
	{
		if (nullptr != pname) { *pname = g.MovingWindow->Name; }
		if (nullptr != pb_client_area) { *pb_client_area = !IsOverControlSurfaces(pos, g.MovingWindow); }
		return true;
	}

	// Adaptation of body of FindHoveredWindow().
	auto const resizeFromEdgesHalfThickness = GetWindowsResizeFromEdgesHalfThickness();
	ImVec2 padding_regular = g.Style.TouchExtraPadding;
	ImVec2 padding_for_resize_from_edges = g.IO.ConfigWindowsResizeFromEdges ? ImMax(g.Style.TouchExtraPadding, ImVec2(resizeFromEdgesHalfThickness, resizeFromEdgesHalfThickness)) : padding_regular;
	for (int i = g.Windows.Size - 1; i >= 0; i--)
	{
		ImGuiWindow* pWindow = g.Windows[i];
		if (!pWindow->Active || pWindow->Hidden)
		{
			continue;
		}
		if (pWindow->Flags & ImGuiWindowFlags_NoMouseInputs)
		{
			continue;
		}

		ImRect bb(pWindow->OuterRectClipped);
		if (pWindow->Flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
		{
			bb.Expand(padding_regular);
		}
		else
		{
			bb.Expand(padding_for_resize_from_edges);
		}
		if (!bb.Contains(pos))
		{
			continue;
		}

		// Found a bHovered pWindow.
		if (nullptr != pname) { *pname = pWindow->Name; }
		if (nullptr != pb_client_area) { *pb_client_area = !IsOverControlSurfaces(pos, pWindow); }
		return true;
	}

	return false;
}

void PublicClearActiveID()
{
	ImGui::ClearActiveID();
}

void RegisterSettingsHandler(
	const char* name,
	void* (*readOpen)(ImGuiContext*, ImGuiSettingsHandler*, const char*),
	void(*readLine)(ImGuiContext*, ImGuiSettingsHandler*, void*, const char*),
	void(*writeAll)(ImGuiContext*, ImGuiSettingsHandler*, ImGuiTextBuffer*))
{
	ImGuiContext& g = *GImGui;
	IM_ASSERT(!g.SettingsLoaded);

	// Add .ini handle for persistent docking data
	ImGuiSettingsHandler ini_handler;
	ini_handler.TypeName = name;
	ini_handler.TypeHash = ImHashStr(name);
	ini_handler.ReadOpenFn = readOpen;
	ini_handler.ReadLineFn = readLine;
	ini_handler.WriteAllFn = writeAll;
	g.SettingsHandlers.push_back(ini_handler);
}

void ClampWindowPosTo(const ImVec2& pos, const ImVec2& size)
{
	auto const rect = ImRect(pos.x, pos.y, pos.x + size.x, pos.y + size.y);
	ImGuiContext& g = *GImGui;
	for (int i = g.Windows.Size - 1; i >= 0; i--)
	{
		ImGuiWindow* pWindow = g.Windows[i];
		if (pWindow->IsFallbackWindow || (ImGuiWindowFlags_NoMove == (ImGuiWindowFlags_NoMove & pWindow->Flags)))
		{
			continue;
		}

		// First check - try adjust by a full size.
		while (pWindow->Pos.x > rect.Max.x)
		{
			pWindow->Pos.x -= size.x;
		}
		while (pWindow->Pos.x + pWindow->Size.x < rect.Min.x)
		{
			pWindow->Pos.x += size.x;
		}
		while (pWindow->Pos.y > rect.Max.y)
		{
			pWindow->Pos.y -= size.y;
		}
		while (pWindow->Pos.y + pWindow->Size.y < rect.Min.y)
		{
			pWindow->Pos.y += size.y;
		}

		// Shift right and bottom.
		auto right = (pWindow->Pos.x + pWindow->Size.x);
		if (right > rect.Max.x)
		{
			pWindow->Pos.x -= (right - rect.Max.x);
			right = rect.Max.x;
		}
		auto bottom = (pWindow->Pos.y + pWindow->Size.y);
		if (bottom > rect.Max.y)
		{
			pWindow->Pos.y -= (bottom - rect.Max.y);
			bottom = rect.Max.y;
		}

		// Shift left and top.
		if (pWindow->Pos.x < pos.x)
		{
			pWindow->Pos.x += (pos.x - pWindow->Pos.x);
			right += (pos.x - pWindow->Pos.x);

			// If right is now outside, rescale size.
			if (right > rect.Max.x)
			{
				pWindow->Size.x -= (right - rect.Max.x);
				pWindow->Size.x = ImMax(1.0f, pWindow->Size.x);
			}
		}
		if (pWindow->Pos.y < pos.y)
		{
			pWindow->Pos.y += (pos.y - pWindow->Pos.y);
			bottom += (pos.y - pWindow->Pos.y);

			// If bottom is now outside, rescale size.
			if (bottom > rect.Max.y)
			{
				pWindow->Size.y -= (bottom - rect.Max.y);
				pWindow->Size.y = ImMax(1.0f, pWindow->Size.y);
			}
		}
	}
}

void OffsetAllWindowPos(const ImVec2& offset)
{
	ImGuiContext& g = *GImGui;
	for (int i = g.Windows.Size - 1; i >= 0; i--)
	{
		ImGuiWindow* pWindow = g.Windows[i];
		if (pWindow->IsFallbackWindow)
		{
			continue;
		}

		pWindow->Pos += offset;
	}
}

void GatherAllWindowRects(float fRescale, Seoul::OsWindowRegions& rv, char const* sMainFormName)
{
	using namespace Seoul;

	ImGuiID mainFormID = 0;
	if (nullptr != sMainFormName)
	{
		mainFormID = ImHashStr(sMainFormName);
	}

	ImGuiContext& g = *GImGui;
	auto const resizeFromEdgesHalfThickness = GetWindowsResizeFromEdgesHalfThickness();
	auto const paddingRegular = ImMax(g.Style.TouchExtraPadding.x, g.Style.TouchExtraPadding.y);
	auto const paddingWithResize = g.IO.ConfigWindowsResizeFromEdges
		? ImMax(paddingRegular, resizeFromEdgesHalfThickness)
		: paddingRegular;

	for (int i = g.Windows.Size - 1; i >= 0; i--)
	{
		ImGuiWindow* pWindow = g.Windows[i];
		if (pWindow->IsFallbackWindow || pWindow->Hidden || !pWindow->Active)
		{
			continue;
		}

		auto bb = pWindow->Rect();
		Float fMargin = 0.0f;
		if (0 == (pWindow->Flags & ImGuiWindowFlags_NoMouseInputs))
		{
			if (pWindow->Flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
			{
				fMargin = paddingRegular;
			}
			else
			{
				fMargin = paddingWithResize;
			}
		}

		// TODO: Unclear - it appears the pWindow rect is one pixel too big?
		bb.Expand(-1.0f);

		// +1 on right and bottom because generally in Seoul Engine, we
		// treat (right - left) of a pixel rectangle as the width but
		// ImGui does not (e.g. x <= right = inside the rect, where in
		// Seoul Engine, x < right = inside the rect).
		OsWindowRegion rgn;
		rgn.m_fInputMargin = fMargin;
		rgn.m_Rect = Rectangle2DInt(
			(Int32)(Floor(bb.Min.x) / fRescale),
			(Int32)(Floor(bb.Min.y) / fRescale),
			(Int32)(Ceil(bb.Max.x + 1) / fRescale),
			(Int32)(Ceil(bb.Max.y + 1) / fRescale));

		// Mark the main form.
		if (nullptr != sMainFormName && mainFormID == pWindow->ID)
		{
			rgn.m_bMainForm = true;
		}

		rv.PushBack(rgn);
	}
}

// Disable mouse hover behavior for the current frame.
void DisableMouseHover()
{
	ImGuiContext& g = *GImGui;
	g.NavDisableMouseHover = true;
}

// Current pWindow will submit draw commands.
bool IsWindowActiveAndVisible()
{
	ImGuiContext& g = *GImGui;
	if (nullptr == g.CurrentWindow)
	{
		return false;
	}

	return (g.CurrentWindow->Active) && (!g.CurrentWindow->Hidden);
}

bool BeginPopupModalEx(const char* name, const ImVec2& vCenterIfVirtualized, bool* p_open, ImGuiWindowFlags flags)
{
	using namespace Seoul;

	if (DevUI::Root::Get() && DevUI::Root::Get()->IsVirtualizedDesktop())
	{
		SetNextWindowPos(vCenterIfVirtualized, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	}

	return BeginPopupModal(name, p_open, flags);
}

ImVec2 GetWindowCenter()
{
	return GetWindowPos() + GetWindowSize() * 0.5f;
}

void NewLineEx(float advance)
{
	ImGuiWindow* pWindow = GetCurrentWindow();
	if (pWindow->SkipItems)
		return;

	const ImGuiLayoutType backup_layout_type = pWindow->DC.LayoutType;
	pWindow->DC.LayoutType = ImGuiLayoutType_Vertical;
	if (pWindow->DC.CurrLineSize.y > 0.0f)     // In the event that we are on a line with items that is smaller that FontSize high, we will preserve its height.
		ItemSize(ImVec2(0, 0));
	else
		ItemSize(ImVec2(0.0f, advance));
	pWindow->DC.LayoutType = backup_layout_type;
}

bool GetActiveWindowID(ImGuiID& windowID)
{
	ImGuiContext& g = *GImGui;
	auto pWindow = g.ActiveIdWindow;
	if (nullptr == pWindow)
	{
		return false;
	}

	windowID = pWindow->ID;
	return true;
}

bool GetWindowScrollValues(ImGuiID windowID, float& x, float& maxx, float& y, float& maxy)
{
	auto pWindow = FindWindowByID(windowID);
	if (nullptr != pWindow)
	{
		x = pWindow->Scroll.x;
		y = pWindow->Scroll.y;
		maxx = pWindow->ScrollMax.x;
		maxy = pWindow->ScrollMax.y;
		return true;
	}

	return false;
}

bool SetWindowScrollX(ImGuiID windowID, float x)
{
	auto pWindow = FindWindowByID(windowID);
	if (nullptr != pWindow)
	{
		pWindow->ScrollTarget.x = x;
		pWindow->ScrollTargetCenterRatio.x = 0.0f;
		return true;
	}

	return false;
}

bool SetWindowScrollY(ImGuiID windowID, float y)
{
	auto pWindow = FindWindowByID(windowID);
	if (nullptr != pWindow)
	{
		pWindow->ScrollTarget.y = y;
		pWindow->ScrollTargetCenterRatio.y = 0.0f;
		return true;
	}

	return false;
}

void LoadStateStorage(const Seoul::HashTable<Seoul::UInt64, Seoul::Int, Seoul::MemoryBudgets::DevUI>& stateStorage)
{
	for (auto const& pair : stateStorage)
	{
		ImGuiID const windowID = (ImGuiID)(pair.First >> (uint64_t)32);
		ImGuiID const stateID = (ImGuiID)(pair.First & 0x00000000FFFFFFFF);

		if (auto pWindow = FindWindowByID(windowID))
		{
			pWindow->StateStorage.SetInt(stateID, pair.Second);
		}
	}
}

void SaveStateStorage(Seoul::HashTable<Seoul::UInt64, Seoul::Int, Seoul::MemoryBudgets::DevUI>& rStateStorage)
{
	ImGuiContext& g = *GImGui;
	for (auto const& pWindow : g.Windows)
	{
		for (auto const& state : pWindow->StateStorage.Data)
		{
			SEOUL_STATIC_ASSERT(sizeof(pWindow->ID) == 4);
			auto const uId = ((uint64_t)pWindow->ID << (uint64_t)32) | (uint64_t)state.key;
			rStateStorage.Insert(uId, state.val_i);
		}
	}
}

ImVec2 GetWindowInnerRectMin()
{
	return GetCurrentWindow()->InnerRect.Min;
}

ImVec2 GetWindowInnerRectMax()
{
	return GetCurrentWindow()->InnerRect.Max;
}

static float GetStatusBarHeight()
{
	ImGuiWindow const* pWindow = GetCurrentWindowRead();

	ImGuiContext& g = *GImGui;
	return pWindow->CalcFontSize() + g.Style.FramePadding.y * 2.0f;
}

static ImRect StatusBarRect(ImGuiWindow const* pWindow)
{
	return ImRect(
		pWindow->Pos.x,
		pWindow->Pos.y + pWindow->SizeFull.y - GetStatusBarHeight(),
		pWindow->Pos.x + pWindow->SizeFull.x,
		pWindow->Pos.y + pWindow->SizeFull.y);
}

bool BeginStatusBar()
{
	ImGuiWindow* pWindow = GetCurrentWindow();
	if (pWindow->SkipItems)
		return false;

	BeginGroup();
	PushID("##statusbar");

	ImRect statusBarRect = StatusBarRect(pWindow);
	ImRect clip_rect(
		IM_ROUND(statusBarRect.Min.x + pWindow->WindowBorderSize),
		IM_ROUND(statusBarRect.Min.y),
		IM_ROUND(ImMax(statusBarRect.Min.x, statusBarRect.Max.x - ImMax(pWindow->WindowRounding, pWindow->WindowBorderSize))),
		IM_ROUND(statusBarRect.Max.y - pWindow->WindowBorderSize));
	clip_rect.ClipWith(pWindow->OuterRectClipped);
	PushClipRect(clip_rect.Min, clip_rect.Max, false);

	// Draw status bar background.
	{
		ImRect drawRect = statusBarRect;
		drawRect.ClipWith(pWindow->Rect());
		pWindow->DrawList->AddRectFilled(
			drawRect.Min + ImVec2(pWindow->WindowBorderSize, 0),
			drawRect.Max - ImVec2(pWindow->WindowBorderSize, pWindow->WindowBorderSize),
			GetColorU32(ImGuiCol_MenuBarBg),
			pWindow->WindowRounding,
			ImDrawFlags_RoundCornersBottom);
	}

	// We overwrite CursorMaxPos because BeginGroup sets it to CursorPos (essentially the .EmitItem hack in EndMenuBar() would need something analogous here, maybe a BeginGroupEx() with flags).
	pWindow->DC.CursorPos = pWindow->DC.CursorMaxPos = ImVec2(statusBarRect.Min.x + pWindow->WindowPadding.x, statusBarRect.Min.y);
	pWindow->DC.LayoutType = ImGuiLayoutType_Horizontal;
	pWindow->DC.IsSameLine = false;
	AlignTextToFramePadding();
	return true;
}

void EndStatusBar()
{
	ImGuiWindow* pWindow = GetCurrentWindow();
	if (pWindow->SkipItems)
		return;
	ImGuiContext& g = *GImGui;

	PopClipRect();
	PopID();
	g.GroupStack.back().EmitItem = false;
	EndGroup(); // Restore position on layer 0
	pWindow->DC.LayoutType = ImGuiLayoutType_Vertical;
	pWindow->DC.IsSameLine = false;
}

void DockSpaceEx(
	bool bStatusBar,
	ImGuiID id,
	ImGuiDockNodeFlags flags /*= 0*/,
	const ImGuiWindowClass* window_class /*= NULL*/)
{
	ImVec2 size(0, 0);
	if (bStatusBar)
	{
		// DockSpace size, when negative, applies an adjustment based
		// on content avialable. The status bar uses the entire pWindow region,
		// so we need to adjust total size based on the difference of the two.
		ImGuiWindow const* pWindow = GetCurrentWindowRead();
		auto const fStatusBarMinY = StatusBarRect(pWindow).Min.y;
		auto const fDefaultDockMaxY = pWindow->DC.CursorPos.y + GetContentRegionAvail().y;
		auto const fAdjustment = ImMin(0.0f, fStatusBarMinY - fDefaultDockMaxY);

		size.y += fAdjustment;
	}

	DockSpace(id, size, flags, window_class);
}

namespace
{

enum class MenuBarButtonType
{
	Close,
	Minimize,
	Maximize,
	Restore,
};

static const char* TypeToIdStr(MenuBarButtonType eType)
{
	switch (eType)
	{
	case MenuBarButtonType::Close: return "#CLOSE";
	case MenuBarButtonType::Minimize: return "#MINIMIZE";
	case MenuBarButtonType::Maximize: return "#MAXIMIZE";
	case MenuBarButtonType::Restore: return "#MAXIMIZE";
	default:
		return "";
	};
}

} // namespace anonymous

static bool MenuBarButton(MenuBarButtonType eType, const ImVec2& pos, float fDiameter, ImU32 uMenuBarBgCol)
{
	ImGuiWindow* pWindow = ImGui::GetCurrentWindow();
	ImGuiID const id = pWindow->GetID(TypeToIdStr(eType));

	const ImRect bb(pos, pos + ImVec2(fDiameter, fDiameter));

	const bool bAdded = ImGui::ItemAdd(bb, id);
	bool bHovered = false, bHeld = false;
	const bool bPressed = ImGui::ButtonBehavior(bb, id, &bHovered, &bHeld);
	if (!bAdded)
	{
		return bPressed;
	}

	const ImU32 uColor = (bHovered
		? ImGui::GetColorU32((bHeld && bHovered) ? ImGuiCol_ButtonActive : bHovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button)
		: uMenuBarBgCol);

	if (bHovered)
	{
		pWindow->DrawList->AddRectFilled(bb.GetTL(), bb.GetBR(), uColor);
	}

	const float fRadius = ImFloor(fDiameter * 0.5f);
	const ImVec2 vCenter = pos + ImVec2(fRadius, fRadius);
	const float fFillRadius = ImCeil(fRadius * 0.5f);
	const ImVec2 vFillRadius(fFillRadius, fFillRadius);
	const ImU32 uFillColor = ImGui::GetColorU32(ImGuiCol_Text);
	switch (eType)
	{
	case MenuBarButtonType::Close:
	{
		pWindow->DrawList->AddLine(
			vCenter - ImVec2(fFillRadius, fFillRadius),
			vCenter + ImVec2(fFillRadius, fFillRadius),
			uFillColor);
		pWindow->DrawList->AddLine(
			vCenter - ImVec2(fFillRadius, -fFillRadius),
			vCenter + ImVec2(fFillRadius, -fFillRadius),
			uFillColor);
	}
	break;
	case MenuBarButtonType::Minimize:
	{
		pWindow->DrawList->AddLine(
			vCenter - ImVec2(fFillRadius, 0),
			vCenter + ImVec2(fFillRadius, 0),
			uFillColor);
	}
	break;
	case MenuBarButtonType::Maximize:
	{
		auto const v0 = vCenter - vFillRadius;
		auto const v1 = vCenter + vFillRadius;

		pWindow->DrawList->AddRect(v0, v1, uFillColor);
	}
	break;
	case MenuBarButtonType::Restore:
	{
		auto const v0 = vCenter - vFillRadius;
		auto const v1 = vCenter + vFillRadius;

		pWindow->DrawList->AddRect(v0 + ImVec2(0.5f * fFillRadius, 0.0f), v1 - ImVec2(0.0f, 0.5f * fFillRadius), uFillColor);
		pWindow->DrawList->AddRectFilled(v0 + ImVec2(0.0f, 0.5f * fFillRadius), v1 - ImVec2(0.5f * fFillRadius, 0.0f), uColor);
		pWindow->DrawList->AddRect(v0 + ImVec2(0.0f, 0.5f * fFillRadius), v1 - ImVec2(0.5f * fFillRadius, 0.0f), uFillColor);
	}
	break;
	default:
		break;
	};

	return bPressed;
}

void MainMenuAsTitleBarControls(bool& rbOpened, bool& rbMinimized, bool& rbMaximized)
{
	const ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	ImGuiWindow const* pWindow = GetCurrentWindowRead();

	auto const rect = pWindow->MenuBarRect();
	auto const fDiameter = rect.GetHeight();
	auto const uMenuBarBgCol = GetColorU32(ImGuiCol_MenuBarBg);

	auto fPadRight = style.FramePadding.x + fDiameter;
	if (MenuBarButton(
		MenuBarButtonType::Close,
		ImVec2(rect.Max.x - fPadRight, rect.Min.y),
		fDiameter,
		uMenuBarBgCol))
	{
		rbOpened = false;
	}
	fPadRight += fDiameter;

	if (MenuBarButton(
		rbMaximized ? MenuBarButtonType::Restore : MenuBarButtonType::Maximize,
		ImVec2(rect.Max.x - fPadRight, rect.Min.y),
		fDiameter,
		uMenuBarBgCol))
	{
		rbMaximized = !rbMaximized;
	}
	fPadRight += fDiameter;

	if (MenuBarButton(
		MenuBarButtonType::Minimize,
		ImVec2(rect.Max.x - fPadRight, rect.Min.y),
		fDiameter,
		uMenuBarBgCol))
	{
		rbMinimized = true;
	}

	// Support double clicking the title/menu bar to toggle minimize/maximize.
	if (!IsAnyItemHovered() &&
		IsMouseDoubleClicked(ImGuiMouseButton_Left) &&
		IsMouseHoveringRect(rect.Min, rect.Max))
	{
		rbMaximized = !rbMaximized;
	}
}

} // namespace ImGui

#endif // /IMGUI_DISABLE

#endif // /SEOUL_ENABLE_DEV_UI
