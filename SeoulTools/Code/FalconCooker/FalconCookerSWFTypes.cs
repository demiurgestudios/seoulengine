// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing.Imaging;
using System.Linq;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

namespace FalconCooker
{
	[StructLayout(LayoutKind.Explicit)]
	public struct Fixed88
	{
		public static Fixed88 One
		{
			get
			{
				Fixed88 ret = default(Fixed88);
				ret.m_iValue = 256;
				return ret;
			}
		}

		public float FloatValue
		{
			get
			{
				return (m_iValue / 256.0f);
			}

			set
			{
				m_iValue = (Int16)(value * 256.0f);
			}
		}

		[FieldOffset(0)]
		public Int16 m_iValue;

		[FieldOffset(0)]
		public SByte m_iMinor;

		[FieldOffset(1)]
		public SByte m_iMajor;
	}

	[StructLayout(LayoutKind.Explicit)]
	public struct Fixed1616
	{
		public static Fixed1616 One
		{
			get
			{
				Fixed1616 ret = default(Fixed1616);
				ret.m_iValue = 65536;
				return ret;
			}
		}

		public double DoubleValue
		{
			get
			{
				return (m_iValue / 65536.0);
			}

			set
			{
				m_iValue = (Int32)(value * 65536.0);
			}
		}

		[FieldOffset(0)]
		public Int32 m_iValue;

		[FieldOffset(0)]
		public Int16 m_iMinor;

		[FieldOffset(1)]
		public Int16 m_iMajor;
	}

	public struct RGBA
	{
		public byte m_R;
		public byte m_G;
		public byte m_B;
		public byte m_A;
	}

	public struct ColorTransform
	{
		public int m_AddR;
		public int m_AddG;
		public int m_AddB;
		public float m_fMulR;
		public float m_fMulG;
		public float m_fMulB;

		public static readonly ColorTransform Identity = new ColorTransform() { m_fMulR = 1.0f, m_fMulG = 1.0f, m_fMulB = 1.0f };
	}

	public struct ColorTransformWithAlpha
	{
		public ColorTransform m_Transform;
		public int m_AddA;
		public float m_fMulA;

		public static readonly ColorTransformWithAlpha Identity = new ColorTransformWithAlpha() { m_Transform = ColorTransform.Identity, m_fMulA = 1.0f };
	}

	public struct Matrix
	{
		public Fixed1616 m_ScaleX;
		public Fixed1616 m_ScaleY;

		public Fixed1616 m_RotateSkew0;
		public Fixed1616 m_RotateSkew1;

		public int m_iTranslateX;
		public int m_iTranslateY;

		public static Matrix Identity
		{
			get
			{
				Matrix ret = default(Matrix);
				ret.m_ScaleX = Fixed1616.One;
				ret.m_ScaleY = Fixed1616.One;
				return ret;
			}
		}
	}

	public struct Rectangle
	{
        public static Rectangle Create(int iLeft, int iRight, int iTop, int iBottom)
        {
            Rectangle ret;
            ret.m_iLeft = iLeft;
            ret.m_iRight = iRight;
            ret.m_iTop = iTop;
            ret.m_iBottom = iBottom;
            return ret;
        }

		public int m_iLeft;
		public int m_iRight;
		public int m_iTop;
		public int m_iBottom;
	}

	public enum UseFlashType
	{
		NormalRenderer = 0,
		AdvancedTextRendering = 1
	}

	public enum GridFit
	{
		DoNotUseGridFitting = 0,
		PixelGridFit = 1,
		SubPixelGridFit = 2
	}

	public enum Align
	{
		Left = 0,
		Right = 1,
		Center = 2,
		Justify = 3
	}

	public enum BitmapFormat1
	{
		ColormappedImage8Bit = 3,
		RGBImage15Bit = 4,
		RGBImage24Bit = 5
	}

	public enum BitmapFormat2
	{
		ColormappedImage8Bit = 3,
		ARGBImage32Bit = 5
	}

	public enum BlendMode
	{
		Normal0 = 0,
		Normal1 = 1,
		Layer = 2,
		Multiply = 3,
		Screen = 4,
		Lighten = 5,
		Darken = 6,
		Difference = 7,
		Add = 8,
		Subtract = 9,
		Invert = 10,
		Alpha = 11,
		Erase = 12,
		Overlay = 13,
		Hardlight = 14
	}

