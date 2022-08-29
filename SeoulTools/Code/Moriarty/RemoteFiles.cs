// Aggregate used by MoriartyConnection, manages files that have been opened
// by a MoriartyClient, also provides implementations of miscellaneous RPC functionality
// (stat, SetModifiedTime) tied to files.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;

using Moriarty.Commands;
using Moriarty.Extensions;
using Moriarty.Utilities;

namespace Moriarty
{
	/// <summary>
	/// Modes that can be used when creating a file. These
	/// control the type and behavior of read and write access to
	/// the file.
	/// </summary>
	/// <remarks>
	/// Must exactly match the File::Mode enum in SeoulFile.h in the SeoulEngine codebase.
	/// </remarks>
	public enum RemoteFileMode
	{
		/// <summary>
		/// Only readable
		/// </summary>
		Read,

		/// <summary>
		/// Only writeable, an existing file will be zeroed out
		/// </summary>
		WriteTruncate,

		/// <summary>
		/// Only writeable, data will be added to an existing file
		/// </summary>
		WriteAppend,

		/// <summary>
		/// Read/write access, file will be modified in place. File must exist
		/// </summary>
		ReadWrite,
	}

	/// <summary>
	/// Wraps a .NET Stream object to be used for remote file read/write fulfillment.
	/// </summary>
	/// <remarks>
	/// This class enforces the following behavior:
	/// - for read streams, the entire file is read into memory and accessed via a MemoryStream.
	/// - for other streams, a regular FileStream is used.
	/// - in both cases, the open/open-and-read occurs asynchronously. If the Stream is needed,
	/// before the action is done, the caller will wait on the event to signal its completion.
	/// </remarks>
	public sealed class RemoteFileEntry : IDisposable
	{
		#region Private members
		string m_sAbsoluteFilename = string.Empty;
		RemoteFileMode m_eMode = RemoteFileMode.Read;
		Stream m_Stream = null;
		ManualResetEvent m_Event = new ManualResetEvent(false);
		bool m_bAllowCompression = false;
		#endregion

		public RemoteFileEntry(bool bAllowCompression, string sAbsoluteFilename, RemoteFileMode eMode)
		{
			m_sAbsoluteFilename = sAbsoluteFilename;
			m_eMode = eMode;
			m_bAllowCompression = bAllowCompression;

			WaitCallback callaback = null;

			// If a read-only file was requested, read the entire file into memory. This
			// is a herustic - as a rule, clients will want the entire file eventually, so
			// we read it to mask some of the read time in the latency between client/server.
			if (m_eMode == RemoteFileMode.Read)
			{
				callaback = new WaitCallback(delegate(object unused)
				{
					try
					{
						// TODO: The synchronized wrapper is only necessary
						// because we do not sequence file operations - an alternative to this
						// might be to sequence all file operations, possibly in the CookManager
						// worker thread, so that file operations are also sequenced with Cooking.
						m_Stream = Stream.Synchronized(
							new MemoryStream(File.ReadAllBytes(m_sAbsoluteFilename), false));
					}
					catch (Exception)
					{
						m_Stream = null;
					}
					m_Event.Set();
				});
			}
			// Otherwise, use a standard FileStream.
			else
			{
				callaback = new WaitCallback(delegate(object unused)
					{
						try
						{
							// TODO: The synchronized wrapper is only necessary
							// because we do not sequence file operations - an alternative to this
							// might be to sequence all file operations, possibly in the CookManager
							// worker thread, so that file operations are also sequenced with Cooking.
							m_Stream = Stream.Synchronized(new FileStream(
								m_sAbsoluteFilename,
								ToSystemFileMode(m_eMode),
								ToSystemFileAccess(m_eMode)));
						}
						catch (Exception)
						{
							m_Stream = null;
						}
						m_Event.Set();
					});
			}

			// Try to use the ThreadPool, if we can't enqueue the item, just
			// run it synchronously in place.
			if (!ThreadPool.QueueUserWorkItem(callaback))
			{
				callaback(null);
			}
		}

