/**
 * \file DevUIImGuiFont.h
 * \brief Override the default font builtin to ImGui with Roboto.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef DEV_UI_IMGUI_FONT_H
#define DEV_UI_IMGUI_FONT_H

#include "Prereqs.h"

#if SEOUL_ENABLE_DEV_UI

namespace Seoul::DevUI::ImGuiFont
{

void const* GetDataTTF();
unsigned int GetSize();
UInt16 const* GetGlyphRanges();

} // namespace Seoul::DevUI::ImGuiFont

#endif // /SEOUL_ENABLE_DEV_UI

#endif // include guard
