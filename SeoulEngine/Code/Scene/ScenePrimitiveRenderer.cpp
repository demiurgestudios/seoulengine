/**
 * \file ScenePrimitiveRenderer.cpp
 * \brief Handles rendering of simple primitives for debugging
 * purposes (lines, spheres, etc.).
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
#include "ReflectionDefine.h"
#include "RenderCommandStreamBuilder.h"
#include "RenderDevice.h"
#include "ScenePrimitiveRenderer.h"
#include "ScenePrereqs.h"
#include "TextureManager.h"
#include "Triangle3D.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

SEOUL_TYPE(Scene::PrimitiveRenderer, TypeFlags::kDisableNew);

namespace Scene
{

/** EffectParameter used for setting the Camera's ViewProjection transform. */
static const HString kEffectParameterView("seoul_View");
static const HString kEffectParameterProjection("seoul_Projection");

/** Typical IEEE 754 max float 16 value, used for the clip value when disabled. */
static const Float32 kfMaxFloat16 = 65504.0f;

static VertexElement const* GetPrimitiveRendererVertexElements()
{
	static const VertexElement s_kaElements[] =
	{
		// Position and clip plane value (in stream 0)
		{ 0,								// stream
		  0,								// offset
		  VertexElement::TypeFloat4,		// type
		  VertexElement::MethodDefault,		// method
		  VertexElement::UsagePosition,		// usage
		  0u },								// usage index

		// Color (in stream 0)
		{ 0,								// stream
		  16,								// offset
		  VertexElement::TypeColor,			// type
		  VertexElement::MethodDefault,		// method
		  VertexElement::UsageColor,		// usage
		  0u },								// usage index

		VertexElementEnd
	};

	return s_kaElements;
}

static VertexElement const* GetPrimitiveRendererVertexWithNormalElements()
{
	static const VertexElement s_kaElements[] =
	{
		// Position and clip plane value (in stream 0)
		{ 0,								// stream
		  0,								// offset
		  VertexElement::TypeFloat4,		// type
		  VertexElement::MethodDefault,		// method
		  VertexElement::UsagePosition,		// usage
		  0u },								// usage index

		// Normal (in stream 0)
		{ 0,								// stream
		  16,								// offset
		  VertexElement::TypeFloat3,		// type
		  VertexElement::MethodDefault,		// method
		  VertexElement::UsageNormal,		// usage
		  0u },								// usage index

		// Color (in stream 0)
		{ 0,								// stream
		  28,								// offset
		  VertexElement::TypeColor,			// type
		  VertexElement::MethodDefault,		// method
		  VertexElement::UsageColor,		// usage
		  0u },								// usage index

		VertexElementEnd
	};

	return s_kaElements;
}

PrimitiveRenderer::PrimitiveRenderer()
	: m_fDepthBias(kfPrimitiveRendererDepthBias)
	, m_fInfiniteBias(0.0)
	, m_pCamera(nullptr)
	, m_pBuilder(nullptr)
	, m_vIndices()
	, m_vPendingVertices()
	, m_vVertices()
	, m_vVerticesWithNormals()
	, m_tVertices()
	, m_pActiveEffect(nullptr)
	, m_ActiveEffectTechnique()
	, m_ActiveEffectPass()
	, m_pIndexBuffer(RenderDevice::Get()->CreateDynamicIndexBuffer(sizeof(UInt16) * kuMaxIndices, IndexBufferDataFormat::kIndex16))
	, m_pVertexBufferNoNormals(RenderDevice::Get()->CreateDynamicVertexBuffer(sizeof(PrimitiveVertex) * kuMaxVertices, sizeof(PrimitiveVertex)))
	, m_pVertexBufferWithNormals(RenderDevice::Get()->CreateDynamicVertexBuffer(sizeof(PrimitiveVertexWithNormal) * kuMaxVertices, sizeof(PrimitiveVertexWithNormal)))
	, m_pVertexFormatNoNormals(RenderDevice::Get()->CreateVertexFormat(GetPrimitiveRendererVertexElements()))
	, m_pVertexFormatWithNormals(RenderDevice::Get()->CreateVertexFormat(GetPrimitiveRendererVertexWithNormalElements()))
	, m_fClipValue(kfMaxFloat16)
	, m_bLines(false)
	, m_bWantsGenerateNormals(false)
	, m_bDrawingWithNormals(false)
{
}

PrimitiveRenderer::~PrimitiveRenderer()
{
}

void PrimitiveRenderer::BeginFrame(
	const SharedPtr<Camera>& pCamera,
	RenderCommandStreamBuilder& rBuilder)
{
	m_pCamera = pCamera;
	m_fClipValue = kfMaxFloat16;
	m_pBuilder = &rBuilder;
	m_pBuilder->UseVertexFormat(m_pVertexFormatNoNormals.GetPtr());
	m_pBuilder->SetIndices(m_pIndexBuffer.GetPtr());
	m_pBuilder->SetVertices(0u, m_pVertexBufferNoNormals.GetPtr(), 0u, sizeof(PrimitiveVertex));
}

/** Reset (disable) the clip value. */
void PrimitiveRenderer::ResetClipValue()
{
	m_fClipValue = kfMaxFloat16;
}

/**
 * Enable or disable normal generaiton for the current batch.
 * Ignored for lines. Causes an immediate flush on a mode change.
 */
void PrimitiveRenderer::SetGenerateNormals(Bool bGenerateNormals)
{
	if (m_bWantsGenerateNormals != bGenerateNormals)
	{
		if (!m_bLines)
		{
			if (!m_vIndices.IsEmpty())
			{
				InternalFlush();
			}
		}

		m_bWantsGenerateNormals = bGenerateNormals;
	}
}