		/// <returns>
		/// True if compression should be (possibly) applied to read and write
		/// transform operations, false otherwise.
		/// </returns>
		public bool AllowCompression
		{
			get
			{
				return m_bAllowCompression;
			}
		}

		/// <summary>
		/// Cleanup any Stream resoures associated with this
		/// RemoteFile object.
		/// </summary>
		public void Dispose()
		{
			m_Event.WaitOne();
			if (null != m_Stream)
			{
				m_Stream.Close();
				m_Stream.Dispose();
				m_Stream = null;
			}
		}

		/// <returns>
		/// The .NET FileAccess value defined by the SeoulEngine file mode eMode.
		/// </returns>
		public static System.IO.FileAccess ToSystemFileAccess(RemoteFileMode eMode)
		{
			switch (eMode)
			{
				case RemoteFileMode.Read: return FileAccess.Read;
				case RemoteFileMode.ReadWrite: return FileAccess.ReadWrite;
				case RemoteFileMode.WriteAppend: return FileAccess.Write;
				case RemoteFileMode.WriteTruncate: return FileAccess.Write;
				default:
					throw new Exception("Unknown enum \"" + Enum.GetName(typeof(FileMode), eMode) + ".\"");
			}
		}

		/// <returns>
		/// The .NET FileMode value defined by the SeoulEngine file mode eMode.
		/// </returns>
		public static System.IO.FileMode ToSystemFileMode(RemoteFileMode eMode)
		{
			switch (eMode)
			{
				case RemoteFileMode.Read: return System.IO.FileMode.Open;
				case RemoteFileMode.ReadWrite: return System.IO.FileMode.Open;
				case RemoteFileMode.WriteAppend: return System.IO.FileMode.Append;
				case RemoteFileMode.WriteTruncate: return System.IO.FileMode.Create;
				default:
					throw new Exception("Unknown enum \"" + Enum.GetName(typeof(FileMode), eMode) + ".\"");
			}
		}

		/// <returns>
		/// The Stream associated with this RemoteFile object, or null if
		/// the file failed to load.
		///
		/// This method will block until the asynchronous operation to load
		/// the Stream completes.
		/// </returns>
		public Stream Stream
		{
			get
			{
				m_Event.WaitOne();
				return m_Stream;
			}
		}
	}

	/// <summary>
	/// Contains RPC handlers for remote functionality tied to files - includes file open, read, close,
	/// file state, and setting the modification time of a file.
	/// </summary>
	public sealed class RemoteFiles : IDisposable
	{
		#region Private members
		ConcurrentDictionary<UInt32, RemoteFileEntry> m_Files = new ConcurrentDictionary<uint, RemoteFileEntry>();
		MoriartyConnection m_Connection = null;
		int m_iNextHandle = 0;
		#endregion

		/// <returns>
		/// True if eFileType should be possibly compressed
		/// when fulfilling read or write requests over the network, false
		/// otherwise.
		/// </returns>
		public static bool AllowCompression(CoreLib.FileType eFileType)
		{
			switch (eFileType)
			{
				case CoreLib.FileType.Font:
				case CoreLib.FileType.SceneAsset:
				case CoreLib.FileType.ScenePrefab:
				case CoreLib.FileType.Texture0:
				case CoreLib.FileType.Texture1:
				case CoreLib.FileType.Texture2:
				case CoreLib.FileType.Texture3:
				case CoreLib.FileType.Texture4:
				case CoreLib.FileType.UIMovie:
					return false;

				default:
					return true;
			}
		}

		public RemoteFiles(MoriartyConnection connection)
		{
			m_Connection = connection;
		}

		/// <summary>
		/// Destroy resources owned by this RemoteFiles object, cleanup any FileStream
		/// still open.
		/// </summary>
		public void Dispose()
		{
			foreach (KeyValuePair<UInt32, RemoteFileEntry> e in m_Files)
			{
				e.Value.Dispose();
			}
			m_Files.Clear();
		}

