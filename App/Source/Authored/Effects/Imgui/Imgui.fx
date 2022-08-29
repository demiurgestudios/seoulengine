/**
 * \file Imgui.fx
 * \brief Effect used to render geometry produced by the ImGui
 * library, which SeoulEngine uses for editor and developer UI.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#ifndef IMGUI_FX
#define IMGUI_FX

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

///////////////////////////////////////////////////////////////////////////////
// Input/Output
///////////////////////////////////////////////////////////////////////////////
float4 g_ViewProjectionUI : seoul_ViewProjectionUI;

struct vsIn
{
	float2 Position : POSITION;
	float2 TexCoords : TEXCOORD0;
	float4 Color : COLOR0;
};

struct vsOut
{
	float4 Position : SEOUL_POSITION_OUT;
	float2 TexCoords : TEXCOORD0;
	float4 Color : COLOR0;
};

///////////////////////////////////////////////////////////////////////////////
// Vertex Shaders
///////////////////////////////////////////////////////////////////////////////
vsOut Vertex(vsIn input)
{
	vsOut ret;
	ret.Position = SEOUL_OUT_POS(float4(MultiplyAdd(input.Position.xy, g_ViewProjectionUI.xy, g_ViewProjectionUI.zw), 0, 1));
	ret.Color = GetVertexColor(input.Color);
	ret.TexCoords = input.TexCoords;
	return ret;
}

///////////////////////////////////////////////////////////////////////////////
// Fragment Shaders
///////////////////////////////////////////////////////////////////////////////
half4 Fragment(vsOut input) : SEOUL_FRAG0_OUT
{
	half4 texColor = Tex2DColor(TextureTexture, TextureSampler, input.TexCoords);
	half4 ret = texColor * half4(input.Color);
	return ret;
}

///////////////////////////////////////////////////////////////////////////////
// Techniques
///////////////////////////////////////////////////////////////////////////////
SEOUL_TECHNIQUE(seoul_Render)
{
	pass
	{
		SEOUL_ALPHA_BLEND
		SEOUL_DISABLE_DEPTH_AND_STENCIL
		SEOUL_RASTERIZER_STAGE_2D

		SEOUL_PS(Fragment());
		SEOUL_VS(Vertex());
	}
}

#endif // include guard
