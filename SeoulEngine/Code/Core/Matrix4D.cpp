/**
 * \file Matrix4D.cpp
 * \brief Matrix4D represents a 4x4 square matrix.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Algorithms.h"
#include "Matrix3D.h"
#include "Matrix3x4.h"
#include "Matrix4D.h"
#include "Plane.h"

namespace Seoul
{

Matrix4D::Matrix4D(const Matrix3x4& matrix)
	: M00(matrix.M00), M10(matrix.M10), M20(matrix.M20), M30(0.0f)
	, M01(matrix.M01), M11(matrix.M11), M21(matrix.M21), M31(0.0f)
	, M02(matrix.M02), M12(matrix.M12), M22(matrix.M22), M32(0.0f)
	, M03(matrix.M03), M13(matrix.M13), M23(matrix.M23), M33(1.0f)
{
}

/**
 * Assume this Matrix4D is already a standard projection transform,
 * converts it to a biased projection transform. Positive epsilon values
 * result in projected depth values closer to the camera.
 */
Matrix4D Matrix4D::BiasedProjection(Double fEpsilon /*= kfBiasProjectionEpsilon*/) const
{
	// Start with this.
	Matrix4D mReturn(*this);

	// Generate a biased projection matrix, which will generate an
	// offset Z value.
	//
	// See: http://www.terathon.com/gdc07_lengyel.pdf
	//
	// Our transform is "traditional" DirectX
	// W.R.T projection convention ([0, 1]), note
	// differences in values and matrix components
	// compared to the paper.
	Float fNear;
	Float fUnusedFar;
	Matrix4D::ExtractNearFar(mReturn, fNear, fUnusedFar);
	(void)fUnusedFar;

	mReturn.M22 = (Float)(mReturn.M22 + fEpsilon);
	mReturn.M23 = mReturn.M22 * fNear;
	return mReturn;
}

/**
 * Assume this Matrix4D is already a standard projection transform,
 * converts it to an infinite projection transform. Positive epsilon
 * values result in projected epth values closer to the camera, with
 * the minimum depth value being at "infinity". Note that negative values
 * or values < kfInfiniteProjectionEpsilon will result in incorrect
 * projection.
 */
Matrix4D Matrix4D::InfiniteProjection(Double fEpsilon /*= kfInfiniteProjectionEpsilon*/) const
{
	// Epsilon for precision, see paper.
	const Double fInfiniteProjFixedEpsilon = (fEpsilon - 1.0);

	// Start with this.
	Matrix4D mReturn(*this);

	// Generate an infinite projection matrix, which will generate a Z
	// value for every vertex that is at the far plane (with an offset
	// to avoid clipping due to precision error.
	//
	// See: http://www.terathon.com/gdc07_lengyel.pdf
	//
	// Our transform is "traditional" DirectX
	// W.R.T projection convention ([0, 1]), note
	// differences in values and matrix components
	// compared to the paper.
	Float fNear;
	Float fUnusedFar;
	Bool const bPerspective = Matrix4D::ExtractNearFar(mReturn, fNear, fUnusedFar);
	(void)fUnusedFar;

	// Apply infinite projection based on perspective vs. orthographic.
	if (bPerspective)
	{
		mReturn.M22 = (Float)fInfiniteProjFixedEpsilon;
		mReturn.M23 = (Float)(fInfiniteProjFixedEpsilon * (Double)fNear);
		mReturn.M33 = mReturn.M23;
	}
	else
	{
		mReturn.M22 = 0.0f;
		mReturn.M23 = -(Float)fInfiniteProjFixedEpsilon;
	}

	return mReturn;
}

/**
 * Creates an rotation transform from a direction vector.
 *
 * This method uses vBasisDirection as the reference direction for c
 * calculating the rotation of vDirection, where vBasisDirection
 * points down -Z by default.
 *
 * \pre vDirection must be unit length.
 */
Matrix4D Matrix4D::CreateRotationFromDirection(
	const Vector3D& vDirection,
	const Vector3D& vBasisDirection /* = -Vector3D::UnitZ*/)
{
	// Calculate the smallest angle between vDirection and the basis direction
	// Clamp the dot product, because Acos can produce NaNs due to precision
	// error when the value is outside [-1, 1].
	Float fAngle =
		Acos(Clamp(Vector3D::Dot(vBasisDirection, vDirection), -1.0f, 1.0f));

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

	return Matrix4D::CreateRotationFromAxisAngle(vAxis, fAngle);
}

