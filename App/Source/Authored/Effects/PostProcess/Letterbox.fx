/**
 * \file Letterbox.fx
 * \brief Effect for drawing either letterbox or pillarbox border textures
 * as neeeded for a fixed aspect ratio game.
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#ifndef LETTERBOX_FX
#define LETTERBOX_FX

#include "../Common/Common.fxh"

//////////////////////////////////////////////////////////////////////////////
// Uniform Constants
//////////////////////////////////////////////////////////////////////////////

/**
 * \brief Color texture which contains the sub images used for most rendering.
 */
SEOUL_TEXTURE(LetterboxTexture, LetterboxSampler, seoul_LetterboxTexture)
{
	Texture = (LetterboxTexture);
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

	// Adjust position.
	float2 vPosition = (input.Position.xy * 2.0) - float2(1, 1);

	// Under OpenGL ES 2.0, we need to flip the Y position, since coordinates
	// are bottom-up instead of our expected top-down.
#if SEOUL_OGLES2
	ret.Position = SEOUL_OUT_POS(float4(float2(vPosition.x, -vPosition.y), 0, 1));
#else
	ret.Position = SEOUL_OUT_POS(float4(vPosition.xy, 0, 1));
#endif

	ret.TexCoords = input.TexCoords;
	return ret;
}

///////////////////////////////////////////////////////////////////////////////
// Fragment Shaders
///////////////////////////////////////////////////////////////////////////////
half4 Fragment(vsOut input) : SEOUL_FRAG0_OUT
{
	return half4(SEOUL_TEX2D(LetterboxTexture, LetterboxSampler, input.TexCoords));
}

///////////////////////////////////////////////////////////////////////////////
// Techniques
///////////////////////////////////////////////////////////////////////////////
SEOUL_TECHNIQUE(seoul_Render)
{
	pass
	{
		SEOUL_OPAQUE
		SEOUL_DISABLE_DEPTH_AND_STENCIL
		SEOUL_RASTERIZER_STAGE_2D

		SEOUL_PS(Fragment());
		SEOUL_VS(Vertex());
	}
}

#endif // include guard
