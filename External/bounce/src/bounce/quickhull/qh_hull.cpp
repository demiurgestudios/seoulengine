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

#include <bounce/quickhull/qh_hull.h>
#include <bounce/common/template/stack.h>
#include <bounce/common/draw.h>

float32 qhFindAABB(u32 iMin[3], u32 iMax[3], const b3Array<b3Vec3>& vertices)
{
	b3Vec3 min(B3_MAX_FLOAT, B3_MAX_FLOAT, B3_MAX_FLOAT);
	iMin[0] = 0;
	iMin[1] = 0;
	iMin[2] = 0;
	
	b3Vec3 max(-B3_MAX_FLOAT, -B3_MAX_FLOAT, -B3_MAX_FLOAT);
	iMax[0] = 0;
	iMax[1] = 0;
	iMax[2] = 0;

	for (u32 i = 0; i < vertices.Count(); ++i)
	{
		b3Vec3 p = vertices[i];

		for (u32 j = 0; j < 3; ++j)
		{
			if (p[j] < min[j])
			{
				min[j] = p[j];
				iMin[j] = i;
			}

			if (p[j] > max[j])
			{
				max[j] = p[j];
				iMax[j] = i;
			}
		}
	}

	return 3.0f * (b3Abs(max.x) + b3Abs(max.y) + b3Abs(max.z)) * B3_EPSILON;
}

qhHull::qhHull()
{
}

qhHull::~qhHull()
{
}

void qhHull::Construct(void* memory, const b3Array<b3Vec3>& vs)
{
	u32 V = vs.Count();
	u32 E = 3 * V - 6;
	u32 HE = 2 * E;
	u32 F = 2 * V - 4;

	HE *= 2;
	F *= 2;

	// Euler's formula
	// V - E + F = 2
	
	m_freeVertices = NULL;
	qhVertex* vertices = (qhVertex*)memory;
	for (u32 i = 0; i < V; ++i)
	{
		FreeVertex(vertices + i);
	}

	m_freeEdges = NULL;
	qhHalfEdge* edges = (qhHalfEdge*)((u8*)vertices + V * sizeof(qhVertex));
	for (u32 i = 0; i < HE; ++i)
	{
		FreeEdge(edges + i);
	}
	
	m_freeFaces = NULL;
	qhFace* faces = (qhFace*)((u8*)edges + HE * sizeof(qhHalfEdge));
	for (u32 i = 0; i < F; ++i)
	{
		qhFace* f = faces + i;
		f->conflictList.head = NULL;
		f->conflictList.count = 0;
		FreeFace(f);
	}
		
	m_faceList.head = NULL;
	m_faceList.count = 0;
	m_iteration = 0;

	if (!BuildInitialHull(vs))
	{
		return;
	}

	qhVertex* eye = NextVertex();
	while (eye)
	{
		AddVertex(eye);
		eye = NextVertex();
		++m_iteration;
	}
}

