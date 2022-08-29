// ContentChangeManager monitors the current game's Source/ and Data/Config/ folders
// for file changes and informs all registered event handlers when they occur.
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
using Moriarty.Utilities;

namespace Moriarty
{
	/// <summary>
	/// Tracks a single change to a piece of content on disk. Includes the old and
	/// new file paths to the content (these will be the same unless it is a Renamed event)
	/// and the type of file change.
	/// </summary>
	public sealed class ContentChangeEvent
	{
		public ContentChangeEvent(FilePath oldFilePath, FilePath newFilePath, WatcherChangeTypes eEventType)
		{
			m_OldFilePath = oldFilePath;
			m_NewFilePath = newFilePath;
			m_eEventType = eEventType;
		}

		public FilePath m_OldFilePath = new FilePath();
		public FilePath m_NewFilePath = new FilePath();
		public WatcherChangeTypes m_eEventType = WatcherChangeTypes.All;
	}

	/// <summary>
	/// ContentChangeManager fires an event
	/// when a file change event occurs in the game's Source/ or Data/Config/ folders.
	/// </summary>
	public sealed class ContentChangeManager : IDisposable
	{
		#region Private members
		MoriartyManager m_Manager = null;
		FileSystemWatcher m_ConfigWatcher = null;
		FileSystemWatcher m_SourceWatcher = null;
		ConcurrentQueue<ContentChangeEvent> m_IncomingContentChanges = new ConcurrentQueue<ContentChangeEvent>();
		AutoResetEvent m_Event = new AutoResetEvent(false);
		volatile Thread m_Worker = null;
		volatile bool m_bWorkerRunning = false;

		/// <summary>
		/// Invoked when a file in Source/ or Data/Config changes.
		/// </summary>
		void OnFileChanged(
			bool bConfigFile,
			string sOldPath,
			string sNewPath,
			WatcherChangeTypes eEventType)
		{
			// Ignore changes to directories.
			if (Directory.Exists(sNewPath))
			{
				return;
			}

			// Construct the absolute path to the file.
			CoreLib.GameDirectory eGameDirectory = (bConfigFile ? CoreLib.GameDirectory.Config : CoreLib.GameDirectory.Content);
			string sAbsoluteDirectoryPath = (bConfigFile
				? m_Manager.GetGameDirectoryPath(CoreLib.Platform.Unknown, eGameDirectory)
				: m_Manager.GetGameDirectoryPathInSource(CoreLib.Platform.Unknown, eGameDirectory));

			// Compue the old and new file paths.
			FilePath oldPath = new FilePath();
			oldPath.m_eFileType = FileUtility.ExtensionToFileType(Path.GetExtension(sOldPath).ToLower());
			oldPath.m_eGameDirectory = eGameDirectory;
			oldPath.m_sRelativePathWithoutExtension = FileUtility.RemoveExtension(
				sOldPath.Substring(sAbsoluteDirectoryPath.Length));

			FilePath newPath = new FilePath();
			newPath.m_eFileType = FileUtility.ExtensionToFileType(Path.GetExtension(sNewPath).ToLower());
			newPath.m_eGameDirectory = eGameDirectory;
			newPath.m_sRelativePathWithoutExtension = FileUtility.RemoveExtension(
				sNewPath.Substring(sAbsoluteDirectoryPath.Length));

			// Queue up the change event and wake up the worker thread if necessary.
			m_IncomingContentChanges.Enqueue(new ContentChangeEvent(oldPath, newPath, eEventType));
			m_Event.Set();
		}

