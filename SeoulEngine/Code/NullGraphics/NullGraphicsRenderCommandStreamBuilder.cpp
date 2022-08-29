/**
 * \file NullGraphicsRenderCommandStreamBuilder.cpp
 * \brief Nop implementation of a RenderCommandStreamBuilder for
 * contexts without graphics hardware.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "NullGraphicsRenderCommandStreamBuilder.h"

namespace Seoul
{

NullGraphicsRenderCommandStreamBuilder::NullGraphicsRenderCommandStreamBuilder(
	UInt32 zInitialCapacity)
	: RenderCommandStreamBuilder(zInitialCapacity)
{
}

NullGraphicsRenderCommandStreamBuilder::~NullGraphicsRenderCommandStreamBuilder()
{
}

void NullGraphicsRenderCommandStreamBuilder::ExecuteCommandStream(RenderStats& rStats)
{
	SEOUL_ASSERT(IsRenderThread());

	memset(&rStats, 0, sizeof(rStats));
}

} // namespace Seoul
