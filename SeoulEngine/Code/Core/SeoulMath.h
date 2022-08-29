/**
 * \file SeoulMath.h
 * \brief SeoulEngine rough equivalent to the standard <cmath> or <math.h>
 * header. Includes constants and basic utilities.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SEOUL_MATH_H
#define SEOUL_MATH_H

#include "SeoulTypes.h"

#include <float.h>
#include <limits>
#include <limits.h>
#include <math.h>
#include <stdlib.h>

namespace Seoul
{

/**
 * The number of bits the compiler uses to represent a char
 */
const Int CharBitCount = CHAR_BIT;

/**
 * Double-precision value closest to pi
 */
const Double Pi = 3.1415926535897932;

/**
 * Single-precision value closest to pi
 */
const Float fPi = 3.1415926535897932f;

/**
 * Double-precision value closest to 2*pi
 */
const Double TwoPi = 6.2831853071795865;

/**
 * Single-precision value closest to 2*pi
 */
const Float fTwoPi = 6.2831853071795865f;

/**
 * Double-precision value closest to pi/2
 */
const Double PiOverTwo = 1.57079632679489656;

/**
 * Double-precision value closest to pi/2
 */
const Float fPiOverTwo = 1.57079632679489656f;

/**
 * Double-precision value closest to pi/4
 */
const Double PiOverFour = 0.78539816339744828;

/**
 * Double-precision value closest to pi/4
 */
const Float fPiOverFour = 0.78539816339744828f;

/**
 * Double-precision value closest to 1/pi
 */
const Double OneOverPi = 0.31830988618379067;

/**
 * Double-precision value closest to 180/pi
 */
const Double OneEightyOverPi = 57.2957795130823208;

/**
 * Double-precision value closest to pi/180
 */
const Double PiOverOneEighty = 0.017453292519943295;

/**
 * Double-precision value closest to e
 */
const Double EulerNumber = 2.7182818284590452;

/**
 * Double-precision value closest to the square root of 2
 */
const Double SquareRootOfTwo = 1.41421356237309505;

/**
 * Double-precision value closest to the reciprocal of the square root of 2
 */
const Double OneOverSquareRootOfTwo = 0.707106781186547524;

/**
 * Double-precision value closest to the log-base-2 of e
 */
const Double Log2OfE = 1.4426950408889634;

/**
 * Double-precision value closest to the log-base-2 of 10
 */
const Double Log2Of10 = 3.3219280948873626;

/**
 * Double-precision value closest to the natural log of 2
 */
const Double LogEOf2 = 0.69314718055994529;

/**
 * Double-precision value closest to the natural log of 10
 */
const Double LogEOf10 = 2.3025850929940459;

/**
 * Double-precision value closest to the log-base-10 of 2
 */
const Double Log10Of2 = 0.3010299956639812;

/**
 * Double-precision value closest to the log-base-10 of e
 */
const Double Log10OfE = 0.43429448190325182;

/**
 * Default epsilon value for floating-point comparisons
 * corresponds to DBL_EPSILON
 */
const Double Epsilon = DBL_EPSILON;

/**
 * Default epsilon value for floating-point comparisons
 * corresponds to FLT_EPSILON
 */
const Float fEpsilon = FLT_EPSILON;

/**
 * Largest positive finite double-precision value
 * corresponds to DBL_MAX
 */
const Double DoubleMax = 1.7976931348623157e308;

/**
 * Smallest positive normal double-precision value
 * corresponds to DBL_MIN
 */
const Double DoubleMinNormal = 2.2250738585072020e-308;

/**
 * Smallest positive subnormal double-precision value
 */
const Double DoubleMinSubnormal = 5e-324;

/**
 * Largest positive finite single-precision value
 * corresponds to FLT_MAX
 */
const Float FloatMax = 3.402823466e38f;

/**
 * Smallest positive normal single-precision value
 * corresponds to FLT_MIN
 */
const Float FloatMinNormal = 1.175494351e-38f;

/**
 * Smallest positive subnormal single-precision value
 */
const Float FloatMinSubnormal = 1.4012985e-45f;

/**
 * Largest UInt, corresponds to UINT_MAX.
 */