void PrimitiveRenderer::UseEffect(const SharedPtr<Effect>& pEffect)
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
				kEffectParameterView,
				m_pCamera->GetViewMatrix());
			if (m_fInfiniteBias != 0.0)
			{
				m_pBuilder->SetMatrix4DParameter(
					m_pActiveEffect,
					kEffectParameterProjection,
					m_pCamera->GetProjectionMatrix().InfiniteProjection(m_fInfiniteBias));
			}
			else
			{
				m_pBuilder->SetMatrix4DParameter(
					m_pActiveEffect,
					kEffectParameterProjection,
					m_pCamera->GetProjectionMatrix().BiasedProjection(m_fDepthBias));
			}
			InternalUseEffectTechnique(kEffectTechniqueRender);
		}
	}
}

/**
 * Use a specialized effect technique for the upcmoing batch.
 * Not normally necessary to call this, the default is "seoul_Render".
 * Call with no argument to reset to the default.
 */
void PrimitiveRenderer::UseEffectTechnique(HString techniqueName /*= HString()*/)
{
	if (techniqueName.IsEmpty())
	{
		techniqueName = kEffectTechniqueRender;
	}

	InternalUseEffectTechnique(techniqueName);
}

void PrimitiveRenderer::EndFrame()
{
	if (m_ActiveEffectPass.IsValid())
	{
		InternalFlush();

		m_pBuilder->EndEffectPass(m_pActiveEffect, m_ActiveEffectPass);
		m_ActiveEffectPass = EffectPass();
	}

	if (m_ActiveEffectTechnique != HString() &&
		m_pActiveEffect.IsValid())
	{
		m_pBuilder->EndEffect(m_pActiveEffect);
		m_pActiveEffect.Reset();
	}

	m_bWantsGenerateNormals = false;
	m_bDrawingWithNormals = false;
	m_bLines = false;
	m_ActiveEffectTechnique = HString();
	m_pBuilder.Reset();
	m_pCamera.Reset();
}

/** Add a 3D box with the given vExtents and cColor. */
void PrimitiveRenderer::LineBox(
	const Matrix4D& mWorld,
	const Vector3D& vExtents,
	ColorARGBu8 cColor)
{
	InternalStartIndices(24u, true);

	Vector3D const vMax = (vExtents);
	Vector3D const vDimensions = (2.0f * vExtents);

	Vector3D const v0 = (vMax - Vector3D(0, 0, 0));
	Vector3D const v1 = (vMax - Vector3D(0, vDimensions.Y, 0));
	Vector3D const v2 = (vMax - Vector3D(vDimensions.X, 0, 0));
	Vector3D const v3 = (vMax - Vector3D(vDimensions.X, vDimensions.Y, 0));
	Vector3D const v4 = (vMax - Vector3D(0, 0, vDimensions.Z));
	Vector3D const v5 = (vMax - Vector3D(0, vDimensions.Y, vDimensions.Z));
	Vector3D const v6 = (vMax - Vector3D(vDimensions.X, 0, vDimensions.Z));
	Vector3D const v7 = (vMax - Vector3D(vDimensions.X, vDimensions.Y, vDimensions.Z));

	// setup the vVertices
	InternalAddVertex(mWorld, v0, cColor);
	InternalAddVertex(mWorld, v1, cColor);
	InternalAddVertex(mWorld, v2, cColor);
	InternalAddVertex(mWorld, v3, cColor);
	InternalAddVertex(mWorld, v4, cColor);
	InternalAddVertex(mWorld, v5, cColor);
	InternalAddVertex(mWorld, v6, cColor);
	InternalAddVertex(mWorld, v7, cColor);

	// front
	InternalAddLine(2, 3);
	InternalAddLine(3, 7);
	InternalAddLine(7, 6);
	InternalAddLine(6, 2);

	// sides
	InternalAddLine(0, 2);
	InternalAddLine(1, 3);
	InternalAddLine(5, 7);
	InternalAddLine(4, 6);

	// back
	InternalAddLine(0, 1);
	InternalAddLine(1, 5);
	InternalAddLine(5, 4);
	InternalAddLine(4, 0);
}

/** Add a lined circle oriented in 3D space. */
void PrimitiveRenderer::LineCircle(
	const Vector3D& vCenter,
	const Vector3D& vAxis,
	Float32 fRadius,
	Int32 iSegmentsPerRing,
	Bool bRadiusToMidpoint,
	ColorARGBu8 cColor)
{
	// Need a minimum of 4 segments per ring, or we can't even
	// approach something spherical.
	iSegmentsPerRing = Max(iSegmentsPerRing, 4);

	// We also need an even number of segments.
	if (iSegmentsPerRing % 2 != 0)
	{
		iSegmentsPerRing++;
	}

	// Calculate the step size based on the number of segments
	// per ring desired.
	Float32 const fStep = (2.0f * fPi) / ((Float32)iSegmentsPerRing);

	// If we want the radius to go the midpoint of a segment,
	// extend the radius here.
	if (bRadiusToMidpoint)
	{
		// Cos(fStep * 0.5f) gives us the ratio between the
		// hypotenuse and the adjacent side of the triangle, formed
		// by the midpoint of a segment (adjacent) and the distance between
		// a vertex and the vCenter of a the sphere (hypotenuse).
		Float32 fFactor = Cos(fStep * 0.5f);

		// We're working in 2D, so we need to apply the factor once.
		fRadius = (fRadius / fFactor);
	}

	Vector<Float32, MemoryBudgets::Scene> vVertices;

	// Transform of the circle.
	Matrix4D const m = Matrix4D::CreateRotationFromDirection(
		vAxis,
		-Vector3D::UnitZ());

	// Reserve space.
	InternalStartIndices((UInt16)(2 * iSegmentsPerRing), true);

	// Add vertices.
	for (Int32 i = 0; i < iSegmentsPerRing; ++i)
	{
		Float32 fRo = (fStep * i);
		Float32 fX = (fRadius * (Float32)Sin(fRo));
		Float32 fY = (fRadius * (Float32)Cos(fRo));

		InternalAddVertex(
			vCenter + Matrix4D::TransformPosition(m, Vector3D(fX, fY, 0.0f)),
			cColor);
	}

	// Add indices.
	Int32 iPrev = iSegmentsPerRing - 1;
	for (Int32 iNext = 0; iNext < iSegmentsPerRing; ++iNext)
	{
		InternalAddLine((UInt16)iPrev, (UInt16)iNext);

		iPrev = iNext;
	}
}

