/**
 * \file EditorUIViewSceneInspector.h
 * \brief A specialized editor for scene objects.
 * Wraps an EditorUIViewPropertyEditor and
 * adds some additional functionality.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_UI_VIEW_SCENE_INSPECTOR_H
#define EDITOR_UI_VIEW_SCENE_INSPECTOR_H

#include "EditorUIIControllerSceneRoot.h"
#include "EditorUISceneComponentUtil.h"
#include "EditorUIViewPropertyEditor.h"
#include "SeoulHString.h"
#include "Vector.h"
namespace Seoul { namespace EditorUI { class IControllerSceneRoot; } }
namespace Seoul { namespace Scene { class Object; } }
namespace Seoul { namespace Reflection { class Type; } }

#if SEOUL_WITH_SCENE

namespace Seoul::EditorUI
{

class ViewSceneInspector SEOUL_SEALED : public ViewPropertyEditor
{
public:
	ViewSceneInspector();
	~ViewSceneInspector();

	// DevUI::View overrides
	virtual HString GetId() const SEOUL_OVERRIDE
	{
		static const HString kId("Inspector");
		return kId;
	}
	// /DevUI::View overrides

private:
	SceneComponentUtil::ComponentTypes const m_vComponentTypes;

	// DevUI::View overrides
	virtual void DoPrePose(DevUI::Controller& rController, RenderPass& rPass) SEOUL_OVERRIDE;
	// /DevUI::View overrides

	void InternalPoseComponentMenus(IControllerSceneRoot& rController) const;

	SEOUL_DISABLE_COPY(ViewSceneInspector);
}; // class ViewSceneInspector

} // namespace Seoul::EditorUI

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
