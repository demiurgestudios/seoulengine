/**
 * \file FalconScalingGrid.h
 * \brief Utility, handles decomposition of a mesh into 9 pieces
 * for 9-slice scaling and render.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_SCALING_GRID_H
#define FALCON_SCALING_GRID_H

#include "FalconRenderFeature.h"
#include "FalconTriangleListDescription.h"
#include "FalconTypes.h"
#include "Prereqs.h"
#include "UnsafeBuffer.h"
namespace Seoul { namespace Falcon { namespace Clipper { template <typename VERTEX> class MeshClipCacheT; } } }
namespace Seoul { namespace Falcon { namespace Render { class Drawer; } } }
namespace Seoul { namespace Falcon { struct TextureReference; } }

namespace Seoul::Falcon
{

class ScalingGrid SEOUL_SEALED
{
public:
	typedef UnsafeBuffer<UInt16, MemoryBudgets::Falcon> Indices;
	typedef UnsafeBuffer<ShapeVertex, MemoryBudgets::Falcon> Vertices;

	ScalingGrid(Render::Drawer& r);
	~ScalingGrid();

	void DrawTriangleList(
		const Rectangle& scalingGrid,
		const Rectangle& worldBoundsPreClip,
		const TextureReference& textureReference,
		const Matrix2x3& mParentTransform,
		const Matrix2x3& mChildTransform,
		const ColorTransformWithAlpha& cxWorld,
		const Rectangle& localBounds,
		UInt16 const* pIndices,
		UInt32 uIndexCount,
		ShapeVertex const* pVertices,
		UInt32 uVertexCount,
		TriangleListDescription eDescription,
		Render::Feature::Enum eFeature);

private:
	Render::Drawer& m_r;

	Falcon::Clipper::MeshClipCacheT<StandardVertex2D>* m_pMeshClipCache;
	Indices m_vClipI;
	Vertices m_vClipV;
	Vertices m_vWorkArea;

	SEOUL_DISABLE_COPY(ScalingGrid);
}; // class ScalingGrid

} // namespace Seoul::Falcon

#endif // include guard
