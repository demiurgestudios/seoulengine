/**
 * \file ColorBlindVisualization.fx
 * \brief Main Effect used for developer-only visualization of three
 * extreme forms of color blindness (deutanopia,
 * protanopia, and tritanopia).
 *
 * \note All equations in this file are based on:
 *
 * Vienot F., Brettel H., Mollon J. "Digital Video
 * Colourmaps for Checking the Legibility of Displays by Dichromats". 1999
 *
 * http://vision.psychol.cam.ac.uk/jdmollon/papers/colourmaps.pdf
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#ifndef COLOR_BLIND_VISUALIZATION_FX
#define COLOR_BLIND_VISUALIZATION_FX

#include "../Common/Common.fxh"

//////////////////////////////////////////////////////////////////////////////
// Static Constants
//////////////////////////////////////////////////////////////////////////////
/** Factors for our simplification of Deutanopia - less sensitivity to green light. See RedGreenFilter. */
static const float3 kvDeutanopiaFactors = float3(0.292703, 0.707205, -0.022279);

/** Factors for our simplification of Protanopia - less sensitivity to red light. See RedGreenFilter. */
static const float3 kvProtanopiaFactors = float3(0.112400, 0.887600,  0.003998);

/** Factors for our simplification of Achromatopsia (complete color blidness - grayscale weights - NTSC grayscale or luma factors). */
static const float3 kvAchromatopsiaFactors = float3(0.212600, 0.715200, 0.072200);

