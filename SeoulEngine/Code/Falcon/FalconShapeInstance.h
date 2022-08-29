/**
 * \file FalconShapeInstance.h
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

#pragma once
#ifndef FALCON_SHAPE_INSTANCE_H
#define FALCON_SHAPE_INSTANCE_H

#include "FalconInstance.h"
#include "ReflectionDeclare.h"

namespace Seoul::Falcon
{

// forward declarations
class ShapeDefinition;

class ShapeInstance SEOUL_SEALED : public Instance
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(ShapeInstance);

	ShapeInstance(const SharedPtr<ShapeDefinition const>& pShape);
	~ShapeInstance();

	virtual Instance* Clone(AddInterface& rInterface) const SEOUL_OVERRIDE
	{
		ShapeInstance* pReturn = SEOUL_NEW(MemoryBudgets::Falcon) ShapeInstance(m_pShape);
		CloneTo(rInterface, *pReturn);
		return pReturn;
	}

	virtual Bool ComputeLocalBounds(Rectangle& rBounds) SEOUL_OVERRIDE;

	virtual void ComputeMask(
		const Matrix2x3& mParent,
		const ColorTransformWithAlpha& cxParent,
		Render::Poser& rPoser) SEOUL_OVERRIDE;

	virtual void Pose(
		Render::Poser& rPoser,
		const Matrix2x3& mParent,
		const ColorTransformWithAlpha& cxParent) SEOUL_OVERRIDE;

	// Developer only feature, traversal for rendering hit testable areas.
#if SEOUL_ENABLE_CHEATS
	virtual void PoseInputVisualization(
		Render::Poser& rPoser,
		const Matrix2x3& mParent,
		RGBA color) SEOUL_OVERRIDE;
#endif

	virtual void Draw(
		Render::Drawer& rDrawer,
		const Rectangle& worldBoundsPreClip,
		const Matrix2x3& mParentOrWorld,
		const ColorTransformWithAlpha& cxWorld,
		const TextureReference& textureReference,
		Int32 iSubInstanceId) SEOUL_OVERRIDE;

	virtual Bool ExactHitTest(
		const Matrix2x3& mParent,
		Float32 fWorldX,
		Float32 fWorldY,
		Bool bIgnoreVisibility /*= false*/) const SEOUL_OVERRIDE;

	virtual InstanceType GetType() const SEOUL_OVERRIDE
	{
		return InstanceType::kShape;
	}

	virtual Bool HitTest(
		const Matrix2x3& mParent,
		Float32 fWorldX,
		Float32 fWorldY,
		Bool bIgnoreVisibility = false) const SEOUL_OVERRIDE;

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(ShapeInstance);

	SharedPtr<ShapeDefinition const> m_pShape;

	SEOUL_DISABLE_COPY(ShapeInstance);
}; // class ShapeInstance
template <> struct InstanceTypeOf<ShapeInstance> { static const InstanceType Value = InstanceType::kShape; };

} // namespace Seoul::Falcon

#endif // include guard
