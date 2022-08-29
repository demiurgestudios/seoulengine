/**
 * \file SceneMeshRenderer.h
 * \brief Handles rendering of any Mesh instances in a 3D scene.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCENE_MESH_RENDERER_H
#define SCENE_MESH_RENDERER_H

#include "CheckedPtr.h"
#include "EffectPass.h"
#include "SharedPtr.h"
#include "Matrix4D.h"
#include "Prereqs.h"
namespace Seoul { class Camera; }
namespace Seoul { class Effect; }
namespace Seoul { class IndexBuffer; }
namespace Seoul { class Material; }
namespace Seoul { class Mesh; }
namespace Seoul { class RenderCommandStreamBuilder; }
namespace Seoul { class VertexBuffer; }
namespace Seoul { class VertexFormat; }
namespace Seoul { namespace Animation3D { class NetworkInstance; } }

#if SEOUL_WITH_SCENE

namespace Seoul::Scene
{

class MeshRenderer SEOUL_SEALED
{
public:
	typedef Vector<Matrix4D, MemoryBudgets::Scene> WorldStack;

	MeshRenderer();
	~MeshRenderer();

	void BeginFrame(
		const SharedPtr<Camera>& pCamera,
		RenderCommandStreamBuilder& rBuilder);
	Bool DrawAnimatedMesh(
		UInt32 uMeshDrawFlags,
		const Matrix4D& mWorldTransform,
		const SharedPtr<Mesh>& pMesh,
		const Animation3D::NetworkInstance& animationNetworkInstance,
		HString techniqueOverride = HString());
	Bool DrawMesh(
		UInt32 uMeshDrawFlags,
		const Matrix4D& mWorldTransform,
		const SharedPtr<Mesh>& pMesh,
		HString techniqueOverride = HString());
	void EndFrame();
	const SharedPtr<Effect>& GetActiveEffect() const { return m_pActiveEffect; }
	void PopWorldMatrix();
	const Matrix4D& PushWorldMatrix(const Matrix4D& m);
	void UseEffect(const SharedPtr<Effect>& pEffect);

private:
	CheckedPtr<RenderCommandStreamBuilder> m_pBuilder;
	SharedPtr<Camera> m_pCamera;
	WorldStack m_vWorldStack;
	SharedPtr<Effect> m_pActiveEffect;
	HString m_ActiveEffectTechnique;
	EffectPass m_ActiveEffectPass;
	SharedPtr<IndexBuffer> m_pActiveIndexBuffer;
	SharedPtr<Material> m_pActiveMaterial;
	SharedPtr<VertexBuffer> m_pActiveVertexBuffer;
	SharedPtr<VertexFormat> m_pActiveVertexFormat;
	UInt32 m_uActiveMeshDrawFlags;
	Bool m_bWorldStackDirty;

	void InternalCommitViewProjectionTransform();
	Bool InternalUseEffectTechnique(HString techniqueName);

	SEOUL_DISABLE_COPY(MeshRenderer);
}; // class MeshRenderer

} // namespace Seoul::Scene

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
