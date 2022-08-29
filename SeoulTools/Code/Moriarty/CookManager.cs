// Single choke point for cooking. It is not safe (in general) to run multiple
// cook processes simultaneously, so CookManager serves as a queue that runs cooks
// one at a time and informs the associated MoriartyConnection when the cook has
// completed.
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
using System.Runtime.InteropServices;
using System.Threading;

using Moriarty.Utilities;

namespace Moriarty
{
	/// <summary>
	/// Results of an attempted Cook operation. Must be kept in-sync with
	/// the equivalent CookResult enum in the SeoulEngine codebase in CookManager.h
	/// </summary>
	public enum CookResult
	{
		/// <summary>
		/// Should be returned if cooking was requested, was executed, and was successful
		/// </summary>
		Success = 1,

		/// <summary>
		/// Should be returned if cooking was requested with bOnlyIfNeeded = true and a
		/// cooked version of the file already exists which is up-to-date with the source file.
		/// </summary>
		UpToDate,

		/// <summary>
		/// Should be returned if cooking is temporarily disabled with a call to
		/// CookManager::SetCookingEnabled().
		/// </summary>
		ErrorCookingDisabled,

		/// <summary>
		/// Should be returned if some underlying support necessary for the type
		/// of cook was not found - for example, on PC, this might mean an external cooker
		/// EXE is missing.
		/// </summary>
		ErrorMissingCookerSupport,

		/// <summary>
		/// Should be returned if cooking is attempted on an unsupported file type
		/// </summary>
		ErrorCannotCookFileType,

		/// <summary>
		/// Should be returned if the source of a requested cook does not exist
		/// </summary>
		ErrorSourceFileNotFound,

		/// <summary>
		/// Should be returned if cooking was attempted but failed for some reason
		/// </summary>
		ErrorCookingFailed
	};

	public sealed class CookManager : IDisposable
	{
		/// <summary>
		/// A single cooking task - cooks a single file or type of files
		/// (if the path and directory parts of m_FilePath are default values),
		/// and then reports the result to m_Connection.
		/// </summary>
		public sealed class CookEntry
		{
			public MoriartyConnection m_Connection;
			public FilePath m_FilePath;
			public CoreLib.Platform m_ePlatform;
			public bool m_bOnlyIfNeeded;
			public UInt32 m_uResponseToken;
		}

		/// <summary>
		/// A single test task - checks if any of the listed files are out of sync
		/// between the source and cooked version.
		/// </summary>
		public sealed class TestEntry
		{
			public MoriartyConnection m_Connection;
			public CoreLib.Platform m_ePlatform;
			public UInt32 m_uResponseToken;
			public FilePath[] m_aFilePaths;
		}

		#region Private members
		Dictionary<CoreLib.Platform, CoreLib.CookDatabase> m_tCookDatabases = new Dictionary<CoreLib.Platform, CoreLib.CookDatabase>();
		CoreLib.CookDatabase CookDatabase(CoreLib.Platform ePlatform)
		{
			CoreLib.CookDatabase ret;
			lock (m_tCookDatabases)
			{
				if (!m_tCookDatabases.TryGetValue(ePlatform, out ret))
				{
					ret = new CoreLib.CookDatabase(ePlatform);
					m_tCookDatabases.Add(ePlatform, ret);
				}
			}

			return ret;
		}
		MoriartyManager m_Manager = null;
		ConcurrentQueue<CookEntry> m_CookQueue = new ConcurrentQueue<CookEntry>();
		ConcurrentQueue<TestEntry> m_TestQueue = new ConcurrentQueue<TestEntry>();
		volatile bool m_bWorkerThreadRunning = false;
		volatile Thread m_WorkerThread = null;
		AutoResetEvent m_Event = new AutoResetEvent(false);

		/// <returns>
		/// The base arguments string for cooking, same across all cook types.
		/// </returns>
		string GetBaseCommandLineArguments(CookEntry entry)
		{
			string sArguments = string.Empty;

			// Add arguments used for all cook types done by Moriarty's CookManager.
			sArguments += "-local -platform ";
			sArguments += CoreLib.PlatformUtility.ToString(entry.m_ePlatform);

			return sArguments;
		}

