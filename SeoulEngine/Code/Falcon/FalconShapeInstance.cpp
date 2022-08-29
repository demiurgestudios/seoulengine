/**
 * \file FalconShapeInstance.cpp
 * \brief A Falcon::ShapeInstance is directly analagous to
 * a Flash Shape.
 *
 * Shapes are the typical minimum unit of renderable mesh data
 * exported from Flash (occasionally, a BitmapInstance can
 * be exported for Bitmap data, but usually, these are exported
 * as a quad ShapeInstance instead).
 *
 * Shapes define vector shape data as polygons, which Falcon
 * will triangulate for GPU render. Data can either be solid fill
 * vector shapes or Bitmap quads (note: although Falcon has no
 * limitations that prevent vectorized/non-quad Bitmaps, Flash
 * never exports such data).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FalconBitmapDefinition.h"
#include "FalconClipper.h"
#include "FalconDefinition.h"
#include "FalconFCNFile.h"
#include "FalconMovieClipDefinition.h"
#include "FalconMovieClipInstance.h"
#include "FalconRenderDrawer.h"
#include "FalconRenderPoser.h"
#include "FalconScalingGrid.h"
#include "FalconShapeDefinition.h"
#include "FalconShapeInstance.h"
#include "ReflectionDefine.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(Falcon::ShapeInstance, TypeFlags::kDisableNew)
	SEOUL_PARENT(Falcon::Instance)
SEOUL_END_TYPE()

namespace Falcon
{

static inline Bool InsideTriangle(
	const Vector2D& vA,
	const Vector2D& vB,
	const Vector2D& vC,
	const Vector2D& vP)
{
	Vector2D const vBA(vB - vA);
	Vector2D const vPA(vP - vA);
	Float const fCrossAP = Vector2D::Cross(vBA, vPA);
	if (fCrossAP < 0.0f)
	{
		return false;
	}

	Vector2D const vCB(vC - vB);
	Vector2D const vPB(vP - vB);
	Float const fCrossBP = Vector2D::Cross(vCB, vPB);
	if (fCrossBP < 0.0f)
	{
		return false;
	}

	Vector2D const vAC(vA - vC);
	Vector2D const vPC(vP - vC);
	Float const fCrossCP = Vector2D::Cross(vAC, vPC);
	if (fCrossCP < 0.0f)
	{
		return false;
	}

	return true;
}

ShapeInstance::ShapeInstance(const SharedPtr<ShapeDefinition const>& pShape)
	: Instance(pShape->GetDefinitionID())
	, m_pShape(pShape)
{
}

ShapeInstance::~ShapeInstance()
{
}

Bool ShapeInstance::ComputeLocalBounds(Rectangle& rBounds)
{
	rBounds = m_pShape->GetRectangle();
	return true;
}

void ShapeInstance::ComputeMask(
	const Matrix2x3& mParent,
	const ColorTransformWithAlpha& cxParent,
	Render::Poser& rPoser)
{
	// TODO: Reconsider - we don't consider the alpha
	// to match Flash behavior. I've never double checked what
	// happens if you (just) set the visibility of a mask to
	// false and logically it makes sense for visibility and alpha==0.0
	// to have the same behavior (or, in other words, visibility should
	// possibly not be considered here).
	if (!GetVisible())
	{
		return;
	}
	
	// Unlike many code paths, alpha == 0.0 is not considered here. Flash
	// does not hide the mask (or the shapes it reveals) if the cumulative
	// alpha at that mask is 0.0.

	// 3 cases for a shape:
	// - simple (when m_bMatchesBounds is true)
	// - single convex
	// - arbitrary - in this case, must submit each triangle
	//   as a separate convex clipping hull.
	FixedArray<ShapeVertex, 3> aVertices;
	Matrix2x3 const mWorldTransform = (mParent * GetTransform());
	for (auto i = m_pShape->GetFillDrawables().Begin(); m_pShape->GetFillDrawables().End() != i; ++i)
	{
		// Simple shape, just use bounds.
		if (i->m_bMatchesBounds)
		{
			rPoser.ClipStackAddRectangle(mWorldTransform, i->m_Bounds);
		}
		// Convex shape or quad list.
		else if (i->m_eTriangleListDescription != TriangleListDescription::kNotSpecific)
		{
			// Single entry.
			if (TriangleListDescription::kConvex == i->m_eTriangleListDescription ||
				(TriangleListDescription::kQuadList == i->m_eTriangleListDescription && i->m_vVertices.GetSize() == 4u) ||
				(TriangleListDescription::kTextChunk == i->m_eTriangleListDescription && i->m_vVertices.GetSize() == 4u))
			{
				rPoser.ClipStackAddConvexHull(mWorldTransform, i->m_vVertices.Data(), i->m_vVertices.GetSize());
			}
			// Multiple entries, each a quad.
			else
			{
				// Sanity check.
				SEOUL_ASSERT(i->m_vVertices.GetSize() % 4u == 0u);
				for (UInt32 uV = 0u; uV < i->m_vVertices.GetSize(); uV += 4u)
				{
					rPoser.ClipStackAddConvexHull(mWorldTransform, i->m_vVertices.Get(uV), 4u);
				}
			}
		}
		// TODO: If we computed a convex decomposition of the mesh data ahead of time,
		// doing this per-triangle would be avoided.
		else
		{
			// Add each triangle as a separate convex hull.
			UInt32 const uIndices = i->m_vIndices.GetSize();
			for (UInt32 uIndex = 0u; uIndex < uIndices; uIndex += 3)
			{
				aVertices[0] = i->m_vVertices[i->m_vIndices[uIndex+0]];
				aVertices[1] = i->m_vVertices[i->m_vIndices[uIndex+1]];
				aVertices[2] = i->m_vVertices[i->m_vIndices[uIndex+2]];
				rPoser.ClipStackAddConvexHull(mWorldTransform, aVertices.Data(), 3);
			}
		}
	}
}

void ShapeInstance::Pose(
	Render::Poser& rPoser,
	const Matrix2x3& mParent,
	const ColorTransformWithAlpha& cxParent)
{
	if (!GetVisible())
	{
		return;
	}

	ColorTransformWithAlpha const cxWorld = (cxParent * GetColorTransformWithAlpha());
	if (cxWorld.m_fMulA == 0.0f)
	{
		return;
	}

	Matrix2x3 const mWorld = (mParent * GetTransform());

	Bool const bScalingGrid = (nullptr != GetParent() && GetParent()->GetMovieClipDefinition()->HasScalingGrid());
	const ShapeDefinition::FillDrawables& vDrawables = m_pShape->GetFillDrawables();
	ShapeDefinition::FillDrawables::SizeType const zSize = vDrawables.GetSize();
	for (ShapeDefinition::FillDrawables::SizeType i = 0; i < zSize; ++i)
	{
		const ShapeFillDrawable& drawable = vDrawables[i];
		if (drawable.m_vIndices.IsEmpty() || drawable.m_vVertices.IsEmpty())
		{
			continue;
		}

		auto const worldBounds(TransformRectangle(mWorld, drawable.m_Bounds));

		TextureReference reference;
		if (Render::PoserResolveResult::kSuccess != rPoser.ResolveTextureReference(
			worldBounds,
			this,
			rPoser.GetRenderThreshold((Float)drawable.m_Bounds.GetWidth(), (Float)drawable.m_Bounds.GetHeight(), mWorld),
			drawable.m_pBitmapDefinition,
			reference))
		{
			continue;
		}

		auto const worldOcclusion(drawable.m_bCanOcclude
			? ComputeOcclusionRectangle(mWorld, reference, drawable.m_mOcclusionTransform)
			: Rectangle());

		// Special handling if we're 9-slicing. We need to submit the parent transform, not the world
		// transform, since 9-slicing occurs in parent space.
		rPoser.Pose(
			worldBounds,
			this,
			(bScalingGrid ? mParent : mWorld),
			cxWorld,
			reference,
			worldOcclusion,
			drawable.m_eFeature,
			(Int32)i);
	}
}

#if SEOUL_ENABLE_CHEATS
void ShapeInstance::PoseInputVisualization(
	Render::Poser& rPoser,
	const Matrix2x3& mParent,
	RGBA color)
{
	auto const mWorld(mParent * GetTransform());
	for (auto const& e : m_pShape->GetFillDrawables())
	{
		// TODO: Draw the appropriate shape for exact hit testing.
		auto const& bounds = e.m_Bounds;
		auto const worldBounds = TransformRectangle(mWorld, bounds);
		rPoser.PoseInputVisualization(
			worldBounds,
			bounds,
			mWorld,
			color);
	}
}
#endif

void ShapeInstance::Draw(
	Render::Drawer& rDrawer,
	const Rectangle& worldBoundsPreClip,
	const Matrix2x3& mParentOrWorld,
	const ColorTransformWithAlpha& cxWorld,
	const TextureReference& textureReference,
	Int32 iSubInstanceId)
{
	const ShapeDefinition::FillDrawables& vDrawables = m_pShape->GetFillDrawables();
	const ShapeFillDrawable& drawable = vDrawables[(ShapeDefinition::FillDrawables::SizeType)iSubInstanceId];

	Bool const bScalingGrid = (nullptr != GetParent() && GetParent()->GetMovieClipDefinition()->HasScalingGrid());

	// If applying a scaling grid, mWorld will actually be mParent.
	if (bScalingGrid)
	{
		auto const& scalingGrid = GetParent()->GetMovieClipDefinition()->GetScalingGrid();
		rDrawer.GetScalingGrid().DrawTriangleList(
			scalingGrid,
			worldBoundsPreClip,
			textureReference,
			mParentOrWorld,
			GetTransform(),
			cxWorld,
			drawable.m_Bounds,
			drawable.m_vIndices.Data(),
			drawable.m_vIndices.GetSize(),
			drawable.m_vVertices.Data(),
			drawable.m_vVertices.GetSize(),
			drawable.m_eTriangleListDescription,
			drawable.m_eFeature);
	}
	else
	{
		rDrawer.DrawTriangleList(
			worldBoundsPreClip,
			textureReference,
			mParentOrWorld,
			cxWorld,
			drawable.m_vIndices.Get(0),
			(UInt32)drawable.m_vIndices.GetSize(),
			drawable.m_vVertices.Get(0),
			(UInt32)drawable.m_vVertices.GetSize(),
			drawable.m_eTriangleListDescription,
			drawable.m_eFeature);
	}
}

Bool ShapeInstance::ExactHitTest(
	const Matrix2x3& mParent,
	Float32 fWorldX,
	Float32 fWorldY,
	Bool bIgnoreVisibility /*= false*/) const
{
	if (!bIgnoreVisibility)
	{
		if (!GetVisible())
		{
			return false;
		}
	}

	Matrix2x3 const mWorld = (mParent * GetTransform());
	Matrix2x3 const mInverseWorld = mWorld.Inverse();

	Vector2D const vObjectSpace = Matrix2x3::TransformPosition(mInverseWorld, Vector2D(fWorldX, fWorldY));
	Float32 const fObjectSpaceX = vObjectSpace.X;
	Float32 const fObjectSpaceY = vObjectSpace.Y;

	const ShapeDefinition::FillDrawables& vDrawables = m_pShape->GetFillDrawables();
	ShapeDefinition::FillDrawables::SizeType const zSize = vDrawables.GetSize();

	for (ShapeDefinition::FillDrawables::SizeType i = 0; i < zSize; ++i)
	{
		const ShapeFillDrawable& drawable = vDrawables[i];
		if (drawable.m_vIndices.IsEmpty() || drawable.m_vVertices.IsEmpty())
		{
			continue;
		}

		if (drawable.m_bMatchesBounds)
		{
			Rectangle const bounds = drawable.m_Bounds;
			if (fObjectSpaceX < bounds.m_fLeft) { continue; }
			if (fObjectSpaceY < bounds.m_fTop) { continue; }
			if (fObjectSpaceX > bounds.m_fRight) { continue; }
			if (fObjectSpaceY > bounds.m_fBottom) { continue; }

			return true;
		}
		else
		{
			ShapeFillDrawable::Indices::SizeType const zIndices = drawable.m_vIndices.GetSize();
			for (ShapeFillDrawable::Indices::SizeType j = 0; j < zIndices; j += 3)
			{
				if (InsideTriangle(
					drawable.m_vVertices[drawable.m_vIndices[j+0]].m_vP,
					drawable.m_vVertices[drawable.m_vIndices[j+1]].m_vP,
					drawable.m_vVertices[drawable.m_vIndices[j+2]].m_vP,
					vObjectSpace))
				{
					return true;
				}
			}
		}
	}

	return false;
}

