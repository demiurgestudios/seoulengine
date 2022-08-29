/**
 * \file MeshUnlit.fx
 * \brief Effect used for drawing 3D geometry in the game's world.
 * This variation is specific to Seoul Editor, and includes
 * some effects (selection highlight) that are not part of the runtime
 * shader.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#ifndef MESH_UNLIT_FX
#define MESH_UNLIT_FX

#define SEOUL_EDITOR_AND_TOOLS 1
#include "../World/MeshCommon.fxh"

///////////////////////////////////////////////////////////////////////////////
// Techniques
///////////////////////////////////////////////////////////////////////////////
#define SEOUL_MESH_STATES \
	SEOUL_MESH_COMMON_STATES \
	SEOUL_RASTERIZER_STAGE_3D_UNLIT
#define SEOUL_MESH_FRAGMENT EditorFragmentUnlit
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
