/**
 * \file Delegate.h
 * \brief Delegate represents a generic functor that handles both function pointers and member function
 * pointers. Similar in concept and usage to C# delegate, but stack allocated and with clumsier (by
 * necessity) syntax.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef DELEGATE_H
#define DELEGATE_H

#include "DelegateMemberBindHandle.h"
#include "Prereqs.h"

namespace Seoul
{

// Forward declaration of the Delegate subclass, used in a few places in DelegateImpl
template <typename F>
class Delegate;
template <typename R, typename... TYPES>
class Delegate<R(TYPES...)>;

/**
 * "Detail" base class of Delegate - required to implement the Delegate<void(void)> syntax.
 * Intentionally not polymorphic - we don't want the overhead of a vtable pointer, partially
 * defeats the purposes of the Delegate<> type. This is not a problem in practice, as
 * DelegateImpl has a trivial destructor and cannot be instantiated directly anyway.
 */
template <typename R, typename... TYPES>
class DelegateImpl SEOUL_ABSTRACT
{
public:
	/**
	 * Signature of the wrapper function - for the various possible
	 * invokes that a delegate can support (global function, global function with
	 * implicit void* argument, member function), a wrapper global function is generated
	 * using templates. This function applies its arguments to the target function
	 * of the Delegate<> as needed.
	 */
	typedef R(*Caller)(void* pObject, TYPES...);

	/**
	 * @return Internal caller function pointer used when resolving the delegate.
	 */
	Caller GetCaller() const
	{
		return m_pCaller;
	}

	/**
	 * @return Internal object used when resolving the delegate - can be
	 * nullptr if the internal function pointer is a global function pointer
	 * and not a member function pointer.
	 */
	void* GetObject() const
	{
		return m_pObject;
	}

private:
	friend class Delegate<R(TYPES...)>;
	/** Caller function - must be non-null for this DelegateImpl to be invokable. */
	Caller m_pCaller;

	/** Target object - can be null (for wrapper global functions). */
	void* m_pObject;

	/** Prevent DelegateImpl from being constructed directly. */
	DelegateImpl()
		: m_pCaller(nullptr)
		, m_pObject(nullptr)
	{}

	/** Constructor to instantiate a DelegateImpl from raw wrapper function and object. */
	DelegateImpl(Caller pCaller, void* pObject)
		: m_pCaller(pCaller)
		, m_pObject(pObject)
	{}
};

// Make the Delegate base definition a little cleaner to work with.
// TEMP: #define SEOUL_DELEGATE_BASE_CLASS DelegateImpl<R SEOUL_DELEGATE_COMMA SEOUL_DELEGATE_T_ARGS>

template <typename R, typename... TYPES>
class Delegate<R(TYPES...)> : public DelegateImpl<R, TYPES...>
{
public:
	/** Convenience alias for the parent class. */
	using Super = DelegateImpl<R, TYPES...>;
	/** Redefine the Caller signature, workaround some template headaches in older GCC versions. */
	typedef R(*Caller)(void* pObject, TYPES...);

	Delegate()
	{
		// Nop
	}

	/** Construct this Delegate from raw wrapper caller function and target object. */
	Delegate(Caller pCaller, void* pObject)
		: Super(pCaller, pObject)
	{
	}

	Delegate(const Delegate& d)
		: Super(d.m_pCaller, d.m_pObject)
	{
	}

	Delegate(const Super& d)
		: Super(d.m_pCaller, d.m_pObject)
	{
	}

	Delegate& operator=(const Delegate& d)
	{
		Super::m_pCaller = d.m_pCaller;
		Super::m_pObject = d.m_pObject;

		return *this;
	}

	Delegate& operator=(const Super& d)
	{
		Super::m_pCaller = d.m_pCaller;
		Super::m_pObject = d.m_pObject;

		return *this;
	}

	/** Comparison - only true if members are pointer equal. */
	Bool operator==(const Delegate& b) const
	{
		return (
			Super::m_pCaller == b.m_pCaller &&
			Super::m_pObject == b.m_pObject);
	}
	Bool operator!=(const Delegate& b) const
	{
		return !(*this == b);
	}

	/**
	 * @return True if this Delegate can be invoked, false otherwise. Allows
	 * this Delegate object to be directly used in if () checks.
	 */
	explicit operator Bool() const
	{
		return (nullptr != Super::m_pCaller);
	}

