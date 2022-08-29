/**
 * \file ReflectionTypeInfo.h
 * \brief SeoulEngine type info, used in the Reflection namespace. Equivalent in scope
 * and usage to std::type_info.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_TYPE_INFO_H
#define REFLECTION_TYPE_INFO_H

#include "Pair.h"
#include "Prereqs.h"
#include "ScopedPtr.h"
#include "SeoulHString.h"
#include "SeoulString.h"
#include "SeoulTypeTraits.h"

namespace Seoul
{

// Forward declarations
namespace Reflection
{
	class Array;
	class Enum;
	class Table;
	class Type;
	class WeakAny;

	template <typename T>
	struct ArrayOfDetail
	{
		static const Reflection::Array& Get();
	};

	template <typename T>
	struct ArrayOfDetailStaticOwner
	{
		static const Reflection::Array& s_kStaticType;
	};

	template <typename T>
	const Reflection::Array& ArrayOfDetailStaticOwner<T>::s_kStaticType(Reflection::ArrayOfDetail<T>::Get());

	template <typename T>
	struct EnumOfDetail
	{
		static const Reflection::Enum& Get();
	};

	// NOTE: We don't inline define the definition of s_kStaticType here
	// but instead in the body of SEOUL_BEGIN_ENUM(). This is to workaround
	// an (apparent) bug (?) starting with Visual Studio 2017 v15.3.0.
	//
	// If we use templated definition, the definition was sometimes
	// be entirely elided, resulting in a link-time failure.
	//
	// See ReflectionDefine.h.
	template <typename T>
	struct EnumOfDetailStaticOwner
	{
		static const Reflection::Enum& s_kStaticType;
	};

	template <typename T>
	struct TableOfDetail
	{
		static const Reflection::Table& Get();
	};

	template <typename T>
	struct TableOfDetailStaticOwner
	{
		static const Reflection::Table& s_kStaticType;
	};

	template <typename T>
	const Reflection::Table& TableOfDetailStaticOwner<T>::s_kStaticType(Reflection::TableOfDetail<T>::Get());

#if !SEOUL_IMPLICIT_TEMPLATED_REFLECTION_DEFINITION
	template <typename T>
	struct TemplateTypeOfDetail
	{
		static const Reflection::Type& Get();
	};
#endif

	template <typename T>
	struct TypeOfDetail
	{
		static const Reflection::Type& Get();
	};

	// NOTE: We don't inline define the definition of s_kStaticType here
	// but instead in the body of SEOUL_BEGIN_TYPE() and
	// SEOUL_BEGIN_TEMPLATE_TYPE(). This is to workaround
	// an (apparent) bug (?) starting with Visual Studio 2017 v15.3.0.
	//
	// If we use templated definition, the definition was sometimes
	// be entirely elided, resulting in a link-time failure.
	//
	// See ReflectionDefine.h.

	template <typename T>
	struct TypeOfDetailStaticOwner
	{
		static const Reflection::Type& s_kStaticType;
	};

} // namespace Reflection

template <typename T>
static const Reflection::Array& ArrayOf()
{
	// Sanity check - this cannot be used in any static initialization
	// code - call Reflection::ArrayOfDetail<T>::Get() instead.
	SEOUL_ASSERT(IsInMainFunction());

	typedef typename RemoveAllCvReferencePointer<T>::Type UnqualifiedType;

	return Reflection::ArrayOfDetailStaticOwner<UnqualifiedType>::s_kStaticType;
}

template <typename T>
static const Reflection::Enum& EnumOf()
{
	// Sanity check - this cannot be used in any static initialization
	// code - call Reflection::EnumOfDetail<T>::Get() instead.
	SEOUL_ASSERT(IsInMainFunction());

	typedef typename RemoveAllCvReferencePointer<T>::Type UnqualifiedType;

	return Reflection::EnumOfDetailStaticOwner<UnqualifiedType>::s_kStaticType;
}

template <typename T>
static const Reflection::Table& TableOf()
{
	// Sanity check - this cannot be used in any static initialization
	// code - call Reflection::TableOfDetail<T>::Get() instead.
	SEOUL_ASSERT(IsInMainFunction());

	typedef typename RemoveAllCvReferencePointer<T>::Type UnqualifiedType;

	return Reflection::TableOfDetailStaticOwner<UnqualifiedType>::s_kStaticType;
}

template <typename T>
static const Reflection::Type& TypeOf()
{
	// Sanity check - this cannot be used in any static initialization
	// code - call Reflection::TypeOfDetail<T>::Get() instead.
	SEOUL_ASSERT(IsInMainFunction());

	typedef typename RemoveAllCvReferencePointer<T>::Type UnqualifiedType;

	return Reflection::TypeOfDetailStaticOwner<UnqualifiedType>::s_kStaticType;
}

/**
 * Utility structure used to detect whether a type T implements the
 * GetReflectionThis() const and GetReflectionThis() member functions. If these functions
 * are present, then polmorphism of the type is supported in contexts such as deserialization
 * or type enumeration. Otherwise, the reflection's knowledge of the type will be limited
 * to the concrete type that is used when the type pointer is wrapped in a WeakAny.
 */
