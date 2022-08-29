/**
 * \file SceneRenderer.cpp
 * \brief Root utility that handles scene rendering. Contains
 * both an FxRenderer and a MeshRenderer.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Camera.h"
#include "ClearFlags.h"
#include "EffectManager.h"
#include "FilePath.h"
#include "RenderCommandStreamBuilder.h"
#include "RenderPass.h"
#include "SceneFxComponent.h"
#include "SceneFxRenderer.h"
#include "SceneMeshDrawComponent.h"
#include "SceneMeshRenderer.h"
#include "SceneObject.h"
#include "ScenePrefabComponent.h"
#include "SceneRenderer.h"
#include "SeoulHString.h"

#if SEOUL_WITH_SCENE

namespace Seoul::Scene
{

#if SEOUL_EDITOR_AND_TOOLS
static const HString kParameterHighlightColor("seoul_HighlightColor");
#endif // /#if SEOUL_EDITOR_AND_TOOLS
static const HString kParameterPickColor("seoul_PickColor");
static const HString kTechniquePick("seoul_Pick");
static const HString kTechniquePickSkinned("seoul_PickSkinned");
static const UInt32 kuInitialPickValue = 1u; // Allow a clear color of 0 to be placeholder and not resolve to anything.

Renderer::Renderer(const RendererConfig& config)
	: m_Config()
	, m_hFxEffect()
	, m_hMeshEffect()
	, m_pFxRenderer(SEOUL_NEW(MemoryBudgets::Scene) FxRenderer)
	, m_pMeshRenderer(SEOUL_NEW(MemoryBudgets::Scene) MeshRenderer)
{
	Configure(config);
}

Renderer::~Renderer()
{
}

void Renderer::Configure(const RendererConfig& config)
{
	m_Config = config;
	m_hFxEffect = EffectManager::Get()->GetEffect(config.m_FxEffectFilePath);
	m_hMeshEffect = EffectManager::Get()->GetEffect(config.m_MeshEffectFilePath);
}

void Renderer::Pick(
	const SharedPtr<Camera>& pCamera,
	const Objects& vObjects,
	RenderPass& rPass,
	RenderCommandStreamBuilder& rBuilder,
	PickTable& rtPickTable)
{
	UInt32 uPickValue = kuInitialPickValue;
	InternalPickMeshes(
		pCamera,
		vObjects,
		rPass,
		rBuilder,
		uPickValue,
		rtPickTable);
}

void Renderer::Render(
	const Cameras& vCameras,
	const Objects& vObjects,
	RenderPass& rPass,
	RenderCommandStreamBuilder& rBuilder,
	HighlightedObjects const* pHighlighted /*= nullptr*/)
{
	// Cache viewports for rendering.
	Viewport const originalViewport = rBuilder.GetCurrentViewport();
	Viewport currentViewport = originalViewport;

	// Render cameras.
	UInt32 uCamerasRendered = 0u;
	UInt32 const uCameras = vCameras.GetSize();
	for (UInt32 i = 0u; i < uCameras; ++i)
	{
		SharedPtr<Camera> pCamera(vCameras[i]);

		// Continue if not enabled.
		if (!pCamera->GetEnabled())
		{
			continue;
		}

		// Setup the viewport.
		currentViewport = pCamera->ApplyRelativeViewport(originalViewport);

		// If it changed, apply.
		if (currentViewport != originalViewport)
		{
			// Apply the new viewport to the viewport and scissor.
			rBuilder.SetCurrentViewport(currentViewport);
			rBuilder.SetScissor(true, currentViewport);
		}

		// If this is not the first Camera, clear the render target.
		if (0u != uCamerasRendered)
		{
			const RenderPass::Settings& settings = rPass.GetSettings();
			if (0u != settings.m_uFlags)
			{
				rBuilder.Clear(
					settings.m_uFlags,
					settings.m_ClearColor,
					settings.m_fClearDepth,
					settings.m_uClearStencil);
			}
		}

		// Render the scene from this Camera's perspective.
		InternalRenderMeshes(pCamera, vObjects, rBuilder, pHighlighted);
		InternalRenderFx(pCamera, vObjects, rBuilder);

		// Count this as a render call.
		uCamerasRendered++;
	}

	// If the viewport was changed, restore it before returning.
	if (currentViewport != originalViewport)
	{
		// Apply the original viewport to the viewport and scissor.
		rBuilder.SetCurrentViewport(originalViewport);
		rBuilder.SetScissor(true, originalViewport);
	}
}