		/// <summary>
		/// RPC handler when an OpenFile RPC is received - registered with
		/// a connection inside MoriartyConnection.
		/// </summary>
		public void OpenFile(MoriartyConnection.ERPC eRPC, UInt32 uToken, BinaryReader reader)
		{
			FilePath filePath = reader.NetReadFilePath();
			RemoteFileMode eMode = (RemoteFileMode)reader.NetReadByte();

			// Log this event.
			m_Connection.LogMessage(eRPC, uToken, filePath.ToString() + ": " + eMode.ToString());

			WaitCallback callback = new WaitCallback(delegate(object unusedArgument)
			{
				// Get a new handle - must be non-zero.
				int iHandle = 0;
				do
				{
					iHandle = Interlocked.Increment(ref m_iNextHandle);
				} while (0 == iHandle);

				// Construct the absolute filename path.
				string sAbsoluteFilename = m_Connection.ConnectionManager.MoriartyManager.GetAbsoluteFilename(
					m_Connection.Platform,
					filePath);

				// If we couldn't insert the file entry, Dispose it and fail.
				RemoteFileEntry remoteFileEntry = new RemoteFileEntry(AllowCompression(filePath.m_eFileType), sAbsoluteFilename, eMode);
				if (!m_Files.TryAdd(unchecked((UInt32)iHandle), remoteFileEntry))
				{
					remoteFileEntry.Dispose();
					remoteFileEntry = null;
					iHandle = 0;
				}

				// Send the response back to the client that the file has been (un)successfully opened.
				m_Connection.SendCommand(new OpenFileResponse(uToken, iHandle, sAbsoluteFilename));
			});

			// Try to run the work item on another thread - if this fails, run it immediately.
			if (!ThreadPool.QueueUserWorkItem(callback))
			{
				callback(null);
			}
		}

		/// <summary>
		/// RPC handler when a CloseFile RPC is received - registered with
		/// a connection inside MoriartyConnection.
		/// </summary>
		public void CloseFile(MoriartyConnection.ERPC eRPC, UInt32 uToken, BinaryReader reader)
		{
			int iHandle = reader.NetReadInt32();

			// Log this event.
			m_Connection.LogMessage(eRPC, uToken, iHandle.ToString());

			// Unregister and destroy the RemoteFileEntry.
			RemoteFileEntry entry = null;
			bool bSuccess = m_Files.TryRemove(unchecked((UInt32)iHandle), out entry);
			if (bSuccess)
			{
				entry.Dispose();
				entry = null;
			}

			// Send the response back to the client.
			m_Connection.SendCommand(new CloseFileResponse(uToken, bSuccess));
		}

		/// <summary>
		/// RPC handler when a ReadFile RPC is received - registered with
		/// a connection inside MoriartyConnection.
		/// </summary>
		public void ReadFile(MoriartyConnection.ERPC eRPC, UInt32 uToken, BinaryReader reader)
		{
			int iHandle = reader.NetReadInt32();
			UInt64 zSizeInBytes = reader.NetReadUInt64();
			UInt64 uOffsetInBytes = reader.NetReadUInt64();

			// TODO: Add a log here, but need to filter it based on verbosity since
			// it can generate a lot of log spam if files are being streamed.

			// Get the entry - ignore the return value from TryGetValue(), we pass
			// a null RemoteFileEntry and use that to indicate failure.
			RemoteFileEntry entry = null;
			m_Files.TryGetValue(unchecked((UInt32)iHandle), out entry);

			// Construct the callback that will do the read and send the response command.
			WaitCallback callback = new WaitCallback(delegate(object unusedArgument)
			{
				m_Connection.SendCommand(new ReadFileResponse(uToken, zSizeInBytes, uOffsetInBytes, entry));
			});

			// Try to run the work item on another thread - if this fails, run it immediately.
			if (!ThreadPool.QueueUserWorkItem(callback))
			{
				callback(null);
			}
		}

