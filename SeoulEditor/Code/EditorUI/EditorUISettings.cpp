/**
 * \file EditorUISettings.cpp
 * \brief Configuration structure for the EditorUI project.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EditorUISettings.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(EditorUI::Settings)
	SEOUL_PROPERTY_N("FxEffectFilePaths", m_aFxEffectFilePaths)
	SEOUL_PROPERTY_N("MeshEffectFilePaths", m_aMeshEffectFilePaths)
	SEOUL_PROPERTY_N("PrimitiveEffectFilePath", m_PrimitiveEffectFilePath)
SEOUL_END_TYPE()

} // namespace Seoul