Bool Renderer::InternalPickMeshes(
	const SharedPtr<Camera>& pCamera,
	const Objects& vObjects,
	RenderPass& rPass,
	RenderCommandStreamBuilder& rBuilder,
	UInt32& ruPickValue,
	PickTable& rtPickTable)
{
	SharedPtr<Effect> pMeshEffect(m_hMeshEffect.GetPtr());
	if (!pMeshEffect.IsValid() || BaseGraphicsObject::kDestroyed == pMeshEffect->GetState())
	{
		return false;
	}

	Frustum const origFrustum = pCamera->GetFrustum();
	Frustum frustum = origFrustum;
	m_pMeshRenderer->BeginFrame(pCamera, rBuilder);
	m_pMeshRenderer->UseEffect(pMeshEffect);

	// TODO: Maintain a MeshDraw list in Renderer separately from
	// the main object list to minimize this set.

	Bool bReturn = false;

	UInt32 const uObjects = vObjects.GetSize();
	for (UInt32 i = 0u; i < uObjects; ++i)
	{
		const Object& object = *vObjects[i];

#if SEOUL_EDITOR_AND_TOOLS
		// Editor only - skip the object if not visible in the editor.
		if (!object.GetVisibleInEditor()) { continue; }
#endif // /#if SEOUL_EDITOR_AND_TOOLS

		// MeshDrawComponent
		{
			SharedPtr<MeshDrawComponent> pMeshDrawComponent(object.GetComponent<MeshDrawComponent>());
			if (pMeshDrawComponent.IsValid())
			{
				ColorARGBu8 cColor = ColorARGBu8::White();
				cColor.m_R = (UInt8)((ruPickValue >> 0u) & 0xFF);
				cColor.m_G = (UInt8)((ruPickValue >> 8u) & 0xFF);
				cColor.m_B = (UInt8)((ruPickValue >> 16u) & 0xFF);
				Color4 const c(cColor);
				rBuilder.SetVector4DParameter(
					pMeshEffect,
					kParameterPickColor,
					Vector4D(c.R, c.G, c.B, c.A));

				if (pMeshDrawComponent->Render(frustum, *m_pMeshRenderer, kTechniquePick, kTechniquePickSkinned))
				{
					SEOUL_VERIFY(rtPickTable.Insert(cColor, SharedPtr<Object>(pMeshDrawComponent->GetOwner())).Second);
					++ruPickValue;
					bReturn = true;
				}
			}
		}

		// TODO: This will never appear at runtime, so it is needless overhead.

		// PrefabComponent
		{
			SharedPtr<PrefabComponent> pComponent(object.GetComponent<PrefabComponent>());
			if (pComponent.IsValid())
			{
				auto const& vObjects = pComponent->GetObjects();
				if (!vObjects.IsEmpty())
				{
					auto const mTransform = object.ComputeNormalTransform();
					Bool const bHasParentTransform = !Matrix4D::Identity().Equals(mTransform);

					// Push the nested transform as a new view transform.
					if (bHasParentTransform)
					{
						auto const mWorld = m_pMeshRenderer->PushWorldMatrix(mTransform);
						frustum.Set(pCamera->GetProjectionMatrix(), pCamera->GetViewMatrix() * mWorld);
					}

					// Set pick color for the entire nest.
					ColorARGBu8 cColor = ColorARGBu8::White();
					cColor.m_R = (UInt8)((ruPickValue >> 0u) & 0xFF);
					cColor.m_G = (UInt8)((ruPickValue >> 8u) & 0xFF);
					cColor.m_B = (UInt8)((ruPickValue >> 16u) & 0xFF);
					Color4 const c(cColor);
					rBuilder.SetVector4DParameter(
						pMeshEffect,
						kParameterPickColor,
						Vector4D(c.R, c.G, c.B, c.A));

					// Pick recursively.
					Bool const bPick = InternalInnerRenderMeshes(
						pCamera,
						frustum,
						vObjects,
						rBuilder,
						kTechniquePick,
						kTechniquePickSkinned);

					// Carry through results.
					if (bPick)
					{
						SEOUL_VERIFY(rtPickTable.Insert(cColor, SharedPtr<Object>(pComponent->GetOwner())).Second);
						++ruPickValue;
						bReturn = true;
					}

					// Pop the nested transform.
					if (bHasParentTransform)
					{
						frustum = origFrustum;
						m_pMeshRenderer->PopWorldMatrix();
					}
				}
			}
		}
	}

	m_pMeshRenderer->EndFrame();
	return bReturn;
}

