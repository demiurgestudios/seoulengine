/**
 * \file SceneFxRenderer.cpp
 * \brief Utility that handles rendering of particles defined
 * in scene Fx.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Camera.h"
#include "Effect.h"
#include "Matrix4D.h"
#include "RenderCommandStreamBuilder.h"
#include "RenderDevice.h"
#include "StandardVertex2D.h"
#include "TextureManager.h"
#include "SceneFxRenderer.h"
#include "ScenePrereqs.h"

#if SEOUL_WITH_SCENE

namespace Seoul::Scene
{

static const UInt32 kuMaxInstances(1024u);
static const UInt32 kuIndexBufferSizeInIndices(kuMaxInstances * 6u);
static const UInt32 kuVertexBufferSizeInVertices(kuMaxInstances * 4u);

static const HString kEffectParameterTexture("seoul_Texture");
#if SEOUL_EDITOR_AND_TOOLS // TODO: Do we need this outisde of the editor?
static const HString kEffectParameterTextureDimensions("seoul_TextureDimensions");
#endif // /#if SEOUL_EDITOR_AND_TOOLS

struct WorldFxVertex SEOUL_SEALED
{
	Vector3D m_vP;
	RGBA m_ColorMultiply;
	RGBA m_ColorAdd;
	Vector2D m_vT;
}; // struct WorldFxVertex
SEOUL_STATIC_ASSERT(sizeof(WorldFxVertex) == 28);

static void* GetWorldFxRendererIndexBufferInitialData()
{
	UInt16* pReturn = (UInt16*)MemoryManager::AllocateAligned(
		sizeof(UInt16) * kuIndexBufferSizeInIndices,
		MemoryBudgets::Rendering,
		SEOUL_ALIGN_OF(UInt16));

	// Indices to draw a quad with 2 triangles.
	static const UInt16 kaIndicesForOneInstance[] =
	{
		0u, 1u, 2u,
		0u, 2u, 3u
	};

	// For each instance...
	for (UInt32 j = 0u; j < kuMaxInstances; ++j)
	{
		// Copy the default indices for one quad, offset by the current instance.
		for (UInt32 i = 0u; i < SEOUL_ARRAY_COUNT(kaIndicesForOneInstance); ++i)
		{
			pReturn[SEOUL_ARRAY_COUNT(kaIndicesForOneInstance) * j + i] = 4 * j + kaIndicesForOneInstance[i];
		}
	}

	return pReturn;
}

static VertexElement const* GetWorldFxRendererVertexElements()
{
	static const VertexElement s_kaElements[] =
	{
		// Position (in stream 0)
		{ 0,								// stream
		  0,								// offset
		  VertexElement::TypeFloat3,		// type
		  VertexElement::MethodDefault,		// method
		  VertexElement::UsagePosition,		// usage
		  0u },								// usage index

		// Color0 (in stream 0)
		{ 0,								// stream
		  12,								// offset
		  VertexElement::TypeColor,			// type
		  VertexElement::MethodDefault,		// method
		  VertexElement::UsageColor,		// usage
		  0u },								// usage index

		// Color1 (in stream 0)
		{ 0,								// stream
		  16,								// offset
		  VertexElement::TypeColor,			// type
		  VertexElement::MethodDefault,		// method
		  VertexElement::UsageColor,		// usage
		  1u },								// usage index

		// TexCoords (in stream 0)
		{ 0,								// stream
		  20,								// offset
		  VertexElement::TypeFloat2,		// type
		  VertexElement::MethodDefault,		// method
		  VertexElement::UsageTexcoord,		// usage
		  0u },								// usage index

		VertexElementEnd
	};

	return s_kaElements;
}

FxRenderer::FxRenderer()
	: m_pCamera(nullptr)
	, m_pBuilder(nullptr)
	, m_vWorldStack()
	, m_pActiveEffect(nullptr)
	, m_ActiveEffectTechnique()
	, m_ActiveEffectPass()
	, m_hActiveTexture()
	, m_pIndexBuffer(RenderDevice::Get()->CreateIndexBuffer(GetWorldFxRendererIndexBufferInitialData(), sizeof(UInt16) * kuIndexBufferSizeInIndices, sizeof(UInt16) * kuIndexBufferSizeInIndices, IndexBufferDataFormat::kIndex16))
	, m_pVertexBuffer(RenderDevice::Get()->CreateDynamicVertexBuffer(sizeof(WorldFxVertex) * kuVertexBufferSizeInVertices, sizeof(WorldFxVertex)))
	, m_pVertexFormat(RenderDevice::Get()->CreateVertexFormat(GetWorldFxRendererVertexElements()))
{
}

FxRenderer::~FxRenderer()
{
}

void FxRenderer::BeginFrame(const SharedPtr<Camera>& pCamera, RenderCommandStreamBuilder& rBuilder)
{
	m_pCamera = pCamera;
	m_pBuilder = &rBuilder;
	m_pBuilder->UseVertexFormat(m_pVertexFormat.GetPtr());
	m_pBuilder->SetIndices(m_pIndexBuffer.GetPtr());
	m_pBuilder->SetVertices(0u, m_pVertexBuffer.GetPtr(), 0u, sizeof(WorldFxVertex));
}

void FxRenderer::UseEffect(const SharedPtr<Effect>& pEffect)
{
	if (pEffect != m_pActiveEffect)
	{
		if (m_ActiveEffectTechnique != HString())
		{
			if (m_ActiveEffectPass.IsValid())
			{
				m_pBuilder->EndEffectPass(m_pActiveEffect, m_ActiveEffectPass);
				m_ActiveEffectPass = EffectPass();
			}

			m_pBuilder->EndEffect(m_pActiveEffect);
			m_pActiveEffect.Reset();
			m_ActiveEffectTechnique = HString();
		}

		m_pActiveEffect = pEffect;

		if (m_pActiveEffect.IsValid())
		{
			m_pBuilder->SetMatrix4DParameter(
				m_pActiveEffect,
				kEffectParameterViewProjection,
				m_pCamera->GetViewProjectionMatrix());
			m_pBuilder->SetTextureParameter(
				m_pActiveEffect,
				kEffectParameterTexture,
				m_hActiveTexture);
#if SEOUL_EDITOR_AND_TOOLS // TODO: Do we need this outisde of the editor?
			{
				auto pTexture(m_hActiveTexture.GetPtr());
				if (pTexture.IsValid())
				{
					m_pBuilder->SetVector4DParameter(
						m_pActiveEffect,
						kEffectParameterTextureDimensions,
						Vector4D((Float)pTexture->GetWidth(), (Float)pTexture->GetHeight(), 0.0f, 0.0f));
				}
			}
#endif // /#if SEOUL_EDITOR_AND_TOOLS
			InternalUseEffectTechnique(kEffectTechniqueRender);
		}
	}
}

void FxRenderer::DrawFx(Fx& fx)
{
	fx.Draw(*this);
}

void FxRenderer::EndFrame()
{
	m_hActiveTexture.Reset();

	if (m_ActiveEffectPass.IsValid())
	{
		m_pBuilder->EndEffectPass(m_pActiveEffect, m_ActiveEffectPass);
		m_ActiveEffectPass = EffectPass();
	}

	if (m_ActiveEffectTechnique != HString() &&
		m_pActiveEffect.IsValid())
	{
		m_pBuilder->EndEffect(m_pActiveEffect);
		m_pActiveEffect.Reset();
	}

	m_ActiveEffectTechnique = HString();
	m_pBuilder.Reset();
	m_pCamera.Reset();
}

void FxRenderer::PopWorldMatrix()
{
	m_vWorldStack.PopBack();
}

const Matrix4D& FxRenderer::PushWorldMatrix(const Matrix4D& m)
{
	m_vWorldStack.PushBack(m_vWorldStack.IsEmpty() ? m : m * m_vWorldStack.Back());

	return m_vWorldStack.Back();
}

Camera& FxRenderer::GetCamera() const
{
	return *m_pCamera;
}

IFxRenderer::Buffer& FxRenderer::LockFxBuffer()
{
	return m_vFxBuffer;
}

inline static WorldFxVertex Create(
	const Matrix4D& mTransform,
	const Vector2D& vCornerPosition,
	const Vector2D& vCornerTexcoord,
	const Vector4D& vTexcoordScaleAndShift,
	RGBA cColor,
	RGBA cAdditive)
{
	Vector3D const vPosition = Matrix4D::TransformPosition(mTransform, Vector3D(vCornerPosition, 0.0f));
	Vector2D const vTexcoords = Vector2D::Clamp(
		Vector2D::ComponentwiseMultiply(vCornerTexcoord, vTexcoordScaleAndShift.GetXY()) + vTexcoordScaleAndShift.GetZW(),
		Vector2D::Zero(),
		Vector2D::One());

	WorldFxVertex ret;
	ret.m_ColorAdd = cAdditive;
	ret.m_ColorMultiply = RGBA::Create(cColor.m_R, cColor.m_G, cColor.m_B, cColor.m_A);
	ret.m_vP = vPosition;
	ret.m_vT = vTexcoords;

	return ret;
}

// TODO: Not minimal, can be optimized if this becomes a bottleneck.
inline static void ScreenAlign(const Camera& camera, Matrix4D& rmTransform)
{
	Vector3D const vWorldCenter = rmTransform.GetTranslation();
	Vector3D const vViewDirection = -camera.GetViewAxis();
	Vector3D const vParticleDirection = Matrix4D::TransformDirection(rmTransform.Inverse().Transpose(), Vector3D::UnitZ());
	rmTransform =
		Matrix4D::CreateTranslation(vWorldCenter) *
		Matrix4D::CreateRotationFromDirection(vViewDirection, vParticleDirection) *
		Matrix4D::CreateTranslation(-vWorldCenter) *
		rmTransform;
}

inline static void GetVertices(
	const FxRenderer::WorldStack vStack,
	const Camera& camera,
	const FxParticle& renderableParticle,
	FxRendererMode eMode,
	Bool bScreenAligned,
	WorldFxVertex* pVertices)
{
	Float const fU = 1.0f;
	Float const fV = 1.0f;

	Matrix4D mTransform;
	if (vStack.IsEmpty())
	{
		mTransform = renderableParticle.m_mTransform;
	}
	else
	{
		mTransform = (vStack.Back() * renderableParticle.m_mTransform);
	}

	if (bScreenAligned)
	{
		ScreenAlign(camera, mTransform);
	}

	RGBA cAdditive;
	switch (eMode)
	{
	case FxRendererMode::kAdditive: cAdditive = RGBA::Create(0, 0, 0, 255); break;
	case FxRendererMode::kAlphaClamp:
	case FxRendererMode::kColorAlphaClamp:
		cAdditive = RGBA::Create(renderableParticle.m_uAlphaClampMin, renderableParticle.m_uAlphaClampMax, 0, 128);
		break;
	case FxRendererMode::kNormal: // fall-through
	default:
		cAdditive = RGBA::TransparentBlack();
		break;
	};

	pVertices[0] = Create(mTransform, Vector2D(-0.5f, -0.5f), Vector2D(0 , fV), renderableParticle.m_vTexcoordScaleAndShift, renderableParticle.m_Color, cAdditive);
	pVertices[1] = Create(mTransform, Vector2D( 0.5f, -0.5f), Vector2D(fU, fV), renderableParticle.m_vTexcoordScaleAndShift, renderableParticle.m_Color, cAdditive);
	pVertices[2] = Create(mTransform, Vector2D( 0.5f,  0.5f), Vector2D(fU,  0), renderableParticle.m_vTexcoordScaleAndShift, renderableParticle.m_Color, cAdditive);
	pVertices[3] = Create(mTransform, Vector2D(-0.5f,  0.5f), Vector2D(0 ,  0), renderableParticle.m_vTexcoordScaleAndShift, renderableParticle.m_Color, cAdditive);
}

void FxRenderer::UnlockFxBuffer(
	UInt32 uParticles,
	FilePath textureFilePath,
	FxRendererMode eMode,
	Bool bNeedsScreenAlign)
{
	// Early out if no instances drawn.
	UInt32 const uInstanceCount = Min(uParticles, kuMaxInstances);
	if (uInstanceCount == 0u)
	{
		return;
	}

	if (m_hActiveTexture.GetKey() != textureFilePath)
	{
		Resolve(textureFilePath, m_hActiveTexture);
		m_pBuilder->SetTextureParameter(m_pActiveEffect, kEffectParameterTexture, m_hActiveTexture);
#if SEOUL_EDITOR_AND_TOOLS // TODO: Do we need this outisde of the editor?
		{
			auto pTexture(m_hActiveTexture.GetPtr());
			if (pTexture.IsValid())
			{
				m_pBuilder->SetVector4DParameter(
					m_pActiveEffect,
					kEffectParameterTextureDimensions,
					Vector4D((Float)pTexture->GetWidth(), (Float)pTexture->GetHeight(), 0.0f, 0.0f));
			}
		}
#endif // /#if SEOUL_EDITOR_AND_TOOLS
	}

	WorldFxVertex* pVertices = (WorldFxVertex*)m_pBuilder->LockVertexBuffer(m_pVertexBuffer.GetPtr(), uInstanceCount * sizeof(WorldFxVertex) * 4u);
	if (nullptr == pVertices)
	{
		m_vFxBuffer.Clear();
		return;
	}

	UInt32 uOutOffset = 0u;
	UInt32 const uSize = m_vFxBuffer.GetSize();
	UInt32 const uStart = (uSize - uInstanceCount);
	for (UInt32 i = uStart; i < uSize; ++i)
	{
		GetVertices(
			m_vWorldStack,
			*m_pCamera,
			m_vFxBuffer[i],
			eMode,
			bNeedsScreenAlign,
			pVertices + uOutOffset);
		uOutOffset += 4u;
	}
	m_pBuilder->UnlockVertexBuffer(m_pVertexBuffer.GetPtr());

	m_vFxBuffer.Clear();

	m_pBuilder->CommitEffectPass(m_pActiveEffect, m_ActiveEffectPass);
	m_pBuilder->DrawIndexedPrimitive(
		PrimitiveType::kTriangleList,
		0,
		0, // Start vertex
		uInstanceCount * 4, // Vertex count
		0,
		uInstanceCount * 2); // Primitive count
}

Bool FxRenderer::InternalUseEffectTechnique(HString techniqueName)
{
	if (techniqueName != m_ActiveEffectTechnique)
	{
		if (m_ActiveEffectTechnique != HString())
		{
			if (m_ActiveEffectPass.IsValid())
			{
				m_pBuilder->EndEffectPass(m_pActiveEffect, m_ActiveEffectPass);
				m_ActiveEffectPass = EffectPass();
			}

			m_pBuilder->EndEffect(m_pActiveEffect);
			m_ActiveEffectTechnique = HString();
		}

		if (techniqueName != HString())
		{
			m_ActiveEffectTechnique = techniqueName;
			m_ActiveEffectPass = m_pBuilder->BeginEffect(m_pActiveEffect, m_ActiveEffectTechnique);
			if (!m_ActiveEffectPass.IsValid())
			{
				m_ActiveEffectTechnique = HString();
				return false;
			}

			if (!m_pBuilder->BeginEffectPass(m_pActiveEffect, m_ActiveEffectPass))
			{
				m_pBuilder->EndEffect(m_pActiveEffect);
				m_ActiveEffectTechnique = HString();
				return false;
			}
		}
	}

	return true;
}

void FxRenderer::Resolve(FilePath filePath, TextureContentHandle& rh)
{
	if (!m_tCache.GetValue(filePath, rh))
	{
		// TODO: Make enabling mips conditional?
		// Scene Fx textures are always mipped.
		TextureConfig config = TextureManager::Get()->GetTextureConfig(filePath);
		config.m_bMipped = true;
		TextureManager::Get()->UpdateTextureConfig(filePath, config);

		rh = TextureManager::Get()->GetTexture(filePath);
		SEOUL_VERIFY(m_tCache.Insert(filePath, rh).Second);
	}
}

} // namespace Seoul::Scene

#endif // /#if SEOUL_WITH_SCENE