/**
* Add a grid with the given transform. Default orientation of the grid is in XZ,
* with (0, 0) the "upper left" corner of the grid (if looking at the great along -Y).
* Each grid cell is 1 unit, so scale accordingly.
*
* If bIncludeBorder is false, then the outer lines will not be included. This can
* be used to nest grids of smaller resolution inside of grids of larger resolution.
*/
void PrimitiveRenderer::LineGrid(
	const Matrix4D& mWorld,
	Int32 iCellsX,
	Int32 iCellsZ,
	Bool bIncludeBorder,
	ColorARGBu8 cColor)
{
	Int32 iFirstX = (bIncludeBorder ? 0 : 1);
	Int32 iLastX = (bIncludeBorder ? iCellsX : iCellsX - 1);
	Int32 iFirstZ = (bIncludeBorder ? 0 : 1);
	Int32 iLastZ = (bIncludeBorder ? iCellsZ : iCellsZ - 1);

	// Reserve.
	InternalStartIndices(
		(iLastX - iFirstX + 1) * 2 +
		(iLastZ - iFirstZ + 1) * 2,
		true);

	// Add vVertices and indices.
	UInt16 uIndexStart = 0;
	for (Int32 iX = iFirstX; iX <= iLastX; ++iX)
	{
		Vector3D v0;
		v0.X = (Float)iX;
		v0.Y = 0.0f;
		v0.Z = 0.0f;

		Vector3D v1;
		v1.X = (Float)iX;
		v1.Y = 0.0f;
		v1.Z = (Float)iCellsZ;

		InternalAddVertex(mWorld, v0, cColor);
		InternalAddVertex(mWorld, v1, cColor);
		InternalAddLine(uIndexStart, uIndexStart + 1);
		uIndexStart += 2;
	}

	for (Int32 iZ = iFirstZ; iZ <= iLastZ; ++iZ)
	{
		Vector3D v0;
		v0.X = 0.0f;
		v0.Y = 0.0f;
		v0.Z = (Float)iZ;

		Vector3D v1;
		v1.X = (Float)iCellsX;
		v1.Y = 0.0f;
		v1.Z = (Float)iZ;

		InternalAddVertex(mWorld, v0, cColor);
		InternalAddVertex(mWorld, v1, cColor);
		InternalAddLine(uIndexStart, uIndexStart + 1);
		uIndexStart += 2;
	}
}

/**
 * Add a lined 3D pyramid for rendering.
 *
 * p0 defines the top or point of the pyramid.
 * p1 defines one corner of the base.
 * p2 defines a second corner of the base, on the diagonal.
 */
void PrimitiveRenderer::LinePyramid(
	const Vector3D& p0,
	const Vector3D& p1,
	const Vector3D& p3,
	ColorARGBu8 cColor)
{
	Vector3D mid = (p3 + p1) * 0.5f;
	Float dis = (mid - p1).Length();
	Vector3D cross = Vector3D::Cross(Vector3D::Normalize(p3 - p1), Vector3D::Normalize(mid - p0));

	Vector3D p2 = (mid - cross * dis);
	Vector3D p4 = (mid + cross * dis);

	// Reserve indices.
	InternalStartIndices(16, true);

	// Add vVertices.
	InternalAddVertex(p0, cColor);
	InternalAddVertex(p1, cColor);
	InternalAddVertex(p2, cColor);
	InternalAddVertex(p3, cColor);
	InternalAddVertex(p4, cColor);

	// Add indices.
	InternalAddLine(0, 1);
	InternalAddLine(0, 2);
	InternalAddLine(0, 3);
	InternalAddLine(0, 4);
	InternalAddLine(1, 2);
	InternalAddLine(2, 3);
	InternalAddLine(3, 4);
	InternalAddLine(4, 1);
}

void PrimitiveRenderer::TriangleBox(
	const Matrix4D& mWorld,
	const Vector3D& vExtents,
	ColorARGBu8 cColor)
{
	// Reserve space for indices.
	InternalStartIndices(6 * 6, false);

	// Add vertices.
	Vector3D const vMax = (vExtents);
	Vector3D const vDimensions = (2.0f * vExtents);

	Vector3D const v0(vMax);
	Vector3D const v1(vMax - Vector3D(0, vDimensions.Y, 0));
	Vector3D const v2(vMax - Vector3D(vDimensions.X, 0, 0));
	Vector3D const v3(vMax - Vector3D(vDimensions.X, vDimensions.Y, 0));
	Vector3D const v4(vMax - Vector3D(0, 0, vDimensions.Z));
	Vector3D const v5(vMax - Vector3D(0, vDimensions.Y, vDimensions.Z));
	Vector3D const v6(vMax - Vector3D(vDimensions.X, 0, vDimensions.Z));
	Vector3D const v7(vMax - Vector3D(vDimensions.X, vDimensions.Y, vDimensions.Z));

	InternalAddVertex(mWorld, v0, cColor);
	InternalAddVertex(mWorld, v1, cColor);
	InternalAddVertex(mWorld, v2, cColor);
	InternalAddVertex(mWorld, v3, cColor);
	InternalAddVertex(mWorld, v4, cColor);
	InternalAddVertex(mWorld, v5, cColor);
	InternalAddVertex(mWorld, v6, cColor);
	InternalAddVertex(mWorld, v7, cColor);

	// Add indices.

	// front
	InternalAddTriangle(0, 2, 1);
	InternalAddTriangle(2, 3, 1);

	// left
	InternalAddTriangle(4, 0, 5);
	InternalAddTriangle(0, 1, 5);

	// right
	InternalAddTriangle(2, 6, 3);
	InternalAddTriangle(6, 7, 3);

	// back
	InternalAddTriangle(6, 4, 7);
	InternalAddTriangle(4, 5, 7);

	// top
	InternalAddTriangle(1, 3, 5);
	InternalAddTriangle(3, 7, 5);

	// bottom
	InternalAddTriangle(4, 6, 0);
	InternalAddTriangle(6, 2, 0);
}

