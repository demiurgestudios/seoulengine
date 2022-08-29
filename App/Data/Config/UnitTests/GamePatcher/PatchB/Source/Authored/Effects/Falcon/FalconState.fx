/**
 * \file FalconState.fx
 * \brief State FX used for rendering geometry produced
 * by the Falcon Flash player library. Sets render state
 * shared by all Falcon rendering techniques.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#ifndef FALCON_STATE_FX
#define FALCON_STATE_FX

#include "../Common/Common.fxh"

///////////////////////////////////////////////////////////////////////////////
// Techniques
///////////////////////////////////////////////////////////////////////////////
SEOUL_TECHNIQUE(seoul_State)
{
	pass
	{
		SEOUL_ALPHA_BLEND_PRE_MULTIPLIED
		SEOUL_DISABLE_DEPTH_AND_STENCIL
		SEOUL_RASTERIZER_STAGE_2D
	}
}

// Define extended blend mode techniques
#define SEOUL_EXT_BLEND_ITER(d3d9src, d3d9dst, src, dst) \
	SEOUL_TECHNIQUE(seoul_State_Extended_##d3d9src##_##d3d9dst) \
	{ \
		pass \
		{ \
			SEOUL_EXT_BLEND_MODE(d3d9src, d3d9dst, src, dst) \
			SEOUL_DISABLE_DEPTH_AND_STENCIL \
			SEOUL_RASTERIZER_STAGE_2D \
		} \
	}

// Eval.
SEOUL_FOR_EACH_EXT_BLEND_MODE

#undef SEOUL_EXT_BLEND_ITER

// Special case.
SEOUL_TECHNIQUE(seoul_State_Extended_ColorAlphaShape)
{
	pass
	{
		SEOUL_ALPHA_BLEND_PRE_MULTIPLIED
		SEOUL_DISABLE_DEPTH_AND_STENCIL
		SEOUL_RASTERIZER_STAGE_2D
	}
}

SEOUL_TECHNIQUE(seoul_InputVisualizationState)
{
	pass
	{
		SEOUL_ALPHA_BLEND_PRE_MULTIPLIED
		SEOUL_ENABLE_DEPTH_DISABLE_STENCIL
		SEOUL_RASTERIZER_STAGE_2D
	}
}

SEOUL_TECHNIQUE(seoul_ShadowAccumulateState)
{
	pass
	{
		SEOUL_SHADOW_ACCUMULATE_BLEND
		SEOUL_DISABLE_DEPTH_AND_STENCIL
		SEOUL_RASTERIZER_STAGE_2D
	}
}

SEOUL_TECHNIQUE(seoul_ShadowApplyState)
{
	pass
	{
		SEOUL_SHADOW_APPLY_BLEND
		SEOUL_DISABLE_DEPTH_AND_STENCIL
		SEOUL_RASTERIZER_STAGE_2D
	}
}

#endif // include guard
