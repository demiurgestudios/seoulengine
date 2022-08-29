/**
 * \file OGLES2VertexBuffer.cpp
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

#include "Algorithms.h"
#include "OGLES2RenderDevice.h"
#include "OGLES2VertexBuffer.h"

namespace Seoul
{

OGLES2VertexBuffer::OGLES2VertexBuffer(
	void const* pInitialData,
	UInt32 zInitialDataSizeInBytes,
	UInt32 zTotalSizeInBytes,
	UInt32 zVertexStrideInBytes,
	Bool bDynamic)
	: VertexBuffer(zVertexStrideInBytes, zTotalSizeInBytes)
	, m_pDynamicData(nullptr)
	, m_pInitialData(pInitialData)
	, m_zInitialDataSizeInBytes(zInitialDataSizeInBytes)
	, m_VertexBuffer(0)
	, m_bDynamic(bDynamic)
{
	// Cannot have initial data for a dynamic buffer.
	SEOUL_ASSERT(zInitialDataSizeInBytes <= zTotalSizeInBytes);
	SEOUL_ASSERT(!m_bDynamic || nullptr == m_pInitialData);

	// If this is a dynamic buffer, allocate a system memory area to store
	// the data.
	if (bDynamic)
	{
		m_pDynamicData = (Byte*)MemoryManager::Allocate(zTotalSizeInBytes, MemoryBudgets::Rendering);
	}
}

OGLES2VertexBuffer::~OGLES2VertexBuffer()
{
	SEOUL_ASSERT(IsRenderThread());

	// Clean up the vertex buffer object.
	if (0u != m_VertexBuffer)
	{
		glDeleteBuffers(1, &m_VertexBuffer);
	}

	// Clean up the dynamic buffer, if it exists.
	if (nullptr != m_pDynamicData)
	{
		MemoryManager::Deallocate(m_pDynamicData);
		m_pDynamicData = nullptr;
	}

	// Destroy the initial data, if it's still allocated.
	InternalFreeInitialData();
}

Bool OGLES2VertexBuffer::OnCreate()
{
	SEOUL_ASSERT(IsRenderThread());

	// If this is not a dynamic buffer, create an OpenGl object.
	if (nullptr == m_pDynamicData)
	{
		glGenBuffers(1, &m_VertexBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer);

		GLenum usage = (GL_STATIC_DRAW);
		glBufferData(
			GL_ARRAY_BUFFER,
			GetTotalSizeInBytes(),
			m_pInitialData,
			usage);
	}
	// Otherwise, just initialize the dynamic buffer area.
	else if (nullptr != m_pDynamicData)
	{
		memcpy(m_pDynamicData, m_pInitialData, m_zInitialDataSizeInBytes);
	}

	InternalFreeInitialData();

	SEOUL_VERIFY(VertexBuffer::OnCreate());
	return true;
}

// Destroy the initial data, if it's still allocated.
void OGLES2VertexBuffer::InternalFreeInitialData()
{
	if (nullptr != m_pInitialData)
	{
		MemoryManager::Deallocate((void*)m_pInitialData);
		m_pInitialData = nullptr;
	}

	m_zInitialDataSizeInBytes = 0u;
}

} // namespace Seoul