void PrimitiveRenderer::TriangleCapsule(
	const Vector3D& p0,
	const Vector3D& p1,
	Float32 fRadius,
	Int32 iSegmentsPerRing,
	Bool bRadiusToMidpoint,
	ColorARGBu8 cColor)
{
	// Need a minimum of 4 segments per ring, or we can't even
	// approach something spherical.
	iSegmentsPerRing = Max(iSegmentsPerRing, 4);

	// We also need an even number of segments.
	if (iSegmentsPerRing % 2 != 0)
	{
		iSegmentsPerRing++;
	}

	// Calculate the step size based on the number of segments
	// per ring desired.
	Float32 const fStep = fTwoPi / ((Float32)iSegmentsPerRing);

	// If we want the radius to go the midpoint of a segment,
	// extend the radius here.
	if (bRadiusToMidpoint)
	{
		// Cos(fStep * 0.5f) gives us the ratio between the
		// hypotenuse and the adjacent side of the triangle, formed
		// by the midpoint of a segment (adjacent) and the distance between
		// a vertex and the center of a the sphere (hypotenuse).
		Float32 const fFactor = Cos(fStep * 0.5f);

		// We're working in 3D, so we need to apply the factor twice - once
		// to get to the midpoint of a segment, and again to get to the
		// midpoint of the quad formed by 4 connected segments on the
		// surface of the sphere.
		fRadius = (fRadius / fFactor) / fFactor;
	}

	// Compute a transform matrix for positioning and orienting the
	// capsule.
	Matrix4D const mTransform(
		Matrix4D::CreateTranslation((p1 + p0) * 0.5f) *
		Matrix4D::CreateRotationFromDirection(Vector3D::Normalize(p1 - p0), Vector3D::UnitY()));

	Int32 const iIndices = (
		(2 * iSegmentsPerRing + 1) *
		((iSegmentsPerRing + 1) * 6) +
		(iSegmentsPerRing - 1) * (iSegmentsPerRing + 1) * 6);
	Int32 const iVertices = (
		(2 * iSegmentsPerRing + 2) *
		(iSegmentsPerRing + 1) +
		(iSegmentsPerRing - 1) * (iSegmentsPerRing + 1));

	// Reserve space for indices.
	InternalStartIndices(iIndices, false);

	Float32 const fDeltaRingAngle = (fPiOverTwo / (Float32)iSegmentsPerRing);
	Float32 const fDeltaSegAngle = (fTwoPi / (Float32)iSegmentsPerRing);

	Float32 const fHeight = (p1 - p0).Length();

	// Top hemisphere.
	for (Int32 iRing = 0; iRing <= iSegmentsPerRing; ++iRing)
	{
		Float32 const fR0 = fRadius * Sin(iRing * fDeltaRingAngle);
		Float32 const fY0 = fRadius * Cos(iRing * fDeltaRingAngle);

		// Generate the group of segments for the current ring
		for (Int32 iSegment = 0; iSegment <= iSegmentsPerRing; ++iSegment)
		{
			Float32 const fX0 = fR0 * Cos((Float32)iSegment * fDeltaSegAngle);
			Float32 const fZ0 = fR0 * Sin((Float32)iSegment * fDeltaSegAngle);

			// Add the vertex.
			InternalAddVertex(mTransform, Vector3D(fX0, 0.5f * fHeight + fY0, fZ0), cColor);
		}
	}

	// Cylinder.
	Float32 const fDeltaAngle = (fTwoPi / (Float)iSegmentsPerRing);
	Float32 const fDeltaHeight = fHeight / (Float32)iSegmentsPerRing;
	for (Int32 i = 1; i < iSegmentsPerRing; ++i)
	{
		for (Int32 j = 0; j <= iSegmentsPerRing; ++j)
		{
			Float32 const fX0 = fRadius * Cos((Float32)j * fDeltaAngle);
			Float32 const fZ0 = fRadius * Sin((Float32)j * fDeltaAngle);

			// Add the vertex.
			InternalAddVertex(mTransform, Vector3D(fX0, 0.5f * fHeight - (Float32)i * fDeltaHeight, fZ0), cColor);
		}
	}

	// Bottom hemisphere.
	for (Int32 iRing = 0; iRing <= iSegmentsPerRing; ++iRing)
	{
		Float32 const fR0 = fRadius * Sin(fPiOverTwo + (Float32)iRing * fDeltaRingAngle);
		Float32 const fY0 = fRadius * Cos(fPiOverTwo + (Float32)iRing * fDeltaRingAngle);

		// Generate the group of segments for the current ring
		for (Int32 iSegment = 0; iSegment <= iSegmentsPerRing; ++iSegment)
		{
			Float32 const fX0 = fR0 * Cos((Float32)iSegment * fDeltaSegAngle);
			Float32 const fZ0 = fR0 * Sin((Float32)iSegment * fDeltaSegAngle);

			// Add one vertex to the strip which makes up the sphere
			InternalAddVertex(mTransform, Vector3D(fX0, -0.5f * fHeight + fY0, fZ0), cColor);
		}
	}

	// Indices.
	for (Int32 i = 0; i < iVertices - (iSegmentsPerRing + 1); ++i)
	{
		// Add indices.
		InternalAddTriangle(
			(UInt16)(i + iSegmentsPerRing + 1),
			(UInt16)(i + iSegmentsPerRing),
			(UInt16)(i));
		InternalAddTriangle(
			(UInt16)(i + iSegmentsPerRing + 1),
			(UInt16)(i),
			(UInt16)(i + 1));
	}
}

