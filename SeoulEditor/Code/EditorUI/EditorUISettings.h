/**
 * \file EditorUISettings.h
 * \brief Configuration structure for the EditorUI project.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_UI_SETTINGS_H
#define EDITOR_UI_SETTINGS_H

#include "Delegate.h"
#include "EditorUIViewportEffectType.h"
#include "FilePath.h"
#include "FixedArray.h"
namespace Seoul { struct CustomCrashErrorState; }

namespace Seoul::EditorUI
{

struct Settings SEOUL_SEALED
{
	Settings()
		: m_aFxEffectFilePaths()
		, m_aMeshEffectFilePaths()
		, m_PrimitiveEffectFilePath()
	{
	}

	/** FilePath of the Microsoft FX to use for particle FX rendering. */
	FixedArray<FilePath, (UInt32)ViewportEffectType::COUNT> m_aFxEffectFilePaths;

	/** FilePath of the Microsoft FX to use for mesh rendering. */
	FixedArray<FilePath, (UInt32)ViewportEffectType::COUNT> m_aMeshEffectFilePaths;

	/** FilePath of the Microsoft FX to use for primitive rendering. */
	FilePath m_PrimitiveEffectFilePath;
}; // struct Settings

} // namespace Seoul::EditorUI

#endif // include guard