const UInt UIntMax = 0xFFFFFFFF;

/**
 * Largest UInt32, corresponds to _UINT32_MAX
 */
const UInt32 UInt32Max = 0xFFFFFFFF;

/**
 * Largest UInt64, corresponds to ULLONG_MAX.
 */
#if SEOUL_PLATFORM_WINDOWS
const UInt64 UInt64Max = 0xffffffffffffffffui64;
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
const UInt64 UInt64Max = 0xffffffffffffffffLLU;
#else
#error "Define for this platform."
#endif

/**
 * Largest Int, corresponds to INT_MAX.
 */
const Int IntMax = 0x7fffffff;

/**
 * Smallest Int, corresponds to INT_MIN.
 */
const Int IntMin = (-2147483647 - 1);

/**
 * Largest signed 8 bit Int
 */
const Int8 Int8Max = 127;


/**
 * Smallest signed 8 bit Int
 */
const Int8 Int8Min = -128;

/**
 * Largest Int64, corresponds to _I64_MAX.
 */
#if SEOUL_PLATFORM_WINDOWS
const Int64 Int64Max = 9223372036854775807i64;
const Int64 Int64Min = -Int64Max - 1i64;
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
const Int64 Int64Max = 9223372036854775807LL;
const Int64 Int64Min = -Int64Max - 1LL;
#else
#error "Define for this platform."
#endif

/**
 * Largest UInt8.
 */
const UInt8 UInt8Max = 0xFF;

/**
 * Largest UInt16, corresponds to _UINT16_MAX
 */
const UInt16 UInt16Max = 0xFFFF;

/**
 * Largest signed 16 bit Int
 */
const Int16 Int16Max = 32767;

/**
 * Smallest signed 16 bit Int
 */
const Int16 Int16Min = -32768;

/**
 * Largest integer a 32 bit float can safely represent
 */
const Int FlIntMax = (1 << 24);

/**
 * Largest integer a 64 bit float can safely represent.
 */
const Int64 FlInt64Max = ((Int64)1 << (Int64)53);

/**
* Get the sign of a float.  Returns 1 if the number is positive, -1 if it's negative, 0 otherwise
*/
inline Int32 Sign(Float x)
{
	if (x > 0.f)
	{
		return 1;
	}
	else if (x < 0.f)
	{
		return -1;
	}
	else
	{
		return 0;
	}
}

/**
* Get the sign of a float.  Returns 1.f if the number is positive, -1.f if it's negative, 0.f otherwise
*/
inline Float fSign(Float x)
{
	if (x > 0.0f)
	{
		return 1.0f;
	}
	else if (x < 0.0f)
	{
		return -1.0f;
	}
	else
	{
		return 0.0f;
	}
}

/**
* Get the sign of a double. Returns 1 if the number is positive, -1 if it's negative, 0 otherwise
*/
inline Int32 Sign(Double x)
{
	if (x > 0.0)
	{
		return 1;
	}
	else if (x < 0.0)
	{
		return -1;
	}
	else
	{
		return 0;
	}
}

/**
* Get the sign of a double. Returns 1.f if the number is positive, -1.f if it's negative, 0.f otherwise
*/
inline Float fSign(Double x)
{
	if (x > 0.0)
	{
		return 1.0f;
	}
	else if (x < 0.0)
	{
		return -1.0f;
	}
	else
	{
		return 0.0f;
	}
}

/**
* Get the sign of an integer. Returns 1 if the number is positive, -1 if it's negative, 0 otherwise
*/
inline Int32 Sign(Int32 x)
{
	if (x > 0)
	{
		return 1;
	}
	else if (x < 0)
	{
		return -1;
	}
	else
	{
		return 0;
	}
}

/**
 * Converts radians to degrees
 *
 * Converts radians to degrees.
 *
 * @param[in] fRadians Angle in radians
 *
 * @return Equivalent angle in degrees
 */
inline Double RadiansToDegrees(Double fRadians)
{
	return fRadians * OneEightyOverPi;
}

/**
 * Converts radians to degrees
 *
 * Converts radians to degrees.
 *
 * @param[in] fRadians Angle in radians
 *
 * @return Equivalent angle in degrees
 */
