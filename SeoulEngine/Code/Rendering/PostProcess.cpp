/**
 * \file PostProcess.cpp
 * \brief A poseable which represents a post-process.
 *
 * Post-processing is typically applied to the final rendered image, to
 * apply screen space effects such as coloration, DOF, motion blur, etc.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EffectPass.h"
#include "IndexBuffer.h"
#include "PostProcess.h"
#include "ReflectionDataStoreTableUtil.h"
#include "ReflectionDefine.h"
#include "RenderCommandStreamBuilder.h"
#include "RenderDevice.h"
#include "Renderer.h"
#include "RenderPass.h"
#include "VertexBuffer.h"
#include "VertexFormat.h"

namespace Seoul
{

static const HString kParameterTexture("seoul_Texture");
static const HString kSourceTarget("SourceTarget");

SEOUL_BEGIN_TYPE(PostProcess)
	SEOUL_PARENT(IPoseable)
SEOUL_END_TYPE()

struct PostProcessVertex SEOUL_SEALED
{
	Vector2D m_vP;
	Vector2D m_vT;
}; // struct PostProcessVertex
SEOUL_STATIC_ASSERT(sizeof(PostProcessVertex) == 16);

static UInt16* CreateIndexBuffer()
{
	UInt16* pReturn = (UInt16*)MemoryManager::AllocateAligned(
		sizeof(UInt16) * 6,
		MemoryBudgets::Rendering,
		SEOUL_ALIGN_OF(UInt16));

	pReturn[0] = 0;
	pReturn[1] = 1;
	pReturn[2] = 2;
	pReturn[3] = 0;
	pReturn[4] = 2;
	pReturn[5] = 3;

	return pReturn;
}

static PostProcessVertex* CreateVertexBuffer()
{
	PostProcessVertex* pReturn = (PostProcessVertex*)MemoryManager::AllocateAligned(
		sizeof(PostProcessVertex) * 4,
		MemoryBudgets::Rendering,
		SEOUL_ALIGN_OF(UInt16));

	pReturn[0].m_vP = Vector2D(-1,  1);
	pReturn[0].m_vT = Vector2D( 0,  0);
	pReturn[1].m_vP = Vector2D(-1, -1);
	pReturn[1].m_vT = Vector2D( 0,  1);
	pReturn[2].m_vP = Vector2D( 1, -1);
	pReturn[2].m_vT = Vector2D( 1,  1);
	pReturn[3].m_vP = Vector2D( 1,  1);
	pReturn[3].m_vT = Vector2D( 1,  0);

	return pReturn;
}

static SharedPtr<VertexFormat> CreateVertexFormat()
{
	static const VertexElement s_kaVertexFormat[] =
	{
		// Position (in stream 0)
		{ 0,                               // stream
		  0,                               // offset
		  VertexElement::TypeFloat2,       // type
		  VertexElement::MethodDefault,    // method
		  VertexElement::UsagePosition,    // usage
		  0u },                            // usage index

		// Texcoords (in stream 0)
		{ 0,                               // stream
		  8,                               // offset
		  VertexElement::TypeFloat2,       // type
		  VertexElement::MethodDefault,    // method
		  VertexElement::UsageTexcoord,    // usage
		  0u },                            // usage index

		VertexElementEnd
	};

	return RenderDevice::Get()->CreateVertexFormat(s_kaVertexFormat);
}

PostProcess::PostProcess(const DataStoreTableUtil& configSettings)
	: m_pIndexBuffer(RenderDevice::Get()->CreateIndexBuffer(CreateIndexBuffer(), sizeof(UInt16) * 6, sizeof(UInt16) * 6, IndexBufferDataFormat::kIndex16))
	, m_pSourceTarget()
	, m_pVertexBuffer(RenderDevice::Get()->CreateVertexBuffer(CreateVertexBuffer(), sizeof(PostProcessVertex) * 4, sizeof(PostProcessVertex) * 4, sizeof(PostProcessVertex)))
	, m_pVertexFormat(CreateVertexFormat())
{
	// Acquire the source target. Ok if this is undefined, we just set the empty texture.
	HString target;
	(void)configSettings.GetValue(kSourceTarget, target);
	m_pSourceTarget.Reset(Renderer::Get()->GetRenderTarget(target));
}

PostProcess::~PostProcess()
{
}

/**
* Post a PostProcess. Pose our various draw instructions
* into our render pass's render tree.
*/
void PostProcess::Pose(
	Float fDeltaTime,
	RenderPass& rPass,
	IPoseable* pParent /*= nullptr*/)
{
	RenderCommandStreamBuilder& rBuilder = *rPass.GetRenderCommandStreamBuilder();
	BeginPass(rBuilder, rPass);

	auto pEffect(rPass.GetPassEffect().GetPtr());
	if (pEffect.IsValid())
	{
		// Assume the draw effect is only 1 pass.
		auto pass = rBuilder.BeginEffect(pEffect, GetEffectTechnique());
		if (pass.IsValid())
		{
			// If the pass succeeds, setup draw properties and draw.
			if (rBuilder.BeginEffectPass(pEffect, pass))
			{
				// Select the format, indices, and vertex buffer.
				rBuilder.UseVertexFormat(m_pVertexFormat.GetPtr());
				rBuilder.SetIndices(m_pIndexBuffer.GetPtr());
				rBuilder.SetVertices(0u, m_pVertexBuffer.GetPtr(), 0u, m_pVertexBuffer->GetVertexStrideInBytes());

				// Set the source target.
				rBuilder.SetTextureParameter(pEffect, kParameterTexture, TextureContentHandle(m_pSourceTarget.GetPtr()));

				// Commit changes to the pass.
				rBuilder.CommitEffectPass(pEffect, pass);

				// Issue the draw call.
				rBuilder.DrawIndexedPrimitive(
					PrimitiveType::kTriangleList,
					0,
					0,
					4,
					0,
					2);

				// TODO: Move this detail under the hood so it isn't necessary in client
				// code.

				// Clear the texture association.
				rBuilder.SetTextureParameter(pEffect, kParameterTexture, TextureContentHandle());
				rBuilder.CommitEffectPass(pEffect, pass);

				// Done with the pass.
				rBuilder.EndEffectPass(pEffect, pass);
			}

			// Done with the effect.
			rBuilder.EndEffect(pEffect);
		}
	}

	EndPass(rBuilder, rPass);
}

} // namespace Seoul
