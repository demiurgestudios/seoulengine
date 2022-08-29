/**
 * \file EditorUICommandNop.cpp
 * \brief Specialization of DevUI::Command that does
 * nothing. Used as a placeholder or sentinel in
 * a command history.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EditorUICommandNop.h"
#include "ReflectionDefine.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(EditorUI::CommandNop, TypeFlags::kDisableNew)
	SEOUL_PARENT(DevUI::Command)
SEOUL_END_TYPE()

} // namespace Seoul
