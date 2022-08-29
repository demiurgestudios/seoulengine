/**
 * \file Common.fxh
 * \brief Root header for all effects. Defines shared constants,
 * uniforms, and functions.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#ifndef COMMON_FXH
#define COMMON_FXH

//////////////////////////////////////////////////////////////////////////////
// Static Constants
//////////////////////////////////////////////////////////////////////////////
/**
 * Standard factors applied when rendering overdraw mode - per component
 * colors are keyed off of this base value.
 */
static const float kfBaseOverdrawFactor = 0.125;

/**
 * Factors applied to each color component of overdraw modes. By using
 * half against each component, it allows for a form of "high dynamic
 * range" W.R.T. visualizing overdraw.
 */
static const float4 kvOverdrawFactors = float4(0.5, 0.25, 0.125, 0.0) * kfBaseOverdrawFactor;

/** Pi constant. */
static const float kfPI = 3.1415926535897932;

//////////////////////////////////////////////////////////////////////////////
// Uniform constants
//////////////////////////////////////////////////////////////////////////////
#if SEOUL_D3D9
/**
 * \brief Derivations from render target dimensions used for accounting
 * for half-pixel offsets in D3D9.
 *
 * The correction is a negative half-pixel offset - because we're
 * in clip space, this turns into a correction of:
 *
 * = (float2(-0.5, -0.5) * float2(2.0, -2.0)) / float2(target_width, target_height)
 * = (float2(-1.0, 1.0) / float2(target_width, target_height))
 *
 * These values are expected to be in g_vHalfPixelOffsets.xy
 */
float4 g_vHalfPixelOffsets : seoul_HalfPixelOffsets = float4(0, 0, 0, 0);
#endif

//////////////////////////////////////////////////////////////////////////////
// Macros to handle differences in D3D9 vs. D3D11
//////////////////////////////////////////////////////////////////////////////
#if SEOUL_D3D11
#	define SEOUL_FRAG0_OUT SV_Target
#	define SEOUL_FRAG1_OUT SV_Target1
#	define SEOUL_FRAG2_OUT SV_Target2
#	define SEOUL_FRAG3_OUT SV_Target3
#	define SEOUL_POSITION_OUT SV_POSITION
#	define SEOUL_TECHNIQUE(name) technique11 name
#	define SEOUL_TEXTURE(texture_name, sampler_name, semantic) Texture2D texture_name : semantic; SamplerState sampler_name
#	define SEOUL_TEXTURE2D_TYPE Texture2D
#	define SEOUL_PS(func) SetPixelShader(CompileShader(ps_4_0, func))
#	define SEOUL_PS_NULL SetPixelShader(0);
#	define SEOUL_VS(func) SetVertexShader(CompileShader(vs_4_0, func))
#	define SEOUL_VS_NULL SetVertexShader(0);
#	define SEOUL_TEX2D(texture_name, sampler_name, texture_coordinates) texture_name.Sample(sampler_name, texture_coordinates)
#	define SEOUL_TEX2DLOD0(texture_name, sampler_name, texture_coordinates) texture_name.SampleLevel(sampler_name, texture_coordinates, 0)
#	define SEOUL_TEX2DPROJ(texture_name, sampler_name, texture_coordinates) texture_name.Sample(sampler_name, (texture_coordinates.xy / texture_coordinates.w))
#else // #if !SEOUL_D3D11
#	define SEOUL_FRAG0_OUT Color0
#	define SEOUL_FRAG1_OUT Color1
#	define SEOUL_FRAG2_OUT Color2
#	define SEOUL_FRAG3_OUT Color3
#	define SEOUL_TECHNIQUE(name) technique name
#	if SEOUL_OGLES2
#		define SEOUL_TEXTURE(texture_name, sampler_name, semantic) texture texture_name; sampler sampler_name : semantic = sampler_state
#	else
#		define SEOUL_TEXTURE(texture_name, sampler_name, semantic) texture texture_name : semantic; sampler sampler_name = sampler_state
#	endif
#	define SEOUL_TEXTURE2D_TYPE texture
#	define SEOUL_PS(func) PixelShader = compile ps_3_0 func
#	define SEOUL_PS_NULL PixelShader = null;
#	define SEOUL_VS(func) VertexShader = compile vs_3_0 func
#	define SEOUL_VS_NULL VertexShader = null;
#	define SEOUL_POSITION_OUT POSITION
#	define SEOUL_TEX2D(texture_name, sampler_name, texture_coordinates) tex2D(sampler_name, texture_coordinates)
#	define SEOUL_TEX2DLOD0(texture_name, sampler_name, texture_coordinates) tex2Dlod(sampler_name, float4(texture_coordinates, 0, 0))
#	define SEOUL_TEX2DPROJ(texture_name, sampler_name, texture_coordinates) tex2Dproj(sampler_name, texture_coordinates)
#endif // /#if !SEOUL_D3D11

