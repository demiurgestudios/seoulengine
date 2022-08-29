/**
 * \file BaseGraphicsObject.cpp
 * \brief A base class for all HAL objects.
 * Graphics objects include Effect, Texture, RenderSurface2D, etc.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "BaseGraphicsObject.h"
#include "ThreadId.h"

namespace Seoul
{

BaseGraphicsObject::BaseGraphicsObject()
	: m_eState(kDestroyed)
{
}

BaseGraphicsObject::~BaseGraphicsObject()
{
	SEOUL_ASSERT(0 == m_AtomicReferenceCount);
	SEOUL_ASSERT(IsRenderThread());

	// It is the responsibility of the subclass to un-reset itself
	// on destruction. Graphics objects that fail to create (either
	// due to a device that cannot be initialized, or due to an
	// invalid object definition) can also be safely destroyed.
	SEOUL_ASSERT(m_eState == kCreated || m_eState == kDestroyed);

	m_eState = kDestroyed;
}

/**
 * Invoked by RenderDevice when the device is ready to create this Object -
 * once an object is created, it remains created or reset until the object is destroyed by
 * client code.
 */
Bool BaseGraphicsObject::OnCreate()
{
	SEOUL_ASSERT(IsRenderThread() && kDestroyed == m_eState);
	m_eState = kCreated;
	return true;
}

/**
 * Invoked by RenderDevice when the device is in the "reset" state - the device
 * is in a fully operable state and can be used for rendering.
 */
void BaseGraphicsObject::OnReset()
{
	SEOUL_ASSERT(IsRenderThread() && kCreated == m_eState);
	m_eState = kReset;
}

/**
 * Invoked by RenderDevice when the device is in the "lost" state - in this
 * state, the device is still valid but cannot be used for rendering.
 */
void BaseGraphicsObject::OnLost()
{
	SEOUL_ASSERT(IsRenderThread() && kReset == m_eState);
	m_eState = kCreated;
}

} // namespace Seoul
