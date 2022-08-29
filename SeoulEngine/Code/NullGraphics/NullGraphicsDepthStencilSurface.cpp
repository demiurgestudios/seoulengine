/**
 * \file NullGraphicsDepthStencilSurface.cpp
 * \brief Nop implementation of a DepthStencilSurface for
 * contexts without graphics hardware.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "NullGraphicsDepthStencilSurface.h"
#include "ThreadId.h"

namespace Seoul
{

NullGraphicsDepthStencilSurface::NullGraphicsDepthStencilSurface(const DataStoreTableUtil& configSettings)
	: DepthStencilSurface(configSettings)
{
}

NullGraphicsDepthStencilSurface::~NullGraphicsDepthStencilSurface()
{
	SEOUL_ASSERT(IsRenderThread());
}

void NullGraphicsDepthStencilSurface::Select()
{
	SEOUL_ASSERT(IsRenderThread());

	if (this != s_pCurrentSurface)
	{
		s_pCurrentSurface = this;
	}
}

void NullGraphicsDepthStencilSurface::Unselect()
{
	SEOUL_ASSERT(IsRenderThread());

	if (this == s_pCurrentSurface)
	{
		s_pCurrentSurface = nullptr;
	}
}

void NullGraphicsDepthStencilSurface::Resolve()
{
}

void NullGraphicsDepthStencilSurface::OnLost()
{
	SEOUL_ASSERT(IsRenderThread());

	Unselect();

	DepthStencilSurface::OnLost();
}

void NullGraphicsDepthStencilSurface::OnReset()
{
	SEOUL_ASSERT(IsRenderThread());

	DepthStencilSurface::OnReset();
}

} // namespace Seoul
