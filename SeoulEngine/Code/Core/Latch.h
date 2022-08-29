/**
 * \file Latch.h
 *
 * \brief A latch is basically a callback function that runs only after
 * all conditions have fired. You set up a latch with the conditions
 * necessary before the latch will execute. Each condition has a string
 * name.
 *
 * Usage:
 *
 * 1. Create a latch instance.
 * 2. Set the condition list.
 * 3. Clear various conditions, which will eventually call the actor.
 *
 * You can reset the conditions at any time. You will need to re-clear all
 * the conditions to execute the latch.
 *
 * Note that the condition list can have duplicates.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef LATCH_H
#define LATCH_H

#include "List.h"
#include "Prereqs.h"
#include "SeoulHString.h"
#include "SeoulTypes.h"
#include "Vector.h"

namespace Seoul
{

/**
 * A new latch (one that hasn't cleared any conditions yet) is eLatchNew.
 * A latch that already fired is eLatchClosed. Otherwise, the latch is
 * eLatchOpen.
 *
 * If you see eLatchError, something terrible happened, so watch out!
 */
enum ELatchStatus
{
	eLatchNew,
	eLatchOpen,
	eLatchClosed,
	eLatchError
};

/**
 * A latch will execute an action after clearing all conditions. In general,
 * latches only ever execute an action once. You can always reset a latch
 * if you want to change the conditions.
 *
 * A latch with no conditions will automatically close the first time you
 * clear ANY condition.
 *
 * Each latch derivative must override two pure virtual functions: Execute()
 * and Abort().
 *
 * Execute() runs when the latch first closes (but only once - when all latch
 * conditions clear). Abort() runs if the latch deallocates while it's still
 * open.
 */
class Latch SEOUL_ABSTRACT
{
public:
	/**
	 * Create a latch with no conditions. The latch will close when you
	 * clear any condition.
	 */
	Latch();

	Latch(const Vector<String>& vConditions);
	Latch(Byte const* aConditions[], UInt uLength);

	virtual ~Latch();

	/**
	 * Get the current latch status. We should only return New, Open, or
	 * Closed.
	 */
	virtual ELatchStatus GetStatus() const;

	/**
	 * Return true if the latch is still waiting on the given condition.
	 *
	 * @param sCondition input condition
	 * @return true if the condition has not cleared yet
	 */
	virtual Bool Check(const String& sCondition) const;

	/**
	 * Trigger a condition, which will clear the named condition (if it is
	 * still in the conditions list). Once the last condition clears, the
	 * latch will close and the actor will run.
	 *
	 * @param sCondition the name of a latch condition
	 * @return the status after the trigger
	 */
	virtual ELatchStatus Trigger(const String& sCondition);

	/**
	 * Trigger a group of conditions, which will clear any of the named
	 * conditions if they are in the conditions list. If the final condition
	 * clears, the latch will close and the actor will run.
	 *
	 * @param vConditions a listing of conditions
	 * @return the status after the trigger
	 */
	virtual ELatchStatus Trigger(const Vector<String>& vConditions);

	/**
	 * If the FIRST condition in the list matches the named condition, then
	 * we will clear the condition. Once the last condition clears, the latch
	 * will close and the actor will run.
	 *
	 * (Unlike Trigger(), this function relies on the conditions list order.)
	 *
	 * @param sCondition the name of a latch condition
	 * @return the status after the shift
	 */
	virtual ELatchStatus Step(const String& sCondition);

	/**
	 * Force the latch closed. All conditions clear, and if necessary, the
	 * latch action will execute.
	 *
	 * This function is an abomination. You really shouldn't use it! Why
	 * does it exist? That's a good question.
	 */
	virtual void Force();

	/**
	 * Reset the latch to default conditions. Subclasses should override
	 * this function to set up their initial conditions. Note that you must
	 * call one of the other Reset() functions from your override.
	 *
	 * The base Reset() will start off with an empty condition list, so the
	 * latch will close after any attempt to clear.
	 */
	virtual void Reset();

	/**
	 * Specify a new set of conditions, resetting the latch to "New"
	 * status. If the condition list is empty, the latch will close after
	 * any attempt to clear.
	 *
	 * @param vConditions a list of conditions
	 */
	virtual void Reset(const Vector<String>& vConditions);

	/**
	 * Specify a new set of conditions, resetting the latch to "New"
	 * status. If the condition list is empty, the latch will close after
	 * any attempt to clear.
	 *
	 * @param aConditions an array of String conditions
	 */
	virtual void Reset(Byte const* aConditions[], UInt32 uLength);

	/**
	 * Add an additional condition to the list (if it doesn't already exist).
	 * If the condition already exists, nothing happens (it retains its
	 * existing position in the list).
	 *
	 * If the latch has not closed yet, then the condition goes to the end of
	 * the list. If the latch has closed, then we we reset it with the given
	 * condition (so it will fire again). All other states, the latch ignores
	 * the condition (logging a warning).
	 *
	 * @param sCondition string condition
	 */
	virtual void Require(const String& sCondition);

protected:
	/**
	 * Run the latch action.
	 */
	virtual void Execute(void) = 0;
	/**
	 * Freeze the latch so it will not run again. Status becomes eLatchError.
	 */
	virtual void Terminate();

private:
	// a list of pending conditions
	List<String> m_lConditions;

	// the current status
	ELatchStatus m_eStatus;

	SEOUL_DISABLE_COPY(Latch);
}; // class Latch

class NoOpLatch SEOUL_SEALED : public Latch
{
public:
	NoOpLatch()
	{
	}

protected:
	virtual void Execute(void) SEOUL_OVERRIDE
	{
	}

private:
	SEOUL_DISABLE_COPY(NoOpLatch);
}; // class NoOpLatch

} // namespace Seoul

#endif // include guard