void Renderer::InternalRenderFx(
	const SharedPtr<Camera>& pCamera,
	const Objects& vObjects,
	RenderCommandStreamBuilder& rBuilder)
{
	SharedPtr<Effect> pFxEffect(m_hFxEffect.GetPtr());
	if (!pFxEffect.IsValid() || BaseGraphicsObject::kDestroyed == pFxEffect->GetState())
	{
		return;
	}

	Frustum const frustum = pCamera->GetFrustum();
	m_pFxRenderer->BeginFrame(pCamera, rBuilder);
	m_pFxRenderer->UseEffect(pFxEffect);

	(void)InternalInnerRenderFx(
		pCamera,
		frustum,
		vObjects,
		rBuilder);

	m_pFxRenderer->EndFrame();
}

void Renderer::InternalInnerRenderFx(
	const SharedPtr<Camera>& pCamera,
	const Frustum& origFrustum,
	const Objects& vObjects,
	RenderCommandStreamBuilder& rBuilder)
{
	// TODO: Maintain a Fx list in Renderer separately from
	// the main object list to minimize this set.

	Frustum frustum = origFrustum;
	UInt32 const uObjects = vObjects.GetSize();
	for (UInt32 i = 0u; i < uObjects; ++i)
	{
		const Object& object = *vObjects[i];

#if SEOUL_EDITOR_AND_TOOLS
		// Editor only - skip the object if not visible in the editor.
		if (!object.GetVisibleInEditor()) { continue; }
#endif // /#if SEOUL_EDITOR_AND_TOOLS

		// FxDrawComponent
		{
			SharedPtr<FxComponent> pFxDrawComponent(object.GetComponent<FxComponent>());
			if (pFxDrawComponent.IsValid())
			{
				pFxDrawComponent->Render(frustum, *m_pFxRenderer);
			}
		}

		// TODO: This will never appear at runtime, so it is needless overhead.

		// PrefabComponent
		{
			SharedPtr<PrefabComponent> pComponent(object.GetComponent<PrefabComponent>());
			if (pComponent.IsValid())
			{
				auto const& vObjects = pComponent->GetObjects();
				if (!vObjects.IsEmpty())
				{
					auto const mTransform = object.ComputeNormalTransform();
					Bool const bHasParentTransform = !Matrix4D::Identity().Equals(mTransform);

					// Push the nested transform as a new view transform.
					if (bHasParentTransform)
					{
						auto const mWorld = m_pFxRenderer->PushWorldMatrix(mTransform);
						frustum.Set(pCamera->GetProjectionMatrix(), pCamera->GetViewMatrix() * mWorld);
					}

					// Draw recursively.
					InternalInnerRenderFx(
						pCamera,
						frustum,
						vObjects,
						rBuilder);

					// Pop the nested transform.
					if (bHasParentTransform)
					{
						frustum = origFrustum;
						m_pFxRenderer->PopWorldMatrix();
					}
				}
			}
		}
	}
}

void Renderer::InternalRenderMeshes(
	const SharedPtr<Camera>& pCamera,
	const Objects& vObjects,
	RenderCommandStreamBuilder& rBuilder,
	HighlightedObjects const* pHighlighted /* = nullptr */)
{
	SharedPtr<Effect> pMeshEffect(m_hMeshEffect.GetPtr());
	if (!pMeshEffect.IsValid() || BaseGraphicsObject::kDestroyed == pMeshEffect->GetState())
	{
		return;
	}

	Frustum const frustum = pCamera->GetFrustum();
	m_pMeshRenderer->BeginFrame(pCamera, rBuilder);
	m_pMeshRenderer->UseEffect(pMeshEffect);

	(void)InternalInnerRenderMeshes(
		pCamera,
		frustum,
		vObjects,
		rBuilder,
		HString(),
		HString(),
		pHighlighted);

	m_pMeshRenderer->EndFrame();
}

