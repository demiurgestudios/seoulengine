/**
 * \file ScenePrimitiveRenderer.h
 * \brief Handles rendering of simple primitives for debugging
 * purposes (lines, spheres, etc.).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCENE_PRIMITIVE_RENDERER_H
#define SCENE_PRIMITIVE_RENDERER_H

#include "CheckedPtr.h"
#include "EffectPass.h"
#include "Fx.h"
#include "HashFunctions.h"
#include "HashTable.h"
#include "SharedPtr.h"
#include "Prereqs.h"
#include "Texture.h"
namespace Seoul { class Camera; }
namespace Seoul { class Effect; }
namespace Seoul { class IndexBuffer; }
namespace Seoul { class Material; }
namespace Seoul { struct Matrix4D; }
namespace Seoul { class RenderCommandStreamBuilder; }
namespace Seoul { class VertexBuffer; }
namespace Seoul { class VertexFormat; }

#if SEOUL_WITH_SCENE

namespace Seoul
{

namespace Scene
{

struct PrimitiveVertex SEOUL_SEALED
{
	Vector3D m_vP;
	Float m_fClipValue;
	RGBA m_cColor;

	static PrimitiveVertex Zero()
	{
		PrimitiveVertex ret;
		ret.m_vP = Vector3D::Zero();
		ret.m_fClipValue = 0.0f;
		ret.m_cColor = RGBA::Create(0, 0, 0, 0);
		return ret;
	}
}; // struct PrimitiveVertex

} // namespace Scene

template <> struct CanMemCpy<Scene::PrimitiveVertex> { static const Bool Value = true; };
template <> struct CanZeroInit<Scene::PrimitiveVertex> { static const Bool Value = true; };
SEOUL_STATIC_ASSERT(sizeof(Scene::PrimitiveVertex) == 20);

namespace Scene
{

inline Bool operator==(const PrimitiveVertex& a, const PrimitiveVertex& b)
{
	return (
		a.m_cColor == b.m_cColor &&
		a.m_fClipValue == b.m_fClipValue &&
		a.m_vP == b.m_vP);
}

inline Bool operator!=(const PrimitiveVertex& a, const PrimitiveVertex& b)
{
	return !(a == b);
}

inline UInt32 GetHash(const PrimitiveVertex& v)
{
	UInt32 uHash = 0u;
	IncrementalHash(uHash, Seoul::GetHash(v.m_cColor.m_Value));
	IncrementalHash(uHash, Seoul::GetHash(v.m_fClipValue));
	IncrementalHash(uHash, Seoul::GetHash(v.m_vP));
	return uHash;
}

} // namespace Scene

template <>
struct DefaultHashTableKeyTraits<Scene::PrimitiveVertex>
{
	inline static Float GetLoadFactor()
	{
		return 0.75f;
	}

	inline static Scene::PrimitiveVertex GetNullKey()
	{
		return Scene::PrimitiveVertex::Zero();
	}

	static const Bool kCheckHashBeforeEquals = false;
};

namespace Scene
{

struct PrimitiveVertexWithNormal SEOUL_SEALED
{
	Vector3D m_vP;
	Float m_fClipValue;
	Vector3D m_vN;
	RGBA m_cColor;

	static PrimitiveVertexWithNormal Zero()
	{
		PrimitiveVertexWithNormal ret;
		ret.m_vP = Vector3D::Zero();
		ret.m_fClipValue = 0.0f;
		ret.m_vN = Vector3D::Zero();
		ret.m_cColor = RGBA::Create(0, 0, 0, 0);
		return ret;
	}

	static PrimitiveVertexWithNormal Create(const PrimitiveVertex& v)
	{
		PrimitiveVertexWithNormal ret;
		ret.m_vP = v.m_vP;
		ret.m_fClipValue = v.m_fClipValue;
		ret.m_vN = Vector3D::Zero();
		ret.m_cColor = v.m_cColor;
		return ret;
	}
}; // struct PrimitiveVertexWithNormal

} // namespace Scene

template <> struct CanMemCpy<Scene::PrimitiveVertexWithNormal> { static const Bool Value = true; };
template <> struct CanZeroInit<Scene::PrimitiveVertexWithNormal> { static const Bool Value = true; };
SEOUL_STATIC_ASSERT(sizeof(Scene::PrimitiveVertexWithNormal) == 32);

namespace Scene
{

/** How much we offset a projection matrix to minimize z-fighting. */
static const Double kfPrimitiveRendererDepthBias = (kfBiasProjectionEpsilon);

