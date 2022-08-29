/**
 * \file RenderCommandStreamBuilder.cpp
 * \brief RenderCommandStreamBuilder fulfills the same role as the GPU command buffer,
 * allowing Seoul encapsulation of render commands to be queued for later fulfillment
 * on the render thread.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionDefine.h"
#include "RenderCommandStreamBuilder.h"

namespace Seoul
{

SEOUL_TYPE(RenderCommandStreamBuilder, TypeFlags::kDisableNew);

SEOUL_BEGIN_TYPE(RenderStats)
	SEOUL_PROPERTY_N("DrawsSubmitted", m_uDrawsSubmitted)
	SEOUL_PROPERTY_N("MaxDrawsSubmitted", m_uMaxDrawsSubmitted)
	SEOUL_PROPERTY_N("TrianglesSubmittedForDraw", m_uTrianglesSubmittedForDraw)
	SEOUL_PROPERTY_N("MaxTrianglesSubmittedForDraw", m_uMaxTrianglesSubmittedForDraw)
	SEOUL_PROPERTY_N("EffectBegins", m_uEffectBegins)
	SEOUL_PROPERTY_N("MaxEffectBegins", m_uMaxEffectBegins)
SEOUL_END_TYPE()

RenderCommandStreamBuilder::RenderCommandStreamBuilder(UInt32 zInitialCapacity)
	: m_CommandStream(zInitialCapacity, MemoryBudgets::RenderCommandStream)
	, m_CurrentViewport()
	, m_bBufferLocked(false)
{
}

RenderCommandStreamBuilder::~RenderCommandStreamBuilder()
{
	InternalResetCommandStream();
}

void RenderCommandStreamBuilder::InternalResetCommandStream()
{
	m_CommandStream.Clear();
	m_References.Clear();
	m_vReadPixelCallbacks.Clear();
	m_vGrabFrameCallbacks.Clear();

	UInt32 const uBuffers = m_vBuffers.GetSize();
	for (UInt32 i = 0u; i < uBuffers; ++i)
	{
		void* p = m_vBuffers[i];
		m_vBuffers[i] = nullptr;
		MemoryManager::Deallocate(p);
	}

	m_vBuffers.Clear();
}

} // namespace Seoul
