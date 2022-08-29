/**
 * \file SceneScriptComponent.h
 * \brief Binds a script and its configuration into a 3D scene.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCENE_SCRIPT_COMPONENT_H
#define SCENE_SCRIPT_COMPONENT_H

#include "SceneComponent.h"

#if SEOUL_WITH_SCENE

namespace Seoul::Scene
{

// TODO: ScriptFilePath can be confusing,
// since in our current implementation, it is just
// reduced to a class name (the base filename),
// which is assumed to be the same as the
// registered class name of the contained type.

// TODO: Need to support multiple scripts and settings.
// TODO: Possibly need to support inline settings.

class ScriptComponent SEOUL_SEALED : public Component
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(ScriptComponent);

	ScriptComponent();
	~ScriptComponent();

	virtual SharedPtr<Component> Clone(const String& sQualifier) const SEOUL_OVERRIDE;

	FilePath GetScriptFilePath() const
	{
		return m_ScriptFilePath;
	}

	FilePath GetSettingsFilePath() const
	{
		return m_SettingsFilePath;
	}

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(ScriptComponent);
	SEOUL_REFLECTION_FRIENDSHIP(ScriptComponent);

	FilePath m_ScriptFilePath;
	FilePath m_SettingsFilePath;

	SEOUL_DISABLE_COPY(ScriptComponent);
}; // class ScriptComponent

} // namespace Seoul::Scene

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
