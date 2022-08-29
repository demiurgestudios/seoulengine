/**
 * \file UIStage3D.h
 * \brief Similar to UI::TextureSubstitution, except that perspective
 * effects can be applied, to create the illusion of depth. Intended
 * for background plates and similar images.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UI_STAGE3D_H
#define UI_STAGE3D_H

#include "FalconInstance.h"
#include "FilePath.h"
#include "Prereqs.h"
#include "SeoulHString.h"
namespace Seoul { namespace Falcon { class BitmapDefinition; } }

namespace Seoul::UI
{

/** Custom subclass of Falcon::Instance, implements texture perspective logic. */
class Stage3D SEOUL_SEALED : public Falcon::Instance
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(Stage3D);

	Stage3D(
		FilePath filePath,
		Int32 iTextureWidth,
		Int32 iTextureHeight);

	~Stage3D();

	virtual Instance* Clone(Falcon::AddInterface& rInterface) const SEOUL_OVERRIDE
	{
		Stage3D* pReturn = SEOUL_NEW(MemoryBudgets::UIRuntime) Stage3D;
		CloneTo(rInterface, *pReturn);
		return pReturn;
	}

	virtual Bool ComputeLocalBounds(Falcon::Rectangle& rBounds) SEOUL_OVERRIDE;

	virtual void Pose(
		Falcon::Render::Poser& rPoser,
		const Matrix2x3& mParent,
		const Falcon::ColorTransformWithAlpha& cxParent) SEOUL_OVERRIDE;

#if SEOUL_ENABLE_CHEATS
	virtual void PoseInputVisualization(
		Falcon::Render::Poser& rPoser,
		const Matrix2x3& mParent,
		RGBA color) SEOUL_OVERRIDE;
#endif

	virtual void Draw(
		Falcon::Render::Drawer& rDrawer,
		const Falcon::Rectangle& worldBoundsPreClip,
		const Matrix2x3& mWorld,
		const Falcon::ColorTransformWithAlpha& cxWorld,
		const Falcon::TextureReference& textureReference,
		Int32 iSubInstanceId) SEOUL_OVERRIDE;

	virtual Bool HitTest(
		const Matrix2x3& mParent,
		Float32 fWorldX,
		Float32 fWorldY,
		Bool bIgnoreVisibility = false) const SEOUL_OVERRIDE;

	virtual Falcon::InstanceType GetType() const SEOUL_OVERRIDE;

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(Stage3D);

	Stage3D();

	void CloneTo(Falcon::AddInterface& rInterface, Stage3D& rClone) const
	{
		Falcon::Instance::CloneTo(rInterface, rClone);
		rClone.m_vTextureCoordinates = m_vTextureCoordinates;
		rClone.m_FilePath = m_FilePath;
		rClone.m_iTextureWidth = m_iTextureWidth;
		rClone.m_iTextureHeight = m_iTextureHeight;
	}

	void PrepareDebugGrid();

	SharedPtr<Falcon::BitmapDefinition> m_pGrid;
	Vector4D m_vTextureCoordinates;
	FilePath m_FilePath;
	Int32 m_iTextureWidth;
	Int32 m_iTextureHeight;
	Float m_fTextureAlpha;

	SEOUL_DISABLE_COPY(Stage3D);
}; // class UI::Stage3D

} // namespace Seoul::UI

#endif // include guard
