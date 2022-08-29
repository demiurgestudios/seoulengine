/**
 * \file IndexBuffer.h
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
#ifndef INDEX_BUFFER_H
#define INDEX_BUFFER_H

#include "BaseGraphicsObject.h"
#include "SharedPtr.h"
#include "Prereqs.h"

namespace Seoul
{

/**
 * Abstract base class of index buffers.
 * Concrete implementations are platform specific (i.e. D3D9IndexBuffer).
 */
class IndexBuffer SEOUL_ABSTRACT : public BaseGraphicsObject
{
public:
	IndexBuffer(UInt32 zTotalSizeInBytes)
		: m_zTotalSizeInBytes(zTotalSizeInBytes)
	{
	}

	virtual ~IndexBuffer()
	{
		// It is the responsibility of the subclass to un-reset itself
		// on destruction if the graphics object was ever created.
		SEOUL_ASSERT(kCreated == GetState() || kDestroyed == GetState());
	}

	/**
	 * @return The maximum size of this IndexBuffer in bytes.
	 */
	UInt32 GetTotalSizeInBytes() const
	{
		return m_zTotalSizeInBytes;
	}

protected:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(IndexBuffer);

private:
	UInt32 m_zTotalSizeInBytes;

	friend class RenderCommandStreamBuilder;

	SEOUL_DISABLE_COPY(IndexBuffer);
}; // class IndexBuffer

} // namespace Seoul

#endif // include guard
