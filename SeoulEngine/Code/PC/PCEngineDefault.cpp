/**
 * \file PCEngineDefault.cpp
 * \brief Specialization of Engine for the PC platform with default
 * implementations of various online functionality (profiles, sessions, etc.).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "PCEngineDefault.h"
#include "GamePaths.h"

namespace Seoul
{

PCEngineDefault::PCEngineDefault(const PCEngineSettings& settings)
	: PCEngine(settings)
{
}

PCEngineDefault::~PCEngineDefault()
{
}

} // namespace Seoul
