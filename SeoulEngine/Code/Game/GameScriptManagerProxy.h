/**
 * \file GameScriptManagerProxy.h
 * \brief Proxy object created in lua to talk to Game::ScriptManager
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GAME_SCRIPT_MANAGER_PROXY_H
#define GAME_SCRIPT_MANAGER_PROXY_H

#include "Prereqs.h"
#include "Vector.h"
namespace Seoul { namespace Script { class FunctionInterface; } }
namespace Seoul { class String; }

namespace Seoul::Game
{

class ScriptManagerProxy SEOUL_SEALED
{
public:
	ScriptManagerProxy();
	~ScriptManagerProxy();

	void ReceiveState(Script::FunctionInterface* pInterface) const;
	void RestoreState(Script::FunctionInterface* pInterface) const;

private:
};

} // namespace Seoul::Game

#endif // include guard
