/**
 * \file FalconTesselator.cpp
 * \brief A set of utility classes for converting shape path data
 * into tesselated triangles for GPU render.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Algorithms.h"
#include "FalconTesselator.h"
#include "FalconTriangulator.h"

namespace Seoul::Falcon
{

// TODO: Move tesselation into FalconCooker.

struct TesselationSorter
{
	Bool operator()(const TesselationPath& a, const TesselationPath& b)
	{
		return (a.m_pFillStyle1 < b.m_pFillStyle1);
	}
}; // struct TesselationSorter

void Tesselator::BeginShape()
{
	// paths must be empty.
	SEOUL_ASSERT(m_vPaths.IsEmpty());

	m_vPaths.Clear();
	m_vLastPoint = Vector2D::Zero();
}

void Tesselator::EndShape()
{
	// clean - all paths that we will process must have at least 1 defined fill shape
	// and a non-zero number of points.
	{
		Int32 iPaths = (Int32)m_vPaths.GetSize();
		for (Int32 iPath = 0; iPath < iPaths; ++iPath)
		{
			const TesselationPath& path = m_vPaths[iPath];
			if ((nullptr == path.m_pFillStyle0 && nullptr == path.m_pFillStyle1) ||
				path.m_vPoints.IsEmpty())
			{
				Swap(m_vPaths[iPath], m_vPaths.Back());
				m_vPaths.PopBack();
				--iPath;
				--iPaths;
			}
		}
	}

	// done if no paths left after cleaning.
	if (m_vPaths.IsEmpty())
	{
		m_rCallback.EndShape();
		m_vLastPoint = Vector2D::Zero();
		return;
	}

	// update or add paths as needed so all paths have only fill style 1.
	{
		Paths::SizeType const zPaths = m_vPaths.GetSize();
		for (Paths::SizeType iPath = 0; iPath < zPaths; ++iPath)
		{
			TesselationPath& rPath = m_vPaths[iPath];
			FillStyle const* pFillStyle0 = rPath.m_pFillStyle0;
			FillStyle const* pFillStyle1 = rPath.m_pFillStyle1;
			LineStyle const* pLineStyle = rPath.m_pLineStyle;
			rPath.m_pFillStyle0 = nullptr;

			if (nullptr != pFillStyle0)
			{
				// reverse existing path.
				if (nullptr == pFillStyle1)
				{
					// fill style 1 is now fill style 0.
					rPath.m_pFillStyle1 = pFillStyle0;

					// reverse points.
					Reverse(rPath.m_vPoints.Begin(), rPath.m_vPoints.End());
				}
				// generate new path.
				else
				{
					TesselationPath newPath;
					newPath.m_pFillStyle1 = pFillStyle0;
					newPath.m_pLineStyle = pLineStyle;
					newPath.m_vPoints = rPath.m_vPoints;
					Reverse(newPath.m_vPoints.Begin(), newPath.m_vPoints.End());

					// modify last so rPath remains valid.
					m_vPaths.PushBack(newPath);
				}
			}
		}
	}

	// merge and close.
	Paths vClosedPaths;
	{
		vClosedPaths.Reserve(m_vPaths.GetSize());

		// until we've merged all paths.
		while (!m_vPaths.IsEmpty())
		{
			// merge over any closed paths.
			{
				Int32 iPaths = (Int32)m_vPaths.GetSize();
				for (Int32 iPath = 0; iPath < iPaths; ++iPath)
				{
					// empty, just remove.
					if (m_vPaths[iPath].m_vPoints.IsEmpty())
					{
						Swap(m_vPaths[iPath], m_vPaths[iPaths - 1]);
						--iPath;
						--iPaths;
					}
					else if (m_vPaths[iPath].m_vPoints.Front() == m_vPaths[iPath].m_vPoints.Back())
					{
						// TODO: Getting some degenerate closed paths with only 3 vertices.
						// Determine if this is correct Flash data (maybe line only shapes?) or
						// a bug in the tesselator.
						if (m_vPaths[iPath].m_vPoints.GetSize() > 3)
						{
							vClosedPaths.PushBack(m_vPaths[iPath]);
							vClosedPaths.Back().m_vPoints.PopBack();
						}

						Swap(m_vPaths[iPath], m_vPaths[iPaths - 1]);
						--iPath;
						--iPaths;
					}
				}
				m_vPaths.Resize(iPaths);
			}

			// try to close the first path.
			if (!m_vPaths.IsEmpty())
			{
				Bool bMergedOne = false;

				TesselationPath& rTarget = m_vPaths[0];
				Paths::SizeType const zPaths = m_vPaths.GetSize();
				for (Paths::SizeType iSource = 1; iSource < zPaths; ++iSource)
				{
					TesselationPath& rSource = m_vPaths[iSource];
					if (rSource.m_pFillStyle1 != rTarget.m_pFillStyle1)
					{
						continue;
					}

					if (rTarget.m_vPoints.Back() == rSource.m_vPoints.Front())
					{
						// merge all but the first vertex.
						Points::SizeType const zPoints = rSource.m_vPoints.GetSize();
						for (Points::SizeType iPoint = 1; iPoint < zPoints; ++iPoint)
						{
							rTarget.m_vPoints.PushBack(rSource.m_vPoints[iPoint]);
						}

						// clear points.
						rSource.m_vPoints.Clear();

						bMergedOne = true;
					}
				}

				// TODO: We're getting an edge that cannot be closed which
				// is just 2 vertices. It has a fill style but no line style, which is
				// unexpected. Need to determine if this is a tag evaluation bug or something else.

				// If we didn't merge anything into the target, just discard it.
				if (!bMergedOne)
				{
					Swap(m_vPaths.Front(), m_vPaths.Back());
					m_vPaths.PopBack();
				}
			}
		}
	}

	// sort by fill style.
	{
		TesselationSorter sorter;
		QuickSort(vClosedPaths.Begin(), vClosedPaths.End(), sorter);
	}

	// TODO: Not sure if these are generated by Flash, or are being generated in
	// bugged loading/tesselation code in Falcon.

	// eliminate unnecessary vertices.
	{
		Paths::SizeType const zPaths = vClosedPaths.GetSize();
		for (Paths::SizeType iPath = 0; iPath < zPaths; ++iPath)
		{
			TesselationPath& rPath = vClosedPaths[iPath];
			for (Points::SizeType iPoint = 0; iPoint < rPath.m_vPoints.GetSize(); )
			{
				Points::SizeType const i0 = iPoint;
				Points::SizeType const i1 = (i0 + 1 < rPath.m_vPoints.GetSize() ? i0 + 1 : 0);
				Points::SizeType const i2 = (i1 + 1 < rPath.m_vPoints.GetSize() ? i1 + 1 : 0);

				Vector2D const v0(rPath.m_vPoints[i0]);
				Vector2D const v1(rPath.m_vPoints[i1]);
				Vector2D const v2(rPath.m_vPoints[i2]);
				Float const fCross(Vector2D::Cross(Vector2D::Normalize(v1 - v0), Vector2D::Normalize(v2 - v1)));

				if (IsZero(fCross))
				{
					rPath.m_vPoints.Erase(rPath.m_vPoints.Begin() + i1);
					if (i1 < iPoint)
					{
						--iPoint;
					}
				}
				else
				{
					++iPoint;
				}
			}
		}
	}


	// triangulate.
	{
		auto const iBegin = vClosedPaths.Begin();
		auto const iEnd = vClosedPaths.End();
		TesselationCallback::Indices vIndices;
		TesselationCallback::Vertices vVertices;
		for (auto iShapeBegin = iBegin; iEnd != iShapeBegin; )
		{
			const TesselationPath& firstPath = *iShapeBegin;

			auto iShapeEnd = iShapeBegin;
			for (; iEnd != iShapeEnd; ++iShapeEnd)
			{
				if (firstPath.m_pFillStyle1 != iShapeEnd->m_pFillStyle1)
				{
					break;
				}
			}

			Bool const bSuccessful = Triangulator::Triangulate(
				iShapeBegin,
				iShapeEnd,
				vVertices,
				vIndices);

			// Triangulation should never fail.
			SEOUL_ASSERT(bSuccessful);

			if (bSuccessful && !vIndices.IsEmpty())
			{
				Bool bConvex = false;
				Triangulator::Finalize(
					iShapeBegin,
					iShapeEnd,
					vVertices,
					vIndices,
					bConvex);

				m_rCallback.AcceptTriangleList(
					*firstPath.m_pFillStyle1,
					vVertices,
					vIndices, 
					bConvex);
			}

			vIndices.Clear();
			vVertices.Clear();
			iShapeBegin = iShapeEnd;
		}
	}

	m_rCallback.EndShape();
	m_vPaths.Clear();
	m_vLastPoint = Vector2D::Zero();
}

} // namespace Seoul::Falcon
