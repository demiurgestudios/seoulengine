/**
 * \file Color.h
 * \brief Structures representing a 4 component, 32-bit float per
 * component color value and a 4 component, 8-bit unsigned int per
 * component color value.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef COLOR_H
#define COLOR_H

#include "FixedArray.h"
#include "SeoulTypes.h"
#include "Vector4D.h"

namespace Seoul
{

/** Size of the table used to perform fast sRGB to linear color conversions. */
static const UInt32 kSRGBToLinearTableSize = 256u;

/** Size of the table used to perform fast linear to sRGB color conversions. */
static const UInt32 kLinearToSRGBTableSize = 8192u;

/** Table used by FastSRGBGamma. */
extern FixedArray< Float, kSRGBToLinearTableSize> g_kaSRGBToLinear;

/** Table used by FastSRGBDeGamma. */
extern FixedArray< UInt8, kLinearToSRGBTableSize> g_kaLinearToSRGB;

/** Nearness to full opacity (255) that we consider occluding. */
static const UInt32 ku8bitColorOcclusionThreshold = (255u - 7u);

/**
 * 4-byte structure representing an ARGB color.
 */
struct ColorARGBu8 SEOUL_SEALED
{
	ColorARGBu8()
		: m_B(0)
		, m_G(0)
		, m_R(0)
		, m_A(0)
	{
	}

	static ColorARGBu8 Black()
	{
		return ColorARGBu8::Create(0u, 0u, 0u, 255u);
	}

	static ColorARGBu8 Blue()
	{
		return ColorARGBu8::Create(0u, 0u, 255u, 255u);
	}

	static ColorARGBu8 Cyan()
	{
		return ColorARGBu8::Create(0u, 255u, 255u, 255u);
	}

	static ColorARGBu8 Green()
	{
		return ColorARGBu8::Create(0u, 255u, 0u, 255u);
	}

	static ColorARGBu8 Magenta()
	{
		return ColorARGBu8::Create(255u, 0u, 255u, 255u);
	}

	static ColorARGBu8 Red()
	{
		return ColorARGBu8::Create(255u, 0u, 0u, 255u);
	}

	static ColorARGBu8 TransparentBlack()
	{
		return ColorARGBu8::Create(0u, 0u, 0u, 0u);
	}

	static ColorARGBu8 Yellow()
	{
		return ColorARGBu8::Create(255u, 255u, 0u, 255u);
	}

	static ColorARGBu8 White()
	{
		return ColorARGBu8::Create(255u, 255u, 255u, 255u);
	}

	static ColorARGBu8 Create(UInt8 r, UInt8 g, UInt8 b, UInt8 a)
	{
		ColorARGBu8 ret;
		ret.m_R = r;
		ret.m_G = g;
		ret.m_B = b;
		ret.m_A = a;
		return ret;
	}

	static ColorARGBu8 CreateFromFloat(Float fR, Float fG, Float fB, Float fA)
	{
		ColorARGBu8 ret;
		ret.m_R = (UInt8)Clamp(fR * 255.0f + 0.5f, 0.0f, 255.0f);
		ret.m_G = (UInt8)Clamp(fG * 255.0f + 0.5f, 0.0f, 255.0f);
		ret.m_B = (UInt8)Clamp(fB * 255.0f + 0.5f, 0.0f, 255.0f);
		ret.m_A = (UInt8)Clamp(fA * 255.0f + 0.5f, 0.0f, 255.0f);
		return ret;
	}

	union
	{
		struct
		{
			UInt8 m_B;
			UInt8 m_G;
			UInt8 m_R;
			UInt8 m_A;
		};
		UInt32 m_Value;
	};
}; // struct ColorARGBu8
template <> struct CanMemCpy<ColorARGBu8> { static const Bool Value = true; };
template <> struct CanZeroInit<ColorARGBu8> { static const Bool Value = true; };
static inline Bool operator==(ColorARGBu8 a, ColorARGBu8 b)
{
	return (a.m_Value == b.m_Value);
}
static inline Bool operator!=(ColorARGBu8 a, ColorARGBu8 b)
{
	return !(a == b);
}

/**
 * Finds the linearly interpolated (LERP) color
 * at factor fFactor between color a and color b.
 */