		/// <summary>
		/// RPC handler when a SetFileModifiedTime RPC is received - registered with
		/// a connection inside MoriartyConnection.
		/// </summary>
		public void SetFileModifiedTime(MoriartyConnection.ERPC eRPC, UInt32 uToken, BinaryReader reader)
		{
			FilePath filePath = reader.NetReadFilePath();
			UInt64 uModifiedTime = reader.NetReadUInt64();

			// Log the set file modified time event.
			m_Connection.LogMessage(eRPC, uToken, filePath.ToString() + ":" + uModifiedTime.ToString());

			// Construct the callback that will actually do the work.
			WaitCallback callback = new WaitCallback(delegate(object unusedArgument)
			{
				m_Connection.SendCommand(new SetFileModifiedTimeResponse(
					uToken,
					m_Connection.ConnectionManager.MoriartyManager.GetAbsoluteFilename(
						m_Connection.Platform,
						filePath),
					uModifiedTime));
			});

			// Try to run the work item on another thread - if this fails, run it immediately.
			if (!ThreadPool.QueueUserWorkItem(callback))
			{
				callback(null);
			}
		}

		/// <summary>
		/// RPC handler when a StatFile RPC is received - registered with
		/// a connection inside MoriartyConnection.
		/// </summary>
		public void StatFile(MoriartyConnection.ERPC eRPC, UInt32 uToken, BinaryReader reader)
		{
			FilePath filePath = reader.NetReadFilePath();

			// Log the stat event.
			m_Connection.LogMessage(eRPC, uToken, filePath.ToString());

			// Construct the callback that will actually do the work.
			WaitCallback callback = new WaitCallback(delegate(object unusedArgument)
			{
				m_Connection.SendCommand(new FileStatResponse(
					uToken,
					m_Connection.ConnectionManager.MoriartyManager.GetAbsoluteFilename(
						m_Connection.Platform,
						filePath)));
			});

			// Try to run the work item on another thread - if this fails, run it immediately.
			if (!ThreadPool.QueueUserWorkItem(callback))
			{
				callback(null);
			}
		}

		/// <summary>
		/// RPC handler when a WriteFile RPC is received - registered with
		/// a connection inside MoriartyConnection.
		/// </summary>
		public void WriteFile(MoriartyConnection.ERPC eRPC, UInt32 uToken, BinaryReader reader)
		{
			int iHandle = reader.NetReadInt32();
			UInt64 zSizeInBytes = reader.NetReadUInt64();
			UInt64 uOffsetInBytes = reader.NetReadUInt64();

			// TODO: Enforce this on the client or support large sizes.
			byte[] aData = reader.ReadBytes((int)zSizeInBytes);

			// Get the entry - ignore the return value from TryGetValue(), we pass
			// a null RemoteFileEntry and use that to indicate failure.
			RemoteFileEntry entry = null;
			m_Files.TryGetValue(unchecked((UInt32)iHandle), out entry);

			// TODO: Add a log here, but need to filter it based on verbosity since
			// it can generate a lot of log spam if files are being streamed.

			// Construct the callback that will actually do the work.
			WaitCallback callback = new WaitCallback(delegate(object unusedArgument)
			{
				m_Connection.SendCommand(new WriteFileResponse(uToken, zSizeInBytes, uOffsetInBytes, entry, aData));
			});

			// Try to run the work item on another thread - if this fails, run it immediately.
			if (!ThreadPool.QueueUserWorkItem(callback))
			{
				callback(null);
			}
		}

