/**
 * \file Triangle3D.cpp
 * \brief Struct representing a triangle geometric shape in 3D space.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Triangle3D.h"

namespace Seoul
{

/**
 * @return True if vPoint is exactly within this
 * Triangle3D, false otherwise.
 *
 * \sa Ericson, C. 2005. "Real-Time Collision Detection",
 *     Elsevier, Inc. ISBN: 1-55860-732-3, page 204
 */
Bool Triangle3D::Intersects(const Vector3D& vPoint) const
{
	Vector3D const a = (m_vP0 - vPoint);
	Vector3D const b = (m_vP1 - vPoint);
	Vector3D const c = (m_vP2 - vPoint);

	Vector3D const u = Vector3D::Cross(b, c);
	Vector3D const v = Vector3D::Cross(c, a);
	if (Vector3D::Dot(u, v) < 0.0f) { return false; }

	Vector3D const w = Vector3D::Cross(a, b);
	if (Vector3D::Dot(u, w) < 0.0f) { return false; }

	return true;
}

} // namespace Seoul
