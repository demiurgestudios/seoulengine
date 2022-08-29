/**
 * \file PhysicsShapeDef.h
 * \brief Shareable data of shape instances. Shape define collision
 * properties of a physics body.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef PHYSICS_SHAPE_DEF_H
#define PHYSICS_SHAPE_DEF_H

#include "Geometry.h"
#include "PhysicsShapeType.h"
#include "Prereqs.h"
#include "ReflectionDeclare.h"
#include "SharedPtr.h"
#include "Vector.h"
#include "Vector3D.h"

#if SEOUL_WITH_PHYSICS

namespace Seoul::Physics
{

/** Absolute minimum magnitude of shape scaling (1 mm). */
static const Float32 kfMinShapeScaleMag = 1e-3f;

class IShapeData SEOUL_ABSTRACT
{
public:
	SEOUL_REFLECTION_POLYMORPHIC_BASE(IShapeData);

	/** Shared utility function to sanitize shape scaling. */
	static inline Vector3D SanitizeScale(const Vector3D& vScale)
	{
		// Must be positive and greater than kfMinScaleMag.
		return Vector3D::Max(vScale.Abs(), Vector3D(kfMinShapeScaleMag));
	}

	virtual ~IShapeData()
	{
	}

	virtual IShapeData* Clone() const = 0;
	virtual AABB ComputeAABB() const = 0;
	virtual ShapeType GetType() const = 0;

protected:
	SEOUL_REFERENCE_COUNTED(IShapeData);

	IShapeData()
	{
	}

private:
	SEOUL_DISABLE_COPY(IShapeData);
}; // class IShapeData

class BoxShapeData SEOUL_SEALED : public IShapeData
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(BoxShapeData);

	static const ShapeType StaticType = ShapeType::kBox;

	BoxShapeData()
		: m_vCenter(Vector3D::Zero())
		, m_vExtents(Vector3D::Zero())
	{
	}

	~BoxShapeData()
	{
	}

	virtual IShapeData* Clone() const SEOUL_OVERRIDE;
	virtual AABB ComputeAABB() const SEOUL_OVERRIDE;
	virtual ShapeType GetType() const SEOUL_OVERRIDE { return StaticType; }

	// Apply a local scale to this shape and populate the output
	// shape with the result.
	void ComputeScaled(const Vector3D& vInScale, BoxShapeData& rOut) const;

	Vector3D m_vCenter;
	Vector3D m_vExtents;

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(BoxShapeData);
	SEOUL_DISABLE_COPY(BoxShapeData);
}; // class BoxShapeData

class CapsuleShapeData SEOUL_SEALED : public IShapeData
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(CapsuleShapeData);

	static const ShapeType StaticType = ShapeType::kCapsule;

	CapsuleShapeData()
		: m_vP0(Vector3D::Zero())
		, m_vP1(Vector3D::Zero())
		, m_fRadius(0.0f)
	{
	}

	~CapsuleShapeData()
	{
	}

	virtual IShapeData* Clone() const SEOUL_OVERRIDE;
	virtual AABB ComputeAABB() const SEOUL_OVERRIDE;
	virtual ShapeType GetType() const SEOUL_OVERRIDE { return StaticType; }

	// Apply a local scale to this shape and populate the output
	// shape with the result.
	void ComputeScaled(const Vector3D& vInScale, CapsuleShapeData& rOut) const;

	Vector3D m_vP0;
	Vector3D m_vP1;
	Float m_fRadius;

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(CapsuleShapeData);
	SEOUL_DISABLE_COPY(CapsuleShapeData);
}; // class CapsuleShapeData

struct ConvexHullEdge SEOUL_SEALED
{
	UInt8 m_uOrigin{};
	UInt8 m_uTwin{};
	UInt8 m_uFace{};
	UInt8 m_uNext{};
};

class ConvexHullShapeData SEOUL_SEALED : public IShapeData
{
public:
	typedef Vector<ConvexHullEdge, MemoryBudgets::Physics> Edges;
	typedef Vector<UInt8, MemoryBudgets::Physics> Faces;
	typedef Vector<UInt8, MemoryBudgets::Physics> Indices;
	typedef Vector<Plane, MemoryBudgets::Physics> Planes;
	typedef Vector<Vector3D, MemoryBudgets::Physics> Points;