inline ColorARGBu8 Lerp(ColorARGBu8 a, ColorARGBu8 b, Float fFactor)
{
	return ColorARGBu8::Create(
		(UInt8)Min(Lerp((Float)a.m_R, (Float)b.m_R, fFactor) + 0.5f, 255.0f),
		(UInt8)Min(Lerp((Float)a.m_G, (Float)b.m_G, fFactor) + 0.5f, 255.0f),
		(UInt8)Min(Lerp((Float)a.m_B, (Float)b.m_B, fFactor) + 0.5f, 255.0f),
		(UInt8)Min(Lerp((Float)a.m_A, (Float)b.m_A, fFactor) + 0.5f, 255.0f));
}

static inline UInt32 GetHash(ColorARGBu8 c)
{
	return Seoul::GetHash(c.m_Value);
}

template <>
struct DefaultHashTableKeyTraits<ColorARGBu8>
{
	inline static Float GetLoadFactor()
	{
		return 0.75f;
	}

	inline static ColorARGBu8 GetNullKey()
	{
		ColorARGBu8 ret;
		ret.m_Value = 0;
		return ret;
	}

	static const Bool kCheckHashBeforeEquals = false;
};

// TODO: Update naming to clarify this vs. ColorARGBu8
// (this should be called ColorABGRu8).

struct RGBA SEOUL_SEALED
{
	static RGBA Black()
	{
		return Create(0, 0, 0, 255);
	}

	static RGBA Create(UInt8 uR, UInt8 uG, UInt8 uB, UInt8 uA)
	{
		RGBA ret;
		ret.m_R = uR;
		ret.m_G = uG;
		ret.m_B = uB;
		ret.m_A = uA;
		return ret;
	}

	static RGBA Create(ColorARGBu8 c)
	{
		RGBA ret;
		ret.m_R = c.m_R;
		ret.m_G = c.m_G;
		ret.m_B = c.m_B;
		ret.m_A = c.m_A;
		return ret;
	}

	static RGBA Create(RGBA c)
	{
		return c;
	}

	static RGBA TransparentBlack()
	{
		return Create(0, 0, 0, 0);
	}

	static RGBA TransparentWhite()
	{
		return Create(255, 255, 255, 0);
	}

	static RGBA White()
	{
		return Create(255, 255, 255, 255);
	}

	RGBA()
		: m_R(0)
		, m_G(0)
		, m_B(0)
		, m_A(0)
	{
	}

	RGBA& operator+=(RGBA b);
	RGBA& operator-=(RGBA b);
	RGBA& operator*=(RGBA b);

	Bool operator==(RGBA b) const
	{
		return (
			m_R == b.m_R &&
			m_G == b.m_G &&
			m_B == b.m_B &&
			m_A == b.m_A);
	}

	Bool operator!=(RGBA b) const
	{
		return !(*this == b);
	}

	union
	{
		struct
		{
			UInt8 m_R;
			UInt8 m_G;
			UInt8 m_B;
			UInt8 m_A;
		};
		UInt32 m_Value;
	};
}; // struct RGBA
template <> struct CanMemCpy<RGBA> { static const Bool Value = true; };
template <> struct CanZeroInit<RGBA> { static const Bool Value = true; };

static inline RGBA operator+(RGBA a, RGBA b)
{
	RGBA ret;
	ret.m_R = (a.m_R + b.m_R);
	ret.m_G = (a.m_G + b.m_G);
	ret.m_B = (a.m_B + b.m_B);
	ret.m_A = (a.m_A + b.m_A);
	return ret;
}

static inline RGBA operator-(RGBA a, RGBA b)
{
	RGBA ret;
	ret.m_R = (a.m_R - b.m_R);
	ret.m_G = (a.m_G - b.m_G);
	ret.m_B = (a.m_B - b.m_B);
	ret.m_A = (a.m_A - b.m_A);
	return ret;
}

static inline RGBA operator*(RGBA a, RGBA b)
{
	RGBA ret;
	ret.m_R = (UInt8)Min((((Float)a.m_R * (Float)b.m_R) / 255.0f) + 0.5f, 255.0f);
	ret.m_G = (UInt8)Min((((Float)a.m_G * (Float)b.m_G) / 255.0f) + 0.5f, 255.0f);
	ret.m_B = (UInt8)Min((((Float)a.m_B * (Float)b.m_B) / 255.0f) + 0.5f, 255.0f);
	ret.m_A = (UInt8)Min((((Float)a.m_A * (Float)b.m_A) / 255.0f) + 0.5f, 255.0f);
	return ret;
}

inline RGBA& RGBA::operator+=(RGBA b)
{
	*this = (*this + b);
	return *this;
}

inline RGBA& RGBA::operator-=(RGBA b)
{
	*this = (*this - b);
	return *this;
}

