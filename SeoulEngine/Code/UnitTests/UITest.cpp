/**
 * \file UITest.cpp
 * \brief Unit test for functionality in the UI project.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FalconInstance.h"
#include "Logger.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"
#include "UIMotion.h"
#include "UIMotionCollection.h"
#include "UITween.h"
#include "UITest.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(UITest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestMotionCancel)
	SEOUL_METHOD(TestMotionCancelAll)

	SEOUL_METHOD(TestTweensBasic)
	SEOUL_METHOD(TestTweensCancel)
	SEOUL_METHOD(TestTweensCancelAll)
	SEOUL_METHOD(TestTweensValues)
SEOUL_END_TYPE()

namespace
{

static Atomic32 s_CompletionInterfaceCount(0);
class UITestTweenCompletionInterface SEOUL_SEALED : public UI::TweenCompletionInterface
{
public:
	UITestTweenCompletionInterface(UI::TweenCollection& rCollection, Int iTweenToCancel = -1)
		: m_rCollection(rCollection)
		, m_iTweenToCancel(iTweenToCancel)
		, m_bCompleted(false)
	{
		++s_CompletionInterfaceCount;
	}

	~UITestTweenCompletionInterface()
	{
		--s_CompletionInterfaceCount;
	}

	Bool IsComplete() const
	{
		return m_bCompleted;
	}

	virtual void OnComplete() SEOUL_OVERRIDE
	{
		if (m_iTweenToCancel >= 0)
		{
			m_rCollection.CancelTween(m_iTweenToCancel);
		}

		m_bCompleted = true;
	}

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(UITestTweenCompletionInterface);

	UI::TweenCollection& m_rCollection;
	Int const m_iTweenToCancel;
	Bool m_bCompleted;
}; // class UITestTweenCompletionInterface

class UITestFalconInstance SEOUL_SEALED : public Falcon::Instance
{
public:
	UITestFalconInstance()
		: Falcon::Instance(0u)
		, m_fDepth3D(0.0f)
	{
	}

	~UITestFalconInstance()
	{
	}

	virtual Instance* Clone(Falcon::AddInterface& rInterface) const SEOUL_OVERRIDE
	{
		return SEOUL_NEW(MemoryBudgets::Developer) UITestFalconInstance;
	}

	virtual Bool ComputeLocalBounds(Falcon::Rectangle& rBounds) SEOUL_OVERRIDE
	{
		return false;
	}

	virtual Float GetDepth3D() const SEOUL_OVERRIDE
	{
		return m_fDepth3D;
	}

	virtual Falcon::InstanceType GetType() const SEOUL_OVERRIDE
	{
		return Falcon::InstanceType::kCustom;
	}

	virtual Bool HitTest(
		const Matrix2x3& mParent,
		Float32 fWorldX,
		Float32 fWorldY,
		Bool bIgnoreVisibility /*= false*/) const SEOUL_OVERRIDE
	{
		return false;
	}

	virtual void SetDepth3D(Float fDepth3D) SEOUL_OVERRIDE
	{
		m_fDepth3D = fDepth3D;
	}

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(UITestFalconInstance);

	Float m_fDepth3D;
}; // class UITestFalconInstance

class UITestAdvanceInterface SEOUL_SEALED : public Falcon::AdvanceInterface
{
public:
	UITestAdvanceInterface()
	{
	}

	~UITestAdvanceInterface()
	{
	}

	virtual void FalconDispatchEnterFrameEvent(
		Falcon::Instance* pInstance) SEOUL_OVERRIDE
	{
	}

	virtual void FalconDispatchEvent(
		const HString& sEventName,
		Falcon::SimpleActions::EventType eType,
		Falcon::Instance* pInstance) SEOUL_OVERRIDE
	{
	}

	virtual Float FalconGetDeltaTimeInSeconds() const SEOUL_OVERRIDE
	{
		return 0.0f;
	}

	virtual Bool FalconLocalize(
		const HString& sLocalizationToken,
		String& rsLocalizedText) SEOUL_OVERRIDE
	{
		return false;
	}

	virtual void FalconOnAddToParent(
		Falcon::MovieClipInstance* pParent,
		Falcon::Instance* pInstance,
		const HString& sClassName) SEOUL_OVERRIDE
	{
	}

