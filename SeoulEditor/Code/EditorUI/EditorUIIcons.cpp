/**
* \file EditorUIIcons.cpp
* \brief Utility structure with FilePaths to built-in
* editor icons.
*
* Copyright (c) Demiurge Studios, Inc.
* 
* This source code is licensed under the MIT license.
* Full license details can be found in the LICENSE file
* in the root directory of this source tree.
*/

#include "EditorUIIcons.h"

namespace Seoul::EditorUI
{

#define SEOUL_I(name) FilePath::CreateContentFilePath("Authored/Editor/Icons/" #name ".png")

Icons::Icons()
	: m_Audio(SEOUL_I(Audio))
	, m_Brush(SEOUL_I(Brush))
	, m_CSharp(SEOUL_I(CSharp))
	, m_Delete(SEOUL_I(Delete))
	, m_Document(SEOUL_I(Document))
	, m_DocumentText(SEOUL_I(DocumentText))
	, m_EyeClosed(SEOUL_I(EyeClosed))
	, m_EyeOpen(SEOUL_I(EyeOpen))
	, m_Fire(SEOUL_I(Fire))
	, m_Flash(SEOUL_I(Flash))
	, m_FolderClosed(SEOUL_I(FolderClosed))
	, m_FolderOpen(SEOUL_I(FolderOpen))
	, m_Font(SEOUL_I(Font))
	, m_Global(SEOUL_I(Global))
	, m_Logo(SEOUL_I(Logo))
	, m_Lua(SEOUL_I(Lua))
	, m_Mesh(SEOUL_I(Mesh))
	, m_Object(SEOUL_I(Object))
	, m_Prefab(SEOUL_I(Prefab))
	, m_Rotate(SEOUL_I(Rotate))
	, m_Scale(SEOUL_I(Scale))
	, m_Search(SEOUL_I(Search))
	, m_SnapRotation(SEOUL_I(SnapRotation))
	, m_SnapScale(SEOUL_I(SnapScale))
	, m_SnapTranslation(SEOUL_I(SnapTranslation))
	, m_Translate(SEOUL_I(Translate))
	, m_Trash(SEOUL_I(Trash))
{
}

#undef SEOUL_ICON

} // namespace Seoul::EditorUI
