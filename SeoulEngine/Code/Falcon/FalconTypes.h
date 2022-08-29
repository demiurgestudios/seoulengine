/**
 * \file FalconTypes.h
 * \brief Dumping ground for lots of simple types used
 * by the Falcon project.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

// TODO: Falcon was originally a separate project from Seoul,
// meant to be integrated into arbitrary third party engines without
// a Seoul dependency. After the Seoul merge, some types
// were left redundant and separate (RGBA, Rectangle) and still need
// to be replaced with their Seoul counterparts.
//
// Further, this header is likely too heavy in general. Types
// should be refactored out into smaller header files.

#pragma once
#ifndef FALCON_TYPES_H
#define FALCON_TYPES_H

#include "FalconConstants.h"
#include "FixedArray.h"
#include "HashFunctions.h"
#include "HashTable.h"
#include "HtmlTypes.h"
#include "SharedPtr.h"
#include "Matrix2x3.h"
#include "Prereqs.h"
#include "ScopedPtr.h"
#include "SeoulHString.h"
#include "SeoulMath.h"
#include "StandardVertex2D.h"
#include "StreamBuffer.h"
#include "Vector.h"
#include <float.h>
#include <limits.h>
namespace Seoul { class StreamBuffer; }
extern "C" { typedef struct stbtt_fontinfo stbtt_fontinfo; }

namespace Seoul
{

namespace Falcon
{

struct FontOverrides SEOUL_SEALED
{
	Int32 m_iAscentOverride = -1;
	Int32 m_iDescentOverride = -1;
	Int32 m_iLineGapOverride = -1;
	Float32 m_fRescale = 1.0f;
};

struct Fixed88 SEOUL_SEALED
{
	static Fixed88 One()
	{
		Fixed88 ret;
		ret.m_iValue = 256;
		return ret;
	}

	static Fixed88 Zero()
	{
		Fixed88 ret;
		ret.m_iValue = 0;
		return ret;
	}

	Fixed88()
		: m_iValue(0)
	{
	}

	Float GetFloatValue() const
	{
		return (m_iValue / 256.0f);
	}

	void SetFloatValue(Float fValue)
	{
		m_iValue = (Int16)(fValue * 256.0f);
	}

	union
	{
		Int16 m_iValue;
		struct
		{
			Int8 m_iMinor;
			Int8 m_iMajor;
		};
	};
}; // struct Fixed88

struct Fixed1616 SEOUL_SEALED
{
	static Fixed1616 One()
	{
		Fixed1616 ret;
		ret.m_iValue = 65536;
		return ret;
	}

	static Fixed1616 Zero()
	{
		Fixed1616 ret;
		ret.m_iValue = 0;
		return ret;
	}

	Fixed1616()
		: m_iValue(0)
	{
	}

	double GetDoubleValue() const
	{
		return (m_iValue / 65536.0);
	}

	void SetDoubleValue(double fValue)
	{
		m_iValue = (Int32)(fValue * 65536.0);
	}

	union
	{
		Int32 m_iValue;
		struct
		{
			Int16 m_iMinor;
			Int16 m_iMajor;
		};
	};
}; // struct Fixed1616

struct LabelsTraits
{
	static inline Float GetLoadFactor()
	{
		return 0.75f;
	}

	static inline UInt32 GetNullKey()
	{
		return (UInt32)-1;
	}

	static const Bool kCheckHashBeforeEquals = false;
}; // struct LabelsTraits

struct Rectangle SEOUL_SEALED
{
	static Rectangle Create(Float32 fLeft, Float32 fRight, Float32 fTop, Float32 fBottom)
	{
		Rectangle ret;
		ret.m_fLeft = fLeft;
		ret.m_fRight = fRight;
		ret.m_fTop = fTop;
		ret.m_fBottom = fBottom;
		return ret;
	}

	Rectangle()
		: m_fLeft(0)
		, m_fRight(0)
		, m_fTop(0)
		, m_fBottom(0)
	{
	}

	void AbsorbPoint(Float32 fX, Float32 fY)
	{
		m_fLeft = Seoul::Min(fX, m_fLeft);
		m_fRight = Seoul::Max(fX, m_fRight);
		m_fTop = Seoul::Min(fY, m_fTop);
		m_fBottom = Seoul::Max(fY, m_fBottom);
	}

	void AbsorbPoint(const Vector2D& v)
	{
		AbsorbPoint(v.X, v.Y);
	}

	void Expand(Float fDelta)
	{
		m_fLeft -= fDelta;
		m_fRight += fDelta;
		m_fTop -= fDelta;
		m_fBottom += fDelta;
	}

	Vector2D GetCenter() const
	{
		return Vector2D(
			0.5f * (m_fLeft + m_fRight),
			0.5f * (m_fTop + m_fBottom));
	}

	Float32 GetHeight() const
	{
		return (m_fBottom - m_fTop);
	}

	Float32 GetWidth() const
	{
		return (m_fRight - m_fLeft);
	}

	Bool IsZero() const
	{
		return (GetWidth() == 0.0f || GetHeight() == 0.0f);
	}

	static Rectangle InverseMax()
	{
		return Create(FloatMax, -FloatMax, FloatMax, -FloatMax);
	}

	static Rectangle Max()
	{
		return Create(-FloatMax, FloatMax, -FloatMax, FloatMax);
	}

	static Rectangle Merge(const Rectangle& a, const Rectangle& b)
	{
		Rectangle ret;
		ret.m_fLeft = Seoul::Min(a.m_fLeft, b.m_fLeft);
		ret.m_fRight = Seoul::Max(a.m_fRight, b.m_fRight);
		ret.m_fTop = Seoul::Min(a.m_fTop, b.m_fTop);
		ret.m_fBottom = Seoul::Max(a.m_fBottom, b.m_fBottom);
		return ret;
	}

	Bool operator==(const Rectangle& b) const
	{
		return
			(m_fLeft == b.m_fLeft) &&
			(m_fRight == b.m_fRight) &&
			(m_fTop == b.m_fTop) &&
			(m_fBottom == b.m_fBottom);
	}

	Bool operator!=(const Rectangle& b) const
	{
		return !(*this == b);
	}

	Float32 m_fLeft;
	Float32 m_fRight;
	Float32 m_fTop;
	Float32 m_fBottom;
}; // struct Rectangle

// Return true if a completely contains b, false otherwise.
static inline Bool Contains(const Rectangle& a, const Rectangle& b)
{
	if (a.m_fLeft > b.m_fLeft) { return false; }
	if (a.m_fTop > b.m_fTop) { return false; }
	if (a.m_fRight < b.m_fRight) { return false; }
	if (a.m_fBottom < b.m_fBottom) { return false; }

	return true;
}

// Return true if a intersects b at all, false otherwise.
static inline Bool Intersects(const Rectangle& a, const Rectangle& b)
{
	if (a.m_fLeft >= b.m_fRight) { return false; }
	if (a.m_fTop >= b.m_fBottom) { return false; }
	if (a.m_fRight <= b.m_fLeft) { return false; }
	if (a.m_fBottom <= b.m_fTop) { return false; }

	return true;
}

struct ColorTransform SEOUL_SEALED
{
	static ColorTransform Identity()
	{
		ColorTransform ret;
		ret.m_fMulR = 1.0f;
		ret.m_fMulG = 1.0f;
		ret.m_fMulB = 1.0f;
		ret.m_AddR = 0;
		ret.m_AddG = 0;
		ret.m_AddB = 0;
		return ret;
	}

	ColorTransform()
		: m_fMulR(0.0f)
		, m_fMulG(0.0f)
		, m_fMulB(0.0f)
		, m_AddR(0)
		, m_AddG(0)
		, m_AddB(0)
	{
	}

	ColorTransform(Float fMulR, Float fMulG, Float fMulB, UInt8 uAddR, UInt8 uAddG, UInt8 uAddB)
		: m_fMulR(fMulR)
		, m_fMulG(fMulG)
		, m_fMulB(fMulB)
		, m_AddR(uAddR)
		, m_AddG(uAddG)
		, m_AddB(uAddB)
	{
	}

	Bool operator==(const ColorTransform& b) const
	{
		return
			(m_fMulR == b.m_fMulR) &&
			(m_fMulG == b.m_fMulG) &&
			(m_fMulB == b.m_fMulB) &&
			(m_AddR == b.m_AddR) &&
			(m_AddG == b.m_AddG) &&
			(m_AddB == b.m_AddB);
	}

	Bool operator!=(const ColorTransform& b) const
	{
		return !(*this == b);
	}

	Float m_fMulR;
	Float m_fMulG;
	Float m_fMulB;
	UInt8 m_AddR;
	UInt8 m_AddG;
	UInt8 m_AddB;
}; // struct ColorTransform

static inline ColorTransform operator*(const ColorTransform& a, const ColorTransform& b)
{
	ColorTransform ret;
	ret.m_AddR = (UInt8)Clamp((Float)a.m_AddR + a.m_fMulR * (Float)b.m_AddR + 0.5f, 0.0f, 255.0f);
	ret.m_fMulR = (a.m_fMulR * b.m_fMulR);
	ret.m_AddG = (UInt8)Clamp((Float)a.m_AddG + a.m_fMulG * (Float)b.m_AddG + 0.5f, 0.0f, 255.0f);
	ret.m_fMulG = (a.m_fMulG * b.m_fMulG);
	ret.m_AddB = (UInt8)Clamp((Float)a.m_AddB + a.m_fMulB * (Float)b.m_AddB + 0.5f, 0.0f, 255.0f);
	ret.m_fMulB = (a.m_fMulB * b.m_fMulB);
	return ret;
}

static inline RGBA TransformColor(const ColorTransform& m, RGBA rgba)
{
	return RGBA::Create(
		(UInt8)Clamp((Float)rgba.m_R * m.m_fMulR + (Float)m.m_AddR + 0.5f, 0.0f, 255.0f),
		(UInt8)Clamp((Float)rgba.m_G * m.m_fMulG + (Float)m.m_AddG + 0.5f, 0.0f, 255.0f),
		(UInt8)Clamp((Float)rgba.m_B * m.m_fMulB + (Float)m.m_AddB + 0.5f, 0.0f, 255.0f),
		rgba.m_A);
}

struct ColorTransformWithAlpha SEOUL_SEALED
{
	static ColorTransformWithAlpha Identity()
	{
		ColorTransformWithAlpha ret;
		ret.m_fMulR = 1.0f;
		ret.m_fMulG = 1.0f;
		ret.m_fMulB = 1.0f;
		ret.m_fMulA = 1.0f;
		ret.m_AddR = 0;
		ret.m_AddG = 0;
		ret.m_AddB = 0;
		ret.m_uBlendingFactor = 0;
		return ret;
	}

	ColorTransformWithAlpha()
		: m_fMulR(0.0f)
		, m_fMulG(0.0f)
		, m_fMulB(0.0f)
		, m_fMulA(0.0f)
		, m_AddR(0)
		, m_AddG(0)
		, m_AddB(0)
		, m_uBlendingFactor(0)
	{
	}

	ColorTransform GetTransform() const
	{
		return ColorTransform(m_fMulR, m_fMulG, m_fMulB, m_AddR, m_AddG, m_AddB);
	}

	void SetTransform(const ColorTransform& m)
	{
		m_fMulR = m.m_fMulR;
		m_fMulG = m.m_fMulG;
		m_fMulB = m.m_fMulB;
		m_AddR = m.m_AddR;
		m_AddG = m.m_AddG;
		m_AddB = m.m_AddB;
	}

	Bool operator==(const ColorTransformWithAlpha& b) const
	{
		return
			(m_fMulR == b.m_fMulR) &&
			(m_fMulG == b.m_fMulG) &&
			(m_fMulB == b.m_fMulB) &&
			(m_fMulA == b.m_fMulA) &&
			(m_AddR == b.m_AddR) &&
			(m_AddG == b.m_AddG) &&
			(m_AddB == b.m_AddB) &&
			(m_uBlendingFactor == b.m_uBlendingFactor);
	}

	Bool operator!=(const ColorTransformWithAlpha& b) const
	{
		return !(*this == b);
	}

	Float m_fMulR;
	Float m_fMulG;
	Float m_fMulB;
	Float m_fMulA;
	UInt8 m_AddR;
	UInt8 m_AddG;
	UInt8 m_AddB;
	UInt8 m_uBlendingFactor;
}; // struct ColorTransformWithAlpha
SEOUL_STATIC_ASSERT(sizeof(ColorTransformWithAlpha) == 20);

static inline RGBA TransformColor(const ColorTransformWithAlpha& m, RGBA rgba)
{
	return RGBA::Create(
		(UInt8)Clamp((Float)rgba.m_R * m.m_fMulR + (Float)m.m_AddR + 0.5f, 0.0f, 255.0f),
		(UInt8)Clamp((Float)rgba.m_G * m.m_fMulG + (Float)m.m_AddG + 0.5f, 0.0f, 255.0f),
		(UInt8)Clamp((Float)rgba.m_B * m.m_fMulB + (Float)m.m_AddB + 0.5f, 0.0f, 255.0f),
		(UInt8)Clamp((Float)rgba.m_A * m.m_fMulA + 0.5f, 0.0f, 255.0f));
}

static inline ColorTransformWithAlpha operator*(const ColorTransformWithAlpha& a, const ColorTransformWithAlpha& b)
{
	ColorTransformWithAlpha ret;
	ret.m_AddR = (UInt8)Clamp((Float)a.m_AddR + a.m_fMulR * (Float)b.m_AddR + 0.5f, 0.0f, 255.0f);
	ret.m_fMulR = (a.m_fMulR * b.m_fMulR);
	ret.m_AddG = (UInt8)Clamp((Float)a.m_AddG + a.m_fMulG * (Float)b.m_AddG + 0.5f, 0.0f, 255.0f);
	ret.m_fMulG = (a.m_fMulG * b.m_fMulG);
	ret.m_AddB = (UInt8)Clamp((Float)a.m_AddB + a.m_fMulB * (Float)b.m_AddB + 0.5f, 0.0f, 255.0f);
	ret.m_fMulB = (a.m_fMulB * b.m_fMulB);
	ret.m_uBlendingFactor = Max(a.m_uBlendingFactor, b.m_uBlendingFactor);
	ret.m_fMulA = (a.m_fMulA * b.m_fMulA);
	return ret;
}

typedef Seoul::StandardVertex2D ShapeVertex;

enum class TagId
{
	kEnd = 0,
	kShowFrame = 1,
	kDefineShape = 2,
	kPlaceObject = 4,
	kRemoveObject = 5,
	kDefineBits = 6,
	kDefineButton = 7,
	kJPEGTables = 8,
	kSetBackgroundColor = 9,
	kDefineFont = 10,
	kDefineText = 11,
	kDoAction = 12,
	kDefineFontInfo = 13,
	kDefineSound = 14,
	kStartSound = 15,
	kDefineButtonSound = 17,
	kSoundStreamHead = 18,
	kSoundStreamBlock = 19,
	kDefineBitsLossless = 20,
	kDefineBitsJPEG2 = 21,
	kDefineShape2 = 22,
	kDefineButtonCxform = 23,
	kProtect = 24,
	kPlaceObject2 = 26,
	kRemoveObject2 = 28,
	kDefineShape3 = 32,
	kDefineText2 = 33,
	kDefineButton2 = 34,
	kDefineBitsJPEG3 = 35,
	kDefineBitsLossless2 = 36,
	kDefineEditText = 37,
	kDefineSprite = 39,
	kProductInfo = 41, // undocumented tag written by MXMLC, see: http://wahlers.com.br/claus/blog/undocumented-swf-tags-written-by-mxmlc/
	kFrameLabel = 43,
	kSoundStreamHead2 = 45,
	kDefineMorphShape = 46,
	kDefineFont2 = 48,
	kExportAssets = 56,
	kImportAssets = 57,
	kEnableDebugger = 58,
	kDoInitAction = 59,
	kDefineVideoStream = 60,
	kVideoFrame = 61,
	kDefineFontInfo2 = 62,
	kDebugID = 63, // undocumented tag written by MXMLC, see: http://wahlers.com.br/claus/blog/undocumented-swf-tags-written-by-mxmlc/
	kEnableDebugger2 = 64,
	kScriptLimits = 65,
	kSetTabIndex = 66,
	kFileAttributes = 69,
	kPlaceObject3 = 70,
	kImportAssets2 = 71,
	kDefineFontAlignZones = 73,
	kCSMTextSettings = 74,
	kDefineFont3 = 75,
	kSymbolClass = 76,
	kMetadata = 77,
	kDefineScalingGrid = 78,
	kDoABC = 82,
	kDefineShape4 = 83,
	kDefineMorphShape2 = 84,
	kDefineSceneAndFrameLabelData = 86,
	kDefineBinaryData = 87,
	kDefineFontName = 88,
	kStartSound2 = 89,
	kDefineBitsJPEG4 = 90,
	kDefineFont4 = 91,

	// Falcon custom tags.
	kDefineExternalBitmap = 92,
	kDefineFontTrueType = 93,
	kDefineSimpleActions = 94,
};

enum class UseFlashType
{
	kNormalRenderer = 0,
	kAdvancedTextRendering = 1,
};

enum class GridFit
{
	kDoNotUseGridFitting = 0,
	kPixelGridFit = 1,
	kSubPixelGridFit = 2
};

enum class BitmapFormat1
{
	kColormappedImage8Bit = 3,
	kRGBImage15Bit = 4,
	kRGBImage24Bit = 5
};

enum class BitmapFormat2
{
	kColormappedImage8Bit = 3,
	kARGBImage32Bit = 5
};

enum class BlendMode
{
	kNormal0 = 0,
	kNormal1 = 1,
	kLayer = 2,
	kMultiply = 3,
	kScreen = 4,
	kLighten = 5,
	kDarken = 6,
	kDifference = 7,
	kAdd = 8,
	kSubtract = 9,
	kInvert = 10,
	kAlpha = 11,
	kErase = 12,
	kOverlay = 13,
	kHardlight = 14
};

enum class GradientSpreadMode
{
	kPadMode = 0,
	kReflectMode = 1,
	kRepeatMode = 2,
	kReserved = 3
};

enum class GradientInterpolationMode
{
	kNormalRGBMode = 0,
	kLinearRGBMode = 1,
	kReserved2 = 2,
	kReserved3 = 3
};

struct GradientRecord
{
	GradientRecord()
		: m_uRatio(0)
		, m_Color()
	{
	}

	UInt8 m_uRatio;
	RGBA m_Color;
};
typedef Vector<GradientRecord, MemoryBudgets::Falcon> GradientRecords;

struct Gradient
{
	Gradient()
		: m_eSpreadMode((GradientSpreadMode)0)
		, m_eInterpolationMode((GradientInterpolationMode)0)
		, m_vGradientRecords()
		, m_FocalPoint()
		, m_bFocalGradient(false)
	{
	}

	GradientSpreadMode m_eSpreadMode;
	GradientInterpolationMode m_eInterpolationMode;
	GradientRecords m_vGradientRecords;
	Fixed88 m_FocalPoint;
	Bool m_bFocalGradient;
};

enum class FillStyleType
{
	kSoldFill = 0x00,
	kLinearGradientFill = 0x10,
	kRadialGradientFill = 0x12,
	kFocalRadialGradientFill = 0x13,
	kRepeatingBitmapFill = 0x40,
	kClippedBitmapFill = 0x41,
	kNonSmoothedRepeatingBitmapFill = 0x42,
	kNonSmoothedClippedBitmapFill = 0x43,
};

static inline Bool IsBitmap(FillStyleType e)
{
	switch (e)
	{
	case FillStyleType::kRepeatingBitmapFill: return true;
	case FillStyleType::kClippedBitmapFill: return true;
	case FillStyleType::kNonSmoothedRepeatingBitmapFill: return true;
	case FillStyleType::kNonSmoothedClippedBitmapFill: return true;
	default:
		return false;
	};
}

static inline Bool IsGradientFill(FillStyleType e)
{
	switch (e)
	{
	case FillStyleType::kLinearGradientFill: return true;
	case FillStyleType::kRadialGradientFill: return true;
	case FillStyleType::kFocalRadialGradientFill: return true;
	default:
		return false;
	};
}

struct FillStyle
{
	FillStyle()
		: m_eFillStyleType((FillStyleType)0)
		, m_Color()
		, m_mGradientTransform()
		, m_Gradient()
		, m_uBitmapId(0)
		, m_mBitmapTransform()
	{
	}

	FillStyleType m_eFillStyleType;
	RGBA m_Color;
	Matrix2x3 m_mGradientTransform;
	Gradient m_Gradient;
	UInt16 m_uBitmapId;
	Matrix2x3 m_mBitmapTransform;
};

struct LineStyle
{
	LineStyle()
		: m_uWidth(0)
		, m_Color()
	{
	}

	UInt16 m_uWidth;
	RGBA m_Color;
};

enum class LanguageCode
{
	kLatin = 1,
	kJapanese = 2,
	kKorean = 3,
	kSimplifiedChinese = 4,
	kTraditionalChinese = 5
};

struct KerningRecord
{
	KerningRecord()
		: m_iFontKerningCode1(0)
		, m_iFontKerningCode2(0)
		, m_iFontKerningAdjustment(0)
	{
	}

	Int32 m_iFontKerningCode1;
	Int32 m_iFontKerningCode2;
	Int32 m_iFontKerningAdjustment;
};

struct FontLegalDetail
{
	FontLegalDetail()
		: m_iFontID(0)
		, m_sFontName()
		, m_sFontCopyright()
	{
	}

	Int32 m_iFontID;
	HString m_sFontName;
	HString m_sFontCopyright;
};

enum class ShapeRecordFlags : UInt32
{
	kEndShape = 0,
	kStateNewStyles = (1 << 4),
	kStateLineStyle = (1 << 3),
	kStateFillStyle1 = (1 << 2),
	kStateFillStyle0 = (1 << 1),
	kStateMoveTo = (1 << 0),
};
static inline constexpr ShapeRecordFlags operator|(ShapeRecordFlags a, ShapeRecordFlags b)
{
	return (ShapeRecordFlags)((UInt32)a | (UInt32)b);
}
static inline constexpr ShapeRecordFlags operator&(ShapeRecordFlags a, ShapeRecordFlags b)
{
	return (ShapeRecordFlags)((UInt32)a & (UInt32)b);
}

enum class SegmentFlags : UInt32
{
	kFillStyle0 = (1 << 0),
	kFillStyle1 = (1 << 1),
	kLineStyle = (1 << 2),
};
static inline constexpr SegmentFlags operator|(SegmentFlags a, SegmentFlags b)
{
	return (SegmentFlags)((UInt32)a | (UInt32)b);
}
static inline constexpr SegmentFlags operator&(SegmentFlags a, SegmentFlags b)
{
	return (SegmentFlags)((UInt32)a & (UInt32)b);
}

struct FrameEventTraits
{
	static inline Float GetLoadFactor()
	{
		return 0.75f;
	}

	static inline UInt16 GetNullKey()
	{
		return (UInt16)-1;
	}

	static const Bool kCheckHashBeforeEquals = false;
}; // struct FrameEventTraits

enum class SimpleActionValueType
{
	kFalse,
	kNull,
	kNumber,
	kString,
	kTrue,
};

struct SimpleActionValue
{
	SimpleActionValue()
		: m_fValue(0.0)
		, m_eType(SimpleActionValueType::kNull)
	{
	}

	union
	{
		double m_fValue;
		HStringData::InternalIndexType m_hValue;
	};
	SimpleActionValueType m_eType;
}; // struct SimpleActionValue

struct SimpleActions
{
	enum VisibleChange
	{
		kNoVisibleChange = -1,
		kSetVisibleFalse,
		kSetVisibleTrue,
	};

	enum class EventType
	{
		kEventDispatch,
		kEventDispatchBubble,
	};

	typedef HashTable<HString, SimpleActionValue, MemoryBudgets::Falcon> Properties;
	typedef HashTable< HString, Properties, MemoryBudgets::Falcon > PerChildProperties;
	typedef Vector<Pair<HString, EventType>, MemoryBudgets::Falcon> Events;

	struct FrameActions
	{
		FrameActions()
			: m_vEvents()
			, m_tPerChildProperties()
			, m_eVisibleChange(kNoVisibleChange)
			, m_bStop(false)
		{
		}

		Events m_vEvents;
		PerChildProperties m_tPerChildProperties;
		VisibleChange m_eVisibleChange;
		Bool m_bStop;
	}; // FrameActions

	typedef HashTable<UInt16, FrameActions, MemoryBudgets::Falcon, FrameEventTraits> FrameActionsTable;
	FrameActionsTable m_tFrameActions;
}; // struct SimpleActions

struct ShapeEdge
{
	ShapeEdge()
		: m_fAnchorX(0.0f)
		, m_fAnchorY(0.0f)
		, m_fControlX(0.0f)
		, m_fControlY(0.0f)
	{
	}

	Float m_fAnchorX;
	Float m_fAnchorY;
	Float m_fControlX;
	Float m_fControlY;
};

struct Glyph SEOUL_SEALED
{
	Glyph()
		: m_fTX0(0.0f)
		, m_fTY0(0.0f)
		, m_fTX1(0.0f)
		, m_fTY1(0.0f)
		, m_fXOffset(0.0f)
		, m_fYOffset(0.0f)
		, m_fXAdvance(0.0f)
		, m_fWidth(0.0f)
		, m_fHeight(0.0f)
		, m_fTextHeight(0.0f)
	{
	}

	Float m_fTX0;
	Float m_fTY0;
	Float m_fTX1;
	Float m_fTY1;
	Float m_fXOffset;
	Float m_fYOffset;
	Float m_fXAdvance;
	Float m_fWidth;
	Float m_fHeight;
	Float m_fTextHeight;
}; // struct Glyph

struct CookedGlyphEntry
{
	UniChar m_uCodePoint;
	Int32 m_iX0;
	Int32 m_iY0;
	Int32 m_iX1;
	Int32 m_iY1;
	Int32 m_iAdvanceInPixels;
	Int32 m_iLeftSideBearingInPixels;
	Int32 m_iGlyphIndex;
}; // struct CookedGlyphEntry

// Required - serialized to disk, must always be the
// same layout and size.
SEOUL_STATIC_ASSERT(sizeof(CookedGlyphEntry) == 32);

class CookedTrueTypeFontData SEOUL_SEALED
{
public:
	CookedTrueTypeFontData(
		const HString& sUniqueIdentifier,
		void* pData,
		UInt32 uSizeInBytes);
	~CookedTrueTypeFontData();

	Float ComputeLineHeightFromTextHeight(const FontOverrides& overrides, Float fTextHeight) const
	{
		return (GetAscent(overrides) - GetDescent(overrides)) * GetScaleForPixelHeight(fTextHeight);
	}

	Int32 GetAscent(const FontOverrides& overrides) const
	{
		if (overrides.m_iAscentOverride >= 0)
		{
			return (Int32)Ceil(m_fGlyphScaleSDF * (Float32)overrides.m_iAscentOverride);
		}
		else
		{
			return m_iAscent;
		}
	}

	Int32 GetDescent(const FontOverrides& overrides) const
	{
		if (overrides.m_iDescentOverride >= 0)
		{
			return (Int32)Ceil(m_fGlyphScaleSDF * (Float32)overrides.m_iDescentOverride);
		}
		else
		{
			return m_iDescent;
		}
	}

	const HString& GetUniqueIdentifier() const
	{
		return m_sUniqueIdentifier;
	}

	Float GetGlyphAdvance(UniChar uCodePoint) const;
	Float GetGlyphAdvance(UniChar uCodePoint, Float fGlyphHeight) const;

	Int32 GetLineGap(const FontOverrides& overrides) const
	{
		if (overrides.m_iLineGapOverride >= 0)
		{
			return (Int32)Ceil(m_fGlyphScaleSDF * (Float32)overrides.m_iLineGapOverride);
		}
		else
		{
			return m_iLineGap;
		}
	}

	CookedGlyphEntry const* GetGlyph(UInt32 uCodePoint) const
	{
		CookedGlyphEntry const* p = nullptr;
		(void)m_tGlyphs.GetValue(uCodePoint, p);
		return p;
	}

	Bool GetGlyphBitmapDataSDF(
		UniChar uCodePoint,
		void*& rpData,
		Int32& riWidth,
		Int32& riHeight) const;

	Float GetOneEmForPixelHeight(Float fPixelHeight) const
	{
		return m_fGlyphScaleSDF * GetScaleForPixelHeight(fPixelHeight);
	}

	Float GetScaleForPixelHeight(Float fPixelHeight) const
	{
		return (fPixelHeight / kfGlyphHeightSDF);
	}

	// Assumes a single line of basic characters.
	Bool Measure(
		String::Iterator iStringBegin,
		String::Iterator iStringEnd,
		const FontOverrides& overrides,
		Float fPixelHeight,
		Float& rfX0,
		Float& rfY0,
		Float& rfWidth,
		Float& rfHeight,
		Bool bIncludeTrailingWhitespace = false) const;

private:
	SEOUL_REFERENCE_COUNTED(CookedTrueTypeFontData);

	typedef HashTable<UniChar, CookedGlyphEntry const*, MemoryBudgets::FalconFont> Glyphs;

	void InitData();

	StreamBuffer m_Data;
	Glyphs m_tGlyphs;
	Int32 m_iAscent;
	Int32 m_iDescent;
	Int32 m_iLineGap;
	HString m_sUniqueIdentifier;
	ScopedPtr<stbtt_fontinfo> m_pInfo;
	Float m_fGlyphScaleSDF;
	Bool m_bHasValidData;

	SEOUL_DISABLE_COPY(CookedTrueTypeFontData);
}; // class CookedTrueTypeFontData

class TrueTypeFontData SEOUL_SEALED
{
public:
	TrueTypeFontData(
		const HString& sUniqueIdentifier,
		void* pData,
		UInt32 uData);
	~TrueTypeFontData();

	Bool Cook(StreamBuffer& r) const;

	Int32 GetAscent() const
	{
		return m_iAscent;
	}

	Int32 GetDescent() const
	{
		return m_iDescent;
	}

	const HString& GetUniqueIdentifier() const
	{
		return m_sUniqueIdentifier;
	}

	Float GetGlyphAdvance(UniChar uCodePoint, Float fGlyphHeight) const;
	Bool GetGlyphBitmapBox(
		UniChar uCodePoint,
		Float fFontScale,
		Int32& riX0,
		Int32& riY0,
		Int32& riX1,
		Int32& riY1) const;

	Int32 GetLineGap() const
	{
		return m_iLineGap;
	}

	Float GetScaleForPixelHeight(Float fPixelHeight) const;

	Bool WriteGlyphBitmap(
		UniChar uCodePoint,
		UInt8* pOut,
		Int32 iGlyphWidth,
		Int32 iGlyphHeight,
		Int32 iPitch,
		Float fFontScale,
		Bool bSDF);

private:
	SEOUL_REFERENCE_COUNTED(TrueTypeFontData);

	void ReleaseData();

	HString m_sUniqueIdentifier;
	Int32 m_iAscent;
	Int32 m_iDescent;
	Int32 m_iLineGap;
	void* m_pTtfData;
	UInt32 m_uTtfData;
	ScopedPtr<stbtt_fontinfo> m_pInfo;
	Bool m_bHasValidData;

	SEOUL_DISABLE_COPY(TrueTypeFontData);
}; // class TrueTypeFontData

} // namespace Falcon

// These types are considered simple by SeoulEngine functions, even though the
// compiler may disagree due to a zero-initialize default constructor.
template <> struct CanMemCpy<Falcon::ColorTransform> { static const Bool Value = true; };
template <> struct CanMemCpy<Falcon::ColorTransformWithAlpha> { static const Bool Value = true; };
template <> struct CanMemCpy<Falcon::Fixed88> { static const Bool Value = true; };
template <> struct CanMemCpy<Falcon::Fixed1616> { static const Bool Value = true; };
template <> struct CanMemCpy<Falcon::FontLegalDetail> { static const Bool Value = true; };
template <> struct CanMemCpy<Falcon::GradientRecord> { static const Bool Value = true; };
template <> struct CanMemCpy<Falcon::KerningRecord> { static const Bool Value = true; };
template <> struct CanMemCpy<Falcon::LineStyle> { static const Bool Value = true; };
template <> struct CanMemCpy<Falcon::Rectangle> { static const Bool Value = true; };
template <> struct CanMemCpy<Falcon::SimpleActionValue> { static const Bool Value = true; };

template <> struct CanZeroInit<Falcon::ColorTransform> { static const Bool Value = true; };
template <> struct CanZeroInit<Falcon::ColorTransformWithAlpha> { static const Bool Value = true; };
template <> struct CanZeroInit<Falcon::Fixed88> { static const Bool Value = true; };
template <> struct CanZeroInit<Falcon::Fixed1616> { static const Bool Value = true; };
template <> struct CanZeroInit<Falcon::FontLegalDetail> { static const Bool Value = true; };
template <> struct CanZeroInit<Falcon::GradientRecord> { static const Bool Value = true; };
template <> struct CanZeroInit<Falcon::KerningRecord> { static const Bool Value = true; };
template <> struct CanZeroInit<Falcon::LineStyle> { static const Bool Value = true; };
template <> struct CanZeroInit<Falcon::Rectangle> { static const Bool Value = true; };

namespace Falcon
{

static inline Rectangle TransformRectangle(const Matrix2x3& m, const Rectangle& rectangle)
{
	Rectangle ret = Rectangle::InverseMax();

	Vector2D v;
	v = Matrix2x3::TransformPosition(m, Vector2D(rectangle.m_fLeft, rectangle.m_fTop));
	ret.AbsorbPoint(v.X, v.Y);
	v = Matrix2x3::TransformPosition(m, Vector2D(rectangle.m_fLeft, rectangle.m_fBottom));
	ret.AbsorbPoint(v.X, v.Y);
	v = Matrix2x3::TransformPosition(m, Vector2D(rectangle.m_fRight, rectangle.m_fTop));
	ret.AbsorbPoint(v.X, v.Y);
	v = Matrix2x3::TransformPosition(m, Vector2D(rectangle.m_fRight, rectangle.m_fBottom));
	ret.AbsorbPoint(v.X, v.Y);

	return ret;
}

static inline Rectangle TransformRectangle(const Matrix2x3& m, const Rectangle& rectangle, Bool& rbMatchesBounds)
{
	Rectangle ret = Rectangle::InverseMax();

	FixedArray<Vector2D, 4> aCorners;
	aCorners[0] = Matrix2x3::TransformPosition(m, Vector2D(rectangle.m_fLeft, rectangle.m_fTop));
	ret.AbsorbPoint(aCorners[0].X, aCorners[0].Y);
	aCorners[1] = Matrix2x3::TransformPosition(m, Vector2D(rectangle.m_fLeft, rectangle.m_fBottom));
	ret.AbsorbPoint(aCorners[1].X, aCorners[1].Y);
	aCorners[2] = Matrix2x3::TransformPosition(m, Vector2D(rectangle.m_fRight, rectangle.m_fTop));
	ret.AbsorbPoint(aCorners[2].X, aCorners[2].Y);
	aCorners[3] = Matrix2x3::TransformPosition(m, Vector2D(rectangle.m_fRight, rectangle.m_fBottom));
	ret.AbsorbPoint(aCorners[3].X, aCorners[3].Y);

	rbMatchesBounds = true;
	for (UInt32 i = 0; i < aCorners.GetSize(); ++i)
	{
		const Vector2D& v = aCorners[i];
		Float32 const fX = v.X;
		if (!Seoul::Equals(ret.m_fLeft, fX, kfAboutEqualPosition) &&
			!Seoul::Equals(ret.m_fRight, fX, kfAboutEqualPosition))
		{
			rbMatchesBounds = false;
			break;
		}

		Float32 const fY = v.Y;
		if (!Seoul::Equals(ret.m_fBottom, fY, kfAboutEqualPosition) &&
			!Seoul::Equals(ret.m_fTop, fY, kfAboutEqualPosition))
		{
			rbMatchesBounds = false;
			break;
		}
	}

	return ret;
}

static inline Bool Intersects(
	const Rectangle& worldRectangle,
	const Matrix2x3& mToWorld,
	const Rectangle& objectRectangle)
{
	// Compute rectangle values in world space.
	Vector2D const vWorldExtents(Vector2D(
		0.5f * worldRectangle.GetWidth(),
		0.5f * worldRectangle.GetHeight()));
	Vector2D const vWorldCenter(
		Vector2D(worldRectangle.m_fLeft, worldRectangle.m_fTop) +
		vWorldExtents);

	// Compute valus and difference.
	Vector2D const vObjectCenter(Matrix2x3::TransformPosition(mToWorld, objectRectangle.GetCenter()));
	Vector2D const vObjectExtents(Vector2D(0.5f * objectRectangle.GetWidth(), 0.5f * objectRectangle.GetHeight()));
	Vector2D const vDiff(vWorldCenter - vObjectCenter);

	// Transform axes into world space and take
	// absolute value to compute "effective radius".
	Vector2D vAbsXY;
	vAbsXY.X = Abs(mToWorld.M00 * vObjectExtents.X) + Abs(mToWorld.M01 * vObjectExtents.Y);
	vAbsXY.Y = Abs(mToWorld.M10 * vObjectExtents.X) + Abs(mToWorld.M11 * vObjectExtents.Y);

	// Compare effective radius in world space against the cull extents,
	// adjusted by offset difference.
	if ((Abs(vDiff.X) - vAbsXY.X) > vWorldExtents.X)
	{
		return false;
	}

	if ((Abs(vDiff.Y) - vAbsXY.Y) > vWorldExtents.Y)
	{
		return false;
	}

	return true;
}

} // namespace Falcon

} // namespace Seoul

#endif // include guard