	virtual void FalconOnClone(
		Falcon::Instance const* pFromInstance,
		Falcon::Instance* pToInstance) SEOUL_OVERRIDE
	{
	}
}; // class UITestAdvanceInterface

class MotionCompletion SEOUL_SEALED : public UI::MotionCompletionInterface
{
public:
	MotionCompletion(UI::MotionCollection& rCollection, Int iMotionToCancel = -1)
		: m_rCollection(rCollection)
		, m_iMotionToCancel(iMotionToCancel)
		, m_bCompleted(false)
	{
		++s_CompletionInterfaceCount;
	}

	~MotionCompletion()
	{
		--s_CompletionInterfaceCount;
	}

	Bool IsComplete() const
	{
		return m_bCompleted;
	}

	virtual void OnComplete() SEOUL_OVERRIDE
	{
		if (m_iMotionToCancel >= 0)
		{
			m_rCollection.CancelMotion(m_iMotionToCancel);
		}

		m_bCompleted = true;
	}

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(MotionCompletion);

	UI::MotionCollection& m_rCollection;
	Int const m_iMotionToCancel;
	Bool m_bCompleted;
}; // class MotionCompletion

class TestMotion SEOUL_SEALED : public UI::Motion
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(Motion);

	TestMotion()
		: m_fAccumTime(0.0f)
		, m_fDuration(0.0f)
	{
	}

	~TestMotion()
	{
	}

	virtual Bool Advance(Float fDeltaTimeInSeconds) SEOUL_OVERRIDE
	{
		m_fAccumTime += fDeltaTimeInSeconds;
		return (m_fAccumTime >= m_fDuration);
	}

	void SetDurationInSeconds(Float f)
	{
		m_fDuration = f;
	}

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(TestMotion);
	SEOUL_DISABLE_COPY(TestMotion);

	Float m_fAccumTime;
	Float m_fDuration;
}; // class TestMotion

} // namespace anonymous

SEOUL_TYPE(TestMotion, TypeFlags::kDisableCopy);

void UITest::TestMotionCancel()
{
	UI::MotionCollection collection;

	// Make some tweens.
	SharedPtr<TestMotion> pMotion1(SEOUL_NEW(MemoryBudgets::Developer) TestMotion); collection.AddMotion(pMotion1);
	SharedPtr<TestMotion> pMotion2(SEOUL_NEW(MemoryBudgets::Developer) TestMotion); collection.AddMotion(pMotion2);
	SharedPtr<TestMotion> pMotion3(SEOUL_NEW(MemoryBudgets::Developer) TestMotion); collection.AddMotion(pMotion3);

	// Setup tween 2 to cancel tween 3.
	pMotion1->SetIdentifier(0);
	pMotion2->SetIdentifier(1);
	pMotion3->SetIdentifier(2);
	SharedPtr<UITestFalconInstance> pInstance(SEOUL_NEW(MemoryBudgets::Developer) UITestFalconInstance());
	SharedPtr<MotionCompletion> pInterface1(SEOUL_NEW(MemoryBudgets::Developer) MotionCompletion(collection));
	SharedPtr<MotionCompletion> pInterface2(SEOUL_NEW(MemoryBudgets::Developer) MotionCompletion(collection, 2));
	SharedPtr<MotionCompletion> pInterface3(SEOUL_NEW(MemoryBudgets::Developer) MotionCompletion(collection));

	// Setup both 2 and 3 to finish simultaneously. Motion 2 should complete, Motion 3 will not, because
	// it will be cancelled first.
	pMotion2->SetCompletionInterface(pInterface2);
	pMotion2->SetDurationInSeconds(1.0f);
	pMotion2->SetInstance(pInstance);
	pMotion3->SetCompletionInterface(pInterface3);
	pMotion3->SetDurationInSeconds(1.0f);
	pMotion3->SetInstance(pInstance);

	// Also make sure tween 1 is unaffected by all of this.
	pMotion1->SetCompletionInterface(pInterface1);
	pMotion1->SetDurationInSeconds(2.0f);
	pMotion1->SetInstance(pInstance);

	// Advance by 1 second, make sure cancellation occured as we expected.
	collection.Advance(1.0f);
	SEOUL_UNITTESTING_ASSERT(!pInterface1->IsComplete());
	SEOUL_UNITTESTING_ASSERT(pInterface2->IsComplete());
	SEOUL_UNITTESTING_ASSERT(!pInterface3->IsComplete());

	// Now advance by a second, and make sure tween 1 has complete and the
	// alpha is what we expect.
	collection.Advance(1.0f);
	SEOUL_UNITTESTING_ASSERT(pInterface1->IsComplete());
}

