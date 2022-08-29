/**
 * \file GameScriptMain.cpp
 * \brief Proxy for Game::Main
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "GameAuthManager.h"
#include "GameAutomation.h"
#include "GameClientSettings.h"
#include "GameMain.h"
#include "GameScriptMain.h"
#include "PatchablePackageFileSystem.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"
#include "ScriptFunctionInterface.h"
#include "ScriptVm.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(Game::ScriptMain, TypeFlags::kDisableCopy)
	SEOUL_METHOD(GetConfigUpdateCl)
	SEOUL_METHOD(GetServerBaseURL)
	SEOUL_METHOD(ManulUpdateRefreshData)
	SEOUL_METHOD(SetAutomationValue)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "void", "string sKey, object oValue")
SEOUL_END_TYPE()

namespace Game
{

ScriptMain::ScriptMain()
{
}

ScriptMain::~ScriptMain()
{
}

Int32 ScriptMain::GetConfigUpdateCl() const
{
	auto pGameMain = Main::Get();
	auto pConfig = (pGameMain.IsValid() ? pGameMain->GetConfigUpdatePackageFileSystem() : nullptr);
	auto iConfigCl = (pConfig.IsValid() ? pConfig->GetBuildChangelist() : 0);
	return iConfigCl;
}

String ScriptMain::GetServerBaseURL() const
{
	auto pGameMain = Main::Get();
	if (!pGameMain.IsValid())
	{
		return ClientSettings::GetServerBaseURL();
	}
	else
	{
		return pGameMain->GetServerBaseURL();
	}
}

Bool ScriptMain::ManulUpdateRefreshData(const AuthDataRefresh& refreshData)
{
	auto pGameAuthManager(AuthManager::Get());
	if (pGameAuthManager)
	{
		return pGameAuthManager->ManulUpdateRefreshData(refreshData);
	}

	return false;
}

void ScriptMain::SetAutomationValue(Script::FunctionInterface* pInterface)
{
	HString sKey;
	SharedPtr<Script::VmObject> pValue;
	if (!pInterface->GetString(1, sKey))
	{
		pInterface->RaiseError(1, "SetAutomationValue: failed to get key.");
		return;
	}
	if (!pInterface->GetObject(2, pValue))
	{
		pInterface->RaiseError(2, "SetAutomationValue: failed to get value.");
		return;
	}

	if (Automation::Get())
	{
		Automation::Get()->SetGlobalState(sKey, pValue);
	}
}

} // namespace Game

} // namespace Seoul