	SEOUL_REFLECTION_POLYMORPHIC(ConvexHullShapeData);

	static const ShapeType StaticType = ShapeType::kConvexHull;

	ConvexHullShapeData();
	~ConvexHullShapeData();

	/**
	 * Recompute this ConvexHullShapeData by generating a best fit convex hull for the given points
	 * and constraints.
	 */
	void CalculateFromPoints(Vector3D const* pBegin, Vector3D const* pEnd);

	virtual IShapeData* Clone() const SEOUL_OVERRIDE;
	virtual AABB ComputeAABB() const SEOUL_OVERRIDE;
	virtual ShapeType GetType() const SEOUL_OVERRIDE { return StaticType; }

	// Apply a local scale to this shape and populate the output
	// shape with the result.
	void ComputeScaled(const Vector3D& vInScale, ConvexHullShapeData& rOut) const;

	/** @return The center of mass of all points of the convex hull. */
	const Vector3D& GetCenterOfMass() const { return m_vCenterOfMass; }

	/** @return The list of convex face edges. */
	const Edges& GetEdges() const { return m_vEdges; }

	/** @return The list of convex faces. */
	const Faces& GetFaces() const { return m_vFaces; }

	/** @return The indices of the convex hull. */
	const Indices& GetIndices() const { return m_vIndices; }

	/** @return Plane data of convex faces. */
	const Planes& GetPlanes() const { return m_vPlanes; }

	/** @return The vertices of the convex hull. */
	const Points& GetPoints() const { return m_vPoints; }

private:
	SEOUL_REFLECTION_FRIENDSHIP(ConvexHullShapeData);

	Edges m_vEdges;
	Faces m_vFaces;
	Indices m_vIndices; // TODO: Eliminate, this data is redundant and not needed at runtime.
	Planes m_vPlanes;
	Points m_vPoints;
	Vector3D m_vCenterOfMass;

	void ComputeCenterOfMass();
	void ComputeUtils();
	Bool PostSerialize();

	SEOUL_REFERENCE_COUNTED_SUBCLASS(ConvexHullShapeData);
	SEOUL_DISABLE_COPY(ConvexHullShapeData);
}; // class ConvexHullShapeData

class SphereShapeData SEOUL_SEALED : public IShapeData
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(SphereShapeData);

	static const ShapeType StaticType = ShapeType::kSphere;

	SphereShapeData()
		: m_vCenter(Vector3D::Zero())
		, m_fRadius(0.0f)
	{
	}

	~SphereShapeData()
	{
	}

	virtual IShapeData* Clone() const SEOUL_OVERRIDE;
	virtual AABB ComputeAABB() const SEOUL_OVERRIDE;
	virtual ShapeType GetType() const SEOUL_OVERRIDE { return StaticType; }

	Vector3D m_vCenter;
	Float m_fRadius;

	// Apply a local scale to this shape and populate the output
	// shape with the result.
	void ComputeScaled(const Vector3D& vInScale, SphereShapeData& rOut) const;

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(SphereShapeData);
	SEOUL_DISABLE_COPY(SphereShapeData);
}; // class SphereShapeData

class ShapeDef SEOUL_SEALED
{
public:
	ShapeDef();
	ShapeDef(const ShapeDef&);
	~ShapeDef();

	ShapeDef& operator=(const ShapeDef&);

	AABB ComputeAABB() const;

	template <typename T>
	T* GetData()
	{
		if (T::StaticType == GetType())
		{
			return static_cast<T*>(m_pData.GetPtr());
		}
		else
		{
			return nullptr;
		}
	}

	template <typename T>
	T const* GetData() const
	{
		return const_cast<ShapeDef*>(this)->GetData<T>();
	}

	ShapeType GetType() const;
	void SetType(ShapeType eType);

	Float m_fDensity;
	Float m_fFriction;
	Float m_fRestitution;
	Bool m_bSensor;

private:
	SEOUL_REFLECTION_FRIENDSHIP(ShapeDef);

	SharedPtr<IShapeData> m_pData;
}; // class ShapeDef

} // namespace Seoul::Physics

#endif // /#if SEOUL_WITH_PHYSICS

#endif // include guard
