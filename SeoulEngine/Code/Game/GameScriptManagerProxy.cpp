/**
 * \file GameScriptManagerProxy.cpp
 * \brief Proxy object created in lua to talk to Game::ScriptManager
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FileManager.h"
#include "GameScriptManager.h"
#include "GameScriptManagerProxy.h"
#include "ReflectionDefine.h"
#include "ScriptFunctionInterface.h"
#include "SeoulString.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(Game::ScriptManagerProxy, TypeFlags::kDisableCopy)
	SEOUL_METHOD(ReceiveState)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "void", "SlimCS.Table tState, SlimCS.Table tMetatableState")
	SEOUL_METHOD(RestoreState)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(SlimCS.Table, SlimCS.Table)")
SEOUL_END_TYPE()

namespace Game
{

ScriptManagerProxy::ScriptManagerProxy()
{
}

ScriptManagerProxy::~ScriptManagerProxy()
{
}

void ScriptManagerProxy::ReceiveState(Script::FunctionInterface* pInterface) const
{
	pInterface->GetTable(1, ScriptManager::Get()->m_DataStore);
	pInterface->GetTable(2, ScriptManager::Get()->m_MetatablesDataStore);
}

void ScriptManagerProxy::RestoreState(Script::FunctionInterface* pInterface) const
{
	if (!pInterface->PushReturnDataNode(ScriptManager::Get()->m_DataStore, ScriptManager::Get()->m_DataStore.GetRootNode()))
	{
		pInterface->RaiseError(-1, "failed restoring DynamicGameStateData");
	}
	if (!pInterface->PushReturnDataNode(ScriptManager::Get()->m_MetatablesDataStore, ScriptManager::Get()->m_MetatablesDataStore.GetRootNode()))
	{
		pInterface->RaiseError(-1, "failed restoring DynamicGameStateData metatables");
	}
}

} // namespace Game

}
