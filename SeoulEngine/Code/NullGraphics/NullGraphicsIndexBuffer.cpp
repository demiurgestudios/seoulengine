/**
 * \file NullGraphicsIndexBuffer.cpp
 * \brief Nop implementation of an IndexBuffer for
 * contexts without graphics hardware.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "NullGraphicsDevice.h"
#include "NullGraphicsIndexBuffer.h"

namespace Seoul
{

NullGraphicsIndexBuffer::NullGraphicsIndexBuffer(UInt32 zTotalSizeInBytes)
	: IndexBuffer(zTotalSizeInBytes)
{
}

NullGraphicsIndexBuffer::~NullGraphicsIndexBuffer()
{
	SEOUL_ASSERT(IsRenderThread());
}

Bool NullGraphicsIndexBuffer::OnCreate()
{
	SEOUL_ASSERT(IsRenderThread());

	SEOUL_VERIFY(IndexBuffer::OnCreate());
	return true;
}

void NullGraphicsIndexBuffer::OnReset()
{
	SEOUL_ASSERT(IsRenderThread());

	IndexBuffer::OnReset();
}

void NullGraphicsIndexBuffer::OnLost()
{
	SEOUL_ASSERT(IsRenderThread());

	IndexBuffer::OnLost();
}

} // namespace Seoul