bool qhHull::BuildInitialHull(const b3Array<b3Vec3>& vertices)
{
	if (vertices.Count() < 4)
	{
		B3_ASSERT(false);
		return false;
	}

	u32 i1 = 0, i2 = 0;

	{
		// Find the points that maximizes the distance along the 
		// canonical axes.
		// Store tolerance for coplanarity checks.
		u32 aabbMin[3], aabbMax[3];
		m_tolerance = qhFindAABB(aabbMin, aabbMax, vertices);

		// Find the longest segment.
		float32 d0 = 0.0f;

		for (u32 i = 0; i < 3; ++i)
		{
			b3Vec3 A = vertices[aabbMin[i]];
			b3Vec3 B = vertices[aabbMax[i]];
			
			float32 d = b3DistanceSquared(A, B);

			if (d > d0)
			{
				d0 = d;
				i1 = aabbMin[i];
				i2 = aabbMax[i];
			}
		}

		// Coincident check.
		if (d0 < B3_LINEAR_SLOP * B3_LINEAR_SLOP)
		{
			B3_ASSERT(false);
			return false;
		}
	}

	B3_ASSERT(i1 != i2);

	b3Vec3 A = vertices[i1];
	b3Vec3 B = vertices[i2];

	u32 i3 = 0;

	{
		// Find the triangle which has the largest area.
		float32 a0 = 0.0f;

		for (u32 i = 0; i < vertices.Count(); ++i)
		{
			if (i == i1 || i == i2)
			{
				continue;
			}

			b3Vec3 C = vertices[i];

			float32 a = b3AreaSquared(A, B, C);
			
			if (a > a0)
			{
				a0 = a;
				i3 = i;
			}
		}

		// Colinear check.
		if (a0 < (2.0f * B3_LINEAR_SLOP) * (2.0f * B3_LINEAR_SLOP))
		{
			B3_ASSERT(false);
			return false;
		}
	}

	B3_ASSERT(i3 != i1 && i3 != i2);

	b3Vec3 C = vertices[i3];

	b3Vec3 N = b3Cross(B - A, C - A);
	N.Normalize();

	b3Plane plane(N, A);

	u32 i4 = 0;

	{
		// Find the furthest point from the triangle plane.
		float32 d0 = 0.0f;

		for (u32 i = 0; i < vertices.Count(); ++i)
		{
			if (i == i1 || i == i2 || i == i3)
			{
				continue;
			}

			b3Vec3 D = vertices[i];
			
			float32 d = b3Abs(b3Distance(D, plane));

			if (d > d0)
			{
				d0 = d;
				i4 = i;
			}
		}

		// Coplanar check.
		if (d0 < m_tolerance)
		{
			B3_ASSERT(false);
			return false;
		}
	}

	B3_ASSERT(i4 != i1 && i4 != i2 && i4 != i3);

	// Add okay simplex to the hull.
	b3Vec3 D = vertices[i4];

	qhVertex* v1 = AllocateVertex();
	v1->position = A;

	qhVertex* v2 = AllocateVertex();
	v2->position = B;

	qhVertex* v3 = AllocateVertex();
	v3->position = C;

	qhVertex* v4 = AllocateVertex();
	v4->position = D;

	qhFace* faces[4];

	if (b3Distance(D, plane) < 0.0f)
	{
		faces[0] = AddTriangle(v1, v2, v3);
		faces[1] = AddTriangle(v4, v2, v1);
		faces[2] = AddTriangle(v4, v3, v2);
		faces[3] = AddTriangle(v4, v1, v3);
	}
	else
	{
		// Ensure CCW order.
		faces[0] = AddTriangle(v1, v3, v2);
		faces[1] = AddTriangle(v4, v1, v2);
		faces[2] = AddTriangle(v4, v2, v3);
		faces[3] = AddTriangle(v4, v3, v1);
	}

	// Connectivity check.
	bool ok = IsConsistent();
	B3_ASSERT(ok);
	if (!ok)
	{
		return false;
	}

	// Add remaining points to the hull.
	// Assign closest face plane to each of them.
	for (u32 i = 0; i < vertices.Count(); ++i)
	{
		if (i == i1 || i == i2 || i == i3 || i == i4)
		{
			continue;
		}

		b3Vec3 p = vertices[i];

		// Discard internal points since they can't be in the hull.
		float32 d0 = m_tolerance;
		qhFace* f0 = NULL;

		for (u32 j = 0; j < 4; ++j)
		{
			qhFace* f = faces[j];
			float32 d = b3Distance(p, f->plane);
			if (d > d0)
			{
				d0 = d;
				f0 = f;
			}
		}
		
		if (f0)
		{
			qhVertex* v = AllocateVertex();
			v->position = p;
			v->conflictFace = f0;
			f0->conflictList.PushFront(v);
		}
	}

	return true;
}

