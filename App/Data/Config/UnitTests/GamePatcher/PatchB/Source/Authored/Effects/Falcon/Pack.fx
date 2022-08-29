/**
 * \file Pack.fx
 * \brief Effect technique used for packing individual textures
 * into the global texture atlas.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#ifndef PACK_FX
#define PACK_FX

#include "../Common/Common.fxh"

//////////////////////////////////////////////////////////////////////////////
// Uniform Constants
//////////////////////////////////////////////////////////////////////////////
SEOUL_TEXTURE(TextureTexture, TextureSampler, seoul_Texture)
{
	Texture = (TextureTexture);
	AddressU = Clamp;
	AddressV = Clamp;
#if SEOUL_D3D11
	Filter = Min_Mag_Linear_Mip_Point;
#else
	MinFilter = Linear;
	MagFilter = Linear;
#endif
};

SEOUL_TEXTURE(TextureTexture_Secondary, TextureSampler_Secondary, seoul_Texture_Secondary)
{
	Texture = (TextureTexture_Secondary);
	AddressU = Clamp;
	AddressV = Clamp;
#if SEOUL_D3D11
	Filter = Min_Mag_Linear_Mip_Point;
#else
	MinFilter = Linear;
	MagFilter = Linear;
#endif
};

///////////////////////////////////////////////////////////////////////////////
// Input/Output
///////////////////////////////////////////////////////////////////////////////
struct vsIn
{
	float2 Position : POSITION;
	float4 ColorMultiply : COLOR0;
	float4 ColorAddRGBAndBlendingFactor : COLOR1;
	float2 TexCoords : TEXCOORD0;
};

struct vsOut
{
	float4 Position : SEOUL_POSITION_OUT;
	float2 TexCoords : TEXCOORD0;
};

///////////////////////////////////////////////////////////////////////////////
// Vertex Shaders
///////////////////////////////////////////////////////////////////////////////
vsOut Vertex(vsIn input)
{
	vsOut ret;

	// Under OpenGL ES 2.0, we need to flip the Y position, since coordinates
	// are bottom-up instead of our expected top-down.
#if SEOUL_OGLES2
	ret.Position = SEOUL_OUT_POS(float4(float2(input.Position.x, 1.0 - input.Position.y) * float2(2, -2) + float2(-1, 1), 0, 1));
#else
	ret.Position = SEOUL_OUT_POS(float4(input.Position.xy * float2(2, -2) + float2(-1, 1), 0, 1));
#endif

	ret.TexCoords = input.TexCoords;
	return ret;
}

///////////////////////////////////////////////////////////////////////////////
// Fragment Shaders
///////////////////////////////////////////////////////////////////////////////
half4 Fragment(vsOut input) : SEOUL_FRAG0_OUT
{
	return Tex2DColor(TextureTexture, TextureSampler, TextureTexture_Secondary, TextureSampler_Secondary, input.TexCoords);
}

///////////////////////////////////////////////////////////////////////////////
// Techniques
///////////////////////////////////////////////////////////////////////////////
SEOUL_TECHNIQUE(seoul_Pack)
{
	pass
	{
		SEOUL_DISABLE_DEPTH_AND_STENCIL
		SEOUL_OPAQUE
		SEOUL_RASTERIZER_STAGE_2D

		SEOUL_PS(Fragment);
		SEOUL_VS(Vertex);
	}
}

#endif // include guard
