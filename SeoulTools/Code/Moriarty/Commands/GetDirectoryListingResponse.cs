// Response command, sent from server to client, triggered when a
// GetDirectoryListing RPC is submitted to the server.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.Collections.Generic;
using System.IO;
using System.Security;

using Moriarty.Extensions;

namespace Moriarty.Commands
{
	// TODO: Comment
	public sealed class GetDirectoryListingResponse : BaseMoriartyResponse
	{
		private MoriartyConnection.EResult m_eResult = MoriartyConnection.EResult.ResultGenericFailure;
		private List<string> m_vsDirectoryEntries = new List<string>();
		private bool m_bIncludeSubdirectories;
		private bool m_bRecursive;
		private string m_sFileExtension;
		private string m_sAbsoluteDirNameWithTrailingSlash;

		public GetDirectoryListingResponse(UInt32 uResponseToken, string sAbsoluteDirName, bool bIncludeSubdirectories, bool bRecursive, string sFileExtension)
			: base(MoriartyConnection.ERPC.GetDirectoryListing, uResponseToken)
		{
			m_bIncludeSubdirectories = bIncludeSubdirectories;
			m_bRecursive = bRecursive;
			m_sFileExtension = sFileExtension;
			m_sAbsoluteDirNameWithTrailingSlash = sAbsoluteDirName;
			if (!string.IsNullOrEmpty(m_sAbsoluteDirNameWithTrailingSlash) &&
				!m_sAbsoluteDirNameWithTrailingSlash.EndsWith(Path.DirectorySeparatorChar.ToString()))
			{
				m_sAbsoluteDirNameWithTrailingSlash = m_sAbsoluteDirNameWithTrailingSlash + Path.DirectorySeparatorChar.ToString();
			}

			if (!Directory.Exists(sAbsoluteDirName))
			{
				m_eResult = MoriartyConnection.EResult.ResultFileNotFound;
			}
			else
			{
				try
				{
					ReadDirectory(sAbsoluteDirName);
					m_eResult = MoriartyConnection.EResult.ResultSuccess;
				}
				catch (PathTooLongException)
				{
				}
				catch (SecurityException)
				{
				}
				catch (UnauthorizedAccessException)
				{
				}
			}
		}

		private void ReadDirectory(string sAbsoluteDirName)
		{
			foreach (string sEntry in Directory.EnumerateFileSystemEntries(sAbsoluteDirName))
			{
				if (sEntry == "." || sEntry == "..")
				{
					continue;
				}

				string sPath = Path.Combine(sAbsoluteDirName, sEntry);

				bool bIsDirectory = Directory.Exists(sPath);
				if (bIsDirectory && m_bIncludeSubdirectories ||
					!bIsDirectory && sPath.EndsWith(m_sFileExtension))
				{
					string sRelative = sPath.Substring(m_sAbsoluteDirNameWithTrailingSlash.Length);
					m_vsDirectoryEntries.Add(sRelative);
				}

				if (bIsDirectory && m_bRecursive)
				{
					ReadDirectory(sPath);
				}
			}
		}

		protected override void WriteResponse(BinaryWriter writer)
		{
			writer.NetWriteByte((byte)m_eResult);
			writer.NetWriteStringArray(m_vsDirectoryEntries.ToArray());
		}
	}
}