inline Float RadiansToDegrees(Float fRadians)
{
	return fRadians * (Float)OneEightyOverPi;
}

/**
 * Circular clamps the radians fRadians
 * to fall on [0, fTwoPi)
 */
inline Float RadianClampTo0ToTwoPi(Float fRadians)
{
	while (fRadians < 0.0f) { fRadians += fTwoPi; }
	while (fRadians >= fTwoPi) { fRadians -= fTwoPi; }

	return fRadians;
}

/**
 * Circular clamps the radians fRadians
 * to fall on [0, fTwoPi)
 */
inline Double RadianClampTo0ToTwoPi(Double Radians)
{
	while (Radians < 0.0) { Radians += TwoPi; }
	while (Radians >= TwoPi) { Radians -= TwoPi; }

	return Radians;
}

/**
 * Converts degrees to radians
 *
 * Converts degrees to radians
 *
 * @param[in] fDegrees Angle in degrees
 *
 * @return Equivalent angle in radians
 */
inline Double DegreesToRadians(Double fDegrees)
{
	return fDegrees * PiOverOneEighty;
}

/**
 * Circular clamps the degrees fDegrees
 * to fall on [0, 360)
 */
inline Float DegreeClampTo0To360(Float fDegrees)
{
	while (fDegrees < 0.0f) { fDegrees += 360.0f; }
	while (fDegrees >= 360.0f) { fDegrees -= 360.0f; }

	return fDegrees;
}

/**
 * Circular clamps the degrees Degrees
 * to fall on [0, 360)
 */
inline Double DegreeClampTo0To360(Double Degrees)
{
	while (Degrees < 0.0) { Degrees += 360.0; }
	while (Degrees >= 360.0) { Degrees -= 360.0; }

	return Degrees;
}

/**
 * Converts degrees to radians
 *
 * Converts degrees to radians
 *
 * @param[in] fDegrees Angle in degrees
 *
 * @return Equivalent angle in radians
 */
inline Float DegreesToRadians(Float fDegrees)
{
	return fDegrees * (Float)PiOverOneEighty;
}

/**
 * Returns the minimum of two values
 *
 * Returns the minimum of two values
 *
 * @param T Type of values to compare
 *
 * @param[in] a First value to compare
 * @param[in] b Second value to compare
 *
 * @return Minimum of a and b
 */
template <typename T>
inline T Min(T a, T b)
{
	return (a < b) ? a : b;
}

/// Returns the minimum of three values.
template <typename T>
inline T Min(T a, T b, T c)
{
	return Min(a, Min(b, c));
}

/// Returns the minimum of four values.
template <typename T>
inline T Min(T a, T b, T c, T d)
{
	return Min(a, Min(b, Min(c, d)));
}

/**
 * Returns the maximum of two values
 *
 * Returns the maximum of two values
 *
 * @param T Type of values to compare
 *
 * @param[in] a First value to compare
 * @param[in] b Second value to compare
 *
 * @return Maximum of a and b
 */
template <typename T>
inline T Max(T a, T b)
{
	return (a > b) ? a : b;
}

/// Returns the maximum of three values.
template <typename T>
inline T Max(T a, T b, T c)
{
	return Max(a, Max(b, c));
}

/// Returns the maximum of four values.
template <typename T>
inline T Max(T a, T b, T c, T d)
{
	return Max(a, Max(b, Max(c, d)));
}

/**
 * Clamps a value to a given range
 *
 * Clamps a value to a given range.  min must be less than or equal to
 * max.
 *
 * Clamp must handle NaN's such that if value is a NaN, min
 * is returned.
 *
 * @param T Type of values
 *
 * @param[in] value Value to clamp
 * @param[in] min   Minimum value of range to clamp to
 * @param[in] max   Maximum value of range to clamp to
 *
 * @return value, if value is between min and max; min, if
 *         value is less than min; or max, if value is greater than
 *         max
 */
template <typename T>
inline T Clamp(T value, T min, T max)
{
	return Min(Max(value, min), max);
}