inline RGBA& RGBA::operator*=(RGBA b)
{
	*this = (*this * b);
	return *this;
}

static inline RGBA Lerp(RGBA a, RGBA b, Float fWeightOfB)
{
	RGBA ret;
	ret.m_R = (UInt8)Min(Lerp((Float)a.m_R, (Float)b.m_R, fWeightOfB) + 0.5f, 255.0f);
	ret.m_G = (UInt8)Min(Lerp((Float)a.m_G, (Float)b.m_G, fWeightOfB) + 0.5f, 255.0f);
	ret.m_B = (UInt8)Min(Lerp((Float)a.m_B, (Float)b.m_B, fWeightOfB) + 0.5f, 255.0f);
	ret.m_A = (UInt8)Min(Lerp((Float)a.m_A, (Float)b.m_A, fWeightOfB) + 0.5f, 255.0f);
	return ret;
}

static inline RGBA PremultiplyAlpha(RGBA c)
{
	Int32 const iA = (Int32)c.m_A;
	
	RGBA ret;
	ret.m_R = (UInt8)(((Int32)c.m_R * iA + 127) / 255);
	ret.m_G = (UInt8)(((Int32)c.m_G * iA + 127) / 255);
	ret.m_B = (UInt8)(((Int32)c.m_B * iA + 127) / 255);
	ret.m_A = c.m_A;
	return ret;
}

/**
 * A Color4 stores RGBA values in 4 Floats, each having range 0.0-1.0
 */
struct Color4 SEOUL_SEALED
{
	// Float channel values in range 0.0-1.0
	Float R, G, B, A;

	Color4()
		: R(0.0f)
		, G(0.0f)
		, B(0.0f)
		, A(0.0f)
	{
	}

	Color4(Float r, Float g, Float b, Float a)
		: R(r)
		, G(g)
		, B(b)
		, A(a)
	{
	}

	Color4(const Color4& other)
		: R(other.R)
		, G(other.G)
		, B(other.B)
		, A(other.A)
	{
	}

	explicit Color4(const ColorARGBu8& other)
		: R(other.m_R / 255.0f)
		, G(other.m_G / 255.0f)
		, B(other.m_B / 255.0f)
		, A(other.m_A / 255.0f)
	{
	}

	/**
	 * Assign the values of b to this Color4.
	 */
	Color4& operator=(const Color4& b)
	{
		R = b.R;
		G = b.G;
		B = b.B;
		A = b.A;

		return *this;
	}

	Bool operator==(const Color4& b) const
	{
		return (
			R == b.R &&
			G == b.G &&
			B == b.B &&
			A == b.A);
	}

	Bool operator!=(const Color4& b) const
	{
		return !(*this == b);
	}

	Color4& operator+=(const Color4& b)
	{
		R += b.R;
		G += b.G;
		B += b.B;
		A += b.A;

		return *this;
	}

	Color4& operator-=(const Color4& b)
	{
		R -= b.R;
		G -= b.G;
		B -= b.B;
		A -= b.A;

		return *this;
	}

	Float const* GetData() const { return &R; }
	Float* GetData() { return &R; }

	/**
	 * @return this Color4 as a ColorARGBu8.
	 */
	ColorARGBu8 ToColorARGBu8() const
	{
		return ColorARGBu8::Create(
			(UInt8)Clamp(255.0f * R + 0.5f, 0.0f, 255.0f),
			(UInt8)Clamp(255.0f * G + 0.5f, 0.0f, 255.0f),
			(UInt8)Clamp(255.0f * B + 0.5f, 0.0f, 255.0f),
			(UInt8)Clamp(255.0f * A + 0.5f, 0.0f, 255.0f));
	}

	/**
	 * @return this Color4 as an RGBA.
	 */
	RGBA ToRGBA() const
	{
		return RGBA::Create(
			(UInt8)Clamp(255.0f * R + 0.5f, 0.0f, 255.0f),
			(UInt8)Clamp(255.0f * G + 0.5f, 0.0f, 255.0f),
			(UInt8)Clamp(255.0f * B + 0.5f, 0.0f, 255.0f),
			(UInt8)Clamp(255.0f * A + 0.5f, 0.0f, 255.0f));
	}

	/** @return this Color4 as an equivalent Vector4D. */
	Vector4D ToVector4D() const
	{
		return Vector4D(R, G, B, A);
	}

	static Color4 Black()
	{
		return Color4(0.0f, 0.0f, 0.0f, 1.0f);
	}

