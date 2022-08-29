/**
 * \file ReflectionDeclare.h
 * \brief File to include when a type wants to reflect private members - use
 * the SEOUL_REFLECTION_FRIENDSHIP() macro within a private: section of the
 * type declaration.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_DECLARE_H
#define REFLECTION_DECLARE_H

#include "ReflectionWeakAny.h"

namespace Seoul
{

namespace Reflection
{

struct MethodBuilder;
struct NewUtil;
struct PropertyBuilder;
struct TypeBuilder;

class Type;

} // namespace Reflection

/**
 * Use this macro inside a private: block of a type in order to grant access
 * to the reflection system to private members.
 */
#define SEOUL_REFLECTION_FRIENDSHIP(type) \
	friend struct ::Seoul::Reflection::MethodBuilder; \
	friend struct ::Seoul::Reflection::NewUtil; \
	friend struct ::Seoul::Reflection::PropertyBuilder; \
	friend struct ::Seoul::Reflection::TypeBuilder; \
	friend struct ::Seoul::Reflection::TypeOfDetail<type>;

/**
 * For polymorphic types, use this macro in a public: block to define a method which
 * will return WeakAny of a pointer of the most derived class - this macro must be used
 * in every class in the hierarchy to get correct behavior.
 */
#define SEOUL_REFLECTION_POLYMORPHIC_BASE(type) \
	virtual ::Seoul::Reflection::WeakAny GetReflectionThis() const { return ::Seoul::Reflection::WeakAny(this); } \
	virtual ::Seoul::Reflection::WeakAny GetReflectionThis() { return ::Seoul::Reflection::WeakAny(this); }

#define SEOUL_REFLECTION_POLYMORPHIC(type) \
	virtual ::Seoul::Reflection::WeakAny GetReflectionThis() const SEOUL_OVERRIDE { return ::Seoul::Reflection::WeakAny(this); } \
	virtual ::Seoul::Reflection::WeakAny GetReflectionThis() SEOUL_OVERRIDE { return ::Seoul::Reflection::WeakAny(this); }

} // namespace Seoul

#endif // include guard
