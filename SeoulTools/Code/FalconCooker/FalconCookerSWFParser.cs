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
	public abstract class SWFParser
	{
		// Falcon SWFParser currently supports only Flash Player 9 (SWF version 9)
		// through Flash Player 11.4 (SWF version 17) SWF files.
		public const int kiMinimumFlashSwfVersion = 9;
		public const int kiMaximumFlashSwfVersion = 32;
		public static readonly Dictionary<int, string> ktFlashSwfVersionCodeToFlashPlayerVersion = new Dictionary<int, string>()
		{
			{ 9, "Flash Player 9" },
			{ 10, "Flash Player 10.1" },
			{ 11, "Flash Player 10.2" },
			{ 12, "Flash Player 10.3" },
			{ 13, "Flash Player 11.0" },
			{ 14, "Flash Player 11.1" },
			{ 15, "Flash Player 11.2" },
			{ 16, "Flash Player 11.3" },
			{ 17, "Flash Player 11.4" },
			{ 18, "Flash Player 11.5" },
			{ 19, "Flash Player 11.6" },
			{ 20, "Flash Player 11.7" },
			{ 21, "Flash Player 11.8" },
			{ 22, "Flash Player 11.9" },
			{ 23, "Flash Player 12.0" },
			{ 24, "Flash Player 13.0" },
			{ 25, "Flash Player 14.0" },
			{ 26, "Flash Player 15.0" },
			{ 27, "Flash Player 16.0" },
			{ 28, "Flash Player 17.0" },
			{ 29, "Flash Player 18.0" },
			{ 30, "Flash Player 19.0" },
			{ 31, "Flash Player 20.0" },
			{ 32, "Flash Player 21.0" },
		};
		public static string GetFlashPlayerVersionFromSwfVersion(int iFlashSwfVersion)
		{
			string sReturn = string.Empty;
			if (!ktFlashSwfVersionCodeToFlashPlayerVersion.TryGetValue(iFlashSwfVersion, out sReturn))
			{
				sReturn = "Unknown Flash Player version (SWF version '" + iFlashSwfVersion.ToString() + "')";
			}

			return sReturn;
		}

		public static void Swap<T>(ref T a, ref T b)
		{
			T tmp = a;
			a = b;
			b = tmp;
		}

		#region Protected members
		protected List<byte> m_StringBuilder = new List<byte>();
		protected byte[] m_aData = null;
		protected int m_iOffsetInBits = 0;

		protected void Align()
		{
			m_iOffsetInBits = (0 == (m_iOffsetInBits & 0x7)) ? m_iOffsetInBits : ((m_iOffsetInBits & (~(0x7))) + 8);
		}

		protected int OffsetInBits
		{
			get
			{
				return m_iOffsetInBits;
			}

			set
			{
				m_iOffsetInBits = value;
			}
		}

		protected int OffsetInBytes
		{
			get
			{
				return (m_iOffsetInBits >> 3);
			}

			set
			{
				m_iOffsetInBits = ((value << 3) & (~(0x7)));
			}
		}

		protected static byte[] ToBgraBytes(System.Drawing.Bitmap bitmap)
		{
			const int kStride = 4;

			int iWidth = bitmap.Width;
			int iHeight = bitmap.Height;
			int iPitch = (iWidth * kStride);

			// Lock the bitmap data.
			BitmapData dataLock = bitmap.LockBits(
				new System.Drawing.Rectangle(0, 0, iWidth, iHeight),
				ImageLockMode.ReadOnly,
				PixelFormat.Format32bppArgb);

			// Generate an output array of the desired size.
			byte[] aReturn = new byte[iHeight * iPitch];

			// If the lock pitch matches the expected pitch, just copy the entire
			// data in one shot.
			if (iPitch == dataLock.Stride)
			{
				Marshal.Copy(dataLock.Scan0, aReturn, 0, iHeight * iPitch);
			}
			// Otherwise, copy line by line.
			else
			{
				int iLockPitch = dataLock.Stride;
				for (int y = 0; y < iHeight; ++y)
				{
					Marshal.Copy(dataLock.Scan0 + (y * iLockPitch), aReturn, (y * iPitch), iPitch);
				}
			}

			// Unlock the bits.
			bitmap.UnlockBits(dataLock);

			return aReturn;
		}

		protected ColorTransform ReadColorTransform()
		{
			ColorTransform ret = default(ColorTransform);

			Align();

			bool bHasAdd = ReadBit();
			bool bHasMul = ReadBit();

			int nBits = ReadUBits(4);
			if (bHasMul)
			{
				ret.m_fMulR = ReadFBits88(nBits).FloatValue;
				ret.m_fMulG = ReadFBits88(nBits).FloatValue;
				ret.m_fMulB = ReadFBits88(nBits).FloatValue;
			}
			else
			{
				ret.m_fMulR = 1.0f;
				ret.m_fMulG = 1.0f;
				ret.m_fMulB = 1.0f;
			}

			if (bHasAdd)
			{
				ret.m_AddR = ReadSBits(nBits);
				ret.m_AddG = ReadSBits(nBits);
				ret.m_AddB = ReadSBits(nBits);
			}
			else
			{
				ret.m_AddR = 0;
				ret.m_AddG = 0;
				ret.m_AddB = 0;
			}

			return ret;
		}

		protected ColorTransformWithAlpha ReadColorTransformWithAlpha()
		{
			ColorTransformWithAlpha ret = default(ColorTransformWithAlpha);

			Align();

			bool bHasAdd = ReadBit();
			bool bHasMul = ReadBit();

			int nBits = ReadUBits(4);
			if (bHasMul)
			{
				ret.m_Transform.m_fMulR = ReadFBits88(nBits).FloatValue;
				ret.m_Transform.m_fMulG = ReadFBits88(nBits).FloatValue;
				ret.m_Transform.m_fMulB = ReadFBits88(nBits).FloatValue;
				ret.m_fMulA = ReadFBits88(nBits).FloatValue;
			}
			else
			{
				ret.m_Transform.m_fMulR = 1.0f;
				ret.m_Transform.m_fMulG = 1.0f;
				ret.m_Transform.m_fMulB = 1.0f;
				ret.m_fMulA = 1.0f;
			}

			if (bHasAdd)
			{
				ret.m_Transform.m_AddR = ReadSBits(nBits);
				ret.m_Transform.m_AddG = ReadSBits(nBits);
				ret.m_Transform.m_AddB = ReadSBits(nBits);
				ret.m_AddA = ReadSBits(nBits);
			}
			else
			{
				ret.m_Transform.m_AddR = 0;
				ret.m_Transform.m_AddG = 0;
				ret.m_Transform.m_AddB = 0;
				ret.m_AddA = 0;
			}

			return ret;
		}

		protected Fixed88 ReadFixed88()
		{
			Fixed88 ret = default(Fixed88);
			ret.m_iMinor = ReadSByte();
			ret.m_iMajor = ReadSByte();
			return ret;
		}

		protected Fixed1616 ReadFixed1616()
		{
			Fixed1616 ret = default(Fixed1616);
			ret.m_iMinor = ReadInt16();
			ret.m_iMajor = ReadInt16();
			return ret;
		}

		protected KerningRecord ReadKerningRecord(bool bWideCodes)
		{
			KerningRecord ret;
			ret.m_iFontKerningCode1 = (bWideCodes ? (int)ReadUInt16() : (int)ReadByte());
			ret.m_iFontKerningCode2 = (bWideCodes ? (int)ReadUInt16() : (int)ReadByte());
			ret.m_iFontKerningAdjustment = (int)ReadInt16();
			return ret;
		}

		protected Matrix ReadMatrix()
		{
			Matrix ret = default(Matrix);

			Align();

			bool bHasScale = ReadBit();
			if (bHasScale)
			{
				int nScaleBits = ReadUBits(5);
				ret.m_ScaleX = ReadFBits1616(nScaleBits);
				ret.m_ScaleY = ReadFBits1616(nScaleBits);
			}
			else
			{
				ret.m_ScaleX = Fixed1616.One;
				ret.m_ScaleY = Fixed1616.One;
			}

			bool bHasRotate = ReadBit();
			if (bHasRotate)
			{
				int nRotateBits = ReadUBits(5);
				ret.m_RotateSkew0 = ReadFBits1616(nRotateBits);
				ret.m_RotateSkew1 = ReadFBits1616(nRotateBits);
			}

			int nTranslateBits = ReadUBits(5);
			ret.m_iTranslateX = ReadSBits(nTranslateBits);
			ret.m_iTranslateY = ReadSBits(nTranslateBits);

			return ret;
		}

		protected sbyte ReadSByte()
		{
			return unchecked((sbyte)ReadByte());
		}

		protected string ReadSizedString()
		{
			// Length includes the null terminator.
			int nLength = (int)ReadByte();

			if (nLength < 1)
			{
				throw new Exception();
			}

			if (nLength == 1)
			{
				OffsetInBytes += nLength;
				return null;
			}
			else
			{
				string sReturn = Encoding.UTF8.GetString(m_aData, OffsetInBytes, nLength - 1);
				OffsetInBytes += nLength;
				return sReturn;
			}
		}

		protected string ReadString()
		{
			m_StringBuilder.Clear();
			byte c = ReadByte();

			while (c != 0)
			{
				m_StringBuilder.Add(c);
				c = ReadByte();
			}

			if (m_StringBuilder.Count == 0)
			{
				return null;
			}
			else
			{
				return Encoding.UTF8.GetString(m_StringBuilder.ToArray());
			}
		}

		protected Int16 ReadInt16()
		{
			return unchecked((Int16)ReadUInt16());
		}

		protected Int32 ReadInt32()
		{
			return unchecked((Int32)ReadUInt32());
		}

		protected bool ReadBit()
		{
			int b = (int)m_aData[OffsetInBytes];

			bool bReturn = (0 != (b & (1 << (7 - (OffsetInBits - (OffsetInBytes * 8))))));
			OffsetInBits++;
			return bReturn;
		}

		protected Int32 ReadUBits(int nBits)
		{
			int v = 0;

			while (nBits > 0)
			{
				v |= (ReadBit() ? 1 : 0) << (nBits - 1);
				--nBits;
			}

			return v;
		}

		protected Int32 ReadSBits(int nBits)
		{
			int iShiftBy = (32 - nBits);
			int iUnsigned = ReadUBits(nBits);
			int iIntermediate = (iUnsigned << iShiftBy); // shift the result so the sign is the highest bit.
			int iResult = (iIntermediate >> iShiftBy); // shift back, arithmetic shift.

			return iResult;
		}

		protected Fixed88 ReadFBits88(int nBits)
		{
			Int32 iValue = ReadSBits(nBits);
			Fixed88 ret = default(Fixed88);
			ret.m_iValue = (Int16)iValue;
			return ret;
		}

		protected Fixed1616 ReadFBits1616(int nBits)
		{
			Int32 iValue = ReadSBits(nBits);
			Fixed1616 ret = default(Fixed1616);
			ret.m_iValue = iValue;
			return ret;
		}

		protected byte ReadByte()
		{
			Align();

			byte ret = m_aData[OffsetInBytes];
			OffsetInBits += 8;
			return ret;
		}

		protected UInt16 ReadUInt16()
		{
			Align();

			int nOffsetInBytes = (OffsetInBytes);
			UInt16 ret = (UInt16)((m_aData[nOffsetInBytes + 1] << 8) | (m_aData[nOffsetInBytes]));
			OffsetInBits += 16;

			return ret;
		}

		protected UInt32 ReadEncodedUInt32()
		{
			Align();

			UInt32 uReturn = m_aData[OffsetInBytes++];
			if (0 == (uReturn & 0x00000080))
			{
				return uReturn;
			}

			uReturn = (uReturn & 0x0000007F) | ((UInt32)m_aData[OffsetInBytes++] << 7);
			if (0 == (uReturn & 0x00004000))
			{
				return uReturn;
			}

			uReturn = (uReturn & 0x00003FFF) | ((UInt32)m_aData[OffsetInBytes++] << 14);
			if (0 == (uReturn & 0x00200000))
			{
				return uReturn;
			}

			uReturn = (uReturn & 0x001FFFFF) | ((UInt32)m_aData[OffsetInBytes++] << 21);
			if (0 == (uReturn & 0x10000000))
			{
				return uReturn;
			}

			uReturn = (uReturn & 0x0FFFFFFF) | ((UInt32)m_aData[OffsetInBytes++] << 28);
			return uReturn;
		}

		protected UInt32 ReadUInt32()
		{
			Align();

			int nOffsetInBytes = (OffsetInBytes);
			UInt32 ret = (UInt32)(
				(m_aData[nOffsetInBytes + 3] << 24) |
				(m_aData[nOffsetInBytes + 2] << 16) |
				(m_aData[nOffsetInBytes + 1] << 8) |
				(m_aData[nOffsetInBytes + 0]));
			OffsetInBits += 32;

			return ret;
		}

		protected UInt64 ReadUInt64()
		{
			Align();

			int nOffsetInBytes = (OffsetInBytes);
			UInt64 ret = (UInt64)(
				(m_aData[nOffsetInBytes + 7] << 56) |
				(m_aData[nOffsetInBytes + 6] << 48) |
				(m_aData[nOffsetInBytes + 5] << 40) |
				(m_aData[nOffsetInBytes + 4] << 32) |
				(m_aData[nOffsetInBytes + 3] << 24) |
				(m_aData[nOffsetInBytes + 2] << 16) |
				(m_aData[nOffsetInBytes + 1] << 8) |
				(m_aData[nOffsetInBytes + 0]));
			OffsetInBits += 64;

			return ret;
		}

		protected Rectangle ReadRectangle()
		{
			Align();

			int nBits = ReadUBits(5);
			Rectangle ret;
			ret.m_iLeft = ReadSBits(nBits);
			ret.m_iRight = ReadSBits(nBits);
			ret.m_iTop = ReadSBits(nBits);
			ret.m_iBottom = ReadSBits(nBits);
			return ret;
		}

		protected RGBA ReadRGB()
		{
			RGBA ret = default(RGBA);
			ret.m_R = ReadByte();
			ret.m_G = ReadByte();
			ret.m_B = ReadByte();
			ret.m_A = 255;

			return ret;
		}

		protected RGBA ReadRGBA()
		{
			RGBA ret = ReadRGB();
			ret.m_A = ReadByte();
			return ret;
		}

		protected GradientRecord ReadGradientRecord(int iDefineShapeVersion)
		{
			GradientRecord ret = default(GradientRecord);
			ret.m_uRatio = ReadByte();
			if (iDefineShapeVersion >= 3)
			{
				ret.m_Color = ReadRGBA();
			}
			else
			{
				ret.m_Color = ReadRGB();
			}
			return ret;
		}

		protected Gradient ReadGradient(int iDefineShapeVersion)
		{
			Gradient ret = default(Gradient);
			ret.m_SpreadMode = (GradientSpreadMode)ReadUBits(2);
			ret.m_InterpolationMode = (GradientInterpolationMode)ReadUBits(2);
			ret.m_aGradientRecords = new GradientRecord[ReadUBits(4)];
			for (int i = 0; i < ret.m_aGradientRecords.Length; ++i)
			{
				ret.m_aGradientRecords[i] = ReadGradientRecord(iDefineShapeVersion);
			}
			ret.m_bFocalGradient = false;
			return ret;
		}

		protected Gradient ReadFocalGradient(int iDefineShapeVersion)
		{
			Gradient ret = ReadGradient(iDefineShapeVersion);
			ret.m_FocalPoint = ReadFixed88();
			ret.m_bFocalGradient = true;
			return ret;
		}

		protected FillStyle ReadFillStyle(int iDefineShapeVersion)
		{
			FillStyle ret = default(FillStyle);
			ret.m_eFillStyleType = (FillStyleType)ReadByte();
			if (FillStyleType.SoldFill == ret.m_eFillStyleType)
			{
				if (iDefineShapeVersion >= 3)
				{
					ret.m_Color = ReadRGBA();
				}
				else
				{
					ret.m_Color = ReadRGB();
				}
			}

			if (FillStyleType.LinearGradientFill == ret.m_eFillStyleType ||
				FillStyleType.FocalRadialGradientFill == ret.m_eFillStyleType ||
				FillStyleType.RadialGradientFill == ret.m_eFillStyleType)
			{
				ret.m_mGradientTransform = ReadMatrix();

				Align();

				if (FillStyleType.FocalRadialGradientFill == ret.m_eFillStyleType)
				{
					ret.m_Gradient = ReadFocalGradient(iDefineShapeVersion);
				}
				else
				{
					ret.m_Gradient = ReadGradient(iDefineShapeVersion);
				}
			}

			if (FillStyleType.ClippedBitmapFill == ret.m_eFillStyleType ||
				FillStyleType.NonSmoothedClippedBitmapFill == ret.m_eFillStyleType ||
				FillStyleType.NonSmoothedRepeatingBitmapFill == ret.m_eFillStyleType ||
				FillStyleType.RepeatingBitmapFill == ret.m_eFillStyleType)
			{
				ret.m_uBitmapId = ReadUInt16();
				ret.m_mBitmapTransform = ReadMatrix();
			}

			return ret;
		}

		protected FillStyle[] ReadFillStyles(int iDefineShapeVersion)
		{
			byte u = ReadByte();
			int nCount;
			if (0xFF == u)
			{
				nCount = (int)ReadUInt16();
			}
			else
			{
				nCount = (int)u;
			}

			if (0 == nCount)
			{
				return null;
			}

			FillStyle[] ret = new FillStyle[nCount];
			for (int i = 0; i < nCount; ++i)
			{
				ret[i] = ReadFillStyle(iDefineShapeVersion);
			}

			return ret;
		}

		protected LineStyle ReadLineStyle(int iDefineShapeVersion)
		{
			LineStyle ret = default(LineStyle);
			ret.m_uWidth = ReadUInt16();

			if (iDefineShapeVersion >= 4)
			{
				// TODO: Add these and use these in LineStyle.
				int iStartCapStyle = ReadUBits(2);
				int iJoinStyle = ReadUBits(2);
				bool bHasFillFlag = ReadBit();
				bool bNoHScaleFlag = ReadBit();
				bool bNoVScaleFlag = ReadBit();

				if (0 != ReadUBits(5)) // Reserved, must be 0
				{
					throw new Exception();
				}

				bool bNoClose = ReadBit();
				int iEndCapStyle = ReadUBits(2);

				UInt16 uMiterLimitFactor = 0;
				if (2 == iJoinStyle)
				{
					uMiterLimitFactor = ReadUInt16();
				}

				if (!bHasFillFlag)
				{
					ret.m_Color = ReadRGBA();
				}
				else
				{
					FillStyle fillStyle = ReadFillStyle(iDefineShapeVersion);
				}
			}
			else
			{
				if (iDefineShapeVersion >= 3)
				{
					ret.m_Color = ReadRGBA();
				}
				else
				{
					ret.m_Color = ReadRGB();
				}
			}

			return ret;
		}

		protected LineStyle[] ReadLineStyles(int iDefineShapeVersion)
		{
			byte u = ReadByte();
			int nCount;
			if (0xFF == u)
			{
				nCount = (int)ReadUInt16();
			}
			else
			{
				nCount = (int)u;
			}

			if (0 == nCount)
			{
				return null;
			}

			LineStyle[] ret = new LineStyle[nCount];
			for (int i = 0; i < nCount; ++i)
			{
				ret[i] = ReadLineStyle(iDefineShapeVersion);
			}

			return ret;
		}

		protected Shape ReadShape(int iDefineShapeVersion)
		{
			List<FillStyle> totalFillStyles = new List<FillStyle>();
			List<LineStyle> totalLineStyles = new List<LineStyle>();
			List<ShapePath> paths = new List<ShapePath>();

			FillStyle[] aFillStyles = ReadFillStyles(iDefineShapeVersion);
			LineStyle[] aLineStyles = ReadLineStyles(iDefineShapeVersion);

			int nFillBits = ReadUBits(4);
			int nLineBits = ReadUBits(4);
			int iX = 0;
			int iY = 0;
			ShapePath currentPath = default(ShapePath);
			while (true)
			{
				bool bEdgeRecord = ReadBit();
				if (!bEdgeRecord)
				{
					ShapeRecordFlags eFlags = (ShapeRecordFlags)ReadUBits(5);

					// If all flags are 0, this is an end record and we're done processing.
					if (ShapeRecordFlags.EndShape == eFlags)
					{
						if (null != currentPath.m_aEdges)
						{
							paths.Add(currentPath);
							currentPath = default(ShapePath);
						}
						break;
					}
					// Otherwise, this is a StyleChangeRecord.
					else
					{
						// MoveTo defined
						if (ShapeRecordFlags.StateMoveTo == (ShapeRecordFlags.StateMoveTo & eFlags))
						{
							if (null != currentPath.m_aEdges)
							{
								paths.Add(currentPath);
								currentPath = default(ShapePath);
							}

							int nMoveBits = (int)ReadUBits(5);
							iX = ReadSBits(nMoveBits);
							iY = ReadSBits(nMoveBits);

							currentPath.m_fStartX = (float)iX;
							currentPath.m_fStartY = (float)iY;
						}

						if (ShapeRecordFlags.StateFillStyle0 == (ShapeRecordFlags.StateFillStyle0 & eFlags))
						{
							if (null != currentPath.m_aEdges)
							{
								paths.Add(currentPath);
								currentPath = default(ShapePath);
							}

							currentPath.m_iFillStyle0 = ReadUBits(nFillBits) + totalFillStyles.Count;
						}

						if (ShapeRecordFlags.StateFillStyle1 == (ShapeRecordFlags.StateFillStyle1 & eFlags))
						{
							if (null != currentPath.m_aEdges)
							{
								paths.Add(currentPath);
								currentPath = default(ShapePath);
							}

							currentPath.m_iFillStyle1 = ReadUBits(nFillBits) + totalFillStyles.Count;
						}

						if (ShapeRecordFlags.StateLineStyle == (ShapeRecordFlags.StateLineStyle & eFlags))
						{
							if (null != currentPath.m_aEdges)
							{
								paths.Add(currentPath);
								currentPath = default(ShapePath);
							}

							currentPath.m_iLineStyle = ReadUBits(nLineBits) + totalLineStyles.Count;
						}

						if (ShapeRecordFlags.StateNewStyles == (ShapeRecordFlags.StateNewStyles & eFlags))
						{
							if (null != currentPath.m_aEdges)
							{
								paths.Add(currentPath);
								currentPath = default(ShapePath);
							}

							if (null != aFillStyles)
							{
								totalFillStyles.AddRange(aFillStyles);
							}
							aFillStyles = ReadFillStyles(iDefineShapeVersion);

							if (null != aLineStyles)
							{
								totalLineStyles.AddRange(aLineStyles);
							}
							aLineStyles = ReadLineStyles(iDefineShapeVersion);

							nFillBits = ReadUBits(4);
							nLineBits = ReadUBits(4);
						}
					}
				}
				else
				{
					bool bStraightEdge = ReadBit();
					int nNumBits = ReadUBits(4);

					ShapeEdge edge = default(ShapeEdge);
					int iX1 = 0;
					int iY1 = 0;

					// Straight edge
					if (bStraightEdge)
					{
						bool bGeneralLine = ReadBit();
						bool bVerticalLine = !bGeneralLine && ReadBit();

						iX1 = (iX + ((bGeneralLine || !bVerticalLine) ? ReadSBits(nNumBits + 2) : 0));
						iY1 = (iY + ((bGeneralLine || bVerticalLine) ? ReadSBits(nNumBits + 2) : 0));

						edge.m_fAnchorX = (float)iX1;
						edge.m_fAnchorY = (float)iY1;
						edge.m_fControlX = (float)iX1;
						edge.m_fControlY = (float)iY1;
					}
					// Curved edge
					else
					{
						// TODO: Incorrect, we're drawing the anchor and control points with this,
						// not actually evaluating the curve.
						int iControlX = iX + ReadSBits(nNumBits + 2);
						int iControlY = iY + ReadSBits(nNumBits + 2);
						int iAnchorX = iControlX + ReadSBits(nNumBits + 2);
						int iAnchorY = iControlY + ReadSBits(nNumBits + 2);

						edge.m_fAnchorX = (float)iAnchorX;
						edge.m_fAnchorY = (float)iAnchorY;
						edge.m_fControlX = (float)iControlX;
						edge.m_fControlY = (float)iControlY;

						iX1 = iAnchorX;
						iY1 = iAnchorY;
					}

					if (null == currentPath.m_aEdges)
					{
						currentPath.m_aEdges = new ShapeEdge[1];
					}
					else
					{
						Array.Resize(ref currentPath.m_aEdges, currentPath.m_aEdges.Length + 1);
					}
					currentPath.m_aEdges[currentPath.m_aEdges.Length - 1] = edge;

					iX = iX1;
					iY = iY1;
				}
			}

			if (null != aFillStyles)
			{
				totalFillStyles.AddRange(aFillStyles);
			}

			if (null != aLineStyles)
			{
				totalLineStyles.AddRange(aLineStyles);
			}

			// Back to byte boundaries
			Align();

			Shape ret = new Shape();
			ret.m_aFillStyles = totalFillStyles.ToArray();
			ret.m_aLineStyles = totalLineStyles.ToArray();
			ret.m_aPaths = paths.ToArray();

			return ret;
		}
		#endregion

		public SWFParser()
		{
		}
	} // class SWFParser
} // namespace FalconCooker