qhVertex* qhHull::NextVertex()
{
	// Find the point furthest from the current hull.
	float32 d0 = m_tolerance;
	qhVertex* v0 = NULL;

	qhFace* f = m_faceList.head;
	while (f)
	{
		qhVertex* v = f->conflictList.head;
		while (v)
		{
			float32 d = b3Distance(v->position, f->plane);
			if (d > d0)
			{
				d0 = d;
				v0 = v;
			}

			v = v->next;
		}

		f = f->next;
	}

	return v0;
}

void qhHull::AddVertex(qhVertex* eye)
{
	b3StackArray<qhHalfEdge*, 32> horizon;
	BuildHorizon(horizon, eye);

	b3StackArray<qhFace*, 32> newFaces;
	AddNewFaces(newFaces, eye, horizon);
	
	MergeFaces(newFaces);
}

void qhHull::BuildHorizon(b3Array<qhHalfEdge*>& horizon, qhVertex* eye, qhHalfEdge* edge0, qhFace* face)
{
	// Mark face as visible/visited.
	face->state = qhFace::e_visible;

	qhHalfEdge* edge = edge0;

	do
	{
		qhHalfEdge* adjEdge = edge->twin;
		qhFace* adjFace = adjEdge->face;

		if (adjFace->state == qhFace::e_invisible)
		{
			if (b3Distance(eye->position, adjFace->plane) > m_tolerance)
			{
				BuildHorizon(horizon, eye, adjEdge, adjFace);
			}
			else
			{
				horizon.PushBack(edge);
			}
		}

		edge = edge->next;

	} while (edge != edge0);
}

void qhHull::BuildHorizon(b3Array<qhHalfEdge*>& horizon, qhVertex* eye)
{
	// Clean visited flags
	{
		qhFace* f = m_faceList.head;
		while (f)
		{
			f->state = qhFace::e_invisible;

			qhHalfEdge* e = f->edge;
			do
			{
				e = e->next;
			} while (e != f->edge);
			f = f->next;
		}
	}

	// todo
	// Iterative DFS.
	// Ensure CCW order of horizon edges.
	
	// Build horizon.
	BuildHorizon(horizon, eye, eye->conflictFace->edge, eye->conflictFace);
}

qhFace* qhHull::AddTriangle(qhVertex* v1, qhVertex* v2, qhVertex* v3)
{
	qhFace* face = AllocateFace();

	qhHalfEdge* e1 = AllocateEdge();
	qhHalfEdge* e2 = AllocateEdge();
	qhHalfEdge* e3 = AllocateEdge();

	e1->tail = v1;
	e1->prev = e3;
	e1->next = e2;
	e1->twin = FindTwin(v2, v1);
	if (e1->twin)
	{
		e1->twin->twin = e1;
	}
	e1->face = face;

	e2->tail = v2;
	e2->prev = e1;
	e2->next = e3;
	e2->twin = FindTwin(v3, v2);
	if (e2->twin)
	{
		e2->twin->twin = e2;
	}
	e2->face = face;

	e3->tail = v3;
	e3->prev = e2;
	e3->next = e1;
	e3->twin = FindTwin(v1, v3);
	if (e3->twin)
	{
		e3->twin->twin = e3;
	}
	e3->face = face;

	face->edge = e1;
	face->center = (v1->position + v2->position + v3->position) / 3.0f;
	face->plane = b3Plane(v1->position, v2->position, v3->position);
	face->state = qhFace::e_invisible;

	m_faceList.PushFront(face);

	return face;
}