/** Clamp for circular spaces defined in degrees. Clamp the value to [-180, 180]. */
template <typename T>
static inline T ClampDegrees(T value)
{
	while (value > (T)180)
	{
		value -= (T)360;
	}
	while (value < (T)-180)
	{
		value += (T)360;
	}

	return value;
}

/** Clamp for circular spaces defined in radians. Clamp the value to [-Pi, Pi]. */
template <typename T>
static inline T ClampRadians(T value)
{
	while (value > (T)Pi)
	{
		value -= (T)TwoPi;
	}
	while (value < (T)-Pi)
	{
		value += (T)TwoPi;
	}

	return value;
}

inline Int    Abs( Int    X) { return ::abs(X);   }
inline Int64  Abs( Int64  X) { return ::llabs(X);   }
inline Float  Abs( Float  X) { return ::fabsf(X); }
inline Double Abs( Double X) { return ::fabs(X);  }
inline Float  Acos(Float  X) { return ::acosf(X); }
inline Double Acos(Double x) { return ::acos(x);  }
inline Float  Asin(Float  X) { return ::asinf(X); }
inline Double Asin(Double X) { return ::asin(X);  }
inline Float  Atan(Float  X) { return ::atanf(X); }
inline Double Atan(Double X) { return ::atan(X);  }
inline Float  Atan2(Float  X, Float  Y) { return ::atan2f(X, Y); }
inline Double Atan2(Double X, Double Y) { return ::atan2(X, Y);  }
inline Float  Ceil(Float  X) { return ::ceilf(X); }
inline Double Ceil(Double X) { return ::ceil(X);  }
inline Float  Cos( Float  X) { return ::cosf(X);  }
inline Double Cos( Double X) { return ::cos(X);   }
inline Float  Exp( Float  X) { return ::expf(X);  }
inline Double Exp( Double X) { return ::exp(X);   }
inline Float  Floor(Float  X) { return ::floorf(X); }
inline Double Floor(Double X) { return ::floor(X);  }
inline Float  Fmod(Float  X, Float  Y) { return ::fmodf(X, Y); }
inline Double Fmod(Double X, Double Y) { return ::fmod(X, Y);  }
inline Float  LogE(Float  X) { return ::logf(X);    }
inline Double LogE(Double X) { return ::log(X);     }
inline Float  Log10(Float  X) { return ::log10f(X); }
inline Double Log10(Double X) { return ::log10(X);  }
inline Float  Pow(Float  X, Float  Y) { return ::powf(X, Y); }
inline Double Pow(Double X, Double Y) { return ::pow(X, Y);  }
inline Float  Sin( Float  X) { return ::sinf(X);  }
inline Double Sin( Double X) { return ::sin(X);   }
inline Float  Sqrt(Float  X) { return ::sqrtf(X); }
inline Double Sqrt(Double X) { return ::sqrt(X);  }
inline Float  Tan( Float  X) { return ::tanf(X);  }
inline Double Tan( Double X) { return ::tan(X);   }

// "Fast" variations of trig and other math functions - on some platforms, less expensive,
// but also less accurate. See platform specific documentation for specific
// limitations.
inline Float FastCeil(Float fValue) { return Ceil(fValue); }
inline Float FastFloor(Float fValue) { return Floor(fValue); }
inline Float FastCos(Float fValue) { return Cos(fValue); }
inline Float FastSin(Float fValue) { return Sin(fValue); }

/**
 * Count the number of 1 bits in a 32-bit unsigned integer.
 *
 * \sa https://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
 */
static inline UInt32 CountBits(UInt32 u)
{
	u = (u & 0x55555555) + ((u >> 1) & 0x55555555);
	u = (u & 0x33333333) + ((u >> 2) & 0x33333333);
	u = (u + (u >> 4)) & 0x0F0F0F0F;
	u = (u + (u >> 8));
	u = (u + (u >> 16));
	return (u & 0xFF);
}

/**
 * Check for floating point zero with error
 */
inline Bool IsZero(Double val, Double eps = Epsilon)
{
	return Abs(val) <= eps;
}

/**
 * Check for floating point zero with error
 */
inline Bool IsZero(Float val, Float eps = fEpsilon)
{
	return Abs(val) <= eps;
}

