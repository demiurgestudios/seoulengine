/**
 * \file NullGraphicsDevice.cpp
 * \brief Nop implementation of a GraphicsDevice for
 * contexts without graphics hardware.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "NullGraphicsDepthStencilSurface.h"
#include "NullGraphicsDevice.h"
#include "NullGraphicsEffect.h"
#include "NullGraphicsIndexBuffer.h"
#include "NullGraphicsRenderCommandStreamBuilder.h"
#include "NullGraphicsRenderTarget.h"
#include "NullGraphicsTexture.h"
#include "NullGraphicsVertexBuffer.h"
#include "NullGraphicsVertexFormat.h"

namespace Seoul
{

NullGraphicsDevice::NullGraphicsDevice(Int32 iNullViewportWidth /* = 800 */, Int32 iNullViewportHeight /* = 600 */)
	: m_iNullViewportWidth(iNullViewportWidth)
	, m_iNullViewportHeight(iNullViewportHeight)
	, m_BackBufferViewport()
	, m_vpVertexFormats()
	, m_VertexFormatsMutex()
	, m_vGraphicsObjects()
	, m_PendingGraphicsObjects()
{
	SEOUL_ASSERT(IsRenderThread());

	// Initialize back buffer formats
	m_eBackBufferDepthStencilFormat = DepthStencilFormat::kD24S8;
	m_eBackBufferPixelFormat = PixelFormat::kA8R8G8B8;
	m_Caps.m_bBackBufferWithAlpha = PixelFormatHasAlpha(m_eBackBufferPixelFormat);
	m_Caps.m_bBlendMinMax = true;
	m_Caps.m_bBGRA = true;
	m_Caps.m_bETC1 = false;
	m_Caps.m_bIncompleteMipChain = true;

	m_BackBufferViewport = InternalCreateDefaultViewport();
}

NullGraphicsDevice::~NullGraphicsDevice()
{
	SEOUL_ASSERT(IsRenderThread());

	m_vpVertexFormats.Clear();

	SEOUL_VERIFY(InternalDestructorMaintenance());
}

RenderCommandStreamBuilder* NullGraphicsDevice::CreateRenderCommandStreamBuilder(UInt32 zInitialCapacity) const
{
	return SEOUL_NEW(MemoryBudgets::Rendering) NullGraphicsRenderCommandStreamBuilder(zInitialCapacity);
}

Bool NullGraphicsDevice::BeginScene()
{
	SEOUL_ASSERT(IsRenderThread());

	if (!InternalPerFrameMaintenance())
	{
		return false;
	}

	return true;
}

void NullGraphicsDevice::EndScene()
{
	SEOUL_ASSERT(IsRenderThread());
}

const Viewport& NullGraphicsDevice::GetBackBufferViewport() const
{
	return m_BackBufferViewport;
}

RefreshRate NullGraphicsDevice::GetDisplayRefreshRate() const
{
	return RefreshRate();
}

SharedPtr<VertexFormat> NullGraphicsDevice::CreateVertexFormat(VertexElement const* pElements)
{
	Lock lock(m_VertexFormatsMutex);

	UInt32 size = 0u;

	// Count the elements, stop at the terminator.
	while (VertexElementEnd != pElements[size])
	{
		size++;
	}

	// Add one to include the terminator.
	size++;

	// Get the number of currently active formats
	const UInt32 nFormatCount = m_vpVertexFormats.GetSize();

	// For each format
	for (UInt32 i = 0u; i < nFormatCount; ++i)
	{
		// Get the vertex elements of the ith format.
		SEOUL_ASSERT(m_vpVertexFormats[i].IsValid());
		const NullGraphicsVertexFormat::VertexElements& vVertexElements =
			m_vpVertexFormats[i]->GetVertexElements();

		// If the size of elements is not equal to the size of elements
		// that we are attempting to add, we're done - these formats
		// cannot be the same.
		if (size != vVertexElements.GetSize())
		{
			continue;
		}

		// Otherwise, we need to walk the elements and compare all
		// of the members of each.

		Bool bEquivalent = true;
		for (UInt32 j = 0u; j < size; ++j)
		{
			const VertexElement& element = vVertexElements[j];

			if ((pElements[j].m_Stream != element.m_Stream) ||
				(pElements[j].m_Offset != element.m_Offset) ||
				(pElements[j].m_Type != element.m_Type) ||
				(pElements[j].m_Method != element.m_Method) ||
				(pElements[j].m_Usage != element.m_Usage) ||
				(pElements[j].m_UsageIndex != element.m_UsageIndex))
			{
				bEquivalent = false;
				break;
			}
		}

		// If an existing format has the same elements as the
		// elements we're adding, return that format.
		if (bEquivalent)
		{
			return m_vpVertexFormats[i];
		}
	}

	// Create a new vertex format
	NullGraphicsVertexFormat::VertexElements vElements(size);

	// Store the elements in a Vector<> to be added to the format
	// that is being created.
	for (UInt32 i = 0u; i < size; ++i)
	{
		vElements[i] = pElements[i];
	}

	SharedPtr<NullGraphicsVertexFormat> pFormat(SEOUL_NEW(MemoryBudgets::Rendering) NullGraphicsVertexFormat(
		vElements));
	InternalAddObject(pFormat);
	m_vpVertexFormats.PushBack(pFormat);
	return pFormat;
}