qhHalfEdge* qhHull::AddAdjoiningTriangle(qhVertex* eye, qhHalfEdge* horizonEdge)
{
	B3_ASSERT(horizonEdge->face->state == qhFace::e_visible);

	qhFace* face = AllocateFace();

	qhVertex* v1 = eye;
	qhVertex* v2 = horizonEdge->tail;
	qhVertex* v3 = horizonEdge->twin->tail;

	qhHalfEdge* e1 = AllocateEdge();
	qhHalfEdge* e2 = AllocateEdge();
	qhHalfEdge* e3 = AllocateEdge();
	
	e1->tail = v1;
	e1->prev = e3;
	e1->next = e2;
	e1->twin = NULL;
	e1->face = face;

	e2->tail = v2;
	e2->prev = e1;
	e2->next = e3;
	e2->twin = horizonEdge->twin;
	horizonEdge->twin->twin = e2;
	e2->face = face;

	e3->tail = v3;
	e3->prev = e2;
	e3->next = e1;
	e3->twin = NULL;
	e3->face = face;

	horizonEdge->twin = NULL;

	face->edge = e1;
	face->center = (v1->position + v2->position + v3->position) / 3.0f;
	face->plane = b3Plane(v1->position, v2->position, v3->position);
	face->state = qhFace::e_invisible;

	m_faceList.PushFront(face);

	return e1;
}

void qhHull::AddNewFaces(b3Array<qhFace*>& newFaces, qhVertex* eye, const b3Array<qhHalfEdge*>& horizon)
{
	newFaces.Reserve(horizon.Count());

	qhHalfEdge* beginEdge = NULL;
	qhHalfEdge* prevEdge = NULL;

	{
		qhHalfEdge* edge = horizon[0];
		qhHalfEdge* leftEdge = AddAdjoiningTriangle(eye, edge);
		qhHalfEdge* rightEdge = leftEdge->prev;

		prevEdge = rightEdge;

		beginEdge = leftEdge;

		newFaces.PushBack(leftEdge->face);
	}

	for (u32 i = 1; i < horizon.Count() - 1; ++i)
	{
		qhHalfEdge* edge = horizon[i];
		qhHalfEdge* leftEdge = AddAdjoiningTriangle(eye, edge);
		qhHalfEdge* rightEdge = leftEdge->prev;

		leftEdge->twin = prevEdge;
		prevEdge->twin = leftEdge;

		prevEdge = rightEdge;

		newFaces.PushBack(leftEdge->face);
	}

	{
		qhHalfEdge* edge = horizon[horizon.Count() - 1];
		qhHalfEdge* leftEdge = AddAdjoiningTriangle(eye, edge);
		qhHalfEdge* rightEdge = leftEdge->prev;

		leftEdge->twin = prevEdge;
		prevEdge->twin = leftEdge;

		rightEdge->twin = beginEdge;
		beginEdge->twin = rightEdge;

		newFaces.PushBack(leftEdge->face);
	}

	qhFace* f = m_faceList.head;
	while (f)
	{
		if (f->state == qhFace::e_invisible)
		{
			f = f->next;
			continue;
		}

		// Partition conflict vertices.
		qhVertex* v = f->conflictList.head;
		while (v)
		{
			b3Vec3 p = v->position;

			// Use tolerance and discard internal points.
			float32 max = m_tolerance;
			qhFace* iMax = NULL;

			for (u32 i = 0; i < newFaces.Count(); ++i)
			{
				qhFace* newFace = newFaces[i];
				float32 d = b3Distance(p, newFace->plane);
				if (d > max)
				{
					max = d;
					iMax = newFace;
				}
			}

			if (iMax)
			{
				qhVertex* v0 = v;
				v->conflictFace = NULL;
				v = f->conflictList.Remove(v);
				iMax->conflictList.PushFront(v0);
				v0->conflictFace = iMax;
			}
			else
			{
				qhVertex* v0 = v;
				v->conflictFace = NULL;
				v = f->conflictList.Remove(v);
				FreeVertex(v0);
			}
		}

		// Remove face half-edges.
		qhHalfEdge* e = f->edge;
		do
		{
			qhHalfEdge* e0 = e;
			e = e->next;
			FreeEdge(e0);
		} while (e != f->edge);

		// Remove face.
		qhFace* f0 = f;
		f = m_faceList.Remove(f);
		FreeFace(f0);
	}
}