template <typename T>
struct IsReflectionPolymorphic
{
private:
	typedef Byte True[1];
	typedef Byte False[2];

	template <typename U, U MEMBER_FUNC> struct Tester {};

	template <typename U>
	static True& TestForGetReflectionThisConst(Tester< Reflection::WeakAny (U::*)() const, &U::GetReflectionThis >*);

	template <typename U>
	static False& TestForGetReflectionThisConst(...);

	template <typename U>
	static True& TestForGetReflectionThis(Tester< Reflection::WeakAny (U::*)(), &U::GetReflectionThis >*);

	template <typename U>
	static False& TestForGetReflectionThis(...);

public:
	static const Bool Value = (
		sizeof(TestForGetReflectionThisConst<T>(nullptr)) == sizeof(True) &&
		sizeof(TestForGetReflectionThis<T>(nullptr)) == sizeof(True));
}; // struct FulfillsReflectionPolymorphicContract

namespace Reflection
{

/**
 * SimpleTypeInfo is used to identify a handful of type classes
 * quickly and cheaply - most of these classes are built-in types,
 * but a handful are standard Seoul engine types (i.e. String).
 */
enum class SimpleTypeInfo
{
	kBoolean,
	kComplex,
	kCString,
	kEnum,
	kInt8,
	kInt16,
	kInt32,
	kInt64,
	kFloat32,
	kFloat64,
	kHString,
	kString,
	kUInt8,
	kUInt16,
	kUInt32,
	kUInt64,
};

namespace TraitFlags
{
	/**
	 * Flags used to associate various type features with a TypeInfo instance.
	 */
	enum
	{
		/** Type has no special features. */
		kNoFlags = 0u,

		/** The type has a const modifier. */
		kConstant = (1u << 0u),

		/** The type has a const modifier, after removing const and pointer on the type. */
		kInnerConstant = (1u << 1u),

		/** The type is a pointer type. */
		kPointer = (1u << 2u),

		/** The type is a reference type. */
		kReference = (1u << 3u),

		/** The type supports reflection polymorphism (implements GetReflectionThis()). */
		kReflectionPolymorphic = (1u << 4u),

		/** The type is void (void, void const, void volatile, void const volatile) */
		kVoid = (1u << 5u),
	};
}

/**
 * TypeInfo is a complex data structure that includes generic
 * information about a type, such as its size, alignment, a handful
 * of trait flags, and its SimpleTypeInfo. A Reflection::Type
 * object for the Type can also be acquired, which can then be used
 * to perform more powerful reflection of a type.
 */
class TypeInfo SEOUL_SEALED
{
public:
	typedef const Reflection::Type& (*GetTypeFunc)();

	TypeInfo(
		UInt32 zAlignmentInBytes,
		UInt32 uTraitFlags,
		SimpleTypeInfo eSimpleTypeInfo,
		UInt32 zTypeSizeInBytes,
		GetTypeFunc getType)
		: m_zAlignmentInBytes(zAlignmentInBytes)
		, m_uTraitFlags(uTraitFlags)
		, m_eSimpleTypeInfo(eSimpleTypeInfo)
		, m_zSizeInBytes(zTypeSizeInBytes)
		, m_GetType(getType)
	{
	}

	~TypeInfo()
	{
	}

	Bool operator==(const TypeInfo& b) const
	{
		return (this == &b);
	}

	Bool operator!=(const TypeInfo& b) const
	{
		return !(*this == b);
	}

