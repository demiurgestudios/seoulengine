/**
 * \file NullGraphicsVertexBuffer.cpp
 * \brief Nop implementation of a VertexBuffer for
 * contexts without graphics hardware.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "NullGraphicsVertexBuffer.h"
#include "ThreadId.h"

namespace Seoul
{

NullGraphicsVertexBuffer::NullGraphicsVertexBuffer(
	UInt32 zTotalSizeInBytes,
	UInt32 zVertexStrideInBytes)
	: VertexBuffer(zVertexStrideInBytes, zTotalSizeInBytes)
{
}

NullGraphicsVertexBuffer::~NullGraphicsVertexBuffer()
{
	SEOUL_ASSERT(IsRenderThread());
}

Bool NullGraphicsVertexBuffer::OnCreate()
{
	SEOUL_ASSERT(IsRenderThread());

	SEOUL_VERIFY(VertexBuffer::OnCreate());
	return true;
}

void NullGraphicsVertexBuffer::OnReset()
{
	SEOUL_ASSERT(IsRenderThread());

	VertexBuffer::OnReset();
}

void NullGraphicsVertexBuffer::OnLost()
{
	SEOUL_ASSERT(IsRenderThread());

	VertexBuffer::OnLost();
}

} // namespace Seoul
