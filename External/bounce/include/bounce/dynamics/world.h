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

#ifndef B3_WORLD_H
#define B3_WORLD_H

#include <bounce/common/memory/stack_allocator.h>
#include <bounce/common/memory/block_pool.h>
#include <bounce/common/template/list.h>
#include <bounce/dynamics/time_step.h>
#include <bounce/dynamics/joint_manager.h>
#include <bounce/dynamics/contact_manager.h>

struct b3BodyDef;
class b3Body;
class b3QueryListener;
class b3RayCastListener;
class b3ContactListener;
class b3ContactFilter;

struct b3RayCastSingleOutput
{
	b3Shape* shape; // shape
	b3Vec3 point; // intersection point on surface
	b3Vec3 normal; // surface normal of intersection
	float32 fraction; // time of intersection on segment
};

// Use a physics world to create/destroy rigid bodies, execute ray cast and volume queries.
class b3World
{
public:
	b3World();
	~b3World();

	// The filter passed can tell the world to disallow the contact creation between 
	// two shapes.
	void SetContactFilter(b3ContactFilter* filter);

	// The listener passed will be notified when two body shapes begin/stays/ends
	// touching with each other.
	void SetContactListener(b3ContactListener* listener);
	
	// Enable body sleeping. This improves performance.
	void SetSleeping(bool flag);

	// Enable warm-starting for the constraint solvers. This improves stability significantly.
	void SetWarmStart(bool flag);

	// Set the world gravity force.
	void SetGravity(const b3Vec3& gravity);

	// Create a new rigid body.
	b3Body* CreateBody(const b3BodyDef& def);
	
	// Destroy an existing rigid body.
	void DestroyBody(b3Body* body);
	
	// Create a new joint.
	b3Joint* CreateJoint(const b3JointDef& def);

	// Remove a joint from the world and deallocate it from the memory.
	void DestroyJoint(b3Joint* joint);
	 
	// Simulate a physics step.
	// The function parameters are the ammount of time to simulate, 
	// and the number of constraint solver iterations.
	void Step(float32 dt, u32 velocityIterations, u32 positionIterations);
	
	// Perform a ray cast with the world.
	// If the ray doesn't intersect with a shape in the world then return false.
	// The ray cast output is the intercepted shape, the intersection 
	// point in world space, the face normal on the shape associated with the point, 
	// and the intersection fraction.
	bool RayCastSingle(b3RayCastSingleOutput* output, const b3Vec3& p1, const b3Vec3& p2) const;

	// Perform a ray cast with the world.
	// The given ray cast listener will be notified when a ray intersects a shape 
	// in the world. 
	// The ray cast output is the intercepted shape, the intersection 
	// point in world space, the face normal on the shape associated with the point, 
	// and the intersection fraction.
	void RayCast(b3RayCastListener* listener, const b3Vec3& p1, const b3Vec3& p2) const;

	// Perform a AABB query with the world.
	// The query listener will be notified when two shape AABBs are overlapping.
	// If the listener returns false then the query is stopped immediately.
	// Otherwise, it continues searching for new overlapping shape AABBs.
	void QueryAABB(b3QueryListener* listener, const b3AABB3& aabb) const;

	// Get the list of bodies in this world.
	const b3List2<b3Body>& GetBodyList() const;
	b3List2<b3Body>& GetBodyList();

	// Get the list of joints in this world.
	const b3List2<b3Joint>& GetJointList() const;
	b3List2<b3Joint>& GetJointList();

	// Get the list of contacts in this world.
	const b3List2<b3Contact>& GetContactList() const;
	b3List2<b3Contact>& GetContactList();

	// Debug draw the physics entities that belong to this world.
	// The user must implement the debug draw interface b3Draw and b3_debugDraw must have been 
	// set to the user implementation.
	void DebugDraw() const;
	void DrawShape(const b3Transform& xf, const b3Shape* shape) const;
	void DrawJoint(const b3Joint* joint) const;
	void DrawContact(const b3Contact* contact) const;
private :
	enum b3Flags 
	{
		e_shapeAddedFlag = 0x0001,
		e_clearForcesFlag = 0x0002,
	};

	friend class b3Body;
	friend class b3Shape;
	friend class b3Contact;
	friend class b3ConvexContact;
	friend class b3MeshContact;
	friend class b3Joint;

	void Solve(float32 dt, u32 velocityIterations, u32 positionIterations);

	bool m_sleeping;
	bool m_warmStarting;
	u32 m_flags;
	b3Vec3 m_gravity;
	b3Draw* m_debugDraw;

	b3StackAllocator m_stackAllocator;
	b3BlockPool m_bodyBlocks;

	// List of bodies
	b3List2<b3Body> m_bodyList;
	
	// List of joints
	b3JointManager m_jointMan;
	
	// List of contacts
	b3ContactManager m_contactMan;
};

inline void b3World::SetContactListener(b3ContactListener* listener)
{
	m_contactMan.m_contactListener = listener;
}

inline void b3World::SetContactFilter(b3ContactFilter* filter)
{
	m_contactMan.m_contactFilter = filter;
}

inline void b3World::SetGravity(const b3Vec3& gravity)
{
	m_gravity = gravity;
}

inline void b3World::SetWarmStart(bool flag)
{
	m_warmStarting = flag;
}

inline const b3List2<b3Body>& b3World::GetBodyList() const
{
	return m_bodyList;
}

inline b3List2<b3Body>& b3World::GetBodyList()
{
	return m_bodyList;
}

inline const b3List2<b3Joint>& b3World::GetJointList() const
{
	return m_jointMan.m_jointList;
}

inline b3List2<b3Joint>& b3World::GetJointList()
{
	return m_jointMan.m_jointList;
}

inline const b3List2<b3Contact>& b3World::GetContactList() const
{
	return m_contactMan.m_contactList;
}

inline b3List2<b3Contact>& b3World::GetContactList()
{
	return m_contactMan.m_contactList;
}

#endif