bool qhHull::MergeFace(qhFace* rightFace)
{
	qhHalfEdge* e = rightFace->edge;

	do
	{
		qhFace* leftFace = e->twin->face;

		float32 d1 = b3Distance(leftFace->center, rightFace->plane);
		float32 d2 = b3Distance(rightFace->center, leftFace->plane);

		if (d1 < -m_tolerance && d2 < -m_tolerance)
		{
			// convex
			e = e->next;
		}
		else
		{
			// concave or coplanar
			if (leftFace == rightFace)
			{
				e = e->next;
				continue;
			}

			// Move left vertices into right
			qhVertex* v = leftFace->conflictList.head;
			while (v)
			{
				qhVertex* v0 = v;
				v = leftFace->conflictList.Remove(v);
				rightFace->conflictList.PushFront(v0);
				v0->conflictFace = rightFace;
			}
			
			// set right face to reference a non-deleted edge
			B3_ASSERT(e->face == rightFace);
			rightFace->edge = e->prev;
			
			// absorb face
			qhHalfEdge* te = e->twin;
			do
			{
				te->face = rightFace;
				te = te->next;
			} while (te != e->twin);

			// link edges
			e->prev->next = e->twin->next;
			e->next->prev = e->twin->prev;
			e->twin->prev->next = e->next;
			e->twin->next->prev = e->prev;

			FreeEdge(e->twin);
			FreeEdge(e);
			m_faceList.Remove(leftFace);
			FreeFace(leftFace);
			
			rightFace->ComputeCenterAndPlane();

			{
				qhHalfEdge* he = rightFace->edge;
				do
				{
					B3_ASSERT(he->face == rightFace);
					B3_ASSERT(he->twin->twin == he);
					he = he->next;
				} while (he != rightFace->edge);
			}

			return true;
		}

	} while (e != rightFace->edge);

	return false;
}

void qhHull::MergeFaces(b3Array<qhFace*>& newFaces)
{
	for (u32 i = 0; i < newFaces.Count(); ++i)
	{
		qhFace* f = newFaces[i];
		
		if (f->state == qhFace::e_deleted) 
		{
			continue;
		}

		// todo
		// Fix topological and geometrical errors.

		// Merge the faces while there is no face left 
		// to merge.
		while (MergeFace(f));
	}
}

bool qhHull::IsConsistent() const
{
	u32 count = 0;

	qhFace* f = m_faceList.head;
	while (f)
	{
		B3_ASSERT(f->state != qhFace::e_deleted);
		qhHalfEdge* e = f->edge;
		do
		{
			++count;
			//B3_ASSERT(e->face == f);
			B3_ASSERT(e->twin->twin == e);
			B3_ASSERT(count < 10000);
			e = e->next;
		} while (e != f->edge);

		f = f->next;
	}

	return true;
}

void qhHull::Draw(b3Draw* draw) const
{
	qhFace* face = m_faceList.head;
	while (face)
	{
		b3Vec3 c = face->center;
		b3Vec3 n = face->plane.normal;

		b3StackArray<b3Vec3, 32> vs;
		
		const qhHalfEdge* begin = face->edge;
		const qhHalfEdge* edge = begin;
		do
		{
			vs.PushBack(edge->tail->position);
			edge = edge->next;
		} while (edge != begin);

		draw->DrawSolidPolygon(n, vs.Begin(), vs.Count(), b3Color(1.0f, 1.0f, 1.0f, 0.5f));

		qhVertex* v = face->conflictList.head;
		while (v)
		{
			draw->DrawPoint(v->position, 4.0f, b3Color(1.0f, 1.0f, 0.0f));
			draw->DrawSegment(c, v->position, b3Color(1.0f, 1.0f, 0.0f));
			v = v->next;
		}

		draw->DrawSegment(c, c + n, b3Color(1.0f, 1.0f, 1.0f));

		face = face->next;
	}
}