/**
 * \file FalconTesselator.h
 * \brief A set of utility classes for converting shape path data
 * into tesselated triangles for GPU render.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_TESSELATOR_H
#define FALCON_TESSELATOR_H

#include "FalconTypes.h"
#include "Vector.h"

namespace Seoul
{

namespace Falcon
{

// forward declarations
struct ShapeEdge;

class TesselationCallback SEOUL_ABSTRACT
{
public:
	typedef Vector<UInt16, MemoryBudgets::Falcon> Indices;
	typedef Vector<Vector2D, MemoryBudgets::Falcon> LineStrip;
	typedef Vector<Vector2D, MemoryBudgets::Falcon> Vertices;

	TesselationCallback()
	{
	}

	virtual ~TesselationCallback()
	{
	}

	virtual void BeginShape() = 0;

	virtual void AcceptLineStrip(
		const LineStyle& lineStyle,
		const LineStrip& vLineStrip) = 0;
	virtual void AcceptTriangleList(
		const FillStyle& fillStyle,
		const Vertices& vVertices,
		const Indices& vIndices,
		Bool bConvex) = 0;

	virtual void EndShape() = 0;

private:
	SEOUL_DISABLE_COPY(TesselationCallback);
}; // class TesselationCallback

struct TesselationPath SEOUL_SEALED
{
	typedef Vector<Vector2D, MemoryBudgets::Falcon> Points;

	TesselationPath()
		: m_vPoints()
		, m_pFillStyle0(nullptr)
		, m_pFillStyle1(nullptr)
		, m_pLineStyle(nullptr)
	{
	}

	void Swap(TesselationPath& rB)
	{
		m_vPoints.Swap(rB.m_vPoints);
		Seoul::Swap(m_pFillStyle0, rB.m_pFillStyle0);
		Seoul::Swap(m_pFillStyle1, rB.m_pFillStyle1);
		Seoul::Swap(m_pLineStyle, rB.m_pLineStyle);
	}

	Points m_vPoints;
	FillStyle const* m_pFillStyle0;
	FillStyle const* m_pFillStyle1;
	LineStyle const* m_pLineStyle;
}; // struct TesselationPath

class Tesselator SEOUL_SEALED
{
public:
	typedef Vector<TesselationPath, MemoryBudgets::Falcon> Paths;
	typedef Vector<Vector2D, MemoryBudgets::Falcon> Points;

	Tesselator(TesselationCallback& rCallback, Float fPiecewiseLinearApproximationTolerance = 4.5f)
		: m_rCallback(rCallback)
		, m_kfPiecewiseLinearApproximationTolerance(fPiecewiseLinearApproximationTolerance)
		, m_vPaths()
		, m_vLastPoint(Vector2D::Zero())
	{
	}

	~Tesselator()
	{
	}

	void BeginShape();

	void BeginPath(
		FillStyle const* pFillStyle0,
		FillStyle const* pFillStyle1,
		LineStyle const* pLineStyle,
		const Vector2D& vStart)
	{
		m_vPaths.PushBack(TesselationPath());

		TesselationPath& rPath = m_vPaths.Back();
		rPath.m_pFillStyle0 = pFillStyle0;
		rPath.m_pFillStyle1 = pFillStyle1;
		rPath.m_pLineStyle = pLineStyle;

		m_vLastPoint = vStart;
		rPath.m_vPoints.PushBack(vStart);
	}

	void AddCurve(const Vector2D& vLastPoint, const Vector2D& vControlPoint, const Vector2D& vAnchorPoint)
	{
		Vector2D const vLineMidPoint = (vLastPoint + vAnchorPoint) * 0.5f;
		Vector2D const vCurveMidPoint = (vLineMidPoint + vControlPoint) * 0.5f;
		Vector2D const vDifference = (vLineMidPoint - vCurveMidPoint);
		Float const fDistance = Abs(vDifference.X) + Abs(vDifference.Y);

		if (fDistance < m_kfPiecewiseLinearApproximationTolerance)
		{
			AddLine(vAnchorPoint);
		}
		else
		{
			AddCurve(vLastPoint, (vLastPoint + vControlPoint) * 0.5f, vCurveMidPoint);
			AddCurve(vCurveMidPoint, (vControlPoint + vAnchorPoint) * 0.5f, vAnchorPoint);
		}
	}

	void AddEdge(const ShapeEdge& edge)
	{
		if (edge.m_fAnchorX == edge.m_fControlX &&
			edge.m_fAnchorY == edge.m_fControlY)
		{
			AddLine(Vector2D(edge.m_fAnchorX, edge.m_fAnchorY));
		}
		else
		{
			AddCurve(
				m_vLastPoint,
				Vector2D(edge.m_fControlX, edge.m_fControlY),
				Vector2D(edge.m_fAnchorX, edge.m_fAnchorY));
		}
	}

	void AddLine(const Vector2D& vEndPoint)
	{
		SEOUL_ASSERT(!m_vPaths.IsEmpty());

		TesselationPath& rPath = m_vPaths.Back();

		m_vLastPoint = vEndPoint;
		rPath.m_vPoints.PushBack(vEndPoint);
	}

	void EndPath()
	{
		SEOUL_ASSERT(!m_vPaths.IsEmpty());

		const TesselationPath& path = m_vPaths.Back();
		if (nullptr != path.m_pLineStyle && path.m_vPoints.GetSize() > 1)
		{
			m_rCallback.AcceptLineStrip(*path.m_pLineStyle, path.m_vPoints);
		}
	}

	void EndShape();

private:
	TesselationCallback& m_rCallback;
	const Float m_kfPiecewiseLinearApproximationTolerance;

	Paths m_vPaths;
	Vector2D m_vLastPoint;

	SEOUL_DISABLE_COPY(Tesselator);
}; // class Tesselator

} // namespace Falcon

/**
 * Equivalent to std::swap(). Override specifically for TesselationPath.
 */
template <>
inline void Swap<Falcon::TesselationPath>(Falcon::TesselationPath& a, Falcon::TesselationPath& b)
{
	a.Swap(b);
}

} // namespace Seoul

#endif // include guard
