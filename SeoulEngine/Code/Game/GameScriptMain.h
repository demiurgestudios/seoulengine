/**
 * \file GameScriptMain.h
 * \brief Script proxy for GameMain.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GAME_SCRIPT_MAIN_H
#define GAME_SCRIPT_MAIN_H

#include "Prereqs.h"
namespace Seoul { namespace Game { struct AuthDataRefresh; } }
namespace Seoul { namespace Script { class FunctionInterface; } }

namespace Seoul::Game
{

class ScriptMain SEOUL_SEALED
{
public:
	ScriptMain();
	~ScriptMain();

	Int32 GetConfigUpdateCl() const;
	String GetServerBaseURL() const;
	Bool ManulUpdateRefreshData(const AuthDataRefresh& refreshData);
	void SetAutomationValue(Script::FunctionInterface* pInterface);
};

} // namespace Seoul::Game

#endif // include guard
