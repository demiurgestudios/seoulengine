/**
 * \file FalconBitmapInstance.h
 * \brief A direct instantiation of
 * a BitmapDefinition.
 *
 * Typically, the basic Falcon draw component
 * is a ShapeInstance, but occasionally Flash
 * can directly export bitmap definitions as
 * as BitmapInstance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_BITMAP_INSTANCE_H
#define FALCON_BITMAP_INSTANCE_H

#include "FalconInstance.h"
#include "ReflectionDeclare.h"

namespace Seoul::Falcon
{

// forward declarations
class BitmapDefinition;

class BitmapInstance SEOUL_SEALED : public Instance
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(BitmapInstance);

	BitmapInstance();
	BitmapInstance(const SharedPtr<BitmapDefinition const>& pBitmap);
	~BitmapInstance();

	virtual Instance* Clone(AddInterface& rInterface) const SEOUL_OVERRIDE
	{
		BitmapInstance* pReturn = SEOUL_NEW(MemoryBudgets::Falcon) BitmapInstance(m_pBitmap);
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

	virtual InstanceType GetType() const SEOUL_OVERRIDE
	{
		return InstanceType::kBitmap;
	}

	virtual Bool HitTest(
		const Matrix2x3& mParent,
		Float32 fWorldX,
		Float32 fWorldY,
		Bool bIgnoreVisibility = false) const SEOUL_OVERRIDE;

	const SharedPtr<BitmapDefinition const>& GetBitmapDefinition() const
	{
		return m_pBitmap;
	}

	void SetBitmapDefinition(const SharedPtr<BitmapDefinition const>& pBitmap);

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(BitmapInstance);

	SharedPtr<BitmapDefinition const> m_pBitmap;

	SEOUL_DISABLE_COPY(BitmapInstance);
}; // class BitmapInstance
template <> struct InstanceTypeOf<BitmapInstance> { static const InstanceType Value = InstanceType::kBitmap; };

} // namespace Seoul::Falcon

#endif // include guard
