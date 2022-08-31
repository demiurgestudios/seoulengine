/*
* Copyright (c) 2016-2016 Irlan Robson http://www.irlan.net
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the authors be held liable for any damages
* arising from the use of this software.
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*/

#ifndef B3_CLIP_H
#define B3_CLIP_H

#include <bounce/common/template/array.h>
#include <bounce/common/geometry.h>

#define B3_NULL_EDGE (0xFF)

// A combination of features used to uniquely identify a vertex on a feature.
struct b3FeaturePair
{
	u8 inEdge1; // incoming edge on hull 1
	u8 inEdge2; // incoming edge on hull 2
	u8 outEdge1; // outgoing edge on hull 1
	u8 outEdge2; // outgoing edge on hull 2
};

inline b3FeaturePair b3MakePair(u32 inEdge1, u32 inEdge2, u32 outEdge1, u32 outEdge2)
{
	b3FeaturePair out;
	out.inEdge1 = u8(inEdge1);
	out.inEdge2 = u8(inEdge2);
	out.outEdge1 = u8(outEdge1);
	out.outEdge2 = u8(outEdge2);
	return out;
}

// Make a 32-bit key for a feature pair.
inline u32 b3MakeKey(const b3FeaturePair& featurePair)
{
	union b3FeaturePairKey
	{
		b3FeaturePair pair;
		u32 key;
	};

	b3FeaturePairKey key;
	key.pair = featurePair;
	return key.key;
}

// A clip vertex.
struct b3ClipVertex
{
	b3Vec3 position;
	b3FeaturePair pair; // the features that built the clip point
};

// A clip polygon.
typedef b3Array<b3ClipVertex> b3ClipPolygon;

// A clip plane.
struct b3ClipPlane
{
	b3Plane plane;
	u32 id;
};

struct b3Hull;
struct b3Segment;

// Build a clip edge for an edge.
void b3BuildEdge(b3ClipVertex vOut[2],
	const b3Segment* hull);

// Build a clip polygon given an index to the polygon face.
void b3BuildPolygon(b3ClipPolygon& pOut,
	const b3Transform& xf, u32 index, const b3Hull* hull);

// Clip a segment by a plane. 
// Output a segment whose points are behind or on the input plane.
// Return the number of output points.
u32 b3ClipEdgeToPlane(b3ClipVertex vOut[2],
	const b3ClipVertex vIn[2], const b3ClipPlane& plane);

// Clip a polygon by a plane.  
// Output a polygon whose points are behind or on the input plane.
void b3ClipPolygonToPlane(b3ClipPolygon& pOut,
	const b3ClipPolygon& pIn, const b3ClipPlane& plane);

// Clip a segment by a hull face (side planes).
// Return the number of output points.
u32 b3ClipEdgeToFace(b3ClipVertex vOut[2],
	const b3ClipVertex vIn[2], const b3Segment* hull);

// Clip a segment by a hull face (side planes).
// Return the number of output points.
u32 b3ClipEdgeToFace(b3ClipVertex vOut[2],
	const b3ClipVertex vIn[2], const b3Transform& xf, float32 r, u32 index, const b3Hull* hull);

// Clip a polygon by a hull face (side planes).
void b3ClipPolygonToFace(b3ClipPolygon& pOut,
	const b3ClipPolygon& pIn, const b3Transform& xf, float32 r, u32 index, const b3Hull* hull);

#endif