/**
 * Converts mTransform into a transform suitable
 * for transforming 3D direction vectors.
 *
 * This method is unnecessary if a transform is orthonormal, or,
 * the transform contains only translation and rotation. This method is
 * required if a transform contains scale.
 *
 * \sa Lengyel, E. 2004. "Mathematics for 3D Game Programming and
 *   Computer Graphics". Chapter 3.5.
 */
Matrix4D Matrix4D::CreateNormalTransform(const Matrix4D& mTransform)
{
	Matrix3D m3x3(
		mTransform.M00, mTransform.M01, mTransform.M02,
		mTransform.M10, mTransform.M11, mTransform.M12,
		mTransform.M20, mTransform.M21, mTransform.M22);

	m3x3 = m3x3.Inverse().Transpose();

	return Matrix4D(
		m3x3.M00, m3x3.M01, m3x3.M02, mTransform.M03,
		m3x3.M10, m3x3.M11, m3x3.M12, mTransform.M13,
		m3x3.M20, m3x3.M21, m3x3.M22, mTransform.M23,
		0.0f, 0.0f, 0.0f, 1.0f);
}

/**
 * Creates an RT transform using mRotation
 * to define the rotation and vTranslation to define
 * the translation.
 */
Matrix4D Matrix4D::CreateRotationTranslation(
	const Matrix3D& mRotation,
	const Vector3D& vTranslation)
{
	Matrix4D mReturn = CreateFromMatrix3D(mRotation);
	mReturn.SetTranslation(vTranslation);

	return mReturn;
}

/**
 * Creates an RT transform using qRotation
 * to define the rotation and vTranslation to define
 * the translation.
 */
Matrix4D Matrix4D::CreateRotationTranslation(
	const Quaternion& qRotation,
	const Vector3D& vTranslation)
{
	Matrix4D mReturn = qRotation.GetMatrix4D();
	mReturn.SetTranslation(vTranslation);

	return mReturn;
}

/**
 * Creates a projection transform with no perspective
 * effect. Transforms from a right-handed view space with +X to the
 * right, +Y up, and -Z into the screen.
 *
 * \sa  Section 4.6.1 of Akenine-Moller, T., Haines, E., Hoffman, N. 2008.
 *      "Real-Time Rendering: Third Edition", AK Peters, Ltd.
 */
Matrix4D Matrix4D::CreateOrthographic(
	Float fLeft,
	Float fRight,
	Float fBottom,
	Float fTop,
	Float fNear,
	Float fFar)
{
	SEOUL_ASSERT(fRight - fLeft >= fEpsilon);
	SEOUL_ASSERT(fTop - fBottom >= fEpsilon);
	SEOUL_ASSERT(fFar - fNear >= fEpsilon);

	Matrix4D ret = Matrix4D(
		2.0f / (fRight - fLeft), 0.0f,                     0.0f,                  -(fRight + fLeft) / (fRight - fLeft),
		0.0f,                    2.0f / (fTop - fBottom),  0.0f,                  -(fTop + fBottom) / (fTop - fBottom),
		0.0f,                    0.0f,                    -1.0f / (fFar - fNear), -fNear / (fFar - fNear),
		0.0f,                    0.0f,                     0.0f,                  1.0f);

	return ret;
}

/**
 * Creates a perspective transform that transforms from a
 * right-handed view space with +X to the right, +Y up, and -Z into
 * the screen.
 *
 * Values are in view space. For a standard symmetrical perspective
 * transform, fRight should be equal to -fLeft and fTop should be equal
 * to -fBottom.
 */
Matrix4D Matrix4D::CreatePerspectiveOffCenter(
	Float fLeft,
	Float fRight,
	Float fBottom,
	Float fTop,
	Float fNear,
	Float fFar)
{
	SEOUL_ASSERT(fRight - fLeft >= fEpsilon);
	SEOUL_ASSERT(fTop - fBottom >= fEpsilon);
	SEOUL_ASSERT(fFar - fNear >= fEpsilon);

	Matrix4D m;

	m.M00 = (2.0f * fNear) / (fRight - fLeft);
	m.M01 = 0.0f;
	m.M02 = (fRight + fLeft) / (fRight - fLeft);
	m.M03 = 0.0f;

	m.M10 = 0.0f;
	m.M11 = (2.0f * fNear) / (fTop - fBottom);
	m.M12 = (fTop + fBottom) / (fTop - fBottom);
	m.M13 = 0.0f;

	m.M20 = 0.0f;
	m.M21 = 0.0f;
	m.M22 = -(fFar / (fFar - fNear));
	m.M23 = (-fFar * fNear) / (fFar - fNear);

	m.M30 = 0.0f;
	m.M31 = 0.0f;
	m.M32 = -1.0f;
	m.M33 = 0.0f;

	return m;
}

