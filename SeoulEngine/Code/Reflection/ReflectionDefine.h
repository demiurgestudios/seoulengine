/**
 * \file ReflectionDefine.h
 * \brief File to include to define the reflection capabilities of a type -
 * to avoid executable bloat, do not include this header in other header files,
 * only in CPP files.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_DEFINE_H
#define REFLECTION_DEFINE_H

#include "FixedArray.h"
#include "ReflectionArrayDetail.h"
#include "ReflectionBuilders.h"
#include "ReflectionMethod.h"
#include "ReflectionMethodBinder.h"
#include "ReflectionMethodDetail.h"
#include "ReflectionPrereqs.h"
#include "ReflectionProperty.h"
#include "ReflectionPropertyDetail.h"
#include "ReflectionTableDetail.h"
#include "ReflectionType.h"
#include "ReflectionTypeDetail.h"
#include "ReflectionTypeInfo.h"

namespace Seoul
{

namespace Reflection
{

/**
 * Implementation of ArrayOfDetail::Get(), which is used to generate
 * a Reflection::Array specialization for types that fulfill the array
 * contract.
 */
template <typename T>
const Reflection::Array& ArrayOfDetail<T>::Get()
{
	static const Reflection::ArrayDetail::ArrayT<T> s_kArray;
	return s_kArray;
}

/**
 * Implementation of TableOfDetail::Get(), which is used to generate
 * a Reflection::Table specialization for types that fulfill the table
 * contract.
 */
template <typename T>
const Reflection::Table& TableOfDetail<T>::Get()
{
	static const Reflection::TableDetail::TableT<T> s_kTable;
	return s_kTable;
}

} // namespace Reflection

// Primary macros used to define class reflection.

/**
 * SEOUL_BEGIN_TYPE() is the macro to use when you want to define properties, attributes, etc. for
 * a type.
 *
 * \example
 *   SEOUL_BEGIN_TYPE(Foo)
 *       SEOUL_ATTRIBUTE_DEV_ONLY(Description, "A super awesome class.")
 *       SEOUL_PROPERTY(X)
 *           SEOUL_ATTRIBUTE_DEV_ONLY(Description, "X position of this super awesome class.")
 *       SEOUL_PROPERTY("Bar", m_Bar)
 *   SEOUL_END_TYPE()
 */
