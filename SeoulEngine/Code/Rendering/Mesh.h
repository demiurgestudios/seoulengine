/**
 * \file Mesh.h
 * \brief A mesh contains vertex data and a collection of primitive groups
 * that describe renderable geometry, potentially with multiple materials.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef MESH_H
#define MESH_H

#include "AABB.h"
#include "Asset.h"
#include "ContentHandle.h"
#include "ContentKey.h"
#include "FilePath.h"
#include "SharedPtr.h"
#include "MaterialLibrary.h"
#include "Matrix4D.h"
#include "SeoulString.h"
#include "Vector.h"
namespace Seoul { class PrimitiveGroup; }
namespace Seoul { class VertexBuffer; }
namespace Seoul { struct VertexElement; }
namespace Seoul { class VertexFormat; }

namespace Seoul
{

class Mesh SEOUL_SEALED
{
public:
	typedef Vector<Matrix3x4, MemoryBudgets::Rendering> InverseBindPoses;
	typedef Vector<PrimitiveGroup*, MemoryBudgets::Rendering> PrimitiveGroups;
	typedef Vector<Vector3D, MemoryBudgets::Rendering> Vertices;

	Mesh();
	~Mesh();

	Bool Load(FilePath filePath, SyncFile& file);

	/**
	* Returns the AABB of this Mesh in the local coordinate system
	* of this Mesh.
	*
	* To get the AABB of the Mesh in the original world space of
	* this Mesh, this AABB must be transformed with
	* GetMeshToOriginalWorldTransform().
	*/
	const AABB& GetBoundingBox() const
	{
		return m_AABB;
	}

	/**
	 * For animated meshes, defines the inverse bind poses matrix to use
	 * when applying skinning.
	 */
	const InverseBindPoses& GetInverseBindPoses() const
	{
		return m_vInverseBindPoses;
	}

	/**
	* Sets the bounding box of this mesh.
	*/
	void SetBoundingBox(const AABB& newAABB)
	{
		m_AABB = newAABB;
	}

	/**
	 * The number of primitive groups that make up this Mesh.
	 *
	 * Each primitive group can have its own Material and has
	 * its own index buffer. Primitive groups share a vertex buffer.
	 */
	UInt32 GetPrimitiveGroupCount() const
	{
		return m_vPrimitiveGroups.GetSize();
	}

	/**
	 * Gets the ith primitive group of this Mesh.
	 *
	 * \pre i must be a valid primitive group, less than the PrimitiveCount
	 * of this Mesh.
	 */
	PrimitiveGroup const* GetPrimitiveGroup(UInt32 i) const
	{
		SEOUL_ASSERT(i < GetPrimitiveGroupCount());
		return m_vPrimitiveGroups[i];
	}

	/**
	 * Gets the VertexFormat of this Mesh.
	 */
	const SharedPtr<VertexFormat>& GetVertexFormat() const
	{
		return m_pVertexFormat;
	}

	/**
	 * Gets the vertex buffer of this Mesh.
	 *
	 * Each primitive group defines an index buffer that is used
	 * to draw primitives defined by the vertices contained within this
	 * Mesh's vertex buffer.
	 */
	const SharedPtr<VertexBuffer>& GetVertexBuffer() const
	{
		return m_pVertexBuffer;
	}

	/** @return The default material set associated with this Mesh. */
	const SharedPtr<MaterialLibrary>& GetMaterialLibrary() const
	{
		return m_pMaterialLibrary;
	}

	/** Update the material library associated with this Mesh. */
	void SetMaterialLibrary(const SharedPtr<MaterialLibrary>& pMaterialLibrary)
	{
		m_pMaterialLibrary = pMaterialLibrary;
	}

	/**
	 * Returns the amount of memory occupied by this
	 * mesh, assuming that its primitive groups (and their index buffers)
	 * and its vertex buffer are not shared.
	 */
	UInt32 GetMemoryUsageInBytes() const
	{
		return m_zGraphicsMemoryUsageInBytes;
	}

	// Return the number of primitives across all the primitive groups in
	// this Mesh.
	UInt32 GetPrimitiveCount() const;

#if SEOUL_EDITOR_AND_TOOLS
	// In the Editor/Tools only, Mesh vertices are save for CPU access, to be used
	// for computing physics collision, etc.
	const Vertices& EditorGetVerticies() const { return m_vEditorVertices; }
#endif // /#if SEOUL_EDITOR_AND_TOOLS

private:
	void InternalDestroy();

	SEOUL_REFERENCE_COUNTED(Mesh);

	SharedPtr<MaterialLibrary> m_pMaterialLibrary;
	PrimitiveGroups m_vPrimitiveGroups;
	AABB m_AABB;
	SharedPtr<VertexBuffer> m_pVertexBuffer;
	SharedPtr<VertexFormat> m_pVertexFormat;
	UInt32 m_zGraphicsMemoryUsageInBytes;
	InverseBindPoses m_vInverseBindPoses;

#if SEOUL_EDITOR_AND_TOOLS
	Vertices m_vEditorVertices;
#endif // /#if SEOUL_EDITOR_AND_TOOLS

	Bool InternalLoadVertexFormat(SyncFile& file, Vector<VertexElement>& rvVertexElements);
	Bool InternalLoadVertexBuffer(SyncFile& file, const Vector<VertexElement>& vVertexElements);
	Bool InternalLoadPrimitiveGroups(SyncFile& file);

	SEOUL_DISABLE_COPY(Mesh);
}; // class Mesh
static inline SharedPtr<Mesh> GetMeshPtr(const AssetContentHandle& h)
{
	SharedPtr<Asset> p(h.GetPtr());
	if (p.IsValid())
	{
		return p->GetMesh();
	}
	else
	{
		return SharedPtr<Mesh>();
	}
}

} // namespace Seoul

#endif // include guard