		/// <summary>
		/// Manages content changes - filters redundant change events and
		/// waits to dispatch events until a file is available for read on disk.
		/// </summary>
		void WorkerBody(object obj)
		{
			// Track changes that we're currently waiting to dispatch.
			Dictionary<FilePath, ContentChangeEvent> tChanges = new Dictionary<FilePath,ContentChangeEvent>();

			// If touched is true, we don't wait for the signal to fire, since there
			// will likely be pending content events to dispatch in the tChanges
			// dictionary.
			bool bTouched = false;

			// Keep polling until the class is being destroyed.
			while (m_bWorkerRunning)
			{
				// If we didn't process anything the last loop around, wait until we're
				// signaled to do work.
				if (!bTouched)
				{
					m_Event.WaitOne();
				}

				// Initially, no work was done.
				bTouched = false;

				// Exit out if the worker is no longer running
				if (!m_bWorkerRunning)
				{
					return;
				}

				// For each event in the queue, process redundant events.
				ContentChangeEvent p = null;
				if (m_IncomingContentChanges.TryDequeue(out p))
				{
					// If there was an event on the incoming queue, we've done work.
					bTouched = true;

					// If there already exists any entry for the target file,
					// merge this entry into it.
					ContentChangeEvent existing = null;
					if (tChanges.TryGetValue(p.m_NewFilePath, out existing))
					{
						// If the new entry is a rename entry, use the most recent
						// old name, and set the event to rename.
						if (WatcherChangeTypes.Renamed == p.m_eEventType)
						{
							existing.m_eEventType = WatcherChangeTypes.Renamed;
							existing.m_OldFilePath = p.m_OldFilePath;
						}
						// Else, just keep the existing entry.
					}
					// If there isn't an existing entry, just insert the incoming entry.
					else
					{
						tChanges.Add(p.m_NewFilePath, p);
					}

					// Loop back around, keep processing the incoming queue until it's empty.
					continue;
				}

				// Now process the entries table - for each entry, check if
				// the file can be opened - if this succeeds, dispatch the entry.
				Dictionary<FilePath, ContentChangeEvent> tRemainingChanges = new Dictionary<FilePath,ContentChangeEvent>();
				foreach (KeyValuePair<FilePath, ContentChangeEvent> t in tChanges)
				{
					// We've done work for this loop, so immediately try to do work
					// the next time around.
					bTouched = true;

					// For content, we're monitoring the source folder. Otherwise we're just
					// monitoring the config folder.
					string sAbsoluteFilename = (t.Value.m_NewFilePath.m_eGameDirectory == CoreLib.GameDirectory.Content
						? m_Manager.ProjectSettings.Settings.GetAbsoluteFilenameInSource(CoreLib.Platform.Unknown, t.Value.m_NewFilePath)
						: m_Manager.ProjectSettings.Settings.GetAbsoluteFilename(CoreLib.Platform.Unknown, t.Value.m_NewFilePath));

					// If the file path is a directory, skip it
					if (Directory.Exists(sAbsoluteFilename))
					{
						continue;
					}

					// Try to open the file - if this succeeds and it can be read,
					// insert the entry into the outgoing changes queue.
					bool bCanRead = false;
					try
					{
						using (FileStream file = File.OpenRead(sAbsoluteFilename))
						{
							bCanRead = file.CanRead;
						}
					}
					catch (FileNotFoundException)
					{
						// If the file doesn't exist, skip it (this can
						// sometimes happen for the temporary files that some
						// text editors leave behind)
						continue;
					}
					catch (DirectoryNotFoundException)
					{
						// Likewise for a missing directory in the file path
						continue;
					}
					catch (Exception)
					{
						// Treat any exceptions as an indication that the file
						// is still in use by the content export tool, and we
						// should wait to dispatch the change event.
						bCanRead = false;
					}

					// If the file is available for read, dispatch the change.
					if (bCanRead)
					{
						// Only dispatch if we have some events registered.
						if (null != ContentChange)
						{
							ContentChange(t.Value);
						}
					}
					// Otherwise, insert it into the remaining changes dictionary.
					else
					{
						tRemainingChanges.Add(t.Key, t.Value);
					}
				}

				// Exchange remaining changes for changes being processed.
				tChanges = tRemainingChanges;
			}
		}

