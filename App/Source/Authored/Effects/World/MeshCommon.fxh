/**
 * \file MeshCommon.fxh
 * \brief Effect used for drawing 3D geometry in the game's world.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#ifndef MESH_COMMON_FXH
#define MESH_COMMON_FXH

#include "../Common/Common.fxh"

//////////////////////////////////////////////////////////////////////////////
// Static Constants
//////////////////////////////////////////////////////////////////////////////
/**
 * \brief Maximum number of 3x4 matrices in a skinning palette.
 */
static const int kSkinningMatricesCount = 68;

/**
 * \brief Maximum number of float4 entries in a skinning palette.
 */
static const int kSkinningPaletteSize = 3 * kSkinningMatricesCount;

//////////////////////////////////////////////////////////////////////////////
// Uniform Constants
//////////////////////////////////////////////////////////////////////////////
/**
 * \brief Diffuse color contains material solid color data.
 */
float4 DiffuseColor : seoul_DiffuseColor;

/**
 * \brief Diffuse texture contains mat color data.
 */
SEOUL_TEXTURE(DiffuseTexture, DiffuseSampler, seoul_DiffuseTexture)
{
	Texture = (DiffuseTexture);
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

/**
 * \brief Emissive color contains material solid color data.
 */
float4 EmissiveColor : seoul_EmissiveColor;

/**
 * \brief Emissive texture contains mat color data.
 */
SEOUL_TEXTURE(EmissiveTexture, EmissiveSampler, seoul_EmissiveTexture)
{
	Texture = (EmissiveTexture);
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

/**
 * \brief Alpha mask, contains 1-bit masking data.
 */
float4 AlphaMaskColor : seoul_AlphaMaskColor;

/**
 * \brief Alpha mask texture, contains 1-bit masking data.
 */
SEOUL_TEXTURE(AlphaMaskTexture, AlphaMaskSampler, seoul_AlphaMaskTexture)
{
	Texture = (AlphaMaskTexture);
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

#if SEOUL_EDITOR_AND_TOOLS
/**
 * \brief Specific to the editor, color used for selection
 * highlight.
 */
float4 HighlightColor : seoul_HighlightColor = float4(0, 0, 0, 0);
#endif // /#if SEOUL_EDITOR_AND_TOOLS

/** When picking, constant color to render. */
float4 PickColor : seoul_PickColor;

/** Skinning palette for meshes that use animation skinning. */
float4 Skinning[kSkinningPaletteSize] : seoul_SkinningPalette;

/** Global view space to projection space transform. */
float4x4 ViewProjection : seoul_ViewProjection;

/** Global object space to world space transform. */
float4x4 WorldTransform : seoul_WorldTransform;

/** Global object space to world space transform, for normal vectors. */
float4x4 WorldNormalTransform : seoul_WorldNormalTransform;

///////////////////////////////////////////////////////////////////////////////
// Input/Output
///////////////////////////////////////////////////////////////////////////////
struct vsIn
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
	float2 TexCoords : TEXCOORD0;
};

struct vsInSkinned
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
	float2 TexCoords : TEXCOORD0;
	float4 BoneWeights : BLENDWEIGHT;
#if SEOUL_D3D11
	uint4 BoneIndices : BLENDINDICES;
#else
	float4 BoneIndices : BLENDINDICES;
#endif
};

struct vsOut
{
	float4 Position : SEOUL_POSITION_OUT;
	float2 TexCoords : TEXCOORD0;
};

struct vsOutLit
{
	float4 Position : SEOUL_POSITION_OUT;
	float2 TexCoords : TEXCOORD0;
	float3 WorldNormal : TEXCOORD1;
};

///////////////////////////////////////////////////////////////////////////////
// Functions
///////////////////////////////////////////////////////////////////////////////
#if SEOUL_EDITOR_AND_TOOLS
half3 ApplyHighlight(half3 diffuse)
{
	return half3(lerp(diffuse, HighlightColor.rgb, HighlightColor.a));
}
#endif // /#if SEOUL_EDITOR_AND_TOOLS

/**
 * \brief Extracts a 3x4 skinning transform from the skinning palette
 * at matrix index \a index.
 */
float3x4 GetSkinning3x4(float index)
{
	float i = (index * 3);

	float3x4 ret;
	ret._11_12_13_14 = Skinning[i + 0];
	ret._21_22_23_24 = Skinning[i + 1];
	ret._31_32_33_34 = Skinning[i + 2];
	return ret;
}

/**
 * \brief Calculates the full 3x4 skinning transform given weights \a weights
 * and indices \a indices.
 */
float3x4 GetSkinning3x4(float4 weights, float4 indices)
{
	float3x4 ret;
	ret  = weights.x * GetSkinning3x4(indices.x);
	ret += weights.y * GetSkinning3x4(indices.y);
	ret += weights.z * GetSkinning3x4(indices.z);
	ret += weights.w * GetSkinning3x4(indices.w);

	return ret;
}

///////////////////////////////////////////////////////////////////////////////
// Vertex Shaders
///////////////////////////////////////////////////////////////////////////////
vsOut Vertex(vsIn input)
{
	float4x4 wvp = mul(WorldTransform, ViewProjection);

	vsOut ret;
	ret.Position = SEOUL_OUT_POS(mul(float4(input.Position.xyz, 1.0), wvp));
	ret.TexCoords = input.TexCoords;
	return ret;
}

vsOut VertexSkinned(vsInSkinned input)
{
	float4x4 wvp = mul(WorldTransform, ViewProjection);

	float4 localPosition = float4(input.Position.xyz, 1.0);
	float3x4 skinMatrix = GetSkinning3x4(input.BoneWeights, input.BoneIndices);
	float4 skinnedPosition = float4(mul(skinMatrix, localPosition), 1.0);

	vsOut ret;
	ret.Position = SEOUL_OUT_POS(mul(skinnedPosition, wvp));
	ret.TexCoords = input.TexCoords;
	return ret;
}

vsOutLit VertexLit(vsIn input)
{
	float4x4 wvp = mul(WorldTransform, ViewProjection);

	vsOutLit ret;
	ret.Position = SEOUL_OUT_POS(mul(float4(input.Position.xyz, 1.0), wvp));
	ret.TexCoords = input.TexCoords;
	ret.WorldNormal = mul(float4(input.Normal.xyz, 0.0), WorldNormalTransform).xyz;
	return ret;
}

vsOutLit VertexLitSkinned(vsInSkinned input)
{
	float4x4 wvp = mul(WorldTransform, ViewProjection);

	float4 localNormal = float4(input.Normal.xyz, 0.0);
	float4 localPosition = float4(input.Position.xyz, 1.0);
	float3x4 skinMatrix = GetSkinning3x4(input.BoneWeights, input.BoneIndices);
	float4 skinnedNormal = float4(mul(skinMatrix, localNormal), 0.0);
	float4 skinnedPosition = float4(mul(skinMatrix, localPosition), 1.0);

	vsOutLit ret;
	ret.Position = SEOUL_OUT_POS(mul(skinnedPosition, wvp));
	ret.TexCoords = input.TexCoords;
	ret.WorldNormal = mul(skinnedNormal, WorldNormalTransform).xyz;
	return ret;
}

///////////////////////////////////////////////////////////////////////////////
// Fragment Shaders
///////////////////////////////////////////////////////////////////////////////
typedef int FragmentMode;
#define FM_0 0
#define FM_C 1
#define FM_T 2
#define FM_ARGS uniform FragmentMode eDiffuse, uniform FragmentMode eEmissive, uniform FragmentMode eAlphaMask
#define FM_PASS eDiffuse, eEmissive, eAlphaMask

half3 FragmentUnlitCommon(vsOut input, FM_ARGS)
{
	half3 ret = half3(0, 0, 0);
	if (FM_0 != eDiffuse)
	{
		if (FM_T == eDiffuse)
		{
			ret += Tex2DColor(DiffuseTexture, DiffuseSampler, input.TexCoords).rgb;
		}
		else
		{
			ret += half3(DiffuseColor.rgb);
		}
	}
	if (FM_0 != eEmissive)
	{
		if (FM_T == eEmissive)
		{
			ret += Tex2DColor(EmissiveTexture, EmissiveSampler, input.TexCoords).rgb;
		}
		else
		{
			ret += half3(EmissiveColor.rgb);
		}
	}
	if (FM_0 != eAlphaMask)
	{
		half mask;
		if (FM_T == eAlphaMask)
		{
			mask = Tex2DColor(AlphaMaskTexture, AlphaMaskSampler, input.TexCoords).g; // TODO:
		}
		else
		{
			mask = half(AlphaMaskColor.g); // TODO:
		}
		clip(mask - 0.5);
	}
	return ret;
}

half4 FragmentUnlit(vsOut input, FM_ARGS) : SEOUL_FRAG0_OUT
{
	half3 ret = FragmentUnlitCommon(input, FM_PASS);
	return half4(ret, 1.0);
}

#if SEOUL_EDITOR_AND_TOOLS
half4 EditorFragmentUnlit(vsOut input, FM_ARGS) : SEOUL_FRAG0_OUT
{
	half3 ret = FragmentUnlitCommon(input, FM_PASS);
	ret = ApplyHighlight(ret);
	return half4(ret, 1.0);
}
#endif // /#if SEOUL_EDITOR_AND_TOOLS

half4 FragmentPick(vsOut input) : SEOUL_FRAG0_OUT
{
	return half4(PickColor.rgba);
}

///////////////////////////////////////////////////////////////////////////////
// Techniques
///////////////////////////////////////////////////////////////////////////////
#define SEOUL_MESH_COMMON_STATES_OVERDRAW \
	SEOUL_ALPHA_BLEND_PRE_MULTIPLIED \
	SEOUL_ENABLE_DEPTH_DISABLE_STENCIL

#define SEOUL_MESH_COMMON_STATES \
	SEOUL_OPAQUE \
	SEOUL_ENABLE_DEPTH_DISABLE_STENCIL

#endif // include guard
