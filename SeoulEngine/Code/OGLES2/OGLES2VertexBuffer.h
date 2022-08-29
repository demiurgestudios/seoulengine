/**
 * \file OGLES2VertexBuffer.h
 * \brief A collection of GPU vertices used for drawing geometry.
 * Vertices include position, normal, and other data that can be stored
 * per vertex to represent renderable geometry.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef OGLES2_VERTEX_BUFFER_H
#define OGLES2_VERTEX_BUFFER_H

#include "OGLES2Util.h"
#include "Mutex.h"
#include "Platform.h"
#include "Vector.h"
#include "VertexBuffer.h"

namespace Seoul
{

/**
 * OGLES2 specialization of VertexBuffer. Provides methods to
 * Lock()/Unlock() the buffer as well as check usage state, but
 * is otherwise an opaque wrapper around a GL vertex buffer.
 */
class OGLES2VertexBuffer SEOUL_SEALED : public VertexBuffer
{
public:
	virtual Bool OnCreate() SEOUL_OVERRIDE;

private:
	OGLES2VertexBuffer(
		void const* pInitialData,
		UInt32 zInitialDataSizeInBytes,
		UInt32 zTotalSizeInBytes,
		UInt32 zVertexStrideInBytes,
		Bool bDynamic);
	virtual ~OGLES2VertexBuffer();

	friend class OGLES2RenderDevice;
	friend class OGLES2RenderCommandStreamBuilder;

	SEOUL_REFERENCE_COUNTED_SUBCLASS(OGLES2VertexBuffer);

	void InternalFreeInitialData();

	Byte* m_pDynamicData;
	void const* m_pInitialData;
	UInt32 m_zInitialDataSizeInBytes;
	GLuint m_VertexBuffer;
	Bool m_bDynamic;

	SEOUL_DISABLE_COPY(OGLES2VertexBuffer);
}; // class OGLES2VertexBuffer

} // namespace Seoul

#endif // include guard
