/**
 * \file ScenePrereqs.h
 * \brief Miscellaneous shared globals for the Scene project.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCENE_PREREQS_H
#define SCENE_PREREQS_H

#include "SeoulHString.h"

#if SEOUL_WITH_SCENE

namespace Seoul::Scene
{

/** Common technique for main scene rendering. */
static const HString kEffectTechniqueRender("seoul_Render");

/** EffectParameter used for setting the Camera's ViewProjection transform. */
static const HString kEffectParameterViewProjection("seoul_ViewProjection");

} // namespace Seoul::Scene

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
