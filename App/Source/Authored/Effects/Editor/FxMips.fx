/**
 * \file FxMips.fx
 * \brief Effect used for drawing 3D particle FX in the game's world.
 * This variation is specific to Seoul Editor and is used to
 * render mip utilization of an fx.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#ifndef FX_MIPS_FX
#define FX_MIPS_FX

#define SEOUL_EDITOR_AND_TOOLS 1
#include "../World/FxCommon.fxh"

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

float4 TextureDimensions : seoul_TextureDimensions;

///////////////////////////////////////////////////////////////////////////////
// Input/Output
///////////////////////////////////////////////////////////////////////////////
struct vsOutMips
{
	float4 Position : SEOUL_POSITION_OUT;
	float4 ColorMultiply : COLOR0;
	float4 ColorAddRGBAndBlendingFactor : COLOR1;
	float2 TexCoords : TEXCOORD0;
	float2 AlphaShapeTerms : TEXCOORD1;
	float2 MipVizTexCoords : TEXCOORD2;
};

///////////////////////////////////////////////////////////////////////////////
// Vertex Shaders
///////////////////////////////////////////////////////////////////////////////
vsOutMips VertexMips(vsIn input)
{
	vsOut base = Vertex(input);

	vsOutMips ret;
	ret.Position = base.Position;
	ret.ColorMultiply = base.ColorMultiply;
	ret.ColorAddRGBAndBlendingFactor = base.ColorAddRGBAndBlendingFactor;
	ret.TexCoords = base.TexCoords;
	ret.AlphaShapeTerms = base.AlphaShapeTerms;
	ret.MipVizTexCoords = (ret.TexCoords * TextureDimensions.xy) / float2(8.0, 8.0);
	return ret;
}

///////////////////////////////////////////////////////////////////////////////
// Fragment Shaders
///////////////////////////////////////////////////////////////////////////////
half4 EditorFragmenteMips(vsOutMips input) : SEOUL_FRAG0_OUT
{
	vsOut tmp;
	tmp.ColorMultiply = input.ColorMultiply;
	tmp.ColorAddRGBAndBlendingFactor = input.ColorAddRGBAndBlendingFactor;
	tmp.TexCoords = input.TexCoords;
	tmp.AlphaShapeTerms = input.AlphaShapeTerms;
	half4 diffuse = Fragment(tmp);

	half4 mipViz = Tex2DColor(MipVizTexture, MipVizSampler, input.MipVizTexCoords);

	return half4(lerp(diffuse.rgb, mipViz.rgb, mipViz.a), diffuse.a);
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

		SEOUL_PS(EditorFragmenteMips());
		SEOUL_VS(VertexMips());
	}
}

#endif // include guard
