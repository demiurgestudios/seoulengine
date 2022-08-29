/**
 * \file D3D11IndexBuffer.cpp
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

#include "D3D11Device.h"
#include "D3D11IndexBuffer.h"
#include "D3D11Util.h"
#include "ThreadId.h"

namespace Seoul
{

D3D11IndexBuffer::D3D11IndexBuffer(
	void const* pInitialData,
	UInt32 zInitialDataSizeInBytes,
	UInt32 zTotalSizeInBytes,
	IndexBufferDataFormat eFormat,
	Bool bDynamic)
	: IndexBuffer(zTotalSizeInBytes)
	, m_pInitialData(pInitialData)
	, m_zInitialDataSizeInBytes(zInitialDataSizeInBytes)
	, m_eFormat(eFormat)
	, m_pIndexBuffer(nullptr)
	, m_bDynamic(bDynamic)
{
	// Cannot have initial data for a dynamic buffer.
	SEOUL_ASSERT(zInitialDataSizeInBytes <= zTotalSizeInBytes);
	SEOUL_ASSERT(!m_bDynamic || nullptr == m_pInitialData);
}

D3D11IndexBuffer::~D3D11IndexBuffer()
{
	SEOUL_ASSERT(IsRenderThread());

	// Clean up the index buffer object.
	SafeRelease(m_pIndexBuffer);

	// Destroy the initial data, if it's still allocated.
	InternalFreeInitialData();
}

Bool D3D11IndexBuffer::OnCreate()
{
	SEOUL_ASSERT(IsRenderThread());

	D3D11_BUFFER_DESC desc;
	memset(&desc, 0, sizeof(desc));
	desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	desc.ByteWidth = GetTotalSizeInBytes();
	desc.CPUAccessFlags = (m_bDynamic ? D3D11_CPU_ACCESS_WRITE : 0u);
	desc.MiscFlags = 0;
	desc.StructureByteStride = (IndexBufferDataFormat::kIndex32 == m_eFormat ? 4u : 2u);
	desc.Usage = (m_bDynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT);

	D3D11_SUBRESOURCE_DATA data;
	memset(&data, 0, sizeof(data));
	data.pSysMem = m_pInitialData;

	ID3D11Buffer* pBuffer = nullptr;
	if (S_OK == GetD3D11Device().GetD3DDevice()->CreateBuffer(
		&desc,
		(nullptr != m_pInitialData ? &data : nullptr),
		&pBuffer) && nullptr != pBuffer)
	{
		InternalFreeInitialData();
		m_pIndexBuffer = pBuffer;
		SEOUL_VERIFY(IndexBuffer::OnCreate());
		return true;
	}

	return false;
}

void D3D11IndexBuffer::InternalFreeInitialData()
{
	if (nullptr != m_pInitialData)
	{
		MemoryManager::Deallocate((void*)m_pInitialData);
		m_pInitialData = nullptr;
	}

	m_zInitialDataSizeInBytes = 0u;
}

} // namespace Seoul
