// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.Collections.Concurrent;
using System.Diagnostics;
using System.Net;
using System.Net.Sockets;
using System.Threading;

namespace SlimCSVS.Package.debug
{
	/// <summary>
	/// ConnectionManager listens for incoming connections and spawns
	/// new Connection instances as those connections are received.
	/// </summary>
	public sealed class ConnectionManager : IDisposable
	{
		#region Private members
		readonly ad7.Engine m_Engine;
		volatile Thread m_Worker = null;
		volatile bool m_bWorkerRunning = false;
		AutoResetEvent m_ExitEvent = new AutoResetEvent(false /*Unsignaled*/);
		int m_iListenerPort = 0;
		ConcurrentQueue<Connection> m_Connections = new ConcurrentQueue<Connection>();
		AutoResetEvent m_ReceiveEvent = new AutoResetEvent(false /*Unsignaled*/);

		/// <summary>
		/// Call periodically, if a connection has been dropped, this method will Dispose()
		/// it and removed it from the set of all connections.
		/// </summary>
		void CleanupClosedConnections()
		{
			// Walk the current number of connections - this method
			// uses a concurrent queue so that new connections
			// can be created while we're removing any existing
			// ones that have been disconnected.
			int nCount = m_Connections.Count;
			int i = 0;
			while (i++ < nCount)
			{
				Connection connection = null;
				if (m_Connections.TryDequeue(out connection))
				{
					// If the connection has dropped and is not pending a connect,
					// dispose it.
					if (!connection.HasConnection && !connection.HasPendingConnection)
					{
						connection.Dispose();
					}
					// Otherwise, put it back in the queue.
					else
					{
						m_Connections.Enqueue(connection);
					}
				}
			}
		}

		/// <summary>
		/// Body of the worker thread that listens for incoming connections and
		/// spawns new Connection instances as those connections are established.
		/// </summary>
		void WorkerBody(object obj)
		{
			// Instantiate a TcpListener on the specified port.
			TcpListener tcpListener = new TcpListener(IPAddress.Any, m_iListenerPort);

			// Label used if the listener has failed in an expected way and we want
			// to attempt to listen again after a delay.
			tryagain:
			try
			{
				// Attempt to start the listener - this can fail for several expected
				// and unexpected reasons.
				tcpListener.Start();
			}
			catch (SocketException e)
			{
				// If the port is blocked, warn, sleep, then try again.
				if (e.SocketErrorCode == SocketError.AddressAlreadyInUse)
				{
					// Wait 5 seconds - we don't sleep here so we can response to a m_bWorkerRunning change.
					Stopwatch timer = Stopwatch.StartNew();
					while (m_bWorkerRunning && timer.Elapsed.TotalSeconds < 5.0)
					{
						Thread.Yield();
					}
					timer.Stop();

					// Try the listen again if we're still running.
					if (m_bWorkerRunning)
					{
						goto tryagain;
					}
					// Otherwise, bail out
					else
					{
						return;
					}
				}
				// Otherwise, handle this error as a generic unknown error and
				// stop trying to listen.
				else
				{
					Debug.WriteLine("Unknown error attempting to listen for connections, server is disabled! Error: " + e.Message);
					return;
				}

			}
			catch (Exception e)
			{
				// Any other exception, log and bail from listening.
				Debug.WriteLine("Unknown error attempting to listen for connections, server is disabled! Error: " + e.Message);
				return;
			}

			// Tell the environment we succeeded.
			Debug.WriteLine("Listening on port " + m_iListenerPort.ToString());

			// While we're still running, keep waiting for pending connections.
			while (m_bWorkerRunning)
			{
				// Wait for a new connection or until we're requested to exit
				IAsyncResult asyncAccept = tcpListener.BeginAcceptSocket(null, null);
				WaitHandle[] waitHandles = new WaitHandle[] { m_ExitEvent, asyncAccept.AsyncWaitHandle };
				int nSignaledIndex;

				// If we don't have a pending connection, periodically cleanup closed connections.
				while ((nSignaledIndex = WaitHandle.WaitAny(waitHandles, 1000)) == WaitHandle.WaitTimeout)
				{
					CleanupClosedConnections();
				}

				// If we're not running anymore, break out of the loop.
				if (nSignaledIndex == 0)
				{
					break;
				}

				// Start the new connection.
				try
				{
					m_Connections.Enqueue(new Connection(this, tcpListener.EndAcceptSocket(asyncAccept)));
				}
				catch (Exception e)
				{
					Debug.WriteLine("Listener failed accepting conectin: " + e.Message);
				}
			}

			tcpListener.Stop();
		}

		/// <summary>
		/// Called by a Connection when it has completed handshaking
		/// and is conencted to the client.
		/// </summary>
		internal void HandleConnect(Connection connection)
		{
			if (null != OnConnect)
			{
				OnConnect(connection);
			}
		}

		/// <summary>
		/// Called by a Connection when it has disconnected from
		/// the client - this will not be called if the initial handshake
		/// did not complete before the disconnect occured.
		/// </summary>
		internal void HandleDisconnect(Connection connection)
		{
			if (null != OnDisconnect)
			{
				OnDisconnect(connection);
			}
		}

		/// <summary>
		/// Called by a Connection when it has received a message
		/// and enqueued it in its receive buffer.
		/// </summary>
		internal void HandleReceive()
		{
			m_ReceiveEvent.Set();
		}
		#endregion

		public ConnectionManager(ad7.Engine engine, int iListenerPort)
		{
			m_Engine = engine;
			m_iListenerPort = iListenerPort;
			m_Worker = new Thread(WorkerBody);
			m_Worker.Name = "ConnectionManager Worker Thread";
			m_bWorkerRunning = true;
			m_Worker.Start();
		}

		/// <summary>
		/// Cleanup resources used by this ConnectionManager - stop the
		/// worker thread and close/dispose any remaining connections.
		/// </summary>
		public void Dispose()
		{
			if (m_bWorkerRunning)
			{
				// Trigger the worker thread to exit and wait for it
				m_bWorkerRunning = false;
				m_ExitEvent.Set();
				m_Worker.Join();
				m_Worker = null;
			}

			Connection connection = null;
			while (m_Connections.TryDequeue(out connection))
			{
				connection.Dispose();
			}
		}

		public delegate void ConnectDelegate(Connection connection);
		public delegate void DisconnectDelegate(Connection connection);

		public event ConnectDelegate OnConnect;
		public event DisconnectDelegate OnDisconnect;

		/// <summary>
		/// Get the master Engine of this ConnectionManager.
		/// </summary>
		public ad7.Engine Engine { get { return m_Engine; } }

		public Message ReceiveOne()
		{
			while (true)
			{
				var aConnections = m_Connections.ToArray();
				foreach (var c in aConnections)
				{
					var msg = c.Receive();
					if (null != msg)
					{
						return msg;
					}
				}

				// Wait last - ReceiveOne() should
				// always check the connections array
				// before waiting on the receive signal.
				m_ReceiveEvent.WaitOne();
			}
		}

		/// <summary>
		/// Enqueue a command to be sent to all connected clients.
		/// </summary>
		public void SendToAll(Message msg)
		{
			var aConnections = m_Connections.ToArray();
			foreach (var c in aConnections)
			{
				c.Send(msg);
			}
		}
	}
}