namespace
{

class UITestMotionCancelAll SEOUL_SEALED : public UI::MotionCompletionInterface
{
public:
	UITestMotionCancelAll(UI::MotionCollection& r, const SharedPtr<Falcon::Instance>& p)
		: m_r(r)
		, m_p(p)
	{
	}

	~UITestMotionCancelAll()
	{
	}

	virtual void OnComplete() SEOUL_OVERRIDE
	{
		m_r.CancelAllMotions(m_p);
	}

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(UITestMotionCancelAll);

	UI::MotionCollection& m_r;
	SharedPtr<Falcon::Instance> m_p;
}; // class UITestMotionCancelAll

} // namespace anonymous

// Regression for bug in cancel all, if cancel all is called from within
// a completion callback.
void UITest::TestMotionCancelAll()
{
	UI::MotionCollection collection;

	// Different instances.
	{
		// Make some tweens.
		Vector< SharedPtr<UI::Motion>, MemoryBudgets::UIData > v;
		for (UInt32 i = 0u; i < 3u; ++i)
		{
			SharedPtr<TestMotion> pMotion(SEOUL_NEW(MemoryBudgets::Developer) TestMotion);
			pMotion->SetDurationInSeconds(1.0f);
			pMotion->SetInstance(SharedPtr<Falcon::Instance>(SEOUL_NEW(MemoryBudgets::Developer) UITestFalconInstance));

			if (1u == i)
			{
				pMotion->SetCompletionInterface(SharedPtr<UI::MotionCompletionInterface>(SEOUL_NEW(MemoryBudgets::Developer) UITestMotionCancelAll(collection, pMotion->GetInstance())));
			}

			v.PushBack(pMotion);
		}

		collection.Advance(0.5f);
		collection.Advance(0.5f);
		collection.Advance(0.5f);
	}

	// Same instance.
	{
		// Instance to share.
		SharedPtr<Falcon::Instance> pInstance(SEOUL_NEW(MemoryBudgets::Developer) UITestFalconInstance);

		// Make some tweens.
		Vector< SharedPtr<TestMotion>, MemoryBudgets::UIData > v;
		for (UInt32 i = 0u; i < 3u; ++i)
		{
			SharedPtr<TestMotion> pMotion(SEOUL_NEW(MemoryBudgets::Developer) TestMotion);
			pMotion->SetDurationInSeconds(1.0f);
			pMotion->SetInstance(pInstance);

			if (1u == i)
			{
				pMotion->SetCompletionInterface(SharedPtr<UI::MotionCompletionInterface>(SEOUL_NEW(MemoryBudgets::Developer) UITestMotionCancelAll(collection, pMotion->GetInstance())));
			}

			v.PushBack(pMotion);
		}

		UITestAdvanceInterface advance;
		collection.Advance(0.5f);
		collection.Advance(0.5f);
		collection.Advance(0.5f);
	}
}

