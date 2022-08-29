/**
 * \file Falcon.fx
 * \brief Main Effect used for rendering geometry produced
 * by the Falcon Flash player library.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#ifndef FALCON_FX
#define FALCON_FX

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

SEOUL_TEXTURE(DetailTexture, DetailSampler, seoul_Detail)
{
	Texture = (DetailTexture);
	AddressU = Wrap;
	AddressV = Wrap;
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
float4 g_Perspective : seoul_Perspective;
float4 g_ViewProjectionUI : seoul_ViewProjectionUI;

struct vsIn2D
{
	float2 Position : POSITION;
	float4 ColorMultiply : COLOR0;
	float4 ColorAddRGBAndBlendingFactor : COLOR1;
	float4 TexCoords : TEXCOORD0;
};

struct vsIn3D
{
	float2 Position : POSITION;
	float4 ColorMultiply : COLOR0;
	float4 ColorAddRGBAndBlendingFactor : COLOR1;
	float4 TexCoords : TEXCOORD0;
	float Depth3D : TEXCOORD1;
};

struct vsOutAllFeatures
{
	float4 Position : SEOUL_POSITION_OUT;
	float4 ColorMultiply : COLOR0;
	float4 ColorAddRGBAndBlendingFactor : COLOR1;
	float4 TexCoords : TEXCOORD0;
	float2 AlphaShapeTerms : TEXCOORD1;
};

struct vsOutAlphaShape
{
	float4 Position : SEOUL_POSITION_OUT;
	float4 ColorMultiply : COLOR0;
	float4 TexCoords : TEXCOORD0;
	float2 AlphaShapeTerms : TEXCOORD1;
};

struct vsOutColorMultiply
{
	float4 Position : SEOUL_POSITION_OUT;
	float4 ColorMultiply : COLOR0;
	float4 TexCoords : TEXCOORD0;
};

struct vsOutColorMultiplyAdd
{
	float4 Position : SEOUL_POSITION_OUT;
	float4 ColorMultiply : COLOR0;
	float4 ColorAddRGBAndBlendingFactor : COLOR1;
	float4 TexCoords : TEXCOORD0;
};

struct vsOutNoColor
{
	float4 Position : SEOUL_POSITION_OUT;
};

///////////////////////////////////////////////////////////////////////////////
// Vertex Shaders
///////////////////////////////////////////////////////////////////////////////
vsOutAllFeatures VertexAllFeatures2D(vsIn2D input)
{
	float4 colorAdd = GetVertexColor(input.ColorAddRGBAndBlendingFactor);
	const bool alphaShape = (colorAdd.a > 0.0 && colorAdd.a < 1.0);
	colorAdd.a = (alphaShape ? 1.0 : (1.0 - colorAdd.a));

	vsOutAllFeatures ret;
	ret.AlphaShapeTerms.x = (alphaShape ? (1.0 / (colorAdd.g - colorAdd.r)) : 1.0);
	ret.AlphaShapeTerms.y = (alphaShape ? (-colorAdd.r * ret.AlphaShapeTerms.x) : 0.0);
	ret.Position = SEOUL_OUT_POS(float4(MultiplyAdd(input.Position.xy, g_ViewProjectionUI.xy, g_ViewProjectionUI.zw), 0, 1));
	ret.ColorMultiply = GetVertexColor(input.ColorMultiply);
	ret.ColorAddRGBAndBlendingFactor = (alphaShape ? float4(0, 0, 0, 1) : float4(colorAdd.rgb * ret.ColorMultiply.a, colorAdd.a));
	ret.ColorMultiply = float4(ret.ColorMultiply.rgb * ret.ColorMultiply.a, ret.ColorMultiply.a * ret.ColorAddRGBAndBlendingFactor.a);
	ret.TexCoords = input.TexCoords;
	return ret;
}

vsOutAlphaShape VertexAlphaShape2D(vsIn2D input)
{
	float4 colorAdd = GetVertexColor(input.ColorAddRGBAndBlendingFactor);
	const bool alphaShape = (colorAdd.a > 0.0 && colorAdd.a < 1.0);
	colorAdd.a = (alphaShape ? 1.0 : (1.0 - colorAdd.a));

	vsOutAlphaShape ret;
	ret.AlphaShapeTerms.x = (alphaShape ? (1.0 / (colorAdd.g - colorAdd.r)) : 1.0);
	ret.AlphaShapeTerms.y = (alphaShape ? (-colorAdd.r * ret.AlphaShapeTerms.x) : 0.0);
	ret.Position = SEOUL_OUT_POS(float4(MultiplyAdd(input.Position.xy, g_ViewProjectionUI.xy, g_ViewProjectionUI.zw), 0, 1));
	ret.ColorMultiply = GetVertexColor(input.ColorMultiply);
	ret.ColorMultiply = float4(ret.ColorMultiply.rgb * ret.ColorMultiply.a, ret.ColorMultiply.a * colorAdd.a);
	ret.TexCoords = input.TexCoords;
	return ret;
}

vsOutColorMultiply VertexColorMultiply2D(vsIn2D input)
{
	float4 colorAdd = GetVertexColor(input.ColorAddRGBAndBlendingFactor);
	colorAdd.a = (1.0 - colorAdd.a);

	vsOutColorMultiply ret;
	ret.Position = SEOUL_OUT_POS(float4(MultiplyAdd(input.Position.xy, g_ViewProjectionUI.xy, g_ViewProjectionUI.zw), 0, 1));
	ret.ColorMultiply = GetVertexColor(input.ColorMultiply);
	ret.ColorMultiply = float4(ret.ColorMultiply.rgb * ret.ColorMultiply.a, ret.ColorMultiply.a * colorAdd.a);
	ret.TexCoords = input.TexCoords;

	return ret;
}

vsOutColorMultiplyAdd VertexColorMultiplyAdd2D(vsIn2D input)
{
	float4 colorAdd = GetVertexColor(input.ColorAddRGBAndBlendingFactor);
	colorAdd.a = (1.0 - colorAdd.a);

	vsOutColorMultiplyAdd ret;
	ret.Position = SEOUL_OUT_POS(float4(MultiplyAdd(input.Position.xy, g_ViewProjectionUI.xy, g_ViewProjectionUI.zw), 0, 1));
	ret.ColorMultiply = GetVertexColor(input.ColorMultiply);
	ret.ColorAddRGBAndBlendingFactor = float4(colorAdd.rgb * ret.ColorMultiply.a, colorAdd.a);
	ret.ColorMultiply = float4(ret.ColorMultiply.rgb * ret.ColorMultiply.a, ret.ColorMultiply.a * ret.ColorAddRGBAndBlendingFactor.a);
	ret.TexCoords = input.TexCoords;

	return ret;
}

vsOutNoColor VertexNoColor2D(vsIn2D input)
{
	vsOutNoColor ret;
	ret.Position = SEOUL_OUT_POS(float4(MultiplyAdd(input.Position.xy, g_ViewProjectionUI.xy, g_ViewProjectionUI.zw), 0, 1));
	return ret;
}

vsOutAllFeatures VertexAllFeatures3D(vsIn3D input)
{
	float4 colorAdd = GetVertexColor(input.ColorAddRGBAndBlendingFactor);
	const bool alphaShape = (colorAdd.a > 0.0 && colorAdd.a < 1.0);
	colorAdd.a = (alphaShape ? 1.0 : (1.0 - colorAdd.a));

	vsOutAllFeatures ret;
	ret.AlphaShapeTerms.x = (alphaShape ? (1.0 / (colorAdd.g - colorAdd.r)) : 1.0);
	ret.AlphaShapeTerms.y = (alphaShape ? (-colorAdd.r * ret.AlphaShapeTerms.x) : 0.0);
	ret.Position = SEOUL_OUT_POS(float4(MultiplyAdd(input.Position.xy, g_ViewProjectionUI.xy, g_ViewProjectionUI.zw), 0, (1 - (input.Depth3D * g_Perspective.x))));
	ret.ColorMultiply = GetVertexColor(input.ColorMultiply);
	ret.ColorAddRGBAndBlendingFactor = (alphaShape ? float4(0, 0, 0, 1) : float4(colorAdd.rgb * ret.ColorMultiply.a, colorAdd.a));
	ret.ColorMultiply = float4(ret.ColorMultiply.rgb * ret.ColorMultiply.a, ret.ColorMultiply.a * ret.ColorAddRGBAndBlendingFactor.a);
	ret.TexCoords = input.TexCoords;
	return ret;
}

vsOutAlphaShape VertexAlphaShape3D(vsIn3D input)
{
	float4 colorAdd = GetVertexColor(input.ColorAddRGBAndBlendingFactor);
	const bool alphaShape = (colorAdd.a > 0.0 && colorAdd.a < 1.0);
	colorAdd.a = (alphaShape ? 1.0 : (1.0 - colorAdd.a));

	vsOutAlphaShape ret;
	ret.AlphaShapeTerms.x = (alphaShape ? (1.0 / (colorAdd.g - colorAdd.r)) : 1.0);
	ret.AlphaShapeTerms.y = (alphaShape ? (-colorAdd.r * ret.AlphaShapeTerms.x) : 0.0);
	ret.Position = SEOUL_OUT_POS(float4(MultiplyAdd(input.Position.xy, g_ViewProjectionUI.xy, g_ViewProjectionUI.zw), 0, (1 - (input.Depth3D * g_Perspective.x))));
	ret.ColorMultiply = GetVertexColor(input.ColorMultiply);
	ret.ColorMultiply = float4(ret.ColorMultiply.rgb * ret.ColorMultiply.a, ret.ColorMultiply.a * colorAdd.a);
	ret.TexCoords = input.TexCoords;
	return ret;
}

vsOutColorMultiply VertexColorMultiply3D(vsIn3D input)
{
	float4 colorAdd = GetVertexColor(input.ColorAddRGBAndBlendingFactor);
	colorAdd.a = (1.0 - colorAdd.a);

	vsOutColorMultiply ret;
	ret.Position = SEOUL_OUT_POS(float4(MultiplyAdd(input.Position.xy, g_ViewProjectionUI.xy, g_ViewProjectionUI.zw), 0, (1 - (input.Depth3D * g_Perspective.x))));
	ret.ColorMultiply = GetVertexColor(input.ColorMultiply);
	ret.ColorMultiply = float4(ret.ColorMultiply.rgb * ret.ColorMultiply.a, ret.ColorMultiply.a * colorAdd.a);
	ret.TexCoords = input.TexCoords;

	return ret;
}

vsOutColorMultiplyAdd VertexColorMultiplyAdd3D(vsIn3D input)
{
	float4 colorAdd = GetVertexColor(input.ColorAddRGBAndBlendingFactor);
	colorAdd.a = (1.0 - colorAdd.a);

	vsOutColorMultiplyAdd ret;
	ret.Position = SEOUL_OUT_POS(float4(MultiplyAdd(input.Position.xy, g_ViewProjectionUI.xy, g_ViewProjectionUI.zw), 0, (1 - (input.Depth3D * g_Perspective.x))));
	ret.ColorMultiply = GetVertexColor(input.ColorMultiply);
	ret.ColorAddRGBAndBlendingFactor = float4(colorAdd.rgb * ret.ColorMultiply.a, colorAdd.a);
	ret.ColorMultiply = float4(ret.ColorMultiply.rgb * ret.ColorMultiply.a, ret.ColorMultiply.a * ret.ColorAddRGBAndBlendingFactor.a);
	ret.TexCoords = input.TexCoords;

	return ret;
}

vsOutNoColor VertexNoColor3D(vsIn3D input)
{
	vsOutNoColor ret;
	ret.Position = SEOUL_OUT_POS(float4(MultiplyAdd(input.Position.xy, g_ViewProjectionUI.xy, g_ViewProjectionUI.zw), 0, (1 - (input.Depth3D * g_Perspective.x))));
	return ret;
}

///////////////////////////////////////////////////////////////////////////////
// Fragment Shaders
///////////////////////////////////////////////////////////////////////////////
half4 FragmentAllFeatures(vsOutAllFeatures input) : SEOUL_FRAG0_OUT
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

	half4 texColor = Tex2DColor(TextureTexture, TextureSampler, input.TexCoords.xy);
	texColor = half4(saturate(MultiplyAdd(texColor, input.AlphaShapeTerms.x, input.AlphaShapeTerms.y)));
	half4 ret = texColor * half4(input.ColorMultiply);
	ret.rgb += half3(input.ColorAddRGBAndBlendingFactor.rgb * texColor.a);
	return ret;
}

half4 FragmentColorAlphaShape(vsOutAllFeatures input) : SEOUL_FRAG0_OUT
{
	// NOTE: Unlike all other places, this shader assumes the input
	// texture has not been premultilplied. This avoids the following
	// logic:
	//   if (texColor.a != 0)
	//   {
	//     texColor.rgb /= texColor.a;
	//
	// And instead just allows saturation and multiplication against rgb.

	float4 texColor = Tex2DColor(TextureTexture, TextureSampler, input.TexCoords.xy);
	texColor.a = saturate(texColor.a * input.AlphaShapeTerms.x + input.AlphaShapeTerms.y);
	texColor.rgb *= texColor.a;
	half4 ret = half4(texColor) * half4(input.ColorMultiply);
	ret.rgb += half3(input.ColorAddRGBAndBlendingFactor.rgb * texColor.a);
	return ret;
}

half4 FragmentAllFeaturesOverdraw(vsOutNoColor input) : SEOUL_FRAG0_OUT
{
	return half4(kvOverdrawFactors * 3.0);
}

half4 FragmentAllFeaturesSecondaryTexture(vsOutAllFeatures input) : SEOUL_FRAG0_OUT
{
	half4 texColor = Tex2DColor(TextureTexture, TextureSampler, TextureTexture_Secondary, TextureSampler_Secondary, input.TexCoords.xy);
	texColor = half4(saturate(MultiplyAdd(texColor, input.AlphaShapeTerms.x, input.AlphaShapeTerms.y)));
	half4 ret = texColor * half4(input.ColorMultiply);
	ret.rgb += half3(input.ColorAddRGBAndBlendingFactor.rgb * texColor.a);
	return ret;
}

half4 FragmentColorAlphaShapeSecondaryTexture(vsOutAllFeatures input) : SEOUL_FRAG0_OUT
{
	float4 texColor = Tex2DColor(TextureTexture, TextureSampler, TextureTexture_Secondary, TextureSampler_Secondary, input.TexCoords.xy);
	texColor.a = saturate(texColor.a * input.AlphaShapeTerms.x + input.AlphaShapeTerms.y);
	texColor.rgb *= texColor.a;
	half4 ret = half4(texColor) * half4(input.ColorMultiply);
	ret.rgb += half3(input.ColorAddRGBAndBlendingFactor.rgb * texColor.a);
	return ret;
}

half4 FragmentAllFeaturesDetail(vsOutAllFeatures input) : SEOUL_FRAG0_OUT
{
	half4 texColor = Tex2DColor(TextureTexture, TextureSampler, input.TexCoords.xy);
	texColor = half4(saturate(MultiplyAdd(texColor, input.AlphaShapeTerms.x, input.AlphaShapeTerms.y)));
	texColor.rgb *= Tex2DColor(DetailTexture, DetailSampler, input.TexCoords.zw).rgb;
	half4 ret = texColor * half4(input.ColorMultiply);
	ret.rgb += half3(input.ColorAddRGBAndBlendingFactor.rgb * texColor.a);
	return ret;
}

half4 FragmentAllFeaturesDetailOverdraw(vsOutNoColor input) : SEOUL_FRAG0_OUT
{
	return half4(kvOverdrawFactors * 4.0);
}

half4 FragmentAllFeaturesDetailSecondaryTexture(vsOutAllFeatures input) : SEOUL_FRAG0_OUT
{
	half4 texColor = Tex2DColor(TextureTexture, TextureSampler, TextureTexture_Secondary, TextureSampler_Secondary, input.TexCoords.xy);
	texColor = half4(saturate(MultiplyAdd(texColor, input.AlphaShapeTerms.x, input.AlphaShapeTerms.y)));
	texColor.rgb *= Tex2DColor(DetailTexture, DetailSampler, input.TexCoords.zw).rgb;
	half4 ret = texColor * half4(input.ColorMultiply);
	ret.rgb += half3(input.ColorAddRGBAndBlendingFactor.rgb * texColor.a);
	return ret;
}

half4 FragmentAlphaShape(vsOutAlphaShape input) : SEOUL_FRAG0_OUT
{
	half4 texColor = Tex2DColor(TextureTexture, TextureSampler, input.TexCoords.xy);
	texColor = half4(saturate(MultiplyAdd(texColor, input.AlphaShapeTerms.x, input.AlphaShapeTerms.y)));
	half4 ret = texColor * half4(input.ColorMultiply);
	return ret;
}

half4 FragmentAlphaShapeOverdraw(vsOutAlphaShape input) : SEOUL_FRAG0_OUT
{
	return half4(kvOverdrawFactors * 2.0);
}

half4 FragmentAlphaShapeSecondaryTexture(vsOutAlphaShape input) : SEOUL_FRAG0_OUT
{
	half4 texColor = Tex2DColor(TextureTexture, TextureSampler, TextureTexture_Secondary, TextureSampler_Secondary, input.TexCoords.xy);
	texColor = half4(saturate(MultiplyAdd(texColor, input.AlphaShapeTerms.x, input.AlphaShapeTerms.y)));
	half4 ret = texColor * half4(input.ColorMultiply);
	return ret;
}

half4 FragmentColorMultiply(vsOutColorMultiply input) : SEOUL_FRAG0_OUT
{
	half4 texColor = Tex2DColor(TextureTexture, TextureSampler, input.TexCoords.xy);
	half4 ret = texColor * half4(input.ColorMultiply);
	return ret;
}

half4 FragmentColorMultiplyOverdraw(vsOutNoColor input) : SEOUL_FRAG0_OUT
{
	return half4(kvOverdrawFactors);
}

half4 FragmentColorMultiplySecondaryTexture(vsOutColorMultiply input) : SEOUL_FRAG0_OUT
{
	half4 texColor = Tex2DColor(TextureTexture, TextureSampler, TextureTexture_Secondary, TextureSampler_Secondary, input.TexCoords.xy);
	half4 ret = texColor * half4(input.ColorMultiply);
	return ret;
}

half4 FragmentColorMultiplyAdd(vsOutColorMultiplyAdd input) : SEOUL_FRAG0_OUT
{
	half4 texColor = Tex2DColor(TextureTexture, TextureSampler, input.TexCoords.xy);
	half4 ret = texColor * half4(input.ColorMultiply);
	ret.rgb += half3(input.ColorAddRGBAndBlendingFactor.rgb * texColor.a);
	return ret;
}

half4 FragmentColorMultiplyAddOverdraw(vsOutNoColor input) : SEOUL_FRAG0_OUT
{
	return half4(kvOverdrawFactors * 2.0);
}

half4 FragmentColorMultiplyAddSecondaryTexture(vsOutColorMultiplyAdd input) : SEOUL_FRAG0_OUT
{
	half4 texColor = Tex2DColor(TextureTexture, TextureSampler, TextureTexture_Secondary, TextureSampler_Secondary, input.TexCoords.xy);
	half4 ret = texColor * half4(input.ColorMultiply);
	ret.rgb += half3(input.ColorAddRGBAndBlendingFactor.rgb * texColor.a);
	return ret;
}

half4 FragmentNoColor(vsOutNoColor input) : SEOUL_FRAG0_OUT
{
	return half4(0, 0, 0, 0);
}

half4 FragmentShadowTwoPass(vsOutColorMultiply input) : SEOUL_FRAG0_OUT
{
	half texAlpha = Tex2DColor(TextureTexture, TextureSampler, input.TexCoords.xy).a;
	half ret = texAlpha * half(input.ColorMultiply.a);

	// Two pass shadow inverts alpha on write - we accumulate shadow values
	// in the back buffer alpha channel, then apply it as a multiply against
	// the back buffer color to achieve a shadow effect.
	return half4(0, 0, 0, 1 - ret);
}

half4 FragmentShadowTwoPassSecondaryTexture(vsOutColorMultiply input) : SEOUL_FRAG0_OUT
{
	half texAlpha = Tex2DColor(TextureTexture, TextureSampler, TextureTexture_Secondary, TextureSampler_Secondary, input.TexCoords.xy).a;
	half ret = half(texAlpha * input.ColorMultiply.a);

	// Two pass shadow inverts alpha on write - we accumulate shadow values
	// in the back buffer alpha channel, then apply it as a multiply against
	// the back buffer color to achieve a shadow effect.
	return half4(0, 0, 0, 1 - ret);
}

///////////////////////////////////////////////////////////////////////////////
// Techniques
///////////////////////////////////////////////////////////////////////////////
SEOUL_TECHNIQUE(seoul_RenderColorAlphaShapeSecondaryTexture2D)
{
	pass
	{
		SEOUL_PS(FragmentColorAlphaShapeSecondaryTexture);
		SEOUL_VS(VertexAllFeatures2D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderColorAlphaShape2D)
{
	pass
	{
		SEOUL_PS(FragmentColorAlphaShape);
		SEOUL_VS(VertexAllFeatures2D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderColorAlphaShapeSecondaryTexture3D)
{
	pass
	{
		SEOUL_PS(FragmentColorAlphaShapeSecondaryTexture);
		SEOUL_VS(VertexAllFeatures3D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderColorAlphaShape3D)
{
	pass
	{
		SEOUL_PS(FragmentColorAlphaShape);
		SEOUL_VS(VertexAllFeatures3D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderAllFeaturesSecondaryTexture2D)
{
	pass
	{
		SEOUL_PS(FragmentAllFeaturesSecondaryTexture);
		SEOUL_VS(VertexAllFeatures2D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderAllFeatures2D)
{
	pass
	{
		SEOUL_PS(FragmentAllFeatures);
		SEOUL_VS(VertexAllFeatures2D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderAllFeaturesOverdraw2D)
{
	pass
	{
		SEOUL_PS(FragmentAllFeaturesOverdraw);
		SEOUL_VS(VertexAllFeatures2D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderAllFeaturesDetailSecondaryTexture2D)
{
	pass
	{
		SEOUL_PS(FragmentAllFeaturesDetailSecondaryTexture);
		SEOUL_VS(VertexAllFeatures2D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderAllFeaturesDetail2D)
{
	pass
	{
		SEOUL_PS(FragmentAllFeaturesDetail);
		SEOUL_VS(VertexAllFeatures2D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderAllFeaturesDetailOverdraw2D)
{
	pass
	{
		SEOUL_PS(FragmentAllFeaturesDetailOverdraw);
		SEOUL_VS(VertexAllFeatures2D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderAlphaShapeSecondaryTexture2D)
{
	pass
	{
		SEOUL_PS(FragmentAlphaShapeSecondaryTexture);
		SEOUL_VS(VertexAlphaShape2D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderAlphaShape2D)
{
	pass
	{
		SEOUL_PS(FragmentAlphaShape);
		SEOUL_VS(VertexAlphaShape2D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderAlphaShapeOverdraw2D)
{
	pass
	{
		SEOUL_PS(FragmentAlphaShapeOverdraw);
		SEOUL_VS(VertexAlphaShape2D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderColorMultiply2D)
{
	pass
	{
		SEOUL_PS(FragmentColorMultiply);
		SEOUL_VS(VertexColorMultiply2D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderColorMultiplyOverdraw2D)
{
	pass
	{
		SEOUL_PS(FragmentColorMultiplyOverdraw);
		SEOUL_VS(VertexNoColor2D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderColorMultiplySecondaryTexture2D)
{
	pass
	{
		SEOUL_PS(FragmentColorMultiplySecondaryTexture);
		SEOUL_VS(VertexColorMultiply2D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderColorMultiplyAdd2D)
{
	pass
	{
		SEOUL_PS(FragmentColorMultiplyAdd);
		SEOUL_VS(VertexColorMultiplyAdd2D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderColorMultiplyAddOverdraw2D)
{
	pass
	{
		SEOUL_PS(FragmentColorMultiplyAddOverdraw);
		SEOUL_VS(VertexNoColor2D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderColorMultiplyAddSecondaryTexture2D)
{
	pass
	{
		SEOUL_PS(FragmentColorMultiplyAddSecondaryTexture);
		SEOUL_VS(VertexColorMultiplyAdd2D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderNoColor2D)
{
	pass
	{
		SEOUL_PS(FragmentNoColor);
		SEOUL_VS(VertexNoColor2D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderShadowTwoPass2D)
{
	pass
	{
		SEOUL_PS(FragmentShadowTwoPass);
		SEOUL_VS(VertexColorMultiply2D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderShadowTwoPassSecondaryTexture2D)
{
	pass
	{
		SEOUL_PS(FragmentShadowTwoPassSecondaryTexture);
		SEOUL_VS(VertexColorMultiply2D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderAllFeaturesSecondaryTexture3D)
{
	pass
	{
		SEOUL_PS(FragmentAllFeaturesSecondaryTexture);
		SEOUL_VS(VertexAllFeatures3D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderAllFeatures3D)
{
	pass
	{
		SEOUL_PS(FragmentAllFeatures);
		SEOUL_VS(VertexAllFeatures3D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderAllFeaturesOverdraw3D)
{
	pass
	{
		SEOUL_PS(FragmentAllFeaturesOverdraw);
		SEOUL_VS(VertexAllFeatures3D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderAllFeaturesDetailSecondaryTexture3D)
{
	pass
	{
		SEOUL_PS(FragmentAllFeaturesDetailSecondaryTexture);
		SEOUL_VS(VertexAllFeatures3D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderAllFeaturesDetail3D)
{
	pass
	{
		SEOUL_PS(FragmentAllFeaturesDetail);
		SEOUL_VS(VertexAllFeatures3D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderAllFeaturesDetailOverdraw3D)
{
	pass
	{
		SEOUL_PS(FragmentAllFeaturesDetailOverdraw);
		SEOUL_VS(VertexAllFeatures3D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderAlphaShapeSecondaryTexture3D)
{
	pass
	{
		SEOUL_PS(FragmentAlphaShapeSecondaryTexture);
		SEOUL_VS(VertexAlphaShape3D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderAlphaShape3D)
{
	pass
	{
		SEOUL_PS(FragmentAlphaShape);
		SEOUL_VS(VertexAlphaShape3D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderAlphaShapeOverdraw3D)
{
	pass
	{
		SEOUL_PS(FragmentAlphaShapeOverdraw);
		SEOUL_VS(VertexAlphaShape3D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderColorMultiply3D)
{
	pass
	{
		SEOUL_PS(FragmentColorMultiply);
		SEOUL_VS(VertexColorMultiply3D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderColorMultiplyOverdraw3D)
{
	pass
	{
		SEOUL_PS(FragmentColorMultiplyOverdraw);
		SEOUL_VS(VertexNoColor3D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderColorMultiplySecondaryTexture3D)
{
	pass
	{
		SEOUL_PS(FragmentColorMultiplySecondaryTexture);
		SEOUL_VS(VertexColorMultiply3D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderColorMultiplyAdd3D)
{
	pass
	{
		SEOUL_PS(FragmentColorMultiplyAdd);
		SEOUL_VS(VertexColorMultiplyAdd3D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderColorMultiplyAddOverdraw3D)
{
	pass
	{
		SEOUL_PS(FragmentColorMultiplyAddOverdraw);
		SEOUL_VS(VertexNoColor3D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderColorMultiplyAddSecondaryTexture3D)
{
	pass
	{
		SEOUL_PS(FragmentColorMultiplyAddSecondaryTexture);
		SEOUL_VS(VertexColorMultiplyAdd3D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderNoColor3D)
{
	pass
	{
		SEOUL_PS(FragmentNoColor);
		SEOUL_VS(VertexNoColor3D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderShadowTwoPass3D)
{
	pass
	{
		SEOUL_PS(FragmentShadowTwoPass);
		SEOUL_VS(VertexColorMultiply3D);
	}
}

SEOUL_TECHNIQUE(seoul_RenderShadowTwoPassSecondaryTexture3D)
{
	pass
	{
		SEOUL_PS(FragmentShadowTwoPassSecondaryTexture);
		SEOUL_VS(VertexColorMultiply3D);
	}
}

#endif // include guard
