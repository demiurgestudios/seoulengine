/**
 * \file FalconRenderState.h
 * Shared state across Drawer, Poser, and Optimizer.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_RENDER_STATE_H
#define FALCON_RENDER_STATE_H

#include "Delegate.h"
#include "FalconRenderCommand.h"
#include "FalconTypes.h"
#include "Prereqs.h"
#include "SeoulMath.h"
#include "FalconTextureCacheSettings.h"
namespace Seoul { namespace Falcon { class ClipStack; } }
namespace Seoul { namespace Falcon { namespace Render { class Features; } } }
namespace Seoul { namespace Falcon { class RendererInterface; } }
namespace Seoul { namespace Falcon { class Stage3DSettings; } }
namespace Seoul { namespace Falcon { class Texture; } }
namespace Seoul { namespace Falcon { class TextureCache; } }
namespace Seoul { namespace Falcon { struct TextureCacheSettings; } }

namespace Seoul::Falcon
{

namespace Render
{

struct StateSettings SEOUL_SEALED
{
	StateSettings()
		: m_DrawTriangleListRI()
		, m_CacheSettings()
		, m_pInterface(nullptr)
		, m_uMaxIndexCountBatch(8192u)
		, m_uMaxVertexCountBatch(2048u)
	{
	}

	// Up reference to the GPU backend, used by Drawer to submit a draw command.
	Delegate<void(
		const SharedPtr<Texture>& pColorTex,
		const SharedPtr<Texture>& pDetailTex,
		UInt16 const* pIndices,
		UInt32 uIndexCount,
		Float32 const* pDepths3D,
		ShapeVertex const* pVertices,
		UInt32 uVertexCount,
		const Features& features)> m_DrawTriangleListRI;

	// Configuration of the Renderer texture cache.
	TextureCacheSettings m_CacheSettings;

	// Up reference to the GPU backend.
	Falcon::RendererInterface* m_pInterface;

	// Used for clamping index and vertex buffer generation.
	UInt32 m_uMaxIndexCountBatch;
	UInt32 m_uMaxVertexCountBatch;
}; // struct StateSettings

class State SEOUL_SEALED
{
public:
	State(const StateSettings& settings);
	~State();

	Float ComputeDepth3D(Float fY) const
	{
		// Clamp here is NaN safe, so this is correct, even if the stage top/bottom is 0.
		return Clamp((fY - m_fStage3DTopY) / (m_fStage3DBottomY - m_fStage3DTopY), 0.0f, 1.0f);
	}

	/** W is 1.0f / Clamp(1.0f - (depth * perspective)), used for 3D planar projection. */
	Float ComputeCurrentOneOverW() const
	{
		return ComputeCurrentOneOverW(GetModifiedDepth3D());
	}

	/** W is 1.0f / Clamp(1.0f - (depth * perspective)), used for 3D planar projection. */
	Float ComputeCurrentOneOverW(Float fDepth) const
	{
		Float const fW = 1.0f / Clamp(1.0f - (fDepth * GetPerspectiveFactor()), 1e-4f, 1.0f);
		return fW;
	}

	/** W is Clamp(1.0f - (depth * perspective)), used for 3D planar projection. */
	Float ComputeCurrentW() const
	{
		Float const fW = Clamp(1.0f - (GetModifiedDepth3D() * GetPerspectiveFactor()), 0.0f, 1.0f);
		return fW;
	}

	/** Return project depth value, factoring in m_iIgnoreDepthProjection. */
	Float GetModifiedDepth3D() const { return (0 == m_iIgnoreDepthProjection ? m_fRawDepth3D : 0.0f); }

	/**
	 * Given a point in 3D projected world space, return the unprojected 2D world space.
	 */
	Vector2D InverseProject(const Vector2D& v) const
	{
		return InverseProject(v, GetModifiedDepth3D());
	}

	Vector2D InverseProject(const Vector2D& v, Float fDepth) const
	{
		Float const fW = ComputeCurrentOneOverW(fDepth);
		auto const& vp = m_vViewProjectionTransform;
		auto const vScale = vp.GetXY();
		auto const vShift = vp.GetZW();

		// Convert Falcon world space into clip space.
		auto const vProj(Vector2D(
			(((v.X - m_WorldCullRectangle.m_fLeft) / m_WorldCullRectangle.GetWidth()) - 0.5f) * 2.0f,
			(((v.Y - m_WorldCullRectangle.m_fTop) / m_WorldCullRectangle.GetHeight()) - 0.5f) * -2.0f));

		// Now multiply by W to deproject the point.
		auto const vPostProj = (vProj * fW);

		// Finally, apply the inverse of the view projection transform to place the point
		// back in world vUnproj.
		auto const vUnproj = Vector2D::ComponentwiseDivide((vPostProj - vShift), vScale);

		return vUnproj;
	}

	/**
	* Project a 2D point to its 3D post projection position - meant for bounds compensation
	* and other CPU side computations. Rendering projection is done by the GPU so that
	* texture sampling is perspective correct.
	*/
	Vector2D Project(const Vector2D& v) const
	{
		return Project(v, GetModifiedDepth3D());
	}

	Vector2D Project(const Vector2D& v, Float fDepth) const
	{
		Float const fW = ComputeCurrentOneOverW(fDepth);
		auto const& vp = m_vViewProjectionTransform;
		auto const vScale = vp.GetXY();
		auto const vShift = vp.GetZW();

		// Project the point into projection space.
		auto const vProj = Vector2D::ComponentwiseMultiply(v, vScale) + vShift;

		// Now divide by W to place the coordinate in clip space [-1, 1].
		auto const vPostProj = (vProj * fW);

		// Because our UI world space is just a 2D space, we can convert clip space
		// back into Falcon world space with a rescale and shift.
		auto const vUnproj(Vector2D(
			(vPostProj.X * 0.5f + 0.5f) * m_WorldCullRectangle.GetWidth() + m_WorldCullRectangle.m_fLeft,
			(vPostProj.Y * -0.5f + 0.5f) * m_WorldCullRectangle.GetHeight() + m_WorldCullRectangle.m_fTop));

		return vUnproj;
	}

	void EndPhase();

	Float GetPerspectiveFactor() const;

	StateSettings const m_Settings;
	ScopedPtr<TextureCache> m_pCache;
	ScopedPtr<ClipStack> m_pClipStack;
	ScopedPtr<Stage3DSettings> m_pStage3DSettings;
	Render::CommandBuffer m_Buffer;
	Int32 m_iInPlanarShadowRender;
	Int32 m_iInDeferredDrawingRender;
	Vector4D m_vViewProjectionTransform;
	Rectangle m_WorldCullRectangle;
	Float m_fWorldWidthToScreenWidth;
	Float m_fWorldHeightToScreenHeight;
	Double m_fMaxCostInBatchFromOverfill;
	Float m_fWorldCullScreenArea;
	Float32 m_fRawDepth3D;
	Int32 m_iIgnoreDepthProjection;
	Float32 m_fPerspectiveFactorAdjustment;
	Float m_fStage3DTopY;
	Float m_fStage3DBottomY;

private:
	SEOUL_DISABLE_COPY(State);
}; // class State

} // namespace Render

} // namespace Seoul::Falcon

#endif // include guard