inline Float Lerp(Float F0, Float F1, Float Alpha)
{
	return F0 * (1.0f - Alpha) + F1 * Alpha;
}

inline Int32 Lerp(Int32 a, Int32 b, Float fAlpha)
{
	return (Int32)Lerp((Float)a, (Float)b, fAlpha);
}

inline Double Lerp(Double D0, Double D1, Double Alpha)
{
	return D0 * (1.0 - Alpha) + D1 * Alpha;
}

/**
 * Equivalent to Lerp for a circular Degrees space.
 *
 * @return Value in degrees on [-180, 180].
 */
template <typename T>
inline T LerpDegrees(T a, T b, T alpha)
{
	// Normalize angles so a linear lerp is correct.
	while (b - a > (T)180)
	{
		b -= (T)360;
	}
	while (a - b > (T)180)
	{
		a -= (T)360;
	}

	return ClampDegrees(Lerp(a, b, alpha));
}

/**
 * Compares one Float to another using the given tolerance.
 *
 * @return true if Float a is equal to Float b within the given tolerance, otherwise false.
 */
inline Bool Equals(Float a, Float b, Float fTolerance = fEpsilon)
{
	return (Abs(a - b) <= fTolerance);
}

/**
 * Compares one Double to another using the given tolerance.
 *
 * @return true if Double a is equal to Double b within the given tolerance, otherwise false.
 */
inline Bool Equals(Double a, Double b, Double Tolerance = Epsilon)
{
	return (Abs(a - b) <= Tolerance);
}

/**
 * Compare one Float in degrees to another using the given tolerance.
 */
inline Bool EqualDegrees(Float a, Float b, Float fTolerance = fEpsilon)
{
	// Normalize values.
	while (b - a > 180.0f)
	{
		b -= 360.0f;
	}
	while (a - b > 180.0f)
	{
		a -= 360.0f;
	}

	return Equals(a, b, fTolerance);
}

/**
 * Compare one Float in degrees to another using the given tolerance.
 */
inline Bool EqualDegrees(Double a, Double b, Double Tolerance = Epsilon)
{
	// Normalize values.
	while (b - a > 180.0)
	{
		b -= 360.0;
	}
	while (a - b > 180.0)
	{
		a -= 360.0;
	}

	return Equals(a, b, Tolerance);
}

/**
 * Compare one Float in degrees to another using the given tolerance.
 */
inline Bool EqualRadians(Float a, Float b, Float fTolerance = fEpsilon)
{
	// Normalize values.
	while (b - a > fPi)
	{
		b -= fTwoPi;
	}
	while (a - b > fPi)
	{
		a -= fTwoPi;
	}

	return Equals(a, b, fTolerance);
}

/**
 * Compare one Float in degrees to another using the given tolerance.
 */
inline Bool EqualRadians(Double a, Double b, Double Tolerance = Epsilon)
{
	// Normalize values.
	while (b - a > Pi)
	{
		b -= TwoPi;
	}
	while (a - b > Pi)
	{
		a -= TwoPi;
	}

	return Equals(a, b, Tolerance);
}

/**
 * Returns true if f is not a number, false otherwise.
 */
inline Bool IsNaN(Float f)
{
#if defined(_MSC_VER)
	return (_isnan(f) != 0);
#else
	return isnan(f);
#endif
}

/**
 * Returns true if f is not a number, false otherwise.
 */
inline Bool IsNaN(Double f)
{
#if defined(_MSC_VER)
	return (_isnan(f) != 0);
#else
	return isnan(f);
#endif
}


/**
 * Returns true if f is infinity (positive or negative),
 *  false otherwise.
 */
inline Bool IsInf(Float f)
{
#if defined(_MSC_VER)
	return (_finite(f) == 0);
#else
	return isinf(f);
#endif
}

/**
 * Returns true if f is infinity (positive or negative),
 *  false otherwise.
 */
inline Bool IsInf(Double f)
{
#if defined(_MSC_VER)
	return (_finite(f) == 0);
#else
	return isinf(f);
#endif
}

/**
 * @return The time uMilliseconds converted to microseconds.
 */
