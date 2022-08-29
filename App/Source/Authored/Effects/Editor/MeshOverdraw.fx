/**
 * \file MeshOverdraw.fx
 * \brief Effect used for drawing 3D geometry in the game's world.
 * Developer mode, renders for overdraw visualization.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#ifndef OVERDRAW_FX
#define OVERDRAW_FX

#define SEOUL_EDITOR_AND_TOOLS 1
#include "../World/MeshCommon.fxh"

///////////////////////////////////////////////////////////////////////////////
// Fragment Shaders
///////////////////////////////////////////////////////////////////////////////
half4 EditorFragmentOverdraw(vsOut input, FM_ARGS) : SEOUL_FRAG0_OUT
{
	float value = 0;
	value += (FM_T == eDiffuse  ? 1.5 : (FM_C == eDiffuse ? 0.5 : 0));
	value += (FM_T == eEmissive ? 1.5 : (FM_C == eEmissive ? 0.5 : 0));
	return half4(kvOverdrawFactors * value);
}

///////////////////////////////////////////////////////////////////////////////
// Techniques
///////////////////////////////////////////////////////////////////////////////
#define SEOUL_MESH_STATES \
	SEOUL_MESH_COMMON_STATES_OVERDRAW \
	SEOUL_RASTERIZER_STAGE_3D_UNLIT
#define SEOUL_MESH_FRAGMENT EditorFragmentOverdraw
#define SEOUL_MESH_VERTEX Vertex
#define SEOUL_MESH_VERTEX_SKINNED VertexSkinned
#include "../World/MeshVariations.fxh"

SEOUL_TECHNIQUE(seoul_Pick)
{
	pass
	{
		SEOUL_MESH_COMMON_STATES
		SEOUL_RASTERIZER_STAGE_3D_UNLIT

		SEOUL_PS(FragmentPick());
		SEOUL_VS(Vertex());
	}
}

SEOUL_TECHNIQUE(seoul_PickSkinned)
{
	pass
	{
		SEOUL_MESH_COMMON_STATES
		SEOUL_RASTERIZER_STAGE_3D_UNLIT

		SEOUL_PS(FragmentPick());
		SEOUL_VS(VertexSkinned());
	}
}

#endif // include guard