class PrimitiveRenderer SEOUL_SEALED
{
public:
	static const UInt32 kuMaxVertices = 4096u;
	static const UInt32 kuMaxIndices = kuMaxVertices * 4u;

	PrimitiveRenderer();
	~PrimitiveRenderer();

	void BeginFrame(
		const SharedPtr<Camera>& pCamera,
		RenderCommandStreamBuilder& rBuilder);
	void UseEffect(const SharedPtr<Effect>& pEffect);
	// Use a specialized effect technique for the upcmoing batch.
	// Not normally necessary to call this, the default is "seoul_Render".
	// Call with no argument to reset to the default.
	void UseEffectTechnique(HString techniqueName = HString());
	void EndFrame();

	void Line(
		const Vector3D& v0,
		const Vector3D& v1,
		ColorARGBu8 cColor)
	{
		InternalStartIndices(2u, true);

		// setup the vVertices
		InternalAddVertex(v0, cColor);
		InternalAddVertex(v1, cColor);

		// setup indices
		InternalAddLine(0, 1);
	}

	void LineBox(
		const Matrix4D& mWorld,
		const Vector3D& vExtents,
		ColorARGBu8 cColor);

	void LineCircle(
		const Vector3D& vCenter,
		const Vector3D& vAxis,
		Float32 fRadius,
		Int32 iSegmentsPerRing,
		Bool bRadiusToMidpoint,
		ColorARGBu8 cColor);

	void LineGrid(
		const Matrix4D& mWorld,
		Int32 iCellsX,
		Int32 iCellsZ,
		Bool bIncludeBorder,
		ColorARGBu8 cColor);

	void LinePyramid(
		const Vector3D& p0,
		const Vector3D& p1,
		const Vector3D& p3,
		ColorARGBu8 cColor);

	/** Get the current view space clip value. */
	Float32 GetClipValue() const
	{
		return m_fClipValue;
	}

	/** Reset (disable) the clip value. */
	void ResetClipValue();

	/**
	 * Set the current view space clip value. Will be applied
	 * to all primitives added until it is changed again.
	 */
	void SetClipValue(Float32 fClipValue)
	{
		m_fClipValue = fClipValue;
	}

	// Enable or disable normal generaiton for the current batch.
	// Ignored for lines. Causes an immediate flush on a mode change.
	void SetGenerateNormals(Bool bGenerateNormals);

	void Triangle(
		const Vector3D& p0,
		const Vector3D& p1,
		const Vector3D& p2,
		ColorARGBu8 cColor)
	{
		InternalStartIndices(3, false);

		InternalAddVertex(p0, cColor);
		InternalAddVertex(p1, cColor);
		InternalAddVertex(p2, cColor);

		InternalAddTriangle(0, 1, 2);
	}

	void TriangleBox(
		const Matrix4D& mWorld,
		const Vector3D& vExtents,
		ColorARGBu8 cColor);

	void TriangleCapsule(
		const Vector3D& p0,
		const Vector3D& p1,
		Float32 fRadius,
		Int32 iSegmentsPerRing,
		Bool bRadiusToMidpoint,
		ColorARGBu8 cColor);

	void TriangleCone(
		const Vector3D& p0,
		const Vector3D& p1,
		Float32 fRadius,
		Int32 iSegmentsPerRing,
		Bool bRadiusToMidpoint,
		ColorARGBu8 cColor);

	/** Adapted from: http://apparat-engine.blogspot.com/2013/04/procdural-meshes-cylinder.html */
	void TriangleCylinder(
		const Vector3D& p0,
		const Vector3D& p1,
		Float32 fRadius,
		Int32 iSegmentsPerRing,
		Bool bRadiusToMidpoint,
		ColorARGBu8 cColor);

	/**
	 * Creates indices and vertices for a solid filled quad.
	 */
	void TriangleQuad(
		const Vector3D& p0,
		const Vector3D& p1,
		const Vector3D& p2,
		const Vector3D& p3,
		ColorARGBu8 cColor)
	{
		Triangle(p0, p1, p2, cColor);
		Triangle(p2, p1, p3, cColor);
	}

	void TriangleSphere(
		const Vector3D& vCenter,
		Float32 fRadius,
		Int32 iSegmentsPerRing,
		Bool bRadiusToMidpoint,
		ColorARGBu8 cColor);

