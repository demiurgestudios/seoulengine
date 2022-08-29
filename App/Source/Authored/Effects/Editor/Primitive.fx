/**
 * \file Primitive.fx
 * \brief Effect used for drawing simple 3D geometry in the game's world
 * (e.g. debug rendering lines and simple triangle shapes).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#ifndef PRIMITIVE_FX
#define PRIMITIVE_FX

#include "../Common/Common.fxh"

//////////////////////////////////////////////////////////////////////////////
// Static Constants
//////////////////////////////////////////////////////////////////////////////
// Scaling applied to a cell's derivative
// to compute fade out and line fattening.
static const float kGridFineFadeScale = 7.5;
static const float kGridCoarseFadeScale = 1.5;
static const float kGridLineScale = 17.0;

// Line width - in portion of a cell (a value
// of 1 here would draw a cell all black).
static const float kGridMaxLineWidth = 1.0;
static const float kGridMinLineWidth = 0.1;

// Used to increase or decrease weight, to
// use a heavier grid line at coarser cells.
static const float kGridFineFade = 0.4;
static const float kGridModulator = 10;

//////////////////////////////////////////////////////////////////////////////
// Uniform Constants
//////////////////////////////////////////////////////////////////////////////
/** Global world to view space transform. */
float4x4 View : seoul_View;

/** Global view to projection transform. */
float4x4 Projection : seoul_Projection;

///////////////////////////////////////////////////////////////////////////////
// Input/Output
///////////////////////////////////////////////////////////////////////////////
struct vsIn
{
	float4 PositionAndClip : POSITION;
	float4 Color : COLOR0;
};

struct vsInCameraLighting
{
	float4 PositionAndClip : POSITION;
	float3 Normal : NORMAL;
	float4 Color : COLOR0;
};

struct vsOut
{
	float4 Position : SEOUL_POSITION_OUT;
	float4 Color : COLOR0;
	float Clip : TEXCOORD0;
};

struct vsOutCameraLighting
{
	float4 Position : SEOUL_POSITION_OUT;
	float3 WorldNormal : TEXCOORD0;
	float4 Color : COLOR0;
	float Clip : TEXCOORD2;
};

struct vsOutGrid
{
	float4 Position : SEOUL_POSITION_OUT;
	float4 Color : COLOR0;
	float Clip : TEXCOORD0;
	float4 PositionWorld : TEXCOORD1;
};

///////////////////////////////////////////////////////////////////////////////
// Functions
///////////////////////////////////////////////////////////////////////////////
// Fogging and line fattening of grid lines.
void GetGridQualityFactors(
	float2 gridCoords,
	out float fineFadeOut,
	out float coarseFadeOut,
	out float lineWidth)
{
	// Compute grid cell derivative along x and y.
	float2 dx = ddx(gridCoords);
	float2 dy = ddy(gridCoords);

	// Overall derivative is the max of the
	// square of either dimension.
	float derivative = max(dot(dx, dx), dot(dy, dy));

	// Apply the derivative factor to line width
	// and fade, based on constant control values.
	lineWidth = lerp(kGridMinLineWidth, kGridMaxLineWidth, saturate(kGridLineScale * derivative));
	fineFadeOut = saturate(kGridFineFadeScale * derivative);
	coarseFadeOut = saturate(kGridCoarseFadeScale * derivative);
}

// Determine how much to blend in a black grid line -
// distance to a whole integer location.
float GetGridFactor(float2 position)
{
	// Get fade and line fattening factors.
	float fineFadeOut;
	float coarseFadeOut;
	float lineWidth;
	GetGridQualityFactors(
		position,
		fineFadeOut,
		coarseFadeOut,
		lineWidth);

	// Compute distance from grid edges at fine resolution.
	float2 fineRounded = round(position.xy);
	float2 fineAbsDiff = abs(position.xy - fineRounded.xy);
	float fineFactor = min(fineAbsDiff.x, fineAbsDiff.y);
	float fineAlpha = saturate((fineFactor / lineWidth) + kGridFineFade);

	// Compute distance from grid edges at coarse resolution.
	float2 coarseRounded = round(position.xy / kGridModulator) * kGridModulator;
	float2 coarseAbsDiff = abs(position.xy - coarseRounded.xy);
	float coarseFactor = min(coarseAbsDiff.x, coarseAbsDiff.y);
	float coarseAlpha = saturate(coarseFactor / lineWidth);

	// Alpha is the returned blend used to darken
	// edges to identify grid lines.
	return min(
		lerp(fineAlpha, 1.0, fineFadeOut),
		lerp(coarseAlpha, 1.0, coarseFadeOut));
}

///////////////////////////////////////////////////////////////////////////////
// Vertex Shaders
///////////////////////////////////////////////////////////////////////////////
vsOut Vertex(vsIn input)
{
	float4 view = mul(float4(input.PositionAndClip.xyz, 1.0), View);

	vsOut ret;
	ret.Position = SEOUL_OUT_POS(mul(view, Projection));
	ret.Color = GetVertexColor(input.Color);
	ret.Clip = (input.PositionAndClip.w + view.z);
	return ret;
}

vsOutCameraLighting VertexCameraLighting(vsInCameraLighting input)
{
	float4 view = mul(float4(input.PositionAndClip.xyz, 1.0), View);

	vsOutCameraLighting ret;
	ret.Position = SEOUL_OUT_POS(mul(view, Projection));
	ret.WorldNormal = input.Normal.xyz;
	ret.Color = GetVertexColor(input.Color);
	ret.Clip = (input.PositionAndClip.w + view.z);
	return ret;
}

