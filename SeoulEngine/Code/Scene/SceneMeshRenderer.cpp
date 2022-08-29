/**
* \file SceneMeshRenderer.cpp
* \brief Handles rendering of any Mesh instances in a 3D scene.
*
* Copyright (c) Demiurge Studios, Inc.
* 
* This source code is licensed under the MIT license.
* Full license details can be found in the LICENSE file
* in the root directory of this source tree.
*/

#include "Animation3DNetworkInstance.h"
#include "Camera.h"
#include "Effect.h"
#include "MaterialLibrary.h"
#include "Matrix4D.h"
#include "Mesh.h"
#include "RenderCommandStreamBuilder.h"
#include "SceneMeshDrawFlags.h"
#include "SceneMeshRenderer.h"
#include "PrimitiveGroup.h"
#include "ScenePrereqs.h"

#if SEOUL_WITH_SCENE

namespace Seoul::Scene
{

/** EffectParameter used for committing a mesh's skinning palette, when applicable and defined. */
static const HString kEffectParameterSkinningPalette("seoul_SkinningPalette");

/** EffectParameter used for setting a Mesh's world transform. */
static const HString kEffectParameterWorldTransform("seoul_WorldTransform");

/** EffectParameter used for setting a Mesh's world transform (excluding scale. */
static const HString kEffectParameterWorldNormalTransform("seoul_WorldNormalTransform");

MeshRenderer::MeshRenderer()
	: m_pBuilder()
	, m_pCamera()
	, m_vWorldStack()
	, m_pActiveEffect(nullptr)
	, m_ActiveEffectTechnique()
	, m_ActiveEffectPass()
	, m_pActiveIndexBuffer(nullptr)
	, m_pActiveMaterial(nullptr)
	, m_pActiveVertexBuffer(nullptr)
	, m_pActiveVertexFormat(nullptr)
	, m_uActiveMeshDrawFlags(0u)
	, m_bWorldStackDirty(false)
{
}

MeshRenderer::~MeshRenderer()
{
}

void MeshRenderer::BeginFrame(
	const SharedPtr<Camera>& pCamera,
	RenderCommandStreamBuilder& rBuilder)
{
	m_pCamera = pCamera;
	m_pBuilder = &rBuilder;
}

Bool MeshRenderer::DrawAnimatedMesh(
	UInt32 uMeshDrawFlags,
	const Matrix4D& mWorldTransform,
	const SharedPtr<Mesh>& pMesh,
	const Animation3D::NetworkInstance& animationNetworkInstance,
	HString techniqueOverride /*= HString()*/)
{
	Bool bReturn = false;

	// TODO: Cache this data.
	auto const mWorldNormalTransform = Matrix4D::CreateNormalTransform(mWorldTransform);
	Bool const bMirrored = (mWorldTransform.Determinant() < 0.0f);

	if (uMeshDrawFlags != m_uActiveMeshDrawFlags)
	{
		// Update flags.
		UInt32 const uOldFlags = m_uActiveMeshDrawFlags;
		m_uActiveMeshDrawFlags = uMeshDrawFlags;

		// If we're changing Sky mode or InfiniteDepth mode, update View * Projection transform.
		if (((MeshDrawFlags::kSky | MeshDrawFlags::kInfiniteDepth) & m_uActiveMeshDrawFlags) !=
			((MeshDrawFlags::kSky | MeshDrawFlags::kInfiniteDepth) & uOldFlags))
		{
			// Set the view projection transform.
			InternalCommitViewProjectionTransform();
		}
	}

	// If still necessary, push the view projection transform due to a dirty
	// world stack. We factor these additional world transform into the view * projection
	// transform.
	if (m_bWorldStackDirty)
	{
		InternalCommitViewProjectionTransform();
	}

	SharedPtr<VertexFormat> pVertexFormat(pMesh->GetVertexFormat());
	if (pVertexFormat != m_pActiveVertexFormat)
	{
		m_pBuilder->UseVertexFormat(pVertexFormat.GetPtr());
		m_pActiveVertexFormat = pVertexFormat;
	}

	SharedPtr<VertexBuffer> pVertexBuffer(pMesh->GetVertexBuffer());
	if (pVertexBuffer != m_pActiveVertexBuffer)
	{
		m_pBuilder->SetVertices(
			0u,
			pVertexBuffer.GetPtr(),
			0u,
			pVertexBuffer->GetVertexStrideInBytes());
		m_pActiveVertexBuffer = pVertexBuffer;
	}

	const SharedPtr<MaterialLibrary> pMaterialLibrary = pMesh->GetMaterialLibrary();
	if (pMaterialLibrary.IsValid())
	{
		MaterialLibrary::Materials const& vMaterials = pMaterialLibrary->GetMaterials();
		UInt32 const uPrimitiveGroups = pMesh->GetPrimitiveGroupCount();

		if (uPrimitiveGroups != 0u)
		{
			// Commit the skinning palette.
			animationNetworkInstance.CommitSkinningPalette(
				*m_pBuilder,
				m_pActiveEffect,
				kEffectParameterSkinningPalette);
		}

		for (UInt32 i = 0u; i < uPrimitiveGroups; ++i)
		{
			PrimitiveGroup const* pGroup = pMesh->GetPrimitiveGroup(i);
			Int32 const iMaterialId = pGroup->GetMaterialId();
			if (iMaterialId >= 0 && iMaterialId < (Int32)vMaterials.GetSize())
			{
				SharedPtr<Material> pMaterial(vMaterials[iMaterialId].GetPtr());
				if (pMaterial.IsValid())
				{
					if (pMaterial != m_pActiveMaterial)
					{
						if (m_pActiveMaterial.IsValid())
						{
							m_pActiveMaterial->Uncommit(*m_pBuilder, m_pActiveEffect);
						}

						HString technique = techniqueOverride;
						if (technique.IsEmpty())
						{
							technique = pMaterial->GetTechnique();
						}

						if (!InternalUseEffectTechnique(technique))
						{
							m_pActiveMaterial.Reset();
							continue;
						}

						pMaterial->Commit(*m_pBuilder, m_pActiveEffect);
						m_pActiveMaterial = pMaterial;
					}

					m_pBuilder->SetIndices(bMirrored
						? pGroup->GetMirroredIndexBuffer()
						: pGroup->GetIndexBuffer());

					// TODO: Need to sort InfiniteDepth meshes, so they draw, either,
					// back to front, or with ever increasing depth biases.

					// TODO: If sorting, a mesh with kSky should be draw last.
					if (MeshDrawFlags::kSky == (MeshDrawFlags::kSky & m_uActiveMeshDrawFlags))
					{
						// Sky meshes ignore their translation.
						Matrix4D m(mWorldTransform);
						m.SetTranslation(Vector3D::Zero());
						Matrix4D mNormal(mWorldNormalTransform);
						mNormal.SetTranslation(Vector3D::Zero());

						// Sky meshes ignore world transform.
						m_pBuilder->SetMatrix4DParameter(
							m_pActiveEffect,
							kEffectParameterWorldTransform,
							m);
						m_pBuilder->SetMatrix4DParameter(
							m_pActiveEffect,
							kEffectParameterWorldNormalTransform,
							mNormal);
					}
					else
					{
						// Otherwise, set the provided world transform.
						m_pBuilder->SetMatrix4DParameter(
							m_pActiveEffect,
							kEffectParameterWorldTransform,
							mWorldTransform);
						m_pBuilder->SetMatrix4DParameter(
							m_pActiveEffect,
							kEffectParameterWorldNormalTransform,
							mWorldNormalTransform);
					}

					m_pBuilder->CommitEffectPass(m_pActiveEffect, m_ActiveEffectPass);
					m_pBuilder->DrawIndexedPrimitive(
						pGroup->GetPrimitiveType(),
						pGroup->GetStartVertex(),
						0,
						pGroup->GetNumVertices(),
						0,
						pGroup->GetNumPrimitives());

					bReturn = true;
				}
			}
		}
	}

	return bReturn;
}

Bool MeshRenderer::DrawMesh(
	UInt32 uMeshDrawFlags,
	const Matrix4D& mWorldTransform,
	const SharedPtr<Mesh>& pMesh,
	HString techniqueOverride /*= HString()*/)
{
	Bool bReturn = false;

	// TODO: Cache this data.
	auto const mWorldNormalTransform = Matrix4D::CreateNormalTransform(mWorldTransform);
	Bool const bMirrored = (mWorldTransform.Determinant() < 0.0f);

	if (uMeshDrawFlags != m_uActiveMeshDrawFlags)
	{
		// Update flags.
		UInt32 const uOldFlags = m_uActiveMeshDrawFlags;
		m_uActiveMeshDrawFlags = uMeshDrawFlags;

		// If we're changing Sky mode or InfiniteDepth mode, update View * Projection transform.
		if (((MeshDrawFlags::kSky | MeshDrawFlags::kInfiniteDepth) & m_uActiveMeshDrawFlags) !=
			((MeshDrawFlags::kSky | MeshDrawFlags::kInfiniteDepth) & uOldFlags))
		{
			// Set the view projection transform.
			InternalCommitViewProjectionTransform();
		}
	}

	// If still necessary, push the view projection transform due to a dirty
	// view stack.
	if (m_bWorldStackDirty)
	{
		InternalCommitViewProjectionTransform();
	}

	SharedPtr<VertexFormat> pVertexFormat(pMesh->GetVertexFormat());
	if (pVertexFormat != m_pActiveVertexFormat)
	{
		m_pBuilder->UseVertexFormat(pVertexFormat.GetPtr());
		m_pActiveVertexFormat = pVertexFormat;
	}

	SharedPtr<VertexBuffer> pVertexBuffer(pMesh->GetVertexBuffer());
	if (pVertexBuffer != m_pActiveVertexBuffer)
	{
		m_pBuilder->SetVertices(
			0u,
			pVertexBuffer.GetPtr(),
			0u,
			pVertexBuffer->GetVertexStrideInBytes());
		m_pActiveVertexBuffer = pVertexBuffer;
	}

	const SharedPtr<MaterialLibrary> pMaterialLibrary = pMesh->GetMaterialLibrary();
	if (pMaterialLibrary.IsValid())
	{
		MaterialLibrary::Materials const& vMaterials = pMaterialLibrary->GetMaterials();
		UInt32 const uPrimitiveGroups = pMesh->GetPrimitiveGroupCount();
		for (UInt32 i = 0u; i < uPrimitiveGroups; ++i)
		{
			PrimitiveGroup const* pGroup = pMesh->GetPrimitiveGroup(i);
			Int32 const iMaterialId = pGroup->GetMaterialId();
			if (iMaterialId >= 0 && iMaterialId < (Int32)vMaterials.GetSize())
			{
				SharedPtr<Material> pMaterial(vMaterials[iMaterialId].GetPtr());
				if (pMaterial.IsValid())
				{
					if (pMaterial != m_pActiveMaterial)
					{
						if (m_pActiveMaterial.IsValid())
						{
							m_pActiveMaterial->Uncommit(*m_pBuilder, m_pActiveEffect);
						}

						HString technique = techniqueOverride;
						if (technique.IsEmpty())
						{
							technique = pMaterial->GetTechnique();
						}

						if (!InternalUseEffectTechnique(technique))
						{
							m_pActiveMaterial.Reset();
							continue;
						}

						pMaterial->Commit(*m_pBuilder, m_pActiveEffect);
						m_pActiveMaterial = pMaterial;
					}

					m_pBuilder->SetIndices(bMirrored
						? pGroup->GetMirroredIndexBuffer()
						: pGroup->GetIndexBuffer());

					// TODO: Need to sort InfiniteDepth meshes, so they draw, either,
					// back to front, or with ever increasing depth biases.

					// TODO: If sorting, a mesh with kSky should be draw last.
					if (MeshDrawFlags::kSky == (MeshDrawFlags::kSky & m_uActiveMeshDrawFlags))
					{
						// Sky meshes ignore their translation.
						Matrix4D m(mWorldTransform);
						m.SetTranslation(Vector3D::Zero());
						Matrix4D mNormal(mWorldNormalTransform);
						mNormal.SetTranslation(Vector3D::Zero());

						// Sky meshes ignore world transform.
						m_pBuilder->SetMatrix4DParameter(
							m_pActiveEffect,
							kEffectParameterWorldTransform,
							m);
						m_pBuilder->SetMatrix4DParameter(
							m_pActiveEffect,
							kEffectParameterWorldNormalTransform,
							mNormal);
					}
					else
					{
						// Otherwise, set the provided world transform.
						m_pBuilder->SetMatrix4DParameter(
							m_pActiveEffect,
							kEffectParameterWorldTransform,
							mWorldTransform);
						m_pBuilder->SetMatrix4DParameter(
							m_pActiveEffect,
							kEffectParameterWorldNormalTransform,
							mWorldNormalTransform);
					}

					m_pBuilder->CommitEffectPass(m_pActiveEffect, m_ActiveEffectPass);
					m_pBuilder->DrawIndexedPrimitive(
						pGroup->GetPrimitiveType(),
						pGroup->GetStartVertex(),
						0,
						pGroup->GetNumVertices(),
						0,
						pGroup->GetNumPrimitives());

					bReturn = true;
				}
			}
		}
	}

	return bReturn;
}

void MeshRenderer::EndFrame()
{
	if (m_pActiveMaterial.IsValid())
	{
		m_pActiveMaterial->Uncommit(*m_pBuilder, m_pActiveEffect);
		m_pActiveMaterial.Reset();
	}

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
	m_pActiveIndexBuffer.Reset();
	m_pActiveVertexBuffer.Reset();
	m_pActiveVertexFormat.Reset();
	m_uActiveMeshDrawFlags = 0u;
	m_pBuilder.Reset();
	m_pCamera.Reset();
}

void MeshRenderer::PopWorldMatrix()
{
	m_vWorldStack.PopBack();
	m_bWorldStackDirty = true;
}

const Matrix4D& MeshRenderer::PushWorldMatrix(const Matrix4D& m)
{
	m_vWorldStack.PushBack(m_vWorldStack.IsEmpty() ? m : m * m_vWorldStack.Back());
	m_bWorldStackDirty = true;

	return m_vWorldStack.Back();
}

void MeshRenderer::UseEffect(const SharedPtr<Effect>& pEffect)
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
			// Commit the View * Projection transform now.
			InternalCommitViewProjectionTransform();
		}
	}
}

