/**
 * \file DelegateTest.h
 * \brief Tests for the Delegate class.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef DELEGATE_TEST_H
#define DELEGATE_TEST_H

#include "Delegate.h"
#include "Logger.h"
#include "Prereqs.h"
#include "SeoulString.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class DelegateTest SEOUL_SEALED
{
public:
	SEOUL_DELEGATE_TARGET(DelegateTest);

	void TestDefault();
	void TestApi0();
	void TestApi0Instance();
	void TestApi0ConstInstance();
	void TestApi0ImplicitArg();
	void TestApi1();
	void TestApi1Instance();
	void TestApi1ConstInstance();
	void TestApi1ImplicitArg();
	void TestDanglingDelegate();

private:
	static void ImplicitArg0(void* p);
	static void ImplicitArg1(void* p, Int i);

	static void Static0();
	static void Static1(Int i);
	
	void Instance0();
	void Instance1(Int i);
	
	void ConstInstance0() const;
	void ConstInstance1(Int i) const;

	void ClearCalls();
};

#endif // /#if SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
