/**
 * \file ScriptMotion.h
 * \brief Linker hooks for motion utils in the ScriptMotion project.
 * Include this header in an accessible CPP of your project to
 * make these utilities available to script.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_MOTION_H
#define SCRIPT_MOTION_H

#include "ReflectionDeclare.h"

namespace Seoul
{

SEOUL_LINK_ME(class, ScriptMotionApproach);
SEOUL_LINK_ME(class, ScriptMotionPointToMove);

} // namespace Seoul

#endif // include guard