/**
 * Create a perspective transform from the given camera parameters.
 */
Matrix4D Matrix4D::CreatePerspectiveFromVerticalFieldOfView(
	Float fFovInRadians,
	Float fAspectRatio,
	Float fNear,
	Float fFar)
{
	Matrix4D m;

	const Float k = Tan(fFovInRadians * 0.5f);

	const Float t = fNear * k;
	const Float b = -t;
	const Float r = fAspectRatio * t;
	const Float l = -r;

	m = CreatePerspectiveOffCenter(l, r, b, t, fNear, fFar);

	return m;
}

/**
 * Create a Matrix4D with an rotation part that is a
 * rotation around axis vAxis of fAngleInRadians radians.
 *
 * \sa http://www.gamedev.net/community/forums/topic.asp?topic_id=463471
 */
Matrix4D Matrix4D::CreateRotationFromAxisAngle(
	const Vector3D& vAxis,
	Float fAngleInRadians)
{
	const Float fS = Sin(fAngleInRadians);
	const Float fC = Cos(fAngleInRadians);
	const Float fT = (1.0f - fC);

	const Float fX = vAxis.X;
	const Float fY = vAxis.Y;
	const Float fZ = vAxis.Z;

	Matrix4D ret(
		(fT * fX * fX) + fC       , (fT * fX * fY) - (fZ * fS), (fT * fX * fZ) + (fY * fS), 0.0f,
		(fT * fX * fY) + (fZ * fS), (fT * fY * fY) +  fC      , (fT * fY * fZ) - (fX * fS), 0.0f,
		(fT * fX * fZ) - (fY * fS), (fT * fY * fZ) + (fX * fS), (fT * fZ * fZ) + fC       , 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);

	return ret;
}

// How this was calculated:
//
// DC = (n.x * v.x + n.y * v.y + n.z * v.z + [1.0|0.0] * d)
// ret = (v - (2.0f * DC * n))

// ret.x = v.x - (2.0 * DC * n.x)
// ret.y = v.y - (2.0 * DC * n.y)
// ret.z = v.z - (2.0 * DC * n.z)

// ret.x = v.x - ((2.0 * n.x) * (n.x * v.x + n.y * v.y + n.z * v.z + [1.0|0.0] * d))
// ret.y = v.y - ((2.0 * n.y) * (n.x * v.x + n.y * v.y + n.z * v.z + [1.0|0.0] * d))
// ret.z = v.z - ((2.0 * n.z) * (n.x * v.x + n.y * v.y + n.z * v.z + [1.0|0.0] * d))

// ret.x = v.x - ((2.0 * n.x * n.x * v.x) +
//                (2.0 * n.x * n.y * v.y) +
//                (2.0 * n.x * n.z * v.z) +
//                (2.0 * n.x * [1|0] * d))
// ret.y = v.y - ((2.0 * n.y * n.x * v.x) +
//                (2.0 * n.y * n.y * v.y) +
//                (2.0 * n.y * n.z * v.z) +
//                (2.0 * n.y * [1|0] * d))
// ret.z = v.z - ((2.0 * n.z * n.x * v.x) +
//                (2.0 * n.z * n.y * v.y) +
//                (2.0 * n.z * n.z * v.z) +
//                (2.0 * n.z * [1|0] * d))

// ret.x = v.x - (2.0 * n.x * n.x * v.x) -
//               (2.0 * n.x * n.y * v.y) -
//               (2.0 * n.x * n.z * v.z) -
//               (2.0 * n.x * [1|0] * d)
// ret.y = v.y - (2.0 * n.y * n.x * v.x) -
//               (2.0 * n.y * n.y * v.y) -
//               (2.0 * n.y * n.z * v.z) -
//               (2.0 * n.y * [1|0] * d)
// ret.z = v.z - (2.0 * n.z * n.x * v.x) -
//               (2.0 * n.z * n.y * v.y) -
//               (2.0 * n.z * n.z * v.z) -
//               (2.0 * n.z * [1|0] * d)

