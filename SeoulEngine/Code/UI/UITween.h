/**
 * \file UITween.h
 * \brief A UI::Tween is applied to a Falcon::Instance to perform
 * runtime, usually procedural animation of a property. UI::Tween
 * can also be used as a basic timing mechanis, to fire a callback
 * after a period of game time has passed without directly
 * affecting a Falcon::Instance property.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UI_TWEEN_H
#define UI_TWEEN_H

#include "CheckedPtr.h"
#include "FalconInstance.h"
#include "HashSet.h"
#include "SharedPtr.h"
#include "Prereqs.h"
#include "ReflectionDeclare.h"
#include "UITweenTarget.h"
#include "UITweenType.h"

namespace Seoul::UI
{

/**
 * Base class of completion callbacks - inherit
 * and implement OnCompletion() to receive
 * UI::Tween completion events.
 */
class TweenCompletionInterface SEOUL_ABSTRACT
{
public:
	virtual ~TweenCompletionInterface()
	{
	}

	// Required. Will be invoked on completion of a UITween.
	virtual void OnComplete() = 0;

protected:
	SEOUL_REFERENCE_COUNTED(TweenCompletionInterface);

	TweenCompletionInterface()
	{
	}

private:
	SEOUL_DISABLE_COPY(TweenCompletionInterface);
}; // class UI::TweenCompletionInterface

class Tween SEOUL_SEALED
{
public:
	Tween(Int iIdentifier);
	~Tween();

	// Called each frame to animate the tween.
	Bool Advance(Float fDeltaTimeInSeconds);

	/** @return Bound completion interface or nullptr if none was specified. */
	const SharedPtr<TweenCompletionInterface>& GetCompletionInterface() const
	{
		return m_pCompletionInterface;
	}

	/** @return The total play time of the tween. */
	Float GetDurationInSeconds() const
	{
		return m_fDurationInSeconds;
	}

	/** @return The target/final value of the tweened property. */
	Float GetEndValue() const
	{
		return m_fEndValue;
	}

	/**
	 * @return The unique identifier of the tween -
	 * not used by the UI::Tween itself, but can be used
	 * to reference tweens after it has been started.
	 */
	Int GetIdentifier() const
	{
		return m_iIdentifier;
	}

	/**
	 * @return The Falcon::Instance that owns and is being affected
	 * by this UITween.
	 */
	const SharedPtr<Falcon::Instance>& GetInstance() const
	{
		return m_pInstance;
	}

	/** @return The next UI::Tween in the doubly linked list of tweens. */
	CheckedPtr<Tween> GetNext() const
	{
		return m_pNext;
	}

	/** @return The previous UI::Tween in the doubly linked list of tweens. */
	CheckedPtr<Tween> GetPrev() const
	{
		return m_pPrev;
	}

	/** @return The initial/starting value of this UI::Tween's affected property. */
	Float GetStartValue() const
	{
		return m_fStartValue;
	}

	/** @return The target property of this UITween. */
	TweenTarget GetTarget() const
	{
		return m_eTarget;
	}

	/** @return The curve used to interpolate the start/end values of this UITween. */
	TweenType GetType() const
	{
		return m_eTweenType;
	}

	// Place this UI::Tween into a new doubly linked list. Removes it from any current list.
	void InsertBack(CheckedPtr<Tween>& rpHead, CheckedPtr<Tween>& rpTail);

	// Remove this UI::Tween from its curren doubly linked list. nop if not a list member.
	void Remove();

	/** Set the target of this UI::Tween to the invalid state. */
	void ResetInstance()
	{
		m_pInstance.Reset();
	}

	/** Update the interface to be invoked on UI::Tween completion. */
	void SetCompletionInterface(const SharedPtr<TweenCompletionInterface>& pCompletionInterface)
	{
		m_pCompletionInterface = pCompletionInterface;
	}

	/** Update the total playtime of the tween. */
	void SetDurationInSeconds(Float fDurationInSeconds)
	{
		m_fDurationInSeconds = fDurationInSeconds;
	}

	/** Update the target/final property value of the tween. */
	void SetEndValue(Float fEndValue)
	{
		m_fEndValue = fEndValue;
	}

	/**
	 * Update the target of this UITween.
	 *
	 * \pre pInstance must be valid.
	 */
	void SetInstance(const SharedPtr<Falcon::Instance>& pInstance)
	{
		SEOUL_ASSERT(pInstance.IsValid());
		m_pInstance = pInstance;
	}

	/** Update the initial value of the tween - this is the property value at time 0. */
	void SetStartValue(Float fStartValue)
	{
		m_fStartValue = fStartValue;
	}

	/** Update the property being affected by the tween. */
	void SetTarget(TweenTarget eTarget)
	{
		m_eTarget = eTarget;
	}

	/** Update the type of curve used to interpolate between start and end values of the tween. */
	void SetType(TweenType eTweenType)
	{
		m_eTweenType = eTweenType;
	}

private:
	SEOUL_REFLECTION_FRIENDSHIP(Tween);

	SharedPtr<Falcon::Instance> m_pInstance;
	SharedPtr<TweenCompletionInterface> m_pCompletionInterface;
	CheckedPtr< CheckedPtr<Tween> > m_ppHead;
	CheckedPtr< CheckedPtr<Tween> > m_ppTail;
	CheckedPtr<Tween> m_pNext;
	CheckedPtr<Tween> m_pPrev;
	Int m_iIdentifier;

	Float m_fStartValue;
	Float m_fEndValue;
	Float m_fDurationInSeconds;
	TweenTarget m_eTarget;
	TweenType m_eTweenType;
	Float m_fElapsedInSeconds;

	SEOUL_DISABLE_COPY(Tween);
}; // class UI::Tween

/**
 * Utility, provides functionality to manage UI::Tween instances.
 */
class TweenCollection SEOUL_SEALED
{
public:
	TweenCollection();
	~TweenCollection();

	// Return a new UI::Tween, inserted at the end of the tween evaluation list.
	CheckedPtr<Tween> AcquireTween();

	// Update all UI::Tween instances owned and release/destroy any that have completed.
	void Advance(Falcon::AdvanceInterface& rInterface, Float fDeltaTimeInSeconds);

	// Mark a tween as cancelled. This will be applied prior to the next Advance() of the tween.
	void CancelTween(Int iIdentifier)
	{
		(void)m_PendingCancels.Insert(iIdentifier);
	}

	// Remove all tween matching the given instance.
	void CancelAllTweens(const SharedPtr<Falcon::Instance>& pInstance);

private:
	typedef HashSet<Int, MemoryBudgets::UIRuntime> Cancels;
	Cancels m_PendingCancels;
	Cancels m_CurrentCancels;

	CheckedPtr<Tween> m_pTweenHead;
	CheckedPtr<Tween> m_pTweenTail;
	CheckedPtr<Tween> m_pFreeTweenHead;
	CheckedPtr<Tween> m_pFreeTweenTail;

	CheckedPtr<Tween> m_pAdvanceCurrent;
	Int m_iTweenId;
	Atomic32 m_InAdvance;

	static void Destroy(CheckedPtr<Tween>& rpHead, CheckedPtr<Tween>& rpTail);
	CheckedPtr<Tween> Free(CheckedPtr<Tween>& rp);

	SEOUL_DISABLE_COPY(TweenCollection);
}; // class UI::TweenCollection

} // namespace Seoul::UI

#endif // include guard
