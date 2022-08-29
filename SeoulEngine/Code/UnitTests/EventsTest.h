/**
 * \file EventsTest.h
 * \brief Tests for the Events library.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EVENTS_TEST_H
#define EVENTS_TEST_H

#include "Delegate.h"
#include "Logger.h"
#include "Prereqs.h"
#include "SeoulString.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class EventsTest SEOUL_SEALED
{
public:
	SEOUL_DELEGATE_TARGET(EventsTest);

	EventsTest();
	~EventsTest();

	void TestAddRemove();
	void TestMoveLastCallbackToFirst();
	void TestNoCallbacks();
	void TestEventEnable();
	void TestEventHandled();
	void TestEvent0Args();
	void TestEvent1Arg();
	void TestEvent2Args();
	void TestEvent3Args();
	void TestEvent4Args();
	void TestEvent5Args();
	void TestEvent6Args();
	void TestEvent7Args();
	void TestEvent8Args();

	// Static callback, specifically for handled event testing.
	static Bool StaticHandledCallback0();

	// Static callbacks used to test the game event system
	static Bool StaticCallback0();
	static void StaticCallback1(Int arg1);
	static Bool StaticCallback2(Int arg1, Float arg2);
	static void StaticCallback3(Int arg1, Float arg2, void *arg3);
	static Bool StaticCallback4(Int arg1, Float arg2, void *arg3, const Byte *arg4);
	static void StaticCallback5(Int arg1, Float arg2, void *arg3, const Byte *arg4, const String & arg5);
	static Bool StaticCallback6(Int arg1, Float arg2, void *arg3, const Byte *arg4, const String & arg5, Bool arg6);
	static void StaticCallback7(Int arg1, Float arg2, void *arg3, const Byte *arg4, const String & arg5, Bool arg6, Double arg7);
	static Bool StaticCallback8(Int arg1, Float arg2, void *arg3, const Byte *arg4, const String & arg5, Bool arg6, Double arg7, const UShort arg8);

	// Class method callbacks used to test the game event system
	void Callback0();
	Bool Callback1(Int arg1);
	void Callback2(Int arg1, Float arg2);
	Bool Callback3(Int arg1, Float arg2, void *arg3);
	void Callback4(Int arg1, Float arg2, void *arg3, const Byte *arg4);
	Bool Callback5(Int arg1, Float arg2, void *arg3, const Byte *arg4, const String & arg5);
	void Callback6(Int arg1, Float arg2, void *arg3, const Byte *arg4, const String & arg5, Bool arg6);
	Bool Callback7(Int arg1, Float arg2, void *arg3, const Byte *arg4, const String & arg5, Bool arg6, Double arg7);
	void Callback8(Int arg1, Float arg2, void *arg3, const Byte *arg4, const String & arg5, Bool arg6, Double arg7, const UShort arg8);

	// Invalid callbacks which should never be called
	static Bool InvalidStaticCallback(void *arg);
	Bool InvalidCallback(void *arg);

private:
	// String used to keep track of static callback calls
	static String s_sStaticCallbackCalls;

	// String used to keep track of class method callback calls
	String m_sCallbackCalls;
};

#endif // /#if SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