	void TriangleTorus(
		const Vector3D& vCenter,
		const Vector3D& vAxis,
		Float32 fInnerRadius,
		Float32 fOuterRadius,
		Int32 iSegmentsPerRing,
		Int32 iTotalRings,
		Bool bRadiusToMidpoint,
		ColorARGBu8 cColor);

	void UseDepthBias(Double fDepthBias = kfPrimitiveRendererDepthBias);

	void UseInfiniteProjection(Double fBias = 0.0);

private:
	typedef Vector<UInt16, MemoryBudgets::Rendering> Indices;
	typedef Vector<PrimitiveVertex, MemoryBudgets::Rendering> Vertices;
	typedef Vector<PrimitiveVertexWithNormal, MemoryBudgets::Rendering> VerticesWithNormals;
	typedef HashTable<PrimitiveVertex, UInt16, MemoryBudgets::Rendering> VertexTable;

	Double m_fDepthBias;
	Double m_fInfiniteBias;
	SharedPtr<Camera> m_pCamera;
	CheckedPtr<RenderCommandStreamBuilder> m_pBuilder;
	Indices m_vIndices;
	Indices m_vPendingVertices;
	Vertices m_vVertices;
	VerticesWithNormals m_vVerticesWithNormals;
	VertexTable m_tVertices;
	SharedPtr<Effect> m_pActiveEffect;
	HString m_ActiveEffectTechnique;
	EffectPass m_ActiveEffectPass;
	SharedPtr<IndexBuffer> m_pIndexBuffer;
	SharedPtr<VertexBuffer> m_pVertexBufferNoNormals;
	SharedPtr<VertexBuffer> m_pVertexBufferWithNormals;
	SharedPtr<VertexFormat> m_pVertexFormatNoNormals;
	SharedPtr<VertexFormat> m_pVertexFormatWithNormals;
	Float32 m_fClipValue;
	Bool m_bLines;
	Bool m_bWantsGenerateNormals;
	Bool m_bDrawingWithNormals;

	Bool ShouldDrawWithNormals() const
	{
		return (!m_bLines && m_bWantsGenerateNormals);
	}

	void InternalAddLine(UInt16 i0, UInt16 i1)
	{
		m_vIndices.PushBack(m_vPendingVertices[i0]);
		m_vIndices.PushBack(m_vPendingVertices[i1]);
	}

	void InternalAddTriangle(UInt16 i0, UInt16 i1, UInt16 i2)
	{
		m_vIndices.PushBack(m_vPendingVertices[i0]);
		m_vIndices.PushBack(m_vPendingVertices[i1]);
		m_vIndices.PushBack(m_vPendingVertices[i2]);
	}

	void InternalAddVertex(
		const Matrix4D& m,
		const Vector3D& v0,
		ColorARGBu8 cColor)
	{
		InternalAddVertex(
			Matrix4D::TransformPosition(m, v0),
			cColor);
	}

	void InternalAddVertex(
		const Vector3D& v0,
		ColorARGBu8 cColor)
	{
		PrimitiveVertex vertex;
		vertex.m_vP = v0;
		vertex.m_cColor = RGBA::Create(cColor);
		vertex.m_fClipValue = m_fClipValue;
		UInt16 uIndex = 0;
		if (!m_tVertices.GetValue(vertex, uIndex))
		{
			uIndex = (UInt16)m_vVertices.GetSize();
			m_vVertices.PushBack(vertex);
			SEOUL_VERIFY(m_tVertices.Insert(vertex, uIndex).Second);
		}

		m_vPendingVertices.PushBack(uIndex);
	}

	Bool InternalCommitNoNormals(UInt32 uIndicesCount, UInt32 uVerticesCount);
	Bool InternalCommitWithNormals(UInt32 uIndicesCount, UInt32 uVerticesCount);
	void InternalFlush();
	void InternalPopulateVerticesWithNormals(UInt32 uIndicesCount, UInt32 uVerticesCount);
	void InternalStartIndices(UInt16 uIndices, Bool bLines)
	{
		if (m_bLines != bLines)
		{
			if (!m_vIndices.IsEmpty())
			{
				InternalFlush();
			}
			m_bLines = bLines;
		}

		if (m_vIndices.GetSize() + uIndices > kuMaxIndices)
		{
			InternalFlush();
		}

		m_vPendingVertices.Clear();
	}
	Bool InternalUseEffectTechnique(HString techniqueName);

	SEOUL_DISABLE_COPY(PrimitiveRenderer);
}; // class PrimitiveRenderer

} // namespace Scene

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