	/**
	 * @return True if this Delegate can be invoked, false otherwise. Explicit
	 * version of operator Bool().
	 */
	Bool IsValid() const
	{
		return (nullptr != Super::m_pCaller);
	}

	/**
	 * Invoke this Delegate with appropriate arguments.
	 *
	 * @return The result of the invoke, or nothing if this Delegate is specialized on
	 * a void return type.
	 *
	 * \warning This method will SEOUL_FAIL() if Delegate::IsValid() returns false
	 * for this Delegate.
	 */
	R operator()(TYPES... args) const
	{
		// We want crashes due to dangling 'this' to reliably occur here, so we force
		// a crash, even in Ship.
#if SEOUL_ASSERTIONS_DISABLED

		if ((nullptr == Super::m_pCaller))
		{
			// Trigger a segmentation fault explicitly.
			(*((UInt volatile*)nullptr)) = 1;
		}

#else // !#if SEOUL_ASSERTIONS_DISABLED

		// Extra parentheses due to GCC 4.1.1 having trouble
		// parsing this line otherwise.
		SEOUL_ASSERT((nullptr != Super::m_pCaller));

#endif // /!#if SEOUL_ASSERTIONS_DISABLED

		return (*Super::m_pCaller)(Super::m_pObject, args...);
	}

	/**
	 * Reset this Delegate to the invalid state.
	 */
	void Reset()
	{
		Super::m_pCaller = nullptr;
		Super::m_pObject = nullptr;
	}
};

// Functionality necessary to bind functions of various signatures
// to their corresponding delegate type - facilitates the
// convenient syntax SEOUL_BIND_DELEGATE(...).
namespace DelegateBindDetail
{
	// Global function binding, no implicit argument
	template <typename R, typename... TYPES>
	struct BinderFunc SEOUL_SEALED
	{
		typedef Delegate<R(TYPES...)> DelegateType;

		template <R(*F)(TYPES...)>
		static R CallFunction(void* pObject, TYPES... args)
		{
			return (F)(args...);
		}

		template <R(*F)(TYPES...)>
		inline DelegateType Apply()
		{
			return DelegateType(&CallFunction<F>, nullptr);
		}
	};

	template <typename R, typename... TYPES>
	inline BinderFunc<R, TYPES...> Bind(R(*f)(TYPES...))
	{
		return BinderFunc<R, TYPES...>();
	}

	// Global function binding with an implicit argument
	template <typename R, typename... TYPES>
	struct BinderFuncImplicitArgument SEOUL_SEALED
	{
		typedef Delegate<R(TYPES...)> DelegateType;

		BinderFuncImplicitArgument(void* pObject)
			: m_pObject(pObject)
		{
		}

		template <R(*F)(void*, TYPES...)>
		static R CallFunctionWithObject(void* pObject, TYPES... args)
		{
			return (F)(pObject, args...);
		}

		template <R(*F)(void*, TYPES...)>
		inline DelegateType Apply()
		{
			return DelegateType(&CallFunctionWithObject<F>, m_pObject);
		}

		void* const m_pObject;
	};

	template <typename R, typename... TYPES>
	inline BinderFuncImplicitArgument<R, TYPES...> Bind(R(*f)(void*, TYPES...), void* pObject)
	{
		return BinderFuncImplicitArgument<R, TYPES...>(pObject);
	}

	// Member function binding
	template <typename T, typename R, typename... TYPES>
	struct BinderMemberFunc SEOUL_SEALED
	{
		typedef Delegate<R(TYPES...)> DelegateType;

		SEOUL_STATIC_ASSERT_MESSAGE(DelegateMemberBindHandleAnchorGlobal::IsDelegateTarget<T>::Value, "Type T in SEOUL_BIND_DELEGATE(..., T*) must be a delegate target, include the SEOUL_DELEGATE_TARGET(class_name) macro in a public: section of the class declaration.");

		BinderMemberFunc(T* pObject)
			: m_pObject(DelegateMemberBindHandle::ToVoidStar(DelegateMemberBindHandleAnchorGlobal::GetHandle(pObject)))
		{
		}