//////////////////////////////////////////////////////////////////////////////
// Uniform Constants
//////////////////////////////////////////////////////////////////////////////
SEOUL_TEXTURE(TextureTexture, TextureSampler, seoul_Texture)
{
	Texture = (TextureTexture);
	AddressU = Clamp;
	AddressV = Clamp;
#if SEOUL_D3D11
	Filter = Min_Mag_Mip_Point;
#else
	MinFilter = Point;
	MagFilter = Point;
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

//////////////////////////////////////////////////////////////////////////////
// Functions
//////////////////////////////////////////////////////////////////////////////
/**
 * Deuteranopia and Protanopia are both simulated with a red-green filter.
 *
 * Our simplification of Deuteranopia describes an extreme with less
 * sensitivity to green light.
 *
 * Our simplification of Protanopia describes an extreme with less
 * sensitivity to red light.
 */
float4 RedGreenFilter(float3 k, float4 c)
{
	// sRGB -> linear RGB.
	float4 l = NonLinearToLinear(c);

	// Applie constants to combine RG into a single channel, then
	// re-weight blue.
	float lr = saturate(l.r * k.x + l.g * k.y);
	float lb = saturate(l.r * k.z - l.g * k.z + l.b);

	// Back to sRGB color space.
	return LinearToNonLinear(float4(lr, lr, lb, l.a));
}

/**
 * Effects of Tritanopia are more complex. Roughly, blue skews towards
 * green and yellow skews towards violet. It is describe with
 * a linear transformation on the LMS color space.
 */
float4 TritanopiaFilter(float4 c)
{
	const float kfE0 = 0.05059983 + 0.08585369 + 0.00952420;
	const float kfE1 = 0.01893033 + 0.08925308 + 0.01370054;
	const float kfE2 = 0.00292202 + 0.00975732 + 0.07145979;
	const float kfInflectionPoint = kfE1 / kfE0;
	const float kfA1 = -kfE2 * 0.007009;
	const float kfB1 =  kfE2 * 0.0914;
	const float kfC1 =  kfE0 * 0.007009 - kfE1 * 0.0914;
	const float kfA2 =  kfE1 * 0.3636 - kfE2 * 0.2237;
	const float kfB2 =  kfE2 * 0.1284 - kfE0 * 0.3636;
	const float kfC2 =  kfE0 * 0.2237 - kfE1 * 0.1284;

	// Conversion of linear RGB into the LMS color space.
	const float3 kvL = float3(0.05059983, 0.08585369, 0.00952420);
	const float3 kvM = float3(0.01893033, 0.08925308, 0.01370054);
	const float3 kvS = float3(0.00292202, 0.00975732, 0.07145979);

	// sRGB -> linear RGB.
	float4 l = NonLinearToLinear(c);

	// linear RGB -> LMS color space.
	float L = dot(kvL, l.rgb);
	float M = dot(kvM, l.rgb);
	float S = dot(kvS, l.rgb);

	// Apply factors to shift blue towards green and yellow
	// towards violet. Luminance of 0 means black, which
	// means no conversion.
	if (L != 0)
	{
		float t = (M / L);
		S = (t < kfInflectionPoint)
			? (-(kfA1 * L + kfB1 * M) / kfC1)
			: (-(kfA2 * L + kfB2 * M) / kfC2);
	}

	// Convert saturated values back into linear RGB.
	float lr = saturate(L * 30.830854 - M * 29.832659 + S * 1.610474);
	float lg = saturate(-L * 6.481468 + M * 17.715578 - S * 2.532642);
	float lb = saturate(-L * 0.375690 - M * 1.199062  + S * 14.273846);

	// Convert linear RGB back into sRGB.
	return LinearToNonLinear(float4(lr, lg, lb, l.a));
}

/**
 * Effect of Achromatopsia is complete loss of color (conversion to grayscale).
 */
float4 GrayscaleFilter(float4 c)
{
	// sRGB -> linear RGB.
	float4 l = NonLinearToLinear(c);

	// Compute grayscale factor.
	float f = dot(kvAchromatopsiaFactors, l.rgb);

	// Assign and saturate.
	l.rgb = saturate(float3(f, f, f));

	// Back to sRGB color space.
	return LinearToNonLinear(l);
}

///////////////////////////////////////////////////////////////////////////////
// Vertex Shaders
///////////////////////////////////////////////////////////////////////////////
vsOut Vertex(vsIn input)
{
	vsOut ret;

	// Under OpenGL ES 2.0, we need to flip the Y position, since coordinates
	// are bottom-up instead of our expected top-down.
#if SEOUL_OGLES2
	ret.Position = SEOUL_OUT_POS(float4(float2(input.Position.x, -input.Position.y), 0, 1));
#else
	ret.Position = SEOUL_OUT_POS(float4(input.Position.xy, 0, 1));
#endif

	ret.TexCoords = input.TexCoords;
	return ret;
}

///////////////////////////////////////////////////////////////////////////////
// Fragment Shaders
///////////////////////////////////////////////////////////////////////////////
half4 FragmentDeutanopia(vsOut input) : SEOUL_FRAG0_OUT
{
	float4 c = SEOUL_TEX2D(TextureTexture, TextureSampler, input.TexCoords);
	c = RedGreenFilter(kvDeutanopiaFactors, c);
	return half4(c);
}

half4 FragmentProtanopia(vsOut input) : SEOUL_FRAG0_OUT
{
	float4 c = SEOUL_TEX2D(TextureTexture, TextureSampler, input.TexCoords);
	c = RedGreenFilter(kvProtanopiaFactors, c);
	return half4(c);
}

half4 FragmentTritanopia(vsOut input) : SEOUL_FRAG0_OUT
{
	float4 c = SEOUL_TEX2D(TextureTexture, TextureSampler, input.TexCoords);
	c = TritanopiaFilter(c);
	return half4(c);
}

half4 FragmentAchromatopsia(vsOut input) : SEOUL_FRAG0_OUT
{
	float4 c = SEOUL_TEX2D(TextureTexture, TextureSampler, input.TexCoords);
	c = GrayscaleFilter(c);
	return half4(c);
}

///////////////////////////////////////////////////////////////////////////////
// Techniques
///////////////////////////////////////////////////////////////////////////////
SEOUL_TECHNIQUE(seoul_RenderDeutanopia)
{
	pass
	{
		SEOUL_PS(FragmentDeutanopia());
		SEOUL_VS(Vertex());
	}
}

SEOUL_TECHNIQUE(seoul_RenderProtanopia)
{
	pass
	{
		SEOUL_PS(FragmentProtanopia());
		SEOUL_VS(Vertex());
	}
}

SEOUL_TECHNIQUE(seoul_RenderTritanopia)
{
	pass
	{
		SEOUL_PS(FragmentTritanopia());
		SEOUL_VS(Vertex());
	}
}

SEOUL_TECHNIQUE(seoul_RenderAchromatopsia)
{
	pass
	{
		SEOUL_PS(FragmentAchromatopsia());
		SEOUL_VS(Vertex());
	}
}

#endif // include guard
