/**
 * \file D3D11VertexBuffer.cpp
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
#include "D3D11VertexBuffer.h"
#include "D3D11Util.h"
#include "ThreadId.h"

namespace Seoul
{

D3D11VertexBuffer::D3D11VertexBuffer(
	void const* pInitialData,
	UInt32 zInitialDataSizeInBytes,
	UInt32 zTotalSizeInBytes,
	UInt32 zVertexStrideInBytes,
	Bool bDynamic)
	: VertexBuffer(zVertexStrideInBytes, zTotalSizeInBytes)
	, m_pInitialData(pInitialData)
	, m_zInitialDataSizeInBytes(zInitialDataSizeInBytes)
	, m_pVertexBuffer(nullptr)
	, m_bDynamic(bDynamic)
{
	// Cannot have initial data for a dynamic buffer.
	SEOUL_ASSERT(zInitialDataSizeInBytes <= zTotalSizeInBytes);
	SEOUL_ASSERT(!m_bDynamic || nullptr == m_pInitialData);
}

D3D11VertexBuffer::~D3D11VertexBuffer()
{
	SEOUL_ASSERT(IsRenderThread());

	// Clean up the vertex buffer object.
	SafeRelease(m_pVertexBuffer);

	// Destroy the initial data, if it's still allocated.
	InternalFreeInitialData();
}

Bool D3D11VertexBuffer::OnCreate()
{
	SEOUL_ASSERT(IsRenderThread());

	D3D11_BUFFER_DESC desc;
	memset(&desc, 0, sizeof(desc));
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.ByteWidth = GetTotalSizeInBytes();
	desc.CPUAccessFlags = (m_bDynamic ? D3D11_CPU_ACCESS_WRITE : 0u);
	desc.MiscFlags = 0;
	desc.StructureByteStride = GetVertexStrideInBytes();
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
		m_pVertexBuffer = pBuffer;
		SEOUL_VERIFY(VertexBuffer::OnCreate());
		return true;
	}

	return false;
}

void D3D11VertexBuffer::InternalFreeInitialData()
{
	if (nullptr != m_pInitialData)
	{
		MemoryManager::Deallocate((void*)m_pInitialData);
		m_pInitialData = nullptr;
	}

	m_zInitialDataSizeInBytes = 0u;
}

} // namespace Seoul
