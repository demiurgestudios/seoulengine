/**
 * \file BaseGraphicsObject.h
 * \brief A base class for all HAL objects.
 * Graphics objects include Effect, Texture, RenderSurface2D, etc.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef BASE_GRAPHICS_OBJECT_H
#define BASE_GRAPHICS_OBJECT_H

#include "SharedPtr.h"
#include "Prereqs.h"

namespace Seoul
{

/**
 * Base class for all graphics HAL objects.
 *
 * \warning As a rule, all member functions of BaseGraphicsObject
 * can only be called on the render thread - the only operation
 * that is safe for other threads is the constructor, and the
 * GetState() method.
 */
class BaseGraphicsObject SEOUL_ABSTRACT
{
public:
	virtual Bool OnCreate();
	virtual void OnReset();
	virtual void OnLost();

	enum State
	{
		/** State of an object that is neither created or reset. */
		kDestroyed,

		/** State of an object that is created, but not reset. */
		kCreated,

		/** State of a fully reset object - can be used for rendering. */
		kReset
	};

	/**
	 * @return The current state of this BaseGraphicsObject. If any object is in
	 * any state other than kReset, it cannot be used for rendering.
	 */
	State GetState() const
	{
		return m_eState;
	}

protected:
	SEOUL_REFERENCE_COUNTED(BaseGraphicsObject);

	BaseGraphicsObject();
	virtual ~BaseGraphicsObject();

private:
	Atomic32Value<State> m_eState;

	SEOUL_DISABLE_COPY(BaseGraphicsObject);
}; // class BaseGraphicsObject

} // namespace Seoul

#endif // include guard