void UITest::TestTweensBasic()
{
	UI::TweenCollection collection;

	CheckedPtr<UI::Tween> pTween(collection.AcquireTween());

	// Check default state.
	SEOUL_UNITTESTING_ASSERT(!pTween->GetCompletionInterface().IsValid());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, pTween->GetDurationInSeconds());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, pTween->GetEndValue());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, pTween->GetIdentifier());
	SEOUL_UNITTESTING_ASSERT(!pTween->GetInstance().IsValid());
	SEOUL_UNITTESTING_ASSERT(!pTween->GetNext().IsValid());
	SEOUL_UNITTESTING_ASSERT(!pTween->GetPrev().IsValid());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, pTween->GetStartValue());
	SEOUL_UNITTESTING_ASSERT_EQUAL(UI::TweenTarget::kTimer, pTween->GetTarget());
	SEOUL_UNITTESTING_ASSERT_EQUAL(UI::TweenType::kLine, pTween->GetType());

	SharedPtr<UITestFalconInstance> pInstance(SEOUL_NEW(MemoryBudgets::Developer) UITestFalconInstance());
	SharedPtr<UITestTweenCompletionInterface> pInterface(SEOUL_NEW(MemoryBudgets::Developer) UITestTweenCompletionInterface(collection));

	// Configure.
	pTween->SetCompletionInterface(pInterface);
	pTween->SetDurationInSeconds(1.0f);
	pTween->SetInstance(pInstance);
	pTween->SetTarget(UI::TweenTarget::kTimer);

	// Now advance by the time and make sure it completes.
	UITestAdvanceInterface advance;
	collection.Advance(advance, 1.0f);
	SEOUL_UNITTESTING_ASSERT(pInterface->IsComplete());

	// Now get a new tween. This will have the same address as the previous
	// tween (due to pooling), default values, but a new id.
	CheckedPtr<UI::Tween> pTween2(collection.AcquireTween());

	// Check state.
	SEOUL_UNITTESTING_ASSERT_EQUAL(pTween, pTween2);
	SEOUL_UNITTESTING_ASSERT(!pTween2->GetCompletionInterface().IsValid());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, pTween2->GetDurationInSeconds());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, pTween2->GetEndValue());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, pTween2->GetIdentifier());
	SEOUL_UNITTESTING_ASSERT(!pTween2->GetInstance().IsValid());
	SEOUL_UNITTESTING_ASSERT(!pTween2->GetNext().IsValid());
	SEOUL_UNITTESTING_ASSERT(!pTween2->GetPrev().IsValid());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, pTween2->GetStartValue());
	SEOUL_UNITTESTING_ASSERT_EQUAL(UI::TweenTarget::kTimer, pTween2->GetTarget());
	SEOUL_UNITTESTING_ASSERT_EQUAL(UI::TweenType::kLine, pTween2->GetType());
}

void UITest::TestTweensCancel()
{
	UI::TweenCollection collection;

	// Make some tweens.
	CheckedPtr<UI::Tween> pTween1(collection.AcquireTween());
	CheckedPtr<UI::Tween> pTween2(collection.AcquireTween());
	CheckedPtr<UI::Tween> pTween3(collection.AcquireTween());

	// Setup tween 2 to cancel tween 3.
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, pTween2->GetIdentifier());
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, pTween3->GetIdentifier());
	SharedPtr<UITestFalconInstance> pInstance(SEOUL_NEW(MemoryBudgets::Developer) UITestFalconInstance());
	SharedPtr<UITestTweenCompletionInterface> pInterface1(SEOUL_NEW(MemoryBudgets::Developer) UITestTweenCompletionInterface(collection));
	SharedPtr<UITestTweenCompletionInterface> pInterface2(SEOUL_NEW(MemoryBudgets::Developer) UITestTweenCompletionInterface(collection, 2));
	SharedPtr<UITestTweenCompletionInterface> pInterface3(SEOUL_NEW(MemoryBudgets::Developer) UITestTweenCompletionInterface(collection));

	// Setup both 2 and 3 to finish simultaneously. Tween 2 should complete, Tween 3 will not, because
	// it will be cancelled first.
	pTween2->SetCompletionInterface(pInterface2);
	pTween2->SetDurationInSeconds(1.0f);
	pTween2->SetInstance(pInstance);
	pTween2->SetTarget(UI::TweenTarget::kTimer);
	pTween3->SetCompletionInterface(pInterface3);
	pTween3->SetDurationInSeconds(1.0f);
	pTween3->SetInstance(pInstance);
	pTween3->SetTarget(UI::TweenTarget::kTimer);

	// Also make sure tween 1 is unaffected by all of this.
	pTween1->SetCompletionInterface(pInterface1);
	pTween1->SetDurationInSeconds(2.0f);
	pTween1->SetInstance(pInstance);
	pTween1->SetStartValue(1.0f);
	pTween1->SetEndValue(0.0f);
	pTween1->SetTarget(UI::TweenTarget::kAlpha);

	// Advance by 1 second, make sure cancellation occured as we expected.
	UITestAdvanceInterface advance;
	collection.Advance(advance, 1.0f);
	SEOUL_UNITTESTING_ASSERT(!pInterface1->IsComplete());
	SEOUL_UNITTESTING_ASSERT(pInterface2->IsComplete());
	SEOUL_UNITTESTING_ASSERT(!pInterface3->IsComplete());

	// Now advance by a second, and make sure tween 1 has complete and the
	// alpha is what we expect.
	collection.Advance(advance, 1.0f);
	SEOUL_UNITTESTING_ASSERT(pInterface1->IsComplete());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, pInstance->GetAlpha());

	// Finally, spawn three new tweens and make sure they are equal
	// to the first three (we should get tween 2, then tween 3, finally tween 1).
	auto pNewTween1(collection.AcquireTween());
	auto pNewTween2(collection.AcquireTween());
	auto pNewTween3(collection.AcquireTween());

	SEOUL_UNITTESTING_ASSERT_EQUAL(pNewTween1, pTween2);
	SEOUL_UNITTESTING_ASSERT_EQUAL(pNewTween2, pTween3);
	SEOUL_UNITTESTING_ASSERT_EQUAL(pNewTween3, pTween1);
}

