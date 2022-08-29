/**
 * \file FalconRenderPoser.h
 * The render poser flattens the scene graph into a list
 * of instances to render, which can be further optimized
 * and re-ordered prior to render submission.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */


#pragma once
#ifndef FALCON_RENDER_POSER_H
#define FALCON_RENDER_POSER_H

#include "CheckedPtr.h"
#include "Color.h"
#include "FalconConstants.h"
#include "FalconRenderFeature.h"
#include "Pair.h"
#include "Prereqs.h"
#include "SharedPtr.h"
#include "Vector.h"
namespace Seoul { namespace Falcon { class BitmapDefinition; } }
namespace Seoul { namespace Falcon { struct ColorTransformWithAlpha; } }
namespace Seoul { namespace Falcon { class Renderable; } }
namespace Seoul { namespace Falcon { namespace Render { class State; } } }
namespace Seoul { namespace Falcon { struct Rectangle; } }
namespace Seoul { namespace Falcon { struct TextureReference; } }
namespace Seoul { class FilePath; }
namespace Seoul { struct Matrix2x3; }
namespace Seoul { struct StandardVertex2D; }
namespace Seoul { struct Vector2D; }

namespace Seoul::Falcon
{

namespace Render
{

enum class PoserResolveResult
{
	/** Texture resolve succeeded. */
	kSuccess,

	/** Texture resolve failed because resolution quad was culled. */
	kCulled,

	/** Texture resolve failed because resolution quad was culled, but texture has been successfully prefetched. */
	kCulledAndPrefetched,

	/** Texture resolve failed because texture is not yet loaded. */
	kNotReady,
};

/**
 * The Poser is responsible for flattening the Falcon
 * scene graph into a list of renderables, that can
 * be further rearranged and pruned, to optimize
 * rendering.
 */
class Poser SEOUL_SEALED
{
public:
	Poser();
	~Poser();

	void Begin(State& rState);
	void End();

	/**
	 * @return Shared renderer state. The state
	 * instance is used across the Poser, Drawer, and
	 * Optimizer.
	 */
	const State& GetState() const
	{
		return *m_pState;
	}

	void BeginScissorClip(const Rectangle& worldRectangle);
	void EndScissorClip();

	void BeginPlanarShadows();
	void EndPlanarShadows();

	void BeginDeferDraw();
	void EndDeferDraw();
	void FlushDeferredDraw();

	Bool InPlanarShadow() const;

	void ClipStackAddConvexHull(
		const Matrix2x3& m,
		StandardVertex2D const* p,
		UInt32 u,
		Float fTolerance = kfAboutEqualPosition);
	void ClipStackAddConvexHull(
		const Matrix2x3& m,
		Vector2D const* p,
		UInt32 u,
		Float fTolerance = kfAboutEqualPosition);
	void ClipStackAddRectangle(
		const Matrix2x3& m,
		const Rectangle& rect,
		Float fTolerance = kfAboutEqualPosition);

	void ClipStackPop();
	Bool ClipStackPush();

	void PopDepth3D(Float f, Bool bIgnoreDepthProjection);
	void PushDepth3D(Float f, Bool bIgnoreDepthProjection);
	Pair<Float, Int> ReplaceDepth3D(Float f);
	Pair<Float, Int> ReplaceDepth3D(Float f, Int iIgnoreDepthProjection);

	Float GetRenderThreshold(
		Float fLocalRenderWidth,
		Float fLocalRenderHeight,
		const Matrix2x3& mWorldTransform) const;

	void Pose(
		const Rectangle& worldRectangle,
		Renderable* pRenderable,
		const Matrix2x3& mWorld,
		const ColorTransformWithAlpha& cxWorld,
		const TextureReference& reference,
		const Rectangle& worldOcclusionRectangle,
		Feature::Enum eFeature,
		Int32 iSubRenderableId = -1);

	// Variation of Pose for shapes that have variable 3D depth.
	// Call this function with the highest depth value, to
	// ensure world culling and occlusion rectangles are large
	// enough for accurate culling.
	void PoseWithFarthestDepth(
		Float fDepth3D,
		const Rectangle& worldRectangle,
		Renderable* pRenderable,
		const Matrix2x3& mWorld,
		const ColorTransformWithAlpha& cxWorld,
		const TextureReference& reference,
		const Rectangle& worldOcclusionRectangle,
		Feature::Enum eFeature,
		Int32 iSubRenderableId = -1);

	// Developer only feature, traversal for rendering hit testable areas.
#if SEOUL_ENABLE_CHEATS
	Bool PoseInputVisualization(
		const Rectangle& worldRectangle,
		const Rectangle& inputBounds,
		const Matrix2x3& mWorld,
		RGBA color);
#endif // /#if SEOUL_ENABLE_CHEATS

	PoserResolveResult ResolveTextureReference(
		const Rectangle& worldRectangle,
		Renderable const* pRenderable,
		Float fRenderThreshold,
		const FilePath& filePath,
		TextureReference& rTextureReference,
		Bool bPrefetch = false,
		Bool bUsePacked = true);

	PoserResolveResult ResolveTextureReference(
		const Rectangle& worldRectangle,
		Renderable const* pRenderable,
		Float fRenderThreshold,
		const SharedPtr<BitmapDefinition const>& p,
		TextureReference& rTextureReference,
		Bool bPrefetch = false,
		Bool bUsePacked = true);

	Vector2D InverseDepthProject(Float fWorldX, Float fWorldY) const;

private:
	CheckedPtr<State> m_pState;
	typedef Vector<Rectangle, MemoryBudgets::Falcon> ScissorStack;
	ScissorStack m_vScissorStack;

	// Called on occlusion and world cull rectangles
	// to account for masking/clipping and 3D projection, for
	// accurate visibility and occlusion tests.
	Rectangle ClipAndProjectOcclusion(const Rectangle& rect) const;
	Rectangle ClipAndProjectWorldCull(const Rectangle& rect) const;
	Rectangle DepthProject(const Rectangle& rect) const;

	SEOUL_DISABLE_COPY(Poser);
}; // class Poser

} // namespace Render

/**
 * Utility for computing occlusion rectangles. The input
 * rectangle is expected to be the tight fitting bounds
 * of the occlusion shape (the shape is a quad
 * and the corners line up with texture coordinates
 * of (0, 0) and (1, 1).
 */
Rectangle ComputeOcclusionRectangle(
	const Matrix2x3& m,
	const TextureReference& reference,
	const Rectangle& bounds);
Rectangle ComputeOcclusionRectangle(
	const Matrix2x3& m,
	const TextureReference& reference,
	const Matrix2x3& mOcclusionTransform);

} // namespace Seoul::Falcon

#endif // include guard
