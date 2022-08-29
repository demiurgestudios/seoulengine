/**
 * \file UIFxRenderer.h
 * \brief Specialization of IFxRenderer for binding into the UI system's
 * rendering backend.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UI_FX_RENDERER_H
#define UI_FX_RENDERER_H

#include "FalconRenderable.h"
#include "Fx.h"
namespace Seoul { namespace Falcon { namespace Render { class Poser; } } }

namespace Seoul::UI
{

class FxRenderer SEOUL_SEALED : public IFxRenderer, public Falcon::Renderable
{
public:
	FxRenderer();
	~FxRenderer();

	void BeginPose(Falcon::Render::Poser& rPoser);
	void EndPose();

	// Falcon::Renderable overrides
	virtual void Draw(
		Falcon::Render::Drawer& /*rDrawer*/,
		const Falcon::Rectangle& worldBoundsPreClip,
		const Matrix2x3& /*mWorld*/,
		const Falcon::ColorTransformWithAlpha& /*cxWorld*/,
		const Falcon::TextureReference& /*textureReference*/,
		Int32 /*iSubInstanceId*/) SEOUL_OVERRIDE;

	virtual Bool CastShadow() const SEOUL_OVERRIDE;
	virtual Vector2D GetShadowPlaneWorldPosition() const SEOUL_OVERRIDE;
	// /Falcon::Renderable overrides

	// IFxRenderer overrides
	virtual Camera& GetCamera() const SEOUL_OVERRIDE;
	virtual Buffer& LockFxBuffer() SEOUL_OVERRIDE;
	virtual void UnlockFxBuffer(
		UInt32 uParticles,
		FilePath textureFilePath,
		FxRendererMode eMode,
		Bool bNeedsScreenAlign) SEOUL_OVERRIDE;
	// /IFxRenderer overrides

private:
	CheckedPtr<Falcon::Render::Poser> m_pPoser;
	typedef Vector<FxRendererMode, MemoryBudgets::Fx> Modes;
	Modes m_vModes;
	IFxRenderer::Buffer m_vFxBuffer;
	UInt32 m_uLastPoseFrame;

	SEOUL_DISABLE_COPY(FxRenderer);
}; // class UI::FxRenderer

} // namespace Seoul::UI

#endif // include guard
