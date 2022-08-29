/**
 * \file EditorUIMainFormScene.h
 * \brief EditorUI main form for modifying a root scene prefab.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_UI_MAIN_FORM_SCENE_H
#define EDITOR_UI_MAIN_FORM_SCENE_H

#include "DevUIMainForm.h"
#include "Vector.h"
namespace Seoul { namespace EditorUI { class ControllerScene; } }
namespace Seoul { namespace EditorUI { struct Settings; } }
namespace Seoul { class DevUI::View; }

#if SEOUL_WITH_SCENE

namespace Seoul::EditorUI
{

class MainFormScene SEOUL_SEALED : public DevUI::MainForm
{
public:
	MainFormScene(const SharedPtr<ControllerScene>& pController);
	~MainFormScene();

	// DevUI::MainForm overrides
	virtual DevUI::Controller& GetController() SEOUL_OVERRIDE;
	virtual void PrePoseMainMenu() SEOUL_OVERRIDE;
	// /DevUI::MainForm overrides

private:
	SharedPtr<ControllerScene> const m_pController;

	SEOUL_DISABLE_COPY(MainFormScene);
}; // class MainFormScene

} // namespace Seoul::EditorUI

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