void MeshRenderer::InternalCommitViewProjectionTransform()
{
	// Full View * Projection transform.
	Matrix4D mViewProjection;

	// Sky or InfiniteDepth transform.
	if (0u != (m_uActiveMeshDrawFlags & (MeshDrawFlags::kSky | MeshDrawFlags::kInfiniteDepth)))
	{
		Matrix4D mView(m_pCamera->GetViewMatrix());

		Double fEpsilon;

		if (MeshDrawFlags::kSky == (MeshDrawFlags::kSky & m_uActiveMeshDrawFlags))
		{
			// For sky, view transform without translation.
			mView.SetTranslation(Vector3D::Zero());

			// Default epsilon.
			fEpsilon = kfInfiniteProjectionEpsilon;
		}
		else
		{
			// View is unchanged for infinite depth.

			// Double the epsilon, place infinite depth in front of sky.
			fEpsilon = (kfInfiniteProjectionEpsilon + kfInfiniteProjectionEpsilon);
		}

		// Infinite projection transform.
		Matrix4D mProjection(m_pCamera->GetProjectionMatrix());
		mProjection = mProjection.InfiniteProjection(fEpsilon);

		// Setup.
		mViewProjection = (mProjection * mView);
	}
	// Normal transform.
	else
	{
		// Setup.
		mViewProjection = m_pCamera->GetViewProjectionMatrix();
	}

	// Append world stack if not empty.
	if (!m_vWorldStack.IsEmpty())
	{
		mViewProjection *= m_vWorldStack.Back();
	}

	// Commit.
	m_pBuilder->SetMatrix4DParameter(
		m_pActiveEffect,
		kEffectParameterViewProjection,
		mViewProjection);

	// No longer dirty.
	m_bWorldStackDirty = false;
}

Bool MeshRenderer::InternalUseEffectTechnique(HString techniqueName)
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

} // namespace Seoul::Scene

#endif // /#if SEOUL_WITH_SCENE
