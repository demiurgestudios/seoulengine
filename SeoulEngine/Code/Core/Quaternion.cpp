/**
 * \file Quaternion.cpp
 * \brief SeoulEngine implementation of the quaternion number system,
 * primarily used to represent 3D rotations.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Matrix3D.h"
#include "Matrix3x4.h"
#include "Matrix4D.h"
#include "Quaternion.h"
#include "Vector3D.h"

namespace Seoul
{

/**
 * Converts this unit quaternion into a 3x3 rotation matrix
 *
 * Converts this unit quaternion into a 3x3 rotation matrix which represents the
 * same rotation.  If this quaternion does not have unit length, the results are
 * undefined.
 *
 * @return A 3x3 rotation matrix corresponding to the same rotation as this
 *         quaternion
 *
 * \sa http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToMatrix/index.htm
 */
Matrix3D Quaternion::GetMatrix3D() const
{
	Matrix3D ret;

	const Float fTwoX = 2.0f * X;
	const Float fTwoY = 2.0f * Y;
	const Float fTwoZ = 2.0f * Z;

	const Float fTwoWX = W * fTwoX;
	const Float fTwoWY = W * fTwoY;
	const Float fTwoWZ = W * fTwoZ;
	const Float fTwoXX = X * fTwoX;
	const Float fTwoXY = X * fTwoY;
	const Float fTwoXZ = X * fTwoZ;
	const Float fTwoYY = Y * fTwoY;
	const Float fTwoYZ = Y * fTwoZ;
	const Float fTwoZZ = Z * fTwoZ;

	ret.M00 = 1.0f - (fTwoYY + fTwoZZ);
	ret.M01 = (fTwoXY - fTwoWZ);
	ret.M02 = (fTwoXZ + fTwoWY);

	ret.M10 = (fTwoXY + fTwoWZ);
	ret.M11 = 1.0f - (fTwoXX + fTwoZZ);
	ret.M12 = (fTwoYZ - fTwoWX);

	ret.M20 = (fTwoXZ - fTwoWY);
	ret.M21 = (fTwoYZ + fTwoWX);
	ret.M22 = 1.0f - (fTwoXX + fTwoYY);

	return ret;
}

/**
 * Converts this unit quaternion into a 4x4 rotation matrix
 *
 * Converts this unit quaternion into a 4x4 rotation matrix which represents the
 * same rotation.  If this quaternion does not have unit length, the results are
 * undefined.
 *
 * @return A 4x4 rotation matrix corresponding to the same rotation as this
 *         quaternion
 */
Matrix4D Quaternion::GetMatrix4D() const
{
	Matrix4D ret = Matrix4D::Identity();
	ret.SetRotation(GetMatrix3D());

	return ret;
}


/**
 * \sa Matrix4D::CreateRotationFromDirection
 */
Quaternion Quaternion::CreateFromDirection(
	const Vector3D& vDirection,
	const Vector3D& vBasisDirection /* = -Vector3D::UnitZ*/)
{
	// Calculate the smallest angle between vDirection and the basis direction
	// Clamp the dot product, because Acos can produce NaNs due to precision
	// error when the value is outside [-1, 1].
	Float fAngle = Acos(Clamp(Vector3D::Dot(vBasisDirection, vDirection), -1.0f, 1.0f));

	// Calculate a vector for rotation by calculating the cross product
	// of the basis vector and vDirection. UnitCross normalizes the
	// resulting vector before returning it.
	Vector3D vAxis = Vector3D::UnitCross(vBasisDirection, vDirection);

	// Check if the basis and the direction were parallel
	if (vAxis.IsZero(1e-3f))
	{
		// If so, use the major basis direction with minimum contribution
		// to the basis direction we are rotating from.
		Vector3D const vAbsBasis = vBasisDirection.Abs();
		Vector3D const vMinBasis = (vBasisDirection.X < vBasisDirection.Y
			? (vAbsBasis.X < vAbsBasis.Z ? Vector3D::UnitX() : Vector3D::UnitZ())
			: (vAbsBasis.Y < vAbsBasis.Z ? Vector3D::UnitY() : Vector3D::UnitZ()));
		vAxis = Vector3D::UnitCross(vBasisDirection, vMinBasis);
	}

	return Quaternion::CreateFromAxisAngle(vAxis, fAngle);
}

