/**
 * \file MeshMips.fx
 * \brief Effect used for drawing 3D geometry in the game's world.
 * This variation is specific to Seoul Editor and is used to
 * render mip utilization of a mesh.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#ifndef MESH_MIPS_FX
#define MESH_MIPS_FX

#define SEOUL_EDITOR_AND_TOOLS 1
#include "../World/MeshCommon.fxh"

//////////////////////////////////////////////////////////////////////////////
// Uniform Constants
//////////////////////////////////////////////////////////////////////////////
SEOUL_TEXTURE(MipVizTexture, MipVizSampler, seoul_MipVizTexture)
{
	Texture = (MipVizTexture);
	AddressU = Wrap;
	AddressV = Wrap;
#if SEOUL_D3D11
	Filter = Min_Mag_Mip_Linear;
#else
	MinFilter = Linear;
	MipFilter = Linear;
	MagFilter = Linear;
#endif
};

float4 DiffuseTextureDimensions : seoul_DiffuseTextureDimensions;

///////////////////////////////////////////////////////////////////////////////
// Input/Output
///////////////////////////////////////////////////////////////////////////////
struct vsOutMips
{
	float4 Position : SEOUL_POSITION_OUT;
	float2 TexCoords : TEXCOORD0;
	float2 MipVizTexCoords : TEXCOORD1;
};

///////////////////////////////////////////////////////////////////////////////
// Vertex Shaders
///////////////////////////////////////////////////////////////////////////////
vsOutMips VertexMips(vsIn input)
{
	vsOut base = Vertex(input);

	vsOutMips ret;
	ret.Position = base.Position;
	ret.TexCoords = base.TexCoords;
	ret.MipVizTexCoords = (ret.TexCoords * DiffuseTextureDimensions.xy) / float2(8.0, 8.0);
	return ret;
}

vsOutMips VertexMipsSkinned(vsInSkinned input)
{
	vsOut base = VertexSkinned(input);

	vsOutMips ret;
	ret.Position = base.Position;
	ret.TexCoords = base.TexCoords;
	ret.MipVizTexCoords = (ret.TexCoords * DiffuseTextureDimensions.xy) / float2(8.0, 8.0);
	return ret;
}

///////////////////////////////////////////////////////////////////////////////
// Fragment Shaders
///////////////////////////////////////////////////////////////////////////////
half4 EditorFragmentMips(vsOutMips input, FM_ARGS) : SEOUL_FRAG0_OUT
{
	vsOut tmp;
	tmp.Position = input.Position;
	tmp.TexCoords = input.TexCoords;
	half4 diffuse = FragmentUnlit(tmp, FM_PASS);
	if (FM_T == eDiffuse)
	{
		half4 mipViz = Tex2DColor(MipVizTexture, MipVizSampler, input.MipVizTexCoords);
		return half4(lerp(diffuse.rgb, mipViz.rgb, mipViz.a), diffuse.a);
	}
	else
	{
		return diffuse;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Techniques
///////////////////////////////////////////////////////////////////////////////
#define SEOUL_MESH_STATES \
	SEOUL_MESH_COMMON_STATES \
	SEOUL_RASTERIZER_STAGE_3D_UNLIT
#define SEOUL_MESH_FRAGMENT EditorFragmentMips
#define SEOUL_MESH_VERTEX VertexMips
#define SEOUL_MESH_VERTEX_SKINNED VertexMipsSkinned
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