namespace
{

class UITestCancelAll SEOUL_SEALED : public UI::TweenCompletionInterface
{
public:
	UITestCancelAll(UI::TweenCollection& r, const SharedPtr<Falcon::Instance>& p)
		: m_r(r)
		, m_p(p)
	{
	}

	~UITestCancelAll()
	{
	}

	virtual void OnComplete() SEOUL_OVERRIDE
	{
		m_r.CancelAllTweens(m_p);
	}

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(UITestCancelAll);

	UI::TweenCollection& m_r;
	SharedPtr<Falcon::Instance> m_p;
}; // class UITestCancelAll

} // namespace anonymous

// Regression for bug in cancel all, if cancel all is called from within
// a completion callback.
void UITest::TestTweensCancelAll()
{
	UI::TweenCollection collection;

	// Different instances.
	{
		// Make some tweens.
		Vector< CheckedPtr<UI::Tween>, MemoryBudgets::UIData > v;
		for (UInt32 i = 0u; i < 3u; ++i)
		{
			auto pTween(collection.AcquireTween());
			pTween->SetDurationInSeconds(1.0f);
			pTween->SetEndValue(0.0f);
			pTween->SetStartValue(1.0f);
			pTween->SetInstance(SharedPtr<Falcon::Instance>(SEOUL_NEW(MemoryBudgets::Developer) UITestFalconInstance));
			pTween->SetTarget(UI::TweenTarget::kAlpha);

			if (1u == i)
			{
				pTween->SetCompletionInterface(SharedPtr<UI::TweenCompletionInterface>(SEOUL_NEW(MemoryBudgets::Developer) UITestCancelAll(collection, pTween->GetInstance())));
			}

			v.PushBack(pTween);
		}

		UITestAdvanceInterface advance;
		collection.Advance(advance, 0.5f);
		collection.Advance(advance, 0.5f);
		collection.Advance(advance, 0.5f);
	}

	// Same instance.
	{
		// Instance to share.
		SharedPtr<Falcon::Instance> pInstance(SEOUL_NEW(MemoryBudgets::Developer) UITestFalconInstance);

		// Make some tweens.
		Vector< CheckedPtr<UI::Tween>, MemoryBudgets::UIData > v;
		for (UInt32 i = 0u; i < 3u; ++i)
		{
			auto pTween(collection.AcquireTween());
			pTween->SetDurationInSeconds(1.0f);
			pTween->SetEndValue(0.0f);
			pTween->SetStartValue(1.0f);
			pTween->SetInstance(pInstance);
			pTween->SetTarget(UI::TweenTarget::kAlpha);

			if (1u == i)
			{
				pTween->SetCompletionInterface(SharedPtr<UI::TweenCompletionInterface>(SEOUL_NEW(MemoryBudgets::Developer) UITestCancelAll(collection, pTween->GetInstance())));
			}

			v.PushBack(pTween);
		}

		UITestAdvanceInterface advance;
		collection.Advance(advance, 0.5f);
		collection.Advance(advance, 0.5f);
		collection.Advance(advance, 0.5f);
	}
}