/**
 * Constructs a unit quaternion from a 3x3 rotation matrix
 *
 * Constructs a unit quaternion from a 3x3 rotation matrix.
 *
 * @param[in] m 3x3 transformation matrix to convert to a quaternion
 *
 * @return The resulting unit quaternion, or the invalid quaternion if
 *         the rotation portion of the input matrix is not
 *         orthonormal.
 */
Quaternion Quaternion::CreateFromRotationMatrix(const Matrix3D& m)
{
	// Numerically stable (see http://en.wikipedia.org/wiki/Rotation_matrix#Quaternion)
	Float Qxx = m.M00;
	Float Qyy = m.M11;
	Float Qzz = m.M22;

	Float fTrace = Qxx + Qyy + Qzz;

	Quaternion qReturn;
	if (fTrace > 0.0f)
	{
		Float r = Sqrt(1.0f + fTrace);
		Float s = 0.5f / r;

		qReturn.W = 0.5f * r;
		qReturn.X = (m.M21 - m.M12) * s;
		qReturn.Y = (m.M02 - m.M20) * s;
		qReturn.Z = (m.M10 - m.M01) * s;
	}
	else if (Qxx >= Qyy && Qxx >= Qzz)
	{
		Float rSqr = 1.0f + Qxx - Qyy - Qzz;

		if (rSqr < Epsilon)
		{
			return Quaternion::Invalid();
		}

		Float r = Sqrt(rSqr);
		Float s = 0.5f / r;

		qReturn.W = (m.M21 - m.M12) * s;
		qReturn.X = 0.5f * r;
		qReturn.Y = (m.M01 + m.M10) * s;
		qReturn.Z = (m.M02 + m.M20) * s;
	}
	else if (Qyy >= Qzz)
	{
		Float rSqr = 1.0f - Qxx + Qyy - Qzz;

		if (rSqr < Epsilon)
		{
			return Quaternion::Invalid();
		}

		Float r = Sqrt(rSqr);
		Float s = 0.5f / r;

		qReturn.W = (m.M02 - m.M20) * s;
		qReturn.X = (m.M01 + m.M10) * s;
		qReturn.Y = 0.5f * r;
		qReturn.Z = (m.M12 + m.M21) * s;
	}
	else
	{
		Float rSqr = 1.0f - Qxx - Qyy + Qzz;

		if (rSqr < Epsilon)
		{
			return Quaternion::Invalid();
		}

		Float r = Sqrt(rSqr);
		Float s = 0.5f / r;

		qReturn.W = (m.M10 - m.M01) * s;
		qReturn.X = (m.M02 + m.M20) * s;
		qReturn.Y = (m.M12 + m.M21) * s;
		qReturn.Z = 0.5f * r;
	}

	qReturn.Normalize();
	return qReturn;
}

/**
 * Constructs a unit quaternion from a 3x4 rotation matrix
 *
 * Constructs a unit quaternion from a 3x4 rotation matrix.  This
 * constructs the quaternion from the rotation matrix in the upper-left 3x3
 * submatrix; the rest of the matrix is ignored.
 *
 * @param[in] m 3x4 transformation matrix to convert to a quaternion
 *
 * @return The resulting unit quaternion, or the invalid quaternion if
 *         the rotation portion of the input matrix is not
 *         orthonormal.
 */
Quaternion Quaternion::CreateFromRotationMatrix(const Matrix3x4& m)
{
	return Quaternion::CreateFromRotationMatrix(Matrix3D(
		m.M00, m.M01, m.M02,
		m.M10, m.M11, m.M12,
		m.M20, m.M21, m.M22));
}

/**
 * Constructs a unit quaternion from a 4x4 rotation matrix
 *
 * Constructs a unit quaternion from a 4x4 rotation matrix.  This
 * constructs the quaternion from the rotation matrix in the upper-left 3x3
 * submatrix; the rest of the matrix is ignored.
 *
 * @param[in] m 4x4 transformation matrix to convert to a quaternion
 *
 * @return The resulting unit quaternion, or the invalid quaternion if
 *         the rotation portion of the input matrix is not
 *         orthonormal.
 */
Quaternion Quaternion::CreateFromRotationMatrix(const Matrix4D& m)
{
	return Quaternion::CreateFromRotationMatrix(Matrix3D(
		m.M00, m.M01, m.M02,
		m.M10, m.M11, m.M12,
		m.M20, m.M21, m.M22));
}

