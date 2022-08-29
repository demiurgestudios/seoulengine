/**
 * \file EditorUIViewportEffectType.h
 * \brief Viewport rendering modes.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_UI_VIEWPORT_EFFECT_TYPE_H
#define EDITOR_UI_VIEWPORT_EFFECT_TYPE_H

#include "Prereqs.h"

namespace Seoul::EditorUI
{

enum class ViewportEffectType
{
	kUnlit,
	kWireframe,
	kMips,
	kNormals,
	kOverdraw,
	COUNT,
};

} // namespace Seoul::EditorUI

#endif // include guard
