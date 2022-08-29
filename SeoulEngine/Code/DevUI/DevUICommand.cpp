/**
 * \file DevUICommand.cpp
 * \brief Base class for undo/redo style command processing in a DevUI::Root subclass/project.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DevUICommand.h"
#include "ReflectionDefine.h"

namespace Seoul
{

SEOUL_TYPE(DevUI::Command, TypeFlags::kDisableNew);

} // namespace Seoul
