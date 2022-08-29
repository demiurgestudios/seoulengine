/**
 * \file NullGraphicsRenderTarget.cpp
 * \brief Nop implementation of a RenderTarget for
 * contexts without graphics hardware.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "NullGraphicsDevice.h"
#include "NullGraphicsRenderTarget.h"
#include "ThreadId.h"

namespace Seoul
{

NullGraphicsRenderTarget::NullGraphicsRenderTarget(const DataStoreTableUtil& configSettings)
	: RenderTarget(configSettings)
{
}

NullGraphicsRenderTarget::~NullGraphicsRenderTarget()
{
}

void NullGraphicsRenderTarget::Select()
{
	SEOUL_ASSERT(IsRenderThread());

	if (SupportsSimultaneousInputOutput() || s_pActiveRenderTarget != this)
	{
		s_pActiveRenderTarget = this;
	}
}

void NullGraphicsRenderTarget::Resolve()
{
	SEOUL_ASSERT(IsRenderThread());
}

void NullGraphicsRenderTarget::Unselect()
{
	SEOUL_ASSERT(IsRenderThread());

	if (s_pActiveRenderTarget != this)
	{
		return;
	}

	// Reset.
	s_pActiveRenderTarget = nullptr;
}

void NullGraphicsRenderTarget::OnLost()
{
	SEOUL_ASSERT(IsRenderThread());

	Unselect();

	RenderTarget::OnLost();
}

void NullGraphicsRenderTarget::OnReset()
{
	SEOUL_ASSERT(IsRenderThread());

	// Refresh the width and height, in case they are dependent on
	// the back buffer.
	InternalRefreshWidthAndHeight();

	RenderTarget::OnReset();
}

} // namespace Seoul
