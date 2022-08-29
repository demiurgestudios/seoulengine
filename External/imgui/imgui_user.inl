/**
 * \file imgui_user.h
 * \brief Demiurge specific additions that need access to imgui.cpp.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef IMGUI_USER_INL
#define IMGUI_USER_INL

namespace ImGui
{

void DockWindowByName(const char* name, ImGuiID dockSpaceID)
{
	auto window = FindWindowByName(name);
	if (nullptr != window)
	{
		SetWindowDock(window, dockSpaceID, 0);
	}
	// Otherwise, associate the ID with the settings.
	else
	{
		auto settings = ImGui::FindWindowSettings(ImHashStr(name));
		if (nullptr == settings)
		{
			settings = ImGui::CreateNewWindowSettings(name);
		}

		settings->DockId = dockSpaceID;
	}
}

float GetColumnMinX()
{
	ImGuiWindow* window = GetCurrentWindow();
	ImGuiOldColumns* columns = window->DC.CurrentColumns;
	return columns->OffMinX;
}

float GetColumnMaxX()
{
	ImGuiWindow* window = GetCurrentWindow();
	ImGuiOldColumns* columns = window->DC.CurrentColumns;
	return columns->OffMaxX;
}

ImGuiID GetGlobalDockSpaceID(const char* name)
{
	return ImHashStr(name);
}

float GetHoveredTime()
{
	return GImGui->HoveredIdTimer;
}

float GetWindowsResizeFromEdgesHalfThickness()
{
	return WINDOWS_HOVER_PADDING;
}

bool GetWindowPosSizeByName(const char* name, ImVec2& pos, ImVec2& size, bool excludeTitleBar)
{
	auto window = FindWindowByName(name);
	if (nullptr == window)
	{
		return false;
	}

	pos = window->Pos;
	size = window->Size;

	if (excludeTitleBar && 0 == (ImGuiWindowFlags_NoTitleBar & window->Flags))
	{
		auto const height = window->TitleBarHeight();
		pos.y += height;
		size.y -= height;
	}

	return true;
}

static ImGuiKey GetImGuiKeyAndModifiers(const char* s, bool& alt, bool& ctrl, bool& shift)
{
	if (NULL == s)
	{
		return ImGuiKey_None;
	}

	while (*s)
	{
		// TODO: Cleanup.
		if (0 == ImStrnicmp(s, "alt", 3)) { alt = true; s += 3; }
		else if (0 == ImStrnicmp(s, "ctrl", 4)) { ctrl = true; s += 4; }
		else if (0 == ImStrnicmp(s, "shift", 5)) { shift = true; s += 5; }
		else if (*s == '+') { ++s; }
		else if (0 == ImStrnicmp(s, "del", 3)) { return ImGuiKey_Delete; }
		else if (0 == ImStrnicmp(s, "escape", 6)) { return ImGuiKey_Escape; }
		else if (0 == ImStrnicmp(s, "f1", 2)) { return ImGuiKey_F1; }
		else if (0 == ImStrnicmp(s, "f2", 2)) { return ImGuiKey_F2; }
		else if (0 == ImStrnicmp(s, "f3", 2)) { return ImGuiKey_F3; }
		else if (0 == ImStrnicmp(s, "f4", 2)) { return ImGuiKey_F4; }
		else if (0 == ImStrnicmp(s, "f5", 2)) { return ImGuiKey_F5; }
		else if (0 == ImStrnicmp(s, "f6", 2)) { return ImGuiKey_F6; }
		else if (0 == ImStrnicmp(s, "f7", 2)) { return ImGuiKey_F7; }
		else if (0 == ImStrnicmp(s, "f8", 2)) { return ImGuiKey_F8; }
		else if (0 == ImStrnicmp(s, "f9", 2)) { return ImGuiKey_F9; }
		else if (0 == ImStrnicmp(s, "f10", 3)) { return ImGuiKey_F10; }
		else if (0 == ImStrnicmp(s, "f11", 3)) { return ImGuiKey_F11; }
		else if (0 == ImStrnicmp(s, "f12", 3)) { return ImGuiKey_F12; }
		else if (*s >= 'a' && *s <= 'z') { return (ImGuiKey)(ImGuiKey_A + (*s - 'a')); }
		else if (*s >= 'A' && *s <= 'Z') { return (ImGuiKey)(ImGuiKey_A + (*s - 'A')); }
		else
		{
			break;
		}
	}

	return ImGuiKey_None;
}

bool InputUInt(const char* label, unsigned int* v, unsigned int step, unsigned int step_fast, ImGuiInputTextFlags flags)
{
	// Hexadecimal input provided as a convenience but the flag name is awkward. Typically you'd use InputText() to parse your own data, if you want to handle prefixes.
	const char* format = (flags & ImGuiInputTextFlags_CharsHexadecimal) ? "%08X" : "%d";
	return InputScalar(label, ImGuiDataType_U32, (void*)v, (void*)(step > 0.0f ? &step : nullptr), (void*)(step_fast > 0.0f ? &step_fast : nullptr), format, flags);
}

bool IsShortcutPressed(const char* shortcut)
{
	if (shortcut)
	{
		ImGuiContext& g = *GImGui;

		bool alt = false;
		bool ctrl = false;
		bool shift = false;
		ImGuiKey key = GetImGuiKeyAndModifiers(shortcut, alt, ctrl, shift);
		if (!g.IO.WantTextInput &&
			alt == g.IO.KeyAlt &&
			ctrl == g.IO.KeyCtrl &&
			shift == g.IO.KeyShift &&
			IsKeyPressed(key))
		{
			return true;
		}
	}

	return false;
}

bool IsWindowClicked(int mouse_button)
{
	return IsMouseClicked(mouse_button) && IsWindowHovered();
}

bool IsWindowMoving()
{
	ImGuiContext& g = *GImGui;
	return (nullptr != g.MovingWindow && g.MovingWindow == g.CurrentWindow);
}

// 0..3: corners (Lower-right, Lower-left, Unused, Unused)
// 4..7: borders (Top, Right, Bottom, Left)
static ImGuiID GetWindowResizeID(ImGuiWindow* window, int n)
{
    IM_ASSERT(n >= 0 && n <= 7);
    ImGuiID id = window->DockIsActive ? window->DockNode->HostWindow->ID : window->ID;
    id = ImHashStr("#RESIZE", 0, id);
    id = ImHashData(&n, sizeof(int), id);
    return id;
}

bool IsWindowResizing()
{
	ImGuiContext& g = *GImGui;
	for (int i = 0; i < 8; ++i)
	{
		auto const handleID = GetWindowResizeID(g.CurrentWindow, i);
		if (g.ActiveId == handleID)
		{
			return true;
		}
	}

	return false;
}

static bool MenuItemExShortcutHandler(bool pressed, bool visible, const char* shortcut, bool enabled)
{
	if (enabled && !pressed && shortcut)
	{
		pressed = IsShortcutPressed(shortcut);
		if (pressed)
		{
			if (visible)
			{
				ImGui::CloseCurrentPopup();
			}
		}
	}

	return pressed;
}

bool MenuItemEx(bool visible, const char* label, const char* shortcut, bool selected, bool enabled)
{
	bool pressed = false;
	if (visible)
	{
		pressed = MenuItem(label, shortcut, selected, enabled);
	}

	return MenuItemExShortcutHandler(pressed, visible, shortcut, enabled);
}

bool MenuItemEx(bool visible, const char* label, const char* shortcut, bool* p_selected, bool enabled)
{
	bool pressed = false;
	if (visible)
	{
		pressed = MenuItem(label, shortcut, p_selected, enabled);
	}

	return MenuItemExShortcutHandler(pressed, visible, shortcut, enabled);
}

void OpenPopupEx(const char* str_id, bool reopen_existing)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;
	ImGuiID id = window->GetID(str_id);
	int current_stack_size = g.OpenPopupStack.Size;
	if (g.OpenPopupStack.Size < current_stack_size + 1 ||
		(reopen_existing || g.OpenPopupStack[current_stack_size].PopupId != id))
	{
		OpenPopupEx(id);
	}
}

void SetNextWindowBringToDisplayBack()
{
	ImGuiContext& g = *GImGui;
	g.NextWindowData.Flags |= ImGuiNextWindowDataFlags_BringToDisplayBack;
}

void UndockWindowByName(const char* name)
{
	ImGuiContext& g = *GImGui;
	auto const id = ImHashStr(name);
	for (int i = g.Windows.Size - 1; i >= 0; i--)
	{
		ImGuiWindow* window = g.Windows[i];
		if (id == window->ID)
		{
			DockContextProcessUndockWindow(&g, window);
			return;
		}
	}
}

// When leaving virtualized mode, we need to undock all windows
// that are docked with the main form.
void UndockAllFromDockSpace(ImGuiID dockSpaceID)
{
	ImGuiContext& g = *GImGui;
	ImGuiDockNode* node = DockContextFindNodeByID(&g, dockSpaceID);
	if (nullptr == node)
	{
		return;
	}
	
	for (auto window : node->Windows)
	{
		DockContextProcessUndockWindow(&g, window);
	}
}

} // namespace ImGui

#endif
