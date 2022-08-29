// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.RegularExpressions;

namespace FalconCooker
{
	public class BitmapResolver : IDisposable
	{
		#region Private members
		[DllImport("msvcrt.dll", CallingConvention = CallingConvention.Cdecl)]
		static extern int memcmp(IntPtr b1, IntPtr b2, long count);

		[DllImport("msvcrt.dll", EntryPoint = "memcpy", CallingConvention = CallingConvention.Cdecl, SetLastError = false)]
		public static extern IntPtr memcpy(IntPtr dest, IntPtr src, UIntPtr count);

        static T Max<T>(T a, T b)
            where T : IComparable<T>
        {
            return (a.CompareTo(b) > 0) ? a : b;
        }

        static T Min<T>(T a, T b)
            where T : IComparable<T>
        {
            return (a.CompareTo(b) < 0) ? a : b;
        }

		int m_iFilenameNumber = 0;
		string m_sImageDirectory = string.Empty;
		string m_sInOnlyImgDir = string.Empty;
		string m_sImagePrefix = string.Empty;
		string m_sInOnlyImagePrefix = string.Empty;
		struct ExistingBitmap : IDisposable
		{
			#region Private members
			System.Drawing.Bitmap m_Bitmap;
			Rectangle m_VisibleRectangle;

			void ComputeVisibleRectangle()
			{
				// Initial visible rectangle is the entire
				// bitmap.
				m_VisibleRectangle = Rectangle.Create(
					0,
					m_Bitmap.Width,
					0,
					m_Bitmap.Height);

				System.Drawing.Imaging.BitmapData bitmapLock = null;
				try
				{
					int iWidth = m_Bitmap.Width;
					int iHeight = m_Bitmap.Height;

                    if (iWidth != 0 && iHeight != 0)
                    {
                        bitmapLock = m_Bitmap.LockBits(
                            new System.Drawing.Rectangle(0, 0, iWidth, iHeight),
                            System.Drawing.Imaging.ImageLockMode.ReadOnly,
                            System.Drawing.Imaging.PixelFormat.Format32bppArgb);
                        byte[] aPixels = new byte[iWidth * iHeight * 4];
                        Marshal.Copy(bitmapLock.Scan0, aPixels, 0, aPixels.Length);
                        m_Bitmap.UnlockBits(bitmapLock);
                        bitmapLock = null;

                        // Start with the inverse rectangle. Note the
                        // slightly unexpected values, because right/bottom
                        // are always coordinate+1, and left/top are always
                        // just coordinate.
                        Rectangle visibleRectangle = Rectangle.Create(
                            iWidth - 1,
                            1,
                            iHeight - 1,
                            1);

                        // Now walk all pixels, and use Max/Min of the coordinate to find
                        // the bounding rectangle of visible pixels.
                        for (int y = 0; y < iHeight; ++y)
                        {
                            for (int x = 0; x < iWidth; ++x)
                            {
                                int i = (y * iWidth * 4) + (x * 4);
                                byte a = aPixels[i + 3];

                                // TODO: Use a threshold instead of 0?
                                if (0 != a)
                                {
                                    visibleRectangle.m_iLeft = Min(visibleRectangle.m_iLeft, x);
                                    visibleRectangle.m_iRight = Max(visibleRectangle.m_iRight, x + 1);
                                    visibleRectangle.m_iTop = Min(visibleRectangle.m_iTop, y);
                                    visibleRectangle.m_iBottom = Max(visibleRectangle.m_iBottom, y + 1);
                                }
                            }
                        }

                        // Assign and we're done.
                        m_VisibleRectangle = visibleRectangle;
                    }
				}
				finally
				{
					if (null != bitmapLock)
					{
						m_Bitmap.UnlockBits(bitmapLock);
						bitmapLock = null;
					}
				}
			}
			#endregion

			public ExistingBitmap(string sFilename, int iWidth, int iHeight, bool bInputOnly)
			{
				m_sFilename = sFilename;
				m_iWidth = iWidth;
				m_iHeight = iHeight;
				m_Bitmap = null;
				m_VisibleRectangle = Rectangle.Create(0, iWidth, 0, iHeight);
				m_bInputOnly = bInputOnly;
			}

			public void Dispose()
			{
				if (null != m_Bitmap)
				{
					m_Bitmap.Dispose();
					m_Bitmap = null;
				}
			}