Bool ShapeInstance::HitTest(
	const Matrix2x3& mParent,
	Float32 fWorldX,
	Float32 fWorldY,
	Bool bIgnoreVisibility /*= false*/) const
{
	if (!bIgnoreVisibility)
	{
		if (!GetVisible())
		{
			return false;
		}
	}

	Matrix2x3 const mWorld = (mParent * GetTransform());
	Matrix2x3 const mInverseWorld = mWorld.Inverse();

	Vector2D const vObjectSpace = Matrix2x3::TransformPosition(mInverseWorld, Vector2D(fWorldX, fWorldY));
	Float32 const fObjectSpaceX = vObjectSpace.X;
	Float32 const fObjectSpaceY = vObjectSpace.Y;

	const ShapeDefinition::FillDrawables& vDrawables = m_pShape->GetFillDrawables();
	ShapeDefinition::FillDrawables::SizeType const zSize = vDrawables.GetSize();

	for (ShapeDefinition::FillDrawables::SizeType i = 0; i < zSize; ++i)
	{
		const ShapeFillDrawable& drawable = vDrawables[i];
		if (drawable.m_vIndices.IsEmpty() || drawable.m_vVertices.IsEmpty())
		{
			continue;
		}

		Rectangle const bounds = drawable.m_Bounds;
		if (fObjectSpaceX < bounds.m_fLeft) { continue; }
		if (fObjectSpaceY < bounds.m_fTop) { continue; }
		if (fObjectSpaceX > bounds.m_fRight) { continue; }
		if (fObjectSpaceY > bounds.m_fBottom) { continue; }

		return true;
	}

	return false;
}

} // namespace Falcon

} // namespace Seoul
