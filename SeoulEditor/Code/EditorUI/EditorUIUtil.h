/**
 * \file EditorUIUtil.h
 * \brief Miscellaneous utility functions for the EditorUI project.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_UI_UTIL_H
#define EDITOR_UI_UTIL_H

#include "DevUIImGui.h"
#include "Prereqs.h"
#include "Vector2D.h"
namespace Seoul { class Camera; }
namespace Seoul { struct Vector3D; }
namespace Seoul { struct Viewport; }
namespace Seoul { namespace Reflection { class AttributeCollection; } }

namespace Seoul::EditorUI
{

Float32 ComputeGizmoScale(
	Float fDesiredSizeInPixels,
	const Camera& camera,
	const Viewport& viewport,
	const Vector3D& vGizmoWorldPosition);

template <typename T>
static inline Bool ImGuiEnumNameUtil(void* pData, Int iIndex, Byte const** psOut)
{
	*psOut = EnumToString<T>(iIndex);
	return true;
}

Bool InputText(Byte const* sLabel, String& rs, ImGuiInputTextFlags eFlags = 0, ImGuiInputTextCallback pCallback = nullptr, void* pUserData = nullptr);

void SetTooltipEx(Byte const* sLabel);
void SetTooltipEx(const Reflection::AttributeCollection& attributes);

} // namespace Seoul::EditorUI

#endif // include guard
