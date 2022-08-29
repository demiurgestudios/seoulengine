// Command, sent from server to client, which is used to initialize
// or refresh the client's file stat cash for a particular game directory.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

using Moriarty.Extensions;
using Moriarty.Utilities;

namespace Moriarty.Commands
{
	/// <summary>
	/// Command, sent from server to client, which is used to initialize
	/// or refresh the client's file stat cash for a particular game directory.
	/// </summary>
	public sealed class FileStatCacheRefreshCommand : BaseMoriartyCommand
	{
		#region Private members
		byte[] m_aData = null;
		#endregion

		public FileStatCacheRefreshCommand(CoreLib.GameDirectory eGameDirectory, string sAbsoluteDirectoryPath)
			: base(MoriartyConnection.ERPC.StatFileCacheRefreshEvent)
		{
			// Generate the data.
			try
			{
				string[] asFiles;
				if (Directory.Exists(sAbsoluteDirectoryPath))
				{
					asFiles = Directory.GetFiles(sAbsoluteDirectoryPath, "*.*", SearchOption.AllDirectories);
				}
				else
				{
					asFiles = new string[0];
				}

				using (MemoryStream memoryStream = new MemoryStream())
				{
					using (BinaryWriter writer = new BinaryWriter(memoryStream))
					{
						foreach (string s in asFiles)
						{
							FilePath filePath = new FilePath();
							filePath.m_eFileType = FileUtility.ExtensionToFileType(Path.GetExtension(s).ToLower());
							if (filePath.m_eFileType == CoreLib.FileType.Unknown)
							{
								continue;
							}
							filePath.m_eGameDirectory = eGameDirectory;
							filePath.m_sRelativePathWithoutExtension = FileUtility.RemoveExtension(
								s.Substring(sAbsoluteDirectoryPath.Length));

							UInt64 zFileSizeInBytes = CoreLib.Utilities.GetFileSize(s);
							UInt64 uUnixTimestamp = CoreLib.Utilities.GetModifiedTime(s);

							writer.NetWriteFilePath(filePath);
							writer.NetWriteUInt64(zFileSizeInBytes);
							writer.NetWriteUInt64(uUnixTimestamp);
						}
					}

					m_aData = CoreLib.Utilities.LZ4Compress(memoryStream.ToArray());
				}
			}
			catch (Exception)
			{
				m_aData = null;
			}
		}

		protected override void WriteCommand(BinaryWriter writer)
		{
			if (null != m_aData && m_aData.Length > 0)
			{
				writer.NetWriteUInt32((UInt32)m_aData.Length);
				writer.Write(m_aData);
			}
			else
			{
				writer.NetWriteUInt32((UInt32)0);
			}
		}
	}
} // namespace Moriarty.Commands