			public string m_sFilename;
			public int m_iWidth;
			public int m_iHeight;
			public System.Drawing.Bitmap Bitmap
			{
				get
				{
					if (null == m_Bitmap)
					{
						m_Bitmap = new System.Drawing.Bitmap(m_sFilename);
						ComputeVisibleRectangle();
					}

					return m_Bitmap;
				}

				set
				{
					if (null != m_Bitmap)
					{
						m_Bitmap.Dispose();
					}

					m_Bitmap = value;
					ComputeVisibleRectangle();
				}
			}

			public Rectangle VisibleRectangle
			{
				get
				{
					return m_VisibleRectangle;
				}
			}

			public bool m_bInputOnly;
		}
		List<ExistingBitmap> m_ExistingBitmaps = new List<ExistingBitmap>();

		static Int32 EndianSwap32(Int32 v)
		{
			return (Int32)EndianSwap32((UInt32)v);
		}

		static UInt32 EndianSwap32(UInt32 v)
		{
			return
				(UInt32)(
					((v & 0x000000FF) << 24) |
					((v & 0x0000FF00) << 8) |
					((v & 0x00FF0000) >> 8) |
					((v & 0xFF000000) >> 24));
		}

		static bool GetPNGDimensions(string sFilename, ref int riWidth, ref int riHeight)
		{
			try
			{
				const long kOffsetToWidthAndHeight = 16;

				using (FileStream file = File.Open(sFilename, FileMode.Open, FileAccess.Read))
				{
					using (BinaryReader reader = new BinaryReader(file))
					{
						file.Seek(kOffsetToWidthAndHeight, SeekOrigin.Begin);
						int iWidth = reader.ReadInt32();
						int iHeight = reader.ReadInt32();

						if (BitConverter.IsLittleEndian)
						{
							iWidth = EndianSwap32(iWidth);
							iHeight = EndianSwap32(iHeight);
						}

						riWidth = iWidth;
						riHeight = iHeight;
						return true;
					}
				}
			}
			catch
			{
				return false;
			}
		}

		string ResolveBitmapFilename()
		{
			string sFilename = string.Empty;
			do
			{
				sFilename = Path.ChangeExtension(
					Path.Combine(m_sImageDirectory, "falcon_image_" + m_iFilenameNumber.ToString("D4")),
					".png");
				m_iFilenameNumber++;
			} while (File.Exists(sFilename));

			return sFilename;
		}

		string WriteNewBitmap(Bitmap bitmap, out Rectangle visibleRectangle)
		{
			string sNewFilename = ResolveBitmapFilename();
			System.Drawing.Bitmap newBitmap = new System.Drawing.Bitmap(
				bitmap.m_nWidth,
				bitmap.m_nHeight);

			System.Drawing.Imaging.BitmapData newBitmapLock = null;
			GCHandle bitmapPin = default(GCHandle);
			try
			{
				newBitmapLock = newBitmap.LockBits(
					new System.Drawing.Rectangle(0, 0, bitmap.m_nWidth, bitmap.m_nHeight),
					System.Drawing.Imaging.ImageLockMode.WriteOnly,
					System.Drawing.Imaging.PixelFormat.Format32bppArgb);
				bitmapPin = GCHandle.Alloc(bitmap.m_aBgraData, GCHandleType.Pinned);

				memcpy(newBitmapLock.Scan0, bitmapPin.AddrOfPinnedObject(), (UIntPtr)bitmap.m_aBgraData.Length);
			}
			finally
			{
				bitmapPin.Free();
				bitmapPin = default(GCHandle);

				newBitmap.UnlockBits(newBitmapLock);
				newBitmapLock = null;
			}

			newBitmap.Save(sNewFilename);

			{
				ExistingBitmap existingBitmap = new ExistingBitmap(sNewFilename, bitmap.m_nWidth, bitmap.m_nHeight, false);
				existingBitmap.Bitmap = newBitmap;
				m_ExistingBitmaps.Add(existingBitmap);
				visibleRectangle = existingBitmap.VisibleRectangle;
			}

			return sNewFilename;
		}
		#endregion

