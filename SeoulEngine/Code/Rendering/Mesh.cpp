/**
 * \file Mesh.cpp
 * \brief A mesh contains vertex data and a collection of primitive groups
 * that describe renderable geometry, potentially with multiple materials.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Matrix3x4.h"
#include "Mesh.h"
#include "PrimitiveGroup.h"
#include "ReflectionDefine.h"
#include "RenderDevice.h"
#include "SeoulFileReaders.h"
#include "VertexElement.h"
#include "VertexFormat.h"

namespace Seoul
{

Mesh::Mesh()
	: m_pMaterialLibrary()
	, m_vPrimitiveGroups()
	, m_AABB()
	, m_pVertexBuffer()
	, m_pVertexFormat()
	, m_zGraphicsMemoryUsageInBytes(0u)
	, m_vInverseBindPoses()
#if SEOUL_EDITOR_AND_TOOLS
	, m_vEditorVertices()
#endif // /#if SEOUL_EDITOR_AND_TOOLS
{
}

Mesh::~Mesh()
{
	InternalDestroy();
}

/**
 * Loads this Mesh from a SOL file as binary data.
 *
 * \pre file's file pointer must be located at the beginning of the
 * Mesh data.
 *
 * @return RESULT_OK if loading succeeds, RESULT_ERROR otherwise. If this
 * method returns RESULT_ERROR, the status of file's file pointer is
 * undefined.
 *
 * If load fails, this Mesh will be cleared and set to its default
 * state.
 */
Bool Mesh::Load(FilePath filePath, SyncFile& file)
{
	InternalDestroy();

	Vector<VertexElement> vVertexElements;

	// verify that this is mesh data.
	if (!VerifyDelimiter(DataTypeMesh, file)) { goto error; }

	// read local space bounding box.
	if (!ReadAABB(file, m_AABB)) { goto error; }

	// vertex format.
	if (!InternalLoadVertexFormat(file, vVertexElements)) { goto error; }

	// vertex buffer.
	if (!InternalLoadVertexBuffer(file, vVertexElements)) { goto error; }

	// primitive groups.
	if (!InternalLoadPrimitiveGroups(file)) { goto error; }

	// inverse bind pose transforms.
	if (!ReadBuffer(file, m_vInverseBindPoses)) { goto error; }

	return true;

error:
	InternalDestroy();
	return false;
}

/**
 * Restores this Mesh to its default state and releases
 * all resources that it owns.
 */
void Mesh::InternalDestroy()
{
#if SEOUL_EDITOR_AND_TOOLS
	m_vEditorVertices.Clear();
#endif // /#if SEOUL_EDITOR_AND_TOOLS
	m_vInverseBindPoses.Clear();
	m_zGraphicsMemoryUsageInBytes = 0u;
	m_pVertexFormat.Reset();
	m_pVertexBuffer.Reset();
	m_AABB = AABB::CreateFromCenterAndExtents(
		Vector3D::Zero(),
		Vector3D::Zero());
	SafeDeleteVector(m_vPrimitiveGroups);
}

/**
 * Helper function, loads a VertexFormat from a file
 * as binary data.
 *
 * @return True if loading succeeds, false otherwise.
 */
Bool Mesh::InternalLoadVertexFormat(SyncFile& file, Vector<VertexElement>& rvVertexElements)
{
	// Verify vertex declaration data type.
	if (!VerifyDelimiter(DataTypeVertexDecl, file)) { return false; }

	// Read vertex declaration element count.
	UInt32 nElementCount = 0u;
	if (!ReadUInt32(file, nElementCount)) { return false; }

	rvVertexElements.Resize(nElementCount + 1u);
	for (UInt32 i = 0u; i < nElementCount; ++i)
	{
		if (!VerifyDelimiter(DataTypeVertexElement, file)) { return false; }

		VertexElement& element = rvVertexElements[i];
		if (!ReadUInt16(file, element.m_Stream)) { return false; }
		if (!ReadUInt16(file, element.m_Offset)) { return false; }
		if (!ReadUInt8(file, element.m_Type)) { return false; }
		if (!ReadUInt8(file, element.m_Method)) { return false; }
		if (!ReadUInt8(file, element.m_Usage)) { return false; }
		if (!ReadUInt8(file, element.m_UsageIndex)) { return false; }
	}
	rvVertexElements[nElementCount] = VertexElementEnd;

	m_pVertexFormat = RenderDevice::Get()->CreateVertexFormat(rvVertexElements.Get(0u));

	return true;
}