SharedPtr<DepthStencilSurface> NullGraphicsDevice::CreateDepthStencilSurface(const DataStoreTableUtil& configSettings)
{
	SharedPtr<DepthStencilSurface> pReturn(SEOUL_NEW(MemoryBudgets::Rendering) NullGraphicsDepthStencilSurface(configSettings));
	InternalAddObject(pReturn);
	return pReturn;
}

SharedPtr<RenderTarget> NullGraphicsDevice::CreateRenderTarget(const DataStoreTableUtil& configSettings)
{
	SharedPtr<RenderTarget> pReturn(SEOUL_NEW(MemoryBudgets::Rendering) NullGraphicsRenderTarget(configSettings));
	InternalAddObject(pReturn);
	return pReturn;
}

SharedPtr<IndexBuffer> NullGraphicsDevice::CreateIndexBuffer(
	void const* pInitialData,
	UInt32 zInitialDataSizeInBytes,
	UInt32 zTotalSizeInBytes,
	IndexBufferDataFormat eFormat)
{
	MemoryManager::Deallocate((void*)pInitialData);

	SharedPtr<IndexBuffer> pReturn(SEOUL_NEW(MemoryBudgets::Rendering) NullGraphicsIndexBuffer(
		zTotalSizeInBytes));
	InternalAddObject(pReturn);
	return pReturn;
}

SharedPtr<IndexBuffer> NullGraphicsDevice::CreateDynamicIndexBuffer(
	UInt32 zTotalSizeInBytes,
	IndexBufferDataFormat eFormat)
{
	SharedPtr<IndexBuffer> pReturn(SEOUL_NEW(MemoryBudgets::Rendering) NullGraphicsIndexBuffer(
		zTotalSizeInBytes));
	InternalAddObject(pReturn);
	return pReturn;
}

SharedPtr<VertexBuffer> NullGraphicsDevice::CreateVertexBuffer(
	void const* pInitialData,
	UInt32 zInitialDataSizeInBytes,
	UInt32 zTotalSizeInBytes,
	UInt32 zStrideInBytes)
{
	MemoryManager::Deallocate((void*)pInitialData);

	SharedPtr<VertexBuffer> pReturn(SEOUL_NEW(MemoryBudgets::Rendering) NullGraphicsVertexBuffer(
		zTotalSizeInBytes,
		zStrideInBytes));
	InternalAddObject(pReturn);
	return pReturn;
}

SharedPtr<VertexBuffer> NullGraphicsDevice::CreateDynamicVertexBuffer(
	UInt32 zTotalSizeInBytes,
	UInt32 zStrideInBytes)
{
	SharedPtr<VertexBuffer> pReturn(SEOUL_NEW(MemoryBudgets::Rendering) NullGraphicsVertexBuffer(
		zTotalSizeInBytes,
		zStrideInBytes));
	InternalAddObject(pReturn);
	return pReturn;
}

SharedPtr<BaseTexture> NullGraphicsDevice::CreateTexture(
	const TextureConfig& config,
	const TextureData& data,
	UInt uWidth,
	UInt uHeight,
	PixelFormat eFormat)
{
	SharedPtr<BaseTexture> pReturn(SEOUL_NEW(MemoryBudgets::Rendering) NullGraphicsTexture(
		uWidth,
		uHeight,
		eFormat,
		data.HasSecondary()));
	InternalAddObject(pReturn);
	return pReturn;
}

