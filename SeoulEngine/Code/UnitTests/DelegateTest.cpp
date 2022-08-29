/**
 * \file DelegateTest.cpp
 * \brief Tests for the Delegate class.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Delegate.h"
#include "DelegateTest.h"
#include "ReflectionDefine.h"
#include "ScopedAction.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(DelegateTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestDefault)
	SEOUL_METHOD(TestApi0)
	SEOUL_METHOD(TestApi0Instance)
	SEOUL_METHOD(TestApi0ConstInstance)
	SEOUL_METHOD(TestApi0ImplicitArg)
	SEOUL_METHOD(TestApi1)
	SEOUL_METHOD(TestApi1Instance)
	SEOUL_METHOD(TestApi1ConstInstance)
	SEOUL_METHOD(TestApi1ImplicitArg)
	SEOUL_METHOD(TestDanglingDelegate)
SEOUL_END_TYPE();

static String s_sImplicit;
static String s_sStatic;
static String s_sInstance;
static String s_sConstInstance;

void DelegateTest::TestDefault()
{
	Delegate<void()> del;
	SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, del.GetCaller());
	SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, del.GetObject());
	SEOUL_UNITTESTING_ASSERT(!del);
	SEOUL_UNITTESTING_ASSERT(!del.IsValid());
	del.Reset();
	SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, del.GetCaller());
	SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, del.GetObject());
}

void DelegateTest::TestApi0()
{
	auto const deferred(MakeDeferredAction([&]() { ClearCalls(); }));

	auto const a = SEOUL_BIND_DELEGATE(&DelegateTest::Static0);
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, a.GetCaller());
	SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, a.GetObject());
	SEOUL_UNITTESTING_ASSERT(a);
	SEOUL_UNITTESTING_ASSERT(a.IsValid());
	auto const b = a;
	SEOUL_UNITTESTING_ASSERT_EQUAL(b.GetCaller(), a.GetCaller());
	SEOUL_UNITTESTING_ASSERT_EQUAL(b.GetObject(), a.GetObject());
	SEOUL_UNITTESTING_ASSERT(b == a);
	SEOUL_UNITTESTING_ASSERT(b);
	SEOUL_UNITTESTING_ASSERT(b.IsValid());
	auto const c(a);
	SEOUL_UNITTESTING_ASSERT_EQUAL(c.GetCaller(), a.GetCaller());
	SEOUL_UNITTESTING_ASSERT_EQUAL(c.GetObject(), a.GetObject());
	SEOUL_UNITTESTING_ASSERT(c == a);
	SEOUL_UNITTESTING_ASSERT(c);
	SEOUL_UNITTESTING_ASSERT(c.IsValid());
	const Delegate<void()> d;
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(d.GetCaller(), a.GetCaller());
	SEOUL_UNITTESTING_ASSERT_EQUAL(d.GetObject(), a.GetObject());
	SEOUL_UNITTESTING_ASSERT(d != a);
	SEOUL_UNITTESTING_ASSERT(!d);
	SEOUL_UNITTESTING_ASSERT(!d.IsValid());

	a();
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sImplicit);
	SEOUL_UNITTESTING_ASSERT_EQUAL("S0", s_sStatic);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sInstance);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sConstInstance);
	b();
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sImplicit);
	SEOUL_UNITTESTING_ASSERT_EQUAL("S0S0", s_sStatic);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sInstance);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sConstInstance);
	c();
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sImplicit);
	SEOUL_UNITTESTING_ASSERT_EQUAL("S0S0S0", s_sStatic);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sInstance);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sConstInstance);
}

void DelegateTest::TestApi0Instance()
{
	auto const deferred(MakeDeferredAction([&]() { ClearCalls(); }));

	auto const a = SEOUL_BIND_DELEGATE(&DelegateTest::Instance0, this);
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, a.GetCaller());
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, a.GetObject());
	SEOUL_UNITTESTING_ASSERT(a);
	SEOUL_UNITTESTING_ASSERT(a.IsValid());
	auto const b = a;
	SEOUL_UNITTESTING_ASSERT_EQUAL(b.GetCaller(), a.GetCaller());
	SEOUL_UNITTESTING_ASSERT_EQUAL(b.GetObject(), a.GetObject());
	SEOUL_UNITTESTING_ASSERT(b == a);
	SEOUL_UNITTESTING_ASSERT(b);
	SEOUL_UNITTESTING_ASSERT(b.IsValid());
	auto const c(a);
	SEOUL_UNITTESTING_ASSERT_EQUAL(c.GetCaller(), a.GetCaller());
	SEOUL_UNITTESTING_ASSERT_EQUAL(c.GetObject(), a.GetObject());
	SEOUL_UNITTESTING_ASSERT(c == a);
	SEOUL_UNITTESTING_ASSERT(c);
	SEOUL_UNITTESTING_ASSERT(c.IsValid());
	const Delegate<void()> d;
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(d.GetCaller(), a.GetCaller());
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(d.GetObject(), a.GetObject());
	SEOUL_UNITTESTING_ASSERT(d != a);
	SEOUL_UNITTESTING_ASSERT(!d);
	SEOUL_UNITTESTING_ASSERT(!d.IsValid());

	a();
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sImplicit);
	SEOUL_UNITTESTING_ASSERT_EQUAL("I0", s_sInstance);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sStatic);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sConstInstance);
	b();
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sImplicit);
	SEOUL_UNITTESTING_ASSERT_EQUAL("I0I0", s_sInstance);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sStatic);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sConstInstance);
	c();
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sImplicit);
	SEOUL_UNITTESTING_ASSERT_EQUAL("I0I0I0", s_sInstance);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sStatic);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sConstInstance);
}

void DelegateTest::TestApi0ConstInstance()
{
	auto const deferred(MakeDeferredAction([&]() { ClearCalls(); }));

	auto const a = SEOUL_BIND_DELEGATE(&DelegateTest::ConstInstance0, this);
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, a.GetCaller());
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, a.GetObject());
	SEOUL_UNITTESTING_ASSERT(a);
	SEOUL_UNITTESTING_ASSERT(a.IsValid());
	auto const b = a;
	SEOUL_UNITTESTING_ASSERT_EQUAL(b.GetCaller(), a.GetCaller());
	SEOUL_UNITTESTING_ASSERT_EQUAL(b.GetObject(), a.GetObject());
	SEOUL_UNITTESTING_ASSERT(b == a);
	SEOUL_UNITTESTING_ASSERT(b);
	SEOUL_UNITTESTING_ASSERT(b.IsValid());
	auto const c(a);
	SEOUL_UNITTESTING_ASSERT_EQUAL(c.GetCaller(), a.GetCaller());
	SEOUL_UNITTESTING_ASSERT_EQUAL(c.GetObject(), a.GetObject());
	SEOUL_UNITTESTING_ASSERT(c == a);
	SEOUL_UNITTESTING_ASSERT(c);
	SEOUL_UNITTESTING_ASSERT(c.IsValid());
	const Delegate<void()> d;
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(d.GetCaller(), a.GetCaller());
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(d.GetObject(), a.GetObject());
	SEOUL_UNITTESTING_ASSERT(d != a);
	SEOUL_UNITTESTING_ASSERT(!d);
	SEOUL_UNITTESTING_ASSERT(!d.IsValid());

	a();
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sImplicit);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sInstance);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sStatic);
	SEOUL_UNITTESTING_ASSERT_EQUAL("C0", s_sConstInstance);
	b();
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sImplicit);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sInstance);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sStatic);
	SEOUL_UNITTESTING_ASSERT_EQUAL("C0C0", s_sConstInstance);
	c();
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sImplicit);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sInstance);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sStatic);
	SEOUL_UNITTESTING_ASSERT_EQUAL("C0C0C0", s_sConstInstance);
}

void DelegateTest::TestApi0ImplicitArg()
{
	auto const deferred(MakeDeferredAction([&]() { ClearCalls(); }));

	auto const a = SEOUL_BIND_DELEGATE(&DelegateTest::ImplicitArg0, (void*)(size_t)7);
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, a.GetCaller());
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, a.GetObject());
	SEOUL_UNITTESTING_ASSERT(a);
	SEOUL_UNITTESTING_ASSERT(a.IsValid());
	auto const b = a;
	SEOUL_UNITTESTING_ASSERT_EQUAL(b.GetCaller(), a.GetCaller());
	SEOUL_UNITTESTING_ASSERT_EQUAL(b.GetObject(), a.GetObject());
	SEOUL_UNITTESTING_ASSERT(b == a);
	SEOUL_UNITTESTING_ASSERT(b);
	SEOUL_UNITTESTING_ASSERT(b.IsValid());
	auto const c(a);
	SEOUL_UNITTESTING_ASSERT_EQUAL(c.GetCaller(), a.GetCaller());
	SEOUL_UNITTESTING_ASSERT_EQUAL(c.GetObject(), a.GetObject());
	SEOUL_UNITTESTING_ASSERT(c == a);
	SEOUL_UNITTESTING_ASSERT(c);
	SEOUL_UNITTESTING_ASSERT(c.IsValid());
	const Delegate<void()> d;
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(d.GetCaller(), a.GetCaller());
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(d.GetObject(), a.GetObject());
	SEOUL_UNITTESTING_ASSERT(d != a);
	SEOUL_UNITTESTING_ASSERT(!d);
	SEOUL_UNITTESTING_ASSERT(!d.IsValid());

	a();
	SEOUL_UNITTESTING_ASSERT_EQUAL("A0", s_sImplicit);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sInstance);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sStatic);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sConstInstance);
	b();
	SEOUL_UNITTESTING_ASSERT_EQUAL("A0A0", s_sImplicit);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sInstance);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sStatic);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sConstInstance);
	c();
	SEOUL_UNITTESTING_ASSERT_EQUAL("A0A0A0", s_sImplicit);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sInstance);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sStatic);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sConstInstance);
}

void DelegateTest::TestApi1()
{
	auto const deferred(MakeDeferredAction([&]() { ClearCalls(); }));

	auto const a = SEOUL_BIND_DELEGATE(&DelegateTest::Static1);
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, a.GetCaller());
	SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, a.GetObject());
	SEOUL_UNITTESTING_ASSERT(a);
	SEOUL_UNITTESTING_ASSERT(a.IsValid());
	auto const b = a;
	SEOUL_UNITTESTING_ASSERT_EQUAL(b.GetCaller(), a.GetCaller());
	SEOUL_UNITTESTING_ASSERT_EQUAL(b.GetObject(), a.GetObject());
	SEOUL_UNITTESTING_ASSERT(b == a);
	SEOUL_UNITTESTING_ASSERT(b);
	SEOUL_UNITTESTING_ASSERT(b.IsValid());
	auto const c(a);
	SEOUL_UNITTESTING_ASSERT_EQUAL(c.GetCaller(), a.GetCaller());
	SEOUL_UNITTESTING_ASSERT_EQUAL(c.GetObject(), a.GetObject());
	SEOUL_UNITTESTING_ASSERT(c == a);
	SEOUL_UNITTESTING_ASSERT(c);
	SEOUL_UNITTESTING_ASSERT(c.IsValid());
	const Delegate<void(Int)> d;
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(d.GetCaller(), a.GetCaller());
	SEOUL_UNITTESTING_ASSERT_EQUAL(d.GetObject(), a.GetObject());
	SEOUL_UNITTESTING_ASSERT(d != a);
	SEOUL_UNITTESTING_ASSERT(!d);
	SEOUL_UNITTESTING_ASSERT(!d.IsValid());

	a(3);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sImplicit);
	SEOUL_UNITTESTING_ASSERT_EQUAL("S3", s_sStatic);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sInstance);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sConstInstance);
	b(5);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sImplicit);
	SEOUL_UNITTESTING_ASSERT_EQUAL("S3S5", s_sStatic);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sInstance);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sConstInstance);
	c(4);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sImplicit);
	SEOUL_UNITTESTING_ASSERT_EQUAL("S3S5S4", s_sStatic);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sInstance);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sConstInstance);
}

void DelegateTest::TestApi1Instance()
{
	auto const deferred(MakeDeferredAction([&]() { ClearCalls(); }));

	auto const a = SEOUL_BIND_DELEGATE(&DelegateTest::Instance1, this);
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, a.GetCaller());
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, a.GetObject());
	SEOUL_UNITTESTING_ASSERT(a);
	SEOUL_UNITTESTING_ASSERT(a.IsValid());
	auto const b = a;
	SEOUL_UNITTESTING_ASSERT_EQUAL(b.GetCaller(), a.GetCaller());
	SEOUL_UNITTESTING_ASSERT_EQUAL(b.GetObject(), a.GetObject());
	SEOUL_UNITTESTING_ASSERT(b == a);
	SEOUL_UNITTESTING_ASSERT(b);
	SEOUL_UNITTESTING_ASSERT(b.IsValid());
	auto const c(a);
	SEOUL_UNITTESTING_ASSERT_EQUAL(c.GetCaller(), a.GetCaller());
	SEOUL_UNITTESTING_ASSERT_EQUAL(c.GetObject(), a.GetObject());
	SEOUL_UNITTESTING_ASSERT(c == a);
	SEOUL_UNITTESTING_ASSERT(c);
	SEOUL_UNITTESTING_ASSERT(c.IsValid());
	const Delegate<void(Int)> d;
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(d.GetCaller(), a.GetCaller());
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(d.GetObject(), a.GetObject());
	SEOUL_UNITTESTING_ASSERT(d != a);
	SEOUL_UNITTESTING_ASSERT(!d);
	SEOUL_UNITTESTING_ASSERT(!d.IsValid());

	a(3);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sImplicit);
	SEOUL_UNITTESTING_ASSERT_EQUAL("I3", s_sInstance);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sStatic);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sConstInstance);
	b(5);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sImplicit);
	SEOUL_UNITTESTING_ASSERT_EQUAL("I3I5", s_sInstance);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sStatic);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sConstInstance);
	c(4);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sImplicit);
	SEOUL_UNITTESTING_ASSERT_EQUAL("I3I5I4", s_sInstance);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sStatic);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sConstInstance);
}

void DelegateTest::TestApi1ConstInstance()
{
	auto const deferred(MakeDeferredAction([&]() { ClearCalls(); }));

	auto const a = SEOUL_BIND_DELEGATE(&DelegateTest::ConstInstance1, this);
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, a.GetCaller());
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, a.GetObject());
	SEOUL_UNITTESTING_ASSERT(a);
	SEOUL_UNITTESTING_ASSERT(a.IsValid());
	auto const b = a;
	SEOUL_UNITTESTING_ASSERT_EQUAL(b.GetCaller(), a.GetCaller());
	SEOUL_UNITTESTING_ASSERT_EQUAL(b.GetObject(), a.GetObject());
	SEOUL_UNITTESTING_ASSERT(b == a);
	SEOUL_UNITTESTING_ASSERT(b);
	SEOUL_UNITTESTING_ASSERT(b.IsValid());
	auto const c(a);
	SEOUL_UNITTESTING_ASSERT_EQUAL(c.GetCaller(), a.GetCaller());
	SEOUL_UNITTESTING_ASSERT_EQUAL(c.GetObject(), a.GetObject());
	SEOUL_UNITTESTING_ASSERT(c == a);
	SEOUL_UNITTESTING_ASSERT(c);
	SEOUL_UNITTESTING_ASSERT(c.IsValid());
	const Delegate<void(Int)> d;
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(d.GetCaller(), a.GetCaller());
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(d.GetObject(), a.GetObject());
	SEOUL_UNITTESTING_ASSERT(d != a);
	SEOUL_UNITTESTING_ASSERT(!d);
	SEOUL_UNITTESTING_ASSERT(!d.IsValid());

	a(3);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sImplicit);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sInstance);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sStatic);
	SEOUL_UNITTESTING_ASSERT_EQUAL("C3", s_sConstInstance);
	b(5);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sImplicit);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sInstance);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sStatic);
	SEOUL_UNITTESTING_ASSERT_EQUAL("C3C5", s_sConstInstance);
	c(4);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sImplicit);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sInstance);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sStatic);
	SEOUL_UNITTESTING_ASSERT_EQUAL("C3C5C4", s_sConstInstance);
}

void DelegateTest::TestApi1ImplicitArg()
{
	auto const deferred(MakeDeferredAction([&]() { ClearCalls(); }));

	auto const a = SEOUL_BIND_DELEGATE(&DelegateTest::ImplicitArg1, (void*)(size_t)7);
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, a.GetCaller());
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(nullptr, a.GetObject());
	SEOUL_UNITTESTING_ASSERT(a);
	SEOUL_UNITTESTING_ASSERT(a.IsValid());
	auto const b = a;
	SEOUL_UNITTESTING_ASSERT_EQUAL(b.GetCaller(), a.GetCaller());
	SEOUL_UNITTESTING_ASSERT_EQUAL(b.GetObject(), a.GetObject());
	SEOUL_UNITTESTING_ASSERT(b == a);
	SEOUL_UNITTESTING_ASSERT(b);
	SEOUL_UNITTESTING_ASSERT(b.IsValid());
	auto const c(a);
	SEOUL_UNITTESTING_ASSERT_EQUAL(c.GetCaller(), a.GetCaller());
	SEOUL_UNITTESTING_ASSERT_EQUAL(c.GetObject(), a.GetObject());
	SEOUL_UNITTESTING_ASSERT(c == a);
	SEOUL_UNITTESTING_ASSERT(c);
	SEOUL_UNITTESTING_ASSERT(c.IsValid());
	const Delegate<void(Int)> d;
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(d.GetCaller(), a.GetCaller());
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(d.GetObject(), a.GetObject());
	SEOUL_UNITTESTING_ASSERT(d != a);
	SEOUL_UNITTESTING_ASSERT(!d);
	SEOUL_UNITTESTING_ASSERT(!d.IsValid());

	a(3);
	SEOUL_UNITTESTING_ASSERT_EQUAL("A3", s_sImplicit);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sInstance);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sStatic);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sConstInstance);
	b(5);
	SEOUL_UNITTESTING_ASSERT_EQUAL("A3A5", s_sImplicit);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sInstance);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sStatic);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sConstInstance);
	c(4);
	SEOUL_UNITTESTING_ASSERT_EQUAL("A3A5A4", s_sImplicit);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sInstance);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sStatic);
	SEOUL_UNITTESTING_ASSERT_EQUAL("", s_sConstInstance);
}

namespace
{

struct DangleTester
{
	SEOUL_DELEGATE_TARGET(DangleTester);

	void Method()
	{
		SEOUL_UNITTESTING_ASSERT(false); // Not called.
	}
};

} // namespace anonymous

void DelegateTest::TestDanglingDelegate()
{
	Delegate<void()> d;
	{
		DangleTester tester;
		d = SEOUL_BIND_DELEGATE(&DangleTester::Method, &tester);
	}

	// Internal conversion knowledge to test handle test.
	auto const h = DelegateMemberBindHandle::ToHandle(d.GetObject());
	SEOUL_UNITTESTING_ASSERT(h.IsValid());
	auto const p = DelegateMemberBindHandleAnchorGlobal::GetPointer<DangleTester>(h);
	SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, p);
}

void DelegateTest::ImplicitArg0(void* p)
{
	SEOUL_UNITTESTING_ASSERT_EQUAL((void*)(size_t)7, p);
	s_sImplicit.Append("A0");
}

void DelegateTest::Static0()
{
	s_sStatic.Append("S0");
}

void DelegateTest::Instance0()
{
	s_sInstance.Append("I0");
}

void DelegateTest::ConstInstance0() const
{
	s_sConstInstance.Append("C0");
}

void DelegateTest::ImplicitArg1(void* p, Int i)
{
	SEOUL_UNITTESTING_ASSERT_EQUAL((void*)(size_t)7, p);
	s_sImplicit.Append(String::Printf("A%d", i));
}

void DelegateTest::Static1(Int i)
{
	s_sStatic.Append(String::Printf("S%d", i));
}

void DelegateTest::Instance1(Int i)
{
	s_sInstance.Append(String::Printf("I%d", i));
}

void DelegateTest::ConstInstance1(Int i) const
{
	s_sConstInstance.Append(String::Printf("C%d", i));
}

void DelegateTest::ClearCalls()
{
	s_sImplicit.Clear();
	s_sStatic.Clear();
	s_sInstance.Clear();
	s_sConstInstance.Clear();
}

#endif // /#if SEOUL_UNIT_TESTS

} // namespace Seoul
