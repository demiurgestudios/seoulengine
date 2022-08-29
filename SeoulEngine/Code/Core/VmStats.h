/**
 * \file VmStats.h
 * \brief Core level global data structure for tracking disparate
 * parts of a typical game configs script <-> native interfaces
 * and harnesses.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef VM_STATS_H
#define VM_STATS_H

#include "Atomic32.h"
#include "Prereqs.h"

namespace Seoul
{

struct VmStats SEOUL_SEALED
{
	Atomic32 m_UIBindingUserData;
	Atomic32 m_UINodes;
}; // struct VmStats

extern VmStats g_VmStats;

} // namespace Seoul

#endif // include guard
