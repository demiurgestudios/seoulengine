/**
 * \file UIMotion.h
 * \brief A UI::Motion is applied to a Falcon::Instance to perform
 * runtime custom movement of the instance
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UI_MOTION_H
#define UI_MOTION_H

#include "CheckedPtr.h"
#include "FalconInstance.h"
#include "SharedPtr.h"
#include "Prereqs.h"

namespace Seoul::UI
{

/**
 * Base class of completion callbacks - inherit
 * and implement OnCompletion() to receive
 * UI::Motion completion events.
 */
class MotionCompletionInterface SEOUL_ABSTRACT
{
public:
	virtual ~MotionCompletionInterface()
	{
	}

	// Required. Will be invoked on completion of a UIMotion.
	virtual void OnComplete() = 0;

protected:
	SEOUL_REFERENCE_COUNTED(MotionCompletionInterface);

	MotionCompletionInterface()
	{
	}

private:
	SEOUL_DISABLE_COPY(MotionCompletionInterface);
}; // class UI::MotionCompletionInterface


   /**
   * Base class of UI::Motion - inherit
   * and implement Advance()
   */
class Motion SEOUL_ABSTRACT
{
public:
	SEOUL_REFLECTION_POLYMORPHIC_BASE(Motion);

	Motion();
	virtual ~Motion();

	// Called each frame to animate the motion.
	virtual Bool Advance(Float fDeltaTimeInSeconds) = 0;

	/** @return Bound completion interface or nullptr if none was specified. */
	const SharedPtr<MotionCompletionInterface>& GetCompletionInterface() const
	{
		return m_pCompletionInterface;
	}

	/**
	* Update the unique identifier of this UIMotion.
	*
	* \pre pInstance must be valid.
	*/
	void SetIdentifier(Int iIdentifier)
	{
		m_iIdentifier = iIdentifier;
	}

	/**
	* @return The unique identifier of the UI::Motion -
	* not used by the UI::Motion itself, but can be used
	* to reference UI::Motion after it has been started.
	*/
	Int GetIdentifier() const
	{
		return m_iIdentifier;
	}

	/**
	 * @return The Falcon::Instance that owns and is being affected
	 * by this UIMotion.
	 */
	const SharedPtr<Falcon::Instance>& GetInstance() const
	{
		return m_pInstance;
	}

	/** Set the target of this UI::Motion to the invalid state. */
	void ResetInstance()
	{
		m_pInstance.Reset();
	}

	/** Update the interface to be invoked on UI::Motion completion. */
	void SetCompletionInterface(const SharedPtr<MotionCompletionInterface>& pCompletionInterface)
	{
		m_pCompletionInterface = pCompletionInterface;
	}

	/**
	 * Update the target of this UIMotion.
	 *
	 * \pre pInstance must be valid.
	 */
	void SetInstance(const SharedPtr<Falcon::Instance>& pInstance)
	{
		SEOUL_ASSERT(pInstance.IsValid());
		m_pInstance = pInstance;
	}

protected:
	// Reference counted
	SEOUL_REFERENCE_COUNTED(Motion);

private:
	SharedPtr<Falcon::Instance> m_pInstance;
	SharedPtr<MotionCompletionInterface> m_pCompletionInterface;
	Int m_iIdentifier;

	SEOUL_DISABLE_COPY(Motion);
}; // class UI::Motion

} // namespace Seoul::UI

#endif // include guard