	/**
	 * @return True if the type described by this TypeInfo has a const modifier.
	 */
	Bool IsConstant() const
	{
		return (TraitFlags::kConstant == (m_uTraitFlags & TraitFlags::kConstant));
	}

	/**
	 * @return True if the type described by this TypeInfo has a const modifier,
	 * after removing any constant and pointer modifier.
	 */
	Bool IsInnerConstant() const
	{
		return (TraitFlags::kInnerConstant == (m_uTraitFlags & TraitFlags::kInnerConstant));
	}

	/**
	 * @return True if the type described by this TypeInfo is a pointer type.
	 */
	Bool IsPointer() const
	{
		return (TraitFlags::kPointer == (m_uTraitFlags & TraitFlags::kPointer));
	}

	/**
	 * @return True if the type described by this TypeInfo is a reference type.
	 */
	Bool IsReference() const
	{
		return (TraitFlags::kReference == (m_uTraitFlags & TraitFlags::kReference));
	}

	/**
	 * @return True if the type described by this TypeInfo can be manipulated
	 * as polymorphic by the reflection system (it defines GetReflectionThis()), false
	 * otherwise.
	 */
	Bool IsReflectionPolymorphic() const
	{
		return (TraitFlags::kReflectionPolymorphic == (m_uTraitFlags & TraitFlags::kReflectionPolymorphic));
	}

	/**
	 * @return True if the type described by this TypeInfo is void (void,
	 * void const, void volatile, void const volatile).
	 */
	Bool IsVoid() const
	{
		return (TraitFlags::kVoid == (m_uTraitFlags & TraitFlags::kVoid));
	}

	/**
	 * @return Alignment of the type described by this TypeInfo.
	 */
	UInt32 GetAlignmentInBytes() const
	{
		return m_zAlignmentInBytes;
	}

	/**
	 * @return A SimpleTypeInfo enum value that represents the type
	 * described by this TypeInfo.
	 */
	SimpleTypeInfo GetSimpleTypeInfo() const
	{
		return m_eSimpleTypeInfo;
	}

	/**
	 * @return Size of the type described by this TypeInfo.
	 */
	UInt32 GetSizeInBytes() const
	{
		return m_zSizeInBytes;
	}

	// Get a Reflection::Type object that can be used for
	// more powerful reflection operations on the type
	// described by this TypeInfo object.
	const Reflection::Type& GetType() const
	{
		return m_GetType();
	}

private:
	UInt32 const m_zAlignmentInBytes;
	UInt32 const m_uTraitFlags;
	SimpleTypeInfo const m_eSimpleTypeInfo;
	UInt32 const m_zSizeInBytes;
	GetTypeFunc const m_GetType;

