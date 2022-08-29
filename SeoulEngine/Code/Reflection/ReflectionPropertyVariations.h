/**
 * \file ReflectionPropertyVariations.h
 * \brief Defines all the variations of Property that are supported
 * for reflection - internal use only, should only be included
 * by files that want to implement functionality that needs to vary
 * per property type supported.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#ifndef SEOUL_PROPERTY_VARIATION_IMPL_FILENAME
#error ReflectionPropertyVariations.h should only be by files that define SEOUL_PROPERTY_VARIATION_IMPL_FILENAME.
#endif

	// Variation getter/setter, point to member property:
	//
	//     T m_MemberVariable
	//
#	define SEOUL_PROP_NAME PointerToMemberProperty
#	define SEOUL_PROP_1_SIG T (C::*P1)
#	define SEOUL_PROP_GET_PTR(instance, value) value = &(instance->*P1)
#	define SEOUL_PROP_GET(instance, value) value = (instance->*P1)
#	define SEOUL_PROP_SET(instance, value) (instance->*P1) = value
#	include SEOUL_PROPERTY_VARIATION_IMPL_FILENAME

	// Variation getter only:
	//
	//     const T& Get() const
	//
#	define SEOUL_PROP_NAME PointerToConstMemberConstRefGet
#	define SEOUL_PROP_1_SIG const T& (C::*P1)() const
#	define SEOUL_PROP_GET(instance, value) value = (instance->*P1)()
#	include SEOUL_PROPERTY_VARIATION_IMPL_FILENAME

	// Variation getter only:
	//
	//     T Get() const
	//
#	define SEOUL_PROP_NAME PointerToConstMemberGet
#	define SEOUL_PROP_1_SIG T (C::*P1)() const
#	define SEOUL_PROP_GET(instance, value) value = (instance->*P1)()
#	include SEOUL_PROPERTY_VARIATION_IMPL_FILENAME

	// Variation getter only, using global function, where C is the class of Type
	// that this property belongs to.
	//
	//     T Get(const C&)
	//
#	define SEOUL_PROP_NAME GlobalFunctionGetx
#	define SEOUL_PROP_1_SIG T (*P1)(const C&)
#	define SEOUL_PROP_GET(instance, value) value = (P1)(*instance)
#	include SEOUL_PROPERTY_VARIATION_IMPL_FILENAME

	// Variation getter/setter, global function that returns a reference
	//
	//     T& Get(C&)
	//
#	define SEOUL_PROP_NAME GlobalFunctionReferenceGet
#	define SEOUL_PROP_1_SIG T& (*P1)(C&)
#	define SEOUL_PROP_GET_PTR(instance, value) value = &((P1)(*((C*)instance)))
#	define SEOUL_PROP_GET(instance, value) value = (P1)(const_cast<C&>(*instance))
#	define SEOUL_PROP_SET(instance, value) (P1)(*instance) = value
#	include SEOUL_PROPERTY_VARIATION_IMPL_FILENAME

	// Variation getter/setter, global function, where C is the class of Type
	// that this property belongs to.
	//
	//     T Get(const C&)
	//  void Set(C&, T)
	//
#	define SEOUL_PROP_NAME GlobalFunctionGetSetx
#	define SEOUL_PROP_1_SIG T (*P1)(const C&)
#	define SEOUL_PROP_2_SIG void (*P2)(C&, T)
#	define SEOUL_PROP_GET(instance, value) value = (P1)(*instance)
#	define SEOUL_PROP_SET(instance, value) (P2)(*instance, value)
#	include SEOUL_PROPERTY_VARIATION_IMPL_FILENAME

	// Variation getter/setter for:
	//
	//     const T& Get() const
	//     void Set(const T&)
	//
#	define SEOUL_PROP_NAME PointerToMemberConstRefGetterConstRefSetter
#	define SEOUL_PROP_1_SIG const T& (C::*P1)() const
#	define SEOUL_PROP_2_SIG void (C::*P2)(const T&)
#	define SEOUL_PROP_GET(instance, value) value = (instance->*P1)()
#	define SEOUL_PROP_SET(instance, value) (instance->*P2)(value)
#	include SEOUL_PROPERTY_VARIATION_IMPL_FILENAME

	// Variation getter/setter for:
	//
	//     T Get() const
	//     void Set(T)
	//
#	define SEOUL_PROP_NAME PointerToMemberGetterSetter
#	define SEOUL_PROP_1_SIG T (C::*P1)() const
#	define SEOUL_PROP_2_SIG void (C::*P2)(T)
#	define SEOUL_PROP_GET(instance, value) value = (instance->*P1)()
#	define SEOUL_PROP_SET(instance, value) (instance->*P2)(value)
#	include SEOUL_PROPERTY_VARIATION_IMPL_FILENAME

	// Variation getter/setter for:
	//
	//     T Get() const
	//     void Set(const T&)
	//
#	define SEOUL_PROP_NAME PointerToMemberGetterConstRefSetter
#	define SEOUL_PROP_1_SIG T (C::*P1)() const
#	define SEOUL_PROP_2_SIG void (C::*P2)(const T&)
#	define SEOUL_PROP_GET(instance, value) value = (instance->*P1)()
#	define SEOUL_PROP_SET(instance, value) (instance->*P2)(value)
#	include SEOUL_PROPERTY_VARIATION_IMPL_FILENAME

	// Variation setter for:
	//
	//     void Set(const T&)
	//
#	define SEOUL_PROP_NAME PointerToConstRefSetter
#	define SEOUL_PROP_1_SIG void (C::*P1)(const T&)
#	define SEOUL_PROP_SET(instance, value) (instance->*P1)(value)
#	include SEOUL_PROPERTY_VARIATION_IMPL_FILENAME

#undef SEOUL_PROPERTY_VARIATION_IMPL_FILENAME
