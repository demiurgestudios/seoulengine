/**
 * \file ReflectionMethodBinder.h
 * \brief ReflectionMethodBinder is an internal utility type
 * that maps a SEOUL_METHOD*() macro to a concrete class to
 * wrap the signature of the method used within the macro.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_METHOD_BINDER_H
#define REFLECTION_METHOD_BINDER_H

#include "Prereqs.h"
#include "ReflectionMethodDetail.h"

namespace Seoul::Reflection
{

namespace MethodDetail
{

struct DummyType {};

template <typename R, typename A1 = DummyType, typename A2 = DummyType, typename A3 = DummyType, typename A4 = DummyType, typename A5 = DummyType, typename Dummy = DummyType> class BinderImpl;
template <typename F> class Binder;

// Implementation Method classes for each variation - see
// ReflectionMethodVariations.h for the variations currently implemented,
// and ReflectionMethodBinderInternal.h for the template that is used
// to implement the Method class for each variation.
#define SEOUL_METHOD_VARIATION_IMPL_FILENAME "ReflectionMethodBinderInternal.h"
#include "ReflectionMethodVariations.h"

} // namespace MethodDetail

} // namespace Seoul::Reflection

#endif // include guard