void PrimitiveRenderer::TriangleCone(
	const Vector3D& p0,
	const Vector3D& p1,
	Float32 fRadius,
	Int32 iSegmentsPerRing,
	Bool bRadiusToMidpoint,
	ColorARGBu8 cColor)
{
	const Int32 kFixedVertexCount = 2;

	// Need a minimum of 4 segments per ring, or we can't even
	// approach something spherical.
	iSegmentsPerRing = Max(iSegmentsPerRing, 4);

	// We also need an even number of segments.
	if (iSegmentsPerRing % 2 != 0)
	{
		iSegmentsPerRing++;
	}

	// Calculate the step size based on the number of segments
	// per ring desired.
	Float32 fStep = (2.0f * fPi) / ((Float32)iSegmentsPerRing);

	// If we want the radius to go the midpoint of a segment,
	// extend the radius here.
	if (bRadiusToMidpoint)
	{
		// Cos(fStep * 0.5f) gives us the ratio between the
		// hypotenuse and the adjacent side of the triangle, formed
		// by the midpoint of a segment (adjacent) and the distance between
		// a vertex and the center of a the sphere (hypotenuse).
		Float32 fFactor = (Float32)Cos(fStep * 0.5f);

		// We're working in 2D, so we need to apply the factor once.
		fRadius = (fRadius / fFactor);
	}

	// Reserve space for indices.
	InternalStartIndices((UInt16)(iSegmentsPerRing * 6), false);

	// Transform.
	Matrix4D const m(
		Matrix4D::CreateTranslation(p0) *
		Matrix4D::CreateRotationFromDirection(
			Vector3D::Normalize(p1 - p0),
			-Vector3D::UnitZ()));

	// Vertices.
	Float32 const fDistance = (p1 - p0).Length();

	// Front vertex, a the tip.
	InternalAddVertex(m, Vector3D(0, 0, -fDistance), cColor);

	// Bottom vertex, in the center of the base.
	InternalAddVertex(m, Vector3D::Zero(), cColor);

	for (Int32 i = 0; i < iSegmentsPerRing; i++)
	{
		Float32 fRo = (fStep * i);
		Float32 fX = (fRadius * Sin(fRo));
		Float32 fY = (fRadius * Cos(fRo));

		InternalAddVertex(m, Vector3D(fX, fY, 0), cColor);
	}

	// Sides
	Int32 iPrev = (iSegmentsPerRing - 1);
	for (Int32 i = 0; i < iSegmentsPerRing; i++)
	{
		InternalAddTriangle(
			iPrev + kFixedVertexCount,
			i + kFixedVertexCount,
			0);
		InternalAddTriangle(
			i + kFixedVertexCount,
			iPrev + kFixedVertexCount,
			1);

		iPrev = i;
	}

	// TODO: bottom.
}

/** Adapted from: http://apparat-engine.blogspot.com/2013/04/procdural-meshes-cylinder.html */
void PrimitiveRenderer::TriangleCylinder(
	const Vector3D& p0,
	const Vector3D& p1,
	Float32 fRadius,
	Int32 iSegmentsPerRing,
	Bool bRadiusToMidpoint,
	ColorARGBu8 cColor)
{
	Float32 const fHeight = (p0 - p1).Length();
	Int32 const iNumVerticesPerRow = iSegmentsPerRing + 1;
	Int32 const iNumIndices = iSegmentsPerRing * 2 * 6;

	// Reserve space for indices.
	InternalStartIndices((UInt16)iNumIndices, false);

	// Transform.
	Matrix4D const m =
		Matrix4D::CreateTranslation(p0) *
		Matrix4D::CreateRotationFromDirection(Vector3D::Normalize(p1 - p0), Vector3D::UnitY());

	// Vertices.
	Float32 fTheta = 0.0f;
	Float32 const fHorizontalAngularStride = (fPi * 2.0f) / (Float32)iSegmentsPerRing;
	for (Int32 iVerticalIt = 0; iVerticalIt < 2; iVerticalIt++)
	{
		for (Int32 iHorizontalIt = 0; iHorizontalIt < iNumVerticesPerRow; iHorizontalIt++)
		{
			Float32 fX;
			Float32 fY;
			Float32 fZ;

			fTheta = (fHorizontalAngularStride * iHorizontalIt);

			if (iVerticalIt == 0)
			{
				// upper circle
				fX = fRadius * Cos(fTheta);
				fY = fHeight;
				fZ = fRadius * Sin(fTheta);
			}
			else
			{
				// lower circle
				fX = fRadius * (Float32)Cos(fTheta);
				fY = 0;
				fZ = fRadius * (Float32)Sin(fTheta);
			}

			InternalAddVertex(
				m,
				Vector3D(fX, fY, fZ),
				cColor);
		}
	}

	// Caps.
	InternalAddVertex(m, Vector3D(0, fHeight, 0), cColor);
	InternalAddVertex(m, Vector3D(0, 0, 0), cColor);

	// Indices
	for (Int32 iVerticalIt = 0; iVerticalIt < 1; iVerticalIt++)
	{
		for (Int32 iHorizontalIt = 0; iHorizontalIt < iSegmentsPerRing; iHorizontalIt++)
		{
			Int32 lt = (iHorizontalIt + iVerticalIt * (iNumVerticesPerRow));
			Int32 rt = ((iHorizontalIt + 1) + iVerticalIt * (iNumVerticesPerRow));

			Int32 lb = (iHorizontalIt + (iVerticalIt + 1) * (iNumVerticesPerRow));
			Int32 rb = ((iHorizontalIt + 1) + (iVerticalIt + 1) * (iNumVerticesPerRow));

			InternalAddTriangle(lt, rt, lb);
			InternalAddTriangle(rt, rb, lb);
		}
	}

	for (Int32 iVerticalIt = 0; iVerticalIt < 1; iVerticalIt++)
	{
		for (Int32 iHorizontalIt = 0; iHorizontalIt < iSegmentsPerRing; iHorizontalIt++)
		{
			Int32 lt = (iHorizontalIt + iVerticalIt * (iNumVerticesPerRow));
			Int32 rt = ((iHorizontalIt + 1) + iVerticalIt * (iNumVerticesPerRow));

			Int32 iPatchIndexTop = (iNumVerticesPerRow * 2);
			InternalAddTriangle(lt, iPatchIndexTop, rt);
		}
	}

	for (Int32 iVerticalIt = 0; iVerticalIt < 1; iVerticalIt++)
	{
		for (Int32 iHorizontalIt = 0; iHorizontalIt < iSegmentsPerRing; iHorizontalIt++)
		{
			Int32 lb = (iHorizontalIt + (iVerticalIt + 1) * (iNumVerticesPerRow));
			Int32 rb = ((iHorizontalIt + 1) + (iVerticalIt + 1) * (iNumVerticesPerRow));

			Int32 iPatchIndexBottom = (iNumVerticesPerRow * 2 + 1);
			InternalAddTriangle(lb, rb, iPatchIndexBottom);
		}
	}
}

