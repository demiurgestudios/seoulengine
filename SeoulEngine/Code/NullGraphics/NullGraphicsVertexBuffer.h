/**
 * \file NullGraphicsVertexBuffer.h
 * \brief Nop implementation of a VertexBuffer for
 * contexts without graphics hardware.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef NULL_GRAPHICS_VERTEX_BUFFER_H
#define NULL_GRAPHICS_VERTEX_BUFFER_H

#include "VertexBuffer.h"

namespace Seoul
{

class NullGraphicsVertexBuffer SEOUL_SEALED : public VertexBuffer
{
public:
	virtual Bool OnCreate() SEOUL_OVERRIDE;
	virtual void OnReset() SEOUL_OVERRIDE;
	virtual void OnLost() SEOUL_OVERRIDE;

private:
	NullGraphicsVertexBuffer(
		UInt32 zTotalSizeInBytes,
		UInt32 zVertexStrideInBytes);
	~NullGraphicsVertexBuffer();

	friend class NullGraphicsDevice;
	friend class NullGraphicsRenderCommandStreamBuilder;

	SEOUL_REFERENCE_COUNTED_SUBCLASS(NullGraphicsVertexBuffer);

	SEOUL_DISABLE_COPY(NullGraphicsVertexBuffer);
}; // class NullGraphicsVertexBuffer

} // namespace Seoul

#endif // include guard