inline UInt64 ConvertMillisecondsToMicroseconds(UInt32 uMilliseconds)
{
	return (UInt64)uMilliseconds * 1000u;
}

/**
 * Returns the vertial feild of view (FOV) that corresponds with the given horizontal FOV and aspect ratio
 *
 * @param[in] fHorizontalFOV the desired horizontal field of view in radians
 * @param[in] fAspectRatio the aspect ratio of the viewport
 *
 * @return The desired vertial field of view in radians
 */
inline Float GetVerticalFOVFromHorizontal(Float fHorizontalFOV, Float fAspectRatio)
{
	return 2.0f * Atan(Tan(fHorizontalFOV / 2.0f) / fAspectRatio);
}

/**
 * Returns the horizontal feild of view (FOV) that corresponds with the given vertical FOV and aspect ratio
 *
 * @param[in] fVerticalFOV the desired vertical field of view in radians
 * @param[in] fAspectRatio the aspect ratio of the viewport
 *
 * @return The desired horizontal field of view in radians
*/
inline Float GetHorizontalFOVFromVertical(Float fVerticalFOV, Float fAspectRatio)
{
	return 2.0f * Atan(Tan(fVerticalFOV / 2.0f) * fAspectRatio);
}

/**
 * Utility - add Int32 a and b with clamping (no overflow).
 *
 * @return
 *   If the result of (a + b) is > Int32Max = Int32Max
 *   If the result of (a + b) is < Int32Min = Int32Min
 *   Otherwise, (a + b).
 */
inline Int32 AddInt32Clamped(Int32 a, Int32 b)
{
	return (Int32)Clamp((Int64)a + (Int64)b, (Int64)IntMin, (Int64)IntMax);
}

/**
* Utility - subtract Int32 a and b with clamping (no overflow).
*
* @return
*   If the result of (a - b) is > Int32Max = Int32Max
*   If the result of (a - b) is < Int32Min = Int32Min
*   Otherwise, (a - b).
*/
inline Int32 SubInt32Clamped(Int32 a, Int32 b)
{
	return (Int32)Clamp((Int64)a - (Int64)b, (Int64)IntMin, (Int64)IntMax);
}

/**
 * @return The floating point value f rounded to the nearest integer boundary.
 */
inline Float Round(Float f)
{
	return (f > 0.0f ? Floor(f + 0.5f) : Ceil(f - 0.5f));
}

/**
 * @return The double precision floating point value f rounded to the
 * nearest integer boundary.
 */
inline Double Round(Double f)
{
	return (f > 0.0 ? Floor(f + 0.5) : Ceil(f - 0.5));
}

namespace GlobalRandom
{

// Exactly equivalent to the members of PseudoRandom, except
// tied to a single global, thread-safe instance of that class.
void GetSeed(UInt64& ruX, UInt64& ruY);
Float64 NormalRandomFloat64();
Float64 NormalRandomFloat64(Float64 fMean, Float64 fStandardDeviation);
void SetSeed(UInt64 uX, UInt64 uY);
Float32 UniformRandomFloat32();
Float64 UniformRandomFloat64();
Int32 UniformRandomInt32();
UInt32 UniformRandomUInt32();
Int64 UniformRandomInt63();
Int64 UniformRandomInt64();
UInt64 UniformRandomUInt64();
UInt32 UniformRandomUInt32n(UInt32 u);
UInt64 UniformRandomUInt64n(UInt64 u);

/**
 * Convenience generator compatible with Seoul::RandomShuffle()
 * (as well as std::random_shuffle, which was deprecated in C++14).
 */
static inline ptrdiff_t RandomShuffleGenerator(ptrdiff_t n)
{
	return (ptrdiff_t)UniformRandomUInt64n((UInt64)n);
}

} // namespace GlobalRandom

/**
 * Platform agnostic variation of standard rand()
 * (always has the same max value of 32,767).
 *
 * Prefer usage of IRand() over GlobalRandom functions
 * over this function, as they provide more flexibility
 * and control.
 */
static inline Int IRand()
{
	return (Int)(GlobalRandom::UniformRandomUInt32n(32768));
}

} // namespace Seoul

#endif // include guard