	static Color4 Magenta()
	{
		return Color4(1.0f, 0.0f, 1.0f, 1.0f);
	}

	static Color4 White()
	{
		return Color4(1.0f, 1.0f, 1.0f, 1.0f);
	}
};
template <> struct CanMemCpy<Color4> { static const Bool Value = true; };
template <> struct CanZeroInit<Color4> { static const Bool Value = true; };

inline Color4 operator-(const Color4& a, const Color4 b)
{
	return Color4(
		a.R - b.R,
		a.G - b.G,
		a.B - b.B,
		a.A - b.A);
}

inline Color4 operator+(const Color4& a, const Color4 b)
{
	return Color4(
		a.R + b.R,
		a.G + b.G,
		a.B + b.B,
		a.A + b.A);
}

/**
 * Finds the linearly interpolated (LERP) color
 * at factor fFactor between color a and color b.
 */
inline Color4 Lerp(const Color4& a, const Color4& b, Float fFactor)
{
	return Color4(
		Seoul::Lerp(a.R, b.R, fFactor),
		Seoul::Lerp(a.G, b.G, fFactor),
		Seoul::Lerp(a.B, b.B, fFactor),
		Seoul::Lerp(a.A, b.A, fFactor));
}

/**
 * @return c (which c is expected to be an r, g, or b value in non-linear Gamma space) converted into
 * linear gamma space.
 */
inline static Float SRGBDeGamma(UInt8 c)
{
	const Float kThreshold = 0.04045f;
	const Float kAlpha = 0.055f;
	const Float k1 = (Float)(1.0 / 12.92);
	const Float k2 = (Float)(1.0 / (kAlpha + 1.0));
	const Float kPower = 2.4f;

	const Float fC = (c / 255.0f);

	return ((fC <= kThreshold) ? (fC * k1) : Pow(((fC + kAlpha) * k2), kPower));
}

/**
 * Convert non-linear gamma in the sRGB color
 * space to a linear color value.
 */
inline static Color4 SRGBDeGamma(ColorARGBu8 c)
{
	Color4 ret;
	ret.R = SRGBDeGamma(c.m_R);
	ret.G = SRGBDeGamma(c.m_G);
	ret.B = SRGBDeGamma(c.m_B);
	ret.A = (c.m_A / 255.0f);
	return ret;
}

/**
 * Convert linear color value into the sRGB color
 * non-linear color space.
 */
inline static UInt8 SRGBGamma(Float f)
{
	const Float kThreshold = 0.0031308f;
	const Float kAlpha = 0.055f;
	const Float k1 = 12.92f;
	const Float k2 = (kAlpha + 1.0f);
	const Float kPower = (Float)(1.0 / 2.4);

	// Adjust the value to cover the entire [0, 255] sRGB byte range.
	const Float fValue = ((f <= kThreshold) ? (f * k1) : (k2 * Pow(f, kPower)) - kAlpha);
	const UInt8 returnValue = (UInt8)((fValue * 255.0f) + 0.5f);
	return returnValue;
}

/**
 * Convert linear color value into the sRGB color
 * non-linear color space.
 */
inline static ColorARGBu8 SRGBGamma(const Color4& c)
{
	ColorARGBu8 ret;
	ret.m_R = SRGBGamma(c.R);
	ret.m_G = SRGBGamma(c.G);
	ret.m_B = SRGBGamma(c.B);
	ret.m_A = (Byte)(c.A * 255.0);
	return ret;
}

/**
 * @return An R, G, or B color value in sRGB color space in c
 * to the equivalent linear space color value.
 */
inline static Float FastSRGBDeGamma(UInt8 c)
{
	return g_kaSRGBToLinear[c];
}

/**
 * Equivalent to SRGBDeGamma, but uses a fast lookup table. In almost
 * all cases, you want to call this function instead of SRGBDeGamma.
 */
inline static Color4 FastSRGBDeGamma(ColorARGBu8 c)
{
	Color4 ret;
	ret.R = FastSRGBDeGamma(c.m_R);
	ret.G = FastSRGBDeGamma(c.m_G);
	ret.B = FastSRGBDeGamma(c.m_B);
	ret.A = (c.m_A / 255.0f);
	return ret;
}

/**
 * @return An R, G, or B color value in linear color space in c
 * to the equivalent sRGB color space value.
 */
inline static UInt8 FastSRGBGamma(Float f)
{
	return g_kaLinearToSRGB[(Int)(f * (kLinearToSRGBTableSize - 1))];
}

/**
 * Equivalent to SRGBGamma, but uses a fast lookup table. In almost
 * all cases, you want to call this function instead of SRGBGamma.
 */
