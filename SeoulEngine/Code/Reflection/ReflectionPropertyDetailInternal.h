/**
 * \file ReflectionPropertyDetailInternal.h
 * \brief Internal header used by ReflectionPropertyDetail.h - should not be included
 * by any other file.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#ifndef REFLECTION_PROPERTY_DETAIL_H
#error ReflectionPropertyDetailInternal.h should only be included by ReflectionPropertyDetail.h
#endif

#if !defined(SEOUL_PROP_1_SIG) || !defined(SEOUL_PROP_NAME)
#   error "Please define all necessary macros before including this file."
#endif

#define SEOUL_DETAIL_NAMESPACE SEOUL_MACRO_JOIN(SEOUL_PROP_NAME, ImplDetail)
#if defined(SEOUL_PROP_2_SIG)
#define SEOUL_TEMPLATE_DECL typename C, typename T, SEOUL_PROP_1_SIG, SEOUL_PROP_2_SIG
#define SEOUL_TEMPLATE_SPEC C, T, P1, P2
#else
#define SEOUL_TEMPLATE_DECL typename C, typename T, SEOUL_PROP_1_SIG
#define SEOUL_TEMPLATE_SPEC C, T, P1
#endif

#if defined(SEOUL_PROP_GET)
#define SEOUL_HAS_GET 1
#else
#define SEOUL_HAS_GET 0
#endif

#if defined(SEOUL_PROP_SET)
#define SEOUL_HAS_SET 1
#else
#define SEOUL_HAS_SET 0
#endif

#if defined(SEOUL_PROP_GET_PTR)
#define SEOUL_HAS_GET_PTR 1
#else
#define SEOUL_HAS_GET_PTR 0
#endif

namespace SEOUL_DETAIL_NAMESPACE
{
	template <SEOUL_TEMPLATE_DECL, Bool B_DISABLE_GET>
	struct ImplGet
	{
		static Bool TryGet(const Property& prop, const WeakAny& thisPointer, Any& rValue)
		{
			C const* p = nullptr;
			if (PointerCast(thisPointer, p))
			{
				SEOUL_ASSERT(nullptr != p);
				SEOUL_PROP_GET(p, rValue);
				return true;
			}
			return false;
		}
	};

	template <SEOUL_TEMPLATE_DECL>
	struct ImplGet<SEOUL_TEMPLATE_SPEC, true>
	{
		static Bool TryGet(const Property& prop, const WeakAny& thisPointer, Any& rValue)
		{
			return false;
		}
	};

	template <SEOUL_TEMPLATE_DECL, Bool B_DISABLE_SET>
	struct ImplSet
	{
		static Bool TrySet(const Property& prop, const WeakAny& thisPointer, const WeakAny& value)
		{
			C* p = nullptr;
			if (PointerCast(thisPointer, p))
			{
				SEOUL_ASSERT(nullptr != p);
				typename RemoveConst< typename RemoveReference<T>::Type >::Type val;
				if (TypeConstruct(value, val))
				{
					SEOUL_PROP_SET(p, val);
					return true;
				}
			}

			return false;
		}
	};

	template <SEOUL_TEMPLATE_DECL>
	struct ImplSet<SEOUL_TEMPLATE_SPEC, true>
	{
		static Bool TrySet(const Property& prop, const WeakAny& thisPointer, const WeakAny& value)
		{
			return false;
		}
	};

	template <SEOUL_TEMPLATE_DECL, Bool B_DISABLE_GET_PTR>
	struct ImplGetPtr
	{
		static Bool TryGetPtr(const Property& prop, const WeakAny& thisPointer, WeakAny& rValue)
		{
			C* p = nullptr;
			if (PointerCast(thisPointer, p))
			{
				SEOUL_ASSERT(nullptr != p);
				SEOUL_PROP_GET_PTR(p, rValue);
				return true;
			}
			return false;
		}
	};

	template <SEOUL_TEMPLATE_DECL>
	struct ImplGetPtr<SEOUL_TEMPLATE_SPEC, true>
	{
		static Bool TryGetPtr(const Property& prop, const WeakAny& thisPointer, WeakAny& rValue)
		{
			return false;
		}
	};

	template <SEOUL_TEMPLATE_DECL, Bool B_DISABLE_GET_PTR>
	struct ImplGetConstPtr
	{
		static Bool TryGetConstPtr(const Property& prop, const WeakAny& thisPointer, WeakAny& rValue)
		{
			C const* p = nullptr;
			if (PointerCast(thisPointer, p))
			{
				SEOUL_ASSERT(nullptr != p);
				SEOUL_PROP_GET_PTR(p, rValue);
				return true;
			}
			return false;
		}
	};

	template <SEOUL_TEMPLATE_DECL>
	struct ImplGetConstPtr<SEOUL_TEMPLATE_SPEC, true>
	{
		static Bool TryGetConstPtr(const Property& prop, const WeakAny& thisPointer, WeakAny& rValue)
		{
			return false;
		}
	};
} // namespace SEOUL_DETAIL_NAMESPACE

template <typename C, typename T>
struct SEOUL_PROP_NAME
{
	template <size_t zStringArrayLengthInBytes>
	SEOUL_PROP_NAME(Byte const (&sName)[zStringArrayLengthInBytes])
		: m_Name(CStringLiteral(sName))
	{
	}

#if defined(SEOUL_PROP_2_SIG)
	template <SEOUL_PROP_1_SIG, SEOUL_PROP_2_SIG>
	inline Property* Apply()
	{
		static const Bool kbCanGet = (0 == SEOUL_HAS_GET);
		static const Bool kbCanSet = (0 == SEOUL_HAS_SET);
		static const Bool kbCanGetPtr = (0 == SEOUL_HAS_SET || 0 == SEOUL_HAS_GET || 0 == SEOUL_HAS_GET_PTR);
		static const Bool kbCanGetConstPtr = (0 == SEOUL_HAS_GET || 0 == SEOUL_HAS_GET_PTR);

		UInt32 uFlags = 0u;
#if (0 == SEOUL_HAS_GET)
		uFlags |= PropertyFlags::kDisableGet
#endif
#if (0 == SEOUL_HAS_SET)
		uFlags |= PropertyFlags::kDisableSet
#endif

		return SEOUL_NEW(MemoryBudgets::Reflection) Property(
			m_Name,
			TypeInfoDetail::TypeInfoImpl<T>::Get(),
			SEOUL_DETAIL_NAMESPACE::ImplGet<SEOUL_TEMPLATE_SPEC, kbCanGet>::TryGet,
			SEOUL_DETAIL_NAMESPACE::ImplSet<SEOUL_TEMPLATE_SPEC, kbCanSet>::TrySet,
			SEOUL_DETAIL_NAMESPACE::ImplGetPtr<SEOUL_TEMPLATE_SPEC, kbCanGetPtr>::TryGetPtr,
			SEOUL_DETAIL_NAMESPACE::ImplGetConstPtr<SEOUL_TEMPLATE_SPEC, kbCanGetConstPtr>::TryGetConstPtr,
			uFlags);
	}

	template <SEOUL_PROP_1_SIG, SEOUL_PROP_2_SIG, Int FLAGS>
	inline Property* Apply()
	{
		static const Bool kbCanGet = (0 == SEOUL_HAS_GET || (0 != (PropertyFlags::kDisableGet & FLAGS)));
		static const Bool kbCanSet = (0 == SEOUL_HAS_SET || (0 != (PropertyFlags::kDisableSet & FLAGS)));
		static const Bool kbCanGetPtr = (0 == SEOUL_HAS_SET || 0 == SEOUL_HAS_GET || 0 == SEOUL_HAS_GET_PTR);
		static const Bool kbCanGetConstPtr = (0 == SEOUL_HAS_GET || 0 == SEOUL_HAS_GET_PTR);

		UInt32 uFlags = FLAGS;
#if (0 == SEOUL_HAS_GET)
		uFlags |= PropertyFlags::kDisableGet
#endif
#if (0 == SEOUL_HAS_SET)
		uFlags |= PropertyFlags::kDisableSet
#endif

		return SEOUL_NEW(MemoryBudgets::Reflection) Property(
			m_Name,
			TypeInfoDetail::TypeInfoImpl<C>::Get(),
			TypeInfoDetail::TypeInfoImpl<T>::Get(),
			SEOUL_DETAIL_NAMESPACE::ImplGet<SEOUL_TEMPLATE_SPEC, kbCanGet>::TryGet,
			SEOUL_DETAIL_NAMESPACE::ImplSet<SEOUL_TEMPLATE_SPEC, kbCanSet>::TrySet,
			SEOUL_DETAIL_NAMESPACE::ImplGetPtr<SEOUL_TEMPLATE_SPEC, kbCanGetPtr>::TryGetPtr,
			SEOUL_DETAIL_NAMESPACE::ImplGetConstPtr<SEOUL_TEMPLATE_SPEC, kbCanGetConstPtr>::TryGetConstPtr,
			uFlags);
	}
#else
	template <SEOUL_PROP_1_SIG>
	inline Property* Apply()
	{
		static const Bool kbCanGet = (0 == SEOUL_HAS_GET);
		static const Bool kbCanSet = (0 == SEOUL_HAS_SET);
		static const Bool kbCanGetPtr = (0 == SEOUL_HAS_SET || 0 == SEOUL_HAS_GET || 0 == SEOUL_HAS_GET_PTR);
		static const Bool kbCanGetConstPtr = (0 == SEOUL_HAS_GET || 0 == SEOUL_HAS_GET_PTR);

		UInt32 uFlags = 0u;
#if (0 == SEOUL_HAS_GET)
		uFlags |= PropertyFlags::kDisableGet;
#endif
#if (0 == SEOUL_HAS_SET)
		uFlags |= PropertyFlags::kDisableSet;
#endif

		return SEOUL_NEW(MemoryBudgets::Reflection) Property(
			m_Name,
			TypeInfoDetail::TypeInfoImpl<T>::Get(),
			SEOUL_DETAIL_NAMESPACE::ImplGet<SEOUL_TEMPLATE_SPEC, kbCanGet>::TryGet,
			SEOUL_DETAIL_NAMESPACE::ImplSet<SEOUL_TEMPLATE_SPEC, kbCanSet>::TrySet,
			SEOUL_DETAIL_NAMESPACE::ImplGetPtr<SEOUL_TEMPLATE_SPEC, kbCanGetPtr>::TryGetPtr,
			SEOUL_DETAIL_NAMESPACE::ImplGetConstPtr<SEOUL_TEMPLATE_SPEC, kbCanGetConstPtr>::TryGetConstPtr,
			uFlags);
	}

	template <SEOUL_PROP_1_SIG, Int FLAGS>
	inline Property* Apply()
	{
		static const Bool kbCanGet = (0 == SEOUL_HAS_GET || (0 != (PropertyFlags::kDisableGet & FLAGS)));
		static const Bool kbCanSet = (0 == SEOUL_HAS_SET || (0 != (PropertyFlags::kDisableSet & FLAGS)));
		static const Bool kbCanGetPtr = (0 == SEOUL_HAS_SET || 0 == SEOUL_HAS_GET || 0 == SEOUL_HAS_GET_PTR);
		static const Bool kbCanGetConstPtr = (0 == SEOUL_HAS_GET || 0 == SEOUL_HAS_GET_PTR);

		UInt32 uFlags = FLAGS;
#if (0 == SEOUL_HAS_GET)
		uFlags |= PropertyFlags::kDisableGet;
#endif
#if (0 == SEOUL_HAS_SET)
		uFlags |= PropertyFlags::kDisableSet;
#endif

		return SEOUL_NEW(MemoryBudgets::Reflection) Property(
			m_Name,
			TypeInfoDetail::TypeInfoImpl<T>::Get(),
			SEOUL_DETAIL_NAMESPACE::ImplGet<SEOUL_TEMPLATE_SPEC, kbCanGet>::TryGet,
			SEOUL_DETAIL_NAMESPACE::ImplSet<SEOUL_TEMPLATE_SPEC, kbCanSet>::TrySet,
			SEOUL_DETAIL_NAMESPACE::ImplGetPtr<SEOUL_TEMPLATE_SPEC, kbCanGetPtr>::TryGetPtr,
			SEOUL_DETAIL_NAMESPACE::ImplGetConstPtr<SEOUL_TEMPLATE_SPEC, kbCanGetConstPtr>::TryGetConstPtr,
			uFlags);
	}
#endif

	HString m_Name;
};

#if defined(SEOUL_PROP_2_SIG)
template <typename C, typename T, size_t zStringArrayLengthInBytes>
static inline SEOUL_PROP_NAME<C, T> Bind(Byte const (&sName)[zStringArrayLengthInBytes], SEOUL_PROP_1_SIG, SEOUL_PROP_2_SIG)
{
	return SEOUL_PROP_NAME<C, T>(sName);
}
#else
template <typename C, typename T, size_t zStringArrayLengthInBytes>
static inline SEOUL_PROP_NAME<C, T> Bind(Byte const (&sName)[zStringArrayLengthInBytes], SEOUL_PROP_1_SIG)
{
	return SEOUL_PROP_NAME<C, T>(sName);
}
#endif

#undef SEOUL_HAS_GET_PTR
#undef SEOUL_HAS_SET
#undef SEOUL_HAS_GET

#undef SEOUL_TEMPLATE_SPEC
#undef SEOUL_TEMPLATE_DECL
#undef SEOUL_DETAIL_NAMESPACE

#undef SEOUL_PROP_NAME
#undef SEOUL_PROP_1_SIG

#if defined(SEOUL_PROP_2_SIG)
#	undef SEOUL_PROP_2_SIG
#endif

#if defined(SEOUL_PROP_GET)
#undef SEOUL_PROP_GET
#endif

#if defined(SEOUL_PROP_SET)
#undef SEOUL_PROP_SET
#endif

#if defined(SEOUL_PROP_GET_PTR)
#undef SEOUL_PROP_GET_PTR
#endif