void UITest::TestTweensValues()
{
	SharedPtr<UITestFalconInstance> pInstance(SEOUL_NEW(MemoryBudgets::Developer) UITestFalconInstance());

	UI::TweenCollection collection;

	// Alpha
	{
		auto pTween(collection.AcquireTween());
		pTween->SetDurationInSeconds(1.0f);
		pTween->SetEndValue(0.0f);
		pTween->SetStartValue(1.0f);
		pTween->SetInstance(pInstance);
		pTween->SetTarget(UI::TweenTarget::kAlpha);

		UITestAdvanceInterface advance;
		collection.Advance(advance, 0.5f);

		SEOUL_UNITTESTING_ASSERT_EQUAL(0.5f, pInstance->GetAlpha());

		collection.Advance(advance, 0.5f);

		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, pInstance->GetAlpha());
	}

	// Depth3D
	{
		auto pTween(collection.AcquireTween());
		pTween->SetDurationInSeconds(1.0f);
		pTween->SetEndValue(0.0f);
		pTween->SetStartValue(1.0f);
		pTween->SetInstance(pInstance);
		pTween->SetTarget(UI::TweenTarget::kDepth3D);

		UITestAdvanceInterface advance;
		collection.Advance(advance, 0.5f);

		SEOUL_UNITTESTING_ASSERT_EQUAL(0.5f, pInstance->GetDepth3D());

		collection.Advance(advance, 0.5f);

		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, pInstance->GetDepth3D());
	}

	// Position X
	{
		auto pTween(collection.AcquireTween());
		pTween->SetDurationInSeconds(1.0f);
		pTween->SetEndValue(25.0f);
		pTween->SetStartValue(-5.0f);
		pTween->SetInstance(pInstance);
		pTween->SetTarget(UI::TweenTarget::kPositionX);

		UITestAdvanceInterface advance;
		collection.Advance(advance, 0.5f);

		SEOUL_UNITTESTING_ASSERT_EQUAL(10.0f, pInstance->GetPosition().X);

		collection.Advance(advance, 0.5f);

		SEOUL_UNITTESTING_ASSERT_EQUAL(25.0f, pInstance->GetPosition().X);
	}

	// Position Y
	{
		auto pTween(collection.AcquireTween());
		pTween->SetDurationInSeconds(1.0f);
		pTween->SetEndValue(-5.0f);
		pTween->SetStartValue(25.0f);
		pTween->SetInstance(pInstance);
		pTween->SetTarget(UI::TweenTarget::kPositionY);

		UITestAdvanceInterface advance;
		collection.Advance(advance, 0.5f);

		SEOUL_UNITTESTING_ASSERT_EQUAL(10.0f, pInstance->GetPosition().Y);

		collection.Advance(advance, 0.5f);

		SEOUL_UNITTESTING_ASSERT_EQUAL(-5.0f, pInstance->GetPosition().Y);
	}

	// Rotation
	{
		auto pTween(collection.AcquireTween());
		pTween->SetDurationInSeconds(1.0f);
		pTween->SetEndValue(25.0f);
		pTween->SetStartValue(-5.0f);
		pTween->SetInstance(pInstance);
		pTween->SetTarget(UI::TweenTarget::kRotation);

		UITestAdvanceInterface advance;
		collection.Advance(advance, 0.5f);

		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(10.0f, pInstance->GetRotationInDegrees(), 1e-4f);

		collection.Advance(advance, 0.5f);

		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(25.0f, pInstance->GetRotationInDegrees(), 1e-4f);
	}

	// Scale X
	{
		auto pTween(collection.AcquireTween());
		pTween->SetDurationInSeconds(1.0f);
		pTween->SetEndValue(25.0f);
		pTween->SetStartValue(-5.0f);
		pTween->SetInstance(pInstance);
		pTween->SetTarget(UI::TweenTarget::kScaleX);

		UITestAdvanceInterface advance;
		collection.Advance(advance, 0.5f);

		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(10.0f, pInstance->GetScale().X, 1e-4f);

		collection.Advance(advance, 0.5f);

		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(25.0f, pInstance->GetScale().X, 1e-4f);
	}

	// Scale Y
	{
		auto pTween(collection.AcquireTween());
		pTween->SetDurationInSeconds(1.0f);
		pTween->SetEndValue(-5.0f);
		pTween->SetStartValue(25.0f);
		pTween->SetInstance(pInstance);
		pTween->SetTarget(UI::TweenTarget::kScaleY);

		UITestAdvanceInterface advance;
		collection.Advance(advance, 0.5f);

		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(10.0f, pInstance->GetScale().Y, 1e-4f);

		collection.Advance(advance, 0.5f);

		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(25.0f, pInstance->GetRotationInDegrees(), 1e-4f);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(25.0f, pInstance->GetScale().X, 1e-4f);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(-5.0f, pInstance->GetScale().Y, 1e-4f);
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
