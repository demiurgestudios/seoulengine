/**
 * \file FxOverdraw.fx
 * \brief Effect used for drawing 3D particle FX in the game's world.
 * Developer mode, renders for overdraw visualization.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#ifndef FX_OVERDRAW_FX
#define FX_OVERDRAW_FX

#include "../World/FxCommon.fxh"

///////////////////////////////////////////////////////////////////////////////
// Fragment Shaders
///////////////////////////////////////////////////////////////////////////////
half4 EditorFragmentOverdraw(vsOut input) : SEOUL_FRAG0_OUT
{
	return half4(kvOverdrawFactors);
}

///////////////////////////////////////////////////////////////////////////////
// Techniques
///////////////////////////////////////////////////////////////////////////////
SEOUL_TECHNIQUE(seoul_Render)
{
	pass
	{
		SEOUL_ALPHA_BLEND_PRE_MULTIPLIED
		SEOUL_ENABLE_DEPTH_NO_WRITES_DISABLE_STENCIL
		SEOUL_RASTERIZER_STAGE_3D_FX

		SEOUL_PS(EditorFragmentOverdraw());
		SEOUL_VS(Vertex());
	}
}

#endif // include guard
