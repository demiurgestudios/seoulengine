/**
 * \file PrimitiveGroup.cpp
 * \brief A set of primitives defined by a single index buffer.
 *
 * PrimitiveGroup is coupled with a vertex buffer and vertex format, defined
 * elsewhere, to complete the data needed to define renderable geometry.
 *
 * PrimitiveGroup can be used in classes like Mesh, to define multiple
 * drawable things per vertex buffer. A PrimitiveGroup can also just be used
 * to represent a bundle of drawable data.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "PrimitiveGroup.h"
#include "RenderDevice.h"
#include "SeoulFileReaders.h"

namespace Seoul
{

/**
 * Utility, creates a new heap allocated buffer
 * of indices equivalent to the input indices,
 * with flipped winding order.
 */
static SharedPtr<IndexBuffer> MirrorIndices(
	const SharedPtr<IndexBuffer>& pUnmirrored,
	UInt16 const* pInputIndices,
	UInt32 uNumIndices,
	PrimitiveType ePrimitiveType)
{
	switch (ePrimitiveType)
	{
	case PrimitiveType::kLineList: // fall-through
	case PrimitiveType::kLineStrip: // fall-through
	case PrimitiveType::kNone: // fall-through
	case PrimitiveType::kPointList:
		// Nop, exact copy, reuse unmirrored version.
		return pUnmirrored;

	case PrimitiveType::kTriangleList:
	{
		UInt32 const uSizeInBytes = (UInt32)(sizeof(UInt16) * uNumIndices);
		UInt16* pMirroredIndices = (UInt16*)MemoryManager::AllocateAligned(
			uSizeInBytes,
			MemoryBudgets::Rendering,
			SEOUL_ALIGN_OF(UInt16));
		memcpy(pMirroredIndices, pInputIndices, uSizeInBytes);

		for (UInt32 i = 0u; i < uNumIndices; i += 3u)
		{
			Seoul::Swap(pMirroredIndices[i + 1u], pMirroredIndices[i + 2u]);
		}

		return RenderDevice::Get()->CreateIndexBuffer(
			pMirroredIndices,
			uNumIndices * sizeof(UInt16),
			uNumIndices * sizeof(UInt16),
			IndexBufferDataFormat::kIndex16);
	}

	default:
		SEOUL_FAIL("Out-of-sync enum.");
		return pUnmirrored;
	};
}

PrimitiveGroup::PrimitiveGroup()
	: m_pIndexBuffer(nullptr)
	, m_pMirroredIndexBuffer(nullptr)
	, m_ePrimitiveType(PrimitiveType::kNone)
	, m_nNumIndices(0u)
	, m_StartVertex(0u)
	, m_nNumVertices(0u)
	, m_iMaterialId(-1)
{
}

/**
 * Constructs a PrimitiveGroup from the data that defines
 * it.
 */
PrimitiveGroup::PrimitiveGroup(
	const SharedPtr<IndexBuffer>& pIndexBuffer,
	const SharedPtr<IndexBuffer>& pMirroredIndexBuffer,
	PrimitiveType eType,
	UInt nIndices,
	UInt iStartVert,
	UInt nVerts,
	Int iMaterialId)
	: m_pIndexBuffer(pIndexBuffer)
	, m_pMirroredIndexBuffer(pMirroredIndexBuffer)
	, m_ePrimitiveType(eType)
	, m_nNumIndices(nIndices)
	, m_StartVertex(iStartVert)
	, m_nNumVertices(nVerts)
	, m_iMaterialId(iMaterialId)
{
}

PrimitiveGroup::~PrimitiveGroup()
{
}

/**
 * Load the PrimitiveGroup from a sync file.
 *
 * If reading fails for any reason, this function returns immediately
 * with the error code RESULT_ERROR. The file pointer of the file parameter
 * will be at an unspecified position. The PrimitiveGroup will also be
 * restored to its default state and any resources created during the partial
 * load will be released.
 */
Bool PrimitiveGroup::Load(SyncFile& file)
{
	InternalClear();

	UInt16* pRawData = nullptr;

	CheckedPtr<RenderDevice> pDevice(RenderDevice::Get());

	// Read and verify the PrimitiveGroup delimiter
	if (!VerifyDelimiter(DataTypePrimitiveGroup, file))
	{
		goto error;
	}

	// Material index
	if (!ReadInt32(file, m_iMaterialId)) { goto error; }

	// PrimitiveType
	if (!ReadEnum(file, m_ePrimitiveType)) { goto error; }

	// Index count
	if (!ReadUInt32(file, m_nNumIndices)) { goto error; }

	// Start vertex
	if (!ReadUInt32(file, m_StartVertex)) { goto error; }

	// Vertex count
	if (!ReadUInt32(file, m_nNumVertices)) { goto error;; }

	pRawData = (UInt16*)MemoryManager::AllocateAligned(
		m_nNumIndices * sizeof(UInt16),
		MemoryBudgets::Rendering,
		SEOUL_ALIGN_OF(UInt16));

	if (file.ReadRawData(pRawData, sizeof(UInt16) * m_nNumIndices) != sizeof(UInt16) * m_nNumIndices)
	{
		MemoryManager::Deallocate(pRawData);
		goto error;
	}

	// Read index buffer data into an IndexBuffer object.
	m_pIndexBuffer = pDevice->CreateIndexBuffer(
		pRawData,
		m_nNumIndices * sizeof(UInt16),
		m_nNumIndices * sizeof(UInt16),
		IndexBufferDataFormat::kIndex16);

	// Now create a mirrored buffer.
	m_pMirroredIndexBuffer = MirrorIndices(
		m_pIndexBuffer,
		pRawData,
		m_nNumIndices,
		m_ePrimitiveType);

	return true;

error:
	InternalClear();
	return false;
}

/**
 * Restores the primitive group to its default state, releasing
 * any resources.
 */
void PrimitiveGroup::InternalClear()
{
	m_iMaterialId = -1;
	m_nNumVertices = 0u;
	m_StartVertex = 0u;
	m_nNumIndices = 0u;
	m_ePrimitiveType = PrimitiveType::kNone;
	m_pMirroredIndexBuffer.Reset();
	m_pIndexBuffer.Reset();
}

} // namespace Seoul
