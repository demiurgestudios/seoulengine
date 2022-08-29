/**
 * \file EditorUIViewSceneViewport.h
 * \brief EditorUI view that renders a 3D viewport
 * of a scene hierarchy.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_UI_VIEW_SCENE_VIEWPORT_H
#define EDITOR_UI_VIEW_SCENE_VIEWPORT_H

#include "EditorUICamera.h"
#include "DevUIView.h"
#include "EditorUISceneRenderer.h" // TODO: Eliminate this include.
#include "Point2DInt.h"
#include "ScopedPtr.h"
namespace Seoul { namespace EditorScene { struct CameraState; } }
namespace Seoul { namespace EditorUI { struct Settings; } }
namespace Seoul { namespace EditorUI { class SceneRenderer; } }

#if SEOUL_WITH_SCENE

namespace Seoul::EditorUI
{

class ViewSceneViewport SEOUL_SEALED : public DevUI::View
{
public:
	ViewSceneViewport(const Settings& settings);
	~ViewSceneViewport();

	// DevUI::View overrides
	virtual HString GetId() const SEOUL_OVERRIDE
	{
		static const HString kId("Scene");
		return kId;
	}
	// /DevUI::View overrides

private:
	ScopedPtr<SceneRenderer> m_pRenderer;
	SceneRenderer::CurrentPick m_ContextMenuPick;
	CameraMovement m_CameraMovement;
	Camera m_Camera;
	Bool m_bCapturedRightMouse;
	Bool m_bDragginRightMouse;

	// DevUI::View overrides
	virtual void DoPrePose(DevUI::Controller& rController, RenderPass& rPass) SEOUL_OVERRIDE;
	// /DevUI::View overrides

	Bool InternalCanPlaceObject(FilePath filePath) const;
	void InternalFocusCamera(IControllerSceneRoot& r);
	void InternalPlaceObject(const Viewport& viewport, IControllerSceneRoot& r, FilePath filePath) const;
	void InternalPrePoseAxisGizmo(const Viewport& viewport);
	void InternalPrePoseCameraMode(EditorScene::CameraState& rState);
	void InternalPrePoseRenderMode();
	void InternalPrePoseSnapRotation();
	void InternalPrePoseSnapScale();
	void InternalPrePoseSnapTranslation();
	void InternalPrePoseToolBar(EditorScene::CameraState& rState);
	void InternalPrePoseTransformGizmoMode();

	SEOUL_DISABLE_COPY(ViewSceneViewport);
}; // class ViewSceneViewport

} // namespace Seoul::EditorUI

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