	public enum GradientSpreadMode
	{
		PadMode = 0,
		ReflectMode = 1,
		RepeatMode = 2,
		Reserved = 3
	}

	public enum GradientInterpolationMode
	{
		NormalRGBMode = 0,
		LinearRGBMode = 1,
		Reserved2 = 2,
		Reserved3 = 3
	}

	public struct GradientRecord
	{
		public byte m_uRatio;
		public RGBA m_Color;
	}

	public struct Gradient
	{
		public GradientSpreadMode m_SpreadMode;
		public GradientInterpolationMode m_InterpolationMode;
		public GradientRecord[] m_aGradientRecords;
		public Fixed88 m_FocalPoint;
		public bool m_bFocalGradient;
	}

	public enum FillStyleType
	{
		SoldFill = 0x00,
		LinearGradientFill = 0x10,
		RadialGradientFill = 0x12,
		FocalRadialGradientFill = 0x13,
		RepeatingBitmapFill = 0x40,
		ClippedBitmapFill = 0x41,
		NonSmoothedRepeatingBitmapFill = 0x42,
		NonSmoothedClippedBitmapFill = 0x43,
	}

	public struct FillStyle
	{
		public FillStyleType m_eFillStyleType;
		public RGBA m_Color;
		public Matrix m_mGradientTransform;
		public Gradient m_Gradient;
		public UInt16 m_uBitmapId;
		public Matrix m_mBitmapTransform;
	}

	public struct LineStyle
	{
		public UInt16 m_uWidth;
		public RGBA m_Color;
	}

	public enum DictionaryObjectType
	{
		Unknown,
		BinaryData,
		Bitmap,
		EditText,
		Font,
		Shape,
		Sprite
	}

	public abstract class DictionaryObject
	{
		public DictionaryObjectType m_eType = DictionaryObjectType.Unknown;

		protected DictionaryObject(DictionaryObjectType eType)
		{
			m_eType = eType;
		}
	}

	public class BinaryData : DictionaryObject
	{
		public BinaryData() : base(DictionaryObjectType.BinaryData) { }

		public byte[] m_aData;
	}

	public class Bitmap : DictionaryObject
	{
		public Bitmap() : base(DictionaryObjectType.Bitmap) { }

		public int m_nWidth;
		public int m_nHeight;
		public bool m_bLossy;
		public byte[] m_aBgraData;
	}

	public class EditText : DictionaryObject
	{
		public EditText() : base(DictionaryObjectType.EditText) { }

		public Rectangle m_Bounds;
		public bool m_bHasText;
		public string m_sVariableName;
		public string m_sInitialText;
		public bool m_bWordWrap;
		public bool m_bMultiline;
		public bool m_bPassword;
		public bool m_bReadOnly;
		public bool m_bHasTextColor;
		public RGBA m_TextColor;
		public bool m_bHasMaxLength;
		public UInt16 m_uMaxLength;
		public bool m_bHasFont;
		public UInt16 m_uFontHeight;
		public UInt16 m_uFontID;
		public bool m_bHasFontClass;
		public string m_sFontClass;
		public bool m_bAutoSize;
		public bool m_bHasLayout;
		public Align m_eAlign;
		public UInt16 m_uLeftMargin = 0;
		public UInt16 m_uRightMargin = 0;
		public UInt16 m_uIndent = 0;
		public Int16 m_iLeading = 0;
		public bool m_bNoSelect;
		public bool m_bBorder;
		public bool m_bWasStatic;
		public bool m_bHtml;
		public bool m_bUseOutlines;
	}

	public enum LanguageCode
	{
		Latin = 1,
		Japanese = 2,
		Korean = 3,
		SimplifiedChinese = 4,
		TraditionalChinese = 5
	}

	public struct KerningRecord
	{
		public int m_iFontKerningCode1;
		public int m_iFontKerningCode2;
		public int m_iFontKerningAdjustment;
	}

	public sealed class ABCData
	{
		public bool m_bLazyInitialize;
		public string m_sName;
		public byte[] m_aBytecode;
		public ABCFile m_Data;
	};

