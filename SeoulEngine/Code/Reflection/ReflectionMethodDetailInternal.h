/**
 * \file ReflectionMethodDetailInternal.h
 * \brief This file is included multiple times in ReflectionMethodDetail, one for each method variation
 * defined in ReflectionMethodVariations.h, in order to define all the subclasses of Reflection::Method
 * needed to bind methods, both constant and not-constant, of 0 to many arguments.
 *
 * Think of this file as  "function" and the SEOUL_METHOD_VARIATION_ macros as "arguments", and it
 * makes it somewhat easier to parse and understand.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#ifndef  REFLECTION_METHOD_DETAIL_H
#error ReflectionMethodDetailInternal.h should only be included by ReflectionMethodDetail.h
#endif

// Required "arguments" when including this file - these must be defined prior to the include and will be undefined
// at the end of this file.
#if !defined(SEOUL_METHOD_VARIATION_ARGC) || !defined(SEOUL_METHOD_VARIATION_T) || !defined(SEOUL_METHOD_VARIATION_T_ARGS) || !defined(SEOUL_METHOD_VARIATION_ARGS)  || !defined(SEOUL_METHOD_VARIATION_PARAMS)  || !defined(SEOUL_METHOD_VARIATION_PREFIX) || !defined(SEOUL_METHOD_VARIATION_TYPE_IDS)
#   error "Please define all necessary macros before including this file."
#endif

// Determine if we need a comma after the return argument in various contexts - only ever "false" if we have 0 arguments.
#if (0 == SEOUL_METHOD_VARIATION_ARGC)
#define SEOUL_METHOD_VARIATION_COMMA
#else
#define SEOUL_METHOD_VARIATION_COMMA ,
#endif

// Define some names to clean things up below. These are:
// - SEOUL_METHOD_VARIATION_FUNC_NAME - class name of the binder class for regular functions (either global, or static member)
// - SEOUL_METHOD_VARIATION_CONST_NAME - class name of the binder class for constant methods
// - SEOUL_METHOD_VARIATION_CONST_HELPER_NAME - invoker used to handle "void" vs. other return value types, const methods.
// - SEOUL_METHOD_VARIATION_NAME - class name of the binder class for non-constant methods
// - SEOUL_METHOD_VARIATION_HELPER_NAME - invoker used to handle "void" vs. other return value types, non-constant methods.
// - SEOUL_METHOD_VARIATION_INVOKER_NAME SEOUL_CONCAT(SEOUL_METHOD_VARIATION_PREFIX, Invoker)
//   - invoker is a wrapper - it takes a HELPER type and the arguments of the class, and beyond that
//     does exactly the same operation. It allows us to define the body of TryInvoke() once for both
//     constant and non-constant methods. Reduce some boilerplating, which is always the battle in this sort
//     of code.
// - SEOUL_METHOD_VARIATION_SCRIPT_INVOKER_NAME SEOUL_CONCAT(SEOUL_METHOD_VARIATION_PREFIX, ScriptInvoker)
//   - identical to Invoker, but functionality specific to invocation of functions from Script data.
#define SEOUL_METHOD_VARIATION_FUNC_NAME SEOUL_CONCAT(SEOUL_METHOD_VARIATION_PREFIX, Function)
#define SEOUL_METHOD_VARIATION_FUNC_HELPER_NAME SEOUL_CONCAT(SEOUL_METHOD_VARIATION_PREFIX, InvokeFunctionHelper)
#define SEOUL_METHOD_VARIATION_CONST_NAME SEOUL_CONCAT(SEOUL_METHOD_VARIATION_PREFIX, ConstMethod)
#define SEOUL_METHOD_VARIATION_CONST_HELPER_NAME SEOUL_CONCAT(SEOUL_METHOD_VARIATION_PREFIX, InvokeConstMethodHelper)
#define SEOUL_METHOD_VARIATION_NAME SEOUL_CONCAT(SEOUL_METHOD_VARIATION_PREFIX, NonConstMethod)
#define SEOUL_METHOD_VARIATION_HELPER_NAME SEOUL_CONCAT(SEOUL_METHOD_VARIATION_PREFIX, InvokeMethodHelper)
#define SEOUL_METHOD_VARIATION_INVOKER_NAME SEOUL_CONCAT(SEOUL_METHOD_VARIATION_PREFIX, Invoker)
#define SEOUL_METHOD_VARIATION_SCRIPT_INVOKER_NAME SEOUL_CONCAT(SEOUL_METHOD_VARIATION_PREFIX, ScriptInvoker)

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Const method invoker - actually invokes a typed method, used to handle "void" vs. non-void return argument.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename C, typename R SEOUL_METHOD_VARIATION_COMMA SEOUL_METHOD_VARIATION_T, R (C::*M)(SEOUL_METHOD_VARIATION_T_ARGS) const>
struct SEOUL_METHOD_VARIATION_CONST_HELPER_NAME
{
	typedef typename RemoveConst<typename RemoveReference<R>::Type>::Type ReturnType;
	static const Bool NeedsThis = true;

	static void Invoke(
		C const* p,
		SEOUL_METHOD_VARIATION_ARGS SEOUL_METHOD_VARIATION_COMMA
		Any& rReturnValue)
	{
		SEOUL_ASSERT(nullptr != p);
		rReturnValue = (p->*M)(SEOUL_METHOD_VARIATION_PARAMS);
	}

	static void ScriptInvoke(
		C const* p,
		SEOUL_METHOD_VARIATION_ARGS SEOUL_METHOD_VARIATION_COMMA
		ReturnType& rReturnValue)
	{
		SEOUL_ASSERT(nullptr != p);
		rReturnValue = (p->*M)(SEOUL_METHOD_VARIATION_PARAMS);
	}
};

template <typename C SEOUL_METHOD_VARIATION_COMMA SEOUL_METHOD_VARIATION_T, void (C::*M)(SEOUL_METHOD_VARIATION_T_ARGS) const>
struct SEOUL_METHOD_VARIATION_CONST_HELPER_NAME<
	C,
	void,
	SEOUL_METHOD_VARIATION_T_ARGS SEOUL_METHOD_VARIATION_COMMA
	M>
{
	static const Bool NeedsThis = true;

#if (SEOUL_METHOD_VARIATION_ARGC == 1)
	typedef A0 Arg0;
#endif // /#if (SEOUL_METHOD_VARIATION_ARGC == 1)

	static void Invoke(
		C const* p,
		SEOUL_METHOD_VARIATION_ARGS SEOUL_METHOD_VARIATION_COMMA
		Any& rReturnValue)
	{
		SEOUL_ASSERT(nullptr != p);
		(p->*M)(SEOUL_METHOD_VARIATION_PARAMS);
	}

	static void ScriptInvoke(
		C const* p SEOUL_METHOD_VARIATION_COMMA
		SEOUL_METHOD_VARIATION_ARGS)
	{
		SEOUL_ASSERT(nullptr != p);
		(p->*M)(SEOUL_METHOD_VARIATION_PARAMS);
	}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Non-const method invoker - actually invokes a typed method, used to handle "void" vs. non-void return argument.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename C, typename R SEOUL_METHOD_VARIATION_COMMA SEOUL_METHOD_VARIATION_T, R (C::*M)(SEOUL_METHOD_VARIATION_T_ARGS)>
struct SEOUL_METHOD_VARIATION_HELPER_NAME
{
	typedef typename RemoveConst<typename RemoveReference<R>::Type>::Type ReturnType;
	static const Bool NeedsThis = true;

	static void Invoke(
		C* p,
		SEOUL_METHOD_VARIATION_ARGS SEOUL_METHOD_VARIATION_COMMA
		Any& rReturnValue)
	{
		SEOUL_ASSERT(nullptr != p);
		rReturnValue = (p->*M)(SEOUL_METHOD_VARIATION_PARAMS);
	}

	static void ScriptInvoke(
		C* p,
		SEOUL_METHOD_VARIATION_ARGS SEOUL_METHOD_VARIATION_COMMA
		ReturnType& rReturnValue)
	{
		SEOUL_ASSERT(nullptr != p);
		rReturnValue = (p->*M)(SEOUL_METHOD_VARIATION_PARAMS);
	}
};

template <typename C SEOUL_METHOD_VARIATION_COMMA SEOUL_METHOD_VARIATION_T, void (C::*M)(SEOUL_METHOD_VARIATION_T_ARGS)>
struct SEOUL_METHOD_VARIATION_HELPER_NAME<
	C,
	void,
	SEOUL_METHOD_VARIATION_T_ARGS SEOUL_METHOD_VARIATION_COMMA
	M>
{
	static const Bool NeedsThis = true;

#if (SEOUL_METHOD_VARIATION_ARGC == 1)
	typedef A0 Arg0;
#endif // /#if (SEOUL_METHOD_VARIATION_ARGC == 1)

	static void Invoke(
		C* p,
		SEOUL_METHOD_VARIATION_ARGS SEOUL_METHOD_VARIATION_COMMA
		Any& rReturnValue)
	{
		SEOUL_ASSERT(nullptr != p);
		(p->*M)(SEOUL_METHOD_VARIATION_PARAMS);
	}

	static void ScriptInvoke(
		C* p SEOUL_METHOD_VARIATION_COMMA
		SEOUL_METHOD_VARIATION_ARGS)
	{
		SEOUL_ASSERT(nullptr != p);
		(p->*M)(SEOUL_METHOD_VARIATION_PARAMS);
	}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function invoker - actually invokes a typed method, used to handle "void" vs. non-void return argument.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename R SEOUL_METHOD_VARIATION_COMMA SEOUL_METHOD_VARIATION_T, R (*F)(SEOUL_METHOD_VARIATION_T_ARGS)>
struct SEOUL_METHOD_VARIATION_FUNC_HELPER_NAME
{
	typedef typename RemoveConst<typename RemoveReference<R>::Type>::Type ReturnType;
	static const Bool NeedsThis = false;

	static void Invoke(
		void*,
		SEOUL_METHOD_VARIATION_ARGS SEOUL_METHOD_VARIATION_COMMA
		Any& rReturnValue)
	{
		rReturnValue = (*F)(SEOUL_METHOD_VARIATION_PARAMS);
	}

	static void ScriptInvoke(
		void*,
		SEOUL_METHOD_VARIATION_ARGS SEOUL_METHOD_VARIATION_COMMA
		ReturnType& rReturnValue)
	{
		rReturnValue = (*F)(SEOUL_METHOD_VARIATION_PARAMS);
	}
};

template <SEOUL_METHOD_VARIATION_T SEOUL_METHOD_VARIATION_COMMA void (*F)(SEOUL_METHOD_VARIATION_T_ARGS)>
struct SEOUL_METHOD_VARIATION_FUNC_HELPER_NAME<
	void,
	SEOUL_METHOD_VARIATION_T_ARGS SEOUL_METHOD_VARIATION_COMMA
	F>
{
	static const Bool NeedsThis = false;

#if (SEOUL_METHOD_VARIATION_ARGC == 1)
	typedef A0 Arg0;
#endif // /#if (SEOUL_METHOD_VARIATION_ARGC == 1)

	static void Invoke(
		void*,
		SEOUL_METHOD_VARIATION_ARGS SEOUL_METHOD_VARIATION_COMMA
		Any& rReturnValue)
	{
		(*F)(SEOUL_METHOD_VARIATION_PARAMS);
	}

	static void ScriptInvoke(
		void* SEOUL_METHOD_VARIATION_COMMA
		SEOUL_METHOD_VARIATION_ARGS)
	{
		(*F)(SEOUL_METHOD_VARIATION_PARAMS);
	}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Shared invoker - takes function signature and the INVOKE type, and then actually unpacks arguments from WeakAny
// and invokes the method. Allows us to share this body between constant and non-constant method binders.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename C, typename R SEOUL_METHOD_VARIATION_COMMA SEOUL_METHOD_VARIATION_T, typename INVOKE>
struct SEOUL_METHOD_VARIATION_INVOKER_NAME
{
	static MethodInvokeResult TryInvoke(
		Any& rReturnValue,
		const WeakAny& thisPointer,
		const MethodArguments& aArguments)
	{
		// Convert the this pointer to the concrete class type we expect.
		C* p = nullptr;
		if (MethodInvokerPointerCast<C>::Cast(thisPointer, p))
		{
			// Now extract arguments from each WeakAny argument into their compile time types.
#if (SEOUL_METHOD_VARIATION_ARGC > 0)
			typename RemoveConst<typename RemoveReference<A0>::Type>::Type a0;
			if (!TypeConstruct(aArguments[0], a0)) { return MethodInvokeResult(0); }
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 1)
			typename RemoveConst<typename RemoveReference<A1>::Type>::Type a1;
			if (!TypeConstruct(aArguments[1], a1)) { return MethodInvokeResult(1); }
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 2)
			typename RemoveConst<typename RemoveReference<A2>::Type>::Type a2;
			if (!TypeConstruct(aArguments[2], a2)) { return MethodInvokeResult(2); }
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 3)
			typename RemoveConst<typename RemoveReference<A3>::Type>::Type a3;
			if (!TypeConstruct(aArguments[3], a3)) { return MethodInvokeResult(3); }
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 4)
			typename RemoveConst<typename RemoveReference<A4>::Type>::Type a4;
			if (!TypeConstruct(aArguments[4], a4)) { return MethodInvokeResult(4); }
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 5)
			typename RemoveConst<typename RemoveReference<A5>::Type>::Type a5;
			if (!TypeConstruct(aArguments[5], a5)) { return MethodInvokeResult(5); }
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 6)
			typename RemoveConst<typename RemoveReference<A6>::Type>::Type a6;
			if (!TypeConstruct(aArguments[6], a6)) { return MethodInvokeResult(6); }
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 7)
			typename RemoveConst<typename RemoveReference<A7>::Type>::Type a7;
			if (!TypeConstruct(aArguments[7], a7)) { return MethodInvokeResult(7); }
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 8)
			typename RemoveConst<typename RemoveReference<A8>::Type>::Type a8;
			if (!TypeConstruct(aArguments[8], a8)) { return MethodInvokeResult(8); }
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 9)
			typename RemoveConst<typename RemoveReference<A9>::Type>::Type a9;
			if (!TypeConstruct(aArguments[9], a9)) { return MethodInvokeResult(9); }
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 10)
			typename RemoveConst<typename RemoveReference<A10>::Type>::Type a10;
			if (!TypeConstruct(aArguments[10], a10)) { return MethodInvokeResult(10); }
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 11)
			typename RemoveConst<typename RemoveReference<A11>::Type>::Type a11;
			if (!TypeConstruct(aArguments[11], a11)) { return MethodInvokeResult(11); }
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 12)
			typename RemoveConst<typename RemoveReference<A12>::Type>::Type a12;
			if (!TypeConstruct(aArguments[12], a12)) { return MethodInvokeResult(12); }
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 13)
			typename RemoveConst<typename RemoveReference<A13>::Type>::Type a13;
			if (!TypeConstruct(aArguments[13], a13)) { return MethodInvokeResult(13); }
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 14)
			typename RemoveConst<typename RemoveReference<A14>::Type>::Type a14;
			if (!TypeConstruct(aArguments[14], a14)) { return MethodInvokeResult(14); }
#endif

			// Now invoke with the invoke type passed as a template argument.
			INVOKE::Invoke(
				p,
				SEOUL_METHOD_VARIATION_PARAMS SEOUL_METHOD_VARIATION_COMMA
				rReturnValue);

			return MethodInvokeResult();
		}
		else
		{
			// Failed to cast 'this', null this.
			return MethodInvokeResult(MethodInvokeResultCode::kNullThis);
		}
	}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Shared invoker - takes function signature and the INVOKE type, and then actually implements variations
// of invocation from script data.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename C, typename R SEOUL_METHOD_VARIATION_COMMA SEOUL_METHOD_VARIATION_T, typename INVOKE, Bool B_WEAK>
struct SEOUL_METHOD_VARIATION_SCRIPT_INVOKER_NAME
{
	static int ScriptInvoke(lua_State* pVm)
	{
		// Convert the this pointer to the concrete class type we expect.
		C* p = nullptr;
		{
			void* pThisPointer = lua_touserdata(pVm, 1);
			if (B_WEAK && nullptr != pThisPointer)
			{
				pThisPointer = *((void**)pThisPointer);
			}

			p = (C*)pThisPointer;
		}

		// Check this.
		if (nullptr == p && INVOKE::NeedsThis)
		{
			// Ok to just call error here, no auto variables.
			luaL_error(pVm, "argument 1 is nil, expected type '%s'", TypeOf<C>().GetName().CStr());
			return 0;
		}

		// Now extract arguments from each WeakAny argument into their compile time types.
#if (SEOUL_METHOD_VARIATION_ARGC > 0)
		typename RemoveConst<typename RemoveReference<A0>::Type>::Type a0;
		FromScriptVm(pVm, 2, a0);
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 1)
		typename RemoveConst<typename RemoveReference<A1>::Type>::Type a1;
		FromScriptVm(pVm, 3, a1);
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 2)
		typename RemoveConst<typename RemoveReference<A2>::Type>::Type a2;
		FromScriptVm(pVm, 4, a2);
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 3)
		typename RemoveConst<typename RemoveReference<A3>::Type>::Type a3;
		FromScriptVm(pVm, 5, a3);
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 4)
		typename RemoveConst<typename RemoveReference<A4>::Type>::Type a4;
		FromScriptVm(pVm, 6, a4);
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 5)
		typename RemoveConst<typename RemoveReference<A5>::Type>::Type a5;
		FromScriptVm(pVm, 7, a5);
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 6)
		typename RemoveConst<typename RemoveReference<A6>::Type>::Type a6;
		FromScriptVm(pVm, 8, a6);
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 7)
		typename RemoveConst<typename RemoveReference<A7>::Type>::Type a7;
		FromScriptVm(pVm, 9, a7);
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 8)
		typename RemoveConst<typename RemoveReference<A8>::Type>::Type a8;
		FromScriptVm(pVm, 10, a8);
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 9)
		typename RemoveConst<typename RemoveReference<A9>::Type>::Type a9;
		FromScriptVm(pVm, 11, a9);
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 10)
		typename RemoveConst<typename RemoveReference<A10>::Type>::Type a10;
		FromScriptVm(pVm, 12, a10);
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 11)
		typename RemoveConst<typename RemoveReference<A11>::Type>::Type a11;
		FromScriptVm(pVm, 13, a11);
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 12)
		typename RemoveConst<typename RemoveReference<A12>::Type>::Type a12;
		FromScriptVm(pVm, 14, a12);
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 13)
		typename RemoveConst<typename RemoveReference<A13>::Type>::Type a13;
		FromScriptVm(pVm, 15, a13);
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 14)
		typename RemoveConst<typename RemoveReference<A14>::Type>::Type a14;
		FromScriptVm(pVm, 16, a14);
#endif

		// Now invoke with the invoke type passed as a template argument.
		typename RemoveConst<typename RemoveReference<R>::Type>::Type ret;
		INVOKE::ScriptInvoke(
			p,
			SEOUL_METHOD_VARIATION_PARAMS SEOUL_METHOD_VARIATION_COMMA
			ret);
		
		// Push the return value.
		ToScriptVm(pVm, ret);

		// Done, one return value.
		return 1;
	}
};

template <typename C SEOUL_METHOD_VARIATION_COMMA SEOUL_METHOD_VARIATION_T, typename INVOKE, Bool B_WEAK>
struct SEOUL_METHOD_VARIATION_SCRIPT_INVOKER_NAME<C, void SEOUL_METHOD_VARIATION_COMMA SEOUL_METHOD_VARIATION_T_ARGS, INVOKE, B_WEAK>
{
	static int ScriptInvoke(lua_State* pVm)
	{
		// Convert the this pointer to the concrete class type we expect.
		C* p = nullptr;
		{
			void* pThisPointer = lua_touserdata(pVm, 1);
			if (B_WEAK && nullptr != pThisPointer)
			{
				pThisPointer = *((void**)pThisPointer);
			}

			p = (C*)pThisPointer;
		}

		// Check this.
		if (nullptr == p && INVOKE::NeedsThis)
		{
			// Ok to just call error here, no auto variables.
			luaL_error(pVm, "argument 1 is nil, expected type '%s'", TypeOf<C>().GetName().CStr());
			return 0;
		}

		// Now extract arguments from each WeakAny argument into their compile time types.
#if (SEOUL_METHOD_VARIATION_ARGC > 0)
		typename RemoveConst<typename RemoveReference<A0>::Type>::Type a0;
		FromScriptVm(pVm, 2, a0);
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 1)
		typename RemoveConst<typename RemoveReference<A1>::Type>::Type a1;
		FromScriptVm(pVm, 3, a1);
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 2)
		typename RemoveConst<typename RemoveReference<A2>::Type>::Type a2;
		FromScriptVm(pVm, 4, a2);
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 3)
		typename RemoveConst<typename RemoveReference<A3>::Type>::Type a3;
		FromScriptVm(pVm, 5, a3);
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 4)
		typename RemoveConst<typename RemoveReference<A4>::Type>::Type a4;
		FromScriptVm(pVm, 6, a4);
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 5)
		typename RemoveConst<typename RemoveReference<A5>::Type>::Type a5;
		FromScriptVm(pVm, 7, a5);
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 6)
		typename RemoveConst<typename RemoveReference<A6>::Type>::Type a6;
		FromScriptVm(pVm, 8, a6);
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 7)
		typename RemoveConst<typename RemoveReference<A7>::Type>::Type a7;
		FromScriptVm(pVm, 9, a7);
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 8)
		typename RemoveConst<typename RemoveReference<A8>::Type>::Type a8;
		FromScriptVm(pVm, 10, a8);
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 9)
		typename RemoveConst<typename RemoveReference<A9>::Type>::Type a9;
		FromScriptVm(pVm, 11, a9);
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 10)
		typename RemoveConst<typename RemoveReference<A10>::Type>::Type a10;
		FromScriptVm(pVm, 12, a10);
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 11)
		typename RemoveConst<typename RemoveReference<A11>::Type>::Type a11;
		FromScriptVm(pVm, 13, a11);
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 12)
		typename RemoveConst<typename RemoveReference<A12>::Type>::Type a12;
		FromScriptVm(pVm, 14, a12);
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 13)
		typename RemoveConst<typename RemoveReference<A13>::Type>::Type a13;
		FromScriptVm(pVm, 15, a13);
#endif

#if (SEOUL_METHOD_VARIATION_ARGC > 14)
		typename RemoveConst<typename RemoveReference<A14>::Type>::Type a14;
		FromScriptVm(pVm, 16, a14);
#endif

		// Now invoke with the invoke type passed as a template argument.
		INVOKE::ScriptInvoke(
			p SEOUL_METHOD_VARIATION_COMMA
			SEOUL_METHOD_VARIATION_PARAMS);

		// Done, 0 return values.
		return 0;
	}
};

#if (SEOUL_METHOD_VARIATION_ARGC == 1)
template <typename C, typename INVOKE, Bool B_WEAK>
struct SEOUL_METHOD_VARIATION_SCRIPT_INVOKER_NAME<C, void, Script::FunctionInterface*, INVOKE, B_WEAK>
{
	static int ScriptInvoke(lua_State* pVm)
	{
		// Convert the this pointer to the concrete class type we expect.
		C* p = nullptr;
		{
			void* pThisPointer = lua_touserdata(pVm, 1);
			if (B_WEAK && nullptr != pThisPointer)
			{
				pThisPointer = *((void**)pThisPointer);
			}

			p = (C*)pThisPointer;
		}

		// Check this.
		if (nullptr == p && INVOKE::NeedsThis)
		{
			// Ok to just call error here, no auto variables.
			luaL_error(pVm, "argument 1 is nil, expected type '%s'", TypeOf<C>().GetName().CStr());
			return 0;
		}

		// This is a roundabout way to handle the fact that Script::FunctionInterface is an up-value.
		// If we just used Script::FunctionInterface here, most compilers (non-MSVC) would complain
		// about an incomplete type, even though at the point of specialization, Script::FunctionInterface
		// will be defined/complete.
		typedef typename RemovePointer<typename INVOKE::Arg0>::Type Type;

		// Now invoke with the invoke type passed as a template argument.
		Type interf(pVm);
		INVOKE::ScriptInvoke(
			p,
			&interf);
		
		return interf.OnCFuncExit();
	}
};
#endif // /#if (SEOUL_METHOD_VARIATION_ARGC > 0)

// The purpose of this assert is to remind you to add another case to
// TryInvoke() above when you increase the size of the MethodArguments array.
// Once you have done so, you can update this assert accordingly.
SEOUL_STATIC_ASSERT(MethodArguments::StaticSize == 15);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Actual Reflection::Method specialization for constant methods.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename C, typename R SEOUL_METHOD_VARIATION_COMMA SEOUL_METHOD_VARIATION_T, R (C::*M)(SEOUL_METHOD_VARIATION_T_ARGS) const>
class SEOUL_METHOD_VARIATION_CONST_NAME SEOUL_SEALED : public Method
{
public:
	template <size_t zStringArrayLengthInBytes>
	SEOUL_METHOD_VARIATION_CONST_NAME(Byte const (&sName)[zStringArrayLengthInBytes])
		: Method(CStringLiteral(sName))
	{
	}

	virtual const MethodTypeInfo& GetTypeInfo() const SEOUL_OVERRIDE
	{
		static const MethodTypeInfo kMethodTypeInfo(
			MethodTypeInfo::kConst,
			TypeId<C>(),
			TypeId<R>() SEOUL_METHOD_VARIATION_COMMA
			SEOUL_METHOD_VARIATION_TYPE_IDS);

		return kMethodTypeInfo;
	}

	virtual MethodInvokeResult TryInvoke(
		Any& rReturnValue,
		const WeakAny& thisPointer,
		const MethodArguments& aArguments) const SEOUL_OVERRIDE
	{
		typedef SEOUL_METHOD_VARIATION_CONST_HELPER_NAME<
				C,
				R SEOUL_METHOD_VARIATION_COMMA
				SEOUL_METHOD_VARIATION_T_ARGS,
				M> InvokeType;

		return SEOUL_METHOD_VARIATION_INVOKER_NAME<C const, R SEOUL_METHOD_VARIATION_COMMA SEOUL_METHOD_VARIATION_T_ARGS, InvokeType>::TryInvoke(
			rReturnValue,
			thisPointer,
			aArguments);
	}

	virtual void ScriptBind(lua_State* pVm, Bool bWeak) const SEOUL_OVERRIDE
	{
		typedef SEOUL_METHOD_VARIATION_CONST_HELPER_NAME<
				C,
				R SEOUL_METHOD_VARIATION_COMMA
				SEOUL_METHOD_VARIATION_T_ARGS,
				M> InvokeType;

		if (bWeak)
		{
			typedef SEOUL_METHOD_VARIATION_SCRIPT_INVOKER_NAME<
				C const,
				R SEOUL_METHOD_VARIATION_COMMA SEOUL_METHOD_VARIATION_T_ARGS,
				InvokeType,
				true> InvokerType;

			SEOUL_BIND_SCRIPT_CLOSURE();
		}
		else
		{
			typedef SEOUL_METHOD_VARIATION_SCRIPT_INVOKER_NAME<
				C const,
				R SEOUL_METHOD_VARIATION_COMMA SEOUL_METHOD_VARIATION_T_ARGS,
				InvokeType,
				false> InvokerType;

			SEOUL_BIND_SCRIPT_CLOSURE();
		}
	}
}; // class SEOUL_METHOD_VARIATION_CONST_NAME

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Actual Reflection::Method specialization for non-constant methods.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename C, typename R SEOUL_METHOD_VARIATION_COMMA SEOUL_METHOD_VARIATION_T, R (C::*M)(SEOUL_METHOD_VARIATION_T_ARGS)>
class SEOUL_METHOD_VARIATION_NAME SEOUL_SEALED : public Method
{
public:
	template <size_t zStringArrayLengthInBytes>
	SEOUL_METHOD_VARIATION_NAME(Byte const (&sName)[zStringArrayLengthInBytes])
		: Method(CStringLiteral(sName))
	{
	}

	virtual const MethodTypeInfo& GetTypeInfo() const SEOUL_OVERRIDE
	{
		static const MethodTypeInfo kMethodTypeInfo(
			0u,
			TypeId<C>(),
			TypeId<R>() SEOUL_METHOD_VARIATION_COMMA
			SEOUL_METHOD_VARIATION_TYPE_IDS);

		return kMethodTypeInfo;
	}

	virtual MethodInvokeResult TryInvoke(
		Any& rReturnValue,
		const WeakAny& thisPointer,
		const MethodArguments& aArguments) const SEOUL_OVERRIDE
	{
		typedef SEOUL_METHOD_VARIATION_HELPER_NAME<
				C,
				R SEOUL_METHOD_VARIATION_COMMA
				SEOUL_METHOD_VARIATION_T_ARGS,
				M> InvokeType;

		return SEOUL_METHOD_VARIATION_INVOKER_NAME<C, R SEOUL_METHOD_VARIATION_COMMA SEOUL_METHOD_VARIATION_T_ARGS, InvokeType>::TryInvoke(
			rReturnValue,
			thisPointer,
			aArguments);
	}

	virtual void ScriptBind(lua_State* pVm, Bool bWeak) const SEOUL_OVERRIDE
	{
		typedef SEOUL_METHOD_VARIATION_HELPER_NAME<
				C,
				R SEOUL_METHOD_VARIATION_COMMA
				SEOUL_METHOD_VARIATION_T_ARGS,
				M> InvokeType;

		if (bWeak)
		{
			typedef SEOUL_METHOD_VARIATION_SCRIPT_INVOKER_NAME<
				C,
				R SEOUL_METHOD_VARIATION_COMMA SEOUL_METHOD_VARIATION_T_ARGS,
				InvokeType,
				true> InvokerType;
		
			SEOUL_BIND_SCRIPT_CLOSURE();
		}
		else
		{
			typedef SEOUL_METHOD_VARIATION_SCRIPT_INVOKER_NAME<
				C,
				R SEOUL_METHOD_VARIATION_COMMA SEOUL_METHOD_VARIATION_T_ARGS,
				InvokeType,
				false> InvokerType;
		
			SEOUL_BIND_SCRIPT_CLOSURE();
		}
	}
}; // class SEOUL_METHOD_VARIATION_NAME

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Actual Reflection::Method specialization for functions.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename C, typename R SEOUL_METHOD_VARIATION_COMMA SEOUL_METHOD_VARIATION_T, R (*F)(SEOUL_METHOD_VARIATION_T_ARGS)>
class SEOUL_METHOD_VARIATION_FUNC_NAME SEOUL_SEALED : public Method
{
public:
	template <size_t zStringArrayLengthInBytes>
	SEOUL_METHOD_VARIATION_FUNC_NAME(Byte const (&sName)[zStringArrayLengthInBytes])
		: Method(CStringLiteral(sName))
	{
	}

	virtual const MethodTypeInfo& GetTypeInfo() const SEOUL_OVERRIDE
	{
		static const MethodTypeInfo kMethodTypeInfo(
			MethodTypeInfo::kStatic,
			TypeId<C>(),
			TypeId<R>() SEOUL_METHOD_VARIATION_COMMA
			SEOUL_METHOD_VARIATION_TYPE_IDS);

		return kMethodTypeInfo;
	}

	virtual MethodInvokeResult TryInvoke(
		Any& rReturnValue,
		const WeakAny& thisPointer,
		const MethodArguments& aArguments) const SEOUL_OVERRIDE
	{
		typedef SEOUL_METHOD_VARIATION_FUNC_HELPER_NAME<
				R SEOUL_METHOD_VARIATION_COMMA
				SEOUL_METHOD_VARIATION_T_ARGS,
				F> InvokeType;

		return SEOUL_METHOD_VARIATION_INVOKER_NAME<void, R SEOUL_METHOD_VARIATION_COMMA SEOUL_METHOD_VARIATION_T_ARGS, InvokeType>::TryInvoke(
			rReturnValue,
			WeakAny(),
			aArguments);
	}

	virtual void ScriptBind(lua_State* pVm, Bool bWeak) const SEOUL_OVERRIDE
	{
		typedef SEOUL_METHOD_VARIATION_FUNC_HELPER_NAME<
				R SEOUL_METHOD_VARIATION_COMMA
				SEOUL_METHOD_VARIATION_T_ARGS,
				F> InvokeType;

		if (bWeak)
		{
			typedef SEOUL_METHOD_VARIATION_SCRIPT_INVOKER_NAME<
				void,
				R SEOUL_METHOD_VARIATION_COMMA SEOUL_METHOD_VARIATION_T_ARGS,
				InvokeType,
				true> InvokerType;
		
			SEOUL_BIND_SCRIPT_CLOSURE();
		}
		else
		{
			typedef SEOUL_METHOD_VARIATION_SCRIPT_INVOKER_NAME<
				void,
				R SEOUL_METHOD_VARIATION_COMMA SEOUL_METHOD_VARIATION_T_ARGS,
				InvokeType,
				false> InvokerType;
		
			SEOUL_BIND_SCRIPT_CLOSURE();
		}
	}
}; // class SEOUL_METHOD_VARIATION_NAME

// Undef a number of macros, either created in this file or expected as "arguments" to this file.
#undef SEOUL_METHOD_VARIATION_SCRIPT_INVOKER_NAME
#undef SEOUL_METHOD_VARIATION_INVOKER_NAME
#undef SEOUL_METHOD_VARIATION_HELPER_NAME
#undef SEOUL_METHOD_VARIATION_NAME
#undef SEOUL_METHOD_VARIATION_CONST_HELPER_NAME
#undef SEOUL_METHOD_VARIATION_CONST_NAME
#undef SEOUL_METHOD_VARIATION_FUNC_HELPER_NAME
#undef SEOUL_METHOD_VARIATION_FUNC_NAME

#undef SEOUL_METHOD_VARIATION_COMMA

#undef SEOUL_METHOD_VARIATION_TYPE_IDS
#undef SEOUL_METHOD_VARIATION_PREFIX
#undef SEOUL_METHOD_VARIATION_PARAMS
#undef SEOUL_METHOD_VARIATION_ARGS
#undef SEOUL_METHOD_VARIATION_T_ARGS
#undef SEOUL_METHOD_VARIATION_T
#undef SEOUL_METHOD_VARIATION_ARGC
