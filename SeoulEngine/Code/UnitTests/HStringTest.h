/**
 * \file HStringTest.h
 * \brief Unit tests for the Seoul::HString class.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef HSTRING_TEST_H
#define HSTRING_TEST_H

#include "Prereqs.h"
#include "Delegate.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

// Forward declarations
class Thread;

class HStringTest SEOUL_SEALED
{
public:
	SEOUL_DELEGATE_TARGET(HStringTest);

	void TestBasic();
	void TestCanonical();
	void TestEmpty();
	void TestIntConvenience();
	void TestMultithreaded();
	void TestSubString();

private:
	Int TestMultithreadedBody(const Thread&);
}; // class HStringTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
