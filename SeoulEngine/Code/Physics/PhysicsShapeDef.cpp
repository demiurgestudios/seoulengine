/**
 * \file PhysicsShapeDef.cpp
 * \brief Shareable data of shape instances. Shape define collision
 * properties of a physics body.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "PhysicsShapeDef.h"
#include "PhysicsUtil.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"
#include "StackOrHeapArray.h"

#if SEOUL_WITH_PHYSICS

#include <bounce/quickhull/qh_hull.h>

namespace Seoul
{

namespace Reflection
{

namespace Attributes
{

#if SEOUL_EDITOR_AND_TOOLS
/**
 * Editor/tools only handling of ShapeType for
 * allowing selection of the shape data in
 * a manner that supports undo/redo.
 */
class PhysicsShapeTypeEnumLike SEOUL_SEALED : public EnumLike
{
public:
	virtual void GetNames(Names& rvNames) const SEOUL_OVERRIDE
	{
		rvNames = EnumOf<Physics::ShapeType>().GetNames();
	}

	virtual void NameToValue(HString name, Reflection::Any& rValue) const SEOUL_OVERRIDE
	{
		using namespace Physics;

		// Convert the name to a type.
		auto eType = ShapeType::kNone;
		if (!EnumOf<ShapeType>().TryGetValue(name, eType))
		{
			return;
		}

		// TODO: Retrieve the value in rValue
		// and copy through data from old to new.

		// Get the existing value, check if it's
		// already the desired type. Leave unmodified if so.
		if (rValue.IsOfType< SharedPtr<IShapeData> >())
		{
			auto p(rValue.Cast< SharedPtr<IShapeData> >());

			auto eOldType = ShapeType::kNone;
			if (p.IsValid())
			{
				eOldType = p->GetType();
			}

			// Return if same type.
			if (eOldType == eType)
			{
				return;
			}
		}

		// Now instantiate a new data blob for the new type.
		SharedPtr<IShapeData> p;
		switch (eType)
		{
		case ShapeType::kBox: p.Reset(SEOUL_NEW(MemoryBudgets::Physics) BoxShapeData); break;
		case ShapeType::kCapsule: p.Reset(SEOUL_NEW(MemoryBudgets::Physics) CapsuleShapeData); break;
		case ShapeType::kConvexHull: p.Reset(SEOUL_NEW(MemoryBudgets::Physics) ConvexHullShapeData); break;
		case ShapeType::kSphere: p.Reset(SEOUL_NEW(MemoryBudgets::Physics) SphereShapeData); break;

		case ShapeType::kNone:
		default:
			// No data.
			break;
		};

		// Assign to output.
		rValue = p;
	}

	virtual void ValueToName(const Reflection::Any& value, HString& rName) const SEOUL_OVERRIDE
	{
		using namespace Physics;

		// Retrieve type from existing value.
		if (value.IsOfType< SharedPtr<IShapeData> >())
		{
			auto p(value.Cast< SharedPtr<IShapeData> >());

			// Extract the type.
			auto eType = ShapeType::kNone;
			if (p.IsValid())
			{
				eType = p->GetType();
			}

			// If successful, we're done.
			if (EnumOf<ShapeType>().TryGetName(eType, rName))
			{
				return;
			}
		}

		rName = HString();
	}
}; // class PhysicsShapeTypeEnumLike
#endif // /#if SEOUL_EDITOR_AND_TOOLS

} // namespace Attributes

} // namespace Reflection

SEOUL_BEGIN_TYPE(Physics::ShapeDef, TypeFlags::kDisableCopy)
	SEOUL_ATTRIBUTE(NotRequired)

#if SEOUL_EDITOR_AND_TOOLS
	SEOUL_PROPERTY_N("Type", m_pData)
		SEOUL_ATTRIBUTE(DoNotSerialize)
		SEOUL_ATTRIBUTE(PhysicsShapeTypeEnumLike)
		SEOUL_DEV_ONLY_ATTRIBUTE(Description, "Collision shape selection.")