vsOutGrid VertexGrid(vsIn input)
{
	float4 world = float4(input.PositionAndClip.xyz, 1.0);
	float4 view = mul(world, View);

	vsOutGrid ret;
	ret.Position = SEOUL_OUT_POS(mul(view, Projection));
	ret.Color = GetVertexColor(input.Color);
	ret.Clip = (input.PositionAndClip.w + view.z);
	ret.PositionWorld = world;
	return ret;
}

///////////////////////////////////////////////////////////////////////////////
// Fragment Shaders
///////////////////////////////////////////////////////////////////////////////
half4 Fragment(vsOut input) : SEOUL_FRAG0_OUT
{
	clip(input.Clip);

	return half4(input.Color);
}

half4 FragmentCameraLighting(vsOutCameraLighting input) : SEOUL_FRAG0_OUT
{
	clip(input.Clip);

	// Standard ndotl term.
	float ndotl = max(dot(normalize(input.WorldNormal.xyz), normalize(float3(1, 1, 1))), 0.0);

	// Half hemisphere lighting, don't fade to black.
	float lighting = lerp(0.6, 0.9, ndotl);

	return half4(
		input.Color.rgb * lighting,
		input.Color.a);
}

half4 FragmentGridXZ(vsOutGrid input) : SEOUL_FRAG0_OUT
{
	clip(input.Clip);

	half4 ret = half4(
		input.Color.rgb,
		input.Color.a * (1.0 - GetGridFactor(input.PositionWorld.xz)));
	return ret;
}

half4 FragmentGridXY(vsOutGrid input) : SEOUL_FRAG0_OUT
{
	clip(input.Clip);

	half4 ret = half4(
		input.Color.rgb,
		input.Color.a * (1.0 - GetGridFactor(input.PositionWorld.xy)));
	return ret;
}

half4 FragmentGridYZ(vsOutGrid input) : SEOUL_FRAG0_OUT
{
	clip(input.Clip);

	half4 ret = half4(
		input.Color.rgb,
		input.Color.a * (1.0 - GetGridFactor(input.PositionWorld.yz)));
	return ret;
}

///////////////////////////////////////////////////////////////////////////////
// Techniques
///////////////////////////////////////////////////////////////////////////////
#define SEOUL_PRIMITIVE_COMMON_STATES \
	SEOUL_DISABLE_STENCIL \
	FillMode = Solid;
#define SEOUL_PRIMITIVE_CULL_BACK_FACE \
	CullMode = SEOUL_BACK_FACE_CULLING;
#define SEOUL_PRIMITIVE_CULL_NONE \
	CullMode = None;
#define SEOUL_PRIMITIVE_OPAQUE SEOUL_OPAQUE
#define SEOUL_PRIMITIVE_BLENDED SEOUL_ALPHA_BLEND

SEOUL_TECHNIQUE(seoul_Render)
{
	pass
	{
		SEOUL_RASTERIZER_STAGE_3D_UNLIT
		SEOUL_ALPHA_BLEND
		SEOUL_ENABLE_DEPTH_DISABLE_STENCIL

		SEOUL_VS(Vertex());
		SEOUL_PS(Fragment());
	}
}

SEOUL_TECHNIQUE(seoul_RenderNoDepthTest)
{
	pass
	{
		SEOUL_RASTERIZER_STAGE_3D_UNLIT
		SEOUL_ALPHA_BLEND
		SEOUL_DISABLE_DEPTH_AND_STENCIL

		SEOUL_VS(Vertex());
		SEOUL_PS(Fragment());
	}
}

SEOUL_TECHNIQUE(seoul_RenderGizmo)
{
	pass
	{
		SEOUL_RASTERIZER_STAGE_3D_UNLIT
		SEOUL_ALPHA_BLEND
		SEOUL_DISABLE_DEPTH_AND_STENCIL

		SEOUL_VS(VertexCameraLighting());
		SEOUL_PS(FragmentCameraLighting());
	}
}

SEOUL_TECHNIQUE(seoul_RenderGizmoNoLighting)
{
	pass
	{
		SEOUL_RASTERIZER_STAGE_3D_UNLIT
		SEOUL_ALPHA_BLEND
		SEOUL_DISABLE_DEPTH_AND_STENCIL

		SEOUL_VS(Vertex());
		SEOUL_PS(Fragment());
	}
}

SEOUL_TECHNIQUE(seoul_RenderGridXZ)
{
	pass
	{
		SEOUL_RASTERIZER_STAGE_3D_LINES
		SEOUL_ALPHA_BLEND
		SEOUL_ENABLE_DEPTH_NO_WRITES_DISABLE_STENCIL

		SEOUL_VS(VertexGrid());
		SEOUL_PS(FragmentGridXZ());
	}
}

SEOUL_TECHNIQUE(seoul_RenderGridXY)
{
	pass
	{
		SEOUL_RASTERIZER_STAGE_3D_LINES
		SEOUL_ALPHA_BLEND
		SEOUL_ENABLE_DEPTH_NO_WRITES_DISABLE_STENCIL

		SEOUL_VS(VertexGrid());
		SEOUL_PS(FragmentGridXY());
	}
}

SEOUL_TECHNIQUE(seoul_RenderGridYZ)
{
	pass
	{
		SEOUL_RASTERIZER_STAGE_3D_LINES
		SEOUL_ALPHA_BLEND
		SEOUL_ENABLE_DEPTH_NO_WRITES_DISABLE_STENCIL

		SEOUL_VS(VertexGrid());
		SEOUL_PS(FragmentGridYZ());
	}
}

#endif // include guard