// ret.x = v.x * (1 - (2 * n.x * n.x)) -
//               (2.0 * n.x * n.y * v.y) -
//               (2.0 * n.x * n.z * v.z) -
//               (2.0 * n.x * [1|0] * d)
// ret.y =     - (2.0 * n.y * n.x * v.x) -
//         v.y * (1 - (2 * n.y * n.y)) -
//               (2.0 * n.y * n.z * v.z) -
//               (2.0 * n.y * [1|0] * d)
// ret.z =     - (2.0 * n.z * n.x * v.x) -
//               (2.0 * n.z * n.y * v.y) -
//         v.z * (1 - (2 * n.z * n.z)) -
//               (2.0 * n.z * [1|0] * d)

// Transform =
//     [1 - (2 * n.x * n.x)] [  - (2 * n.y * n.x)] [  - (2 * n.z * n.x)] [0]
//     [  - (2 * n.x * n.y)] [1 - (2 * n.y * n.y)] [  - (2 * n.z * n.y)] [0]
//     [  - (2 * n.x * n.z)] [  - (2 * n.y * n.z)] [1 - (2 * n.z * n.z)] [0]
//     [  - (2 * n.x * d)  ] [  - (2 * n.y * d)  ] [  - (2 * n.z * d)  ] [1]

/**
 * Returns a transform which will mirror a point around the given
 * plane plane.
 */
Matrix4D Matrix4D::CreateReflection(const Plane& plane)
{
	Float d = plane.D;
	const Vector3D& n = plane.GetNormal();

	Matrix4D ret;
	ret.M00 = 1.0f - (2.0f * n.X * n.X); ret.M10 =      - (2.0f * n.Y * n.X); ret.M20 =      - (2.0f * n.Z * n.X); ret.M30 = 0.0f;
	ret.M01 =      - (2.0f * n.X * n.Y); ret.M11 = 1.0f - (2.0f * n.Y * n.Y); ret.M21 =      - (2.0f * n.Z * n.Y); ret.M31 = 0.0f;
	ret.M02 =      - (2.0f * n.X * n.Z); ret.M12 =      - (2.0f * n.Y * n.Z); ret.M22 = 1.0f - (2.0f * n.Z * n.Z); ret.M32 = 0.0f;
	ret.M03 =      - (2.0f * n.X * d);   ret.M13 =      - (2.0f * n.Y * d);   ret.M23 =      - (2.0f * n.Z * d);   ret.M33 = 1.0f;

	return ret;
}

/**
 * Returns a rotation transform which will rotate points
 * by fAngleInRadians around the x axis.
 */
