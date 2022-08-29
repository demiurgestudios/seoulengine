/**
 * \file GameDevUIMainForm.h
 * \brief Specialization of DevUIMainForm for GameDevUI.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GAME_DEV_UI_MAIN_FORM_H
#define GAME_DEV_UI_MAIN_FORM_H

#include "DevUIMainForm.h"

namespace Seoul::Game
{

class DevUIMainForm SEOUL_SEALED : public DevUI::MainForm
{
public:
	DevUIMainForm();
	~DevUIMainForm();

	// API.
	virtual DevUI::Controller& GetController() SEOUL_OVERRIDE { return *m_pController; }
	virtual void PrePoseMainMenu() SEOUL_OVERRIDE;

	// Settings.
	virtual void ImGuiPrepForLoadSettings() SEOUL_OVERRIDE;

private:
	SEOUL_DISABLE_COPY(DevUIMainForm);

	void InternalPrePoseViewsMenu();

	SharedPtr<DevUI::Controller> m_pController;
}; // class Game::DevUIMainForm

} // namespace Seoul::Game

#endif // include guard
