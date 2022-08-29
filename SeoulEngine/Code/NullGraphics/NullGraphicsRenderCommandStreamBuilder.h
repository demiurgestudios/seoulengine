/**
 * \file NullGraphicsRenderCommandStreamBuilder.h
 * \brief Nop implementation of a RenderCommandStreamBuilder for
 * contexts without graphics hardware.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef NULL_GRAPHICS_RENDER_COMMAND_STREAM_BUILDER_H
#define NULL_GRAPHICS_RENDER_COMMAND_STREAM_BUILDER_H

#include "RenderCommandStreamBuilder.h"

namespace Seoul
{

// Forward declarations
class NullGraphicsEffectStateManager;
class NullGraphicsVertexFormat;

class NullGraphicsRenderCommandStreamBuilder SEOUL_SEALED : public RenderCommandStreamBuilder
{
public:
	NullGraphicsRenderCommandStreamBuilder(UInt32 zInitialCapacity);
	~NullGraphicsRenderCommandStreamBuilder();

	virtual void ExecuteCommandStream(RenderStats& rStats) SEOUL_OVERRIDE;

private:
	SEOUL_DISABLE_COPY(NullGraphicsRenderCommandStreamBuilder);
}; // class NullGraphicsRenderCommandStreamBuilder

} // namespace Seoul

#endif // include guard