/**
 * Constructs a unit quaternion from a set of Euler angles
 *
 * Constructs a unit quaternion from a set of Euler angles: the yaw about the
 * Y-axis, the pitch about the X-axis, and the roll about the Z-axis.  Note that
 * this differs from QuaternionFromYawPitchRollZXY only in the order of the axes
 * about which the angles rotate.
 *
 * @param[in] fYaw   Yaw about the Y-axis
 * @param[in] fPitch Pitch about the X-axis
 * @param[in] fRoll  Roll about the Z-axis
 *
 * @return A unit quaternion corresponding to the rotation defined by the given
 *         Euler angles
 */
Quaternion Quaternion::CreateFromYawPitchRollYXZ(Float fYaw, Float fPitch, Float fRoll)
{
	return
		Quaternion::CreateFromRotationZ(fRoll) *
		Quaternion::CreateFromRotationX(fPitch) *
		Quaternion::CreateFromRotationY(fYaw);
}

/**
 * Constructs a unit quaternion from a set of Euler angles
 *
 * Constructs a unit quaternion from a set of Euler angles: the yaw about the
 * Z-axis, the pitch about the X-axis, and the roll about the Y-axis.  Note that
 * this differs from QuaternionFromYawPitchRollYXZ only in the order of the axes
 * about which the angles rotate.
 *
 * @param[in] fYaw   Yaw about the Z-axis
 * @param[in] fPitch Pitch about the X-axis
 * @param[in] fRoll  Roll about the Y-axis
 *
 * @return A unit quaternion corresponding to the rotation defined by the given
 *         Euler angles
 */
Quaternion Quaternion::CreateFromYawPitchRollZXY(Float fYaw, Float fPitch, Float fRoll)
{
	return
		Quaternion::CreateFromRotationY(fRoll) *
		Quaternion::CreateFromRotationX(fPitch) *
		Quaternion::CreateFromRotationZ(fYaw);
}

/**
 * Constructs a unit quaternion from a set of Euler angles
 *
 * Constructs a unit quaternion from a set of Euler angles: the yaw about the
 * Z-axis, the pitch about the X-axis, and the roll about the Y-axis.
 * Roll is applied first, the pitch second, and the yaw last.
 *
 * @param[in] fYaw   Yaw about the Z-axis
 * @param[in] fPitch Pitch about the X-axis
 * @param[in] fRoll  Roll about the Y-axis
 *
 * @return A unit quaternion corresponding to the rotation defined by the given
 *         Euler angles
 */
Quaternion Quaternion::CreateFromRollPitchYawYXZ(Float fRoll, Float fPitch, Float fYaw)
{
	return
		Quaternion::CreateFromRotationZ(fYaw) *
		Quaternion::CreateFromRotationX(fPitch) *
		Quaternion::CreateFromRotationY(fRoll);
}

/**
 * Performs spherical linear interpolation between two unit quaternions
 *
 * Performs spherical linear interpolation between two unit quaternions.  If
 * either quaternion is not a unit quaternion, the results are undefined.
 *
 * @param[in] q0 Start quaternion
 * @param[in] q1 End quaternion
 * @param[in] fT Interpolation parameter
 *
 * @return The quaternion that is the spherical linear interpolation between
 *         q0 and q1 at parameter fT.  When fT is 0, q0 is
 *         returned; when fT is 1, q1 is returned.
 */
Quaternion Quaternion::Slerp(const Quaternion& q0, const Quaternion& q1, Float fT)
{
	Float fOpposite = 0.0f;
	Float fInverse = 0.0f;
	Float fDot = (q0.W * q1.W) + (q0.X * q1.X) + (q0.Y * q1.Y) + (q0.Z * q1.Z);
	Bool bFlag = false;

	if (fDot < 0.0f)
	{
		bFlag = true;
		fDot = -fDot;
	}

	if (Seoul::Equals(fDot, 1.0f, 1e-6f))
	{
		fInverse = (1.0f - fT);
		fOpposite = (bFlag ? -fT : fT);
	}
	else
	{
		Float const fAcos = Acos(fDot);
		Float const fInvSin = (1.0f / Sin(fAcos));

		fInverse = (Sin((1.0f - fT) * fAcos)) * fInvSin;
		fOpposite = bFlag ? (-Sin(fT * fAcos) * fInvSin) : (Sin(fT * fAcos) * fInvSin);
	}

	Quaternion const qResult(
		(fInverse * q0.X) + (fOpposite * q1.X),
		(fInverse * q0.Y) + (fOpposite * q1.Y),
		(fInverse * q0.Z) + (fOpposite * q1.Z),
		(fInverse * q0.W) + (fOpposite * q1.W));

	return qResult;
}

} // namespace Seoul
