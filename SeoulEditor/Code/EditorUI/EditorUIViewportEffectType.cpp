/**
 * \file EditorUIViewportEffectType.cpp
 * \brief Viewport rendering modes.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EditorUIViewportEffectType.h"
#include "ReflectionDefine.h"

namespace Seoul
{

SEOUL_BEGIN_ENUM(EditorUI::ViewportEffectType)
	SEOUL_ENUM_N("Unlit", EditorUI::ViewportEffectType::kUnlit)
	SEOUL_ENUM_N("Wireframe", EditorUI::ViewportEffectType::kWireframe)
	SEOUL_ENUM_N("Mips", EditorUI::ViewportEffectType::kMips)
	SEOUL_ENUM_N("Normals", EditorUI::ViewportEffectType::kNormals)
	SEOUL_ENUM_N("Overdraw", EditorUI::ViewportEffectType::kOverdraw)
SEOUL_END_ENUM()

} // namespace Seoul
