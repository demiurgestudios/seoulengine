// Encapsulates a single connection between the Moriarty application
// and a client program. Each connection has its own socket, its own worker
// thread, and its own (mostly) independent processing. Common processing
// may occur in MoriartyManager when appropriate (i.e. cooking, which can only
// be safely done one at a time).
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
using System.Net;
using System.Net.Sockets;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

using Moriarty.Commands;
using Moriarty.Extensions;
using Moriarty.Utilities;

namespace Moriarty
{
	public sealed class MoriartyConnection : IDisposable
	{
		#region Data validation constants - must be kept in sync with equivalent values in MoriartyRPC.h in the SeoulEngine codebase
		/// <summary>
		/// Moriarty protocol version -- increment this every time you make a
		/// non-backwards-compatible change
		/// </summary>
		public const UInt32 kProtocolVersion = 0;

		/// <summary>
		/// Magic constant sent after initiating a connection to avoid other applications
		/// accidentally connecting to a Moriarty server and trying to use some other
		/// protocol.
		/// </summary>
		public const UInt32 kConnectMagic = 0x0DDD15C5;

		/// <summary>
		/// Magic constant sent back from the server to the client after a connection is
		/// initiated.
		/// </summary>
		public const UInt32 kConnectResponseMagic = 0xABADD00D;
		#endregion

		#region RPC Constants
		/// <summary>
		/// Enumeration of all RPC types.
		/// </summary>
		/// <remarks>
		/// Must be kept in-sync with equivalent values in the SeoulEngine code base
		/// in MoriartyRPC.h
		/// </remarks>
		[Flags]
		public enum ERPC
		{
			LogMessage = 0,
			StatFile,
			OpenFile,
			CloseFile,
			ReadFile,
			WriteFile,
			SetFileModifiedTime,
			GetDirectoryListing,
			CookFile,
			KeyboardKeyEvent,
			ContentChangeEvent,
			KeyboardCharEvent,
			StatFileCacheRefreshEvent,
			CreateDirPath,
			Delete,
			Rename,
			SetReadOnlyBit,
			Copy,
            DeleteDirectory,

            /// <summary>
            /// Flag indicating that a message is an RPC respons
            /// </summary>
            ResponseFlag = 0x80,

			Unknown = int.MaxValue,
		};

		/// <returns>
		/// True if the incoming RPC type eRPC is expected
		/// to include a response token, false otherwise.
		/// </returns>
		public static bool RequiresResponseToken(ERPC eRPC)
		{
			return (ERPC.LogMessage != eRPC);
		}

		/// <summary>
		/// Result codes and error numbers for RPCs.
		/// </summary>
		/// <remarks>
		/// Must be kept in-sync with equivalent values in SeoulEngine code base
		/// in MoriartyRPC.h
		/// </remarks>
		public enum EResult
		{
			ResultSuccess,   //< RPC succeeded
			ResultRPCFailed, //< RPC failed to complete (e.g. socket was unexpectedly closed)
			ResultCanceled,  //< RPC was canceled

			// For all of the following codes, the RPC was completed but failed on the
			// server for some other reason
			ResultGenericFailure, //< Unspecified failure
			ResultFileNotFound,   //< File not found (ENOENT)
			ResultAccessDenied,   //< Access denied (EACCES)
		};

		/// <summary>
		/// kStatFile flags - keep in-sync with equivalent value in SeoulEngine, MoriartyRPC.h
		/// </summary>
		public const byte kFlag_StatFile_Directory = 0x01;

		/// <summary>
		/// kGetDirectoryListing flags - keep in-sync with equivalent values in SeoulEngine, MoriartyRPC.
		/// </summary>
		public const byte kFlag_GetDirectoryListing_IncludeSubdirectories = 0x01;
		public const byte kFlag_GetDirectoryListing_Recursive             = 0x02;

		/// <summary>
		/// kCookFile flags - keep in-sync with equivalent value in SeoulEngine, MoriartyRPC.h
		/// </summary>
		public const byte kFlag_CookFile_CheckTimestamp = 0x01;
		#endregion

