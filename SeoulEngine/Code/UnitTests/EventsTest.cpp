/**
 * \file EventsTest.cpp
 * \brief Tests for the Events library.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EventsManager.h"
#include "EventsTest.h"
#include "ReflectionDefine.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(EventsTest)
	SEOUL_ATTRIBUTE(UnitTest, Attributes::UnitTest::kInstantiateForEach) // Want Events::Manager and other resources to be recreated for each test.
	SEOUL_METHOD(TestAddRemove)
	SEOUL_METHOD(TestMoveLastCallbackToFirst)
	SEOUL_METHOD(TestNoCallbacks)
	SEOUL_METHOD(TestEventEnable)
	SEOUL_METHOD(TestEventHandled)
	SEOUL_METHOD(TestEvent0Args)
	SEOUL_METHOD(TestEvent1Arg)
	SEOUL_METHOD(TestEvent2Args)
	SEOUL_METHOD(TestEvent3Args)
	SEOUL_METHOD(TestEvent4Args)
	SEOUL_METHOD(TestEvent5Args)
	SEOUL_METHOD(TestEvent6Args)
	SEOUL_METHOD(TestEvent7Args)
	SEOUL_METHOD(TestEvent8Args)
SEOUL_END_TYPE();

static const HString kEvent("An event");

/**
 * String used to keep track of calls to the various static callbacks.  Every
 * time a static callback is called, it appends its ID to this string.  We then
 * test this string to make sure the proper set of callbacks was called when
 * triggering an event.
 */
String EventsTest::s_sStaticCallbackCalls;

/**
 * Initializes the test fixture by initializing the game event manager.
 */
EventsTest::EventsTest()
{
	SEOUL_UNITTESTING_ASSERT(!Events::Manager::Get().IsValid());
	SEOUL_NEW(MemoryBudgets::Event) Events::Manager;

	s_sStaticCallbackCalls.Clear();
	m_sCallbackCalls.Clear();
}

/**
 * Shuts down the test fixture by shutting down the game event manager.
 */
EventsTest::~EventsTest()
{
	SEOUL_DELETE Events::Manager::Get();
	SEOUL_UNITTESTING_ASSERT(!Events::Manager::Get().IsValid());
}

