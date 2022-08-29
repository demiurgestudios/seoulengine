/**
 * \file GameFxPreview.cpp
 * \brief UI::Movie subclass that implements preview rendering and some logic
 * for visual fx.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Camera.h"
#include "DevUIConfig.h"
#include "FalconMovieClipInstance.h"
#include "FxManager.h"
#include "GameFxPreview.h"
#include "InputManager.h"
#include "ParticleEmitterInstance.h"
#include "ReflectionDefine.h"
#include "RenderCommandStreamBuilder.h"
#include "Renderer.h"
#include "RenderPass.h"
#include "UIContext.h"
#include "UIFxRenderer.h"
#include "UIManager.h"
#include "UIRenderer.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(Game::FxPreview, TypeFlags::kDisableCopy)
	SEOUL_PARENT(UI::Movie)
	SEOUL_PROPERTY_N("InitalPreviewPosition", m_vInitalPreviewPosition)
SEOUL_END_TYPE()

namespace Game
{

static const HString kFxPreviewActive("FxPreviewActive");

static inline Bool ShowGame()
{
#if SEOUL_ENABLE_DEV_UI
	return DevUI::GetDevUIConfig().m_FxPreviewConfig.m_bShowGame;
#else // !SEOUL_ENABLE_DEV_UI
	return true;
#endif // /!SEOUL_ENABLE_DEV_UI
}

FxPreview::FxPreview()
	: m_pRenderer(SEOUL_NEW(MemoryBudgets::Fx) UI::FxRenderer)
	, m_vInitalPreviewPosition(Vector3D::Zero())
{
}

FxPreview::~FxPreview()
{
}

Viewport FxPreview::GetViewport() const
{
	// Use the full viewport if we're not showing the game.
	if (ShowGame())
	{
		return UI::Movie::GetViewport();
	}
	else
	{
		return UI::g_UIContext.GetRootViewport();
	}
}

void FxPreview::OnLoad()
{
	CheckedPtr<FxManager> pFxManager(FxManager::Get());
	if (pFxManager.IsValid())
	{
		pFxManager->SetPreviewFxPosition(m_vInitalPreviewPosition);
	}
}

void FxPreview::OnPose(
	RenderPass& rPass,
	UI::Renderer& rRenderer)
{
	CheckedPtr<FxManager> pFxManager(FxManager::Get());
	if (!pFxManager)
	{
		// Nothing to do.
		return;
	}

	// Cache whether a preview is valid or not.
	Bool const bIsValid = pFxManager->IsPreviewFxValid();

	// Nothing else to do if not valid.
	if (!bIsValid)
	{
		// Nothing to do.
		return;
	}

	pFxManager->SetPreviewFxCamera(rRenderer.GetCameraPtr());
	{
		SharedPtr<Falcon::MovieClipInstance> pRoot;
		if (GetRootMovieClip(pRoot))
		{
			auto const m2x3(pRoot->ComputeWorldTransform());
			Matrix4D const m(
				m2x3.M00, m2x3.M01, 0.0f, m2x3.M02,
				m2x3.M10, m2x3.M11, 0.0f, m2x3.M12,
				0.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f);

			pFxManager->SetPreviewFxTransform(m);
		}
	}

	// Always use the full viewport.
	auto const viewport = GetViewport();
	rRenderer.PushViewport(viewport);

	rRenderer.BeginMovieFxOnly(this, *m_pRenderer);
	pFxManager->RenderPreviewFx(*m_pRenderer);
	rRenderer.EndMovieFxOnly(*m_pRenderer);

	rRenderer.PopViewport();
}

void FxPreview::OnTick(RenderPass& rPass, Float fDeltaTimeInSeconds)
{
	UI::Movie::OnTick(rPass, fDeltaTimeInSeconds);

	CheckedPtr<FxManager> pFxManager(FxManager::Get());

	// Update the preview fx.
	if (pFxManager.IsValid())
	{
		pFxManager->UpdatePreviewFx(fDeltaTimeInSeconds);
	}

	// Cache whether a preview is valid or not.
	Bool const bIsValid = (pFxManager.IsValid() && pFxManager->IsPreviewFxValid());

	// Update input and render flags.
	SetAllowInputToScreensBelow(!bIsValid);
	SetBlocksRenderBelow(bIsValid && !ShowGame());

	// Update condition.
	UI::Manager::Get()->SetCondition(kFxPreviewActive, bIsValid);
}

} // namespace Game

} // namespace Seoul