	public class Font : DictionaryObject
	{
		public Font() : base(DictionaryObjectType.Font) { }

		public bool m_bHasLayout;
		public bool m_bShiftJIS;
		public bool m_bSmallText;
		public bool m_bANSI;
		public bool m_bWideOffsets;
		public bool m_bWideCodes;
		public bool m_bItalic;
		public bool m_bBold;
		public LanguageCode m_eLanguageCode;
		public string m_sFontName;
		public int[] m_aOffsetTable;
		public int[] m_aGlyphShapeTable;
		public int[] m_aCodeTable;
		public int m_iFontAscent;
		public int m_iFontDescent;
		public int m_iFontLeading;
		public int[] m_aFontAdvanceTable;
		public Rectangle[] m_aFontBoundsTable;
		public KerningRecord[] m_aFontKerningTable;
	}

	public struct FontLegalDetail
	{
		public int m_iFontID;
		public string m_sFontName;
		public string m_sFontCopyright;
	}

	[Flags]
	public enum ShapeRecordFlags
	{
		EndShape = 0,
		StateNewStyles = (1 << 4),
		StateLineStyle = (1 << 3),
		StateFillStyle1 = (1 << 2),
		StateFillStyle0 = (1 << 1),
		StateMoveTo = (1 << 0),
	}

	[Flags]
	public enum SegmentFlags
	{
		FillStyle0 = (1 << 0),
		FillStyle1 = (1 << 1),
		LineStyle = (1 << 2),
	}

	public struct ShapeEdge
	{
		public float m_fAnchorX;
		public float m_fAnchorY;
		public float m_fControlX;
		public float m_fControlY;
	}

	public struct ShapePath
	{
		public int m_iFillStyle0;
		public int m_iFillStyle1;
		public int m_iLineStyle;
		public float m_fStartX;
		public float m_fStartY;
		public ShapeEdge[] m_aEdges;
		public bool m_bNewShape;
	}

	public class Shape : DictionaryObject
	{
		public Shape() : base(DictionaryObjectType.Shape) { }

		public FillStyle[] m_aFillStyles;
		public LineStyle[] m_aLineStyles;
		public ShapePath[] m_aPaths;
	}

	public enum DisplayListTagType
	{
		Unknown,
		PlaceObject,
		PlaceObject2,
		PlaceObject3,
		RemoveObject,
		RemoveObject2,
		ShowFrame,
	}

	public abstract class DisplayListTag
	{
		public DisplayListTagType m_eType = DisplayListTagType.Unknown;

		protected DisplayListTag(DisplayListTagType eType)
		{
			m_eType = eType;
		}
	}

	public class PlaceObject : DisplayListTag
	{
		public PlaceObject()
			: base(DisplayListTagType.PlaceObject)
		{
		}

		public PlaceObject(DisplayListTagType eType)
			: base(eType)
		{
		}

		public UInt16 m_uDepth = 0;
		public bool m_bHasCharacterClassName;
		public UInt16 m_uCharacterID = 0;
		public string m_sCharacterClassName;
		public ColorTransformWithAlpha m_ColorTransform = ColorTransformWithAlpha.Identity;
		public Matrix m_mTransform = Matrix.Identity;
	}

	public class PlaceObject2 : PlaceObject
	{
		public PlaceObject2()
			: base(DisplayListTagType.PlaceObject2)
		{
		}

		public PlaceObject2(DisplayListTagType eType)
			: base(eType)
		{
		}

		public bool m_bHasClipActions;
		public bool m_bHasClipDepth;
		public bool m_bHasName;
		public bool m_bHasRatio;
		public bool m_bHasColorTransform;
		public bool m_bHasMatrix;
		public bool m_bHasCharacter;
		public bool m_bHasMove;
		public UInt16 m_uRatio;
		public string m_sName;
		public UInt16 m_uClipDepth;
	}

	public sealed class PlaceObject3 : PlaceObject2
	{
		public PlaceObject3()
			: base(DisplayListTagType.PlaceObject3)
		{
		}

