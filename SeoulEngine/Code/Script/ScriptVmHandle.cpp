/**
 * \file ScriptVmHandle.cpp
 * \brief Specialization of AtomicHandle<> for Script::Vm, allows thread-
 * safe, weak referencing of Script::Vm instances.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ScriptVmHandle.h"

namespace Seoul
{

// NOTE: Assignment here is necessary to convince the linker in our GCC toolchain
// to include this definition. Otherwise, it strips it.
template <>
AtomicHandleTableCommon::Data Script::VmHandleTable::s_Data = AtomicHandleTableCommon::Data();

} // namespace Seoul
