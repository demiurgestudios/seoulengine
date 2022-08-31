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

#ifndef B3_VERTEX_HULL_SAT_H
#define B3_VERTEX_HULL_SAT_H

#include <bounce/collision/sat/sat.h>

///////////////////////////////////////////////////////////////////////////////////////////////////

float32 b3ProjectVertex(const b3Vec3& hull, const b3Plane& plane);

b3FaceQuery b3QueryFaceSeparation(const b3Transform& xf1, const b3Vec3& hull1,
	const b3Transform& xf2, const b3Hull* hull2);

#endif
