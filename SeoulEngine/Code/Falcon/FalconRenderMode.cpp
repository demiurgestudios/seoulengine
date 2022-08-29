/**
 * \file FalconRenderMode.cpp
 * \brief Global control of Falcon rendering.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FalconRenderMode.h"
#include "ReflectionDefine.h"

namespace Seoul
{

#if SEOUL_ENABLE_CHEATS
SEOUL_BEGIN_ENUM(Falcon::Render::Mode::Enum)
	SEOUL_ENUM_N("Default", Falcon::Render::Mode::kDefault)
	SEOUL_ENUM_N("Occlusion", Falcon::Render::Mode::kOcclusion)
	SEOUL_ENUM_N("Overdraw", Falcon::Render::Mode::kOverdraw)
	SEOUL_ENUM_N("TextureResolution", Falcon::Render::Mode::kTextureResolution)
	SEOUL_ENUM_N("WorldBounds", Falcon::Render::Mode::kWorldBounds)
SEOUL_END_ENUM()
#endif // /#if SEOUL_ENABLE_CHEATS

} // namespace Seoul
