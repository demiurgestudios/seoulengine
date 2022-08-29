/**
 * \file EditorUIDragSourceFilePath.h
 * \brief Drag and drop source that points at an asset
 * FilePath.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_UI_DRAG_SOURCE_FILE_PATH_H
#define EDITOR_UI_DRAG_SOURCE_FILE_PATH_H

#include "FilePath.h"

namespace Seoul::EditorUI
{

// Tag for a drag and drop operation that is the currently selected objects of a scene.
struct DragSourceFilePath
{
	FilePath m_FilePath{};
}; // struct DragSourceFilePath

} // namespace Seoul::EditorUI

#endif // include guard