		public BitmapResolver(string sImageDirectory, string sInOnlyImgDir, string sImagePrefix, string sInOnlyImagePrefix)
		{
			m_sImageDirectory = sImageDirectory;
			m_sInOnlyImgDir = sInOnlyImgDir;
			m_sImagePrefix = sImagePrefix;
			m_sInOnlyImagePrefix = sInOnlyImagePrefix;

			string[] asExistingBitmapFiles = null;
			string[] asAdditionalExistingBitmapFiles = null;

			if (Directory.Exists(m_sImageDirectory))
			{
				asExistingBitmapFiles = Directory.GetFiles(
					 m_sImageDirectory,
					"*.png");
			}
			else
			{
				asExistingBitmapFiles = new string[0];
			}

			if (!string.IsNullOrEmpty(m_sInOnlyImgDir) && Directory.Exists(m_sInOnlyImgDir))
			{
				asAdditionalExistingBitmapFiles = Directory.GetFiles(
					m_sInOnlyImgDir,
					"*.png");
			}
			else
			{ 
				asAdditionalExistingBitmapFiles = new string[0];
			}

			foreach (string s in asExistingBitmapFiles)
			{
				int iWidth = 0;
				int iHeight = 0;
				if (GetPNGDimensions(s, ref iWidth, ref iHeight))
				{
					m_ExistingBitmaps.Add(new ExistingBitmap(s, iWidth, iHeight, false));
				}
			}

			foreach (string s in asAdditionalExistingBitmapFiles)
			{
				int iWidth = 0;
				int iHeight = 0;
				if (GetPNGDimensions(s, ref iWidth, ref iHeight))
				{
					m_ExistingBitmaps.Add(new ExistingBitmap(s, iWidth, iHeight, true));
				}
			}
		}

		public void Dispose()
		{
			foreach (ExistingBitmap e in m_ExistingBitmaps)
			{
				e.Dispose();
			}
		}

		public string Resolve(Bitmap bitmap, out Rectangle visibleRectangle)
		{
			int nCount = m_ExistingBitmaps.Count;
			for (int i = 0; i < nCount; ++i)
			{
				ExistingBitmap existing = m_ExistingBitmaps[i];

				if (existing.m_iWidth == bitmap.m_nWidth &&
					existing.m_iHeight == bitmap.m_nHeight)
				{
					System.Drawing.Bitmap existingBitmap = existing.Bitmap;
					System.Drawing.Imaging.BitmapData existingLock = null;
					GCHandle bitmapPin = default(GCHandle);

					try
					{
						// Lock the bits of each image.
						existingLock = existingBitmap.LockBits(
							new System.Drawing.Rectangle(0, 0, existingBitmap.Width, existingBitmap.Height),
							System.Drawing.Imaging.ImageLockMode.ReadOnly,
							existingBitmap.PixelFormat);
						bitmapPin = GCHandle.Alloc(bitmap.m_aBgraData, GCHandleType.Pinned);

						// TODO: JPEG data decompression from DefineBits is inconsistent due
						// to our usage of .NET Bitmap, the output of which can vary between
						// CPU architectures, version of GDI, etc. See also:
						// http://stackoverflow.com/questions/11872850/jpeg-decompression-inconsistent-across-windows-architectures
						//
						// For now, using an unfortunate workaround, performing a comparison with
						// tolerance when the Bitmap data is lossy. The long term solution is
						// (if .NET Bitmap cannot be made consistent) to use a different JPEG
						// decompression library, or remove support for JPEG data from SWF.
						if (bitmap.m_bLossy)
						{
							byte[] aExistingData = new byte[bitmap.m_aBgraData.Length];
							Marshal.Copy(existingLock.Scan0, aExistingData, 0, aExistingData.Length);

							bool bMatch = true;
							for (int j = 0; j < aExistingData.Length; ++j)
							{
								int iDiff = ((int)aExistingData[j] - (int)bitmap.m_aBgraData[j]);
								iDiff = (iDiff < 0 ? -iDiff : iDiff);

								if (iDiff > 4)
								{
									bMatch = false;
									break;
								}
							}

							if (bMatch)
							{
								visibleRectangle = existing.VisibleRectangle;
								return (existing.m_bInputOnly ? m_sInOnlyImagePrefix : m_sImagePrefix)
									+ Path.GetFileName(existing.m_sFilename);
							}
						}
						else
						{
							// Perform a memcmp of the bits - if they are exactly the same, resuse
							// the existing file for this texture dependency and mark that
							// we're done.
							if (0 == memcmp(existingLock.Scan0, bitmapPin.AddrOfPinnedObject(), bitmap.m_aBgraData.Length))
							{
								visibleRectangle = existing.VisibleRectangle;
								return (existing.m_bInputOnly ? m_sInOnlyImagePrefix : m_sImagePrefix)
									+ Path.GetFileName(existing.m_sFilename);
							}
						}
					}
					finally
					{
						bitmapPin.Free();
						bitmapPin = default(GCHandle);

						existingBitmap.UnlockBits(existingLock);
						existingLock = null;
					}
				}
			}

			string sReturn = WriteNewBitmap(bitmap, out visibleRectangle);
			return m_sImagePrefix + Path.GetFileName(sReturn);
		}
	}
} // namespace FalconCooker