/**
* Helper function, loads a VertexBuffer from a file
* as binary data.
*
* @return True if loading succeeds, false otherwise.
*/
Bool Mesh::InternalLoadVertexBuffer(SyncFile& file, const Vector<VertexElement>& vVertexElements)
{
	UInt32 nVertexBufferSize;
	if (!ReadUInt32(file, nVertexBufferSize)) { return false; }

	m_zGraphicsMemoryUsageInBytes += nVertexBufferSize;

	void* pData = MemoryManager::Allocate(nVertexBufferSize, MemoryBudgets::Rendering);
	if (file.ReadRawData(pData, nVertexBufferSize) != nVertexBufferSize)
	{
		MemoryManager::Deallocate(pData);
		return false;
	}

	// Acquire positions if in the editor.
#if SEOUL_EDITOR_AND_TOOLS

	// Find the offset to position - we only capture if
	// the position component is at least 3 elements.
	Int32 iOffset = -1;
	for (auto const& elem : m_pVertexFormat->GetVertexElements())
	{
		if (elem.m_Usage == VertexElement::UsagePosition &&
			(elem.m_Type == VertexElement::TypeFloat4 || elem.m_Type == VertexElement::TypeFloat3))
		{
			iOffset = (Int32)elem.m_Offset;
			break;
		}
	}

	// Capture position if found.
	if (iOffset >= 0)
	{
		// Compute expect number of vertices and reserve that much space.
		UInt32 const uStride = m_pVertexFormat->GetVertexStride(0u);
		m_vEditorVertices.Reserve(nVertexBufferSize / uStride);

		// Iterate and append.
		for (Int32 i = iOffset; iOffset + sizeof(Vector3D) <= (Int32)nVertexBufferSize; iOffset += (Int32)uStride)
		{
			Vector3D v;
			memcpy(&v, (Byte const*)pData + iOffset, sizeof(Vector3D));
			m_vEditorVertices.PushBack(v);
		}
	}
#endif // /#if SEOUL_EDITOR_AND_TOOLS

	m_pVertexBuffer = RenderDevice::Get()->CreateVertexBuffer(
		pData,
		nVertexBufferSize,
		nVertexBufferSize,
		m_pVertexFormat->GetVertexStride(0u));
	if (!m_pVertexBuffer.IsValid()) { return false; }

	return true;
}

/**
* Helper function, loads a list of PrimitiveGroups
* from a file as binary data.
*
* @return True if loading succeeds, false otherwise.
*/
Bool Mesh::InternalLoadPrimitiveGroups(SyncFile& file)
{
	// Read primitive group count
	UInt32 nPrimitiveGroupCount;
	if (!ReadUInt32(file, nPrimitiveGroupCount)) { return false; }

	// We depend on InternalClear() being called before this function
	// so that the primitive group Vector is empty, to avoid
	// memory leaks.
	SEOUL_ASSERT(m_vPrimitiveGroups.IsEmpty());
	m_vPrimitiveGroups.Resize(nPrimitiveGroupCount);
	for (UInt32 i = 0u; i < nPrimitiveGroupCount; ++i)
	{
		m_vPrimitiveGroups[i] = SEOUL_NEW(MemoryBudgets::Rendering) PrimitiveGroup();
		SEOUL_ASSERT(m_vPrimitiveGroups[i]);

		if (!m_vPrimitiveGroups[i]->Load(file)) { return false; }

		m_zGraphicsMemoryUsageInBytes += m_vPrimitiveGroups[i]->GetGraphicsMemoryUsageInBytes();
	}

	return true;
}

/**
 * @return The number of primitives across all the primitive groups in
 * this Mesh.
 */
UInt32 Mesh::GetPrimitiveCount() const
{
	UInt32 nReturn = 0u;
	const UInt32 nPrimitiveGroups = m_vPrimitiveGroups.GetSize();
	for (UInt32 i = 0u; i < nPrimitiveGroups; ++i)
	{
		SEOUL_ASSERT(m_vPrimitiveGroups[i]);
		nReturn += m_vPrimitiveGroups[i]->GetNumPrimitives();
	}

	return nReturn;
}

} // namespace Seoul
