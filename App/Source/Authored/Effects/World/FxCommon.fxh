/**
 * \file FxCommon.fxh
 * \brief Effect used for drawing 3D particle FX in the game's world.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#ifndef FX_COMMON_FXH
#define FX_COMMON_FXH

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

/** Global view space to projection space transform. */
float4x4 ViewProjection : seoul_ViewProjection;

///////////////////////////////////////////////////////////////////////////////
// Input/Output
///////////////////////////////////////////////////////////////////////////////
struct vsIn
{
	float3 Position : POSITION;
	float4 ColorMultiply : COLOR0;
	float4 ColorAddRGBAndBlendingFactor : COLOR1;
	float2 TexCoords : TEXCOORD0;
};

struct vsOut
{
	float4 Position : SEOUL_POSITION_OUT;
	float4 ColorMultiply : COLOR0;
	float4 ColorAddRGBAndBlendingFactor : COLOR1;
	float2 TexCoords : TEXCOORD0;
	float2 AlphaShapeTerms : TEXCOORD1;
};

///////////////////////////////////////////////////////////////////////////////
// Vertex Shaders
///////////////////////////////////////////////////////////////////////////////
vsOut Vertex(vsIn input)
{
	float4 colorAdd = GetVertexColor(input.ColorAddRGBAndBlendingFactor);
	const bool alphaShape = (colorAdd.a > 0.0 && colorAdd.a < 1.0);
	colorAdd.a = (alphaShape ? 1.0 : (1.0 - colorAdd.a));

	vsOut ret;
	ret.AlphaShapeTerms.x = (alphaShape ? (1.0 / (colorAdd.g - colorAdd.r)) : 1.0);
	ret.AlphaShapeTerms.y = (alphaShape ? (-colorAdd.r * ret.AlphaShapeTerms.x) : 0.0);
	ret.Position = SEOUL_OUT_POS(float4(mul(float4(input.Position.xyz, 1.0), ViewProjection)));
	ret.ColorMultiply = GetVertexColor(input.ColorMultiply);
	ret.ColorAddRGBAndBlendingFactor = (alphaShape ? float4(0, 0, 0, 1) : float4(colorAdd.rgb * ret.ColorMultiply.a, colorAdd.a));
	ret.ColorMultiply = float4(ret.ColorMultiply.rgb * ret.ColorMultiply.a, ret.ColorMultiply.a * ret.ColorAddRGBAndBlendingFactor.a);
	ret.TexCoords = input.TexCoords;
	return ret;
}

///////////////////////////////////////////////////////////////////////////////
// Fragment Shaders
///////////////////////////////////////////////////////////////////////////////
half4 Fragment(vsOut input) : SEOUL_FRAG0_OUT
{
	// The math we're trying to achieve here is:
	//   - (rgb * saturate(a * k0 + k1) / a)
	// because rgb is premultiplied against a, and we're trying to recompute a.
	//
	// However, we don't want to pay the cost (or precision loss) of the correct form.
	// Fortunately, our use case of alpha shape (for SDF rendering) means that the
	// form we're using below is correct in the two possible use cases:
	//   - if the input is an SDF, then RGB will be AAA, (rgb / a) cancels out, and
	//     the result is equivalent to what we have here.
	//   - if the input is regular color image data, then k0 will be 1 and k1 will be 0,
	//     and then the entire operation effectively becomes a nop.

	half4 texColor = Tex2DColor(TextureTexture, TextureSampler, input.TexCoords);
	texColor = half4(saturate(MultiplyAdd(texColor, input.AlphaShapeTerms.x, input.AlphaShapeTerms.y)));
	half4 ret = texColor * half4(input.ColorMultiply);
	ret.rgb += half3(input.ColorAddRGBAndBlendingFactor.rgb * texColor.a);
	return ret;
}

#endif // include guard
