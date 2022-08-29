// Comand sent from the server to a client when a piece of content
// changes in the current game's Source/ or Data/Config/ folders.
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
	/// Types of ContentChange events tracked by ContentChangeManager.
	/// </summary>
	/// <remarks>
	/// This enum must be kept in-sync with the equivalent enum in
	/// FileChangeNotifier.h in the SeoulEngine codebase
	/// </remarks>
	public enum ContentChangeEventType
	{
		/// <summary>
		/// Invalid event, you will never receive this, it is only used internally
		/// </summary>
		Unknown = 0,

		/// <summary>
		/// File was added, both sOldPath and sNewPath will refere to the same path
		/// </summary>
		Added,

		/// <summary>
		/// File was deleted, both sOldPath and sNewPath will refere to the same path
		/// </summary>
		Removed,

		/// <summary>
		/// File was changed, both sOldPath and sNewPath will refere to the same path
		/// </summary>
		Modified,

		/// <summary>
		/// File was renamed, sOldPath will refer to the previous filename, sNewPath will refer to the new filename
		/// </summary>
		Renamed,
	};

	/// <summary>
	/// Command sent from server to client when a monitored piece of content
	/// changes. Includes the old filename, new filename, and the type of file change.
	/// </summary>
	public sealed class ContentChangeEventCommand : BaseMoriartyCommand
	{
		#region Private members
		UInt64 m_zFileSizeInBytes = 0u;
		UInt64 m_uUnixTimestamp = 0u;
		FilePath m_OldFilePath = default(FilePath);
		FilePath m_NewFilePath = default(FilePath);
		ContentChangeEventType m_eContentChangeEventType = ContentChangeEventType.Unknown;
		#endregion

		/// <returns>
		/// The ContentChangeEvent that corresponds to a System WatcherChangeTypes
		/// enum.
		/// </returns>
		public static ContentChangeEventType ToContentChangeEventType(WatcherChangeTypes eType)
		{
			switch (eType)
			{
				case WatcherChangeTypes.Renamed: return ContentChangeEventType.Renamed;
				case WatcherChangeTypes.Deleted: return ContentChangeEventType.Removed;
				case WatcherChangeTypes.Created: return ContentChangeEventType.Added;
				case WatcherChangeTypes.Changed: return ContentChangeEventType.Modified;
				case WatcherChangeTypes.All: return ContentChangeEventType.Unknown;
				default:
					return ContentChangeEventType.Unknown;
			}
		}

		public ContentChangeEventCommand(
			FilePath oldFilePath,
			FilePath newFilePath,
			UInt64 zFileSizeInBytes,
			UInt64 uUnixTimestamp,
			ContentChangeEventType eContentChangeEventType)
			: base(MoriartyConnection.ERPC.ContentChangeEvent)
		{
			m_OldFilePath = oldFilePath;
			m_NewFilePath = newFilePath;
			m_zFileSizeInBytes = zFileSizeInBytes;
			m_uUnixTimestamp = uUnixTimestamp;
			m_eContentChangeEventType = eContentChangeEventType;
		}

		protected override void WriteCommand(BinaryWriter writer)
		{
			writer.NetWriteFilePath(m_OldFilePath);
			writer.NetWriteFilePath(m_NewFilePath);
			writer.NetWriteUInt64(m_zFileSizeInBytes);
			writer.NetWriteUInt64(m_uUnixTimestamp);
			writer.NetWriteByte((byte)m_eContentChangeEventType);
		}
	}
} // namespace Moriarty.Commands
