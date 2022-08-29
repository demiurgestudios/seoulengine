/**
 * \file AppPCRunScript.h
 * \brief Defines the main function for a build run that will execute
 * an arbitrary Lua script and then exit.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef APP_PC_RUN_SCRIPT_H
#define APP_PC_RUN_SCRIPT_H

#include "Prereqs.h"

namespace Seoul
{

#if !SEOUL_SHIP
Int AppPCRunScript(Bool bGenerateReflectionDef, const String& sFilePath);
#endif // /#if !SEOUL_SHIP

} // namespace Seoul

#endif // include guard
