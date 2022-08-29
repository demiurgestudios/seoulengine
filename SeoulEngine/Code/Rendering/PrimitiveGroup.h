/**
 * \file PrimitiveGroup.h
 * \brief A set of primitives defined by a single index buffer.
 *
 * PrimitiveGroup is coupled with a vertex buffer and vertex format, defined
 * elsewhere, to complete the data needed to define renderable geometry.
 *
 * PrimitiveGroup can be used in classes like Mesh, to define multiple
 * drawable things per vertex buffer. A PrimitiveGroup can also be used
 * to just represent a bundle of drawable data.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef PRIMITIVE_GROUP_H
#define PRIMITIVE_GROUP_H

#include "SharedPtr.h"
#include "PrimitiveType.h"
#include "ScopedPtr.h"
namespace Seoul { class IndexBuffer; }
namespace Seoul { class SyncFile; }

namespace Seoul
{

class PrimitiveGroup SEOUL_SEALED
{
public:
	Bool Load(SyncFile& LevelFile);

	PrimitiveGroup();
	PrimitiveGroup(
		const SharedPtr<IndexBuffer>& pIndexBuffer,
		const SharedPtr<IndexBuffer>& pMirroredIndexBuffer,
		PrimitiveType eType,
		UInt nIndices,
		UInt iStartVert,
		UInt nVerts,
		Int iMaterialId);
	~PrimitiveGroup();

	/**
	 * @return The amount of memory occupied by this PrimitiveGroup's
	 * index buffer in bytes.
	 */
	UInt32 GetGraphicsMemoryUsageInBytes() const
	{
		return m_nNumIndices * sizeof(UInt16);
	}

	IndexBuffer* GetIndexBuffer() const
	{
		return m_pIndexBuffer.GetPtr();
	}

	/**
	 * Equivalent to GetIndexBuffer() when an object is mirrored.
	 * Inverted winding order so a mesh does not draw "inside out".
	 */
	IndexBuffer* GetMirroredIndexBuffer() const
	{
		return m_pMirroredIndexBuffer.GetPtr();
	}

	PrimitiveType GetPrimitiveType() const
	{
		return m_ePrimitiveType;
	}

	UInt GetNumIndices() const
	{
		return m_nNumIndices;
	}

	/**
	 * Sets the number of indices.
	 *
	 * This can often be used to adjust the PrimitiveGroup to draw from
	 * a subset of its index buffer. Note that if PrimitiveGroup::Load()
	 * is called, this will be reset to the total count of the entire
	 * IndexBuffer that is read from disk.
	 */
	void SetNumIndices(UInt nNumIndices)
	{
		m_nNumIndices = nNumIndices;
	}

	/**
	 * Gets the number of primitives defined by this PrimitiveGroup,
	 * derived from the PrimitiveType and the number of indices.
	 */
	UInt GetNumPrimitives() const
	{
		return GetNumberOfPrimitives(m_ePrimitiveType, m_nNumIndices);
	}

	/**
	 * Offset into the vertex buffer that this PrimitiveGroup is
	 * drawn with at which the PrimitiveGroup's vertices begin.
	 */
	UInt GetStartVertex() const
	{
		return m_StartVertex;
	}

	void SetStartVertex(UInt startVertex)
	{
		m_StartVertex = startVertex;
	}

	/**
	 * The total number of vertices that this index buffer
	 * will access.
	 *
	 * Those vertices will be contiguous, and the number
	 * will be less than or equal to the total number of vertices in the
	 * the vertex buffer that this PrimitiveGroup is rendered with.
	 */
	UInt GetNumVertices() const
	{
		return m_nNumVertices;
	}

	void SetNumVertices(UInt nNumVertices)
	{
		m_nNumVertices = nNumVertices;
	}

	/**
	 * The material ID is used to store an index into an array
	 * of materials held by this PrimitiveGroup's owner, to attach
	 * the PrimitiveGroup to a particular drawable material.
	 *
	 * MaterialId can be -1, which indicates no material.
	 *
	 * Materials are stored this way for usages like Model,
	 * which clone the original set of Materials that a mesh uses. Storing
	 * a pointer to the material itself would result in the wrong material
	 * being used.
	 */
	Int GetMaterialId() const
	{
		return m_iMaterialId;
	}

private:
	SharedPtr<IndexBuffer> m_pIndexBuffer;
	SharedPtr<IndexBuffer> m_pMirroredIndexBuffer;
	PrimitiveType m_ePrimitiveType;
	UInt m_nNumIndices;
	UInt m_StartVertex;
	UInt m_nNumVertices;
	Int m_iMaterialId;

	void InternalClear();

	SEOUL_DISABLE_COPY(PrimitiveGroup);
}; // class PrimitiveGroup

} // namespace Seoul

#endif // include guard