Bool Renderer::InternalInnerRenderMeshes(
	const SharedPtr<Camera>& pCamera,
	const Frustum& origFrustum,
	const Objects& vObjects,
	RenderCommandStreamBuilder& rBuilder,
	HString techniqueOverride,
	HString skinnedTechniqueOverride,
	HighlightedObjects const* pHighlighted /* = nullptr */)
{
	// TODO: Maintain a MeshDraw list in Renderer separately from
	// the main object list to minimize this set.

	Bool bReturn = false;

	// Editor only, highlight color.
#if SEOUL_EDITOR_AND_TOOLS
	static const ColorARGBu8 kcHighlight = ColorARGBu8::Create(240, 81, 51, 127);
	ColorARGBu8 cHighlightColor = ColorARGBu8::TransparentBlack();
#endif // /#if SEOUL_EDITOR_AND_TOOLS

	Frustum frustum = origFrustum;
	UInt32 const uObjects = vObjects.GetSize();
	for (UInt32 i = 0u; i < uObjects; ++i)
	{
		const Object& object = *vObjects[i];

#if SEOUL_EDITOR_AND_TOOLS
		// Editor only - skip the object if not visible in the editor.
		if (!object.GetVisibleInEditor()) { continue; }

		// Editor only, highlight color.
		{
			ColorARGBu8 c = ColorARGBu8::TransparentBlack();
			if (nullptr != pHighlighted && pHighlighted->HasKey(vObjects[i]))
			{
				c = kcHighlight;
			}

			if (c != cHighlightColor)
			{
				cHighlightColor = c;
				rBuilder.SetVector4DParameter(
					m_pMeshRenderer->GetActiveEffect(),
					kParameterHighlightColor,
					Color4(c).ToVector4D());
			}
		}
#endif // /#if SEOUL_EDITOR_AND_TOOLS

		// MeshDrawComponent
		{
			SharedPtr<MeshDrawComponent> pMeshDrawComponent(object.GetComponent<MeshDrawComponent>());
			if (pMeshDrawComponent.IsValid())
			{
				bReturn = pMeshDrawComponent->Render(frustum, *m_pMeshRenderer, techniqueOverride, skinnedTechniqueOverride) || bReturn;
			}
		}

		// TODO: This will never appear at runtime, so it is needless overhead.

		// PrefabComponent
		{
			SharedPtr<PrefabComponent> pComponent(object.GetComponent<PrefabComponent>());
			if (pComponent.IsValid())
			{
				auto const& vObjects = pComponent->GetObjects();
				if (!vObjects.IsEmpty())
				{
					auto const mTransform = object.ComputeNormalTransform();
					Bool const bHasParentTransform = !Matrix4D::Identity().Equals(mTransform);

					// Push the nested transform as a new view transform.
					if (bHasParentTransform)
					{
						auto const mWorld = m_pMeshRenderer->PushWorldMatrix(mTransform);
						frustum.Set(pCamera->GetProjectionMatrix(), pCamera->GetViewMatrix() * mWorld);
					}

					// Draw recursively.
					bReturn = InternalInnerRenderMeshes(
						pCamera,
						frustum,
						vObjects,
						rBuilder,
						techniqueOverride,
						skinnedTechniqueOverride) || bReturn;

					// Pop the nested transform.
					if (bHasParentTransform)
					{
						frustum = origFrustum;
						m_pMeshRenderer->PopWorldMatrix();
					}
				}
			}
		}
	}

	// Editor only, reset the highlight color if it's not the default.
#if SEOUL_EDITOR_AND_TOOLS
	if (cHighlightColor != ColorARGBu8::TransparentBlack())
	{
		rBuilder.SetVector4DParameter(
			m_pMeshRenderer->GetActiveEffect(),
			kParameterHighlightColor,
			Vector4D::Zero());
	}
#endif // /#if SEOUL_EDITOR_AND_TOOLS

	return bReturn;
}

} // namespace Seoul::Scene

#endif // /#if SEOUL_WITH_SCENE
