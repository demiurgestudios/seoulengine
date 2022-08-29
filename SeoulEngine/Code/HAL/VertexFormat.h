/**
 * \file VertexFormat.h
 * \brief VertexFormat describes the format of each vertex in a vertex
 * buffer.
 *
 * VertexFormat is equivalent to the D3D9 VertexDeclaration.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef VERTEX_FORMAT_H
#define VERTEX_FORMAT_H

#include "BaseGraphicsObject.h"
#include "Prereqs.h"
#include "Vector.h"
#include "VertexElement.h"

namespace Seoul
{

/**
 * Abstract base class of a VertexFormat.
 *
 * Concrete implementations are platform specific (i.e. D3D9VertexFormat).
 */
class VertexFormat SEOUL_ABSTRACT : public BaseGraphicsObject
{
public:
	/**
	 * Defined to match D3D9 software vertex processing - may
	 * be arbitrary in the scope of cross-platform, hardware rendering.
	 */
	static const UInt32 kMaxStreams = 16u;

	typedef Vector<VertexElement, MemoryBudgets::Rendering> VertexElements;

	virtual ~VertexFormat()
	{
		// It is the responsibility of the subclass to un-reset itself
		// on destruction if the graphics object was ever created.
		SEOUL_ASSERT(kCreated == GetState() || kDestroyed == GetState());
	}

	/**
	 * @return A read-only reference to the vector of elements that
	 * define this VertexFormat.
	 *
	 * It is platform dependent whether this vector includes the
	 * VertexEnd terminator. If you are writing code that depends on this
	 * value, you must explicitly check for it.
	 */
	const VertexElements& GetVertexElements() const
	{
		return m_vVertexElements;
	}

	/**
	 * @return The stride in bytes of the tightly packed vertex stream
	 * defined by this VertexFormat a index nStreamIndex.
	 */
	UInt32 GetVertexStride(UInt nStreamIndex) const
	{
		SEOUL_ASSERT(nStreamIndex < kMaxStreams);
		return m_azVertexStride[nStreamIndex];
	}

	/**
	 * Update the vertex stride for a stream index from the default
	 * calculated stride. Can be used when vertex buffers do not contain
	 * tightly packed elements.
	 */
	void SetVertexStride(UInt nStreamIndex, UInt32 zVertexStrideInBytes)
	{
		SEOUL_ASSERT(nStreamIndex < kMaxStreams);
		m_azVertexStride[nStreamIndex] = zVertexStrideInBytes;
	}

protected:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(VertexFormat);

	VertexFormat(
		const VertexElements& vVertexElements)
		: m_vVertexElements(vVertexElements)
	{
		// Precompute the vertex stride for different vertex streams.
		memset(m_azVertexStride, 0, sizeof(m_azVertexStride));
		for (UInt32 i = 0u; i < kMaxStreams; ++i)
		{
			m_azVertexStride[i] = InternalCalculateVertexStride(i);
		}
	}

private:
	VertexElements m_vVertexElements;
	UInt32 m_azVertexStride[kMaxStreams];

	/**
	 * Helper function, used in the VertexFormat constructor to
	 * calculate the vertex stride of various tightly packed vertex
	 * streams that are defined by this VertexFormat.
	 */
	UInt32 InternalCalculateVertexStride(UInt nStreamIndex) const
	{
		UInt32 nBestOffset = 0u;
		UInt32 nStride = 0u;

		const UInt32 nElements = m_vVertexElements.GetSize();
		for (UInt32 i = 0u; i < nElements; ++i)
		{
			const VertexElement& element = m_vVertexElements[i];
			if (element.m_Stream == nStreamIndex &&
				element.m_Offset >= nBestOffset)
			{
				nBestOffset = element.m_Offset;
				nStride = (element.m_Offset + VertexElement::SizeInBytesFromType(element.m_Type));
			}
		}

		return nStride;
	}
}; // class VertexFormat

} // namespace Seoul

#endif // include guard
