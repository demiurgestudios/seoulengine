/**
 * \file ReflectionMethod.h
 * \brief Reflection object used to define a reflectable method of a reflectable
 * class.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_METHOD_H
#define REFLECTION_METHOD_H

#include "FixedArray.h"
#include "Prereqs.h"
#include "ReflectionAny.h"
#include "ReflectionAttribute.h"
#include "ReflectionPrereqs.h"
#include "ReflectionWeakAny.h"
#include "ScopedPtr.h"
#include "SeoulHString.h"
extern "C" { struct lua_State; }

namespace Seoul::Reflection
{

// Forward declarations
struct MethodTypeInfo;

enum class MethodInvokeResultCode : Int16
{
	kSuccess,
	kIncorrectNumberOfArguments,
	kInvalidArgument,
	kNullThis,
	kUnsupportedReturnType,
};

/**
 * Structure returned from method invocation attempts - either indicates success, or indicates
 * failure and includes information about why the invocation failed (currently only the invalid arguments).
 */
struct MethodInvokeResult SEOUL_SEALED
{
	/**
	 * Construction with this variation indicates success.
	 */
	MethodInvokeResult()
		: m_iInvalidArgument(-1)
		, m_iResultCode((Int16)MethodInvokeResultCode::kSuccess)
	{
	}

	/**
	 * Construction with this variation indicates invoke failure, either with
	 * with an explicit argument, or with a failure to construct "this".
	 */
	explicit MethodInvokeResult(Int16 iInvalidArgument)
		: m_iInvalidArgument(iInvalidArgument)
		, m_iResultCode((Int16)MethodInvokeResultCode::kInvalidArgument)
	{
	}

	/**
	 * Construction with this variation provides an explicit failure condition,
	 * with an optional argument.
	 */
	explicit MethodInvokeResult(MethodInvokeResultCode eResultCode, Int16 iInvalidArgument = -1)
		: m_iInvalidArgument(iInvalidArgument)
		, m_iResultCode((Int16)eResultCode)
	{
	}

	/**
	 * @return The invalid argument that caused the invoke failure, or -1 on success, or -1
	 * if the invoke failed due to the "this" pointer.
	 */
	Int32 GetInvalidArgument() const
	{
		return m_iInvalidArgument;
	}

	/**
	 * @return The result of the invocation operation.
	 */
	MethodInvokeResultCode GetResultCode() const
	{
		return (MethodInvokeResultCode)m_iResultCode;
	}

	/**
	 * @return True if the invoke was successful, false otherwise.
	 */
	Bool WasSuccessful() const
	{
		return ((Int16)MethodInvokeResultCode::kSuccess == m_iResultCode);
	}

	/**
	 * @return True if the invoke was successful, false otherwise.
	 */
	explicit operator Bool() const
	{
		return WasSuccessful();
	}

	Int16 m_iInvalidArgument;
	Int16 m_iResultCode;
}; // struct MethodInvokeResult

/**
 * Method describes a class member function - methods can be invoked using
 * an opaque set of WeakAny and Any arguments.
 */
class Method SEOUL_ABSTRACT
{
public:
	virtual ~Method();

	/**
	 * @return The identifying name of the method - must be unique from all other methods in the owner Type.
	 */
	HString GetName() const
	{
		return m_Name;
	}

	/**
	 * @return The collection of attributes associated with this Method - attributes can be used to classify, categorize,
	 * or otherwise add metadata to a method.
	 */
	const AttributeCollection& GetAttributes() const
	{
		return m_Attributes;
	}

	// Gets the method type info associated with this Method.
	virtual const MethodTypeInfo& GetTypeInfo() const = 0;

	// Push onto the script stack a function closure for this method.
	virtual void ScriptBind(lua_State* pVm, Bool bWeak) const = 0;

	// Attempt to invoke the method - can fail if the arguments to this function are not
	// convertible to the underlying arguments of the method.
	virtual MethodInvokeResult TryInvoke(
		Any& rReturnValue,
		const WeakAny& thisPointer,
		const MethodArguments& aArguments) const = 0;

	// Convenience method, used to invoke a method when you want to ignore the return value.
	MethodInvokeResult TryInvoke(
		const WeakAny& thisPointer,
		const MethodArguments& aArguments) const
	{
		Any ignoredReturnValue;
		return TryInvoke(ignoredReturnValue, thisPointer, aArguments);
	}

	// Convenience method, used to invoke a method with 0 arguments.
	MethodInvokeResult TryInvoke(Any& rReturnValue, const WeakAny& thisPointer) const
	{
		return TryInvoke(rReturnValue, thisPointer, k0Arguments);
	}

	// Convenience method, used to invoke a method with 0 arguments when you want to ignore the return value.
	MethodInvokeResult TryInvoke(const WeakAny& thisPointer) const
	{
		Any ignoredReturnValue;
		return TryInvoke(ignoredReturnValue, thisPointer, k0Arguments);
	}

protected:
	Method(HString name);

private:
	AttributeCollection m_Attributes;

	friend struct MethodBuilder;
	friend struct TypeBuilder;
	friend class Type;

	HString m_Name;

	static const MethodArguments k0Arguments;

	SEOUL_DISABLE_COPY(Method);
}; // class Method

/**
 * @return True if the name of Method a is equal to b.
 */
inline Bool operator==(Method const* a, HString b)
{
	SEOUL_ASSERT(nullptr != a);
	return (a->GetName() == b);
}

/**
 * @return True if the name of Method a is *not* equal to b.
 */
inline Bool operator!=(Method const* a, HString b)
{
	return !(a == b);
}

} // namespace Seoul::Reflection

#endif // include guard
