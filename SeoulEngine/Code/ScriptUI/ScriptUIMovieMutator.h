/**
 * \file ScriptUIMovieMutator.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_UI_MOVIE_MUTATOR_H
#define SCRIPT_UI_MOVIE_MUTATOR_H

#include "CommandStream.h"
#include "ReflectionAny.h"
#include "ReflectionMethod.h"

namespace Seoul
{

// Forward declarations
class ScriptUIMovie;

template <typename T, Bool IS_ENUM>
struct FalconUIMovieMutatorToAny
{
	static void ToAny(const T& v, Reflection::Any& rOut)
	{
		rOut = Reflection::Any(v);
	}
};

template <typename T>
struct FalconUIMovieMutatorToAny<T, true>
{
	static void ToAny(const T& v, Reflection::Any& rOut)
	{
		rOut = Reflection::Any((Int)v);
	}
};

class ScriptUIMovieMutator SEOUL_SEALED
{
public:
	ScriptUIMovieMutator(ScriptUIMovie& rMovie, Bool bNativeCall = false);
	~ScriptUIMovieMutator();

	ScriptUIMovie& GetMovie() { return m_rMovie; }

	/**
	 * Invoke a method with 0 arguments.
	 *
	 * param[in] methodName HString identifier for the method to invoke, i.e. "DoIt"
	 */
	void InvokeMethod(
		HString methodName)
	{
		InternalInvokeMethod(methodName, 0);
	}

	/**
	 * Invoke a method with 1 argument.
	 *
	 * param[in] methodName HString identifier for the method to invoke, i.e. "DoIt"
	 * param[in] a1 Argument 1 to pass to the function - must be convertible to a number, boolean, or string.
	 */
	template <typename A1>
	void InvokeMethod(
		HString methodName,
		const A1& a1)
	{
		InternalToAny(a1, m_aMethodArguments[0]);
		InternalInvokeMethod(methodName, 1);
	}

	/**
	 * Invoke a method with 2 arguments.
	 *
	 * param[in] methodName HString identifier for the method to invoke, i.e. "DoIt"
	 * param[in] a1 Argument 1 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a2 Argument 2 to pass to the function - must be convertible to a number, boolean, or string.
	 */
	template <typename A1, typename A2>
	void InvokeMethod(
		HString methodName,
		const A1& a1,
		const A2& a2)
	{
		InternalToAny(a1, m_aMethodArguments[0]);
		InternalToAny(a2, m_aMethodArguments[1]);
		InternalInvokeMethod(methodName, 2);
	}

	/**
	 * Invoke a method with 3 arguments.
	 *
	 * param[in] methodName HString identifier for the method to invoke, i.e. "DoIt"
	 * param[in] a1 Argument 1 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a2 Argument 2 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a3 Argument 3 to pass to the function - must be convertible to a number, boolean, or string.
	 */
	template <typename A1, typename A2, typename A3>
	void InvokeMethod(
		HString methodName,
		const A1& a1,
		const A2& a2,
		const A3& a3)
	{
		InternalToAny(a1, m_aMethodArguments[0]);
		InternalToAny(a2, m_aMethodArguments[1]);
		InternalToAny(a3, m_aMethodArguments[2]);
		InternalInvokeMethod(methodName, 3);
	}

	/**
	 * Invoke a method with 4 arguments.
	 *
	 * param[in] methodName HString identifier for the method to invoke, i.e. "DoIt"
	 * param[in] a1 Argument 1 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a2 Argument 2 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a3 Argument 3 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a4 Argument 4 to pass to the function - must be convertible to a number, boolean, or string.
	 */
	template <typename A1, typename A2, typename A3, typename A4>
	void InvokeMethod(
		HString methodName,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4)
	{
		InternalToAny(a1, m_aMethodArguments[0]);
		InternalToAny(a2, m_aMethodArguments[1]);
		InternalToAny(a3, m_aMethodArguments[2]);
		InternalToAny(a4, m_aMethodArguments[3]);
		InternalInvokeMethod(methodName, 4);
	}

	/**
	 * Invoke a method with 5 arguments.
	 *
	 * param[in] methodName HString identifier for the method to invoke, i.e. "DoIt"
	 * param[in] a1 Argument 1 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a2 Argument 2 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a3 Argument 3 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a4 Argument 4 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a5 Argument 5 to pass to the function - must be convertible to a number, boolean, or string.
	 */
	template <typename A1, typename A2, typename A3, typename A4, typename A5>
	void InvokeMethod(
		HString methodName,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5)
	{
		InternalToAny(a1, m_aMethodArguments[0]);
		InternalToAny(a2, m_aMethodArguments[1]);
		InternalToAny(a3, m_aMethodArguments[2]);
		InternalToAny(a4, m_aMethodArguments[3]);
		InternalToAny(a5, m_aMethodArguments[4]);
		InternalInvokeMethod(methodName, 5);
	}

	/**
	 * Invoke a method with 6 arguments.
	 *
	 * param[in] methodName HString identifier for the method to invoke, i.e. "DoIt"
	 * param[in] a1 Argument 1 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a2 Argument 2 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a3 Argument 3 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a4 Argument 4 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a5 Argument 5 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a6 Argument 6 to pass to the function - must be convertible to a number, boolean, or string.
	 */
	template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
	void InvokeMethod(
		HString methodName,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6)
	{
		InternalToAny(a1, m_aMethodArguments[0]);
		InternalToAny(a2, m_aMethodArguments[1]);
		InternalToAny(a3, m_aMethodArguments[2]);
		InternalToAny(a4, m_aMethodArguments[3]);
		InternalToAny(a5, m_aMethodArguments[4]);
		InternalToAny(a6, m_aMethodArguments[5]);
		InternalInvokeMethod(methodName, 6);
	}

	/**
	 * Invoke a method with 7 arguments.
	 *
	 * param[in] methodName HString identifier for the method to invoke, i.e. "DoIt"
	 * param[in] a1 Argument 1 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a2 Argument 2 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a3 Argument 3 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a4 Argument 4 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a5 Argument 5 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a6 Argument 6 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a7 Argument 7 to pass to the function - must be convertible to a number, boolean, or string.
	 */
	template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
	void InvokeMethod(
		HString methodName,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6,
		const A7& a7)
	{
		InternalToAny(a1, m_aMethodArguments[0]);
		InternalToAny(a2, m_aMethodArguments[1]);
		InternalToAny(a3, m_aMethodArguments[2]);
		InternalToAny(a4, m_aMethodArguments[3]);
		InternalToAny(a5, m_aMethodArguments[4]);
		InternalToAny(a6, m_aMethodArguments[5]);
		InternalToAny(a7, m_aMethodArguments[6]);
		InternalInvokeMethod(methodName, 7);
	}

	/**
	 * Invoke a method with 8 arguments.
	 *
	 * param[in] methodName HString identifier for the method to invoke, i.e. "DoIt"
	 * param[in] a1 Argument 1 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a2 Argument 2 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a3 Argument 3 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a4 Argument 4 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a5 Argument 5 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a6 Argument 6 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a7 Argument 7 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a8 Argument 8 to pass to the function - must be convertible to a number, boolean, or string.
	 */
	template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
	void InvokeMethod(
		HString methodName,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6,
		const A7& a7,
		const A8& a8)
	{
		InternalToAny(a1, m_aMethodArguments[0]);
		InternalToAny(a2, m_aMethodArguments[1]);
		InternalToAny(a3, m_aMethodArguments[2]);
		InternalToAny(a4, m_aMethodArguments[3]);
		InternalToAny(a5, m_aMethodArguments[4]);
		InternalToAny(a6, m_aMethodArguments[5]);
		InternalToAny(a7, m_aMethodArguments[6]);
		InternalToAny(a8, m_aMethodArguments[7]);
		InternalInvokeMethod(methodName, 8);
	}

	/**
	 * Invoke a method with 9 arguments.
	 *
	 * param[in] methodName HString identifier for the method to invoke, i.e. "DoIt"
	 * param[in] a1 Argument 1 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a2 Argument 2 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a3 Argument 3 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a4 Argument 4 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a5 Argument 5 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a6 Argument 6 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a7 Argument 7 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a8 Argument 8 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a9 Argument 9 to pass to the function - must be convertible to a number, boolean, or string.
	 */
	template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9>
	void InvokeMethod(
		HString methodName,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6,
		const A7& a7,
		const A8& a8,
		const A9& a9)
	{
		InternalToAny(a1, m_aMethodArguments[0]);
		InternalToAny(a2, m_aMethodArguments[1]);
		InternalToAny(a3, m_aMethodArguments[2]);
		InternalToAny(a4, m_aMethodArguments[3]);
		InternalToAny(a5, m_aMethodArguments[4]);
		InternalToAny(a6, m_aMethodArguments[5]);
		InternalToAny(a7, m_aMethodArguments[6]);
		InternalToAny(a8, m_aMethodArguments[7]);
		InternalToAny(a9, m_aMethodArguments[8]);
		InternalInvokeMethod(methodName, 9);
	}

	/**
	 * Invoke a method with 10 arguments.
	 *
	 * param[in] methodName HString identifier for the method to invoke, i.e. "DoIt"
	 * param[in] a1 Argument 1 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a2 Argument 2 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a3 Argument 3 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a4 Argument 4 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a5 Argument 5 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a6 Argument 6 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a7 Argument 7 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a8 Argument 8 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a9 Argument 9 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a10 Argument 10 to pass to the function - must be convertible to a number, boolean, or string.
	 */
	template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10>
	void InvokeMethod(
		HString methodName,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6,
		const A7& a7,
		const A8& a8,
		const A9& a9,
		const A10& a10)
	{
		InternalToAny(a1, m_aMethodArguments[0]);
		InternalToAny(a2, m_aMethodArguments[1]);
		InternalToAny(a3, m_aMethodArguments[2]);
		InternalToAny(a4, m_aMethodArguments[3]);
		InternalToAny(a5, m_aMethodArguments[4]);
		InternalToAny(a6, m_aMethodArguments[5]);
		InternalToAny(a7, m_aMethodArguments[6]);
		InternalToAny(a8, m_aMethodArguments[7]);
		InternalToAny(a9, m_aMethodArguments[8]);
		InternalToAny(a10, m_aMethodArguments[9]);
		InternalInvokeMethod(methodName, 10);
	}

	/**
	 * Invoke a method with 11 arguments.
	 *
	 * param[in] methodName HString identifier for the method to invoke, i.e. "DoIt"
	 * param[in] a1 Argument 1 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a2 Argument 2 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a3 Argument 3 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a4 Argument 4 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a5 Argument 5 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a6 Argument 6 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a7 Argument 7 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a8 Argument 8 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a9 Argument 9 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a10 Argument 10 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a11 Argument 11 to pass to the function - must be convertible to a number, boolean, or string.
	 */
	template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11>
	void InvokeMethod(
		HString methodName,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6,
		const A7& a7,
		const A8& a8,
		const A9& a9,
		const A10& a10,
		const A11& a11)
	{
		InternalToAny(a1, m_aMethodArguments[0]);
		InternalToAny(a2, m_aMethodArguments[1]);
		InternalToAny(a3, m_aMethodArguments[2]);
		InternalToAny(a4, m_aMethodArguments[3]);
		InternalToAny(a5, m_aMethodArguments[4]);
		InternalToAny(a6, m_aMethodArguments[5]);
		InternalToAny(a7, m_aMethodArguments[6]);
		InternalToAny(a8, m_aMethodArguments[7]);
		InternalToAny(a9, m_aMethodArguments[8]);
		InternalToAny(a10, m_aMethodArguments[9]);
		InternalToAny(a11, m_aMethodArguments[10]);
		InternalInvokeMethod(methodName, 11);
	}

	/**
	 * Invoke a method with 12 arguments.
	 *
	 * param[in] methodName HString identifier for the method to invoke, i.e. "DoIt"
	 * param[in] a1 Argument 1 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a2 Argument 2 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a3 Argument 3 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a4 Argument 4 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a5 Argument 5 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a6 Argument 6 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a7 Argument 7 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a8 Argument 8 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a9 Argument 9 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a10 Argument 10 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a11 Argument 11 to pass to the function - must be convertible to a number, boolean, or string.
	 */
	template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11, typename A12>
	void InvokeMethod(
		HString methodName,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6,
		const A7& a7,
		const A8& a8,
		const A9& a9,
		const A10& a10,
		const A11& a11,
		const A12& a12)
	{
		InternalToAny(a1, m_aMethodArguments[0]);
		InternalToAny(a2, m_aMethodArguments[1]);
		InternalToAny(a3, m_aMethodArguments[2]);
		InternalToAny(a4, m_aMethodArguments[3]);
		InternalToAny(a5, m_aMethodArguments[4]);
		InternalToAny(a6, m_aMethodArguments[5]);
		InternalToAny(a7, m_aMethodArguments[6]);
		InternalToAny(a8, m_aMethodArguments[7]);
		InternalToAny(a9, m_aMethodArguments[8]);
		InternalToAny(a10, m_aMethodArguments[9]);
		InternalToAny(a11, m_aMethodArguments[10]);
		InternalToAny(a12, m_aMethodArguments[11]);
		InternalInvokeMethod(methodName, 12);
	}

	/**
	 * Invoke a method with 13 arguments.
	 *
	 * param[in] methodName HString identifier for the method to invoke, i.e. "DoIt"
	 * param[in] a1 Argument 1 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a2 Argument 2 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a3 Argument 3 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a4 Argument 4 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a5 Argument 5 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a6 Argument 6 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a7 Argument 7 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a8 Argument 8 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a9 Argument 9 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a10 Argument 10 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a11 Argument 11 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a12 Argument 12 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a13 Argument 13 to pass to the function - must be convertible to a number, boolean, or string.
	 */
	template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11, typename A12, typename A13>
	void InvokeMethod(
		HString methodName,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6,
		const A7& a7,
		const A8& a8,
		const A9& a9,
		const A10& a10,
		const A11& a11,
		const A12& a12,
		const A13& a13)
	{
		InternalToAny(a1, m_aMethodArguments[0]);
		InternalToAny(a2, m_aMethodArguments[1]);
		InternalToAny(a3, m_aMethodArguments[2]);
		InternalToAny(a4, m_aMethodArguments[3]);
		InternalToAny(a5, m_aMethodArguments[4]);
		InternalToAny(a6, m_aMethodArguments[5]);
		InternalToAny(a7, m_aMethodArguments[6]);
		InternalToAny(a8, m_aMethodArguments[7]);
		InternalToAny(a9, m_aMethodArguments[8]);
		InternalToAny(a10, m_aMethodArguments[9]);
		InternalToAny(a11, m_aMethodArguments[10]);
		InternalToAny(a12, m_aMethodArguments[11]);
		InternalToAny(a13, m_aMethodArguments[12]);
		InternalInvokeMethod(methodName, 13);
	}

	/**
	 * Invoke a method with 14 arguments.
	 *
	 * param[in] methodName HString identifier for the method to invoke, i.e. "DoIt"
	 * param[in] a1 Argument 1 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a2 Argument 2 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a3 Argument 3 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a4 Argument 4 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a5 Argument 5 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a6 Argument 6 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a7 Argument 7 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a8 Argument 8 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a9 Argument 9 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a10 Argument 10 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a11 Argument 11 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a12 Argument 12 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a13 Argument 13 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a14 Argument 14 to pass to the function - must be convertible to a number, boolean, or string.
	 */
	template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11, typename A12, typename A13, typename A14>
	void InvokeMethod(
		HString methodName,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6,
		const A7& a7,
		const A8& a8,
		const A9& a9,
		const A10& a10,
		const A11& a11,
		const A12& a12,
		const A13& a13,
		const A14& a14)
	{
		InternalToAny(a1, m_aMethodArguments[0]);
		InternalToAny(a2, m_aMethodArguments[1]);
		InternalToAny(a3, m_aMethodArguments[2]);
		InternalToAny(a4, m_aMethodArguments[3]);
		InternalToAny(a5, m_aMethodArguments[4]);
		InternalToAny(a6, m_aMethodArguments[5]);
		InternalToAny(a7, m_aMethodArguments[6]);
		InternalToAny(a8, m_aMethodArguments[7]);
		InternalToAny(a9, m_aMethodArguments[8]);
		InternalToAny(a10, m_aMethodArguments[9]);
		InternalToAny(a11, m_aMethodArguments[10]);
		InternalToAny(a12, m_aMethodArguments[11]);
		InternalToAny(a13, m_aMethodArguments[12]);
		InternalToAny(a14, m_aMethodArguments[13]);
		InternalInvokeMethod(methodName, 14);
	}

	/**
	 * Invoke a method with 15 arguments.
	 *
	 * param[in] methodName HString identifier for the method to invoke, i.e. "DoIt"
	 * param[in] a1 Argument 1 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a2 Argument 2 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a3 Argument 3 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a4 Argument 4 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a5 Argument 5 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a6 Argument 6 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a7 Argument 7 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a8 Argument 8 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a9 Argument 9 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a10 Argument 10 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a11 Argument 11 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a12 Argument 12 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a13 Argument 13 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a14 Argument 14 to pass to the function - must be convertible to a number, boolean, or string.
	 * param[in] a15 Argument 15 to pass to the function - must be convertible to a number, boolean, or string.
	 */
	template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11, typename A12, typename A13, typename A14, typename A15>
	void InvokeMethod(
		HString methodName,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6,
		const A7& a7,
		const A8& a8,
		const A9& a9,
		const A10& a10,
		const A11& a11,
		const A12& a12,
		const A13& a13,
		const A14& a14,
		const A15& a15)
	{
		InternalToAny(a1, m_aMethodArguments[0]);
		InternalToAny(a2, m_aMethodArguments[1]);
		InternalToAny(a3, m_aMethodArguments[2]);
		InternalToAny(a4, m_aMethodArguments[3]);
		InternalToAny(a5, m_aMethodArguments[4]);
		InternalToAny(a6, m_aMethodArguments[5]);
		InternalToAny(a7, m_aMethodArguments[6]);
		InternalToAny(a8, m_aMethodArguments[7]);
		InternalToAny(a9, m_aMethodArguments[8]);
		InternalToAny(a10, m_aMethodArguments[9]);
		InternalToAny(a11, m_aMethodArguments[10]);
		InternalToAny(a12, m_aMethodArguments[11]);
		InternalToAny(a13, m_aMethodArguments[12]);
		InternalToAny(a14, m_aMethodArguments[13]);
		InternalToAny(a15, m_aMethodArguments[14]);
		InternalInvokeMethod(methodName, 15);
	}

private:
	Reflection::MethodArguments m_aMethodArguments;
	ScriptUIMovie& m_rMovie;
	Bool m_bNativeCall;

	template <typename T>
	static void InternalToAny(const T& v, Reflection::Any& rOut)
	{
		FalconUIMovieMutatorToAny<T, IsEnum<T>::Value>::ToAny(v, rOut);
	}

	static void InternalToAny(Byte const* s, Reflection::Any& rOut)
	{
		rOut = Reflection::Any(s);
	}

	void InternalInvokeMethod(
		HString methodName,
		UInt32 uArgumentCount);

	SEOUL_DISABLE_COPY(ScriptUIMovieMutator);
}; // struct ScriptUIMovieMutator

} // namespace Seoul

#endif // include guard
