/**
 * \file VertexBuffer.h
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
#ifndef VERTEX_BUFFER_H
#define VERTEX_BUFFER_H

#include "BaseGraphicsObject.h"
#include "SharedPtr.h"
#include "Prereqs.h"

namespace Seoul
{

/**
 * Abstract base class of vertex buffers.
 * Concrete implementations are platform specific (i.e. D3D9VertexBuffer).
 */
class VertexBuffer SEOUL_ABSTRACT : public BaseGraphicsObject
{
public:
	VertexBuffer(UInt32 nVertexStrideInBytes, UInt32 zTotalSizeInBytes)
		: m_nVertexStrideInBytes(nVertexStrideInBytes)
		, m_zTotalSizeInBytes(zTotalSizeInBytes)
	{
	}

	virtual ~VertexBuffer()
	{
		// It is the responsibility of the subclass to un-reset itself
		// on destruction if the graphics object was ever created.
		SEOUL_ASSERT(kCreated == GetState() || kDestroyed == GetState());
	}

	UInt32 GetVertexStrideInBytes() const
	{
		return m_nVertexStrideInBytes;
	}

	UInt32 GetTotalSizeInBytes() const
	{
		return m_zTotalSizeInBytes;
	}

protected:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(VertexBuffer);

private:
	UInt32 m_nVertexStrideInBytes;
	UInt32 m_zTotalSizeInBytes;

	friend class RenderCommandStreamBuilder;

	SEOUL_DISABLE_COPY(VertexBuffer);
}; // class VertexBuffer

} // namespace Seoul

#endif // include guard
