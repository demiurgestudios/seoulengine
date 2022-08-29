/**
 * \file ReflectionTypeUtil.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_TYPE_UTIL_H
#define REFLECTION_TYPE_UTIL_H

#include "Prereqs.h"

namespace Seoul
{

// Utility that (effectively) adds
// the kDisableCopy flag to a Reflectable type.
//
// The purpose of this macro over the kDisableCopy
// flag itself is that, applying this macro
// to a type also applies the kDisableCopy flag
// to all subclasses of the type.
#define SEOUL_REFLECTION_NO_DEFAULT_COPY(type) \
	namespace Reflection { class Any; namespace TypeDetail { \
	typedef void (*DefaultCopyDelegate)(Any& rAny); \
	template <typename BINDER> \
	static DefaultCopyDelegate GetDefaultCopyDelegate(type*) { return nullptr; } \
	} }

} // namespace Seoul

#endif // include guard
