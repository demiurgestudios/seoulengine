/**
 * \file ScriptNetworkManager.h
 * \brief Binder instance for instantiating NetworkMessenger
 * instances into script.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_NETWORK_MANAGER_H
#define SCRIPT_NETWORK_MANAGER_H

#include "Prereqs.h"
namespace Seoul { namespace Script { class FunctionInterface; } }

#if SEOUL_WITH_NETWORK

namespace Seoul
{

class ScriptNetworkManager SEOUL_SEALED
{
public:
	ScriptNetworkManager();
	~ScriptNetworkManager();

	void NewMessenger(Script::FunctionInterface* pInterface);

private:
	SEOUL_DISABLE_COPY(ScriptNetworkManager);
}; // class ScriptNetworkManager

} // namespace Seoul

#endif // /#if SEOUL_WITH_NETWORK

#endif // include guard
