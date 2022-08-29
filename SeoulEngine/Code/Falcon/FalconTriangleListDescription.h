/**
 * \file FalconTriangleListDescription.h
 * \brief Provides specificity to an otherwise arbitrary
 * bucket of renderable triangle data.
 *
 * The description typically enables early out optimizations
 * or reduced processing overhead in the render backend.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_TRIANGLE_LIST_DESCRIPTION_H
#define FALCON_TRIANGLE_LIST_DESCRIPTION_H

namespace Seoul::Falcon
{

enum class TriangleListDescription
{
	// No specific properties, can only be treated as an indexed list of triangles.
	kNotSpecific,

	// Triangle list is a list of quads - assumed format is 4 vertices per 6 indices, where
	// each set of 4 can be treated as a quad/convex shape, with indices (0, 1, 2), (0, 2, 3)
	// (quad setup using triangle fans). Triangles are expected to be counter clockwise.
	kQuadList,

	// Triangle list describes a single, convex shape. The assumed property is that the index
	// list can be regenerated for the list of vertices using triangle fans, so the vertices
	// should be ordered counter clockwise around the convex shape.
	kConvex,

	// Triangle list is a list of quads but also a text chunk. This is identical to a kQuadList,
	// except it is also assumed to be in a single horizontal row (in the text's local space).
	// This means the bounds of the chunk can be quickly computed by examining the first 2
	// and last 2 vertices of the entire draw operation.
	kTextChunk,
};

} // namespace Seoul::Falcon

#endif // include guard
