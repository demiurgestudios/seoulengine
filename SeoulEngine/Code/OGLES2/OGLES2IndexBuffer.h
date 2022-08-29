/**
 * \file OGLES2IndexBuffer.h
 * \brief A collection of GPU indices used for drawing geometry.
 * In most cases, geometry is represented as an array of vertices
 * (which include position, normal, and other data) and an array of
 * indices into the array of vertices. Using indirect referencing of
 * vertices allows the vertex buffers to be smaller. IndexBuffer
 * is Seoul engine's object wrapper around a GPU index buffer.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef OGLES2_INDEX_BUFFER_H
#define OGLES2_INDEX_BUFFER_H

#include "OGLES2Util.h"
#include "IndexBuffer.h"
#include "IndexBufferDataFormat.h"

namespace Seoul
{

/**
 * OGLES2 specific implementation of the IndexBuffer class. Supports
 * locking/unlocking of the buffer, but is otherwise an opaque
 * wrapper around an OpenGl ES2 buffer object.
 */
class OGLES2IndexBuffer SEOUL_SEALED : public IndexBuffer
{
public:
	virtual Bool OnCreate() SEOUL_OVERRIDE;

private:
	OGLES2IndexBuffer(
		void const* pInitialData,
		UInt32 zInitialDataSizeInBytes,
		UInt32 zTotalSizeInBytes,
		IndexBufferDataFormat eFormat,
		Bool bDynamic);
	virtual ~OGLES2IndexBuffer();

	friend class OGLES2RenderDevice;
	friend class OGLES2RenderCommandStreamBuilder;

	SEOUL_REFERENCE_COUNTED_SUBCLASS(OGLES2IndexBuffer);

	void InternalFreeInitialData();

	Byte* m_pDynamicData;
	void const* m_pInitialData;
	UInt32 m_zInitialDataSizeInBytes;
	IndexBufferDataFormat m_eFormat;
	GLuint m_IndexBuffer;
	Bool m_bDynamic;

	SEOUL_DISABLE_COPY(OGLES2IndexBuffer);
}; // class OGLES2IndexBuffer

} // namespace Seoul

#endif // include guard