//////////////////////////////////////////////////////////////////////////////
// Extended blend mode evaluation macro
//////////////////////////////////////////////////////////////////////////////
#define SEOUL_FOR_EACH_EXT_BLEND_MODE \
	SEOUL_EXT_BLEND_ITER(InvSrcAlpha, One, Inv_Src_Alpha, One); \
	SEOUL_EXT_BLEND_ITER(InvSrcColor, One, Inv_Src_Color, One); \
	SEOUL_EXT_BLEND_ITER(One, InvSrcColor, One, Inv_Src_Color); \
	SEOUL_EXT_BLEND_ITER(One, SrcAlpha, One, Src_Alpha); \
	SEOUL_EXT_BLEND_ITER(One, SrcColor, One, Src_Color); \
	SEOUL_EXT_BLEND_ITER(SrcAlpha, InvSrcAlpha, Src_Alpha, Inv_Src_Alpha); \
	SEOUL_EXT_BLEND_ITER(SrcAlpha, InvSrcColor, Src_Alpha, Inv_Src_Color); \
	SEOUL_EXT_BLEND_ITER(SrcAlpha, One, Src_Alpha, One); \
	SEOUL_EXT_BLEND_ITER(SrcAlpha, SrcAlpha, Src_Alpha, Src_Alpha); \
	SEOUL_EXT_BLEND_ITER(SrcColor, InvSrcAlpha, Src_Color, Inv_Src_Alpha); \
	SEOUL_EXT_BLEND_ITER(SrcColor, InvSrcColor, Src_Color, Inv_Src_Color); \
	SEOUL_EXT_BLEND_ITER(SrcColor, One, Src_Color, One); \
	SEOUL_EXT_BLEND_ITER(Zero, InvSrcColor, Zero, Inv_Src_Color); \
	SEOUL_EXT_BLEND_ITER(Zero, SrcColor, Zero, Src_Color);