		/// <summary>
		/// RPC handler when a GetDirectoryListing RPC is received -
		/// registered with a connection inside MoriartyConnection.
		/// </summary>
		public void GetDirectoryListing(MoriartyConnection.ERPC eRPC, UInt32 uToken, BinaryReader reader)
		{
			FilePath filePath = reader.NetReadFilePath();
			byte uFlags = reader.NetReadByte();
			string sFileExtension = reader.NetReadString();

			bool bIncludeSubdirectories = ((uFlags & MoriartyConnection.kFlag_GetDirectoryListing_IncludeSubdirectories) != 0);
			bool bRecursive = ((uFlags & MoriartyConnection.kFlag_GetDirectoryListing_Recursive) != 0);

			// Log the event
			m_Connection.LogMessage(eRPC, uToken, String.Format("Path={0} IncludeSubdirectories={1} Recursive={2} FileExtension={3}", filePath.ToString(), bIncludeSubdirectories, bRecursive, sFileExtension));

			// Construct the callback that will actually do the work.
			WaitCallback callback = new WaitCallback(delegate(object unusedArgument)
			{
				m_Connection.SendCommand(new GetDirectoryListingResponse(
					uToken,
					m_Connection.ConnectionManager.MoriartyManager.GetAbsoluteFilename(
						m_Connection.Platform,
						filePath),
					bIncludeSubdirectories,
					bRecursive,
					sFileExtension));
			});

			// Try to run the work item on another thread - if this fails, run it immediately.
			if (!ThreadPool.QueueUserWorkItem(callback))
			{
				callback(null);
			}
		}

		/// <summary>
		/// RPC handler when a CreateDirPath RPC is received - registered with
		/// a connection inside MoriartyConnection.
		/// </summary>
		public void CreateDirPath(MoriartyConnection.ERPC eRPC, UInt32 uToken, BinaryReader reader)
		{
			var filePath = reader.NetReadFilePath();

			// Log the set file modified time event.
			m_Connection.LogMessage(eRPC, uToken, filePath.ToString());

			// Construct the callback that will actually do the work.
			WaitCallback callback = new WaitCallback(delegate(object unusedArgument)
			{
				m_Connection.SendCommand(new CreateDirPathResponse(
					uToken,
					m_Connection.ConnectionManager.MoriartyManager.GetAbsoluteFilename(
						m_Connection.Platform,
						filePath)));
			});

			// Try to run the work item on another thread - if this fails, run it immediately.
			if (!ThreadPool.QueueUserWorkItem(callback))
			{
				callback(null);
			}
		}

		/// <summary>
		/// RPC handler when a Delete RPC is received - registered with
		/// a connection inside MoriartyConnection.
		/// </summary>
		public void Delete(MoriartyConnection.ERPC eRPC, UInt32 uToken, BinaryReader reader)
		{
			var filePath = reader.NetReadFilePath();

			// Log the set file modified time event.
			m_Connection.LogMessage(eRPC, uToken, filePath.ToString());

			// Construct the callback that will actually do the work.
			WaitCallback callback = new WaitCallback(delegate(object unusedArgument)
			{
				m_Connection.SendCommand(new DeleteResponse(
					uToken,
					m_Connection.ConnectionManager.MoriartyManager.GetAbsoluteFilename(
						m_Connection.Platform,
						filePath)));
			});

			// Try to run the work item on another thread - if this fails, run it immediately.
			if (!ThreadPool.QueueUserWorkItem(callback))
			{
				callback(null);
			}
		}

        /// <summary>
        /// RPC handler when a DeleteDirectory RPC is received - registered with
        /// a connection inside MoriartyConnection.
        /// </summary>
        public void DeleteDirectory(MoriartyConnection.ERPC eRPC, UInt32 uToken, BinaryReader reader)
        {
            var filePath = reader.NetReadFilePath();
            var bRecursive = reader.NetReadByte() == 0 ? false : true;

            // Log the set file modified time event.
            m_Connection.LogMessage(eRPC, uToken, filePath.ToString() + ":" + bRecursive.ToString());

            // Construct the callback that will actually do the work.
            WaitCallback callback = new WaitCallback(delegate (object unusedArgument)
            {
                m_Connection.SendCommand(new DeleteDirectoryResponse(
                    uToken,
                    m_Connection.ConnectionManager.MoriartyManager.GetAbsoluteFilename(
                        m_Connection.Platform,
                        filePath),
                    bRecursive));
            });

            // Try to run the work item on another thread - if this fails, run it immediately.
            if (!ThreadPool.QueueUserWorkItem(callback))
            {
                callback(null);
            }
        }