		#region Private members
		ConcurrentQueue<Commands.BaseMoriartyCommand> m_CommandQueue = new ConcurrentQueue<Commands.BaseMoriartyCommand>();
		MoriartyConnectionManager m_ConnectionManager = null;
		RemoteFiles m_RemoteFiles = null;
		volatile bool m_bPendingConnection = true;
		volatile bool m_bConnected = false;
		volatile CoreLib.Platform m_ePlatform = CoreLib.Platform.Unknown;
		volatile Thread m_ReadWorker = null;
		volatile Thread m_WriteWorker = null;
		AutoResetEvent m_WriteEvent = new AutoResetEvent(false);
		volatile bool m_bWorkerRunning = false;
		Socket m_Socket = null;
		NetworkStream m_Stream = null;
		string m_sName = string.Empty;

		Logger m_ClientLog = null;
		Logger m_ServerLog = null;

		// Handlers used to dispatch RPC receives
		delegate void RPCHandler(ERPC eRPC, UInt32 uToken, BinaryReader reader);
		readonly RPCHandler[] m_kaRPCHandlers = null;

		/// <summary>
		/// Setup the mapping between RPC codes and handler delegates for
		/// this MoriartyConnection.
		/// </summary>
		RPCHandler[] GetRPCHandlers()
		{
			RPCHandler[] aReturn = new RPCHandler[Enum.GetNames(typeof(ERPC)).Length];

			// Setup implemented handlers - leaving unimplemented handler as NULL is fine,
			// as this will trigger an exception in the read loop, which is what we want,
			// since if we can't handle an RPC, we'll just put the client in a dead lock
			// state if we don't close the connection.
			aReturn[(int)ERPC.LogMessage] = LogMessage;
			aReturn[(int)ERPC.StatFile] = m_RemoteFiles.StatFile;
			aReturn[(int)ERPC.OpenFile] = m_RemoteFiles.OpenFile;
			aReturn[(int)ERPC.CloseFile] = m_RemoteFiles.CloseFile;
			aReturn[(int)ERPC.ReadFile] = m_RemoteFiles.ReadFile;
			aReturn[(int)ERPC.WriteFile] = m_RemoteFiles.WriteFile;
			aReturn[(int)ERPC.SetFileModifiedTime] = m_RemoteFiles.SetFileModifiedTime;
			aReturn[(int)ERPC.GetDirectoryListing] = m_RemoteFiles.GetDirectoryListing;
			aReturn[(int)ERPC.CookFile] = CookFile;
			aReturn[(int)ERPC.CreateDirPath] = m_RemoteFiles.CreateDirPath;
			aReturn[(int)ERPC.Delete] = m_RemoteFiles.Delete;
			aReturn[(int)ERPC.Rename] = m_RemoteFiles.Rename;
			aReturn[(int)ERPC.SetReadOnlyBit] = m_RemoteFiles.SetReadOnlyBit;
			aReturn[(int)ERPC.Copy] = m_RemoteFiles.Copy;
            aReturn[(int)ERPC.DeleteDirectory] = m_RemoteFiles.DeleteDirectory;

			return aReturn;
		}

		/// <summary>
		/// RPC handler when an ERPC.CookFile is received - a cook is
		/// enqueued in the CookManager. It will handle sending the response
		/// token once the cook has completed.
		/// </summary>
		void CookFile(ERPC eRPC, UInt32 uResponseToken, BinaryReader reader)
		{
			FilePath filePath = reader.NetReadFilePath();
			bool bOnlyIfNeeded = (reader.NetReadByte() == kFlag_CookFile_CheckTimestamp);

			m_ConnectionManager.MoriartyManager.CookManager.Cook(
				uResponseToken,
				this,
				filePath,
                bOnlyIfNeeded);
		}

		/// <summary>
		/// RPC handler when an ERPC.LogMessage RPC is received - uResponseToken
		/// is ignored, and the message is simply echoed to the local client log.
		/// </summary>
		void LogMessage(ERPC eRPC, UInt32 uResponseToken, BinaryReader reader)
		{
			string sMessage = reader.NetReadString();
			m_ClientLog.LogMessage(sMessage);
		}