void EventsTest::TestAddRemove()
{
	Events::Manager::Get()->RegisterCallback(kEvent, SEOUL_BIND_DELEGATE(&StaticCallback0));
	Events::Manager::Get()->RegisterCallback(kEvent, SEOUL_BIND_DELEGATE(&StaticHandledCallback0));
	Events::Manager::Get()->RegisterCallback(kEvent, SEOUL_BIND_DELEGATE(&StaticCallback0));
	Events::Manager::Get()->TriggerEvent(kEvent);
	SEOUL_UNITTESTING_ASSERT_EQUAL("0H", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL(String(), m_sCallbackCalls);
	Events::Manager::Get()->UnregisterCallback(kEvent, SEOUL_BIND_DELEGATE(&StaticHandledCallback0));
	Events::Manager::Get()->TriggerEvent(kEvent);
	SEOUL_UNITTESTING_ASSERT_EQUAL("0H00", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL(String(), m_sCallbackCalls);
	Events::Manager::Get()->RegisterCallback(kEvent, SEOUL_BIND_DELEGATE(&StaticHandledCallback0));
	Events::Manager::Get()->TriggerEvent(kEvent);
	SEOUL_UNITTESTING_ASSERT_EQUAL("0H000H", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL(String(), m_sCallbackCalls);
}

void EventsTest::TestMoveLastCallbackToFirst()
{
	Events::Manager::Get()->RegisterCallback(kEvent, SEOUL_BIND_DELEGATE(&StaticCallback0));
	Events::Manager::Get()->RegisterCallback(kEvent, SEOUL_BIND_DELEGATE(&StaticCallback0));
	Events::Manager::Get()->RegisterCallback(kEvent, SEOUL_BIND_DELEGATE(&StaticHandledCallback0));
	Events::Manager::Get()->TriggerEvent(kEvent);
	SEOUL_UNITTESTING_ASSERT_EQUAL("00H", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL(String(), m_sCallbackCalls);
	Events::Manager::Get()->MoveLastCallbackToFirst(kEvent);
	SEOUL_UNITTESTING_ASSERT_EQUAL("00H", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL(String(), m_sCallbackCalls);
	Events::Manager::Get()->TriggerEvent(kEvent);
	SEOUL_UNITTESTING_ASSERT_EQUAL("00HH", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL(String(), m_sCallbackCalls);
}

/**
 * Test that a trigger of an event with no registered callbacks
 * is (effectively) a no-op.
 */
void EventsTest::TestNoCallbacks()
{
	Events::Manager::Get()->TriggerEvent(kEvent);
	SEOUL_UNITTESTING_ASSERT_EQUAL(String(), s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL(String(), m_sCallbackCalls);
	Events::Manager::Get()->UnregisterCallback(kEvent, SEOUL_BIND_DELEGATE(&StaticCallback0));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String(), s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL(String(), m_sCallbackCalls);
	Events::Manager::Get()->MoveLastCallbackToFirst(kEvent);
	SEOUL_UNITTESTING_ASSERT_EQUAL(String(), s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL(String(), m_sCallbackCalls);
}

/**
 * Tests basic event registration, making sure that we can register and
 * unregister simple events.
 */
void EventsTest::TestEventEnable()
{
	SEOUL_UNITTESTING_ASSERT(Events::Manager::Get()->IsEventEnabled(kEvent));
	Events::Manager::Get()->SetEventEnabled(kEvent, false);
	SEOUL_UNITTESTING_ASSERT(!Events::Manager::Get()->IsEventEnabled(kEvent));
	Events::Manager::Get()->RegisterCallback(kEvent, SEOUL_BIND_DELEGATE(&InvalidStaticCallback));
	SEOUL_UNITTESTING_ASSERT(!Events::Manager::Get()->IsEventEnabled(kEvent));
	Events::Manager::Get()->TriggerEvent(kEvent, (void*)nullptr);
	SEOUL_UNITTESTING_ASSERT(!Events::Manager::Get()->IsEventEnabled(kEvent));
}

/**
 * Test that a callback returning 'true' terminates invocation
 * of callbacks early.
 */
void EventsTest::TestEventHandled()
{
	Events::Manager::Get()->RegisterCallback(kEvent, SEOUL_BIND_DELEGATE(&StaticCallback0));
	Events::Manager::Get()->RegisterCallback(kEvent, SEOUL_BIND_DELEGATE(&StaticHandledCallback0));
	Events::Manager::Get()->RegisterCallback(kEvent, SEOUL_BIND_DELEGATE(&StaticCallback0));
	Events::Manager::Get()->TriggerEvent(kEvent);

	SEOUL_UNITTESTING_ASSERT_EQUAL("0H", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL(String(), m_sCallbackCalls);
}

/**
 * Tests that we can register a callback for an event with 0 arguments, and that
 * we can trigger that event.
 */
void EventsTest::TestEvent0Args()
{
	// Register an event and some callbacks
	Events::Manager::Get()->RegisterCallback(kEvent, SEOUL_BIND_DELEGATE(&EventsTest::StaticCallback0));
	Events::Manager::Get()->RegisterCallback(kEvent, SEOUL_BIND_DELEGATE(&EventsTest::Callback0, this));

	// Trigger the event with valid arguments
	Events::Manager::Get()->TriggerEvent(kEvent);

	// Make sure callbacks got called the proper number of times
	SEOUL_UNITTESTING_ASSERT_EQUAL("0", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL("0", m_sCallbackCalls);

	// Disable the event, trigger it, and make sure callbacks don't get called
	Events::Manager::Get()->SetEventEnabled(kEvent, false);
	Events::Manager::Get()->TriggerEvent(kEvent);

	SEOUL_UNITTESTING_ASSERT_EQUAL("0", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL("0", m_sCallbackCalls);

	// Re-enable and remove a callback
	Events::Manager::Get()->SetEventEnabled(kEvent, true);
	Events::Manager::Get()->UnregisterCallback(kEvent, SEOUL_BIND_DELEGATE(&EventsTest::StaticCallback0));
	Events::Manager::Get()->TriggerEvent(kEvent);

	SEOUL_UNITTESTING_ASSERT_EQUAL("0", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL("00", m_sCallbackCalls);

	// Remove last callback
	Events::Manager::Get()->UnregisterCallback(kEvent, SEOUL_BIND_DELEGATE(&EventsTest::Callback0, this));
	Events::Manager::Get()->TriggerEvent(kEvent);

	SEOUL_UNITTESTING_ASSERT_EQUAL("0", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL("00", m_sCallbackCalls);
}

/**
 * Tests that we can register a callback for an event with 1 argument, and that
 * we can trigger that event.
 */
void EventsTest::TestEvent1Arg()
{
	// Register an event and some callbacks
	Events::Manager::Get()->RegisterCallback(kEvent, SEOUL_BIND_DELEGATE(&StaticCallback1));
	Events::Manager::Get()->RegisterCallback(kEvent, SEOUL_BIND_DELEGATE(&EventsTest::Callback1, this));

	// Trigger the event with valid arguments
	Events::Manager::Get()->TriggerEvent(kEvent, 17);

	// Make sure callbacks got called the proper number of times
	SEOUL_UNITTESTING_ASSERT_EQUAL("1", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL("1", m_sCallbackCalls);

	// Disable the event, trigger it, and make sure callbacks don't get called
	Events::Manager::Get()->SetEventEnabled(kEvent, false);
	Events::Manager::Get()->TriggerEvent(kEvent, 17);

	SEOUL_UNITTESTING_ASSERT_EQUAL("1", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL("1", m_sCallbackCalls);

	// Re-enable and remove a callback
	Events::Manager::Get()->SetEventEnabled(kEvent, true);
	Events::Manager::Get()->UnregisterCallback(kEvent, SEOUL_BIND_DELEGATE(&StaticCallback1));
	Events::Manager::Get()->TriggerEvent(kEvent, 17);

	SEOUL_UNITTESTING_ASSERT_EQUAL("1", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL("11", m_sCallbackCalls);

	// Remove last callback
	Events::Manager::Get()->UnregisterCallback(kEvent, SEOUL_BIND_DELEGATE(&EventsTest::Callback1, this));
	Events::Manager::Get()->TriggerEvent(kEvent, 17);

	SEOUL_UNITTESTING_ASSERT_EQUAL("1", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL("11", m_sCallbackCalls);
}

/**
 * Tests that we can register a callback for an event with 2 arguments, and that
 * we can trigger that event.
 */
void EventsTest::TestEvent2Args()
{
	// Register an event and some callbacks
	Events::Manager::Get()->RegisterCallback(kEvent, SEOUL_BIND_DELEGATE(&StaticCallback2));
	Events::Manager::Get()->RegisterCallback(kEvent, SEOUL_BIND_DELEGATE(&EventsTest::Callback2, this));

	// Trigger the event with valid arguments
	Events::Manager::Get()->TriggerEvent(kEvent, 17, 3.5f);

	// Make sure callbacks got called the proper number of times
	SEOUL_UNITTESTING_ASSERT_EQUAL("2", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL("2", m_sCallbackCalls);

	// Disable the event, trigger it, and make sure callbacks don't get called
	Events::Manager::Get()->SetEventEnabled(kEvent, false);
	Events::Manager::Get()->TriggerEvent(kEvent, 17, 3.5f);

	SEOUL_UNITTESTING_ASSERT_EQUAL("2", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL("2", m_sCallbackCalls);

	// Re-enable and remove a callback
	Events::Manager::Get()->SetEventEnabled(kEvent, true);
	Events::Manager::Get()->UnregisterCallback(kEvent, SEOUL_BIND_DELEGATE(&StaticCallback2));
	Events::Manager::Get()->TriggerEvent(kEvent, 17, 3.5f);

	SEOUL_UNITTESTING_ASSERT_EQUAL("2", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL("22", m_sCallbackCalls);

	// Remove last callback
	Events::Manager::Get()->UnregisterCallback(kEvent, SEOUL_BIND_DELEGATE(&EventsTest::Callback2, this));
	Events::Manager::Get()->TriggerEvent(kEvent, 17, 3.5f);

	SEOUL_UNITTESTING_ASSERT_EQUAL("2", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL("22", m_sCallbackCalls);
}

/**
 * Tests that we can register a callback for an event with 3 arguments, and that
 * we can trigger that event.
 */
void EventsTest::TestEvent3Args()
{
	// Register an event and some callbacks
	Events::Manager::Get()->RegisterCallback(kEvent, SEOUL_BIND_DELEGATE(&StaticCallback3));
	Events::Manager::Get()->RegisterCallback(kEvent, SEOUL_BIND_DELEGATE(&EventsTest::Callback3, this));

	// Trigger the event with valid arguments
	Events::Manager::Get()->TriggerEvent(kEvent, 17, 3.5f, (void *)0x12345678);

	// Make sure callbacks got called the proper number of times
	SEOUL_UNITTESTING_ASSERT_EQUAL("3", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL("3", m_sCallbackCalls);

	// Disable the event, trigger it, and make sure callbacks don't get called
	Events::Manager::Get()->SetEventEnabled(kEvent, false);
	Events::Manager::Get()->TriggerEvent(kEvent, 17, 3.5f, (void *)0x12345678);

	SEOUL_UNITTESTING_ASSERT_EQUAL("3", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL("3", m_sCallbackCalls);

	// Re-enable and remove a callback
	Events::Manager::Get()->SetEventEnabled(kEvent, true);
	Events::Manager::Get()->UnregisterCallback(kEvent, SEOUL_BIND_DELEGATE(&StaticCallback3));
	Events::Manager::Get()->TriggerEvent(kEvent, 17, 3.5f, (void *)0x12345678);

	SEOUL_UNITTESTING_ASSERT_EQUAL("3", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL("33", m_sCallbackCalls);

	// Remove last callback
	Events::Manager::Get()->UnregisterCallback(kEvent, SEOUL_BIND_DELEGATE(&EventsTest::Callback3, this));
	Events::Manager::Get()->TriggerEvent(kEvent, 17, 3.5f, (void *)0x12345678);

	SEOUL_UNITTESTING_ASSERT_EQUAL("3", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL("33", m_sCallbackCalls);
}

/**
 * Tests that we can register a callback for an event with 4 arguments, and that
 * we can trigger that event.
 */
void EventsTest::TestEvent4Args()
{
	// Register an event and some callbacks
	Events::Manager::Get()->RegisterCallback(kEvent, SEOUL_BIND_DELEGATE(&StaticCallback4));
	Events::Manager::Get()->RegisterCallback(kEvent, SEOUL_BIND_DELEGATE(&EventsTest::Callback4, this));

	// Trigger the event with valid arguments
	Events::Manager::Get()->TriggerEvent(kEvent, 17, 3.5f, (void *)0x12345678, "const Byte *");

	// Make sure callbacks got called the proper number of times
	SEOUL_UNITTESTING_ASSERT_EQUAL("4", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL("4", m_sCallbackCalls);

	// Disable the event, trigger it, and make sure callbacks don't get called
	Events::Manager::Get()->SetEventEnabled(kEvent, false);
	Events::Manager::Get()->TriggerEvent(kEvent, 17, 3.5f, (void *)0x12345678, "const Byte *");

	SEOUL_UNITTESTING_ASSERT_EQUAL("4", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL("4", m_sCallbackCalls);

	// Re-enable and remove a callback
	Events::Manager::Get()->SetEventEnabled(kEvent, true);
	Events::Manager::Get()->UnregisterCallback(kEvent, SEOUL_BIND_DELEGATE(&StaticCallback4));
	Events::Manager::Get()->TriggerEvent(kEvent, 17, 3.5f, (void *)0x12345678, "const Byte *");

	SEOUL_UNITTESTING_ASSERT_EQUAL("4", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL("44", m_sCallbackCalls);

	// Remove last callback
	Events::Manager::Get()->UnregisterCallback(kEvent, SEOUL_BIND_DELEGATE(&EventsTest::Callback4, this));
	Events::Manager::Get()->TriggerEvent(kEvent, 17, 3.5f, (void *)0x12345678, "const Byte *");

	SEOUL_UNITTESTING_ASSERT_EQUAL("4", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL("44", m_sCallbackCalls);
}

/**
 * Tests that we can register a callback for an event with 5 arguments, and that
 * we can trigger that event.
 */
void EventsTest::TestEvent5Args()
{
	// Register an event and some callbacks
	Events::Manager::Get()->RegisterCallback(kEvent, SEOUL_BIND_DELEGATE(&StaticCallback5));
	Events::Manager::Get()->RegisterCallback(kEvent, SEOUL_BIND_DELEGATE(&EventsTest::Callback5, this));

	// Trigger the event with valid arguments
	Events::Manager::Get()->TriggerEvent<Int, Float, void *, const Byte *, const String &>(kEvent, 17, 3.5f, (void *)0x12345678, "const Byte *", "String");

	// Make sure callbacks got called the proper number of times
	SEOUL_UNITTESTING_ASSERT_EQUAL("5", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL("5", m_sCallbackCalls);

	// Disable the event, trigger it, and make sure callbacks don't get called
	Events::Manager::Get()->SetEventEnabled(kEvent, false);
	Events::Manager::Get()->TriggerEvent<Int, Float, void *, const Byte *, const String &>(kEvent, 17, 3.5f, (void *)0x12345678, "const Byte *", "String");

	SEOUL_UNITTESTING_ASSERT_EQUAL("5", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL("5", m_sCallbackCalls);

	// Re-enable and remove a callback
	Events::Manager::Get()->SetEventEnabled(kEvent, true);
	Events::Manager::Get()->UnregisterCallback(kEvent, SEOUL_BIND_DELEGATE(&StaticCallback5));
	Events::Manager::Get()->TriggerEvent<Int, Float, void *, const Byte *, const String &>(kEvent, 17, 3.5f, (void *)0x12345678, "const Byte *", "String");

	SEOUL_UNITTESTING_ASSERT_EQUAL("5", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL("55", m_sCallbackCalls);

	// Remove last callback
	Events::Manager::Get()->UnregisterCallback(kEvent, SEOUL_BIND_DELEGATE(&EventsTest::Callback5, this));
	Events::Manager::Get()->TriggerEvent<Int, Float, void *, const Byte *, const String &>(kEvent, 17, 3.5f, (void *)0x12345678, "const Byte *", "String");

	SEOUL_UNITTESTING_ASSERT_EQUAL("5", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL("55", m_sCallbackCalls);
}

/**
 * Tests that we can register a callback for an event with 6 arguments, and that
 * we can trigger that event.
 */
void EventsTest::TestEvent6Args()
{
	// Register an event and some callbacks
	Events::Manager::Get()->RegisterCallback(kEvent, SEOUL_BIND_DELEGATE(&StaticCallback6));
	Events::Manager::Get()->RegisterCallback(kEvent, SEOUL_BIND_DELEGATE(&EventsTest::Callback6, this));

	// Trigger the event with valid arguments
	Events::Manager::Get()->TriggerEvent<Int, Float, void *, const Byte *, const String &, Bool>(kEvent, 17, 3.5f, (void *)0x12345678, "const Byte *", "String", true);

	// Make sure callbacks got called the proper number of times
	SEOUL_UNITTESTING_ASSERT_EQUAL("6", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL("6", m_sCallbackCalls);

	// Disable the event, trigger it, and make sure callbacks don't get called
	Events::Manager::Get()->SetEventEnabled(kEvent, false);
	Events::Manager::Get()->TriggerEvent<Int, Float, void *, const Byte *, const String &, Bool>(kEvent, 17, 3.5f, (void *)0x12345678, "const Byte *", "String", true);

	SEOUL_UNITTESTING_ASSERT_EQUAL("6", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL("6", m_sCallbackCalls);

	// Re-enable and remove a callback
	Events::Manager::Get()->SetEventEnabled(kEvent, true);
	Events::Manager::Get()->UnregisterCallback(kEvent, SEOUL_BIND_DELEGATE(&StaticCallback6));
	Events::Manager::Get()->TriggerEvent<Int, Float, void *, const Byte *, const String &, Bool>(kEvent, 17, 3.5f, (void *)0x12345678, "const Byte *", "String", true);

	SEOUL_UNITTESTING_ASSERT_EQUAL("6", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL("66", m_sCallbackCalls);

	// Remove last callback
	Events::Manager::Get()->UnregisterCallback(kEvent, SEOUL_BIND_DELEGATE(&EventsTest::Callback6, this));
	Events::Manager::Get()->TriggerEvent<Int, Float, void *, const Byte *, const String &, Bool>(kEvent, 17, 3.5f, (void *)0x12345678, "const Byte *", "String", true);

	SEOUL_UNITTESTING_ASSERT_EQUAL("6", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL("66", m_sCallbackCalls);
}

/**
 * Tests that we can register a callback for an event with 7 arguments, and that
 * we can trigger that event.
 */
void EventsTest::TestEvent7Args()
{
	// Register an event and some callbacks
	Events::Manager::Get()->RegisterCallback(kEvent, SEOUL_BIND_DELEGATE(&StaticCallback7));
	Events::Manager::Get()->RegisterCallback(kEvent, SEOUL_BIND_DELEGATE(&EventsTest::Callback7, this));

	// Trigger the event with valid arguments
	Events::Manager::Get()->TriggerEvent<Int, Float, void *, const Byte *, const String &, Bool, Double>(kEvent, 17, 3.5f, (void *)0x12345678, "const Byte *", "String", true, -11.25);

	// Make sure callbacks got called the proper number of times
	SEOUL_UNITTESTING_ASSERT_EQUAL("7", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL("7", m_sCallbackCalls);

	// Disable the event, trigger it, and make sure callbacks don't get called
	Events::Manager::Get()->SetEventEnabled(kEvent, false);
	Events::Manager::Get()->TriggerEvent<Int, Float, void *, const Byte *, const String &, Bool, Double>(kEvent, 17, 3.5f, (void *)0x12345678, "const Byte *", "String", true, -11.25);

	SEOUL_UNITTESTING_ASSERT_EQUAL("7", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL("7", m_sCallbackCalls);

	// Re-enable and remove a callback
	Events::Manager::Get()->SetEventEnabled(kEvent, true);
	Events::Manager::Get()->UnregisterCallback(kEvent, SEOUL_BIND_DELEGATE(&StaticCallback7));
	Events::Manager::Get()->TriggerEvent<Int, Float, void *, const Byte *, const String &, Bool, Double>(kEvent, 17, 3.5f, (void *)0x12345678, "const Byte *", "String", true, -11.25);

	SEOUL_UNITTESTING_ASSERT_EQUAL("7", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL("77", m_sCallbackCalls);

	// Remove last callback
	Events::Manager::Get()->UnregisterCallback(kEvent, SEOUL_BIND_DELEGATE(&EventsTest::Callback7, this));
	Events::Manager::Get()->TriggerEvent<Int, Float, void *, const Byte *, const String &, Bool, Double>(kEvent, 17, 3.5f, (void *)0x12345678, "const Byte *", "String", true, -11.25);

	SEOUL_UNITTESTING_ASSERT_EQUAL("7", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL("77", m_sCallbackCalls);
}

/**
 * Tests that we can register a callback for an event with 8 arguments, and that
 * we can trigger that event.
 */
void EventsTest::TestEvent8Args()
{
	// Register an event and some callbacks
	Events::Manager::Get()->RegisterCallback(kEvent, SEOUL_BIND_DELEGATE(&StaticCallback8));
	Events::Manager::Get()->RegisterCallback(kEvent, SEOUL_BIND_DELEGATE(&EventsTest::Callback8, this));

	// Trigger the event with valid arguments
	Events::Manager::Get()->TriggerEvent<Int, Float, void *, const Byte *, const String &, Bool, Double, UShort>(kEvent, 17, 3.5f, (void *)0x12345678, "const Byte *", "String", true, -11.25, 0xFFFF);

	// Make sure callbacks got called the proper number of times
	SEOUL_UNITTESTING_ASSERT_EQUAL("8", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL("8", m_sCallbackCalls);

	// Disable the event, trigger it, and make sure callbacks don't get called
	Events::Manager::Get()->SetEventEnabled(kEvent, false);
	Events::Manager::Get()->TriggerEvent<Int, Float, void *, const Byte *, const String &, Bool, Double, UShort>(kEvent, 17, 3.5f, (void *)0x12345678, "const Byte *", "String", true, -11.25, 0xFFFF);

	SEOUL_UNITTESTING_ASSERT_EQUAL("8", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL("8", m_sCallbackCalls);

	// Re-enable and remove a callback
	Events::Manager::Get()->SetEventEnabled(kEvent, true);
	Events::Manager::Get()->UnregisterCallback(kEvent, SEOUL_BIND_DELEGATE(&StaticCallback8));
	Events::Manager::Get()->TriggerEvent<Int, Float, void *, const Byte *, const String &, Bool, Double, UShort>(kEvent, 17, 3.5f, (void *)0x12345678, "const Byte *", "String", true, -11.25, 0xFFFF);

	SEOUL_UNITTESTING_ASSERT_EQUAL("8", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL("88", m_sCallbackCalls);

	// Remove last callback
	Events::Manager::Get()->UnregisterCallback(kEvent, SEOUL_BIND_DELEGATE(&EventsTest::Callback8, this));
	Events::Manager::Get()->TriggerEvent<Int, Float, void *, const Byte *, const String &, Bool, Double, UShort>(kEvent, 17, 3.5f, (void *)0x12345678, "const Byte *", "String", true, -11.25, 0xFFFF);

	SEOUL_UNITTESTING_ASSERT_EQUAL("8", s_sStaticCallbackCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL("88", m_sCallbackCalls);
}

/**
 * Static callback, specifically for handled event testing.
 */
Bool EventsTest::StaticHandledCallback0()
{
	s_sStaticCallbackCalls += "H";
	return true;
}

/**
 * Static callback with 0 arguments used to test the game event system
 */
Bool EventsTest::StaticCallback0()
{
	s_sStaticCallbackCalls += "0";
	return false;
}

/**
 * Static callback with 1 arguments used to test the game event system
 */
void EventsTest::StaticCallback1(Int arg1)
{
	SEOUL_UNITTESTING_ASSERT_EQUAL((Int)17, arg1);

	s_sStaticCallbackCalls += "1";
}

/**
 * Static callback with 2 arguments used to test the game event system
 */
Bool EventsTest::StaticCallback2(Int arg1, Float arg2)
{
	SEOUL_UNITTESTING_ASSERT_EQUAL((Int)17, arg1);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3.5f, arg2);

	s_sStaticCallbackCalls += "2";
	return false;
}

/**
 * Static callback with 3 arguments used to test the game event system
 */
void EventsTest::StaticCallback3(Int arg1, Float arg2, void *arg3)
{
	SEOUL_UNITTESTING_ASSERT_EQUAL((Int)17, arg1);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3.5f, arg2);
	SEOUL_UNITTESTING_ASSERT_EQUAL((void *)0x12345678, arg3);

	s_sStaticCallbackCalls += "3";
}

/**
 * Static callback with 4 arguments used to test the game event system
 */
Bool EventsTest::StaticCallback4(Int arg1, Float arg2, void *arg3, const Byte *arg4)
{
	SEOUL_UNITTESTING_ASSERT_EQUAL((Int)17, arg1);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3.5f, arg2);
	SEOUL_UNITTESTING_ASSERT_EQUAL((void *)0x12345678, arg3);
	SEOUL_UNITTESTING_ASSERT(strcmp(arg4, "const Byte *") == 0);

	s_sStaticCallbackCalls += "4";
	return false;
}

/**
 * Static callback with 5 arguments used to test the game event system
 */
void EventsTest::StaticCallback5(Int arg1, Float arg2, void *arg3, const Byte *arg4, const String & arg5)
{
	SEOUL_UNITTESTING_ASSERT_EQUAL((Int)17, arg1);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3.5f, arg2);
	SEOUL_UNITTESTING_ASSERT_EQUAL((void *)0x12345678, arg3);
	SEOUL_UNITTESTING_ASSERT(strcmp(arg4, "const Byte *") == 0);
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("String"), arg5);

	s_sStaticCallbackCalls += "5";
}

/**
 * Static callback with 6 arguments used to test the game event system
 */
Bool EventsTest::StaticCallback6(Int arg1, Float arg2, void *arg3, const Byte *arg4, const String & arg5, Bool arg6)
{
	SEOUL_UNITTESTING_ASSERT_EQUAL((Int)17, arg1);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3.5f, arg2);
	SEOUL_UNITTESTING_ASSERT_EQUAL((void *)0x12345678, arg3);
	SEOUL_UNITTESTING_ASSERT(strcmp(arg4, "const Byte *") == 0);
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("String"), arg5);
	SEOUL_UNITTESTING_ASSERT_EQUAL(true, arg6);

	s_sStaticCallbackCalls += "6";
	return false;
}

/**
 * Static callback with 7 arguments used to test the game event system
 */
void EventsTest::StaticCallback7(Int arg1, Float arg2, void *arg3, const Byte *arg4, const String & arg5, Bool arg6, Double arg7)
{
	SEOUL_UNITTESTING_ASSERT_EQUAL((Int)17, arg1);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3.5f, arg2);
	SEOUL_UNITTESTING_ASSERT_EQUAL((void *)0x12345678, arg3);
	SEOUL_UNITTESTING_ASSERT(strcmp(arg4, "const Byte *") == 0);
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("String"), arg5);
	SEOUL_UNITTESTING_ASSERT_EQUAL(true, arg6);
	SEOUL_UNITTESTING_ASSERT_EQUAL(-11.25, arg7);

	s_sStaticCallbackCalls += "7";
}

/**
 * Static callback with 8 arguments used to test the game event system
 */
Bool EventsTest::StaticCallback8(Int arg1, Float arg2, void *arg3, const Byte *arg4, const String & arg5, Bool arg6, Double arg7, const UShort arg8)
{
	SEOUL_UNITTESTING_ASSERT_EQUAL((Int)17, arg1);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3.5f, arg2);
	SEOUL_UNITTESTING_ASSERT_EQUAL((void *)0x12345678, arg3);
	SEOUL_UNITTESTING_ASSERT(strcmp(arg4, "const Byte *") == 0);
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("String"), arg5);
	SEOUL_UNITTESTING_ASSERT_EQUAL(true, arg6);
	SEOUL_UNITTESTING_ASSERT_EQUAL(-11.25, arg7);
	SEOUL_UNITTESTING_ASSERT_EQUAL((UShort)0xFFFF, arg8);

	s_sStaticCallbackCalls += "8";
	return false;
}

/**
 * Class method callback with 0 arguments used to test the game event system
 */
void EventsTest::Callback0()
{
	m_sCallbackCalls += "0";
}

/**
 * Class method callback with 1 arguments used to test the game event system
 */
Bool EventsTest::Callback1(Int arg1)
{
	SEOUL_UNITTESTING_ASSERT_EQUAL((Int)17, arg1);

	m_sCallbackCalls += "1";
	return false;
}

/**
 * Class method callback with 2 arguments used to test the game event system
 */
void EventsTest::Callback2(Int arg1, Float arg2)
{
	SEOUL_UNITTESTING_ASSERT_EQUAL((Int)17, arg1);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3.5f, arg2);

	m_sCallbackCalls += "2";
}

/**
 * Class method callback with 3 arguments used to test the game event system
 */
Bool EventsTest::Callback3(Int arg1, Float arg2, void *arg3)
{
	SEOUL_UNITTESTING_ASSERT_EQUAL((Int)17, arg1);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3.5f, arg2);
	SEOUL_UNITTESTING_ASSERT_EQUAL((void *)0x12345678, arg3);

	m_sCallbackCalls += "3";
	return false;
}

/**
 * Class method callback with 4 arguments used to test the game event system
 */
void EventsTest::Callback4(Int arg1, Float arg2, void *arg3, const Byte *arg4)
{
	SEOUL_UNITTESTING_ASSERT_EQUAL((Int)17, arg1);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3.5f, arg2);
	SEOUL_UNITTESTING_ASSERT_EQUAL((void *)0x12345678, arg3);
	SEOUL_UNITTESTING_ASSERT(strcmp(arg4, "const Byte *") == 0);

	m_sCallbackCalls += "4";
}

/**
 * Class method callback with 5 arguments used to test the game event system
 */
Bool EventsTest::Callback5(Int arg1, Float arg2, void *arg3, const Byte *arg4, const String & arg5)
{
	SEOUL_UNITTESTING_ASSERT_EQUAL((Int)17, arg1);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3.5f, arg2);
	SEOUL_UNITTESTING_ASSERT_EQUAL((void *)0x12345678, arg3);
	SEOUL_UNITTESTING_ASSERT(strcmp(arg4, "const Byte *") == 0);
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("String"), arg5);

	m_sCallbackCalls += "5";
	return false;
}

/**
 * Class method callback with 6 arguments used to test the game event system
 */
void EventsTest::Callback6(Int arg1, Float arg2, void *arg3, const Byte *arg4, const String & arg5, Bool arg6)
{
	SEOUL_UNITTESTING_ASSERT_EQUAL((Int)17, arg1);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3.5f, arg2);
	SEOUL_UNITTESTING_ASSERT_EQUAL((void *)0x12345678, arg3);
	SEOUL_UNITTESTING_ASSERT(strcmp(arg4, "const Byte *") == 0);
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("String"), arg5);
	SEOUL_UNITTESTING_ASSERT_EQUAL(true, arg6);

	m_sCallbackCalls += "6";
}

/**
 * Class method callback with 7 arguments used to test the game event system
 */
Bool EventsTest::Callback7(Int arg1, Float arg2, void *arg3, const Byte *arg4, const String & arg5, Bool arg6, Double arg7)
{
	SEOUL_UNITTESTING_ASSERT_EQUAL((Int)17, arg1);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3.5f, arg2);
	SEOUL_UNITTESTING_ASSERT_EQUAL((void *)0x12345678, arg3);
	SEOUL_UNITTESTING_ASSERT(strcmp(arg4, "const Byte *") == 0);
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("String"), arg5);
	SEOUL_UNITTESTING_ASSERT_EQUAL(true, arg6);
	SEOUL_UNITTESTING_ASSERT_EQUAL(-11.25, arg7);

	m_sCallbackCalls += "7";
	return false;
}

/**
 * Class method callback with 8 arguments used to test the game event system
 */
void EventsTest::Callback8(Int arg1, Float arg2, void *arg3, const Byte *arg4, const String & arg5, Bool arg6, Double arg7, const UShort arg8)
{
	SEOUL_UNITTESTING_ASSERT_EQUAL((Int)17, arg1);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3.5f, arg2);
	SEOUL_UNITTESTING_ASSERT_EQUAL((void *)0x12345678, arg3);
	SEOUL_UNITTESTING_ASSERT(strcmp(arg4, "const Byte *") == 0);
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("String"), arg5);
	SEOUL_UNITTESTING_ASSERT_EQUAL(true, arg6);
	SEOUL_UNITTESTING_ASSERT_EQUAL(-11.25, arg7);
	SEOUL_UNITTESTING_ASSERT_EQUAL((UShort)0xFFFF, arg8);

	m_sCallbackCalls += "8";
}

/**
 * Static callback with an invalid method signature used to test the game event
 * system.  This should never be called, since its signature is incompatible
 * with the events registered in the tests.
 */
Bool EventsTest::InvalidStaticCallback(void *arg)
{
	SEOUL_UNITTESTING_FAIL("Callback with invalid signature should not have been called");
	return false;
}

/**
 * Class method callback with an invalid method signature used to test the game
 * event system.  This should never be called, since its signature is
 * incompatible with the events registered in the tests.
 */
Bool EventsTest::InvalidCallback(void *arg)
{
	SEOUL_UNITTESTING_FAIL("Callback with invalid signature should not have been called");
	return false;
}

#endif // /#if SEOUL_UNIT_TESTS

} // namespace Seoul