#define SEOUL_BEGIN_TYPE(type, ...) \
	namespace Reflection \
	{ \
		template <> \
		struct TypeOfDetail<type> { static const Reflection::Type& Get(); }; \
		template <> \
		const Reflection::Type& TypeOfDetailStaticOwner<type>::s_kStaticType(Reflection::TypeOfDetail<type>::Get()); \
		const Reflection::Type& TypeOfDetail<type>::Get() \
		{ \
			typedef type ReflectionType; \
			typedef Reflection::TypeTDiscovery<ReflectionType>::Type TypeTImpl; \
			static const TypeTImpl s_kType(Reflection::TypeBuilder( \
				TypeInfoDetail::TypeInfoImpl<ReflectionType>::Get(), \
				#type, \
				Reflection::TypeDetail::NewDelegateBind<ReflectionType, ##__VA_ARGS__>::GetNewDelegate(), \
				Reflection::TypeDetail::DeleteDelegateBind<ReflectionType, ##__VA_ARGS__>::GetDeleteDelegate(), \
				Reflection::TypeDetail::InPlaceNewDelegateBind<ReflectionType, ##__VA_ARGS__>::GetInPlaceNewDelegate(), \
				Reflection::TypeDetail::DestructorDelegateBind<ReflectionType, ##__VA_ARGS__>::GetDestructorDelegate(), \
				Reflection::TypeDetail::GetDefaultCopyDelegate< Reflection::TypeDetail::DefaultCopyDelegateBind< ReflectionType, ##__VA_ARGS__> >((ReflectionType*)nullptr))

/**
 * Utility for SEOUL_GET_FULLY_QUALIFIED_TYPE_NAME,
 * used internally by the reflection system. Discovers
 * all qualifiers on the type and appends them
 * to the name, queried "raw", so it is static
 * initialization safe.
 */
template <typename T>
static inline String GetFullyQualifiedTypeName()
{
	String sReturn;
	if (IsConst<T>::Value)
	{
		sReturn = GetFullyQualifiedTypeName<typename RemoveConst<T>::Type>() + " const";
	}
	else if (IsPointer<T>::Value)
	{
		sReturn = GetFullyQualifiedTypeName<typename RemovePointer<T>::Type>() + "*";
	}
	else if (IsVolatile<T>::Value)
	{
		sReturn = GetFullyQualifiedTypeName<typename RemoveVolatile<T>::Type>() + " volatile";
	}
	else if (IsReference<T>::Value)
	{
		sReturn = GetFullyQualifiedTypeName<typename RemoveReference<T>::Type>() + "&";
	}
	else
	{
		sReturn = String(Reflection::TypeOfDetail<typename RemoveAllCvReferencePointer<T>::Type>::Get().GetName());
	}

	return sReturn;
}
#define SEOUL_GET_FULLY_QUALIFIED_TYPE_NAME(type) GetFullyQualifiedTypeName<type>().CStr()

/** Simple helper - gets a TypeId, safe in reflection definition bodies. */
#define SEOUL_GET_TYPE_INFO(type) (Reflection::TypeInfoDetail::TypeInfoImpl<type>::Get())

/**
 * SEOUL_BEGIN_TEMPLATE_TYPE() is the macro to use when you want to define properties, attributes, etc. for
 * a templated type - i.e. Vector<T>.
 *
 * \example
 *   SEOUL_BEGIN_TEMPLATE_TYPE(Foo, (T), (typename T), ("Foo<%s>", SEOUL_GET_FULLY_QUALIFIED_TYPE_NAME))
 *       SEOUL_ATTRIBUTE_DEV_ONLY(Description, "A super awesome class.")
 *       SEOUL_PROPERTY(X)
 *           SEOUL_ATTRIBUTE_DEV_ONLY(Description, "X position of this super awesome class.")
 *       SEOUL_PROPERTY("Bar", m_Bar)
 *   SEOUL_END_TYPE()
 *
 * The name expression must evaluate to an entirely unique name per specialization of the templated
 * type, or an assertion will occur when the type is added to the Reflection::Registry().
 *
 * \param[in] type Base name of the templated type for which reflection is being defined - i.e. Vector
 * \param[in] args Template args used to specialize the type, surrounded in parentheses - i.e. (T, MEMORY_BUDGETS)
 * \param[in] signature Template declaration for the type, surrounded in parentheses - i.e. (typename T, int MEMORY_BUDGETS)
 * \param[in] name_expression An expression that will be evaluated to generate a unique name for each specialization
 *            of this templated type.
 *
 * Where s_kExplicitBind is a global variable.
 */
#if SEOUL_IMPLICIT_TEMPLATED_REFLECTION_DEFINITION
#define SEOUL_BEGIN_TEMPLATE_TYPE(type, args, signature, name_expression, ...) \
	namespace Reflection \
	{ \
		template <SEOUL_REMOVE_PARENTHESES(signature)> \
		struct TypeOfDetail< type<SEOUL_REMOVE_PARENTHESES(args)> > { static const Reflection::Type& Get(); }; \
		template <SEOUL_REMOVE_PARENTHESES(signature)> \
		struct TypeOfDetailStaticOwner< type<SEOUL_REMOVE_PARENTHESES(args)> > { static const Reflection::Type& s_kStaticType; }; \
		template <SEOUL_REMOVE_PARENTHESES(signature)> \
		const Reflection::Type& TypeOfDetailStaticOwner< type<SEOUL_REMOVE_PARENTHESES(args)> >::s_kStaticType(Reflection::TypeOfDetail< type<SEOUL_REMOVE_PARENTHESES(args)> >::Get()); \
		template <SEOUL_REMOVE_PARENTHESES(signature)> \
		const Reflection::Type& TypeOfDetail< type<SEOUL_REMOVE_PARENTHESES(args)> >::Get() \
		{ \
			typedef type<SEOUL_REMOVE_PARENTHESES(args)> ReflectionType; \
			typedef typename Reflection::TypeTDiscovery<ReflectionType>::Type TypeTImpl; \
			static const TypeTImpl s_kType(Reflection::TypeBuilder( \
				TypeInfoDetail::TypeInfoImpl<ReflectionType>::Get(), \
				String::Printf(SEOUL_REMOVE_PARENTHESES(name_expression)), \
				Reflection::TypeDetail::NewDelegateBind<ReflectionType, ##__VA_ARGS__>::GetNewDelegate(), \
				Reflection::TypeDetail::DeleteDelegateBind<ReflectionType, ##__VA_ARGS__>::GetDeleteDelegate(), \
				Reflection::TypeDetail::InPlaceNewDelegateBind<ReflectionType, ##__VA_ARGS__>::GetInPlaceNewDelegate(), \
				Reflection::TypeDetail::DestructorDelegateBind<ReflectionType, ##__VA_ARGS__>::GetDestructorDelegate(), \
				Reflection::TypeDetail::GetDefaultCopyDelegate< Reflection::TypeDetail::DefaultCopyDelegateBind< ReflectionType, ##__VA_ARGS__> >((ReflectionType*)nullptr))
#else
#define SEOUL_BEGIN_TEMPLATE_TYPE(type, args, signature, name_expression, ...) \
	namespace Reflection \
	{ \
		template <SEOUL_REMOVE_PARENTHESES(signature)> \
		struct TemplateTypeOfDetail< type<SEOUL_REMOVE_PARENTHESES(args)> > { static const Reflection::Type& Get(); }; \
		template <SEOUL_REMOVE_PARENTHESES(signature)> \
		const Reflection::Type& TemplateTypeOfDetail< type<SEOUL_REMOVE_PARENTHESES(args)> >::Get() \
		{ \
			typedef type<SEOUL_REMOVE_PARENTHESES(args)> ReflectionType; \
			typedef typename Reflection::TypeTDiscovery<ReflectionType>::Type TypeTImpl; \
			static const TypeTImpl s_kType(Reflection::TypeBuilder( \
				TypeInfoDetail::TypeInfoImpl<ReflectionType>::Get(), \
				String::Printf(SEOUL_REMOVE_PARENTHESES(name_expression)), \
				Reflection::TypeDetail::NewDelegateBind<ReflectionType, ##__VA_ARGS__>::GetNewDelegate(), \
				Reflection::TypeDetail::DeleteDelegateBind<ReflectionType, ##__VA_ARGS__>::GetDeleteDelegate(), \
				Reflection::TypeDetail::InPlaceNewDelegateBind<ReflectionType, ##__VA_ARGS__>::GetInPlaceNewDelegate(), \
				Reflection::TypeDetail::DestructorDelegateBind<ReflectionType, ##__VA_ARGS__>::GetDestructorDelegate(), \
				Reflection::TypeDetail::GetDefaultCopyDelegate< Reflection::TypeDetail::DefaultCopyDelegateBind< ReflectionType, ##__VA_ARGS__> >((ReflectionType*)nullptr))
#endif

#define SEOUL_END_TYPE() \
				); \
			return s_kType; \
		} \
	}

/**
 * See comment on SEOUL_IMPLICIT_TEMPLATED_REFLECTION_DEFINITION for more information.
 *
 * When SEOUL_IMPLICIT_TEMPLATED_REFLECTION_DEFINITION is defined to 0, specializations
 * of templated types that are referenced by reflection must be explicitly enumerated,
 * to reduce compilation times from redundant specialization definition bloat that would
 * occur if the compiler was allowed to implicitly emit the definitions.
 *
 * A missing specialization will result in a link-time failure (MSVC):
 *
 *   unresolved external symbol "public: static class Seoul::Reflection::Type const & const Seoul::Reflection::TypeOfDetailStaticOwner<class Seoul::Vector<struct Seoul::GameSettings::ComicSlotPurchaseInfo,21> >
 *
 * The missing dependency must then be explicitly enumerated in an appropriate cpp file:
 *
 *   SEOUL_SPEC_TEMPLATE_TYPE(Vector<GameSettings::ComicSlotPurchaseInfo, 21>)
 *
 * (note that the number 21 in the above example is the value of the MemoryBudget enum argument
 *  to the Vector<> template - the linker reduces enums to their raw integer value).
 *
 * An "appropriate cpp file" is usually one that already defines reflection of the argument type of
 * the templated type (e.g. for the example above, the ideal CPP file is likely the CPP
 * file that defines the SEOUL_BEIN_TYPE() reflection for GameSettings::ComicSlotPurchaseInfo). Note
 * that the reflection definition of the templated type must also be visible to the CPP file - often
 * this means ReflectionCoreTemplateTypes.h must be included (for the most common templated types
 * in engine).
 *
 * Strictly speaking, the SEOUL_SPEC_TEMPLATE_TYPE() can be used in any CPP file. It can also be used
 * redundantly (the same specialization in multiple CPP files), though it should *not* be to avoid the
 * redundant definition bloat that SEOUL_SPEC_TEMPLATE_TYPE() is designed to eliminate in the first place
 * In short, SEOUL_SPEC_TEMPLATE_TYPE() should always appear in a single CPP file for a particular
 * specialization, and should never appear in a header file.
 */
#if SEOUL_IMPLICIT_TEMPLATED_REFLECTION_DEFINITION
#define SEOUL_SPEC_TEMPLATE_TYPE(...)
#else
#define SEOUL_SPEC_TEMPLATE_TYPE_IMPL(type) \
	namespace Reflection \
	{ \
		template <> \
		struct TypeOfDetail<SEOUL_REMOVE_PARENTHESES(type)> { static const Reflection::Type& Get(); }; \
		template <> \
		const Reflection::Type& TypeOfDetailStaticOwner<SEOUL_REMOVE_PARENTHESES(type)>::s_kStaticType(Reflection::TypeOfDetail<SEOUL_REMOVE_PARENTHESES(type)>::Get()); \
		const Reflection::Type& TypeOfDetail<SEOUL_REMOVE_PARENTHESES(type)>::Get() { return TemplateTypeOfDetail<SEOUL_REMOVE_PARENTHESES(type)>::Get(); } \
	}
#define SEOUL_SPEC_TEMPLATE_TYPE(...) SEOUL_SPEC_TEMPLATE_TYPE_IMPL((__VA_ARGS__))
#endif

/**
 * SEOUL_TYPE() is useful when all you want to do is define a Type object
 * for a particular class or struct. Using this macro does not allow you to define properties or attributes
 * for reflection - if you need these, use SEOUL_BEGIN_TYPE()/SEOUL_END_TYPE().
 */
#define SEOUL_TYPE(type, ...) \
	SEOUL_BEGIN_TYPE(type, ##__VA_ARGS__) \
	SEOUL_END_TYPE()

#define SEOUL_TEMPLATE_TYPE(type, args, signature, name_expression, ...) \
	SEOUL_BEGIN_TEMPLATE_TYPE(type, args, signature, name_expression, ##__VA_ARGS__) \
	SEOUL_END_TYPE()

/**
 * SEOUL_BEGIN_ENUM() is the macro to use when you want to define an enum type - enum reflection is
 * much simpler than Type reflection. It exists primarily to allow automatic conversion between
 * an enum's values and their string representation, for robust serialization and debugging.
 *
 * SEOUL_BEGIN_ENUM() also defines the Type for an enum - do not use SEOUL_BEGIN_TYPE() and SEOUL_BEGIN_ENUM()
 * on the same type, this will result in a linker error.
 */
#define SEOUL_BEGIN_ENUM(type, ...) \
	SEOUL_STATIC_ASSERT_MESSAGE(IsEnum<type>::Value, "SEOUL_BEGIN_ENUM() cannot be used to define enum reflection for a type T which is not a C enum."); \
	SEOUL_TYPE(type); \
	namespace Reflection \
	{ \
		template<> \
		struct EnumOfDetail<type> { static const Reflection::Enum& Get(); }; \
		template<> \
		const Reflection::Enum& EnumOfDetailStaticOwner<type>::s_kStaticType(Reflection::EnumOfDetail<type>::Get()); \
		const Reflection::Enum& EnumOfDetail<type>::Get() \
		{ \
			static const Reflection::Enum s_kEnum(Reflection::EnumBuilder( \
				TypeInfoDetail::TypeInfoImpl<type>::Get(), \
				#type, \
				##__VA_ARGS__)

#define SEOUL_END_ENUM() \
				); \
				return s_kEnum; \
		} \
	}

/**
 * Define a single enum value in an enumeration - can specify a custom name for the enum with this macro.
 *
 * Only valid inside a SEOUL_BEGIN_ENUM()/SEOUL_END_ENUM() block.
 */
#define SEOUL_ENUM_N(name, value) .AddEnum(name, value)

/**
 * Define a single enum value in an enumeration - the name of the value will be equal to its code name.
 *
 * Only valid inside a SEOUL_BEGIN_ENUM()/SEOUL_END_ENUM() block.
 */
#define SEOUL_ENUM(value) SEOUL_ENUM_N(#value, value)

/**
 * Define an alias - allows for backwards compatibility. Aliases are used when resolving names for
 * properties and methods.
 *
 * Only valid inside a SEOUL_BEGIN_TYPE()/SEOUL_END_TYPE() block, a SEOUL_BEGIN_TEMPLATE_TYPE()/SEOUL_END_TYPE() block,
 * or a SEOUL_BEGIN_ENUM()/SEOUL_END_ENUM() block.
 */
#define SEOUL_ALIAS(from_name, to_name) \
	.AddAlias(from_name, to_name)

/**
 * Define a type alias - same as an alias, but used when retrieving a Type by name from Registry::GetType().
 */
#define SEOUL_TYPE_ALIAS(from_name) \
	.AddTypeAlias(from_name)

/**
 * Place inside a SEOUL_BEGIN_TYPE()/SEOUL_END_TYPE() pair to associate attributes -
 * placing this immediately after the SEOUL_BEGIN_TYPE() associates the attribute with the type
 * itself, while placing it after a property associates it with the property.
 *
 * Only valid inside a SEOUL_BEGIN_TYPE()/SEOUL_END_TYPE() block or a SEOUL_BEGIN_TEMPLATE_TYPE()/SEOUL_END_TYPE() block.
 *
 * The attribute will be associated with either the type, a property, or a method depending on its position. Place it
 * immediately after the type block start to associate it with the type, immediately after a property to associate it with
 * the property, or immediately after a method to associate it with the method.
 */
#define SEOUL_ATTRIBUTE(attribute_name, ...) \
	.AddAttribute(SEOUL_NEW(MemoryBudgets::Reflection) Reflection::Attributes::attribute_name(__VA_ARGS__))

/**
 * Like SEOUL_ATTRIBUTE, but associates the attribute with a particular method argument
 * (0-based index) after creation.
 */
#define SEOUL_ARG_ATTRIBUTE(arg, attribute_name, ...) \
	.AddAttribute((SEOUL_NEW(MemoryBudgets::Reflection) Reflection::Attributes::attribute_name(__VA_ARGS__))->SetArg((arg)))

/**
 * Macros to define a method of a type.
 *
 * Only valid inside a SEOUL_BEGIN_TYPE()/SEOUL_END_TYPE() block or a SEOUL_BEGIN_TEMPLATE_TYPE()/SEOUL_END_TYPE() block.
 */
#define SEOUL_METHOD_N(method_name, method) \
	.AddMethod(Reflection::MethodDetail::Bind(&ReflectionType::method).template Apply< ReflectionType, &ReflectionType::method >(method_name))
#define SEOUL_METHOD(method) SEOUL_METHOD_N(#method, method)

/**
 * Add an immediate parent to the type.
 *
 * Only valid inside a SEOUL_BEGIN_TYPE()/SEOUL_END_TYPE() block or a SEOUL_BEGIN_TEMPLATE_TYPE()/SEOUL_END_TYPE() block.
 */
#define SEOUL_PARENT(parent_type) \
	.AddParent<ReflectionType, parent_type>()

/**
 * Macros to define a property of a type.
 *
 * Only valid inside a SEOUL_BEGIN_TYPE()/SEOUL_END_TYPE() block or a SEOUL_BEGIN_TEMPLATE_TYPE()/SEOUL_END_TYPE() block.
 */

	// SEOUL_PROPERTY_N() and SEOUL_PROPERTY() use MakeTypicalFieldProperty, which is a lighter
	// weight variation of the property infrastructure. It significantly reduces compilation times
	// compared to the full infrastructure used by other property macros (runtime cost is unchanged).
	//
	// Prefer SEOUL_PROPERTY_N() and SEOUL_PROPERTY() when possible (when a property is a read/write
	// field with no property flags). For special cases (getter only methods, properties with flags),
	// SEOUL_PROPERTY_N_EXT() and SEOUL_PROPERTY_EXT() can be used to engage the full property infrastructure
	// on single argument properties.
#define SEOUL_PROPERTY_N(prop_name, prop, ...) \
	.AddProperty(Reflection::PropertyDetail::MakeTypicalFieldProperty(HString(CStringLiteral(prop_name)), &ReflectionType::prop, ##__VA_ARGS__))

#define SEOUL_PROPERTY(prop, ...) \
	.AddProperty(Reflection::PropertyDetail::MakeTypicalFieldProperty(HString(CStringLiteral(#prop)), &ReflectionType::prop, ##__VA_ARGS__))

#define SEOUL_PROPRETY_N_Q_S(prop_name, qualified_prop, signature, ...) \
	.AddProperty(Reflection::PropertyDetail::Bind(prop_name, (signature)qualified_prop).template Apply<qualified_prop, ##__VA_ARGS__>())

#define SEOUL_PROPERTY_N_Q(prop_name, qualified_prop, ...) \
	.AddProperty(Reflection::PropertyDetail::Bind(prop_name, qualified_prop).template Apply<qualified_prop, ##__VA_ARGS__>())

#define SEOUL_PROPERTY_N_EXT(prop_name, prop, ...) \
	.AddProperty(Reflection::PropertyDetail::Bind(prop_name, &ReflectionType::prop).template Apply<&ReflectionType::prop, ##__VA_ARGS__>())

#define SEOUL_PROPERTY_EXT(prop, ...) \
	.AddProperty(Reflection::PropertyDetail::Bind(#prop, &ReflectionType::prop).template Apply<&ReflectionType::prop, ##__VA_ARGS__>())

#define SEOUL_PROPERTY_PAIR_N_Q(prop_name, qualified_prop_get, qualified_prop_set, ...) \
	.AddProperty(Reflection::PropertyDetail::Bind(prop_name, qualified_prop_get, qualified_prop_set).template Apply<qualified_prop_get, qualified_prop_set, ##__VA_ARGS__>())

#define SEOUL_PROPERTY_PAIR_N(prop_name, prop_get, prop_set, ...) \
	.AddProperty(Reflection::PropertyDetail::Bind(prop_name, &ReflectionType::prop_get, &ReflectionType::prop_set).template Apply<&ReflectionType::prop_get, &ReflectionType::prop_set, ##__VA_ARGS__>())

#define SEOUL_PROPERTY_PAIR(prop_base_name, ...) \
	.AddProperty(Reflection::PropertyDetail::Bind(#prop_base_name, &ReflectionType::Get##prop_base_name, &ReflectionType::Set##prop_base_name).template Apply<&ReflectionType::Get##prop_base_name, &ReflectionType::Set##prop_base_name, ##__VA_ARGS__>())

// Macro variations to use for reflection capabilities that should not be included in ship builds - for
// example, elements only used for debugging or to define editor properties (i.e. a description attribute).

#if !SEOUL_SHIP
#define SEOUL_DEV_ONLY_ATTRIBUTE(attribute_name, ...) SEOUL_ATTRIBUTE(attribute_name, ##__VA_ARGS__)

#define SEOUL_DEV_ONLY_METHOD_N(method_name, method) SEOUL_METHOD_N(method_name, method)
#define SEOUL_DEV_ONLY_METHOD(method) SEOUL_METHOD(method)

#define SEOUL_DEV_ONLY_PROPRETY_N_Q_S(prop_name, qualified_prop, signature, ...) SEOUL_PROPRETY_N_Q_S(prop_name, qualified_prop, signature, ##__VA_ARGS__)
#define SEOUL_DEV_ONLY_PROPERTY_N_Q(prop_name, qualified_prop, ...) SEOUL_PROPERTY_N_Q(prop_name, qualified_prop, ##__VA_ARGS__)
#define SEOUL_DEV_ONLY_PROPERTY_N(prop_name, prop, ...) SEOUL_PROPERTY_N(prop_name, prop, ##__VA_ARGS__)
#define SEOUL_DEV_ONLY_PROPERTY(prop, ...) SEOUL_PROPERTY(prop, ##__VA_ARGS__)
#define SEOUL_DEV_ONLY_PROPERTY_PAIR_N(prop_name, prop_get, prop_set, ...) SEOUL_PROPERTY_PAIR_N(prop_name, prop_get, prop_set, ##__VA_ARGS__)
#define SEOUL_DEV_ONLY_PROPERTY_PAIR(prop_base_name, ...) SEOUL_PROPERTY_PAIR(prop_base_name, ##__VA_ARGS__)
#else
#define SEOUL_DEV_ONLY_ATTRIBUTE(attribute_name, ...)

#define SEOUL_DEV_ONLY_METHOD_N(method_name, method)
#define SEOUL_DEV_ONLY_METHOD(method)

#define SEOUL_DEV_ONLY_PROPRETY_N_Q_S(prop_name, qualified_prop, signature, ...)
#define SEOUL_DEV_ONLY_PROPERTY_N_Q(prop_name, qualified_prop, ...)
#define SEOUL_DEV_ONLY_PROPERTY_N(prop_name, prop, ...)
#define SEOUL_DEV_ONLY_PROPERTY(prop, ...)
#define SEOUL_DEV_ONLY_PROPERTY_PAIR_N(prop_name, prop_get, prop_set, ...)
#define SEOUL_DEV_ONLY_PROPERTY_PAIR(prop_base_name, ...)
#endif

} // namespace Seoul

#endif // include guard
