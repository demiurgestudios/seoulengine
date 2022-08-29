/**
 * \file MeshUnlit.fx
 * \brief Effect used for drawing 3D geometry in the game's world.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#ifndef MESH_UNLIT_FX
#define MESH_UNLIT_FX

#include "MeshCommon.fxh"

///////////////////////////////////////////////////////////////////////////////
// Techniques
///////////////////////////////////////////////////////////////////////////////
#define SEOUL_MESH_STATES \
	SEOUL_MESH_COMMON_STATES \
	SEOUL_RASTERIZER_STAGE_3D_UNLIT
#define SEOUL_MESH_FRAGMENT FragmentUnlit
#define SEOUL_MESH_VERTEX Vertex
#define SEOUL_MESH_VERTEX_SKINNED VertexSkinned
#include "MeshVariations.fxh"

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
