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

#ifndef B3_ISLAND_H
#define B3_ISLAND_H

#include <bounce/common/math/vec3.h>

class b3StackAllocator;
class b3Contact;
class b3Joint;
class b3Body;
struct b3Velocity;
struct b3Position;
struct b3Profile;

class b3Island 
{
public :
	b3Island(b3StackAllocator* stack, u32 bodyCapacity, u32 contactCapacity, u32 jointCapacity);
	~b3Island();

	void Clear();
	
	void Add(b3Body* body);
	void Add(b3Contact* contact);
	void Add(b3Joint* joint);
	
	void Solve(const b3Vec3& gravity, float32 dt, u32 velocityIterations, u32 positionIterations, u32 flags);
private :
	enum b3IslandFlags
	{
		e_warmStartBit = 0x0001,
		e_sleepBit = 0x0002
	};

	friend class b3World;

	b3StackAllocator* m_allocator;
	
	b3Body** m_bodies;
	u32 m_bodyCapacity;
	u32 m_bodyCount;

	b3Contact** m_contacts;
	u32 m_contactCapacity;
	u32 m_contactCount;

	b3Joint** m_joints;
	u32 m_jointCapacity;
	u32 m_jointCount;
	
	b3Position* m_positions;
	b3Velocity* m_velocities;
};

#endif