		/// <summary>
		/// Body of the thread that handles writing commands to the socket.
		/// </summary>
		void WriteWorkerBody(object obj)
		{
			try
			{
				// Don't start writing until the read thread has verified
				// the connection.
				while (m_bWorkerRunning && !m_bConnected)
				{
					Thread.Yield();
				}

				// If we're still running, enter the write loop.
				if (m_bWorkerRunning)
				{
					// Buffer writes to the stream so that individual commands
					// get sent out in a single send() call
					using (BufferedStream bufferedStream = new BufferedStream(m_Stream))
					{
						// Construct a BinaryWriter on the stream.
						using (BinaryWriter writer = new BinaryWriter(bufferedStream))
						{
							// Keep looping until we're told to exit.
							while (m_bWorkerRunning)
							{
								// Track if any data was actually written.
								bool bWritten = false;

								// Process all commands in the queue.
								Commands.BaseMoriartyCommand command = null;
								while (
									m_bWorkerRunning &&
									m_CommandQueue.TryDequeue(out command))
								{
									command.Write(writer);
									bWritten = true;
									command = null;
								}

								// If any data was written and we're still running, Flush the writer.
								if (m_bWorkerRunning &&
									bWritten)
								{
									writer.Flush();
								}

								// Wait until we have more data to write.
								m_WriteEvent.WaitOne();
							}
						}
					}
				}
			}
			catch (Exception e)
			{
				if (!(e is SocketException || e is EndOfStreamException || e is IOException))
				{
					// Unexpected exception, log to the manager and exit the Write loop.
					m_ConnectionManager.MoriartyManager.Log.LogMessage("Write Loop: Unexpected exception from MoriartyConnection \"" +
						ToString() + "\": " + e.Message + "\n" + e.StackTrace);
				}
			}
		}

		/// <summary>
		/// Body of the thread that handles reading commands from a socket.
		/// </summary>
		void ReadWorkerBody(object obj)
		{
			try
			{
				using (BinaryReader reader = new BinaryReader(m_Stream))
				{
					// Handshake
					UInt32 uProtocolVersion = reader.NetReadUInt32();
					UInt32 uConnectMagic = reader.NetReadUInt32();
					if (kProtocolVersion != uProtocolVersion ||
						kConnectMagic != uConnectMagic)
					{
						m_ServerLog.LogMessage("Invalid connection.");
						m_bPendingConnection = false;
						return;
					}

					// Platform
					m_ePlatform = (CoreLib.Platform)reader.NetReadUInt32();

					// Cache the name, so we don't need to use RemoteEndPoint when it can potentially throw an
					// exception.
					try
					{
						m_sName = CoreLib.PlatformUtility.ToString(m_ePlatform) + " (" + m_Socket.RemoteEndPoint.ToString() + ")";
					}
					catch (Exception)
					{
						m_sName = "Unknown Connection";
					}

					// Send response.
					using (NetworkStream stream = new NetworkStream(m_Socket, FileAccess.ReadWrite))
					{
						using (BufferedStream bufferedStream = new BufferedStream(stream))
						{
							using (BinaryWriter writer = new BinaryWriter(bufferedStream))
							{
								writer.NetWriteUInt32(kProtocolVersion);
								writer.NetWriteUInt32(kConnectResponseMagic);
							}
						}
					}

					// We've now connected and are no longer pending a connection,
					// tell our ConnectionManager that we've successfully connected.
					m_bConnected = true;
					m_bPendingConnection = false;
					m_ConnectionManager.HandleConnect(this);

					// Receive loop
					while (m_bWorkerRunning)
					{
						// Get the next RPC.
						ERPC eRPC = (ERPC)reader.NetReadByte();
						UInt32 uToken = 0u;

						// If the RPC type requires a response token, read it,
						// otherwise leave the token value at 0 since it is expected
						// to be unused.
						if (RequiresResponseToken(eRPC))
						{
							uToken = reader.NetReadUInt32();
						}

						// Note that, if eRPC is invalid, this will throw an ArgumentOutOfRange exception,
						// which is what we want - if we've got an unknown RPC, we're boned - we can't
						// properly read its data, so leaving the connection open will likely just result
						// in a dead locked client (waiting for a response that will never come).
						m_kaRPCHandlers[(int)eRPC](eRPC, uToken, reader);
					}
				}
			}
			catch (Exception e)
			{
				// Make sure our connection is no longer marked as pending if an operation
				// failed during the initial handshake.
				m_bPendingConnection = false;

				// If not an expected exception type, log an error.
				if (!(e is SocketException || e is EndOfStreamException || e is IOException))
				{
					// This is an unexpected exception - log it to the manager Log, but otherwise
					// handle it like a SocketException.
					m_ConnectionManager.MoriartyManager.Log.LogMessage("Read Loop: Unexpected exception from MoriartyConnection \"" +
						ToString() + "\": " + e.Message + "\n" + e.StackTrace);
				}
			}

			// Tell the connection manager that we're no longer connected.
			m_ConnectionManager.HandleDisconnect(this);
			m_bConnected = false;
		}

