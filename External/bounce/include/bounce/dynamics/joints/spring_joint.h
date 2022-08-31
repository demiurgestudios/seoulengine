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

#ifndef B3_SPRING_JOINT_H
#define B3_SPRING_JOINT_H

#include <bounce/dynamics/joints/joint.h>

struct b3SpringJointDef : public b3JointDef 
{
	b3SpringJointDef()
	{
		type = e_springJoint;
		localAnchorA.SetZero();
		localAnchorB.SetZero();
		length = 0.0f;
		frequencyHz = 0.0f;
		dampingRatio = 0.0f;
	}

	// Initialize this definition from bodies and world anchors.
	void Initialize(b3Body* bodyA, b3Body* bodyB, const b3Vec3& anchorA, const b3Vec3& anchorB);

	// The anchor point relative to body A's origin
	b3Vec3 localAnchorA;

	// The anchor point relative to body B's origin
	b3Vec3 localAnchorB;
	
	// The spring rest length.
	float32 length;
	
	// The mass-spring-damper frequency in Hz
	// 0 = disable softness
	float32 frequencyHz;
	
	// The damping ration in the interval [0, 1]
	// 0 = undamped spring
	// 1 = critical damping
	float32 dampingRatio;
};

// A spring joint constrains the bodies to rotate relative to each 
// other about a specified spring position. 
// The tunable soft parameters control how much/fast the bodies should translate 
// relative to each other about the spring position. 
// This joint can be used to create behaviours such as a car suspension.
class b3SpringJoint : public b3Joint 
{
public:	
	// Get the anchor point on body A in world coordinates.
	b3Vec3 GetAnchorA() const;

	// Get the anchor point on body B in world coordinates.
	b3Vec3 GetAnchorB() const;
	
	// Get the anchor point relative to body A's origin.
	const b3Vec3& GetLocalAnchorA() const;

	// Get the anchor point relative to body B's origin.
	const b3Vec3& GetLocalAnchorB() const;

	// Get the natural spring length.
	float32 GetLength() const;
	
	// Set the natural spring length.
	void SetLength(float32 length);
	
	// Get the damper frequency in Hz.
	float32 GetFrequency() const;

	// Set the damper frequency in Hz.
	void SetFrequency(float32 frequency);

	// Get the damping ratio.
	float32 GetDampingRatio() const;
	
	// Set the damping ratio.
	void SetDampingRatio(float32 ratio);

	// Draw this joint.
	void Draw(b3Draw* draw) const;
private:
	friend class b3Joint;
	friend class b3JointManager;
	friend class b3JointSolver;

	b3SpringJoint(const b3SpringJointDef* def);

	void InitializeConstraints(const b3SolverData* data);
	void WarmStart(const b3SolverData* data);
	void SolveVelocityConstraints(const b3SolverData* data);
	bool SolvePositionConstraints(const b3SolverData* data);

	// Solver shared
	b3Vec3 m_localAnchorA;
	b3Vec3 m_localAnchorB;
	float32 m_length;
	float32 m_frequencyHz;
	float32 m_dampingRatio;

	// Solver temp
	u32 m_indexA;
	u32 m_indexB;
	float32 m_mA;
	float32 m_mB;
	b3Mat33 m_iA;
	b3Mat33 m_iB;
	b3Vec3 m_localCenterA;
	b3Vec3 m_localCenterB;

	float32 m_bias;
	float32 m_gamma;
	b3Vec3 m_n;
	b3Vec3 m_rA;
	b3Vec3 m_rB;
	float32 m_mass;
	float32 m_impulse;
};

#endif