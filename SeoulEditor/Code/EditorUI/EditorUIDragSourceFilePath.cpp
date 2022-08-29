/**
 * \file EditorUIDragSourceFilePath.cpp
 * \brief Drag and drop source that points at an asset
 * FilePath.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EditorUIDragSourceFilePath.h"
#include "ReflectionDefine.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(EditorUI::DragSourceFilePath)
	SEOUL_PROPERTY_N("FilePath", m_FilePath)
SEOUL_END_TYPE()

} // namespace Seoul