        /// <summary>
        /// RPC handler when a Rename RPC is received - registered with
        /// a connection inside MoriartyConnection.
        /// </summary>
        public void Rename(MoriartyConnection.ERPC eRPC, UInt32 uToken, BinaryReader reader)
		{
			var fromFilePath = reader.NetReadFilePath();
			var toFilePath = reader.NetReadFilePath();

			// Log the set file modified time event.
			m_Connection.LogMessage(eRPC, uToken, fromFilePath.ToString() + ":" + toFilePath.ToString());

			// Construct the callback that will actually do the work.
			WaitCallback callback = new WaitCallback(delegate(object unusedArgument)
			{
				m_Connection.SendCommand(new RenameResponse(
					uToken,
					m_Connection.ConnectionManager.MoriartyManager.GetAbsoluteFilename(
						m_Connection.Platform,
						fromFilePath),
					m_Connection.ConnectionManager.MoriartyManager.GetAbsoluteFilename(
						m_Connection.Platform,
						toFilePath)));
			});

			// Try to run the work item on another thread - if this fails, run it immediately.
			if (!ThreadPool.QueueUserWorkItem(callback))
			{
				callback(null);
			}
		}

		/// <summary>
		/// RPC handler when a SetReadOnlyBit RPC is received - registered with
		/// a connection inside MoriartyConnection.
		/// </summary>
		public void SetReadOnlyBit(MoriartyConnection.ERPC eRPC, UInt32 uToken, BinaryReader reader)
		{
			var filePath = reader.NetReadFilePath();
			var bReadOnlyBit = reader.NetReadByte() == 0 ? false : true;

			// Log the set file modified time event.
			m_Connection.LogMessage(eRPC, uToken, filePath.ToString() + ":" + bReadOnlyBit.ToString());

			// Construct the callback that will actually do the work.
			WaitCallback callback = new WaitCallback(delegate(object unusedArgument)
			{
				m_Connection.SendCommand(new SetReadOnlyBitResponse(
					uToken,
					m_Connection.ConnectionManager.MoriartyManager.GetAbsoluteFilename(
						m_Connection.Platform,
						filePath),
					bReadOnlyBit));
			});

			// Try to run the work item on another thread - if this fails, run it immediately.
			if (!ThreadPool.QueueUserWorkItem(callback))
			{
				callback(null);
			}
		}

		/// <summary>
		/// RPC handler when a Copy RPC is received - registered with
		/// a connection inside MoriartyConnection.
		/// </summary>
		public void Copy(MoriartyConnection.ERPC eRPC, UInt32 uToken, BinaryReader reader)
		{
			var fromFilePath = reader.NetReadFilePath();
			var toFilePath = reader.NetReadFilePath();
			var bAllowOverwrite = reader.NetReadByte() == 0 ? false : true;

			// Log the set file modified time event.
			m_Connection.LogMessage(eRPC, uToken, fromFilePath.ToString() + ":" + toFilePath.ToString() + ":" + bAllowOverwrite.ToString());

			// Construct the callback that will actually do the work.
			WaitCallback callback = new WaitCallback(delegate(object unusedArgument)
			{
				m_Connection.SendCommand(new CopyResponse(
					uToken,
					m_Connection.ConnectionManager.MoriartyManager.GetAbsoluteFilename(
						m_Connection.Platform,
						fromFilePath),
					m_Connection.ConnectionManager.MoriartyManager.GetAbsoluteFilename(
						m_Connection.Platform,
						toFilePath),
					bAllowOverwrite));
			});

			// Try to run the work item on another thread - if this fails, run it immediately.
			if (!ThreadPool.QueueUserWorkItem(callback))
			{
				callback(null);
			}
		}
	}
} // namespace Moriarty