		public bool m_bOpaqueBackground;
		public bool m_bHasVisible;
		public bool m_bHasImage;
		public bool m_bHasClassName;
		public bool m_bHasCacheAsBitmap;
		public bool m_bHasBlendMode;
		public bool m_bHasFilterList;
		public string m_sClassName;
		public BlendMode m_eBlendMode = BlendMode.Normal0;
		public byte m_uBitmapCache;
		public bool m_bVisible;
		public RGBA m_BackgroundColor;
	}

	public sealed class RemoveObject : DisplayListTag
	{
		public RemoveObject()
			: base(DisplayListTagType.RemoveObject)
		{
		}
	}

	public sealed class RemoveObject2 : DisplayListTag
	{
		public RemoveObject2()
			: base(DisplayListTagType.RemoveObject2)
		{
		}

		public UInt16 m_uDepth;
	}

	public sealed class ShowFrame : DisplayListTag
	{
		public ShowFrame()
			: base(DisplayListTagType.ShowFrame)
		{
		}
	}

	public class Sprite : DictionaryObject
	{
		public Sprite(int nFrameCount)
			: base(DictionaryObjectType.Sprite)
		{
			m_aFrameOffsets = new int[nFrameCount];
		}

		public int m_nCurrentFrame;
		public int[] m_aFrameOffsets;
		public SortedDictionary<int, string> m_FrameLabels;
		public List<DisplayListTag> m_DisplayListTags = new List<DisplayListTag>();
	}

	public enum TagId
	{
		End = 0,
		ShowFrame = 1,
		DefineShape = 2,
		PlaceObject = 4,
		RemoveObject = 5,
		DefineBits = 6,
		DefineButton = 7,
		JPEGTables = 8,
		SetBackgroundColor = 9,
		DefineFont = 10,
		DefineText = 11,
		DoAction = 12,
		DefineFontInfo = 13,
		DefineSound = 14,
		StartSound = 15,
		DefineButtonSound = 17,
		SoundStreamHead = 18,
		SoundStreamBlock = 19,
		DefineBitsLossless = 20,
		DefineBitsJPEG2 = 21,
		DefineShape2 = 22,
		DefineButtonCxform = 23,
		Protect = 24,
		PlaceObject2 = 26,
		RemoveObject2 = 28,
		DefineShape3 = 32,
		DefineText2 = 33,
		DefineButton2 = 34,
		DefineBitsJPEG3 = 35,
		DefineBitsLossless2 = 36,
		DefineEditText = 37,
		DefineSprite = 39,
		ProductInfo = 41, // undocumented tag written by MXMLC, see: http://wahlers.com.br/claus/blog/undocumented-swf-tags-written-by-mxmlc/
		FrameLabel = 43,
		SoundStreamHead2 = 45,
		DefineMorphShape = 46,
		DefineFont2 = 48,
		ExportAssets = 56,
		ImportAssets = 57,
		EnableDebugger = 58,
		DoInitAction = 59,
		DefineVideoStream = 60,
		VideoFrame = 61,
		DefineFontInfo2 = 62,
		DebugID = 63, // undocumented tag written by MXMLC, see: http://wahlers.com.br/claus/blog/undocumented-swf-tags-written-by-mxmlc/
		EnableDebugger2 = 64,
		ScriptLimits = 65,
		SetTabIndex = 66,
		FileAttributes = 69,
		PlaceObject3 = 70,
		ImportAssets2 = 71,
		DefineFontAlignZones = 73,
		CSMTextSettings = 74,
		DefineFont3 = 75,
		SymbolClass = 76,
		Metadata = 77,
		DefineScalingGrid = 78,
		DoABC = 82,
		DefineShape4 = 83,
		DefineMorphShape2 = 84,
		DefineSceneAndFrameLabelData = 86,
		DefineBinaryData = 87,
		DefineFontName = 88,
		StartSound2 = 89,
		DefineBitsJPEG4 = 90,
		DefineFont4 = 91,

		// Falcon custom tags.
		DefineExternalBitmap = 92,
		DefineFontTrueType = 93,
		DefineMinimalActions = 94,
	}

	public struct SwfFileAttributes
	{
		public bool m_bDirectBlit;
		public bool m_bUseGPU;
		public bool m_bHasMetadata;
		public bool m_bActionScript3;
		public bool m_bUseNetwork;
	}
} // namespace FalconCooker
