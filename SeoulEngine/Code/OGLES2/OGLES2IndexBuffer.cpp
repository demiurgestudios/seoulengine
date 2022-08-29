/**
* \file OGLES2IndexBuffer.cpp
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

#include "OGLES2IndexBuffer.h"
#include "OGLES2RenderDevice.h"

namespace Seoul
{

OGLES2IndexBuffer::OGLES2IndexBuffer(
	void const* pInitialData,
	UInt32 zInitialDataSizeInBytes,
	UInt32 zTotalSizeInBytes,
	IndexBufferDataFormat eFormat,
	Bool bDynamic)
	: IndexBuffer(zTotalSizeInBytes)
	, m_pDynamicData(nullptr)
	, m_pInitialData(pInitialData)
	, m_zInitialDataSizeInBytes(zInitialDataSizeInBytes)
	, m_eFormat(eFormat)
	, m_IndexBuffer(0)
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

OGLES2IndexBuffer::~OGLES2IndexBuffer()
{
	SEOUL_ASSERT(IsRenderThread());

	// Clean up the index buffer object.
	if (0u != m_IndexBuffer)
	{
		glDeleteBuffers(1, &m_IndexBuffer);
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

Bool OGLES2IndexBuffer::OnCreate()
{
	SEOUL_ASSERT(IsRenderThread());

	// If this is not a dynamic buffer, create an OpenGl object.
	if (nullptr == m_pDynamicData)
	{
		glGenBuffers(1, &m_IndexBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexBuffer);

		GLenum usage = (GL_STATIC_DRAW);
		glBufferData(
			GL_ELEMENT_ARRAY_BUFFER,
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

	SEOUL_VERIFY(IndexBuffer::OnCreate());
	return true;
}

// Destroy the initial data, if it's still allocated.
void OGLES2IndexBuffer::InternalFreeInitialData()
{
	if (nullptr != m_pInitialData)
	{
		MemoryManager::Deallocate((void*)m_pInitialData);
		m_pInitialData = nullptr;
	}

	m_zInitialDataSizeInBytes = 0u;
}

} // namespace Seoul