		template <R(T::* M)(TYPES...)>
		static R CallObject(void* pObject, TYPES... args)
		{
			T* pCallee = DelegateMemberBindHandleAnchorGlobal::GetPointer<T>(DelegateMemberBindHandle::ToHandle(pObject));

			// We want crashes due to dangling 'this' to reliably occur here, so we force
			// a crash, even in Ship.
#if SEOUL_ASSERTIONS_DISABLED
			if (nullptr == pCallee)
			{
				// Trigger a segmentation fault explicitly.
				(*((UInt volatile*)nullptr)) = 1;
			}
#else // !#if SEOUL_ASSERTIONS_DISABLED
			SEOUL_ASSERT(nullptr != pCallee);
#endif // /!#if SEOUL_ASSERTIONS_DISABLED

			return (pCallee->*M)(args...);
		}

		template <R(T::* M)(TYPES...)>
		inline DelegateType Apply()
		{
			return DelegateType(&CallObject<M>, m_pObject);
		}

		void* const m_pObject;
	};

	template <typename T, typename R, typename... TYPES>
	inline BinderMemberFunc<T, R, TYPES...> Bind(R(T::* m)(TYPES...), T* pObject)
	{
		return BinderMemberFunc<T, R, TYPES...>(pObject);
	}

	// Member function binding
	template <typename T, typename R, typename... TYPES>
	struct BinderConstMemberFunc SEOUL_SEALED
	{
		typedef Delegate<R(TYPES...)> DelegateType;

		SEOUL_STATIC_ASSERT_MESSAGE(DelegateMemberBindHandleAnchorGlobal::IsDelegateTarget<T>::Value, "Type T in SEOUL_BIND_DELEGATE(..., T*) must be a delegate target, include the SEOUL_DELEGATE_TARGET(class_name) macro in a public: section of the class declaration.");

		BinderConstMemberFunc(T* pObject)
			: m_pObject(DelegateMemberBindHandle::ToVoidStar(DelegateMemberBindHandleAnchorGlobal::GetHandle(pObject)))
		{
		}

		template <R(T::* M)(TYPES...) const>
		static R CallObject(void* pObject, TYPES... args)
		{
			T* pCallee = DelegateMemberBindHandleAnchorGlobal::GetPointer<T>(DelegateMemberBindHandle::ToHandle(pObject));

			// We want crashes due to dangling 'this' to reliably occur here, so we force
			// a crash, even in Ship.
#if SEOUL_ASSERTIONS_DISABLED
			if (nullptr == pCallee)
			{
				// Trigger a segmentation fault explicitly.
				(*((UInt volatile*)nullptr)) = 1;
			}
#else // !#if SEOUL_ASSERTIONS_DISABLED
			SEOUL_ASSERT(nullptr != pCallee);
#endif // /!#if SEOUL_ASSERTIONS_DISABLED

			return (pCallee->*M)(args...);
		}

		template <R(T::* M)(TYPES...) const>
		inline DelegateType Apply()
		{
			return DelegateType(&CallObject<M>, m_pObject);
		}

		void* const m_pObject;
	};

	template <typename T, typename R, typename... TYPES>
	inline BinderConstMemberFunc<T, R, TYPES...> Bind(R(T::* m)(TYPES... args) const, T* pObject)
	{
		return BinderConstMemberFunc<T, R, TYPES...>(pObject);
	}
}


/**
 * SEOUL_BIND_DELEGATE() is used to bind a global function, global function with
 * implicit argument, or a member function to its corresponding delegate. Usage:
 *
 * \example
 *    Global functions
 *
 *    void Bar(...some arguments...);
 *
 *    myDelegate = SEOUL_BIND_DELEGATE(&Bar);
 *
 * \example
 *    Global function with implicit first argument.
 *
 *    class Foo;
 *
 *    void Bar(Foo* pObject);
 *
 *    Foo* pFoo;
 *    myDelegate = SEOUL_BIND_DELEGATE(&Bar, pFoo);
 *
 * \example
 *    Member function
 *
 *    class Foo;
 *
 *    Foo* pFoo;
 *    myDelegate = SEOUL_BIND_DELEGATE(&Foo::Bar, pFoo);
 */
#define SEOUL_BIND_DELEGATE(function, ...) Seoul::DelegateBindDetail::Bind(function, ##__VA_ARGS__).Apply<function>()

} // namespace Seoul

#endif // include guard