		/// <returns>
		/// A FileSystemWatcher configured for use by ContentChangeManager, or null
		/// if an error occurs creating the watcher (directory not found).
		/// </returns>
		/// <param name="bConfigFile">
		/// True if the files that will be monitored by the returned
		/// watcher are Data/Config files, false otherwise.
		/// </param>
		FileSystemWatcher CreateContentWatcher(string sDirectoryPath, string sWildcard, bool bConfigFile)
		{
			FileSystemWatcher ret = null;

			try
			{
				// Create the watcher
				ret = new FileSystemWatcher(sDirectoryPath, sWildcard);

				// Disable watching while we setup callbacks and filters.
				ret.EnableRaisingEvents = false;

				// Setup event callbacks for changed events.
				ret.Changed += new FileSystemEventHandler(delegate(object unusedParameter, FileSystemEventArgs args)
				{
					// Let the renamed callbacks handle those event types.
					if (WatcherChangeTypes.Renamed != args.ChangeType)
					{
						OnFileChanged(bConfigFile, args.FullPath, args.FullPath, args.ChangeType);
					}
				});

				// Setup event callbacks for renamed events, specifically.
				ret.Renamed += new RenamedEventHandler(delegate(object unusedParameter, RenamedEventArgs args)
				{
					OnFileChanged(bConfigFile, args.OldFullPath, args.FullPath, args.ChangeType);
				});

				// Configure remaining settings.
				ret.IncludeSubdirectories = true;
				ret.NotifyFilter = NotifyFilters.CreationTime | NotifyFilters.FileName | NotifyFilters.LastWrite | NotifyFilters.Size;

				// Turn on event notification.
				ret.EnableRaisingEvents = true;
			}
			catch (Exception e)
			{
				m_Manager.Log.LogMessage("Failed creating FileSystemWatcher for \"" + sDirectoryPath + "\", check that " +
					"this directory exists. This will disable cooking support. If you are using Moriarty for launching and " +
					"cheats only, you can safely ignore this message. Exception: " + e.Message);

				// Dispose a watcher that was partially setup.
				if (null != ret)
				{
					ret.Dispose();
					ret = null;
				}
			}

			// Return the created watcher.
			return ret;
		}
		#endregion

		public ContentChangeManager(MoriartyManager manager)
		{
			m_Manager = manager;

			// Construct the Config and Source directory watchers.
			m_ConfigWatcher = CreateContentWatcher(
				m_Manager.ProjectSettings.Settings.GetGameDirectoryPath(
					CoreLib.Platform.Unknown,
					CoreLib.GameDirectory.Config),
				"*.*",
				true);

			// NOTE: We only monitor Authored\\ because all the generated images by design,
			// never change (either new files are added, or existing files are reused). If that changes,
			// this should also change.
			m_SourceWatcher = CreateContentWatcher(
				Path.Combine(
					m_Manager.ProjectSettings.Settings.GetGameDirectoryPathInSource(
						CoreLib.Platform.Unknown,
						CoreLib.GameDirectory.Content),
					"Authored"),
				"*.*",
				false);

			if (null != m_ConfigWatcher && null != m_SourceWatcher)
			{
				// Start up the worker to process events.
				m_bWorkerRunning = true;
				m_Worker = new Thread(WorkerBody);
				m_Worker.Start();
			}
		}

		/// <summary>
		/// Cleanup resources in use by this ContentChangeManager.
		/// </summary>
		public void Dispose()
		{
			if (m_bWorkerRunning)
			{
				m_bWorkerRunning = false;
				m_Event.Set();
				while (m_Worker.IsAlive) ;
				m_Worker = null;
			}

			if (null != m_SourceWatcher)
			{
				m_SourceWatcher.Dispose();
				m_SourceWatcher = null;
			}

			if (null != m_ConfigWatcher)
			{
				m_ConfigWatcher.Dispose();
				m_ConfigWatcher = null;
			}
		}

		// Event that is triggered when a content change occurs.
		public delegate void ContentChangeDelegate(ContentChangeEvent evt);
		public event ContentChangeDelegate ContentChange;
	}
} // namespace Moriarty
