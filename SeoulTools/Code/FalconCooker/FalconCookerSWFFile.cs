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
	//
	// Represents an Adobe Flash SWF file, version 9.
	//
	// See also: http://wwwimages.adobe.com/www.adobe.com/content/dam/Adobe/en/devnet/swf/pdf/swf-file-format-spec.pdf
	// See also: http://opensource.adobe.com/svn/opensource/flex/sdk/trunk/modules/swfutils/src/java/flash/swf/TagValues.java
	//
	public sealed class SWFFile : SWFParser
	{
		#region Private members
		ParseOptions m_eOptions = ParseOptions.None;
		SwfFileAttributes m_SwfFileAttributes;
		string m_sMetadata;
		List<FontLegalDetail> m_FontLegalDetails;
		SortedDictionary<int, string> m_MainTimelineSceneLabels;
		SortedDictionary<int, string> m_MainTimelineFrameLabels;
		RGBA m_BackgroundColor;
		List<ABCData> m_ABCBytecodeData;
		SortedDictionary<int, string> m_ExportedSymbols;
		SortedDictionary<int, object> m_Dictionary = new SortedDictionary<int, object>();
		SortedDictionary<string, int> m_ImportedCharacters = new SortedDictionary<string, int>();
		Rectangle m_FrameSize;
		Fixed88 m_FrameRate;
		Sprite m_RootSprite;

		void Initialize(string sInputFilename)
		{
			Stream stream = null;
			BinaryReader reader = null;

			try
			{
				stream = File.Open(sInputFilename, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
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

				// Populate tags
				Parse();
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

		TagId ParseTag(Sprite sprite)
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
				case TagId.DoAction: // fall-through, deprecated in favor of DoABC
				case TagId.ExportAssets: // fall-through, deprecated in favor of SymbolClass
				case TagId.ImportAssets: // fall-through, deprecated in favor of the m_sHasClasName field of PlaceObject*
				case TagId.DoInitAction: // deprecated in favor of DoABC
					OffsetInBytes = iNewOffsetInBytes;
					break;

				// Audio tags are ignored.
				case TagId.DefineSound: // fall-through
				case TagId.StartSound: // fall-through
				case TagId.DefineButtonSound: // fall-through
				case TagId.SoundStreamHead: // fall-through
				case TagId.SoundStreamHead2: // fall-through
				case TagId.SoundStreamBlock: // fall-through
				case TagId.StartSound2:
					OffsetInBytes = iNewOffsetInBytes;
					break;

				// Video tags are ignored.
				case TagId.DefineVideoStream: // fall-through
				case TagId.VideoFrame:
					OffsetInBytes = iNewOffsetInBytes;
					break;

				// Ignored - used to password protect remote debugging.
				case TagId.DebugID:
				case TagId.EnableDebugger: // fall-through
				case TagId.EnableDebugger2: // fall-through
					OffsetInBytes = iNewOffsetInBytes;
					break;

				// Ignored - tells authoring tools to not allow the input SWF to be edited.
				case TagId.Protect: // fall-through
					OffsetInBytes = iNewOffsetInBytes;
					break;

				// Ignored until we decide to support Flash Player 10+
				case TagId.DefineBitsJPEG4:
				case TagId.DefineFont4:
					OffsetInBytes = iNewOffsetInBytes;
					break;

				// TODO: Tags to either process or more specifically categorize
				// in the deliberately ignored tags above.
				case TagId.DefineButton: // fall-through
				case TagId.DefineMorphShape: // fall-through
				case TagId.ScriptLimits: // fall-through
				case TagId.SetTabIndex: // fall-through
				case TagId.DefineFontAlignZones: // fall-through
				case TagId.DefineScalingGrid: // fall-through
				case TagId.DefineMorphShape2: // fall-through
				case TagId.JPEGTables:
					OffsetInBytes = iNewOffsetInBytes;
					break;

				// Tags that we process
				case TagId.End:
					OffsetInBytes = iNewOffsetInBytes;
					break;
				case TagId.DoABC:
					{
						UInt32 uFlags = ReadUInt32();
						ABCData data = new ABCData();

						// Lazy initialize flag == (1 << 0)
						data.m_bLazyInitialize = (0 != (uFlags & (1 << 0)));

						data.m_sName = ReadString();

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

							if (ParseOptions.ParseABCBytecode == (ParseOptions.ParseABCBytecode & m_eOptions))
							{
								data.m_Data = new ABCFile(aBytecode);
							}
							else
							{
								data.m_aBytecode = aBytecode;
							}
						}

						if (null == m_ABCBytecodeData)
						{
							m_ABCBytecodeData = new List<ABCData>();
						}

						m_ABCBytecodeData.Add(data);
					}
					break;
				case TagId.ProductInfo:
					{
						// TODO: Store.
						UInt32 uProductID = ReadUInt32();
						UInt32 uEdition = ReadUInt32();
						byte uMajorVersion = ReadByte();
						byte uMinorVersion = ReadByte();
						UInt32 uBuildLow = ReadUInt32();
						UInt32 uBuildHigh = ReadUInt32();
						UInt64 uCompilationDate = ReadUInt64();
					}
					break;
				case TagId.SymbolClass:
					{
						int nSymbols = (int)ReadUInt16();
						if (null == m_ExportedSymbols)
						{
							m_ExportedSymbols = new SortedDictionary<int, string>();
						}

						for (int i = 0; i < nSymbols; ++i)
						{
							m_ExportedSymbols.Add((int)ReadUInt16(), ReadString());
						}
					}
					break;
				case TagId.FileAttributes:
					{
						if (false != ReadBit()) // Reserved bit must be false
						{
							throw new Exception();
						}

						m_SwfFileAttributes.m_bDirectBlit = ReadBit();
						m_SwfFileAttributes.m_bUseGPU = ReadBit();
						m_SwfFileAttributes.m_bHasMetadata = ReadBit();
						m_SwfFileAttributes.m_bActionScript3 = ReadBit();

						if (0 != ReadUBits(2)) // Reserved bits must be 0
						{
							throw new Exception();
						}

						m_SwfFileAttributes.m_bUseNetwork = ReadBit();

						if (0 != ReadUBits(24)) // Reserved bits must be 0
						{
							throw new Exception();
						}
					}
					break;
				case TagId.Metadata:
					m_sMetadata = ReadString();
					break;
				case TagId.SetBackgroundColor:
					m_BackgroundColor = ReadRGB();
					break;
				case TagId.DefineSceneAndFrameLabelData:
					{
						int nSceneCount = (int)ReadEncodedUInt32();
						if (nSceneCount > 0)
						{
							m_MainTimelineSceneLabels = new SortedDictionary<int, string>();
							for (int i = 0; i < nSceneCount; ++i)
							{
								m_MainTimelineSceneLabels.Add((int)ReadEncodedUInt32(), ReadString());
							}
						}

						int nFrameLabelCount = (int)ReadEncodedUInt32();
						if (nFrameLabelCount > 0)
						{
							m_MainTimelineFrameLabels = new SortedDictionary<int, string>();
							for (int i = 0; i < nFrameLabelCount; ++i)
							{
								m_MainTimelineFrameLabels.Add((int)ReadEncodedUInt32(), ReadString());
							}
						}
					}
					break;
				case TagId.ImportAssets2:
					{
						string sUrl = ReadString();
						if (1 != ReadByte()) // Reserved byte that must be 1
						{
							throw new Exception();
						}

						if (0 != ReadByte()) // Reserved byte that must be 0
						{
							throw new Exception();
						}

						int nCount = (int)ReadUInt16();
						for (int i = 0; i < nCount; ++i)
						{
							UInt16 uImportedCharacterID = ReadUInt16();
							string sImportedCharacterName = ReadString();

							m_ImportedCharacters.Add(sImportedCharacterName, uImportedCharacterID);
						}
					}
					break;
				case TagId.ShowFrame:
					if (sprite.m_nCurrentFrame < sprite.m_aFrameOffsets.Length)
					{
						sprite.m_aFrameOffsets[sprite.m_nCurrentFrame] = sprite.m_DisplayListTags.Count;
					}
					sprite.m_DisplayListTags.Add(new ShowFrame());
					sprite.m_nCurrentFrame++;
					break;
				case TagId.DefineShape:
					{
						UInt16 uCharacterID = ReadUInt16();
						Rectangle bounds = ReadRectangle();
						m_Dictionary.Add((int)uCharacterID, ReadShape(1));
					}
					break;
				case TagId.PlaceObject:
					{
						PlaceObject placeObject = new PlaceObject();
						placeObject.m_uCharacterID = ReadUInt16();
						placeObject.m_uDepth = ReadUInt16();
						placeObject.m_mTransform = ReadMatrix();
						placeObject.m_ColorTransform.m_Transform = ReadColorTransform();
						placeObject.m_ColorTransform.m_fMulA = 1.0f;
						placeObject.m_ColorTransform.m_AddA = 0;
						sprite.m_DisplayListTags.Add(placeObject);
					}
					break;
				case TagId.RemoveObject:
					throw new Exception("Unimplemented SWF tag id " + Enum.GetName(typeof(TagId), eTagId));
				case TagId.DefineFont:
					throw new Exception("Unimplemented SWF tag id " + Enum.GetName(typeof(TagId), eTagId));
				case TagId.DefineText:
					throw new Exception("Unimplemented SWF tag id " + Enum.GetName(typeof(TagId), eTagId));
				case TagId.DefineFontInfo:
					throw new Exception("Unimplemented SWF tag id " + Enum.GetName(typeof(TagId), eTagId));
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

						m_Dictionary.Add((int)uCharacterID, bitmap);
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

						m_Dictionary.Add((int)uCharacterID, bitmap);
					}
					break;
				case TagId.DefineBitsJPEG2:
					throw new Exception("Unimplemented SWF tag id " + Enum.GetName(typeof(TagId), eTagId));
				case TagId.DefineShape2:
					{
						UInt16 uCharacterID = ReadUInt16();
						Rectangle bounds = ReadRectangle();
						m_Dictionary.Add((int)uCharacterID, ReadShape(2));
					}
					break;
				case TagId.DefineButtonCxform:
					throw new Exception("Unimplemented SWF tag id " + Enum.GetName(typeof(TagId), eTagId));
				case TagId.PlaceObject2:
					{
						PlaceObject2 placeObject2 = new PlaceObject2();
						placeObject2.m_bHasClipActions = ReadBit();
						placeObject2.m_bHasClipDepth = ReadBit();
						placeObject2.m_bHasName = ReadBit();
						placeObject2.m_bHasRatio = ReadBit();
						placeObject2.m_bHasColorTransform = ReadBit();
						placeObject2.m_bHasMatrix = ReadBit();
						placeObject2.m_bHasCharacter = ReadBit();
						placeObject2.m_bHasMove = ReadBit();

						placeObject2.m_uDepth = ReadUInt16();

						// This actually places a new object.
						if (placeObject2.m_bHasCharacter)
						{
							placeObject2.m_uCharacterID = ReadUInt16();
						}
						// Otherwise, this modifies an object at the existing depth.

						if (placeObject2.m_bHasMatrix)
						{
							placeObject2.m_mTransform = ReadMatrix();
						}

						if (placeObject2.m_bHasColorTransform)
						{
							placeObject2.m_ColorTransform = ReadColorTransformWithAlpha();
						}

						if (placeObject2.m_bHasRatio)
						{
							placeObject2.m_uRatio = ReadUInt16();
						}

						if (placeObject2.m_bHasName)
						{
							placeObject2.m_sName = ReadString();
						}

						if (placeObject2.m_bHasClipDepth)
						{
							placeObject2.m_uClipDepth = ReadUInt16();
						}

						if (placeObject2.m_bHasClipActions)
						{
						}
						// TODO: Decide if we want to do anything with the clip actions - for now,
						// just advance to the end of the tag.
						OffsetInBytes = iNewOffsetInBytes;

						sprite.m_DisplayListTags.Add(placeObject2);
					}
					break;
				case TagId.RemoveObject2:
					{
						RemoveObject2 removeObject2 = new RemoveObject2();
						removeObject2.m_uDepth = ReadUInt16();
						sprite.m_DisplayListTags.Add(removeObject2);
					}
					break;
				case TagId.DefineShape3:
					{
						UInt16 uCharacterID = ReadUInt16();
						Rectangle bounds = ReadRectangle();
						m_Dictionary.Add((int)uCharacterID, ReadShape(3));
					}
					break;
				case TagId.DefineText2:
					throw new Exception("Unimplemented SWF tag id " + Enum.GetName(typeof(TagId), eTagId));
				case TagId.DefineButton2:
					throw new Exception("Unimplemented SWF tag id " + Enum.GetName(typeof(TagId), eTagId));
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
						m_Dictionary.Add((int)uCharacterID, bitmap);
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

						m_Dictionary.Add((int)uCharacterID, bitmap);
					}
					break;
				case TagId.DefineEditText:
					{
						UInt16 uCharacterID = ReadUInt16();
						EditText editText = new EditText();

						editText.m_Bounds = ReadRectangle();

						Align();

						editText.m_bHasText = ReadBit();
						editText.m_bWordWrap = ReadBit();
						editText.m_bMultiline = ReadBit();
						editText.m_bPassword = ReadBit();
						editText.m_bReadOnly = ReadBit();
						editText.m_bHasTextColor = ReadBit();
						editText.m_bHasMaxLength = ReadBit();
						editText.m_bHasFont = ReadBit();
						editText.m_bHasFontClass = ReadBit();
						editText.m_bAutoSize = ReadBit();
						editText.m_bHasLayout = ReadBit();
						editText.m_bNoSelect = ReadBit();
						editText.m_bBorder = ReadBit();
						editText.m_bWasStatic = ReadBit();
						editText.m_bHtml = ReadBit();
						editText.m_bUseOutlines = ReadBit();

						// Error for both to be true
						if (editText.m_bHasFont && editText.m_bHasFontClass)
						{
							throw new Exception();
						}

						if (editText.m_bHasFont)
						{
							editText.m_uFontID = ReadUInt16();
						}

						if (editText.m_bHasFontClass)
						{
							editText.m_sFontClass = ReadString();
						}

						if (editText.m_bHasFont || editText.m_bHasFontClass)
						{
							editText.m_uFontHeight = ReadUInt16();
						}

						if (editText.m_bHasTextColor)
						{
							editText.m_TextColor = ReadRGBA();
						}

						if (editText.m_bHasMaxLength)
						{
							editText.m_uMaxLength = ReadUInt16();
						}

						if (editText.m_bHasLayout)
						{
							editText.m_eAlign = (Align)ReadByte();
							editText.m_uLeftMargin = ReadUInt16();
							editText.m_uRightMargin = ReadUInt16();
							editText.m_uIndent = ReadUInt16();
							editText.m_iLeading = ReadInt16();
						}

						editText.m_sVariableName = ReadString();

						if (editText.m_bHasText)
						{
							editText.m_sInitialText = ReadString();
						}

						m_Dictionary.Add((int)uCharacterID, editText);
					}
					break;
				case TagId.CSMTextSettings:
					{
						UInt16 uTextID = ReadUInt16();
						UseFlashType eUseFlashType = (UseFlashType)ReadUBits(2);
						GridFit eGridFit = (GridFit)ReadUBits(3);
						if (0 != ReadUBits(3)) // Reserved bits that must be 0
						{
							throw new Exception();
						}
						float fThickness = (float)ReadUInt32();// TODO: ReadFloat32();
						float fSharpness = (float)ReadUInt32();// TODO: ReadFloat32();
						if (0 != ReadByte()) // Reserved byte that must be 0
						{
							throw new Exception();
						}
					}
					break;
				case TagId.DefineSprite:
					{
						UInt16 uCharacterID = ReadUInt16();
						UInt16 nFrameCount = ReadUInt16();

						Sprite child = new Sprite(nFrameCount);
						m_Dictionary.Add((int)uCharacterID, child);
						Parse(child);
					}
					break;
				case TagId.FrameLabel:
					{
						if (sprite.m_FrameLabels == null)
						{
							sprite.m_FrameLabels = new SortedDictionary<int, string>();
						}
						sprite.m_FrameLabels[sprite.m_nCurrentFrame] = ReadString();

						// TODO: If there is space left in the tag, then the next byte is the "name anchor" flag.
						// Adobe is so freakin inconsistent - they use new tags nearly everywhere else, but here,
						// you need to look at the tag size to determine if this entry is specially encoded or not.
					}
					break;
				case TagId.DefineFont2:
					{
						UInt16 uFontID = ReadUInt16();
						Font font = new Font();
						font.m_bHasLayout = ReadBit();
						font.m_bShiftJIS = ReadBit();
						font.m_bSmallText = ReadBit();
						font.m_bANSI = ReadBit();
						font.m_bWideOffsets = ReadBit();
						font.m_bWideCodes = ReadBit();
						font.m_bItalic = ReadBit();
						font.m_bBold = ReadBit();

						font.m_eLanguageCode = (LanguageCode)ReadByte();
						font.m_sFontName = ReadSizedString();

						int nGlyphs = (int)ReadUInt16();
						int nStartOfOffsetTable = OffsetInBytes;
						if (nGlyphs > 0)
						{
							font.m_aOffsetTable = new int[nGlyphs];
							if (font.m_bWideOffsets)
							{
								for (int i = 0; i < nGlyphs; ++i)
								{
									font.m_aOffsetTable[i] = (int)ReadUInt32();
								}
							}
							else
							{
								for (int i = 0; i < nGlyphs; ++i)
								{
									font.m_aOffsetTable[i] = (int)ReadUInt16();
								}
							}
						}

						int nCodeTableOffsetInBytes = (font.m_bWideOffsets ? (int)ReadUInt32() : (int)ReadUInt16());

						// TODO: Now comes the glyph shape table, skip it for now.
						OffsetInBytes = (nStartOfOffsetTable + nCodeTableOffsetInBytes);

						if (nGlyphs > 0)
						{
							font.m_aCodeTable = new int[nGlyphs];
							for (int i = 0; i < nGlyphs; ++i)
							{
								font.m_aCodeTable[i] = (int)ReadUInt16();
							}
						}

						if (font.m_bHasLayout)
						{
							font.m_iFontAscent = (int)ReadUInt16();
							font.m_iFontDescent = (int)ReadUInt16();
							font.m_iFontLeading = (int)ReadInt16();

							if (nGlyphs > 0)
							{
								font.m_aFontAdvanceTable = new int[nGlyphs];
								for (int i = 0; i < nGlyphs; ++i)
								{
									font.m_aFontAdvanceTable[i] = (int)ReadInt16();
								}

								font.m_aFontBoundsTable = new Rectangle[nGlyphs];
								for (int i = 0; i < nGlyphs; ++i)
								{
									font.m_aFontBoundsTable[i] = ReadRectangle();
								}
							}

							int nKerningCount = (int)ReadUInt16();
							if (nKerningCount > 0)
							{
								font.m_aFontKerningTable = new KerningRecord[nKerningCount];

								for (int i = 0; i < nKerningCount; ++i)
								{
									font.m_aFontKerningTable[i] = ReadKerningRecord(font.m_bWideCodes);
								}
							}
						}

						m_Dictionary.Add((int)uFontID, font);
					}
					break;
				case TagId.DefineFontInfo2:
					throw new Exception("Unimplemented SWF tag id " + Enum.GetName(typeof(TagId), eTagId));
				case TagId.PlaceObject3:
					{
						PlaceObject3 placeObject3 = new PlaceObject3();
						placeObject3.m_bHasClipActions = ReadBit();
						placeObject3.m_bHasClipDepth = ReadBit();
						placeObject3.m_bHasName = ReadBit();
						placeObject3.m_bHasRatio = ReadBit();
						placeObject3.m_bHasColorTransform = ReadBit();
						placeObject3.m_bHasMatrix = ReadBit();
						placeObject3.m_bHasCharacter = ReadBit();
						placeObject3.m_bHasMove = ReadBit();
						if (false != ReadBit()) // Reserved entry that must always be 0.
						{
							throw new Exception();
						}
						placeObject3.m_bOpaqueBackground = ReadBit();
						placeObject3.m_bHasVisible = ReadBit();
						placeObject3.m_bHasImage = ReadBit();
						placeObject3.m_bHasClassName = ReadBit();
						placeObject3.m_bHasCacheAsBitmap = ReadBit();
						placeObject3.m_bHasBlendMode = ReadBit();
						placeObject3.m_bHasFilterList = ReadBit();

						placeObject3.m_uDepth = ReadUInt16();

						// NOTE: Page 38, "PlaceObject3" section says to read this field "If PlaceFlagHasClassName or (PlaceFlagHasImage and PlaceFlagHasCharacter), String",
						// however, we seem to get garbage if the second condition is true.
						if (placeObject3.m_bHasClassName /*|| (placeObject3.m_bHasImage && placeObject3.m_bHasCharacter)*/)
						{
							placeObject3.m_sClassName = ReadString();
						}

						// This actually places a new object.
						if (placeObject3.m_bHasCharacter)
						{
							placeObject3.m_uCharacterID = ReadUInt16();
						}
						// Otherwise, this modifies an object at the existing depth.

						if (placeObject3.m_bHasMatrix)
						{
							placeObject3.m_mTransform = ReadMatrix();
						}

						if (placeObject3.m_bHasColorTransform)
						{
							placeObject3.m_ColorTransform = ReadColorTransformWithAlpha();
						}

						if (placeObject3.m_bHasRatio)
						{
							placeObject3.m_uRatio = ReadUInt16();
						}

						if (placeObject3.m_bHasName)
						{
							placeObject3.m_sName = ReadString();
						}

						if (placeObject3.m_bHasClipDepth)
						{
							placeObject3.m_uClipDepth = ReadUInt16();
						}

						if (placeObject3.m_bHasFilterList)
						{
							// TODO: Read filters
							throw new Exception("Unimplemented.");
						}

						if (placeObject3.m_bHasBlendMode)
						{
							placeObject3.m_eBlendMode = (BlendMode)ReadByte();
						}

						if (placeObject3.m_bHasCacheAsBitmap)
						{
							placeObject3.m_uBitmapCache = ReadByte();
						}

						if (placeObject3.m_bHasVisible)
						{
							placeObject3.m_bVisible = (ReadByte() == 0 ? false : true);
						}

						if (placeObject3.m_bOpaqueBackground)
						{
							placeObject3.m_BackgroundColor = ReadRGBA();
						}

						if (placeObject3.m_bHasClipActions)
						{
						}
						// TODO: Decide if we want to do anything with the clip actions - for now,
						// just advance to the end of the tag.
						OffsetInBytes = iNewOffsetInBytes;

						sprite.m_DisplayListTags.Add(placeObject3);
					}
					break;
				case TagId.DefineFont3:
					{
						UInt16 uFontID = ReadUInt16();
						Font font = new Font();
						font.m_bHasLayout = ReadBit();
						font.m_bShiftJIS = ReadBit();
						font.m_bSmallText = ReadBit();
						font.m_bANSI = ReadBit();
						font.m_bWideOffsets = ReadBit();
						font.m_bWideCodes = ReadBit();

						// DefineFont3, m_bWideCodes must be true
						if (!font.m_bWideCodes)
						{
							throw new Exception();
						}

						font.m_bItalic = ReadBit();
						font.m_bBold = ReadBit();

						font.m_eLanguageCode = (LanguageCode)ReadByte();
						font.m_sFontName = ReadSizedString();

						int nGlyphs = (int)ReadUInt16();
						int nStartOfOffsetTable = OffsetInBytes;
						if (nGlyphs > 0)
						{
							font.m_aOffsetTable = new int[nGlyphs];
							if (font.m_bWideOffsets)
							{
								for (int i = 0; i < nGlyphs; ++i)
								{
									font.m_aOffsetTable[i] = (int)ReadUInt32();
								}
							}
							else
							{
								for (int i = 0; i < nGlyphs; ++i)
								{
									font.m_aOffsetTable[i] = (int)ReadUInt16();
								}
							}
						}

						int nCodeTableOffsetInBytes = (font.m_bWideOffsets ? (int)ReadUInt32() : (int)ReadUInt16());

						// TODO: Now comes the glyph shape table, skip it for now.
						OffsetInBytes = (nStartOfOffsetTable + nCodeTableOffsetInBytes);

						if (nGlyphs > 0)
						{
							font.m_aCodeTable = new int[nGlyphs];
							for (int i = 0; i < nGlyphs; ++i)
							{
								font.m_aCodeTable[i] = (int)ReadUInt16();
							}
						}

						if (font.m_bHasLayout)
						{
							font.m_iFontAscent = (int)ReadUInt16();
							font.m_iFontDescent = (int)ReadUInt16();
							font.m_iFontLeading = (int)ReadInt16();

							if (nGlyphs > 0)
							{
								font.m_aFontAdvanceTable = new int[nGlyphs];
								for (int i = 0; i < nGlyphs; ++i)
								{
									font.m_aFontAdvanceTable[i] = (int)ReadInt16();
								}

								font.m_aFontBoundsTable = new Rectangle[nGlyphs];
								for (int i = 0; i < nGlyphs; ++i)
								{
									font.m_aFontBoundsTable[i] = ReadRectangle();
								}
							}

							int nKerningCount = (int)ReadUInt16();
							if (nKerningCount > 0)
							{
								font.m_aFontKerningTable = new KerningRecord[nKerningCount];

								for (int i = 0; i < nKerningCount; ++i)
								{
									font.m_aFontKerningTable[i] = ReadKerningRecord(font.m_bWideCodes);
								}
							}
						}

						m_Dictionary.Add((int)uFontID, font);
					}
					break;
				case TagId.DefineShape4:
					{
						UInt16 uCharacterID = ReadUInt16();
						Rectangle bounds = ReadRectangle();
						Rectangle edgeBounds = ReadRectangle();

						Align();

						if (0 != ReadUBits(5)) // Reserved 5 bits, always 0
						{
							throw new Exception();
						}

						bool bUsesFillWindingRule = ReadBit();
						bool bUsesNonScalingStrokes = ReadBit();
						bool bUsesScalingStrokes = ReadBit();

						m_Dictionary.Add((int)uCharacterID, ReadShape(4));
					}
					break;
				case TagId.DefineBinaryData:
					{
						UInt16 uBinaryDataID = ReadUInt16();

						if (0 != ReadUInt32()) // Reserved value, must always be 0
						{
							throw new Exception();
						}

						BinaryData binaryData = new BinaryData();
						int nBinaryDataSizeInBytes = (iNewOffsetInBytes - OffsetInBytes);
						if (nBinaryDataSizeInBytes > 0)
						{
							binaryData.m_aData = new byte[nBinaryDataSizeInBytes];
							Array.Copy(
								m_aData,
								OffsetInBytes,
								binaryData.m_aData,
								0,
								nBinaryDataSizeInBytes);
						}

						m_Dictionary.Add((int)uBinaryDataID, binaryData);
					}
					break;
				case TagId.DefineFontName:
					{
						if (null == m_FontLegalDetails)
						{
							m_FontLegalDetails = new List<FontLegalDetail>();
						}

						FontLegalDetail detail;
						detail.m_iFontID = (int)ReadUInt16();
						detail.m_sFontName = ReadString();
						detail.m_sFontCopyright = ReadString();
						m_FontLegalDetails.Add(detail);
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

		void Parse()
		{
			// Reset
			m_Dictionary.Clear();
			m_iOffsetInBits = 0;

			// Read the record
			m_FrameSize = ReadRectangle();
			m_FrameRate = ReadFixed88();

			UInt16 nFrameCount = ReadUInt16();
			m_RootSprite = new Sprite(nFrameCount);

			// Tags
			Parse(m_RootSprite);

			if (OffsetInBytes != m_aData.Length)
			{
				throw new Exception("Invalid or corrupt SWF file.");
			}
		}

		void Parse(Sprite sprite)
		{
			TagId eTagId = TagId.End;
			do
			{
				eTagId = ParseTag(sprite);
			} while (TagId.End != eTagId);

			if (sprite.m_nCurrentFrame != sprite.m_aFrameOffsets.Length)
			{
				throw new Exception("Invalid or corrupt SWF file.");
			}
		}
		#endregion

		[Flags]
		public enum ParseOptions
		{
			None = 0,

			// If set, parse ABC bytecode into an ABCFile structure, otherwise leave as bytecode.
			ParseABCBytecode = (1 << 0),
		}

		public SWFFile(string sInputFilename)
			: this(sInputFilename, ParseOptions.None)
		{
		}

		public SWFFile(string sInputFilename, ParseOptions eOptions)
		{
			m_eOptions = eOptions;

			Initialize(sInputFilename);
		}

		public SwfFileAttributes SwfFileAttributes { get { return m_SwfFileAttributes; } }
		public string Metadata { get { return m_sMetadata; } }
		public SortedDictionary<int, string> ExportedSymbols { get { return m_ExportedSymbols; } }
		public List<ABCData> ABCBytecodeData { get { return m_ABCBytecodeData; } }
		public List<FontLegalDetail> FontLegalDetails { get { return m_FontLegalDetails; } }
		public SortedDictionary<int, string> MainTimelineSceneLabels { get { return m_MainTimelineSceneLabels; } }
		public SortedDictionary<int, string> MainTimelineFrameLabels { get { return m_MainTimelineFrameLabels; } }
		public RGBA BackgroundColor { get { return m_BackgroundColor; } }
		public SortedDictionary<int, object> Dictionary { get { return m_Dictionary; } }
		public SortedDictionary<string, int> ImportedCharacters { get { return m_ImportedCharacters; } }
		public Rectangle FrameSize { get { return m_FrameSize; } }
		public Fixed88 FrameRate { get { return m_FrameRate; } }
		public Sprite RootSprite { get { return m_RootSprite; } }
	} // class SWFFile
} // namespace FalconCooker
