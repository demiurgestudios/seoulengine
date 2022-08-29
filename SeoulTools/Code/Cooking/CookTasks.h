/**
 * \file CookTasks.h
 * \brief Utility header, links all cook tasks provided by
 * the Cooking library.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef COOK_TASKS_H
#define COOK_TASKS_H

#include "ReflectionType.h"

namespace Seoul::Cooking
{

#if SEOUL_WITH_ANIMATION_2D
SEOUL_LINK_ME(class, Animation2DCookTask);
#endif // /#if SEOUL_WITH_ANIMATION_2D
#if SEOUL_WITH_FMOD
SEOUL_LINK_ME(class, AudioCookTask);
#endif
SEOUL_LINK_ME(class, EffectCookTask);
SEOUL_LINK_ME(class, FontCookTask);
#if SEOUL_WITH_FX_STUDIO
SEOUL_LINK_ME(class, FxBankCookTask);
#endif
SEOUL_LINK_ME(class, PackageCookTask);
SEOUL_LINK_ME(class, ProtobufCookTask);
SEOUL_LINK_ME(class, SceneAssetCookTask);
SEOUL_LINK_ME(class, ScenePrefabCookTask);
SEOUL_LINK_ME(class, ScriptCookTask);
SEOUL_LINK_ME(class, ScriptProjectCookTask);
SEOUL_LINK_ME(class, TextureCookTask);
SEOUL_LINK_ME(class, UICookTask);

} // namespace Seoul::Cooking

#endif // include guard
