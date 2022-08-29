// Global manager of all connections - owns the listener socket that
// spawns new connections, and then owns the connections that are created
// from the listener.
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
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;

using Moriarty.Commands;

namespace Moriarty
{
	/// <summary>
	/// MoriartyConnectionManager listens for incoming connections and spawns
	/// new MoriartyConnection instances as those connections are received.
	/// </summary>
	public sealed class MoriartyConnectionManager : IDisposable
	{
		#region Private members
		MoriartyManager m_Manager = null;
		volatile Thread m_Worker = null;
		volatile bool m_bWorkerRunning = false;
		AutoResetEvent m_ExitEvent = new AutoResetEvent(false /*Unsignaled*/);
		int m_iListenerPort = 0;
		ConcurrentQueue<MoriartyConnection> m_Connections = new ConcurrentQueue<MoriartyConnection>();

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
				MoriartyConnection connection = null;
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
		/// spawns new MoriartyConnection instances as those connections are established.
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
					m_Manager.Log.LogMessage("Listener port " + m_iListenerPort.ToString() + " is already in use, " +
						"server is disabled until socket becomes free. Please close any other running Moriarty instances.");

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
					m_Manager.Log.LogMessage("Unknown error attempting to listen for connections, server is disabled! Error: " + e.Message);
					return;
				}

			}
			catch (Exception e)
			{
				// Any other exception, log and bail from listening.
				m_Manager.Log.LogMessage("Unknown error attempting to listen for connections, server is disabled! Error: " + e.Message);
				return;
			}

			// Tell the environment we succeeded.
			m_Manager.Log.LogMessage("Listening on port " + m_iListenerPort.ToString());

			// While we're still running, keep waiting for pending connections.
			while (m_bWorkerRunning)
			{
				// Wait for a new connection or until we're requested to exit
				IAsyncResult asyncAccept = tcpListener.BeginAcceptSocket(null, null);
				WaitHandle[] waitHandles = new WaitHandle[]{m_ExitEvent, asyncAccept.AsyncWaitHandle};
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
					m_Connections.Enqueue(new MoriartyConnection(this, tcpListener.EndAcceptSocket(asyncAccept)));
				}
				catch (Exception e)
				{
					m_Manager.Log.LogMessage("Listener failed accepting conectin: " + e.Message);

					// TODO: Determien if there is an exception that can occur which
					// places the listener in a bad state.
				}
			}

			tcpListener.Stop();
		}

		/// <summary>
		/// Called by a MoriartyConnection when it has completed handshaking
		/// and is conencted to the client.
		/// </summary>
		internal void HandleConnect(MoriartyConnection connection)
		{
			if (null != OnConnect)
			{
				OnConnect(connection);
			}
		}

		/// <summary>
		/// Called by a MoriartyConnection when it has disconnected from
		/// the client - this will not be called if the initial handshake
		/// did not complete before the disconnect occured.
		/// </summary>
		internal void HandleDisconnect(MoriartyConnection connection)
		{
			if (null != OnDisconnect)
			{
				OnDisconnect(connection);
			}
		}
		#endregion

		public MoriartyConnectionManager(
			MoriartyManager manager,
			int iListenerPort)
		{
			m_Manager = manager;
			m_iListenerPort = iListenerPort;
			m_Worker = new Thread(WorkerBody);
			m_Worker.Name = "MoriartyConnectionManager Worker Thread";
			m_bWorkerRunning = true;
			m_Worker.Start();
		}

		/// <summary>
		/// Cleanup resources used by this MoriartyConnectionManager - stop the
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

			MoriartyConnection connection = null;
			while (m_Connections.TryDequeue(out connection))
			{
				connection.Dispose();
			}
		}

		/// <returns>
		/// The MoriartyManager that owns this MoriartyConnectionManager.
		/// </returns>
		public MoriartyManager MoriartyManager
		{
			get
			{
				return m_Manager;
			}
		}

		public delegate void ConnectDelegate(MoriartyConnection connection);
		public delegate void DisconnectDelegate(MoriartyConnection connection);

		public event ConnectDelegate OnConnect;
		public event DisconnectDelegate OnDisconnect;
	}
} // namespace Moriarty
