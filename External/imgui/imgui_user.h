/**
 * \file imgui_user.h
 * \brief Demiurge extension file for the imgui library.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma  once
#ifndef IMGUI_USER_H
#define IMGUI_USER_H

#include "imgui.h"

namespace ImGui
{

namespace ImageButtonAction
{

enum Enum
{
	kNone,
	kSelected = 1,
	kDoubleClicked = 2,
	kDragging = 3,
};

} // namespace ImageButtonAction

IMGUI_API bool CollapsingHeaderEx(const char* label, ImTextureID button_id, bool* p_button_activate, ImGuiTreeNodeFlags flags = 0);
IMGUI_API void DockWindowByName(const char* name, ImGuiID dockSpaceID);
IMGUI_API bool DragFloatNEx(
	const char* labels[],
	float* v,
	int components,
	float v_speed = 1.0f,
	float v_min = 0.0f,
	float v_max = 0.0f,
	const char* display_format = "%.3f",
	ImGuiInputTextFlags const* aflags = nullptr);
IMGUI_API float GetColumnMinX();
IMGUI_API float GetColumnMaxX();
IMGUI_API ImGuiID GetGlobalDockSpaceID(const char* name);
IMGUI_API bool ImageButtonEx(
	ImTextureID user_texture_id,
	const ImVec2& size,
	bool selected = false,
	bool enabled = true,
	const ImVec2& uv0 = ImVec2(0, 0),
	const ImVec2& uv1 = ImVec2(1, 1),
	int frame_padding = -1);
IMGUI_API ImageButtonAction::Enum ImageButtonWithLabel(
	ImTextureID user_texture_id,
	const ImVec2& size,
	const char* label,
	bool selected = false,
	bool enabled = true,
	const ImVec2& uv0 = ImVec2(0, 0),
	const ImVec2& uv1 = ImVec2(1, 1),
	int frame_padding = -1);
IMGUI_API bool ImageButtonCombo(
	ImTextureID user_texture_id,
	const ImVec2& size,
	int* current_item,
	bool(*items_getter)(void* data, int idx, const char** out_text),
	void* data,
	int items_count,
	int height_in_items = -1,
	bool enabled = true,
	bool indeterminate = false);
IMGUI_API float GetHoveredTime();
IMGUI_API bool GetWindowPosSizeByName(const char* name, ImVec2& pos, ImVec2& size, bool excludeTitleBar);
IMGUI_API bool InputUInt(const char* label, unsigned int* v, unsigned int step = 1, unsigned int step_fast = 100, ImGuiInputTextFlags flags = 0);
IMGUI_API bool IsItemHovered(ImGuiID id);
IMGUI_API bool IsMouseHoveringCursorRelative(const ImVec2& pos, const ImVec2& size);
IMGUI_API bool IsShortcutPressed(const char* shortcut);
IMGUI_API bool IsTreeNodeOpen(const char* label, ImGuiTreeNodeFlags flags = 0);
IMGUI_API bool IsWindowClicked(int mouse_button = 0);
IMGUI_API bool IsWindowMoving();
IMGUI_API bool IsWindowResizing();
IMGUI_API void LabelTextEx(const char* label, const char* fmt, ...);
IMGUI_API void LabelTextExV(const char* label, const char* fmt, va_list args);
IMGUI_API bool MenuItemEx(bool visible, const char* label, const char* shortcut = NULL, bool selected = false, bool enabled = true);
IMGUI_API bool MenuItemEx(bool visible, const char* label, const char* shortcut, bool* p_selected, bool enabled = true);
IMGUI_API void OpenPopupEx(const char* str_id, bool reopen_existing);
inline    void SeparatorEx(bool visible) { if (visible) Separator(); }
IMGUI_API void SetNextWindowBringToDisplayBack();
IMGUI_API bool ToolbarButton(ImTextureID texture, bool selected = false, bool enabled = true);
IMGUI_API bool TreeNodeImage(ImTextureID closedTexture, ImTextureID openTexture, const char* label, ImGuiTreeNodeFlags flags = 0);
IMGUI_API bool TreeNodeImageEx(ImTextureID closedTexture, ImTextureID openTexture, const char* label, ImTextureID button_id, bool* p_button_activate, ImGuiTreeNodeFlags flags = 0);
IMGUI_API void MenuBarImage(ImTextureID texture, const ImVec2& vSize);

} // namespace ImGui

#endif // include guard