		/// <summary>
		/// Invoked by ContentChangeManager when a content change event occurs,
		/// we send this to our client if it's still connected.
		/// </summary>
		void ContentChangeManager_ContentChange(ContentChangeEvent evt)
		{
			if (HasConnection)
			{
				string sAbsoluteFilename = m_ConnectionManager.MoriartyManager.GetAbsoluteFilename(
					m_ePlatform,
					evt.m_NewFilePath);

				SendCommand(new ContentChangeEventCommand(
					evt.m_OldFilePath,
					evt.m_NewFilePath,
					CoreLib.Utilities.GetFileSize(sAbsoluteFilename),
					CoreLib.Utilities.GetModifiedTime(sAbsoluteFilename),
					ContentChangeEventCommand.ToContentChangeEventType(evt.m_eEventType)));
			}
		}
		#endregion

		/// <summary>
		/// Structure for setting TCP keep-alive parameters (from MSTcpIP.h)
		/// </summary>
		/// <remarks>
		/// struct tcp_keepalive {
		/// ULONG onoff;
		/// ULONG keepalivetime;
		/// ULONG keepaliveinterval;
		/// };
		/// </remarks>
		[StructLayout(LayoutKind.Explicit)]
		struct TcpKeepAlive
		{
			/// <summary>
			/// 0 to disable keep-alive, non-0 to enable keep-aliv
			/// </summary>
			[FieldOffset(0)]
			public uint OnOff;

			/// <summary>
			/// Idle time after which keep-alive probes will be sent, in m
			/// </summary>
			[FieldOffset(4)]
			public uint KeepAliveTime;

			/// <summary>
			/// Interval between keep-alive probes, in m
			/// </summary>
			[FieldOffset(8)]
			public uint KeepAliveInterval;

			/// <summary>
			/// Converts this structure to a byte arra
			/// </summary>
			public byte[] ToByteArray()
			{
				int size = Marshal.SizeOf(this);
				byte[] resultArray = new byte[size];
				IntPtr thisPtr = Marshal.AllocHGlobal(size);

				Marshal.StructureToPtr(this, thisPtr, true);
				Marshal.Copy(thisPtr, resultArray, 0, size);
				Marshal.FreeHGlobal(thisPtr);

				return resultArray;
			}
		}

		public MoriartyConnection(
			MoriartyConnectionManager connectionManager,
			Socket socket)
		{
			m_ConnectionManager = connectionManager;
			m_RemoteFiles = new RemoteFiles(this);
			m_Socket = socket;
			m_Stream = new NetworkStream(m_Socket, FileAccess.ReadWrite);

			// Disable the Nagle algorithm on the socket.
			m_Socket.SetSocketOption(SocketOptionLevel.Tcp, SocketOptionName.NoDelay, true);

			// Enable TCP keep-alive and set keep-alive interval to 1 minute
			TcpKeepAlive keepAliveOptions = new TcpKeepAlive();
			keepAliveOptions.OnOff = 1;
			keepAliveOptions.KeepAliveTime = 60000;
			keepAliveOptions.KeepAliveInterval = 60000;
			m_Socket.IOControl(IOControlCode.KeepAliveValues, keepAliveOptions.ToByteArray(), new byte[]{});

			// Use just the IP for naming starting with this function - will be updated with the platform later.
			try
			{
				m_sName = ((IPEndPoint)m_Socket.RemoteEndPoint).ToFilenameString();
			}
			catch (Exception)
			{
				m_sName = "Unknown Connection";
			}

			// Open log files.
			m_ClientLog = new Logger(Path.Combine(
				m_ConnectionManager.MoriartyManager.GetGameDirectoryPath(m_ePlatform, CoreLib.GameDirectory.Log),
				m_sName + "_Gamelog.txt"));
			m_ServerLog = new Logger(Path.Combine(
				m_ConnectionManager.MoriartyManager.GetGameDirectoryPath(m_ePlatform, CoreLib.GameDirectory.Log),
				m_sName + "_Moriarty.txt"));

			// Hook up RPC handlers.
			m_kaRPCHandlers = GetRPCHandlers();

			// Construct the send and receive threads.
			m_bWorkerRunning = true;
			m_ReadWorker = new Thread(ReadWorkerBody);
			m_ReadWorker.Name = "MoriartyConnection " + m_sName + " Read Worker";
			m_WriteWorker = new Thread(WriteWorkerBody);
			m_WriteWorker.Name = "MoriartyConnection " + m_sName + " Write Worker";

			// Start the network send/receive threads.
			m_ReadWorker.Start();
			m_WriteWorker.Start();

			// Hookup callbacks for content change events.
			m_ConnectionManager.MoriartyManager.ContentChangeManager.ContentChange += ContentChangeManager_ContentChange;

			// Send initial file stat cache population commands.
			SendCommand(new FileStatCacheRefreshCommand(
				CoreLib.GameDirectory.Config,
				m_ConnectionManager.MoriartyManager.GetGameDirectoryPath(m_ePlatform, CoreLib.GameDirectory.Config)));
			SendCommand(new FileStatCacheRefreshCommand(
				CoreLib.GameDirectory.Content,
				m_ConnectionManager.MoriartyManager.GetGameDirectoryPath(m_ePlatform, CoreLib.GameDirectory.Content)));
		}

