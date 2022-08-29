// Implements functionality to log events to a single file. Also maintains
// a buffer of recently logged events and handles details like timestamps and
// event types.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;

using Moriarty.Extensions;

namespace Moriarty.Utilities
{
	/// <summary>
	/// Logger manages logging of string events to a single file. The most recent
	/// n lines (configurable) can also be queried for UI display or other such purpose.
	/// </summary>
	public sealed class Logger : IDisposable
	{
		#region Private members
		ConcurrentQueue<string> m_Input = new ConcurrentQueue<string>();
		AutoResetEvent m_Event = new AutoResetEvent(false);
		Thread m_Worker = null;
		volatile bool m_bWorkerIsRunning = false;
		TextWriter m_Writer = null;
		ConcurrentQueue<string> m_LogBuffer = new ConcurrentQueue<string>();
		readonly int m_nLogBufferLines = 0;
		string[] m_asLogBuffer = new string[0];

		/// <summary>
		/// Runs on a worker thread, handles formatting and sending lines to the log file.
		/// </summary>
		void WorkerBody()
		{
			while (m_bWorkerIsRunning)
			{
				bool bLogChanged = false;

				// Keep pumping the input queue until we run out of data.
				string sInput = string.Empty;
				while (m_Input.TryDequeue(out sInput))
				{
					// Normalize line endings
					sInput = sInput.Replace("\r\n", "\n");
					sInput = sInput.Replace('\r', '\n');

					// Split the string
					string[] asLines = sInput.Split('\n');

					// For each line in the input string, write it to the log
					// with a timestamp.
					foreach (string sLine in asLines)
					{
						if (string.IsNullOrWhiteSpace(sLine))
						{
							continue;
						}

						if (null != m_Writer)
						{
							m_Writer.WriteLine(DateTime.Now + ": " + sLine);
						}
						// TODO: Probably a better fallback than just writing to the console.
						else
						{
							Console.WriteLine(DateTime.Now + ": " + sLine);
						}

						// Add the lone to the log buffer.
						m_LogBuffer.Enqueue(sLine);

						// Mark that the log was modified, so we know
						// to trigger the "log buffer changed" event.
						bLogChanged = true;
					}
				}

				// Keep the log buffer at its maximum size.
				string sUnusedResult = string.Empty;
				while (m_LogBuffer.Count > m_nLogBufferLines)
				{
					if (!m_LogBuffer.TryDequeue(out sUnusedResult))
					{
						break;
					}
				}

				// Update the lines buffer.
				m_asLogBuffer = m_LogBuffer.ToArray();

				// If the log change, flushtrigger the log buffer changed
				// event.
				if (bLogChanged && null != LogBufferChanged)
				{
					LogBufferChanged();
				}

				// Wait until more input is available.
				m_Event.WaitOne();
			}
		}

		/// <summary>
		/// P/Invoke binding of the Win32 CreateHardLink() function
		/// </summary>
		[DllImport("Kernel32.dll", CharSet = CharSet.Unicode)]
		static extern bool CreateHardLink(
			string lpFileName,
			string lpExistingFileName,
			IntPtr lpSecurityAttributes);
		#endregion

		public Logger(string sAbsoluteLogFilename)
			: this(sAbsoluteLogFilename, true)
		{
		}

		public Logger(string sAbsoluteLogFilename, bool bGenerateTimestampedLogFilename)
			: this(sAbsoluteLogFilename, bGenerateTimestampedLogFilename, 500)
		{
		}

		/// <summary>
		/// Construct this Logger with an absolute filename to write to.
		/// </summary>
		/// <param name="bGenerateTimestampedLogFilename">
		/// If true, the actual file written will
		/// include a date and time stamp. The sLogFilename will be a hard link to the actual
		/// file.
		/// </param>
		/// <param name="nLogBufferInLines">
		/// The maximum number of lines kept in-memory after writing to
		/// the log. Can be retrieved with the LogBuffer property.
		/// </param>
		public Logger(string sAbsoluteLogFilename, bool bGenerateTimestampedLogFilename, int nLogBufferInLines)
		{
			m_nLogBufferLines = nLogBufferInLines;

			// If we're using timestamps, modify the file, and create the hard link.
			if (bGenerateTimestampedLogFilename)
			{
				string sTimestampedLogFilename = Path.Combine(
					Path.GetDirectoryName(sAbsoluteLogFilename),
					Path.GetFileNameWithoutExtension(sAbsoluteLogFilename));
				sTimestampedLogFilename +=
					"-" +
					DateTime.Now.ToFilenameString() +
					Path.GetExtension(sAbsoluteLogFilename);

				// Make sure the path to the log file exists.
				CoreLib.Utilities.CreateDirectoryDependencies(Path.GetDirectoryName(sTimestampedLogFilename));

				try
				{
					m_Writer = new StreamWriter(sTimestampedLogFilename);

					if (File.Exists(sAbsoluteLogFilename))
					{
						File.Delete(sAbsoluteLogFilename);
					}
					CreateHardLink(sAbsoluteLogFilename, sTimestampedLogFilename, IntPtr.Zero);
				}
				catch (Exception)
				{
					// TODO: Report this error
				}
			}
			// Otherwise, just write to the output file.
			else
			{
				try
				{
					m_Writer = new StreamWriter(sAbsoluteLogFilename);
				}
				catch (Exception)
				{
					// TODO: Report this error
				}
			}

			// Start the logger process thread.
			m_Worker = new Thread(WorkerBody);
			m_Worker.Name = "Logger Worker Thread";
			m_bWorkerIsRunning = true;
			m_Worker.Start();
		}

		public void Dispose()
		{
			if (m_bWorkerIsRunning)
			{
				m_bWorkerIsRunning = false;
				m_Event.Set();
				while (m_Worker.IsAlive) ;
				m_Worker = null;
			}

			if (null != m_Writer)
			{
				m_Writer.Close();
				m_Writer.Dispose();
				m_Writer = null;
			}
		}

		/// <returns>
		/// The last n lines written to the log, where n is specified to this
		/// Logger's constructor.
		///
		/// \warning This property can cause thread contention and also allocates memory,
		/// avoid invoking it with high frequency.
		/// </returns>
		public string[] LogBuffer
		{
			get
			{
				return m_asLogBuffer;
			}
		}

		/// <summary>
		/// Enqueue a message to be written to the log.
		/// </summary>
		public void LogMessage(string sMessage)
		{
			Debug.WriteLine(sMessage);

			m_Input.Enqueue(sMessage);
			m_Event.Set();
		}

		/// <summary>
		/// Triggered when the LogBuffer member has been modified
		/// </summary>
		public event Action LogBufferChanged;
	}
} // namespace Moriarty.Utilities
