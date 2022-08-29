/**
 * \file SeoulOS.h
 * \brief Platform-agnostic  around common OS behavior
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SEOUL_OS_H
#define SEOUL_OS_H

#include "Prereqs.h"
#include "SeoulString.h"

namespace Seoul
{

// Get an environment variable by name. Returns "" if unset.
String GetEnvironmentVar(const String& name);

// Get the current username. Only implemented in Windows and Linux; otherwise, returns an empty string.
String GetUsername();

} // namespace Seoul

#endif // include guard