//////////////////////////////////////////////////////////////////////////////
// Macros to handle platform differences in various states and functions.
//////////////////////////////////////////////////////////////////////////////
#if SEOUL_D3D11
	BlendState seoul_AlphaBlend
	{
		AlphaToCoverageEnable = false;
		BlendEnable[0] = true;
		BlendOp = Add;
		BlendOpAlpha = Add;
		DestBlend = Inv_Src_Alpha;
		DestBlendAlpha = Zero;
		RenderTargetWriteMask[0] = 0x07;
		SrcBlend = Src_Alpha;
		SrcBlendAlpha = One;
	};

	BlendState seoul_AlphaBlendPreMultiplied
	{
		AlphaToCoverageEnable = false;
		BlendEnable[0] = true;
		BlendOp = Add;
		BlendOpAlpha = Add;
		DestBlend = Inv_Src_Alpha;
		DestBlendAlpha = Zero;
		RenderTargetWriteMask[0] = 0x07;
		SrcBlend = One;
		SrcBlendAlpha = One;
	};

	BlendState seoul_ShadowAccumulateBlend
	{
		AlphaToCoverageEnable = false;
		BlendEnable[0] = true;
		BlendOp = Add;
		BlendOpAlpha = Min;
		DestBlend = One;
		DestBlendAlpha = One;
		RenderTargetWriteMask[0] = 0x08;
		SrcBlend = Zero;
		SrcBlendAlpha = One;
	};

	BlendState seoul_ShadowApplyBlend
	{
		AlphaToCoverageEnable = false;
		BlendEnable[0] = true;
		BlendOp = Add;
		BlendOpAlpha = Add;
		DestBlend = Dest_Alpha;
		DestBlendAlpha = One;
		RenderTargetWriteMask[0] = 0x0F;
		SrcBlend = Zero;
		SrcBlendAlpha = Zero;
	};

	DepthStencilState seoul_DisableDepthAndStencil
	{
		DepthEnable = false;
		DepthWriteMask = Zero;
		DepthFunc = Less;
		StencilEnable = false;
	};

	DepthStencilState seoul_DisableDepthEnableStencil
	{
		BackFaceStencilFail = Keep;
		BackFaceStencilFunc = Equal;
		BackFaceStencilPass = Replace;
		BackFaceStencilDepthFail = Keep;
		DepthEnable = false;
		DepthWriteMask = Zero;
		DepthFunc = Less;
		StencilEnable = true;
		FrontFaceStencilFail = Keep;
		FrontFaceStencilFunc = Equal;
		FrontFaceStencilPass = Replace;
		FrontFaceStencilDepthFail = Keep;
		StencilReadMask = 0;
		StencilWriteMask = 0;
	};

	DepthStencilState seoul_EnableDepthAndStencil
	{
		BackFaceStencilFail = Keep;
		BackFaceStencilFunc = Equal;
		BackFaceStencilPass = Replace;
		BackFaceStencilDepthFail = Keep;
		DepthEnable = true;
		DepthWriteMask = All;
		DepthFunc = Less;
		StencilEnable = true;
		FrontFaceStencilFail = Keep;
		FrontFaceStencilFunc = Equal;
		FrontFaceStencilPass = Replace;
		FrontFaceStencilDepthFail = Keep;
		StencilReadMask = 0;
		StencilWriteMask = 0;
	};

	DepthStencilState seoul_EnableDepthDisableStencil
	{
		DepthEnable = true;
		DepthWriteMask = All;
		DepthFunc = Less;
		StencilEnable = false;
	};

	DepthStencilState seoul_EnableDepthNoWritesDisableStencil
	{
		DepthEnable = true;
		DepthWriteMask = Zero;
		DepthFunc = Less;
		StencilEnable = false;
	};

	BlendState seoul_Opaque
	{
		AlphaToCoverageEnable = false;
		BlendEnable[0] = false;
		RenderTargetWriteMask[0] = 0x0F;
	};

	RasterizerState seoul_RasterizerStage2D
	{
		CullMode = None;
		DepthBias = 0;
		FillMode = Solid;
		FrontCounterClockwise = true;

		// Scissor always enabled under D3D11, it will be disabled
		// at runtime by expanding the scissor rectangle to the
		// viewport.
		ScissorEnable = true;
	};

	RasterizerState seoul_RasterizerStage3DFx
	{
		CullMode = None;
		DepthBias = 0;
		FillMode = Solid;
		FrontCounterClockwise = true;

		// Scissor always enabled under D3D11, it will be disabled
		// at runtime by expanding the scissor rectangle to the
		// viewport.
		ScissorEnable = true;
	};

	RasterizerState seoul_RasterizerStage3DUnlit
	{
		CullMode = Back;
		DepthBias = 0;
		FillMode = Solid;
		FrontCounterClockwise = true;

		// Scissor always enabled under D3D11, it will be disabled
		// at runtime by expanding the scissor rectangle to the
		// viewport.
		ScissorEnable = true;
	};

	RasterizerState seoul_RasterizerStage3DWireframe
	{
		CullMode = Back;
		DepthBias = 0;
		FillMode = Wireframe;
		FrontCounterClockwise = true;

		// Scissor always enabled under D3D11, it will be disabled
		// at runtime by expanding the scissor rectangle to the
		// viewport.
		ScissorEnable = true;
	};

	RasterizerState seoul_RasterizerStage3DLines
	{
		CullMode = None;
		DepthBias = 0;
		FillMode = Solid;
		FrontCounterClockwise = true;

		// Scissor always enabled under D3D11, it will be disabled
		// at runtime by expanding the scissor rectangle to the
		// viewport.
		ScissorEnable = true;
	};

	// Extended blend modes.
#	define SEOUL_EXT_BLEND_ITER(d3d9src, d3d9dst, src, dst) \
		BlendState seoul_ExtendedBlend_##d3d9src##_##d3d9dst \
		{ \
			AlphaToCoverageEnable = false; \
			BlendEnable[0] = true; \
			BlendOp = Add; \
			BlendOpAlpha = Add; \
			DestBlend = dst; \
			DestBlendAlpha = Zero; \
			RenderTargetWriteMask[0] = 0x07; \
			SrcBlend = src; \
			SrcBlendAlpha = One; \
		}

	// Eval.
	SEOUL_FOR_EACH_EXT_BLEND_MODE