/**
 * Creates indices and vertices for a sphere mesh.
 *
 * @param[in] fRadius Radius of the sphere.
 * @param[in] uSegmentsPerRing # of segments per 360 ring of the sphere.
 * @param[in] bRadiusToMidpoint If this argument is true, the minimum
 * distance to the origin will be fRadius. Otherwise, the maximum distance
 * from the origin will be fRadius.
 * @param[out] indices Resulting indices of the generated sphere mesh.
 * @param[out] vertices Resulting vertices of the generates sphere mesh.
 */
void PrimitiveRenderer::TriangleSphere(
	const Vector3D& vCenter,
	Float32 fRadius,
	Int32 iSegmentsPerRing,
	Bool bRadiusToMidpoint,
	ColorARGBu8 cColor)
{
	// Need a minimum of 4 segments per ring, or we can't even
	// approach something spherical.
	iSegmentsPerRing = Max(iSegmentsPerRing, 4);

	// We also need an even number of segments.
	if (iSegmentsPerRing % 2 != 0)
	{
		iSegmentsPerRing++;
	}

	// Calculate the step size based on the number of segments
	// per ring desired.
	Float32 const fStep = fTwoPi / ((Float32)iSegmentsPerRing);

	// If we want the radius to go the midpoint of a segment,
	// extend the radius here.
	if (bRadiusToMidpoint)
	{
		// Cos(fStep * 0.5f) gives us the ratio between the
		// hypotenuse and the adjacent side of the triangle, formed
		// by the midpoint of a segment (adjacent) and the distance between
		// a vertex and the center of a the sphere (hypotenuse).
		Float32 const fFactor = Cos(fStep * 0.5f);

		// We're working in 3D, so we need to apply the factor twice - once
		// to get to the midpoint of a segment, and again to get to the
		// midpoint of the quad formed by 4 connected segments on the
		// surface of the sphere.
		fRadius = (fRadius / fFactor) / fFactor;
	}

	// Cache counts.
	Int32 const iIndices = (
		(iSegmentsPerRing * 3) +
		(((iSegmentsPerRing / 2) - 2) * iSegmentsPerRing * 6) +
		(iSegmentsPerRing * 3));
	Int32 const iVertices = (
		1 +
		(((iSegmentsPerRing / 2) - 1) * iSegmentsPerRing) +
		1);

	// Reserve space for indices.
	InternalStartIndices((UInt16)iIndices, false);

	// Transformation.
	Matrix4D const m(Matrix4D::CreateTranslation(vCenter));

	// Add vertices.

	// Front vertex, tip of the front cap.
	InternalAddVertex(m, Vector3D(0.0f, fRadius, 0.0f), cColor);

	// Now generate vertices for all the rings. Each ring is perpendicular
	// to the direction formed by drawing a line from the front to the
	// back vertex.
	for (Int32 i = 1; i < (iSegmentsPerRing / 2); i++)
	{
		Float32 const fTheta = (fStep * i);
		Float32 const fY = (fRadius * Cos(fTheta));
		Float32 const fFactor = (fRadius * Sin(fTheta));

		for (Int32 j = 0; j < iSegmentsPerRing; j++)
		{
			Float32 const fRo = (fStep * j);
			Float32 const fX = (fFactor * Sin(fRo));
			Float32 const fZ = (fFactor * Cos(fRo));

			InternalAddVertex(m, Vector3D(fX, fY, fZ), cColor);
		}
	}

	// Back vertex, tip of the back cap.
	InternalAddVertex(m, Vector3D(0.0f, -fRadius, 0.0f), cColor);

	// Add indices.

	// Indices for front cap, connects the front vertex to all
	// of the vertices of the front-most ring.
	{
		Int32 iPrev = (iSegmentsPerRing - 1);
		for (Int32 i = 0; i < iSegmentsPerRing; iPrev = i, i++)
		{
			InternalAddTriangle(0, 1 + i, 1 + iPrev);
		}
	}

	// Indices for internal triangles, connects the vertices
	// of adjacent rings.
	for (Int32 i = 0; i < ((iSegmentsPerRing / 2) - 2); i++)
	{
		// +1 to skip front cap, since we connected the vertices
		// of the front cap to the front vertex in the first loop.
		Int32 iOffset = (i * iSegmentsPerRing) + 1;
		Int32 iPrev = (iSegmentsPerRing - 1);
		for (Int32 j = 0; j < iSegmentsPerRing; iPrev = j, j++)
		{
			Int32 i0 = (iOffset + iPrev);
			Int32 i1 = (iOffset + j);
			Int32 i2 = (iOffset + iPrev + iSegmentsPerRing);
			Int32 i3 = (iOffset + j + iSegmentsPerRing);

			InternalAddTriangle(i0, i1, i2);
			InternalAddTriangle(i2, i1, i3);
		}
	}

	// Indices for back cap, connects the back vertex to all
	// of the vertices of the back-most ring.
	{
		// +1 to skip back cap, since we connected the vertices
		// of the back cap to the second-most back ring during the inner loop.
		Int32 const iOffset = (((iSegmentsPerRing / 2) - 2) * iSegmentsPerRing) + 1;
		Int32 const iLast = (iVertices - 1);

		Int32 iPrev = (iSegmentsPerRing - 1);
		for (Int32 i = 0; i < iSegmentsPerRing; iPrev = i, i++)
		{
			InternalAddTriangle(iOffset + iPrev, iOffset + i, iLast);
		}
	}
}