inline static ColorARGBu8 FastSRGBGamma(const Color4& c)
{
	ColorARGBu8 ret;
	ret.m_R = FastSRGBGamma(c.R);
	ret.m_G = FastSRGBGamma(c.G);
	ret.m_B = FastSRGBGamma(c.B);
	ret.m_A = (UInt8)(c.A * 255.0);
	return ret;
}

struct ColorAdd SEOUL_SEALED
{
	static ColorAdd Create(UInt8 uR, UInt8 uG, UInt8 uB, UInt8 uBlendingFactor)
	{
		ColorAdd ret;
		ret.m_R = uR;
		ret.m_G = uG;
		ret.m_B = uB;
		ret.m_BlendingFactor = uBlendingFactor;
		return ret;
	}

	static ColorAdd Create(RGBA rgba)
	{
		ColorAdd ret;
		ret.m_R = rgba.m_R;
		ret.m_G = rgba.m_G;
		ret.m_B = rgba.m_B;
		ret.m_BlendingFactor = 0;
		return ret;
	}

	ColorAdd()
		: m_R(0)
		, m_G(0)
		, m_B(0)
		, m_BlendingFactor(0)
	{
	}

	RGBA GetRGB() const
	{
		return RGBA::Create(m_R, m_G, m_B, 0);
	}

	ColorAdd& operator+=(ColorAdd b);
	ColorAdd& operator+=(RGBA b);

	Bool operator==(ColorAdd b) const
	{
		return (
			m_R == b.m_R &&
			m_G == b.m_G &&
			m_B == b.m_B &&
			m_BlendingFactor == b.m_BlendingFactor);
	}

	Bool operator!=(ColorAdd b) const
	{
		return !(*this == b);
	}

	UInt8 m_R;
	UInt8 m_G;
	UInt8 m_B;
	// m_BlendingFactor is used to control alpha-blending vs. additive
	// blending in the shader. A value of 0 is fully alpha-blended,
	// a value of 255 is fully additive blending. A value in-between produces
	// some combination between the 2.
	UInt8 m_BlendingFactor;
}; // struct ColorAdd
template <> struct CanMemCpy<ColorAdd> { static const Bool Value = true; };
template <> struct CanZeroInit<ColorAdd> { static const Bool Value = true; };

static inline ColorAdd operator+(ColorAdd a, ColorAdd b)
{
	ColorAdd ret;
	ret.m_R = (a.m_R + b.m_R);
	ret.m_G = (a.m_G + b.m_G);
	ret.m_B = (a.m_B + b.m_B);

	// TODO: Either this, or a multiply.
	ret.m_BlendingFactor = Max(a.m_BlendingFactor, b.m_BlendingFactor);
	return ret;
}

static inline ColorAdd operator+(ColorAdd a, RGBA b)
{
	ColorAdd ret;
	ret.m_R = (a.m_R + b.m_R);
	ret.m_G = (a.m_G + b.m_G);
	ret.m_B = (a.m_B + b.m_B);
	ret.m_BlendingFactor = a.m_BlendingFactor;
	return ret;
}

static inline ColorAdd operator+(RGBA a, ColorAdd b)
{
	ColorAdd ret;
	ret.m_R = (a.m_R + b.m_R);
	ret.m_G = (a.m_G + b.m_G);
	ret.m_B = (a.m_B + b.m_B);
	ret.m_BlendingFactor = b.m_BlendingFactor;
	return ret;
}

inline ColorAdd& ColorAdd::operator+=(ColorAdd b)
{
	*this = (*this + b);
	return *this;
}

inline ColorAdd& ColorAdd::operator+=(RGBA b)
{
	*this = (*this + b);
	return *this;
}

/**
 * Swap R and B channels of an RGBA8 or
 * BGRA8 value.
 */
static inline UInt32 ColorSwapR8B8(UInt32 u)
{
	return
		((u & 0xFF000000) >> 24) | // A
		((u & 0x00FF0000) >> 8)  | // B or R
		((u & 0x0000FF00) << 8)  | // G
		((u & 0x000000FF) << 24);  // R or B
}

/**
 * Utility function - given a buffer of RGBA8 or
 * BGRA8 data, swap the R and B channels.
 */
static inline void ColorSwapR8B8(UInt32* p, UInt32* const pEnd)
{
	for (; p < pEnd; ++p)
	{
		*p = ColorSwapR8B8(*p);
	}
}

} // namespace Seoul

#endif // include guard
