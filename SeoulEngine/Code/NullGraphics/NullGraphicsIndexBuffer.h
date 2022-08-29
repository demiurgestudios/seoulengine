/**
 * \file NullGraphicsIndexBuffer.h
 * \brief Nop implementation of an IndexBuffer for
 * contexts without graphics hardware.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef NULL_GRAPHICS_INDEX_BUFFER_H
#define NULL_GRAPHICS_INDEX_BUFFER_H

#include "CheckedPtr.h"
#include "JobsJob.h"
#include "IndexBuffer.h"
#include "IndexBufferDataFormat.h"

namespace Seoul
{

class NullGraphicsIndexBuffer SEOUL_SEALED : public IndexBuffer
{
public:
	virtual Bool OnCreate() SEOUL_OVERRIDE;
	virtual void OnReset() SEOUL_OVERRIDE;
	virtual void OnLost() SEOUL_OVERRIDE;

private:
	NullGraphicsIndexBuffer(UInt32 zInitialDataSizeInBytes);
	~NullGraphicsIndexBuffer();

	friend class NullGraphicsDevice;
	friend class NullGraphicsRenderCommandStreamBuilder;

	SEOUL_REFERENCE_COUNTED_SUBCLASS(NullGraphicsIndexBuffer);

	SEOUL_DISABLE_COPY(NullGraphicsIndexBuffer);
}; // class NullGraphicsIndexBuffer

} // namespace Seoul

#endif // include guard
