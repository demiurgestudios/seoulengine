/**
 * \file MeshNormals.fx
 * \brief Effect used for drawing 3D geometry in the game's world.
 * This variation is specific to Seoul Editor and is used to
 * render a Mesh's normals for debug rendering.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#ifndef MESH_NORMALS_FX
#define MESH_NORMALS_FX

#define SEOUL_EDITOR_AND_TOOLS 1
#include "../World/MeshCommon.fxh"

///////////////////////////////////////////////////////////////////////////////
// Fragment Shaders
///////////////////////////////////////////////////////////////////////////////
half4 EditorFragmentNormals(vsOutLit input, FM_ARGS) : SEOUL_FRAG0_OUT
{
	float3 n = MultiplyAdd(normalize(input.WorldNormal.xyz), float3(0.5, 0.5, 0.5), float3(0.5, 0.5, 0.5));
	return half4(n.xyz, 1.0);
}

///////////////////////////////////////////////////////////////////////////////
// Techniques
///////////////////////////////////////////////////////////////////////////////
#define SEOUL_MESH_STATES \
	SEOUL_MESH_COMMON_STATES \
	SEOUL_RASTERIZER_STAGE_3D_UNLIT
#define SEOUL_MESH_FRAGMENT EditorFragmentNormals
#define SEOUL_MESH_VERTEX VertexLit
#define SEOUL_MESH_VERTEX_SKINNED VertexLitSkinned
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
