/**
 * \file EditorUISceneRenderer.h
 * \brief View that renders a 3D viewport
 * of a scene hierarchy.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_UI_SCENE_RENDERER_H
#define EDITOR_UI_SCENE_RENDERER_H

#include "AtomicHandle.h"
#include "Axis.h"
#include "EditorUIViewportEffectType.h"
#include "EditorUITransformGizmoHandle.h"
#include "Effect.h"
#include "SharedPtr.h"
#include "Point2DInt.h"
#include "Prereqs.h"
#include "Texture.h"
namespace Seoul { class Camera; }
namespace Seoul { namespace DevUI { class Controller; } }
namespace Seoul { namespace EditorUI { class IControllerSceneRoot; } }
namespace Seoul { namespace EditorUI { class TransformGizmo; } }
namespace Seoul { class IndexBuffer; }
namespace Seoul { class RenderCommandStreamBuilder; }
namespace Seoul { class RenderPass; }
namespace Seoul { class RenderTarget2D; }
namespace Seoul { namespace Scene { class Object; } }
namespace Seoul { namespace Scene { class PrimitiveRenderer; } }
namespace Seoul { namespace Scene { class Renderer; } }
namespace Seoul { namespace Scene { struct RendererConfig; } }
namespace Seoul { class VertexBuffer; }
namespace Seoul { class VertexFormat; }
namespace Seoul { struct Viewport; }
struct ImDrawCmd;
struct ImDrawList;

#if SEOUL_WITH_SCENE

namespace Seoul::EditorUI
{

class SceneRenderer SEOUL_SEALED
{
public:
	struct CurrentPick
	{
		CurrentPick();
		~CurrentPick();

		enum CurrentPickType
		{
			kNone,
			kHandle,
			kObject,
		};

		TransformGizmoHandle m_eHandle;
		SharedPtr<Scene::Object> m_pObject;
		CurrentPickType m_eType;
	};

	SceneRenderer(const Settings& settings);
	~SceneRenderer();

	Seoul::Camera& GetCamera()
	{
		return *m_pCamera;
	}

	void ConfigurePicking(Bool bPick, const Point2DInt& mousePosition);
	const CurrentPick& GetCurrentPick() const
	{
		return m_PickState.m_CurrentPick;
	}

	TransformGizmo& GetGizmo()
	{
		return *m_pTransformGizmo;
	}

	const AtomicHandle<SceneRenderer>& GetThis() const
	{
		return m_hThis;
	}

	void PrePose(DevUI::Controller& rController);

	ViewportEffectType GetSceneRendererType() const
	{
		return m_eSceneRendererType;
	}
	void SetSceneRendererType(ViewportEffectType eType);

private:
	// Callback to submit the scene rendering.
	static void SubmitSceneRenderingCallback(
		const ImDrawList* pParentList,
		const ImDrawCmd* pCommand,
		void* pInContext);
	void SubmitSceneRendering(
		RenderPass& rPass,
		const Viewport& viewport);

	Settings const m_Settings;
	AtomicHandle<SceneRenderer> m_hThis;
	SharedPtr<DevUI::Controller> m_pCachedController;
	ScopedPtr<TransformGizmo> m_pTransformGizmo;
	SharedPtr<Seoul::Camera> m_pCamera;
	ViewportEffectType m_eSceneRendererType;
	Content::Handle<Effect> m_hPrimitiveEffect;
	ScopedPtr<Scene::Renderer> m_pSceneRenderer;
	ScopedPtr<Scene::PrimitiveRenderer> m_pPrimitiveRenderer;
	TextureContentHandle m_hMipVizTexture;

	struct PickState
	{
		PickState()
			: m_PickMousePosition(0, 0)
			, m_bPick(false)
			, m_CurrentPick()
		{
		}

		Point2DInt m_PickMousePosition;
		Bool m_bPick;
		CurrentPick m_CurrentPick;
	};
	PickState m_PickState;

	void InternalPick(
		const Viewport& viewport,
		IControllerSceneRoot* pRoot,
		RenderPass& rPass);
	void InternalRender(
		const Viewport& viewport,
		IControllerSceneRoot* pRoot,
		RenderPass& rPass);
	Bool InternalRenderPrimitives(
		const Viewport& viewport,
		IControllerSceneRoot* pRoot,
		Bool bPicking,
		RenderCommandStreamBuilder& rBuilder);
	void InternalRenderWorldGrid(Axis eAxis);
	Bool InternalRenderTransformGizmo(
		const Viewport& viewport,
		IControllerSceneRoot* pRoot,
		Bool bPicking);

	// SceneReadPixel callbacks.
	friend class SceneReadPixel;
	void OnPick(const SharedPtr<Scene::Object>& pObject);
	void OnPick(TransformGizmoHandle eHandle);
	void OnPickNone();
	// /SceneReadPixel callbacks.

	SEOUL_DISABLE_COPY(SceneRenderer);
}; // class SceneRenderer

typedef AtomicHandle<SceneRenderer> SceneRendererHandle;
typedef AtomicHandleTable<SceneRenderer> SceneRendererHandleTable;

/** Conversion to pointer convenience functions. */
template <typename T>
inline CheckedPtr<T> GetPtr(SceneRendererHandle h)
{
	CheckedPtr<T> pReturn((T*)((SceneRenderer*)SceneRendererHandleTable::Get(h)));
	return pReturn;
}

static inline CheckedPtr<SceneRenderer> GetPtr(SceneRendererHandle h)
{
	CheckedPtr<SceneRenderer> pReturn((SceneRenderer*)SceneRendererHandleTable::Get(h));
	return pReturn;
}

} // namespace Seoul::EditorUI

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