		/// <summary>
		/// Cooks a single one-to-one file defined by entry.
		/// </summary>
		/// <returns>
		/// True if the cook was successful, false otherwise.
		/// </returns>
		CookResult DoOneToOneCook(CookEntry entry)
		{
			string sSourceFilename = FileUtility.ResolveAndSimplifyPath(
				m_Manager.GetAbsoluteFilenameInSource(entry.m_ePlatform, entry.m_FilePath));
			string sCookedFilename = FileUtility.ResolveAndSimplifyPath(
				m_Manager.GetAbsoluteFilename(entry.m_ePlatform, entry.m_FilePath));

			// If there is no source file, nothing more to do.
			if (!File.Exists(sSourceFilename))
			{
				return CookResult.ErrorSourceFileNotFound;
			}

			// If we're conditionally cooking, check if up-to-date.
			if (entry.m_bOnlyIfNeeded)
			{
				if (CookDatabase(entry.m_ePlatform).IsUpToDate(sCookedFilename))
				{
					return CookResult.UpToDate;
				}
			}

			// Create the path up to the location of the file that will be cooked - if this fails,
			// we can't cook.
			if (!CoreLib.Utilities.CreateDirectoryDependencies(Path.GetDirectoryName(sCookedFilename)))
			{
				return CookResult.ErrorCookingFailed;
			}

			// Construct the cooker path and the command-line arguments string.
			string sCookerExecutableFilename =
				Path.Combine(
					m_Manager.GetGameDirectoryPath(entry.m_ePlatform, CoreLib.GameDirectory.ToolsBin),
					"Cooker.exe");

			// If the cooker does not exist, report the error.
			if (!File.Exists(sCookerExecutableFilename))
			{
				return CookResult.ErrorMissingCookerSupport;
			}

			// Arguments for a one-to-one cook.
			string sArguments = GetBaseCommandLineArguments(entry);
			sArguments += " -out_file ";
			sArguments += sCookedFilename;

			// Notify that the cook is happening.
			m_Manager.Log.LogMessage("Cooking with '" + sCookerExecutableFilename + "', with arguments: '" + sArguments + "'.");

			// Run the cooking, if this fails, return a failed cook attempt.
			if (!CoreLib.Utilities.RunCommandLineProcess(
				sCookerExecutableFilename,
				sArguments,
				m_Manager.Log.LogMessage,
				m_Manager.Log.LogMessage))
			{
				m_Manager.Log.LogMessage("Failed cooking \"" + sSourceFilename + "\".");
				return CookResult.ErrorCookingFailed;
			}

			// Note that cooking completed successfully.
			m_Manager.Log.LogMessage("Done cooking \"" + sSourceFilename + "\".");

			return CookResult.Success;
		}

		/// <summary>
		/// Process all entries on the cook queue.
		/// </summary>
		void ProcessCookQueue()
		{
			CookEntry entry = null;
			while (m_bWorkerThreadRunning && m_CookQueue.TryDequeue(out entry))
			{
				CookResult eResult = DoOneToOneCook(entry);
				string sAbsoluteFilename = m_Manager.GetAbsoluteFilename(
					entry.m_ePlatform,
					entry.m_FilePath);
				entry.m_Connection.SendCommand(new Commands.CookFileResponse(
					entry.m_FilePath,
					CoreLib.Utilities.GetFileSize(sAbsoluteFilename),
					CoreLib.Utilities.GetModifiedTime(sAbsoluteFilename),
					entry.m_uResponseToken,
					eResult));
			}
		}

		/// <summary>
		/// Thread body that processes entries on the CookManager queue, one-by-one, and
		/// actually executes the cook process on those entries.
		/// </summary>
		void WorkerBody(object obj)
		{
			while (m_bWorkerThreadRunning)
			{
				ProcessCookQueue();

				m_Event.WaitOne();
			}
		}

		const string kSeoulUtilDLL = "SeoulUtil.dll";
		[DllImport(kSeoulUtilDLL, CallingConvention = CallingConvention.StdCall)]
		static extern void Seoul_InitCore();

		[DllImport(kSeoulUtilDLL, CallingConvention = CallingConvention.StdCall)]
		static extern void Seoul_DeInitCore();
		#endregion

		public CookManager(MoriartyManager manager)
		{
			Seoul_InitCore();

			m_Manager = manager;
			m_WorkerThread = new Thread(WorkerBody);
			m_WorkerThread.Name = "CookManager Worker Thread";
			m_bWorkerThreadRunning = true;
			m_WorkerThread.Start();
		}

		public void Dispose()
		{
			if (m_bWorkerThreadRunning)
			{
				m_bWorkerThreadRunning = false;
				m_Event.Set();
				while (m_WorkerThread.IsAlive) ;
				m_WorkerThread = null;
			}

			Seoul_DeInitCore();
		}

		/// <summary>
		/// Enqueue a cook operation that will cook FilePath. If bOnlyIfNeeded
		/// is true, the file will only be cooked if it is out-of-date with its source, otherwise
		/// it will always be cooked.
		/// </summary>
		public void Cook(
			UInt32 uResponseToken,
			MoriartyConnection connection,
			FilePath filePath,
			bool bOnlyIfNeeded)
		{
			// If the file path is empty, fail immediately.
			if (string.IsNullOrEmpty(filePath.m_sRelativePathWithoutExtension))
			{
				connection.SendCommand(new Commands.CookFileResponse(filePath, 0, 0, uResponseToken, CookResult.ErrorSourceFileNotFound));
				return;
			}

			CookEntry entry = new CookEntry();
			entry.m_Connection = connection;
			entry.m_ePlatform = connection.Platform;
			entry.m_FilePath = filePath;
			entry.m_bOnlyIfNeeded = bOnlyIfNeeded;
			entry.m_uResponseToken = uResponseToken;
			m_CookQueue.Enqueue(entry);
			m_Event.Set();
		}

		/// <summary>
		/// Enqueue a cook test operation that will send a true result if any file in the array
		/// is out-of-sync with its source content, a false result otherwise.
		/// </summary>
		public void IsAnyFileOutOfDate(
			UInt32 uResponseToken,
			MoriartyConnection connection,
			FilePath[] aFilePaths)
		{
			TestEntry entry = new TestEntry();
			entry.m_Connection = connection;
			entry.m_ePlatform = connection.Platform;
			entry.m_uResponseToken = uResponseToken;
			entry.m_aFilePaths = aFilePaths;
			m_TestQueue.Enqueue(entry);
			m_Event.Set();
		}
	}
}