#endif // /#if SEOUL_EDITOR_AND_TOOLS

	SEOUL_PROPERTY_N("Sensor", m_bSensor)
		SEOUL_DEV_ONLY_ATTRIBUTE(Description, "If true, this shape is a sensor (generates contacts but not collision constraints).")
	SEOUL_PROPERTY_N("Density", m_fDensity)
		SEOUL_DEV_ONLY_ATTRIBUTE(Description, "Density in kg/m^3.")
	SEOUL_PROPERTY_N("Friction", m_fFriction)
		SEOUL_DEV_ONLY_ATTRIBUTE(Description, "Friction on [0, 1]")
		SEOUL_DEV_ONLY_ATTRIBUTE(Range, 0.0f, 1.0f)
	SEOUL_PROPERTY_N("Restitution", m_fRestitution)
		SEOUL_DEV_ONLY_ATTRIBUTE(Description, "Bounciness or elasticity.")
		SEOUL_DEV_ONLY_ATTRIBUTE(Range, 0.0f, 1.0f)
	SEOUL_PROPERTY_N("Data", m_pData)
SEOUL_END_TYPE()

SEOUL_SPEC_TEMPLATE_TYPE(SharedPtr<Physics::IShapeData>)
SEOUL_BEGIN_TYPE(Physics::IShapeData, TypeFlags::kDisableNew)
	SEOUL_ATTRIBUTE(PolymorphicKey, "$type")
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Physics::BoxShapeData, TypeFlags::kDisableCopy)
	SEOUL_PARENT(Physics::IShapeData)
	SEOUL_PROPERTY_N("Center", m_vCenter)
		SEOUL_DEV_ONLY_ATTRIBUTE(Description, "Local center of the shape.")
	SEOUL_PROPERTY_N("Extents", m_vExtents)
		SEOUL_DEV_ONLY_ATTRIBUTE(Description, "Half the width, height, depth of the shape.")
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Physics::CapsuleShapeData, TypeFlags::kDisableCopy)
	SEOUL_PARENT(Physics::IShapeData)
	SEOUL_PROPERTY_N("P0", m_vP0)
		SEOUL_DEV_ONLY_ATTRIBUTE(Description, "Local endpoint 0 of the capsule shape.")
	SEOUL_PROPERTY_N("P1", m_vP1)
		SEOUL_DEV_ONLY_ATTRIBUTE(Description, "Local endpoint 1 of the capsule shape.")
	SEOUL_PROPERTY_N("Radius", m_fRadius)
		SEOUL_DEV_ONLY_ATTRIBUTE(Description, "Radius of the capsule shape.")
		SEOUL_DEV_ONLY_ATTRIBUTE(Range, 0.0f, 500.0f)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Physics::ConvexHullShapeData, TypeFlags::kDisableCopy)
	SEOUL_PARENT(Physics::IShapeData)
	SEOUL_PROPERTY_N("Indices", m_vIndices)
		SEOUL_ATTRIBUTE(DoNotEdit)
	SEOUL_PROPERTY_N("Points", m_vPoints)
		SEOUL_ATTRIBUTE(DoNotEdit)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Physics::SphereShapeData, TypeFlags::kDisableCopy)
	SEOUL_PARENT(Physics::IShapeData)
	SEOUL_PROPERTY_N("Center", m_vCenter)
		SEOUL_DEV_ONLY_ATTRIBUTE(Description, "Local center of the shape.")
	SEOUL_PROPERTY_N("Radius", m_fRadius)
		SEOUL_DEV_ONLY_ATTRIBUTE(Description, "Radius of the sphere shape.")
		SEOUL_DEV_ONLY_ATTRIBUTE(Range, 0.0f, 500.0f)
SEOUL_END_TYPE()

