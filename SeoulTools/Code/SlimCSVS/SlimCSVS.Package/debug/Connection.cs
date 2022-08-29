// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.Collections.Concurrent;
using System.Diagnostics;
using System.IO;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using System.Threading;

namespace SlimCSVS.Package.debug
{
	public sealed class Connection : IDisposable
	{
		#region Data validation constants - must be kept in sync with equivalent values in the client implementation.
		/// <summary>
		/// Protocol version -- increment this every time you make a
		/// non-backwards-compatible change
		/// </summary>
		public const UInt32 kProtocolVersion = 3;

		/// <summary>
		/// Magic constant sent after initiating a connection to avoid other applications
		/// accidentally connecting to a debug server and trying to use some other
		/// protocol.
		/// </summary>
		public const UInt32 kConnectMagic = 0x75e7498f;
		#endregion

		#region Private members
		ConcurrentQueue<Message> m_ReceiveQueue = new ConcurrentQueue<Message>();
		ConcurrentQueue<Message> m_SendQueue = new ConcurrentQueue<Message>();
		ConnectionManager m_ConnectionManager = null;
		volatile bool m_bPendingConnection = true;
		volatile bool m_bConnected = false;
		volatile Thread m_ReadWorker = null;
		volatile Thread m_WriteWorker = null;
		AutoResetEvent m_WriteEvent = new AutoResetEvent(false);
		volatile bool m_bWorkerRunning = false;
		Socket m_Socket = null;
		NetworkStream m_Stream = null;

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
								// Process all commands in the queue.
								Message msg = null;
								while (m_bWorkerRunning && m_SendQueue.TryDequeue(out msg))
								{
									msg.Send(writer);
									msg = null;
								}

								// Flush the buffered stream now that we've queued up all
								// our messages.
								writer.Flush();
								bufferedStream.Flush();
								m_Stream.Flush();

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
					Debug.WriteLine("Write Loop: Unexpected exception from Connection \"" +
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
					{
						var msg = Message.Create(reader);
						if (ClientTag.Version != (ClientTag)msg.MessageTag ||
							kProtocolVersion != msg.ReadUInt32() ||
							kConnectMagic != msg.ReadUInt32())
						{
							Debug.WriteLine("Invalid connection.");
							m_bPendingConnection = false;
							return;
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
						// Get the next Message.
						var msg = Message.Create(reader);
						if (null == msg)
						{
							throw new IOException("msg read failed");
						}

						// Place the message in the receive queue.
						m_ReceiveQueue.Enqueue(msg);

						// Signal the connection manager.
						m_ConnectionManager.HandleReceive();
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
					Debug.WriteLine("Read Loop: Unexpected exception from Connection \"" +
						ToString() + "\": " + e.Message + "\n" + e.StackTrace);
				}
			}

			// Tell the connection manager that we're no longer connected.
			m_ConnectionManager.HandleDisconnect(this);
			m_bConnected = false;
		}
		#endregion

		/// <summary>
		/// Structure for setting TCP keep-alive parameters (from MSTcpIP.h)
		/// </summary>
		///
		/// <example>
		/// struct tcp_keepalive {
		///   ULONG onoff;
		///   ULONG keepalivetime;
		///   ULONG keepaliveinterval;
		/// };
		/// </example>
		[StructLayout(LayoutKind.Explicit)]
		struct TcpKeepAlive
		{
			/// <summary>
			/// 0 to disable keep-alive, non-0 to enable keep-alive
			/// </summary>
			[FieldOffset(0)]
			public uint OnOff;

			/// <summary>
			/// Idle time after which keep-alive probes will be sent, in ms
			/// </summary>
			[FieldOffset(4)]
			public uint KeepAliveTime;

			/// <summary>
			/// Interval between keep-alive probes, in ms
			/// </summary>
			[FieldOffset(8)]
			public uint KeepAliveInterval;

			/// <summary>
			/// Converts this structure to a byte array
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

		public Connection(
			ConnectionManager connectionManager,
			Socket socket)
		{
			m_ConnectionManager = connectionManager;
			m_Socket = socket;
			m_Stream = new NetworkStream(m_Socket, FileAccess.ReadWrite);

			// Disable the Nagle algorithm on the socket.
			m_Socket.SetSocketOption(SocketOptionLevel.Tcp, SocketOptionName.NoDelay, true);

			// Enable TCP keep-alive and set keep-alive interval to 1 minute
			TcpKeepAlive keepAliveOptions = new TcpKeepAlive();
			keepAliveOptions.OnOff = 1;
			keepAliveOptions.KeepAliveTime = 60000;
			keepAliveOptions.KeepAliveInterval = 60000;
			m_Socket.IOControl(IOControlCode.KeepAliveValues, keepAliveOptions.ToByteArray(), new byte[] { });

			// Construct the send and receive threads.
			m_bWorkerRunning = true;
			m_ReadWorker = new Thread(ReadWorkerBody);
			m_ReadWorker.Name = "Connection Read Worker";
			m_WriteWorker = new Thread(WriteWorkerBody);
			m_WriteWorker.Name = "Connection Write Worker";

			// Start the network send/receive threads.
			m_ReadWorker.Start();
			m_WriteWorker.Start();
		}

		public void Dispose()
		{
			// Cleanup the network stream and socket if still active.
			if (m_bWorkerRunning)
			{
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
		}

		/// <summary>
		/// The ConnectionManager that owns this Connection.
		/// </summary>
		public ConnectionManager ConnectionManager
		{
			get
			{
				return m_ConnectionManager;
			}
		}

		/// <summary>
		/// Return true if this Connection is still connected to the client, false otherwise.
		/// </summary>
		public bool HasConnection
		{
			get
			{
				return m_bConnected;
			}
		}

		/// <summary>
		/// Return true if this Connection is waiting to complete a handshake with the client.
		/// </summary>
		public bool HasPendingConnection
		{
			get
			{
				return m_bPendingConnection;
			}
		}

		/// <summary>
		/// Attempt to receive a single Message. Return null if no message is pending.
		/// </summary>
		public Message Receive()
		{
			Message ret = null;
			if (m_ReceiveQueue.TryDequeue(out ret))
			{
				return ret;
			}
			else
			{
				return null;
			}
		}

		/// <summary>
		/// Enqueue a command to be sent to the client connected to this Connection.
		/// </summary>
		public void Send(Message msg)
		{
			m_SendQueue.Enqueue(msg);
			m_WriteEvent.Set();
		}
	}
}