#	undef SEOUL_EXT_BLEND_ITER
#	define SEOUL_EXT_BLEND_MODE(d3d9src, d3d9dst, src, dst) \
		SetBlendState(seoul_ExtendedBlend_##d3d9src##_##d3d9dst, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);

#	define SEOUL_ALPHA_BLEND SetBlendState(seoul_AlphaBlend, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
#	define SEOUL_ALPHA_BLEND_PRE_MULTIPLIED SetBlendState(seoul_AlphaBlendPreMultiplied, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
#	define SEOUL_SHADOW_ACCUMULATE_BLEND SetBlendState(seoul_ShadowAccumulateBlend, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
#	define SEOUL_SHADOW_APPLY_BLEND SetBlendState(seoul_ShadowApplyBlend, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
#	define SEOUL_DISABLE_DEPTH_AND_STENCIL SetDepthStencilState(seoul_DisableDepthAndStencil, 0);
#	define SEOUL_DISABLE_DEPTH_ENABLE_STENCIL SetDepthStencilState(seoul_DisableDepthEnableStencil, 0xFF);
#	define SEOUL_ENABLE_DEPTH_AND_STENCIL SetDepthStencilState(seoul_EnableDepthAndStencil, 0xFF);
#	define SEOUL_ENABLE_DEPTH_DISABLE_STENCIL SetDepthStencilState(seoul_EnableDepthDisableStencil, 0xFF);
#	define SEOUL_ENABLE_DEPTH_NO_WRITES_DISABLE_STENCIL SetDepthStencilState(seoul_EnableDepthNoWritesDisableStencil, 0xFF);
#	define SEOUL_RASTERIZER_STAGE_2D SetRasterizerState(seoul_RasterizerStage2D);
#	define SEOUL_RASTERIZER_STAGE_3D_FX SetRasterizerState(seoul_RasterizerStage3DFx);
#	define SEOUL_RASTERIZER_STAGE_3D_UNLIT SetRasterizerState(seoul_RasterizerStage3DUnlit);
#	define SEOUL_RASTERIZER_STAGE_3D_WIREFRAME SetRasterizerState(seoul_RasterizerStage3DWireframe);
#	define SEOUL_RASTERIZER_STAGE_3D_LINES  SetRasterizerState(seoul_RasterizerStage3DLines);
#	define SEOUL_OPAQUE SetBlendState(seoul_Opaque, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
#else
#	define SEOUL_ALPHA_BLEND \
		AlphaBlendEnable = true; \
		AlphaTestEnable = false; \
		BlendOp = Add; \
		ColorWriteEnable = RED|GREEN|BLUE; \
		DestBlend = InvSrcAlpha; \
		SeparateAlphaBlendEnable = false; \
		SrcBlend = SrcAlpha;
#	define SEOUL_ALPHA_BLEND_PRE_MULTIPLIED \
		AlphaBlendEnable = true; \
		AlphaTestEnable = false; \
		BlendOp = Add; \
		ColorWriteEnable = RED|GREEN|BLUE; \
		DestBlend = InvSrcAlpha; \
		SeparateAlphaBlendEnable = false; \
		SrcBlend = One;

#	define SEOUL_EXT_BLEND_MODE(d3d9src, d3d9dst, src, dst) \
		AlphaBlendEnable = true; \
		AlphaTestEnable = false; \
		BlendOp = Add; \
		ColorWriteEnable = RED|GREEN|BLUE; \
		DestBlend = d3d9dst; \
		SeparateAlphaBlendEnable = false; \
		SrcBlend = d3d9src;

		// TODO: This is a workaround for a bug (?) where BlendOpAlpha
		// is ignore/misinterpreted as BlendOp under D3D9. This does not occur
		// when translating this shader into GLSL, which should be exactly
		// the same result.
		//
		// For the D3D9 version of this state, we rely on ColorWriteEnable to
		// disable color channel updates, instead of the blend mode.
#if SEOUL_D3D9
#	define SEOUL_SHADOW_ACCUMULATE_BLEND \
		AlphaBlendEnable = true; \
		AlphaTestEnable = false; \
		BlendOp = Min; \
		ColorWriteEnable = ALPHA; \
		DestBlend = One; \
		SeparateAlphaBlendEnable = false; \
		SrcBlend = One;
#else
#	define SEOUL_SHADOW_ACCUMULATE_BLEND \
		AlphaBlendEnable = true; \
		AlphaTestEnable = false; \
		BlendOp = Add; \
		BlendOpAlpha = Min; \
		ColorWriteEnable = ALPHA; \
		DestBlend = One; \
		DestBlendAlpha = One; \
		SeparateAlphaBlendEnable = true; \
		SrcBlend = Zero; \
		SrcBlendAlpha = One;
#endif
#	define SEOUL_SHADOW_APPLY_BLEND \
		AlphaBlendEnable = true; \
		AlphaTestEnable = false; \
		BlendOp = Add; \
		BlendOpAlpha = Add; \
		ColorWriteEnable = RED|GREEN|BLUE|ALPHA; \
		DestBlend = DestAlpha; \
		DestBlendAlpha = One; \
		SeparateAlphaBlendEnable = true; \
		SrcBlend = Zero; \
		SrcBlendAlpha = Zero;
#	define SEOUL_DISABLE_DEPTH_AND_STENCIL \
		ZEnable = false; \
		ZWriteEnable = false; \
		StencilEnable = false; \
		TwoSidedStencilMode = false;
#	define SEOUL_DISABLE_DEPTH_ENABLE_STENCIL \
		ZEnable = false; \
		ZWriteEnable = false; \
		StencilEnable = true; \
		StencilFail = Keep; \
		StencilZFail = Keep; \
		StencilPass = Replace; \
		StencilFunc = Equal; \
		StencilMask = 0; \
		StencilRef = 0xFF; \
		StencilWriteMask = 0; \
		TwoSidedStencilMode = false;
#	define SEOUL_ENABLE_DEPTH_AND_STENCIL \
		ZEnable = true; \
		ZFunc = Less; \
		ZWriteEnable = true; \
		StencilEnable = true; \
		StencilFail = Keep; \
		StencilZFail = Keep; \
		StencilPass = Replace; \
		StencilFunc = Equal; \
		StencilMask = 0; \
		StencilRef = 0xFF; \
		StencilWriteMask = 0; \
		TwoSidedStencilMode = false;
#	define SEOUL_ENABLE_DEPTH_DISABLE_STENCIL \
		ZEnable = true; \
		ZFunc = Less; \
		ZWriteEnable = true; \
		StencilEnable = false; \
		TwoSidedStencilMode = false;
#	define SEOUL_ENABLE_DEPTH_NO_WRITES_DISABLE_STENCIL \
		ZEnable = true; \
		ZFunc = Less; \
		ZWriteEnable = false; \
		StencilEnable = false; \
		TwoSidedStencilMode = false;
#	define SEOUL_OPAQUE \
		AlphaBlendEnable = false; \
		AlphaTestEnable = false; \
		ColorWriteEnable = RED|GREEN|BLUE|ALPHA;
#	define SEOUL_RASTERIZER_STAGE_2D \
		CullMode = None; \
		DepthBias = 0; \
		FillMode = Solid;
#	define SEOUL_RASTERIZER_STAGE_3D_FX \
		CullMode = None; \
		DepthBias = 0; \
		FillMode = Solid;
#	define SEOUL_RASTERIZER_STAGE_3D_UNLIT \
		CullMode = Cw; \
		DepthBias = 0; \
		FillMode = Solid;
#	define SEOUL_RASTERIZER_STAGE_3D_WIREFRAME \
		CullMode = Cw; \
		DepthBias = 0; \
		FillMode = Wireframe;
#	define SEOUL_RASTERIZER_STAGE_3D_LINES \
		CullMode = None; \
		DepthBias = 0; \
		FillMode = Solid;
#endif

/**
 * \brief Use this #define on clip space position output. Handles
 * correction of half-pixel offset under D3D9.
 */
#if SEOUL_D3D9
#	define SEOUL_OUT_POS(v) float4(v.xy + g_vHalfPixelOffsets.xy * v.w, v.zw)
#else
#	define SEOUL_OUT_POS(v) v
#endif

//////////////////////////////////////////////////////////////////////////////
// Standard functions
//////////////////////////////////////////////////////////////////////////////
/**
 * \brief Handles swizzling of vertex color under various APIs.
 */
float4 GetVertexColor(float4 c)
{
#if SEOUL_D3D9 // Need to swap r and b when rendering with D3D9.
	return c.bgra;
#else
	return c;
#endif
}

/*
 * \brief Converts a linear color value into non-linear space, using sRGB
 * gamma correction.
 *
 * \sa http://en.wikipedia.org/wiki/SRGB
 */
float4 LinearToNonLinear(float4 c)
{
#pragma warning(push)
#pragma warning(disable : 3571)
        static const float kThreshold = 0.0031308;
        static const float kAlpha = 0.055;
        static const float k1 = (12.92);
        static const float k2 = (kAlpha + 1.0);
        static const float kPower = (1.0 / 2.4);

        float4 ret;
        ret.r = (c.r <= kThreshold) ? (c.r * k1) : (k2 * pow(c.r, kPower)) - kAlpha;
        ret.g = (c.g <= kThreshold) ? (c.g * k1) : (k2 * pow(c.g, kPower)) - kAlpha;
        ret.b = (c.b <= kThreshold) ? (c.b * k1) : (k2 * pow(c.b, kPower)) - kAlpha;
        ret.a = c.a;

        return ret;
#pragma warning(pop)
}

/**
 * \brief Wrapper for multiply-add, which on some
 * platforms is a single shader instruction.
 */
float2 MultiplyAdd(float m, float a, float b)
{
	return (m * a) + b;
}

/**
 * \brief Wrapper for multiply-add, which on some
 * platforms is a single shader instruction.
 */
float2 MultiplyAdd(float2 m, float2 a, float2 b)
{
	return (m * a) + b;
}

/**
 * \brief Wrapper for multiply-add, which on some
 * platforms is a single shader instruction.
 */
float3 MultiplyAdd(float3 m, float3 a, float3 b)
{
	return (m * a) + b;
}

/**
 * \brief Wrapper for multiply-add, which on some
 * platforms is a single shader instruction.
 */
float4 MultiplyAdd(float4 m, float4 a, float4 b)
{
	return (m * a) + b;
}

/**
 * \brief Converts a non-linear color value into linear space, using sRGB
 * gamma correction.
 *
 * \sa http://en.wikipedia.org/wiki/SRGB
 */
float4 NonLinearToLinear(float4 c)
{
#pragma warning(push)
#pragma warning(disable : 3571)
        static const float kThreshold = 0.04045;
        static const float kAlpha = 0.055;
        static const float k1 = (1.0 / 12.92);
        static const float k2 = (1.0 / (kAlpha + 1.0));
        static const float kPower = 2.4;

        float4 ret;
        ret.r = (c.r <= kThreshold) ? (c.r * k1) : pow(((c.r + kAlpha) * k2), kPower);
        ret.g = (c.g <= kThreshold) ? (c.g * k1) : pow(((c.g + kAlpha) * k2), kPower);
        ret.b = (c.b <= kThreshold) ? (c.b * k1) : pow(((c.b + kAlpha) * k2), kPower);
        ret.a = c.a;

        return ret;
#pragma warning(pop)
}

/**
 * \brief Wrapper to sample from an alpha only texture - on
 * some platforms, the single component texture is not the
 * alpha component, so this function abstracts that distinction.
 */
half Tex2DAlpha(uniform SEOUL_TEXTURE2D_TYPE textureTexture, uniform sampler textureSampler, float2 uv)
{
	return half(SEOUL_TEX2D(textureTexture, textureSampler, uv).a);
}

/**
 * \brief Wrapper to sample color from a texture - on some
 * platforms, we use a special encoding to separate alpha
 * and color into 2 different textures.
 */
half4 Tex2DColor(uniform SEOUL_TEXTURE2D_TYPE textureTexture, uniform sampler textureSampler, float2 uv)
{
	half4 ret = half4(SEOUL_TEX2D(textureTexture, textureSampler, uv));
	return ret;
}

/**
 * \brief Wrapper to sample color from a texture - on some
 * platforms, we use a special encoding to separate alpha
 * and color into 2 different textures.
 */
half4 Tex2DColor(
	uniform SEOUL_TEXTURE2D_TYPE textureTexture,
	uniform sampler textureSampler,
	uniform SEOUL_TEXTURE2D_TYPE secondaryTextureTexture,
	uniform sampler secondaryTextureSampler,
	float2 uv)
{
	half4 ret = half4(SEOUL_TEX2D(textureTexture, textureSampler, uv));

	// Modulate the alpha by the secondary texture. There are 3 cases:
	// - main texture is uncompressed - secondaryTextureSampler contains a single
	//   pixel (255, 255, 255, 255).
	// - main texture is opaque and compressed with ETC1 - secondaryTextureSampler
	//   contains a single pixel (255, 255, 255, 255).
	// - main texture has alpha and is compressed wiht ETC1 - secondaryTextureSampler
	//   contains the alpha component of the original texture in the
	//   (R, G, B) channels.
#if SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_IOS
	ret.a *= half(SEOUL_TEX2D(secondaryTextureTexture, secondaryTextureSampler, uv).g);
#endif

	return ret;
}

#endif // include guard
