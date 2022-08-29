// FalconCookerSWFSerializer.cs
//
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
	public class SWFCooker : SWFParser, IDisposable
	{
		public const UInt32 kSignature = 0xF17AB839;
		public const UInt32 kVersion = 1;

		#region Private members
		BitmapResolver m_BitmapResolver = null;
		string m_sInputFilename = string.Empty;
		BinaryWriter m_Writer = null;
		string m_sImageDirectory = string.Empty;
		string m_sInOnlyImgDir = string.Empty;
		string m_sImagePrefix = string.Empty;
		string m_sInOnlyImagePrefix = string.Empty;
		bool m_bDoNotAllowLossyCompression = false;
		int m_iMinSwfVersion = -1;

		ABCFile m_ABCFile = null;
		Dictionary<UInt16, string> m_tCharacterIDToSymbolClass = new Dictionary<ushort,string>();
		Dictionary<string, string> m_tSymbolClasses = new Dictionary<string, string>();

		void Cook()
		{
			m_Writer.Write(kVersion);
			m_Writer.Write(kSignature);
			CookSWF();
		}

		void CookSWF()
		{
			Stream stream = null;
			BinaryReader reader = null;

			try
			{
				stream = File.Open(m_sInputFilename, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
				reader = new BinaryReader(stream);

				// Header - starts with 'CSW' or 'FWS', where
				// the C indicates that the rest of the file (after the file
				// size entry) is compressed.
				byte cOrF = reader.ReadByte();
				byte w = reader.ReadByte();
				byte s = reader.ReadByte();

				if ((cOrF != 'C' && cOrF != 'F') ||
					w != 'W' ||
					s != 'S')
				{
					throw new Exception("Invalid header signature.");
				}

				byte version = reader.ReadByte();

				// Check for supported version range.
				if (!(version >= kiMinimumFlashSwfVersion && version <= kiMaximumFlashSwfVersion))
				{
					throw new Exception(GetFlashPlayerVersionFromSwfVersion(version) + " is " +
						"not supported, minimum supported " +
						GetFlashPlayerVersionFromSwfVersion(kiMinimumFlashSwfVersion) + " to " +
						"maximum supported " + GetFlashPlayerVersionFromSwfVersion(kiMaximumFlashSwfVersion));
				}

				// Check for support version against manually specified minimum, if defined.
				if (m_iMinSwfVersion >= 0 && version < m_iMinSwfVersion)
				{
					throw new Exception(GetFlashPlayerVersionFromSwfVersion(version) + " is " +
						"bellow the application set minimum version of " +
						GetFlashPlayerVersionFromSwfVersion(m_iMinSwfVersion));
				}

				int zDataSizeInBytes = (int)reader.ReadUInt32();
				zDataSizeInBytes -= (int)stream.Position;

				// Compressed file, switch the stream over to an inflate stream.
				if (cOrF == 'C')
				{
					// Skip the ZLib header - we can treat the rest as a regular inflate stream.
					byte zlibCompressionMethodAndInfo = reader.ReadByte();
					byte zlibFlagsByte = reader.ReadByte();

					stream = new System.IO.Compression.DeflateStream(
						stream,
						System.IO.Compression.CompressionMode.Decompress);
					reader = new BinaryReader(stream);
				}

				// Read the rest of the data
				m_aData = reader.ReadBytes(zDataSizeInBytes);

				if (m_aData.Length != zDataSizeInBytes)
				{
					throw new Exception("Invalid or corrupt SWF file.");
				}

				// Scan for the SymbolClass entry, we need it before hitting the DoABC entry.
				CookSymbolClassOnly();

				// Populate tags
				CookSWFBody();
			}
			finally
			{
				if (reader != null)
				{
					reader.Dispose();
				}

				if (stream != null)
				{
					stream.Close();
					stream.Dispose();
				}
			}
		}

		void CookSymbolClassOnly()
		{
			// Reset
			m_iOffsetInBits = 0;

			// Read the frame size and rate records.
			ReadRectangle();
			ReadFixed88();

			// Read frame count of root sprite.
			ReadUInt16();

			// Tags
			CookSymbolClassTagOnly();
		}

		void CookSymbolClassTagOnly()
		{
			TagId eTagId = TagId.End;
			do
			{
				eTagId = CookSymbolClassTag();
			} while (TagId.End != eTagId && TagId.SymbolClass != eTagId);
		}

		TagId CookSymbolClassTag()
		{
			// Tag header is an id and a size in bytes
			int iTagOffsetInBytes = OffsetInBytes;
			int iTagData = (int)ReadUInt16();
			TagId eTagId = (TagId)(iTagData >> 6);
			int zTagLengthInBytes = (int)(iTagData & 0x3F);

			// If the size is 0x3F, then there's an additional 32-bit
			// entry describing a "long" tag.
			if (zTagLengthInBytes == 0x3F)
			{
				zTagLengthInBytes = (int)ReadUInt32();
			}

			// Cache the offset before processing the tag.
			int iNewOffsetInBytes = (OffsetInBytes + zTagLengthInBytes);

			if (eTagId == TagId.SymbolClass)
			{
				int nSymbols = (int)ReadUInt16();
				for (int i = 0; i < nSymbols; ++i)
				{
					UInt16 uCharacterID = ReadUInt16();
					string sQualifiedName = ReadString();
					string sUnqualifiedName = sQualifiedName;

					// TODO: Verify that this works in general.
					int iLastDot = sUnqualifiedName.LastIndexOf('.');
					if (iLastDot >= 0)
					{
						sUnqualifiedName = sUnqualifiedName.Substring(iLastDot + 1);

						// Handle the (obnoxious) case of a symbol name that includes a filename
						// extension - we don't want to split on this.
						if (IsKnownExtension(sUnqualifiedName))
						{
							sUnqualifiedName = sQualifiedName;
						}
					}

					m_tCharacterIDToSymbolClass.Add(uCharacterID, sQualifiedName);
					m_tSymbolClasses.Add(sUnqualifiedName, sQualifiedName);
				}
			}
			else
			{
				// Skip the tag body.
				OffsetInBytes = iNewOffsetInBytes;
			}

			// Validate
			if (OffsetInBytes != iNewOffsetInBytes)
			{
				throw new Exception("Invalid or corrupt SWF file.");
			}

			// Skip the tag body.
			OffsetInBytes = iNewOffsetInBytes;

			return eTagId;
		}

		void CookSWFBody()
		{
			// Reset
			m_iOffsetInBits = 0;

			// Capture the start offset of the SWF rectangle, frame rate, and frame count records.
			Align();
			int iHeaderDataStartOffsetInBytes = OffsetInBytes;

			// Consume the frame size and rate records.
			ReadRectangle();
			ReadFixed88();

			// Consume the frame count of the root sprite.
			ReadUInt16();

			// Now write out that block of data.
			m_Writer.Write(m_aData, iHeaderDataStartOffsetInBytes, (OffsetInBytes - iHeaderDataStartOffsetInBytes));

			// Tags
			CookTags();

			if (OffsetInBytes != m_aData.Length)
			{
				throw new Exception("Invalid or corrupt SWF file.");
			}
		}

		TagId CookTag()
		{
			// Tag header is an id and a size in bytes
			int iTagOffsetInBytes = OffsetInBytes;
			int iTagData = (int)ReadUInt16();
			TagId eTagId = (TagId)(iTagData >> 6);
			int zTagLengthInBytes = (int)(iTagData & 0x3F);

			// If the size is 0x3F, then there's an additional 32-bit
			// entry describing a "long" tag.
			if (zTagLengthInBytes == 0x3F)
			{
				zTagLengthInBytes = (int)ReadUInt32();
			}

			// Cache the offset before processing the tag.
			int iNewOffsetInBytes = (OffsetInBytes + zTagLengthInBytes);

			// Process data.
			switch (eTagId)
			{
				// These are deprecated tags, ignored in favor of newer tags.
				case TagId.DoAction: // deprecated in favor of DoABC
				case TagId.ExportAssets: // deprecated in favor of SymbolClass
				case TagId.ImportAssets: // deprecated in favor of the m_sHasClasName field of PlaceObject*
				case TagId.DoInitAction: // deprecated in favor of DoABC
					OffsetInBytes = iNewOffsetInBytes;
					break;

				// Audio tags are ignored.
				case TagId.DefineSound:
				case TagId.StartSound:
				case TagId.DefineButtonSound:
				case TagId.SoundStreamHead:
				case TagId.SoundStreamHead2:
				case TagId.SoundStreamBlock:
				case TagId.StartSound2:
					OffsetInBytes = iNewOffsetInBytes;
					break;

				// Video tags are ignored.
				case TagId.DefineVideoStream:
				case TagId.VideoFrame:
					OffsetInBytes = iNewOffsetInBytes;
					break;

				// Ignored - used to password protect remote debugging.
				case TagId.DebugID:
				case TagId.EnableDebugger:
				case TagId.EnableDebugger2:
					OffsetInBytes = iNewOffsetInBytes;
					break;

				// Ignored - tells authoring tools to not allow the input SWF to be edited.
				case TagId.Protect:
					OffsetInBytes = iNewOffsetInBytes;
					break;

				// Miscellaneous stripped/ignored
				case TagId.CSMTextSettings:
				case TagId.DefineBinaryData:
				case TagId.DefineFontName:
				case TagId.FileAttributes:
				case TagId.Metadata:
				case TagId.ProductInfo:
					OffsetInBytes = iNewOffsetInBytes;
					break;

				// TODO: Ignored until we decide to support Flash Player 10+
				case TagId.DefineBitsJPEG4:
				case TagId.DefineFont4:
					OffsetInBytes = iNewOffsetInBytes;
					break;

				// TODO: Tags to either process or move into the tags
				// we specifically ignore above.
				case TagId.DefineButton:
				case TagId.DefineMorphShape:
				case TagId.ScriptLimits:
				case TagId.SetTabIndex:
				case TagId.DefineFontAlignZones:
				case TagId.DefineMorphShape2:
				case TagId.JPEGTables:
					OffsetInBytes = iNewOffsetInBytes;
					break;

				// Unsupported tags
				case TagId.DefineBitsJPEG2:
				case TagId.DefineButton2:
				case TagId.DefineButtonCxform:
				case TagId.DefineFont:
				case TagId.DefineFontInfo:
				case TagId.DefineFontInfo2:
				case TagId.DefineText:
				case TagId.DefineText2:
				case TagId.RemoveObject:
					throw new Exception("Unimplemented SWF tag id " + Enum.GetName(typeof(TagId), eTagId));

				// Tags that we write out unchanged.
				case TagId.DefineEditText:
				case TagId.DefineFont2:
				case TagId.DefineScalingGrid:
				case TagId.DefineSceneAndFrameLabelData:
				case TagId.DefineShape:
				case TagId.DefineShape2:
				case TagId.DefineShape3:
				case TagId.DefineShape4:
				case TagId.DefineSprite:
				case TagId.End:
				case TagId.FrameLabel:
				case TagId.ImportAssets2:
				case TagId.PlaceObject:
				case TagId.PlaceObject2:
				case TagId.PlaceObject3:
				case TagId.RemoveObject2:
				case TagId.SetBackgroundColor:
				case TagId.ShowFrame:
				case TagId.SymbolClass:
					m_Writer.Write(
						m_aData,
						iTagOffsetInBytes,
						(iNewOffsetInBytes - iTagOffsetInBytes));
					OffsetInBytes = iNewOffsetInBytes;
					break;

				case TagId.DefineBits:
					{
						UInt16 uCharacterID = ReadUInt16();

						int nWidth = 0;
						int nHeight = 0;
						byte[] aData = null;
						bool bJpeg = false;
						using (System.Drawing.Bitmap imageData = new System.Drawing.Bitmap(new MemoryStream(m_aData, OffsetInBytes, iNewOffsetInBytes - OffsetInBytes)))
						{
							nWidth = imageData.Width;
							nHeight = imageData.Height;
							aData = ToBgraBytes(imageData);
							bJpeg = (imageData.RawFormat.Equals(ImageFormat.Jpeg));
						}
						OffsetInBytes = iNewOffsetInBytes;

						Bitmap bitmap = new Bitmap();
						bitmap.m_nWidth = nWidth;
						bitmap.m_nHeight = nHeight;
						bitmap.m_aBgraData = aData;
						bitmap.m_bLossy = bJpeg;

						WriteBitmap(uCharacterID, bitmap);
					}
					break;
				case TagId.DefineBitsJPEG3:
					{
						UInt16 uCharacterID = ReadUInt16();
						int zImageDataSize = (int)ReadUInt32();

						// Next zImageDataSize bytes is either a JPEG, PNG, or GIF89a image.
						int nWidth = 0;
						int nHeight = 0;
						byte[] aData = null;
						bool bJpeg = false;
						using (System.Drawing.Bitmap imageData = new System.Drawing.Bitmap(new MemoryStream(m_aData, OffsetInBytes, zImageDataSize)))
						{
							nWidth = imageData.Width;
							nHeight = imageData.Height;
							aData = ToBgraBytes(imageData);
							bJpeg = (imageData.RawFormat.Equals(ImageFormat.Jpeg));
						}

						OffsetInBytes += zImageDataSize;

						// If the image data is JPEG, then the next chunk is alpha data, zlib
						// compressed with uncompressed size equal to (width * height) of the image
						// data.
						if (bJpeg)
						{
							int iCompressedSize = (iNewOffsetInBytes - (OffsetInBytes + 2));
							using (System.IO.Compression.DeflateStream deflateStream = new System.IO.Compression.DeflateStream(
								new MemoryStream(m_aData, OffsetInBytes + 2, iCompressedSize),
								System.IO.Compression.CompressionMode.Decompress))
							{
								byte[] aAlphaData = new byte[nWidth * nHeight];
								if (aAlphaData.Length != deflateStream.Read(aAlphaData, 0, aAlphaData.Length))
								{
									throw new Exception();
								}

								for (int y = 0; y < nHeight; ++y)
								{
									for (int x = 0; x < nWidth; ++x)
									{
										int iFrom = (y * nWidth) + x;
										int iTo = (y * nWidth * 4) + (x * 4) + 3;

										aData[iTo] = aAlphaData[iFrom];
									}
								}
							}

							OffsetInBytes += (iCompressedSize + 2);
						}

						Bitmap bitmap = new Bitmap();
						bitmap.m_nWidth = nWidth;
						bitmap.m_nHeight = nHeight;
						bitmap.m_aBgraData = aData;
						bitmap.m_bLossy = bJpeg;

						WriteBitmap(uCharacterID, bitmap);
					}
					break;
				case TagId.DefineBitsLossless:
					{
						UInt16 uCharacterID = ReadUInt16();
						BitmapFormat1 eFormat = (BitmapFormat1)ReadByte();
						UInt16 uBitmapWidth = ReadUInt16();
						UInt16 uBitmapHeight = ReadUInt16();

						Bitmap bitmap = new Bitmap();
						bitmap.m_nWidth = (int)uBitmapWidth;
						bitmap.m_nHeight = (int)uBitmapHeight;
						bitmap.m_aBgraData = new byte[bitmap.m_nWidth * bitmap.m_nHeight * 4];
						bitmap.m_bLossy = false;

						int nUncompressedDataSize = 0;
						int nColorTableSize = 0;
						int pitch = 0;
						int padding = 0;
						if (BitmapFormat1.ColormappedImage8Bit == eFormat)
						{
							pitch = 0 == (bitmap.m_nWidth % 4) ? bitmap.m_nWidth : (((bitmap.m_nWidth / 4) * 4) + 4);
							padding = (pitch - bitmap.m_nWidth);

							nColorTableSize = 1 + ((int)ReadByte());
							nUncompressedDataSize = (nColorTableSize * 3) + (pitch * bitmap.m_nHeight);
						}
						else if (BitmapFormat1.RGBImage15Bit == eFormat)
						{
							pitch = 0 == ((bitmap.m_nWidth * 2) % 4) ? (bitmap.m_nWidth * 2) : ((((bitmap.m_nWidth * 2) / 4) * 8) + 4);
							padding = (pitch - (bitmap.m_nWidth * 2));

							nUncompressedDataSize = (pitch * bitmap.m_nHeight);
						}
						else if (BitmapFormat1.RGBImage24Bit == eFormat)
						{
							pitch = (bitmap.m_nWidth * 4);
							padding = 0;

							nUncompressedDataSize = (pitch * bitmap.m_nHeight);
						}

						byte[] aOldData = m_aData;
						int iOldOffsetInBytes = iNewOffsetInBytes;
						int iCompressedDataSize = (iNewOffsetInBytes - OffsetInBytes);
						using (System.IO.Compression.DeflateStream deflateStream = new System.IO.Compression.DeflateStream(
							new MemoryStream(m_aData, OffsetInBytes + 2, iCompressedDataSize),
							System.IO.Compression.CompressionMode.Decompress))
						{
							m_aData = new byte[nUncompressedDataSize];
							if (nUncompressedDataSize != deflateStream.Read(m_aData, 0, nUncompressedDataSize))
							{
								throw new Exception();
							}
							OffsetInBits = 0;
						}

						if (BitmapFormat1.ColormappedImage8Bit == eFormat)
						{
							RGBA[] aColorTable = new RGBA[nColorTableSize];
							for (int i = 0; i < nColorTableSize; ++i)
							{
								aColorTable[i] = ReadRGB();
							}

							for (int y = 0; y < bitmap.m_nHeight; ++y)
							{
								for (int x = 0; x < bitmap.m_nWidth; ++x)
								{
									int index = (y * bitmap.m_nWidth * 4) + (x * 4);
									RGBA value = aColorTable[(int)ReadByte()];

									bitmap.m_aBgraData[index + 0] = value.m_B;
									bitmap.m_aBgraData[index + 1] = value.m_G;
									bitmap.m_aBgraData[index + 2] = value.m_R;
									bitmap.m_aBgraData[index + 3] = value.m_A;
								}

								OffsetInBytes += padding;
							}
						}
						else if (BitmapFormat1.RGBImage15Bit == eFormat)
						{
							for (int y = 0; y < bitmap.m_nHeight; ++y)
							{
								for (int x = 0; x < bitmap.m_nWidth; ++x)
								{
									int index = (y * bitmap.m_nWidth * 4) + (x * 4);

									// Unused bit
									if (false != ReadBit())
									{
										throw new Exception();
									}

									byte r = (byte)ReadUBits(5);
									byte g = (byte)ReadUBits(5);
									byte b = (byte)ReadUBits(5);

									bitmap.m_aBgraData[index + 0] = b;
									bitmap.m_aBgraData[index + 1] = g;
									bitmap.m_aBgraData[index + 2] = r;
									bitmap.m_aBgraData[index + 3] = 255;
								}

								OffsetInBytes += padding;
							}
						}
						// Note that, despite being "24-bit", the data is actually
						// 32-bit, with an unused alpha component.
						else if (BitmapFormat1.RGBImage24Bit == eFormat)
						{
							// Copy the data directly, then swap it
							Array.Copy(
								m_aData,
								OffsetInBytes,
								bitmap.m_aBgraData,
								0,
								bitmap.m_aBgraData.Length);

							// Swap the data, and set the alpha component to 255 (it's
							// 0 in the source, which makes about as much sense as
							// defining a "24-bit" format using 32-bits per pixel, but whatever.
							for (int i = 0; i < bitmap.m_aBgraData.Length; i += 4)
							{
								bitmap.m_aBgraData[i + 0] = 255;
								Swap(ref bitmap.m_aBgraData[i + 0], ref bitmap.m_aBgraData[i + 3]);
								Swap(ref bitmap.m_aBgraData[i + 1], ref bitmap.m_aBgraData[i + 2]);
							}
						}

						m_aData = aOldData;
						OffsetInBytes = iOldOffsetInBytes;

						WriteBitmap(uCharacterID, bitmap);
					}
					break;
				case TagId.DefineBitsLossless2:
					{
						UInt16 uCharacterID = ReadUInt16();
						BitmapFormat2 eFormat = (BitmapFormat2)ReadByte();
						UInt16 uBitmapWidth = ReadUInt16();
						UInt16 uBitmapHeight = ReadUInt16();

						Bitmap bitmap = new Bitmap();
						bitmap.m_nWidth = (int)uBitmapWidth;
						bitmap.m_nHeight = (int)uBitmapHeight;
						bitmap.m_aBgraData = new byte[bitmap.m_nWidth * bitmap.m_nHeight * 4];
						bitmap.m_bLossy = false;

						int nUncompressedDataSize = 0;
						int nColorTableSize = 0;
						int pitch = 0;
						int padding = 0;
						if (BitmapFormat2.ColormappedImage8Bit == eFormat)
						{
							pitch = 0 == (bitmap.m_nWidth % 4) ? bitmap.m_nWidth : (((bitmap.m_nWidth / 4) * 4) + 4);
							padding = (pitch - bitmap.m_nWidth);

							nColorTableSize = 1 + ((int)ReadByte());
							nUncompressedDataSize = (nColorTableSize * 4) + (pitch * bitmap.m_nHeight);
						}
						else if (BitmapFormat2.ARGBImage32Bit == eFormat)
						{
							pitch = (bitmap.m_nWidth * 4);
							padding = 0;

							nUncompressedDataSize = (pitch * bitmap.m_nHeight);
						}

						byte[] aOldData = m_aData;
						int iOldOffsetInBytes = iNewOffsetInBytes;
						int iCompressedDataSize = (iNewOffsetInBytes - OffsetInBytes);
						using (System.IO.Compression.DeflateStream deflateStream = new System.IO.Compression.DeflateStream(
							new MemoryStream(m_aData, OffsetInBytes + 2, iCompressedDataSize),
							System.IO.Compression.CompressionMode.Decompress))
						{
							m_aData = new byte[nUncompressedDataSize];
							if (nUncompressedDataSize != deflateStream.Read(m_aData, 0, nUncompressedDataSize))
							{
								throw new Exception();
							}
							OffsetInBits = 0;
						}

						if (BitmapFormat2.ColormappedImage8Bit == eFormat)
						{
							RGBA[] aColorTable = new RGBA[nColorTableSize];
							for (int i = 0; i < nColorTableSize; ++i)
							{
								aColorTable[i] = ReadRGBA();
							}

							for (int y = 0; y < bitmap.m_nHeight; ++y)
							{
								for (int x = 0; x < bitmap.m_nWidth; ++x)
								{
									int index = (y * bitmap.m_nWidth * 4) + (x * 4);
									RGBA value = aColorTable[(int)ReadByte()];

									bitmap.m_aBgraData[index + 0] = value.m_B;
									bitmap.m_aBgraData[index + 1] = value.m_G;
									bitmap.m_aBgraData[index + 2] = value.m_R;
									bitmap.m_aBgraData[index + 3] = value.m_A;
								}

								OffsetInBytes += padding;
							}
						}
						else if (BitmapFormat2.ARGBImage32Bit == eFormat)
						{
							// Copy the data directly, then swap it
							Array.Copy(
								m_aData,
								OffsetInBytes,
								bitmap.m_aBgraData,
								0,
								bitmap.m_aBgraData.Length);

							// Swap the data,
							for (int i = 0; i < bitmap.m_aBgraData.Length; i += 4)
							{
								Swap(ref bitmap.m_aBgraData[i + 0], ref bitmap.m_aBgraData[i + 3]);
								Swap(ref bitmap.m_aBgraData[i + 1], ref bitmap.m_aBgraData[i + 2]);
							}
						}

						m_aData = aOldData;
						OffsetInBytes = iOldOffsetInBytes;

						WriteBitmap(uCharacterID, bitmap);
					}
					break;
				case TagId.DefineFont3:
					{
						UInt16 uFontCharacterID = ReadUInt16();
						ReadBit(); // m_bHasLayout
						ReadBit(); // m_bShiftJIS
						ReadBit(); // m_bSmallText
						ReadBit(); // m_bANSI
						ReadBit(); // m_bWideOffsets
						bool bWideCodes = ReadBit(); // m_bWideCodes

						// DefineFont3, m_bWideCodes must be true
						if (!bWideCodes)
						{
							throw new Exception();
						}

						bool bItalic = ReadBit(); // m_bItalic
						bool bBold = ReadBit(); // m_bBold

						ReadByte(); // m_eLanguageCode
						string sFontName = ReadSizedString(); // m_sFontName
						OffsetInBytes = iNewOffsetInBytes;

						WriteFont(uFontCharacterID, sFontName, bBold, bItalic);
					}
					break;
				case TagId.DoABC:
					{
						UInt32 uFlags = ReadUInt32();

						// Lazy initialize flag == (1 << 0)
						bool bLazyInitialize = (0 != (uFlags & (1 << 0)));
						string sName = ReadString();

						int zABCSizeInBytes = (iNewOffsetInBytes - OffsetInBytes);
						if (zABCSizeInBytes > 0)
						{
							byte[] aBytecode = new byte[zABCSizeInBytes];
							Array.Copy(
								m_aData,
								OffsetInBytes,
								aBytecode,
								0,
								zABCSizeInBytes);
							OffsetInBytes += zABCSizeInBytes;

							m_ABCFile = new ABCFile(aBytecode);
						}

						// Write out simplified actions in place of the DoABC tag.
						WriteSimpleActions();
					}
					break;
				default:
					throw new Exception("Unknown SWF tag id " + Enum.GetName(typeof(TagId), eTagId));
			}

			// Validate
			if (OffsetInBytes != iNewOffsetInBytes)
			{
				throw new Exception("Invalid or corrupt SWF file.");
			}

			// Skip the tag body.
			OffsetInBytes = iNewOffsetInBytes;

			return eTagId;
		}

		void CookTags()
		{
			TagId eTagId = TagId.End;
			do
			{
				eTagId = CookTag();
			} while (TagId.End != eTagId);
		}

		static bool IsKnownExtension(string s)
		{
			s = s.ToLower().Trim();
			switch (s)
			{
				case "bmp":
				case ".bmp":
				case "png":
				case ".png":
				case "tga":
				case ".tga":
					return true;

				default:
					return false;
			}
		}

		void WriteTagHeaderData(TagId eTagId, int iTagLength)
		{
			if (iTagLength >= 0x3F)
			{
				UInt16 uTagData = (UInt16)(0x3F | (((int)eTagId) << 6));
				m_Writer.Write(uTagData);
				m_Writer.Write((UInt32)iTagLength);
			}
			else
			{
				UInt16 uTagData = (UInt16)((iTagLength & 0x3F) | (((int)eTagId) << 6));
				m_Writer.Write(uTagData);
			}
		}

		void WriteBitmap(UInt16 uCharacterID, Bitmap bitmap)
		{
			// If set to true, throw an exception when the bitmap has lossy data.
			if (m_bDoNotAllowLossyCompression && bitmap.m_bLossy)
			{
				string sQualifiedName = string.Empty;
				if (m_tCharacterIDToSymbolClass.TryGetValue(uCharacterID, out sQualifiedName))
				{
					throw new Exception("Image \"" + sQualifiedName + "\" with dimensions (" +
						bitmap.m_nWidth.ToString() + "x" + bitmap.m_nHeight.ToString() + ") " +
						"is using lossy (JPEG) compression, please update this image to use " +
						"lossless (PNG) compression in Flash.");
				}
				else
				{
					throw new Exception("Unknown image with dimensions (" +
						bitmap.m_nWidth.ToString() + "x" + bitmap.m_nHeight.ToString() + ") " +
						"is using lossy (JPEG) compression, please update this image to use " +
						"lossless (PNG) compression in Flash.");
				}
			}

			Rectangle visibleRectangle = default(Rectangle);
			string sFilename = m_BitmapResolver.Resolve(bitmap, out visibleRectangle);
			int iTagLength = (
				sizeof(UInt16) +
				GetSizedStringWriteLength(sFilename) +
				sizeof(UInt32) +
				sizeof(UInt32) +
				(sizeof(Int32) * 4));

			WriteTagHeaderData(TagId.DefineExternalBitmap, iTagLength);
			m_Writer.Write(uCharacterID);
			WriteSizedString(sFilename);
			m_Writer.Write((UInt32)bitmap.m_nWidth);
			m_Writer.Write((UInt32)bitmap.m_nHeight);
			m_Writer.Write(visibleRectangle);
		}

		void WriteFont(UInt16 uCharacterID, string sFontName, bool bBold, bool bItalic)
		{
			int iTagLength = (
				sizeof(UInt16) +
				GetSizedStringWriteLength(sFontName) +
				sizeof(byte));

			WriteTagHeaderData(TagId.DefineFontTrueType, iTagLength);
			m_Writer.Write(uCharacterID);
			WriteSizedString(sFontName);
			m_Writer.Write((byte)((bBold ? (1 << 7) : 0) | (bItalic ? (1 << 6) : 0)));
		}

		int GetSizedStringWriteLength(string s)
		{
			return (sizeof(byte) + s.Length + 1);
		}

		void WriteSizedString(string s)
		{
			m_Writer.WriteSizedString(s);
		}

		struct SimpleActions
		{
			public string m_sSymbolClass;
			public Dictionary<int, Dictionary<string, EventType>> m_tEvents;
			public List<int> m_FrameStops;
			public Dictionary<int, bool> m_tVisibleChanges;
		}

		enum SimpleActionValueType
		{
			False,
			Null,
			Number,
			String,
			True,
		}

		void WriteSimpleActions()
		{
			AVM2 avm2 = new AVM2(m_ABCFile);

			List<SimpleActions> allActions = new List<SimpleActions>();
			foreach (InstanceInfo instanceInfo in m_ABCFile.Instances)
			{
				string sInstanceName = m_ABCFile.ConstantPool.GetMultiname(null, instanceInfo.m_iName, null);
				string sQualifiedName = sInstanceName;
				if (m_tSymbolClasses.TryGetValue(sInstanceName, out sQualifiedName))
				{
					try
					{
						AVM2Value instance = avm2.CreateInstance(sInstanceName);
						AVM2Object oInstance = (instance.AsObject(avm2) as AVM2Object);

						SimpleActions actions;
						actions.m_sSymbolClass = sQualifiedName;

						AVM2MovieClipObject oMovieClip = (oInstance as AVM2MovieClipObject);
						if (null == oMovieClip)
						{
							continue;
						}

						actions.m_tEvents = oMovieClip.Events;
						actions.m_FrameStops = oMovieClip.FrameStops;
						actions.m_tVisibleChanges = oMovieClip.VisibleChanges;

						allActions.Add(actions);
					}
					catch (Exception e)
					{
						Console.Error.WriteLine($"Failed processing actions on the timeline for script object '{sInstanceName}' ('{sQualifiedName}'): " + e.Message.ToString());
						throw e;
					}
				}
			}

			// Write the tag into a memory stream.
			{
				MemoryStream tagData = new MemoryStream();
				using (BinaryWriter tagWriter = new BinaryWriter(tagData))
				{
					tagWriter.Write((UInt16)allActions.Count);
					foreach (SimpleActions actions in allActions)
					{
						tagWriter.WriteSizedString(actions.m_sSymbolClass);

						// Frame stops
						tagWriter.Write((UInt16)actions.m_FrameStops.Count);
						foreach (UInt16 uFrameStop in actions.m_FrameStops)
						{
							tagWriter.Write(uFrameStop);
						}

						// Events
						tagWriter.Write((UInt16)actions.m_tEvents.Count);
						foreach (KeyValuePair<int, Dictionary<string, EventType>> e in actions.m_tEvents)
						{
							tagWriter.Write((UInt16)e.Key);
							tagWriter.Write((UInt16)e.Value.Count);
							foreach (var pair in e.Value)
							{
								tagWriter.WriteSizedString(pair.Key);
							}
							foreach (var pair in e.Value)
							{
								tagWriter.Write((byte)pair.Value);
							}
						}

						// Visibles
						tagWriter.Write((UInt16)actions.m_tVisibleChanges.Count);
						foreach (KeyValuePair<int, bool> e in actions.m_tVisibleChanges)
						{
							tagWriter.Write((UInt16)e.Key);
							tagWriter.Write((byte)(e.Value ? 1 : 0));
						}

						// Properties - WIP that was later abandoned.
						// Need to write out 0 to avoid breaking the runtime format.
						tagWriter.Write((UInt16)0);
					}
				}

				// Write tag.
				byte[] aTagData = tagData.ToArray();
				WriteTagHeaderData(TagId.DefineMinimalActions, aTagData.Length);
				m_Writer.Write(aTagData);
			}
		}
		#endregion

		public SWFCooker(string sInputFilename, BinaryWriter writer, string sImageDirectory, string sInOnlyImgDir, string sImagePrefix, string sInOnlyImagePrefix, bool bDoNotAllowLossyCompression, int iMinSwfVersion)
		{
			m_BitmapResolver = new BitmapResolver(sImageDirectory, sInOnlyImgDir, sImagePrefix, sInOnlyImagePrefix);
			m_sInputFilename = sInputFilename;
			m_Writer = writer;
			m_sImageDirectory = sImageDirectory;
			m_sInOnlyImgDir = sInOnlyImgDir;
			m_sImagePrefix = sImagePrefix;
			m_sInOnlyImagePrefix = sInOnlyImagePrefix;
			m_bDoNotAllowLossyCompression = bDoNotAllowLossyCompression;
			m_iMinSwfVersion = iMinSwfVersion;
			Cook();
		}

		public void Dispose()
		{
			m_BitmapResolver.Dispose();
			m_BitmapResolver = null;
		}
	}
} // namespace FalconCooker