Matrix4D Matrix4D::CreateRotationX(Float fAngleInRadians)
{
	Float cos = Cos(fAngleInRadians);
	Float sin = Sin(fAngleInRadians);

	return Matrix4D(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f,  cos, -sin, 0.0f,
		0.0f,  sin,  cos, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
}

/**
 * Returns a rotation transform which will rotate points
 * by fAngleInRadians around the y axis.
 */
Matrix4D Matrix4D::CreateRotationY(Float fAngleInRadians)
{
	Float cos = Cos(fAngleInRadians);
	Float sin = Sin(fAngleInRadians);

	return Matrix4D(
		cos,  0.0f,  sin, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		-sin, 0.0f,  cos, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
}

/**
 * Returns a rotation transform which will rotate points
 * by fAngleInRadians around the z axis.
 */
Matrix4D Matrix4D::CreateRotationZ(Float fAngleInRadians)
{
	Float cos = Cos(fAngleInRadians);
	Float sin = Sin(fAngleInRadians);

	return Matrix4D(
		cos,  -sin, 0.0f, 0.0f,
		sin,   cos, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
}

/**
 * Returns a uniform scaling transform with scale fScale.
 */
Matrix4D Matrix4D::CreateScale(Float fScale)
{
	return Matrix4D(
		fScale,   0.0f,   0.0f, 0.0f,
		  0.0f, fScale,   0.0f, 0.0f,
		  0.0f,   0.0f, fScale, 0.0f,
		  0.0f,   0.0f,   0.0f, 1.0f);
}

/**
 * Returns a non-uniform scaling transform with x scale
 * fX, y scale fY, and z scale fZ.
 */
Matrix4D Matrix4D::CreateScale(Float fX, Float fY, Float fZ)
{
	return Matrix4D(
		  fX, 0.0f, 0.0f, 0.0f,
		0.0f,   fY, 0.0f, 0.0f,
		0.0f, 0.0f,   fZ, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
}

/**
 * Returns a non-uniform scaling transform with x scale
 * vScale.X, y scale vScale.Y, and z scale vScale.Z.
 */
Matrix4D Matrix4D::CreateScale(const Vector3D& vScale)
{
	return Matrix4D(
		vScale.X,     0.0f,     0.0f,  0.0f,
		0.0f,     vScale.Y,     0.0f,  0.0f,
		0.0f,         0.0f, vScale.Z,  0.0f,
		0.0f,         0.0f,      0.0f, 1.0f);
}

/**
 * Returns a translation transform with translation
 * components equal to fX, fY, and fZ.
 */
Matrix4D Matrix4D::CreateTranslation(Float fX, Float fY, Float fZ)
{
	return Matrix4D(
		1.0f, 0.0f, 0.0f, fX,
		0.0f, 1.0f, 0.0f, fY,
		0.0f, 0.0f, 1.0f, fZ,
		0.0f, 0.0f, 0.0f, 1.0f);
}

/**
 * Returns a translation transform with translation
 * components equal to the translation of vTranslation.
 */
Matrix4D Matrix4D::CreateTranslation(const Vector3D& vTranslation)
{
	return Matrix4D(
		1.0f, 0.0f, 0.0f, vTranslation.X,
		0.0f, 1.0f, 0.0f, vTranslation.Y,
		0.0f, 0.0f, 1.0f, vTranslation.Z,
		0.0f, 0.0f, 0.0f, 1.0f);
}

/**
 * Returns a 4x4 matrix from 3x3 matrix m.
 */
Matrix4D Matrix4D::CreateFromMatrix3D(const Matrix3D& m)
{
	return Matrix4D(
		m.M00, m.M01, m.M02, 0.0f,
		m.M10, m.M11, m.M12, 0.0f,
		m.M20, m.M21, m.M22, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
}

/**
 * Decompose mTransform into pre-rotation, rotation, and translation parts,
 * where the transform is considered to be (translation * rotation * pre-rotation), or,
 * pre-rotation applied first and then rotation and then translation.
 *
 * @return True if mTransform was successfully decomposed into
 * pre-rotation, rotation, and translation, false otherwise.
 */
Bool Matrix4D::Decompose(
	const Matrix4D& mTransform,
	Matrix3D& rmPreRotation,
	Matrix3D& rmRotation,
	Vector3D& rvTranslation)
{
	// Scale and Rotation
	Matrix3D mPreRotationAndRotation;
	mTransform.GetRotation(mPreRotationAndRotation);

	if (Matrix3D::Decompose(mPreRotationAndRotation, rmPreRotation, rmRotation))
	{
		// Translation
		rvTranslation = mTransform.GetTranslation();
		return true;
	}

	return false;
}

/**
 * Given a projection transform following the convention described
 * in CreatePerspectiveOffCenter(), this method outputs the near plane
 * and far plane distances in arguments rfNear and rfFar.
 */
Bool Matrix4D::ExtractNearFar(
	const Matrix4D& mProjectionTransform,
	Float& rfNear,
	Float& rfFar)
{
	rfNear = (mProjectionTransform.M23 / mProjectionTransform.M22);

	// If 32 of the projection transform is < 0.0, it is a perspective transform.
	if (mProjectionTransform.M32 < 0.0f)
	{
		rfFar = mProjectionTransform.M23 / (1.0f + mProjectionTransform.M22);
		return true;
	}
	// Otherwise, it is an orthographic transform.
	else
	{
		rfFar = (-1.0f / mProjectionTransform.M22) + rfNear;
		return false;
	}
}

/**
 * Given a projection transform following the convention described
 * in CreatePerspectiveOffCenter(), this method returns the FOV of
 * the transform in radians.
 */
Float Matrix4D::ExtractFovInRadians(const Matrix4D& mProjectionTransform)
{
	Float fReturn = 2.0f * (Atan(1.0f / mProjectionTransform.M11));

	return fReturn;
}

/**
 * Given a projection transform following the convention described
 * in CreatePerspectiveOffCenter(), this method returns the aspect
 * ratio of the transform (width / height of the viewport of the camera
 * described by this projection transform).
 */
Float Matrix4D::ExtractAspectRatio(const Matrix4D& mProjectionTransform)
{
	Float fReturn = (mProjectionTransform.M11 / mProjectionTransform.M00);

	return fReturn;
}

/**
 * Given a valid perspective project transform, update the aspect ratio.
 *
 * \pre M00 of the matrix must be non-zero. This is a requirement
 * for an existing, valid perspective project transform.
 */
void Matrix4D::UpdateAspectRatio(Float fAspectRatio)
{
	// Sanity checks.
	SEOUL_ASSERT(!Seoul::IsZero(fAspectRatio));

	M00 = (M11 / fAspectRatio);
}

} // namespace Seoul
