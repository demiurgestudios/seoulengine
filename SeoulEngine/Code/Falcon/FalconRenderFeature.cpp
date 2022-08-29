/**
 * \file FalconRenderFeature.cpp
 * \brief A Render::Feature is a gradually more complex
 * (and thus more costly at runtime) definition of the
 * material that is necessary to render a Falcon batch.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FalconRenderFeature.h"
#include "ReflectionDefine.h"

namespace Seoul
{

SEOUL_BEGIN_ENUM(Falcon::Render::Feature::Enum)
	SEOUL_ENUM_N("None", Falcon::Render::Feature::kNone)
	SEOUL_ENUM_N("ColorMultiply", Falcon::Render::Feature::kColorMultiply)
	SEOUL_ENUM_N("ColorAdd", Falcon::Render::Feature::kColorAdd)
	SEOUL_ENUM_N("AlphaShape", Falcon::Render::Feature::kAlphaShape)
	SEOUL_ENUM_N("Detail", Falcon::Render::Feature::kDetail)
	SEOUL_ENUM_N("Extended_InvSrcAlpha_One", Falcon::Render::Feature::kExtended_InvSrcAlpha_One)
	SEOUL_ENUM_N("Extended_InvSrcColor_One", Falcon::Render::Feature::kExtended_InvSrcColor_One)
	SEOUL_ENUM_N("Extended_One_InvSrcColor", Falcon::Render::Feature::kExtended_One_InvSrcColor)
	SEOUL_ENUM_N("Extended_One_SrcAlpha", Falcon::Render::Feature::kExtended_One_SrcAlpha)
	SEOUL_ENUM_N("Extended_One_SrcColor", Falcon::Render::Feature::kExtended_One_SrcColor)
	SEOUL_ENUM_N("Extended_SrcAlpha_InvSrcAlpha", Falcon::Render::Feature::kExtended_SrcAlpha_InvSrcAlpha)
	SEOUL_ENUM_N("Extended_SrcAlpha_InvSrcColor", Falcon::Render::Feature::kExtended_SrcAlpha_InvSrcColor)
	SEOUL_ENUM_N("Extended_SrcAlpha_One", Falcon::Render::Feature::kExtended_SrcAlpha_One)
	SEOUL_ENUM_N("Extended_SrcAlpha_SrcAlpha", Falcon::Render::Feature::kExtended_SrcAlpha_SrcAlpha)
	SEOUL_ENUM_N("Extended_SrcColor_InvSrcAlpha", Falcon::Render::Feature::kExtended_SrcColor_InvSrcAlpha)
	SEOUL_ENUM_N("Extended_SrcColor_InvSrcColor", Falcon::Render::Feature::kExtended_SrcColor_InvSrcColor)
	SEOUL_ENUM_N("Extended_SrcColor_One", Falcon::Render::Feature::kExtended_SrcColor_One)
	SEOUL_ENUM_N("Extended_Zero_InvSrcColor", Falcon::Render::Feature::kExtended_Zero_InvSrcColor)
	SEOUL_ENUM_N("Extended_Zero_SrcColor", Falcon::Render::Feature::kExtended_Zero_SrcColor)
	SEOUL_ENUM_N("Extended_ColorAlphaShape", Falcon::Render::Feature::kExtended_ColorAlphaShape)
SEOUL_END_ENUM()

} // namespace Seoul
