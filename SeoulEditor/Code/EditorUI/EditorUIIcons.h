/**
 * \file EditorUIIcons.h
 * \brief Utility structure with FilePaths to built-in
 * editor icons.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_UI_ICONS_H
#define EDITOR_UI_ICONS_H

#include "FilePath.h"

namespace Seoul::EditorUI
{

struct Icons SEOUL_SEALED
{
	Icons();

	FilePath const m_Audio;
	FilePath const m_Brush;
	FilePath const m_CSharp;
	FilePath const m_Delete;
	FilePath const m_Document;
	FilePath const m_DocumentText;
	FilePath const m_EyeClosed;
	FilePath const m_EyeOpen;
	FilePath const m_Fire;
	FilePath const m_Flash;
	FilePath const m_FolderClosed;
	FilePath const m_FolderOpen;
	FilePath const m_Font;
	FilePath const m_Global;
	FilePath const m_Logo;
	FilePath const m_Lua;
	FilePath const m_Mesh;
	FilePath const m_Object;
	FilePath const m_Prefab;
	FilePath const m_Rotate;
	FilePath const m_Scale;
	FilePath const m_Search;
	FilePath const m_SnapRotation;
	FilePath const m_SnapScale;
	FilePath const m_SnapTranslation;
	FilePath const m_Translate;
	FilePath const m_Trash;
}; // struct Icons

} // namespace Seoul::EditorUI

#endif // include guard
