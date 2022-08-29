/**
 * \file SceneScriptComponent.cpp
 * \brief Binds a script and its configuration into a 3D scene.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"
#include "SceneScriptComponent.h"
#include "Settings.h"
#include "SettingsManager.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

#if SEOUL_EDITOR_AND_TOOLS
SharedPtr<DataStore const> EditorGetSettingsReadOnly(const Scene::ScriptComponent& comp)
{
	auto const filePath = comp.GetSettingsFilePath();
	if (!filePath.IsValid())
	{
		return SharedPtr<DataStore const>();
	}

	auto const hSettings = SettingsManager::Get()->GetSettings(filePath);
	auto const pSettings(hSettings.GetPtr());
	return pSettings;
}
#endif // /#if SEOUL_EDITOR_AND_TOOLS

SEOUL_BEGIN_TYPE(Scene::ScriptComponent, TypeFlags::kDisableCopy)
	SEOUL_DEV_ONLY_ATTRIBUTE(DisplayName, "Script")
	SEOUL_DEV_ONLY_ATTRIBUTE(Category, "Scripting")
	SEOUL_PARENT(Scene::Component)
	SEOUL_PROPERTY_N("ScriptFilePath", m_ScriptFilePath)
		SEOUL_DEV_ONLY_ATTRIBUTE(Description, "FilePath to script class to instantiate at runtime.")
		SEOUL_DEV_ONLY_ATTRIBUTE(EditorFileSpec, GameDirectory::kContent, FileType::kScript)
	SEOUL_PROPERTY_N("SettingsFilePath", m_SettingsFilePath)
		SEOUL_ATTRIBUTE(NotRequired)
		SEOUL_DEV_ONLY_ATTRIBUTE(Description, "JSON (.json) files used as the script's data.")
		SEOUL_DEV_ONLY_ATTRIBUTE(EditorFileSpec, GameDirectory::kConfig, FileType::kJson)

#if SEOUL_EDITOR_AND_TOOLS
	SEOUL_PROPERTY_N_Q("Settings Preview", EditorGetSettingsReadOnly)
		SEOUL_ATTRIBUTE(DoNotSerialize)
#endif // /#if SEOUL_EDITOR_AND_TOOLS

SEOUL_END_TYPE()

namespace Scene
{

ScriptComponent::ScriptComponent()
	: m_ScriptFilePath()
	, m_SettingsFilePath()
{
}

ScriptComponent::~ScriptComponent()
{
}

SharedPtr<Component> ScriptComponent::Clone(const String& sQualifier) const
{
	SharedPtr<ScriptComponent> pReturn(SEOUL_NEW(MemoryBudgets::SceneComponent) ScriptComponent);
	pReturn->m_ScriptFilePath = m_ScriptFilePath;
	pReturn->m_SettingsFilePath = m_SettingsFilePath;
	return pReturn;
}

} // namespace Scene

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