namespace Physics
{

namespace
{

template <typename T>
class ReadOnlyArrayB3 SEOUL_SEALED : public b3Array<T>
{
public:
	ReadOnlyArrayB3(T const* pBegin, T const* pEnd)
		: b3Array<T>((T*)pBegin, (UInt32)(pEnd - pBegin))
	{
		this->m_count = this->m_capacity;
	}

private:
	SEOUL_DISABLE_COPY(ReadOnlyArrayB3);
}; // class ReadOnlyArrayB3

} // namespace anonymous

IShapeData* BoxShapeData::Clone() const
{
	auto pReturn = SEOUL_NEW(MemoryBudgets::Physics) BoxShapeData;
	pReturn->m_vCenter = m_vCenter;
	pReturn->m_vExtents = m_vExtents;
	return pReturn;
}

AABB BoxShapeData::ComputeAABB() const
{
	return AABB::CreateFromCenterAndExtents(m_vCenter, m_vExtents);
}

void BoxShapeData::ComputeScaled(const Vector3D& vInScale, BoxShapeData& rOut) const
{
	auto const vScale(SanitizeScale(vInScale));

	rOut.m_vCenter = Vector3D::ComponentwiseMultiply(vScale, m_vCenter);
	rOut.m_vExtents = Vector3D::ComponentwiseMultiply(vScale, m_vExtents);
}

IShapeData* CapsuleShapeData::Clone() const
{
	auto pReturn = SEOUL_NEW(MemoryBudgets::Physics) CapsuleShapeData;
	pReturn->m_fRadius = m_fRadius;
	pReturn->m_vP0 = m_vP0;
	pReturn->m_vP1 = m_vP1;
	return pReturn;
}

AABB CapsuleShapeData::ComputeAABB() const
{
	auto const vMin = Vector3D::Min(m_vP0, m_vP1);
	auto const vMax = Vector3D::Max(m_vP0, m_vP1);
	auto const vEnds = Vector3D(m_fRadius);
	return AABB::CreateFromMinAndMax(vMin - vEnds, vMax + vEnds);
}

void CapsuleShapeData::ComputeScaled(const Vector3D& vInScale, CapsuleShapeData& rOut) const
{
	auto const vScale(SanitizeScale(vInScale));

	// Radius is a little complicated. We need to generate a plane, treating
	// the axis of the cylinder as the plane's normal, then "project" the scale
	// onto that plane. The max component of that projected scale is the scaling
	// factor for the radius.
	auto const plane = Plane::CreateFromPositionAndNormal(Vector3D::Zero(), Vector3D::Normalize(m_vP1 - m_vP0));
	auto const vScaleP = plane.ProjectOnto(vScale);
	auto const fRadiusScale = vScaleP.GetMaxComponent();

	rOut.m_fRadius = (m_fRadius * fRadiusScale);
	rOut.m_vP0 = Vector3D::ComponentwiseMultiply(vScale, m_vP0);
	rOut.m_vP1 = Vector3D::ComponentwiseMultiply(vScale, m_vP1);
}

ConvexHullShapeData::ConvexHullShapeData()
	: m_vEdges()
	, m_vFaces()
	, m_vIndices()
	, m_vPlanes()
	, m_vPoints()
	, m_vCenterOfMass()
{
}

ConvexHullShapeData::~ConvexHullShapeData()
{
}

/**
 * Recompute this ConvexHullShapeData by generating a best fit convex hull for the given points
 * and constraints.
 */
void ConvexHullShapeData::CalculateFromPoints(Vector3D const* pBegin, Vector3D const* pEnd)
{
	Indices vIndices;
	Points vPoints;


	// Nothing to compute if no input.
	if (pBegin != pEnd)
	{
		// Wrap the input in a read-only array.
		ReadOnlyArrayB3<b3Vec3> a((b3Vec3 const*)pBegin, (b3Vec3 const*)pEnd);

		// Allocate output scratch space. TODO:
		Vector<Byte, MemoryBudgets::Physics> vBuffer;
		vBuffer.Resize(qhGetMemorySize(a.Count()));

		HashTable<qhVertex*, UInt32, MemoryBudgets::Physics> tLookup;

		// Generate the hull.
		qhHull hull;
		hull.Construct(vBuffer.Data(), a);

		// Now iterate and accumulate.
		UInt32 uIndexCount = 0u;
		UInt32 uFaceCountOffset = 1u;

		auto pFace = hull.m_faceList.head;
		while (nullptr != pFace)
		{
			// Get the edge count - if it's the same
			// as the last edge count, we can just increment
			// the number of faces in the sequence. Otherwise,
			// we need to start a new sequence.
			UInt32 const uEdgeCount = pFace->EdgeCount();
			if (uEdgeCount != uIndexCount)
			{
				uIndexCount = uEdgeCount;
				vIndices.PushBack(uEdgeCount);
				uFaceCountOffset = vIndices.GetSize();
				vIndices.PushBack(0u);
			}

			// Enumerate edges of the face and add to the current face.
			auto pEdge = pFace->edge;
			do
			{
				UInt32 uVertex = 0u;
				if (!tLookup.GetValue(pEdge->tail, uVertex))
				{
					uVertex = vPoints.GetSize();
					vPoints.PushBack(Convert(pEdge->tail->position));
					SEOUL_VERIFY(tLookup.Insert(pEdge->tail, uVertex).Second);
				}

				vIndices.PushBack(uVertex);

				pEdge = pEdge->next;
			} while (pFace->edge != pEdge);

			vIndices[uFaceCountOffset]++;
			pFace = pFace->next;
		}
	}

	// Done, commit.
	m_vIndices.Swap(vIndices);
	m_vPoints.Swap(vPoints);

	// Now generate dependent structures.
	(void)PostSerialize();
}

IShapeData* ConvexHullShapeData::Clone() const
{
	auto pReturn = SEOUL_NEW(MemoryBudgets::Physics) ConvexHullShapeData;
	pReturn->m_vEdges = m_vEdges;
	pReturn->m_vFaces = m_vFaces;
	pReturn->m_vIndices = m_vIndices;
	pReturn->m_vPlanes = m_vPlanes;
	pReturn->m_vPoints = m_vPoints;
	pReturn->m_vCenterOfMass = m_vCenterOfMass;
	return pReturn;
}

AABB ConvexHullShapeData::ComputeAABB() const
{
	return AABB::CalculateFromPoints(m_vPoints.Begin(), m_vPoints.End());
}

void ConvexHullShapeData::ComputeScaled(const Vector3D& vInScale, ConvexHullShapeData& rOut) const
{
	auto const vScale(SanitizeScale(vInScale));

	rOut.m_vEdges = m_vEdges;
	rOut.m_vFaces = m_vFaces;
	rOut.m_vIndices = m_vIndices;
	rOut.m_vPlanes = m_vPlanes;
	rOut.m_vPoints = m_vPoints;
	for (auto& r : rOut.m_vPoints)
	{
		r = Vector3D::ComponentwiseMultiply(vScale, r);
	}
	rOut.ComputeCenterOfMass();
}

void ConvexHullShapeData::ComputeCenterOfMass()
{
	// Edge case.
	if (m_vPoints.IsEmpty())
	{
		m_vCenterOfMass = Vector3D::Zero();
	}
	else
	{
		// Common case - compute mean of all points.
		Double fX = 0.0;
		Double fY = 0.0;
		Double fZ = 0.0;
		for (auto const& v : m_vPoints)
		{
			fX += (Double)v.X;
			fY += (Double)v.Y;
			fZ += (Double)v.Z;
		}

		Double fDenominator = (Double)((Int32)m_vPoints.GetSize());
		m_vCenterOfMass.X = (Float32)(fX / fDenominator);
		m_vCenterOfMass.Y = (Float32)(fY / fDenominator);
		m_vCenterOfMass.Z = (Float32)(fZ / fDenominator);
	}
}

void ConvexHullShapeData::ComputeUtils()
{
	Edges vEdges;
	Faces vFaces;
	Planes vPlanes;

	FixedArray<Vector3D, 3u> aPlaneVertices;
	UInt32 const uIndices = GetIndices().GetSize();
	UInt32 uIndex = 0u;
	while (uIndex < uIndices)
	{
		UInt32 const uFaceIndexCount = GetIndices()[uIndex++];
		UInt32 const uFaceCount = GetIndices()[uIndex++];

		// Sanity check so we don't crash - means bad data though.
		if (uFaceCount * uFaceIndexCount > uIndices)
		{
			break;
		}

		for (UInt32 uFace = 0u; uFace < uFaceCount; ++uFace)
		{
			UInt8 const uOutEdge = (UInt8)m_vEdges.GetSize();
			UInt8 const uOutFace = (UInt8)m_vFaces.GetSize();

			UInt32 uPlaneOut = 0u;
			vFaces.PushBack(uOutEdge);
			for (UInt32 u = uIndex; u < uIndex + uFaceIndexCount; ++u)
			{
				UInt8 const uVertex = GetIndices()[u];
				if (uPlaneOut < aPlaneVertices.GetSize())
				{
					aPlaneVertices[uPlaneOut++] = m_vPoints[uVertex];
				}

				ConvexHullEdge edge;
				edge.m_uFace = uOutFace;
				edge.m_uNext = (uIndex + 1u < uIndex + uFaceIndexCount) ? (UInt8)m_vEdges.GetSize() : uOutEdge;
				edge.m_uOrigin = (UInt8)uVertex;
				edge.m_uTwin = (uIndex == u ? (UInt8)m_vEdges.GetSize() : edge.m_uNext);
				vEdges.PushBack(edge);
			}
			uIndex += uFaceIndexCount;

			vPlanes.PushBack(Plane::CreateFromCorners(aPlaneVertices[0], aPlaneVertices[1], aPlaneVertices[2]));
		}
	}

	m_vEdges.Swap(vEdges);
	m_vFaces.Swap(vFaces);
	m_vPlanes.Swap(vPlanes);
}

Bool ConvexHullShapeData::PostSerialize()
{
	ComputeUtils();
	ComputeCenterOfMass();

	return true;
}

IShapeData* SphereShapeData::Clone() const
{
	auto pReturn = SEOUL_NEW(MemoryBudgets::Physics) SphereShapeData;
	pReturn->m_fRadius = m_fRadius;
	pReturn->m_vCenter = m_vCenter;
	return pReturn;
}

AABB SphereShapeData::ComputeAABB() const
{
	return AABB::CreateFromCenterAndExtents(m_vCenter, Vector3D(m_fRadius));
}

void SphereShapeData::ComputeScaled(const Vector3D& vInScale, SphereShapeData& rOut) const
{
	auto const vScale(SanitizeScale(vInScale));
	rOut.m_fRadius = (m_fRadius * vScale.GetMaxComponent());
	rOut.m_vCenter = Vector3D::ComponentwiseMultiply(vScale, m_vCenter);
}

ShapeDef::ShapeDef()
	: m_fDensity(0.0f)
	, m_fFriction(0.3f)
	, m_fRestitution(0.0f)
	, m_bSensor(false)
	, m_pData()
{
}

ShapeDef::ShapeDef(const ShapeDef& b)
	: m_fDensity(b.m_fDensity)
	, m_fFriction(b.m_fFriction)
	, m_fRestitution(b.m_fRestitution)
	, m_bSensor(b.m_bSensor)
	, m_pData(b.m_pData.IsValid() ? b.m_pData->Clone() : nullptr)
{
}

ShapeDef& ShapeDef::operator=(const ShapeDef& b)
{
	m_fDensity = b.m_fDensity;
	m_fFriction = b.m_fFriction;
	m_fRestitution = b.m_fRestitution;
	m_bSensor = b.m_bSensor;
	m_pData.Reset(b.m_pData.IsValid() ? b.m_pData->Clone() : nullptr);
	return *this;
}

ShapeDef::~ShapeDef()
{
}

AABB ShapeDef::ComputeAABB() const
{
	if (!m_pData.IsValid())
	{
		return AABB::CreateFromCenterAndExtents(Vector3D::Zero(), Vector3D::Zero());
	}

	return m_pData->ComputeAABB();
}

ShapeType ShapeDef::GetType() const
{
	if (!m_pData.IsValid())
	{
		return ShapeType::kNone;
	}
	else
	{
		return m_pData->GetType();
	}
}

void ShapeDef::SetType(ShapeType eType)
{
	// Early out if no change.
	if (GetType() == eType)
	{
		return;
	}

	// Instantiate a new data blob.
	switch (eType)
	{
	case ShapeType::kBox: m_pData.Reset(SEOUL_NEW(MemoryBudgets::Physics) BoxShapeData); break;
	case ShapeType::kCapsule: m_pData.Reset(SEOUL_NEW(MemoryBudgets::Physics) CapsuleShapeData); break;
	case ShapeType::kConvexHull: m_pData.Reset(SEOUL_NEW(MemoryBudgets::Physics) ConvexHullShapeData); break;
	case ShapeType::kSphere: m_pData.Reset(SEOUL_NEW(MemoryBudgets::Physics) SphereShapeData); break;

	case ShapeType::kNone:
	default:
		// No data.
		break;
	};
}

} // namespace Physics

} // namespace Seoul

#endif // /#if SEOUL_WITH_PHYSICS
