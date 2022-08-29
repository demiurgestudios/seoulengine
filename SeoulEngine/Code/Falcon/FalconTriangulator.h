/**
 * \file FalconTriangulator.h
 * \brief Wraps the TESS triangulation/tesselation library,
 * used by Falcon::Tesselator to generated 2D triangle lists
 * from shape paths.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_TRIANGULATOR_H
#define FALCON_TRIANGULATOR_H

#include "FalconTypes.h"
#include "Vector.h"

namespace Seoul::Falcon
{

// Forward declarations
struct TesselationPath;

namespace Triangulator
{
	typedef Vector<UInt16, MemoryBudgets::Falcon> Indices;
	typedef Vector<TesselationPath, MemoryBudgets::Falcon> Paths;
	typedef Vector<Vector2D, MemoryBudgets::Falcon> Vertices;

	void Finalize(
		Paths::ConstIterator iBegin,
		Paths::ConstIterator iEnd,
		Vertices& rvVertices,
		Indices& rvIndices,
		Bool& rbConvex);

	Bool Triangulate(
		Paths::ConstIterator iBegin,
		Paths::ConstIterator iEnd,
		Vertices& rvVertices,
		Indices& rvIndices);
} // namespace Triangulator

} // namespace Seoul::Falcon

#endif // include guard
