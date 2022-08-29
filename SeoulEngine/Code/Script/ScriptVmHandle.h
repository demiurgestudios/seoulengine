/**
 * \file ScriptVmHandle.h
 * \brief Specialization of AtomicHandle<> for Script::Vm, allows thread-
 * safe, weak referencing of Script::Vm instances.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_VM_HANDLE_H
#define SCRIPT_VM_HANDLE_H

#include "AtomicHandle.h"
#include "CheckedPtr.h"
namespace Seoul { namespace Script { class Vm; } }

namespace Seoul
{

namespace Script { typedef AtomicHandle<Vm> VmHandle; }
namespace Script { typedef AtomicHandleTable<Vm> VmHandleTable; }

/** Conversion to pointer convenience functions. */
template <typename T>
inline CheckedPtr<T> GetPtr(Script::VmHandle h)
{
	CheckedPtr<T> pReturn((T*)((Script::Vm*)Script::VmHandleTable::Get(h)));
	return pReturn;
}

static inline CheckedPtr<Script::Vm> GetPtr(Script::VmHandle h)
{
	CheckedPtr<Script::Vm> pReturn((Script::Vm*)Script::VmHandleTable::Get(h));
	return pReturn;
}

} // namespace Seoul

#endif  // include guard
