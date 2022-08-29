/**
 * \file DevUIImGui.h
 * \brief DevUI imgui.h inclusion and overrides.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef DEV_UI_IMGUI_H
#define DEV_UI_IMGUI_H

#include "Geometry.h"
#include "HashTable.h"
#include "InputKeys.h"
#include "Vector.h"

#if SEOUL_ENABLE_DEV_UI

#include <imgui.h>

struct ImGuiSettingsHandler;

namespace Seoul
{

struct OsWindowRegion;
typedef Vector<OsWindowRegion, MemoryBudgets::DevUI> OsWindowRegions;

static inline ImVec2 ToImVec2(const Vector2D& v)
{
	return ImVec2(v.X, v.Y);
}

static inline ImVec4 ToImVec4(const Vector3D& v, Float w)
{
	return ImVec4(v.X, v.Y, v.Z, w);
}

static inline Bool Equals(const ImVec2& a, const ImVec2& b, Float fTolerance = fEpsilon)
{
	return
		Seoul::Equals(a.x, b.x, fEpsilon) &&
		Seoul::Equals(a.y, b.y, fEpsilon);
}

static inline Bool Equals(const ImVec2& a, const ImVec2& b, const ImVec2& vTolerance)
{
	return
		Seoul::Equals(a.x, b.x, vTolerance.x) &&
		Seoul::Equals(a.y, b.y, vTolerance.y);
}

static inline Bool operator==(const ImVec2& a, const ImVec2& b)
{
	return (a.x == b.x && a.y == b.y);
}

static inline Bool operator!=(const ImVec2& a, const ImVec2& b)
{
	return !(a == b);
}

static inline ImVec4 Lerp(const ImVec4& a, const ImVec4& b, Float f)
{
	return ImVec4(
		Lerp(a.x, b.x, f),
		Lerp(a.y, b.y, f),
		Lerp(a.z, b.z, f),
		Lerp(a.w, b.w, f));
}

}

#ifndef IMGUI_DEFINE_MATH_OPERATORS
static inline ImVec2 operator*(const ImVec2& lhs, const float rhs) { return ImVec2(lhs.x*rhs, lhs.y*rhs); }
static inline ImVec2 operator/(const ImVec2& lhs, const float rhs) { return ImVec2(lhs.x / rhs, lhs.y / rhs); }
static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }
static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }
static inline ImVec2 operator*(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x*rhs.x, lhs.y*rhs.y); }
static inline ImVec2 operator/(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x / rhs.x, lhs.y / rhs.y); }
static inline ImVec2& operator+=(ImVec2& lhs, const ImVec2& rhs) { lhs.x += rhs.x; lhs.y += rhs.y; return lhs; }
static inline ImVec2& operator-=(ImVec2& lhs, const ImVec2& rhs) { lhs.x -= rhs.x; lhs.y -= rhs.y; return lhs; }
static inline ImVec2& operator*=(ImVec2& lhs, const float rhs) { lhs.x *= rhs; lhs.y *= rhs; return lhs; }
static inline ImVec2& operator/=(ImVec2& lhs, const float rhs) { lhs.x /= rhs; lhs.y /= rhs; return lhs; }
static inline ImVec4 operator+(const ImVec4& lhs, const ImVec4& rhs) { return ImVec4(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w); }
static inline ImVec4 operator-(const ImVec4& lhs, const ImVec4& rhs) { return ImVec4(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w); }
static inline ImVec4 operator*(const ImVec4& lhs, const ImVec4& rhs) { return ImVec4(lhs.x*rhs.x, lhs.y*rhs.y, lhs.z*rhs.z, lhs.w*rhs.w); }
#endif

namespace ImGui
{

///////////////////////////////////////////////////////////////////////////////
//
// NOTE: Some definitions are defined in imgui_user.inl, which is located
// in External/imgui (despite being entirely Demiurge code), due to the weird
// structuring of that library.
//
///////////////////////////////////////////////////////////////////////////////

float GetWindowsResizeFromEdgesHalfThickness();

bool DragUInt(const char* label, unsigned int* v, float v_speed = 1.0f, unsigned int v_min = 0, unsigned int v_max = 0, const char* display_format = "%.0f");
bool InputTextEx(const char* label, const char* hint, char* buf, int buf_size, const ImVec2& size_arg, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* callback_user_data);
bool IsSpecificItemHovered(ImGuiID id);

void PublicClearActiveID();
bool WillWantCaptureMousePos(const ImVec2& pos, const char** pname = nullptr, bool* pb_client_area = nullptr);

void MarkIniSettingsDirty();
void RegisterSettingsHandler(
	const char* name,
	void* (*readOpen)(ImGuiContext*, ImGuiSettingsHandler*, const char* name),
	void(*readLine)(ImGuiContext* ctx, ImGuiSettingsHandler*, void*, const char* line),
	void(*writeAll)(ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf));

void ClampWindowPosTo(const ImVec2& pos, const ImVec2& size);
void OffsetAllWindowPos(const ImVec2& offset);
void GatherAllWindowRects(float fRescale, Seoul::OsWindowRegions& rv, char const* sMainFormName);

void UndockWindowByName(const char* name);

static inline float GetMainMenuBarHeight()
{
	return GetFontSize() + GetStyle().FramePadding.y * 2.0f;
}

// Disable mouse hover behavior for the current frame.
void DisableMouseHover();

// Current window will submit draw commands.
bool IsWindowActiveAndVisible();

// Custom version of that includes an optional center point for virtualized
// mode.
bool BeginPopupModalEx(const char* name, const ImVec2& vCenterIfVirtualized, bool* p_open = NULL, ImGuiWindowFlags flags = 0);

ImVec2 GetWindowCenter();

void NewLineEx(float advance);

void UndockAllFromDockSpace(ImGuiID dockSpaceID);

bool GetActiveWindowID(ImGuiID& windowID);
bool GetWindowScrollValues(ImGuiID windowID, float& x, float& maxx, float& y, float& maxy);
bool SetWindowScrollX(ImGuiID windowID, float x);
bool SetWindowScrollY(ImGuiID windowID, float y);

static inline bool GetWindowPosSizeByName(const char* name, Seoul::Vector2D& pos, Seoul::Vector2D& size, bool excludeTitleBar)
{
	ImVec2 impos, imsize;
	if (GetWindowPosSizeByName(name, impos, imsize, excludeTitleBar))
	{
		pos = Seoul::Vector2D(impos.x, impos.y);
		size = Seoul::Vector2D(imsize.x, imsize.y);
		return true;
	}

	return false;
}

void LoadStateStorage(const Seoul::HashTable<Seoul::UInt64, Seoul::Int, Seoul::MemoryBudgets::DevUI>& stateStorage);
void SaveStateStorage(Seoul::HashTable<Seoul::UInt64, Seoul::Int, Seoul::MemoryBudgets::DevUI>& rStateStorage);

ImVec2 GetWindowInnerRectMin();
ImVec2 GetWindowInnerRectMax();

bool BeginStatusBar();
void EndStatusBar();
void DockSpaceEx(bool bStatusBar, ImGuiID id, ImGuiDockNodeFlags flags = 0, const ImGuiWindowClass* window_class = NULL);

void MainMenuAsTitleBarControls(bool& rbOpened, bool& rbMinimized, bool& rbMaximized);

} // namespace ImGui

namespace Seoul
{

static inline Vector2D Convert(const ImVec2& v)
{
	return Vector2D(v.x, v.y);
}

static inline ImVec2 Convert(const Vector2D& v)
{
	return ImVec2(v.X, v.Y);
}

static inline void BeginValue(HString name)
{
	ImGui::Bullet();
	ImGui::Text("%s", name.CStr());
	ImGui::NextColumn();
	ImGui::PushID(name.CStr());
}

static inline void BeginValue(Byte const* s)
{
	ImGui::Bullet();
	ImGui::Text("%s", s);
	ImGui::NextColumn();
	ImGui::PushID(s);
}

static inline void EndValue()
{
	ImGui::PopID();
	ImGui::NextColumn();
}

static inline constexpr ImGuiKey ToImGuiKey(InputButton eButton)
{
#define SEOUL_CONV2(from, to) case InputButton::Key##from: return ImGuiKey_##to
#define SEOUL_CONV(name) SEOUL_CONV2(name, name)

	switch (eButton)
	{
	SEOUL_CONV(Tab);
	SEOUL_CONV2(Left, LeftArrow);
	SEOUL_CONV2(Right,RightArrow);
	SEOUL_CONV2(Up, UpArrow);
	SEOUL_CONV2(Down, DownArrow);
	SEOUL_CONV(PageUp);
	SEOUL_CONV(PageDown);
	SEOUL_CONV(Home);
	SEOUL_CONV(End);
	SEOUL_CONV(Insert);
	SEOUL_CONV(Delete);
	SEOUL_CONV(Backspace);
	SEOUL_CONV(Space);
	SEOUL_CONV(Enter);
	SEOUL_CONV(Escape);
	SEOUL_CONV2(LeftControl, LeftCtrl); 
	SEOUL_CONV(LeftShift); 
	SEOUL_CONV(LeftAlt); 
	SEOUL_CONV2(LeftWindows, LeftSuper);
	SEOUL_CONV2(RightControl, RightCtrl); 
	SEOUL_CONV(RightShift); 
	SEOUL_CONV(RightAlt); 
	SEOUL_CONV2(RightWindows, RightSuper);
	SEOUL_CONV2(AppMenu, Menu);
	SEOUL_CONV(0);
	SEOUL_CONV(1);
	SEOUL_CONV(2);
	SEOUL_CONV(3);
	SEOUL_CONV(4);
	SEOUL_CONV(5);
	SEOUL_CONV(6);
	SEOUL_CONV(7);
	SEOUL_CONV(8);
	SEOUL_CONV(9);
	SEOUL_CONV(A); 
	SEOUL_CONV(B); 
	SEOUL_CONV(C); 
	SEOUL_CONV(D);
	SEOUL_CONV(E);
	SEOUL_CONV(F);
	SEOUL_CONV(G);
	SEOUL_CONV(H);
	SEOUL_CONV(I);
	SEOUL_CONV(J);
	SEOUL_CONV(K); 
	SEOUL_CONV(L);
	SEOUL_CONV(M); 
	SEOUL_CONV(N); 
	SEOUL_CONV(O);
	SEOUL_CONV(P);
	SEOUL_CONV(Q);
	SEOUL_CONV(R);
	SEOUL_CONV(S);
	SEOUL_CONV(T);
	SEOUL_CONV(U); 
	SEOUL_CONV(V); 
	SEOUL_CONV(W); 
	SEOUL_CONV(X);
	SEOUL_CONV(Y);
	SEOUL_CONV(Z);
	SEOUL_CONV(F1); 
	SEOUL_CONV(F2);
	SEOUL_CONV(F3); 
	SEOUL_CONV(F4);
	SEOUL_CONV(F5); 
	SEOUL_CONV(F6);
	SEOUL_CONV(F7);
	SEOUL_CONV(F8);
	SEOUL_CONV(F9); 
	SEOUL_CONV(F10); 
	SEOUL_CONV(F11); 
	SEOUL_CONV(F12);
	SEOUL_CONV(Apostrophe);
	SEOUL_CONV(Comma);
	SEOUL_CONV(Minus);
	SEOUL_CONV(Period);
	SEOUL_CONV(Slash);
	SEOUL_CONV(Semicolon); 
	SEOUL_CONV2(Equals, Equal);
	SEOUL_CONV(LeftBracket);
	SEOUL_CONV(Backslash);
	SEOUL_CONV(RightBracket);
	SEOUL_CONV2(Grave, GraveAccent); 
	SEOUL_CONV(CapsLock);
	SEOUL_CONV(ScrollLock);
	SEOUL_CONV(NumLock);
	SEOUL_CONV(PrintScreen);
	SEOUL_CONV(Pause);
	SEOUL_CONV2(Numpad0, Keypad0); 
	SEOUL_CONV2(Numpad1, Keypad1);
	SEOUL_CONV2(Numpad2, Keypad2); 
	SEOUL_CONV2(Numpad3, Keypad3);
	SEOUL_CONV2(Numpad4, Keypad4);
	SEOUL_CONV2(Numpad5, Keypad5);
	SEOUL_CONV2(Numpad6, Keypad6);
	SEOUL_CONV2(Numpad7, Keypad7);
	SEOUL_CONV2(Numpad8, Keypad8);
	SEOUL_CONV2(Numpad9, Keypad9);
	SEOUL_CONV2(NumpadPeriod, KeypadDecimal);
	SEOUL_CONV2(NumpadDivide, KeypadDivide);
	SEOUL_CONV2(NumpadTimes, KeypadMultiply);
	SEOUL_CONV2(NumpadMinus, KeypadSubtract);
	SEOUL_CONV2(NumpadPlus, KeypadAdd);
	SEOUL_CONV2(NumpadEnter, KeypadEnter);
	// TODO: SEOUL_CONV(KeypadEqual);
	default:
		return ImGuiKey_None;
	};

#undef SEOUL_CONV
#undef SEOUL_CONV2
}

} // namespace Seoul

#endif // /SEOUL_ENABLE_DEV_UI

#endif // include guard