SharedPtr<Effect> NullGraphicsDevice::CreateEffectFromFileInMemory(
	FilePath filePath,
	void* pRawEffectFileData,
	UInt32 zFileSizeInBytes)
{
	SharedPtr<Effect> pReturn(SEOUL_NEW(MemoryBudgets::Rendering) NullGraphicsEffect(
		filePath,
		pRawEffectFileData,
		zFileSizeInBytes));
	InternalAddObject(pReturn);
	return pReturn;
}

Viewport NullGraphicsDevice::InternalCreateDefaultViewport() const
{
	Viewport viewport;
	viewport.m_iTargetWidth = m_iNullViewportWidth;
	viewport.m_iTargetHeight = m_iNullViewportHeight;
	viewport.m_iViewportX = 0;
	viewport.m_iViewportY = 0;
	viewport.m_iViewportWidth = viewport.m_iTargetWidth;
	viewport.m_iViewportHeight = viewport.m_iTargetHeight;
	return viewport;
}

void NullGraphicsDevice::InternalAddObject(const SharedPtr<BaseGraphicsObject>& pObject)
{
	if (IsRenderThread())
	{
		if (pObject->OnCreate())
		{
			pObject->OnReset();
		}

		m_vGraphicsObjects.PushBack(pObject);
	}
	else
	{
		SeoulGlobalIncrementReferenceCount(pObject.GetPtr());
		m_PendingGraphicsObjects.Push(pObject.GetPtr());
	}
}

Bool NullGraphicsDevice::InternalPerFrameMaintenance()
{
	// Cleanup existing objects
	Int nCount = (Int)m_vGraphicsObjects.GetSize();
	for (Int i = 0; i < nCount; ++i)
	{
		// If we have a unique reference, the object is no longer in use, so it can be destroyed.
		if (m_vGraphicsObjects[i].IsUnique())
		{
			m_vGraphicsObjects[i].Swap(m_vGraphicsObjects.Back());
			if (BaseGraphicsObject::kReset == m_vGraphicsObjects.Back()->GetState())
			{
				m_vGraphicsObjects.Back()->OnLost();
			}

			m_vGraphicsObjects.PopBack();
			--i;
			--nCount;
			continue;
		}

		// If an object is in the destroyed state, create it.
		if (BaseGraphicsObject::kDestroyed == m_vGraphicsObjects[i]->GetState())
		{
			// If we fail creating it, nothing more we can do.
			if (!m_vGraphicsObjects[i]->OnCreate())
			{
				return false;
			}
		}

		// If an object is in the lost state, reset it.
		if (BaseGraphicsObject::kCreated == m_vGraphicsObjects[i]->GetState())
		{
			m_vGraphicsObjects[i]->OnReset();
		}
	}

	// Handle pending objects in the queue.
	SharedPtr<BaseGraphicsObject> pObject(m_PendingGraphicsObjects.Pop());
	while (pObject.IsValid())
	{
		// Need to decrement the reference count once - it was incremented before inserting into the queue.
		SeoulGlobalDecrementReferenceCount(pObject.GetPtr());

		InternalAddObject(pObject);
		pObject.Reset(m_PendingGraphicsObjects.Pop());
	}

	return true;
}

Bool NullGraphicsDevice::InternalDestructorMaintenance()
{
	// Propagate pending objects.
	{
		Atomic32Type pendingObjectCount = m_PendingGraphicsObjects.GetCount();
		while (pendingObjectCount != 0)
		{
			if (!InternalPerFrameMaintenance())
			{
				return false;
			}

			Atomic32Type newPendingObjectCount = m_PendingGraphicsObjects.GetCount();
			if (newPendingObjectCount == pendingObjectCount)
			{
				break;
			}

			pendingObjectCount = newPendingObjectCount;
		}
	}

	// Now cleanup objects.
	{
		UInt32 uObjectCount = m_vGraphicsObjects.GetSize();
		while (uObjectCount != 0u)
		{
			if (!InternalPerFrameMaintenance())
			{
				return false;
			}

			UInt32 nNewObjectCount = m_vGraphicsObjects.GetSize();
			if (uObjectCount == nNewObjectCount)
			{
				return (0u == nNewObjectCount);
			}

			uObjectCount = nNewObjectCount;
		}
	}

	return true;
}

} // namespace Seoul