/** Adapted from: http://apparat-engine.blogspot.com/2013/04/procedural-meshes-torus.html */
void PrimitiveRenderer::TriangleTorus(
	const Vector3D& vCenter,
	const Vector3D& vAxis,
	Float32 fInnerRadius,
	Float32 fOuterRadius,
	Int32 iSegmentsPerRing,
	Int32 iTotalRings,
	Bool bRadiusToMidpoint,
	ColorARGBu8 cColor)
{
	fInnerRadius = Min(fInnerRadius, fOuterRadius);
	Float32 fRingRadius = (fOuterRadius - fInnerRadius) * 0.5f;

	Int32 iNumVerticesPerRow = iSegmentsPerRing + 1;
	Int32 iNumVerticesPerColumn = iTotalRings + 1;

	Float32 fTheta = 0.0f;
	Float32 fPhi = 0.0f;

	// Reserve space for indices.
	Int32 const iNumIndices = iSegmentsPerRing * iTotalRings * 6;
	InternalStartIndices((UInt16)iNumIndices, false);

	// Transformation.
	Matrix4D const m =
		Matrix4D::CreateTranslation(vCenter) *
		Matrix4D::CreateRotationFromDirection(vAxis, -Vector3D::UnitZ());

	// Add vertices.
	Float32 const fVerticalAngularStride = (fPi * 2.0f) / (Float32)iTotalRings;
	Float32 const fHorizontalAngularStride = (fPi * 2.0f) / (Float32)iSegmentsPerRing;
	for (Int32 iVerticalIt = 0; iVerticalIt < iNumVerticesPerColumn; iVerticalIt++)
	{
		fTheta = fVerticalAngularStride * iVerticalIt;
		for (Int32 iHorizontalIt = 0; iHorizontalIt < iNumVerticesPerRow; iHorizontalIt++)
		{
			fPhi = fHorizontalAngularStride * iHorizontalIt;

			// position
			Float32 const fX = Cos(fTheta) * (fOuterRadius + fRingRadius * (Float32)Cos(fPhi));
			Float32 const fY = Sin(fTheta) * (fOuterRadius + fRingRadius * (Float32)Cos(fPhi));
			Float32 fZ = fRingRadius * Sin(fPhi);

			InternalAddVertex(
				m,
				Vector3D(fX, fY, fZ),
				cColor);
		}
	}

	// Add indices.
	for (Int32 iVerticalIt = 0; iVerticalIt < iTotalRings; iVerticalIt++)
	{
		for (Int32 iHorizontalIt = 0; iHorizontalIt < iSegmentsPerRing; iHorizontalIt++)
		{
			Int32 lt = (iHorizontalIt + iVerticalIt * (iNumVerticesPerRow));
			Int32 rt = ((iHorizontalIt + 1) + iVerticalIt * (iNumVerticesPerRow));

			Int32 lb = (iHorizontalIt + (iVerticalIt + 1) * (iNumVerticesPerRow));
			Int32 rb = ((iHorizontalIt + 1) + (iVerticalIt + 1) * (iNumVerticesPerRow));

			InternalAddTriangle(lt, rt, lb);
			InternalAddTriangle(rt, rb, lb);
		}
	}
}

void PrimitiveRenderer::UseDepthBias(Double fDepthBias /*= kfPrimitiveRendererDepthBias*/)
{
	if (fDepthBias != m_fDepthBias)
	{
		if (0.0 == m_fInfiniteBias)
		{
			InternalFlush();
		}

		m_fDepthBias = fDepthBias;

		if (0.0 == m_fInfiniteBias)
		{
			m_pBuilder->SetMatrix4DParameter(
				m_pActiveEffect,
				kEffectParameterProjection,
				m_pCamera->GetProjectionMatrix().BiasedProjection(m_fDepthBias));
		}
	}
}

void PrimitiveRenderer::UseInfiniteProjection(Double fBias /* = 0.0 */)
{
	if (fBias != m_fInfiniteBias)
	{
		if (0.0 != m_fInfiniteBias)
		{
			InternalFlush();
		}

		m_fInfiniteBias = fBias;

		if (0.0 == m_fInfiniteBias)
		{
			m_pBuilder->SetMatrix4DParameter(
				m_pActiveEffect,
				kEffectParameterProjection,
				m_pCamera->GetProjectionMatrix().BiasedProjection(m_fDepthBias));
		}
		else
		{
			m_pBuilder->SetMatrix4DParameter(
				m_pActiveEffect,
				kEffectParameterProjection,
				m_pCamera->GetProjectionMatrix().InfiniteProjection(m_fInfiniteBias));
		}
	}
}

void PrimitiveRenderer::InternalFlush()
{
	// Early out if no indices.
	UInt32 const uIndicesCount = Min(m_vIndices.GetSize(), kuMaxIndices);
	if (uIndicesCount == 0u)
	{
		return;
	}

	UInt32 const uVerticesCount = Min(m_vVertices.GetSize(), kuMaxVertices);
	Bool const bShouldDrawWithNormals = ShouldDrawWithNormals();
	if (bShouldDrawWithNormals)
	{
		if (!InternalCommitWithNormals(uIndicesCount, uVerticesCount))
		{
			return;
		}
	}
	else
	{
		if (!InternalCommitNoNormals(uIndicesCount, uVerticesCount))
		{
			return;
		}
	}

	if (bShouldDrawWithNormals != m_bDrawingWithNormals)
	{
		m_pBuilder->UseVertexFormat(bShouldDrawWithNormals
			? m_pVertexFormatWithNormals.GetPtr()
			: m_pVertexFormatNoNormals.GetPtr());
		m_pBuilder->SetVertices(
			0u,
			bShouldDrawWithNormals
				? m_pVertexBufferWithNormals.GetPtr()
				: m_pVertexBufferNoNormals.GetPtr(),
			0u,
			bShouldDrawWithNormals
				? sizeof(PrimitiveVertexWithNormal)
				: sizeof(PrimitiveVertex));
	}

	m_pBuilder->CommitEffectPass(m_pActiveEffect, m_ActiveEffectPass);

	m_pBuilder->DrawIndexedPrimitive(
		m_bLines ? PrimitiveType::kLineList : PrimitiveType::kTriangleList,
		0, // Vertex adjustment/offset
		0, // Min index,
		uVerticesCount, // Vertex count,
		0u,
		(m_bLines ? (uIndicesCount / 2u) : (uIndicesCount / 3u)));

	m_bLines = false;
}

