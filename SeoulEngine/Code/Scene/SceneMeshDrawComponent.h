/**
 * \file SceneMeshDrawComponent.h
 * \brief Binds a Mesh (a collection of drawable triangle
 * primitives) into a Scene object.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCENE_MESH_DRAW_COMPONENT_H
#define SCENE_MESH_DRAW_COMPONENT_H

#include "Asset.h"
#include "SceneComponent.h"
#include "SceneMeshDrawFlags.h"
namespace Seoul { class DataNode; }
namespace Seoul { class DataStore; }
namespace Seoul { struct Frustum; }
namespace Seoul { namespace Scene { class MeshRenderer; } }
namespace Seoul { namespace Reflection { struct SerializeContext; } }

#if SEOUL_WITH_SCENE

namespace Seoul::Scene
{

class MeshDrawComponent SEOUL_SEALED : public Component
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(MeshDrawComponent);

	MeshDrawComponent();
	~MeshDrawComponent();

	virtual SharedPtr<Component> Clone(const String& sQualifier) const SEOUL_OVERRIDE;

	UInt32 GetMeshDrawFlags() const
	{
		return m_uMeshDrawFlags;
	}

	const AssetContentHandle& GetMesh() const
	{
		return m_hMesh;
	}

	FilePath GetMeshFilePath() const
	{
		return m_hMesh.GetKey();
	}

	const Vector3D& GetScale() const
	{
		return m_vScale;
	}

	Bool Render(
		const Frustum& frustum,
		MeshRenderer& rRenderer,
		HString techniqueOverride,
		HString skinnedTechniqueOverride);

	void SetMeshFilePath(FilePath filIePath);

	void SetScale(const Vector3D& vScale)
	{
		m_vScale = vScale;
	}

	/**
	 * Set the MeshDrawComponent to visible/not-visible.
	 *
	 * For runtime control of rendering.
	 */
	void SetVisible(Bool bVisible)
	{
		m_bVisible = bVisible;
	}

	/**
	 * Mesh depth will be projected to "infinity" (to the far plane),
	 * effectively disabling depth testing and depth clipping.
	 */
	Bool GetInfiniteDepth() const { return (MeshDrawFlags::kInfiniteDepth == (MeshDrawFlags::kInfiniteDepth & m_uMeshDrawFlags)); }
	void SetInfiniteDepth(Bool b) { if (b) { m_uMeshDrawFlags |= MeshDrawFlags::kInfiniteDepth; } else { m_uMeshDrawFlags &= ~MeshDrawFlags::kInfiniteDepth; } }

	/**
	 * Mesh will be rendered at the origin (ignore its world transform)
	 * and projected to infinity.
	 */
	Bool GetSky() const { return (MeshDrawFlags::kSky == (MeshDrawFlags::kSky & m_uMeshDrawFlags)); }
	void SetSky(Bool b) { if (b) { m_uMeshDrawFlags |= MeshDrawFlags::kSky; } else { m_uMeshDrawFlags &= ~MeshDrawFlags::kSky; } }

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(MeshDrawComponent);
	SEOUL_REFLECTION_FRIENDSHIP(MeshDrawComponent);

#if SEOUL_EDITOR_AND_TOOLS
	Vector3D EditorGetMeshExtents() const;
#endif // /#if SEOUL_EDITOR_AND_TOOLS

	AssetContentHandle m_hMesh;
	Vector3D m_vScale;
	UInt32 m_uMeshDrawFlags;
	Bool m_bVisible;

	SEOUL_DISABLE_COPY(MeshDrawComponent);
}; // class MeshDrawComponent

} // namespace Seoul::Scene

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