		public void Dispose()
		{
			// Cleanup the network stream and socket if still active.
			if (m_bWorkerRunning)
			{
				// Remove callbacks for content change events.
				m_ConnectionManager.MoriartyManager.ContentChangeManager.ContentChange -= ContentChangeManager_ContentChange;

				// Stop running and wake up the threads.
				m_bWorkerRunning = false;
				m_WriteEvent.Set();

				// Close the socket - this is necessary, in case the read or write
				// threads are blocked on a write/read and haven't received a close
				// exception.
				m_Socket.Close();

				// Wait for the writer to finish.
				while (m_WriteWorker.IsAlive) ;
				m_WriteWorker = null;

				// Wait for the reader to finish.
				while (m_ReadWorker.IsAlive) ;
				m_ReadWorker = null;

				// Cleanup the stream and socket.
				m_Stream.Dispose();
				m_Stream = null;
				m_Socket.Dispose();
				m_Socket = null;
			}

			// Dispose the logs and remote files handler.
			m_ServerLog.Dispose();
			m_ClientLog.Dispose();
			m_RemoteFiles.Dispose();
		}

		/// <returns>
		/// The ConnectionManager that owns this MoriartyConnection.
		/// </returns>
		public MoriartyConnectionManager ConnectionManager
		{
			get
			{
				return m_ConnectionManager;
			}
		}

		/// <returns>
		/// True if this MoriartyConnection is still connected to the client, false otherwise.
		/// </returns>
		public bool HasConnection
		{
			get
			{
				return m_bConnected;
			}
		}

		/// <returns>
		/// True if this MoriartyConnection is waiting to complete a handshake with the client.
		/// </returns>
		public bool HasPendingConnection
		{
			get
			{
				return m_bPendingConnection;
			}
		}

		/// <returns>
		/// The platform of the client associated with this MoriartyConnection - this
		/// value will be kUnknown until IsConnected returns true.
		/// </returns>
		public CoreLib.Platform Platform
		{
			get
			{
				return m_ePlatform;
			}
		}

		/// <summary>
		/// Enqueue a command to be sent to the client connected to this MoriartyConnection.
		/// </summary>
		public void SendCommand(Commands.BaseMoriartyCommand command)
		{
			m_CommandQueue.Enqueue(command);
			m_WriteEvent.Set();
		}

		/// <summary>
		/// Log a message associated with a received RPC to the server's log file.
		/// </summary>
		public void LogMessage(ERPC eRPC, UInt32 uToken, string sMessage)
		{
			m_ServerLog.LogMessage(eRPC.ToString() + "(" + uToken.ToString() + "): " + sMessage);
		}

		/// <returns>
		/// The log file used for log messages received from the client to this MoriartyConnection.
		/// </returns>
		public Logger ClientLog
		{
			get
			{
				return m_ClientLog;
			}
		}

		/// <returns>
		/// The log file used for log message generated by this MoriartyConnection.
		/// </returns>
		public Logger ServerLog
		{
			get
			{
				return m_ServerLog;
			}
		}

		/// <returns>
		/// A human readable name that represents this MoriartyConnection - can be used
		/// in the UI, for debugging, etc.
		/// </returns>
		public override string ToString()
		{
			return m_sName;
		}
	}
} // namespace Moriarty