Bool PrimitiveRenderer::InternalCommitNoNormals(UInt32 uIndicesCount, UInt32 uVerticesCount)
{
	// Early out.
	if (0 == uIndicesCount || 0 == uVerticesCount)
	{
		return false;
	}

	// Populate indices.
	{
		UInt16* pIndices = (UInt16*)m_pBuilder->LockIndexBuffer(m_pIndexBuffer.GetPtr(), uIndicesCount * sizeof(UInt16));
		if (nullptr == pIndices)
		{
			m_vIndices.Clear();
			m_vVertices.Clear();
			m_tVertices.Clear();
			return false;
		}
		memcpy(pIndices, m_vIndices.Get(0u), uIndicesCount * sizeof(UInt16));
		m_pBuilder->UnlockIndexBuffer(m_pIndexBuffer.GetPtr());
		m_vIndices.Clear();
	}

	// Populate vertices.
	{
		PrimitiveVertex* pVertices = (PrimitiveVertex*)m_pBuilder->LockVertexBuffer(m_pVertexBufferNoNormals.GetPtr(), uVerticesCount * sizeof(PrimitiveVertex));
		if (nullptr == pVertices)
		{
			m_vVertices.Clear();
			m_tVertices.Clear();
			return false;
		}
		memcpy(pVertices, m_vVertices.Get(0u), uVerticesCount * sizeof(PrimitiveVertex));
		m_pBuilder->UnlockVertexBuffer(m_pVertexBufferNoNormals.GetPtr());
		m_vVertices.Clear();
		m_tVertices.Clear();
	}

	return true;
}

Bool PrimitiveRenderer::InternalCommitWithNormals(UInt32 uIndicesCount, UInt32 uVerticesCount)
{
	// Early out.
	if (0 == uIndicesCount || 0 == uVerticesCount)
	{
		return false;
	}

	InternalPopulateVerticesWithNormals(uIndicesCount, uVerticesCount);

	// Populate indices.
	{
		UInt16* pIndices = (UInt16*)m_pBuilder->LockIndexBuffer(m_pIndexBuffer.GetPtr(), uIndicesCount * sizeof(UInt16));
		if (nullptr == pIndices)
		{
			m_vIndices.Clear();
			m_vVerticesWithNormals.Clear();
			m_tVertices.Clear();
			return false;
		}
		memcpy(pIndices, m_vIndices.Get(0u), uIndicesCount * sizeof(UInt16));
		m_pBuilder->UnlockIndexBuffer(m_pIndexBuffer.GetPtr());
		m_vIndices.Clear();
	}

	// Populate vertices.
	{
		PrimitiveVertexWithNormal* pVertices = (PrimitiveVertexWithNormal*)m_pBuilder->LockVertexBuffer(m_pVertexBufferWithNormals.GetPtr(), uVerticesCount * sizeof(PrimitiveVertexWithNormal));
		if (nullptr == pVertices)
		{
			m_vVerticesWithNormals.Clear();
			m_tVertices.Clear();
			return false;
		}
		memcpy(pVertices, m_vVerticesWithNormals.Get(0u), uVerticesCount * sizeof(PrimitiveVertexWithNormal));
		m_pBuilder->UnlockVertexBuffer(m_pVertexBufferWithNormals.GetPtr());
		m_vVerticesWithNormals.Clear();
		m_tVertices.Clear();
	}

	return true;
}

void PrimitiveRenderer::InternalPopulateVerticesWithNormals(UInt32 uIndicesCount, UInt32 uVerticesCount)
{
	m_vVerticesWithNormals.Resize(m_vVertices.GetSize());
	for (UInt32 i = 0u; i < uVerticesCount; ++i)
	{
		m_vVerticesWithNormals[i] = PrimitiveVertexWithNormal::Create(m_vVertices[i]);
	}
	m_vVertices.Clear();

	// Sanity check - if not true, being called with a run of lines.
	SEOUL_ASSERT(uIndicesCount % 3 == 0);

	for (UInt32 i = 0u; i < uIndicesCount; i += 3)
	{
		PrimitiveVertexWithNormal& rV0(m_vVerticesWithNormals[m_vIndices[i+0]]);
		PrimitiveVertexWithNormal& rV1(m_vVerticesWithNormals[m_vIndices[i+1]]);
		PrimitiveVertexWithNormal& rV2(m_vVerticesWithNormals[m_vIndices[i+2]]);
		Vector3D const v0(rV0.m_vP);
		Vector3D const v1(rV1.m_vP);
		Vector3D const v2(rV2.m_vP);

		Triangle3D const triangle(v0, v1, v2);
		Vector3D const vNormal(triangle.GetNormal());
		rV0.m_vN += vNormal;
		rV1.m_vN += vNormal;
		rV2.m_vN += vNormal;
	}

	for (UInt32 i = 0u; i < uVerticesCount; ++i)
	{
		PrimitiveVertexWithNormal& rV(m_vVerticesWithNormals[i]);
		rV.m_vN = Vector3D::Normalize(rV.m_vN);
	}
}

Bool PrimitiveRenderer::InternalUseEffectTechnique(HString techniqueName)
{
	if (techniqueName != m_ActiveEffectTechnique)
	{
		if (m_ActiveEffectTechnique != HString())
		{
			if (m_ActiveEffectPass.IsValid())
			{
				InternalFlush();

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

} // namespace Scene

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