	SEOUL_DISABLE_COPY(TypeInfo);
}; // class TypeInfo

namespace TypeInfoDetail
{

/**
 * @return All trait flags associated with the type T.
 */
template <typename T>
inline UInt32 GetTraitFlags()
{
	typedef typename RemovePointer< typename RemoveReference< typename RemoveConstVolatile<T>::Type >::Type >::Type InnerType;

	UInt32 uReturn = 0u;
	uReturn |= (Seoul::IsConst<T>::Value ? TraitFlags::kConstant : 0u);
	uReturn |= (Seoul::IsConst<InnerType>::Value ? TraitFlags::kInnerConstant : 0u);
	uReturn |= (Seoul::IsPointer<T>::Value ? TraitFlags::kPointer : 0u);
	uReturn |= (Seoul::IsReference<T>::Value ? TraitFlags::kReference : 0u);
	uReturn |= (Seoul::IsReflectionPolymorphic<T>::Value ? TraitFlags::kReflectionPolymorphic : 0u);
	uReturn |= (Seoul::IsVoid<T>::Value ? TraitFlags::kVoid : 0u);

	return uReturn;
}

/**
 * Used to get the alignment of a type - default implementation does
 * not returna  valid alignment (0u).
 */
template <typename T, Bool B_IS_VOID_OR_ABSTRACT>
struct AlignmentHelper
{
	static UInt32 GetAlignmentOf() { return 0u; }
};

/**
 * Partial specialization of AlignmentHelper for non-void, non-abstract
 * types.
 */
template <typename T>
struct AlignmentHelper<T, false>
{
	static UInt32 GetAlignmentOf() { return SEOUL_ALIGN_OF(T); }
};

template <typename T>
struct SimpleTypeInfoT
{
	static const SimpleTypeInfo Value = IsEnum<T>::Value ? SimpleTypeInfo::kEnum : SimpleTypeInfo::kComplex;
};

template <> struct SimpleTypeInfoT<Bool> { static const SimpleTypeInfo Value = SimpleTypeInfo::kBoolean; };
template <> struct SimpleTypeInfoT<Byte const*> { static const SimpleTypeInfo Value = SimpleTypeInfo::kCString; };
template <> struct SimpleTypeInfoT<Int8> { static const SimpleTypeInfo Value = SimpleTypeInfo::kInt8; };
template <> struct SimpleTypeInfoT<Int16> { static const SimpleTypeInfo Value = SimpleTypeInfo::kInt16; };
template <> struct SimpleTypeInfoT<Int32> { static const SimpleTypeInfo Value = SimpleTypeInfo::kInt32; };
template <> struct SimpleTypeInfoT<Int64> { static const SimpleTypeInfo Value = SimpleTypeInfo::kInt64; };
template <> struct SimpleTypeInfoT<Float32> { static const SimpleTypeInfo Value = SimpleTypeInfo::kFloat32; };
template <> struct SimpleTypeInfoT<Float64> { static const SimpleTypeInfo Value = SimpleTypeInfo::kFloat64; };
template <> struct SimpleTypeInfoT<HString> { static const SimpleTypeInfo Value = SimpleTypeInfo::kHString; };
template <> struct SimpleTypeInfoT<const HString&> { static const SimpleTypeInfo Value = SimpleTypeInfo::kHString; };
template <> struct SimpleTypeInfoT<String> { static const SimpleTypeInfo Value = SimpleTypeInfo::kString; };
template <> struct SimpleTypeInfoT<const String&> { static const SimpleTypeInfo Value = SimpleTypeInfo::kString; };
template <> struct SimpleTypeInfoT<UInt8> { static const SimpleTypeInfo Value = SimpleTypeInfo::kUInt8; };
template <> struct SimpleTypeInfoT<UInt16> { static const SimpleTypeInfo Value = SimpleTypeInfo::kUInt16; };
template <> struct SimpleTypeInfoT<UInt32> { static const SimpleTypeInfo Value = SimpleTypeInfo::kUInt32; };
template <> struct SimpleTypeInfoT<UInt64> { static const SimpleTypeInfo Value = SimpleTypeInfo::kUInt64; };

/** @return The size of a type - specialization for void, sizeof(void) is illegal. */
template <typename T> inline UInt32 GetSizeOf() { return sizeof(T); }
template <> inline UInt32 GetSizeOf<void>() { return 0u; }

template <typename T>
struct TypeInfoImpl
{
	static const Reflection::TypeInfo& Get()
	{
		static const TypeInfo s_kStaticType(
			TypeInfoDetail::AlignmentHelper<T, Seoul::IsAbstract<T>::Value || Seoul::IsVoid<T>::Value>::GetAlignmentOf(),
			TypeInfoDetail::GetTraitFlags<T>(),
			(IsEnum<T>::Value ? SimpleTypeInfo::kEnum : SimpleTypeInfoT<T>::Value),
			TypeInfoDetail::GetSizeOf<T>(),
			TypeOf<T>);
		return s_kStaticType;
	}
};

template <typename T>
struct TypeInfoImplStaticOwner
{
	static const TypeInfo& s_kStaticType;
};

template <typename T>
const TypeInfo& TypeInfoImplStaticOwner<T>::s_kStaticType(TypeInfoImpl<T>::Get());

} // namespace TypeInfoDetail

} // namespace Reflection

template <typename T>
inline Reflection::SimpleTypeInfo SimpleTypeId()
{
	return Reflection::TypeInfoDetail::SimpleTypeInfoT<T>::Value;
}

template <typename T>
inline const Reflection::TypeInfo& TypeId()
{
	// Sanity check - this cannot be used in any static initialization
	// code - call Reflection::TypeInfoDetail::TypeInfoDetail<T>::Get() instead.
	SEOUL_ASSERT(IsInMainFunction());

	return Reflection::TypeInfoDetail::TypeInfoImplStaticOwner<T>::s_kStaticType;
}

} // namespace Seoul

#endif // include guard
