/**
 * \file OGLES2RenderCommandStreamBuilder.h
 * \brief Specialization of RenderCommandStreamBuilder for the Open GL ES2 graphics system.
 * Handles execution of a command buffer of graphics commands with the ES2 api.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef OGLES2_RENDER_COMMAND_STREAM_BUILDER_H
#define OGLES2_RENDER_COMMAND_STREAM_BUILDER_H

#include "FixedArray.h"
#include "RenderCommandStreamBuilder.h"
#include "UnsafeHandle.h"

namespace Seoul
{

// Forward declarations
class GLSLFXLite;
class OGLES2StateManager;
class OGLES2VertexFormat;
class VertexFormat;

/** Maximum number of input vertex attributes that can be specified. */
static const UInt32 kVertexAttributeCount = 16u;

class OGLES2RenderCommandStreamBuilder SEOUL_SEALED : public RenderCommandStreamBuilder
{
public:
	OGLES2RenderCommandStreamBuilder(UInt32 zInitialCapacity);
	~OGLES2RenderCommandStreamBuilder();

	virtual void ExecuteCommandStream(RenderStats& rStats) SEOUL_OVERRIDE;

private:
	struct VertexStream
	{
		VertexStream()
			: m_pBuffer(nullptr)
			, m_zOffsetInBytes(0)
			, m_zStrideInBytes(0)
		{
		}

		Bool operator!=(const VertexStream& b) const
		{
			return (m_pBuffer != b.m_pBuffer ||
				m_zOffsetInBytes != b.m_zOffsetInBytes ||
				m_zStrideInBytes != b.m_zStrideInBytes);
		}

		VertexBuffer* m_pBuffer;
		UInt m_zOffsetInBytes;
		UInt m_zStrideInBytes;
	}; // struct VertexStream

	FixedArray<VertexStream, 2> m_aActiveStreams;
	UInt32 m_uActiveMinVertexIndex;
	FixedArray<VertexStream, 2> m_aCommittedStreams;
	CheckedPtr<VertexFormat> m_pActiveVertexFormat;
	CheckedPtr<VertexFormat> m_pCommitedVertexFormat;
	CheckedPtr<OGLES2IndexBuffer> m_pActiveIndexBuffer;

	FixedArray<Bool, kVertexAttributeCount> m_abActiveVertexAttributes;

	void InternalCommitVertexStreams(UInt32 uMinVertexIndex, UInt32 uInstance = 0u);

	Bool ReadEffectParameter(GLSLFXLite*& rpEffect, UnsafeHandle& rhParameter)
	{
		UnsafeHandle hEffect;
		UnsafeHandle hParameter;

		if (m_CommandStream.Read(&hEffect, sizeof(hEffect)) &&
			m_CommandStream.Read(&hParameter, sizeof(hParameter)))
		{
			rpEffect = StaticCast<GLSLFXLite*>(hEffect);
			rhParameter = hParameter;

			return true;
		}

		return false;
	}

	static UInt32 UpdateTexture(
		OGLES2StateManager& rStateManager,
		BaseTexture* pTexture,
		UInt uLevel,
		const Rectangle2DInt& rectangle,
		Byte* pSource);

	SEOUL_DISABLE_COPY(OGLES2RenderCommandStreamBuilder);
}; // class OGLES2RenderCommandStreamBuilder

} // namespace Seoul

#endif // include guard
