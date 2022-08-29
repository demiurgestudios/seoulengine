// FalconCookerSWFSerializer.cs
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;

namespace FalconCooker
{
	public static class BitmapWriterExtensions
	{
		public static void Write(this BinaryWriter writer, ColorTransform m)
		{
			writer.Write((byte)m.m_AddR);
			writer.Write((byte)m.m_AddG);
			writer.Write((byte)m.m_AddB);
			writer.Write(m.m_fMulR);
			writer.Write(m.m_fMulG);
			writer.Write(m.m_fMulB);
		}

		public static void Write(this BinaryWriter writer, FillStyle e)
		{
			writer.Write(e.m_Color);
			writer.Write((UInt32)e.m_eFillStyleType);
			writer.Write(e.m_Gradient);
			writer.Write(e.m_mBitmapTransform);
			writer.Write(e.m_mGradientTransform);
			writer.Write((UInt16)e.m_uBitmapId);
		}

		public static void Write(this BinaryWriter writer, Gradient gradient)
		{
			// TODO:
		}

		public static void Write(this BinaryWriter writer, LineStyle lineStyle)
		{
			writer.Write(lineStyle.m_Color);
			writer.Write((UInt16)lineStyle.m_uWidth);
		}

		public static void Write(this BinaryWriter writer, Matrix matrix)
		{
			writer.Write((Int32)matrix.m_iTranslateX);
			writer.Write((Int32)matrix.m_iTranslateY);
			writer.Write((Single)matrix.m_RotateSkew0.DoubleValue);
			writer.Write((Single)matrix.m_RotateSkew1.DoubleValue);
			writer.Write((Single)matrix.m_ScaleX.DoubleValue);
			writer.Write((Single)matrix.m_ScaleY.DoubleValue);
		}

		public static void Write(this BinaryWriter writer, Rectangle rectangle)
		{
			writer.Write((Int32)rectangle.m_iLeft);
			writer.Write((Int32)rectangle.m_iRight);
			writer.Write((Int32)rectangle.m_iTop);
			writer.Write((Int32)rectangle.m_iBottom);
		}

		public static void Write(this BinaryWriter writer, RGBA rgba)
		{
			writer.Write((byte)rgba.m_R);
			writer.Write((byte)rgba.m_G);
			writer.Write((byte)rgba.m_B);
			writer.Write((byte)rgba.m_A);
		}

		public static void WriteSizedString(this BinaryWriter writer, string s)
		{
			writer.Write((byte)(s.Length + 1));
			writer.Write(Encoding.UTF8.GetBytes(s));
			writer.Write((byte)'\0');
		}
	}
} // namespace FalconCooker
