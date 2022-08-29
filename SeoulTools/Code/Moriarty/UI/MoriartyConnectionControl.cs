// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Threading;
using System.Windows.Forms;

using Moriarty.Commands;
using Moriarty.Utilities;

namespace Moriarty.UI
{
	public partial class MoriartyConnectionControl : UserControl
	{
		#region Private members
		MoriartyConnection m_Connection = null;
		MoriartyManager m_Manager = null;
		System.Windows.Forms.Timer m_Timer = null;
		int m_iClientLogTextBoxRefreshCount = 0;
		int m_iManagerLogTextBoxRefreshCount = 0;
		int m_iServerLogTextBoxRefreshCount = 0;

		/// <summary>
		/// Using m_i*RefreshCount counters, check
		/// if a log is dirty and if so, refresh the associated
		/// log text box.
		/// </summary>
		void CheckAndRefreshLogTextBox(
			ref int iCheckCounter,
			TextBoxBase textBox,
			Logger log)
		{
			int iCheckCounterNow = iCheckCounter;
			if (iCheckCounterNow != 0)
			{
				RefreshLogTextBox(textBox, log);
				Interlocked.Add(ref iCheckCounter, -iCheckCounterNow);
			}
		}

		/// <summary>
		/// Handler, called when a hyerplink is clicked inside one of MoriartyConnectionControl's
		/// RichTextBox log text boxes.
		/// </summary>
		void m_LogTextBox_LinkClicked(object sender, System.Windows.Forms.LinkClickedEventArgs e)
		{
			try
			{
				// Use shell invoke to visit the hyperlink.
				Process process = Process.Start(e.LinkText);

				// If we successfully launched the link, dispose the process.
				if (null != process)
				{
					process.Dispose();
					process = null;
				}
			}
			catch (Exception)
			{
				// Ignore any exceptions.
			}
		}

		/// <summary>
		/// Event callback in response to MoriartyConnectionControl
		/// dispose.
		/// </summary>
		void MoriartyConnectionControl_Disposed(object sender, EventArgs e)
		{
			if (null != m_Timer)
			{
				m_Timer.Stop();
				m_Timer.Dispose();
				m_Timer = null;
			}
		}

		/// <summary>
		/// Used to bring a log text box (currently, the "Manager", "Server", or "Client"
		/// text boxes) in sync with a Logger instance.
		/// </summary>
		void RefreshLogTextBox(TextBoxBase textBox, Logger log)
		{
			// Must always run on the UI thread.
			BeginInvoke((Action)delegate()
			{
				// The Logger can be null, gracefully handle this case.
				if (log != null)
				{
					// Get an array of lines from the LogBuffer.
					string[] aLines = log.LogBuffer;

					// If there are lines in the buffer, set the current line to the last line.
					int iCurrentLine = (aLines.Length > 0
						? aLines.Length - 1
						: 0);

					// Update the text box.
					textBox.SuspendLayout();
					textBox.Lines = aLines;
					textBox.SelectionLength = 0;
					textBox.SelectionStart = (textBox.TextLength > 0 ? textBox.TextLength - 1 : 0);
					textBox.ScrollToCaret();
					textBox.ResumeLayout();
				}
				// If no log, just set the text box to empty.
				else
				{
					textBox.Lines = new string[0];
				}
			});
		}

		void RefreshClientLogTextBox()
		{
			Interlocked.Increment(ref m_iClientLogTextBoxRefreshCount);
		}

		void RefreshManagerLogTextBox()
		{
			Interlocked.Increment(ref m_iManagerLogTextBoxRefreshCount);
		}

		void RefreshServerLogTextBox()
		{
			Interlocked.Increment(ref m_iServerLogTextBoxRefreshCount);
		}

		void m_Timer_Tick(object sender, EventArgs e)
		{
			CheckAndRefreshLogTextBox(ref m_iClientLogTextBoxRefreshCount, m_ClientLogTextBox, (null != m_Connection ? m_Connection.ClientLog : null));
			CheckAndRefreshLogTextBox(ref m_iManagerLogTextBoxRefreshCount, m_ManagerLogTextBox, (null != m_Manager ? m_Manager.Log : null));
			CheckAndRefreshLogTextBox(ref m_iServerLogTextBoxRefreshCount, m_ServerLogTextBox, (null != m_Connection ? m_Connection.ServerLog : null));
		}
		#endregion

		public MoriartyConnectionControl()
		{
			InitializeComponent();

			// Setup the log refresh timer.
			m_Timer = new System.Windows.Forms.Timer()
			{
				Enabled = true,
				Interval = 500,
			};
			Disposed += new EventHandler(MoriartyConnectionControl_Disposed);
			m_Timer.Tick += new EventHandler(m_Timer_Tick);
			m_Timer.Start();
		}

		/// <returns>
		/// The MoriartyConnection associated with this MoriartyConnectionControl, or
		/// update it to a new value.
		/// </returns>
		public MoriartyConnection Connection
		{
			get
			{
				return m_Connection;
			}

			set
			{
				// If updating the connection, perform maintenance to keep our text
				// boxes in sync.
				if (value != m_Connection)
				{
					// If the old connection is valid, remove ourselves
					// from the old log events.
					if (m_Connection != null)
					{
						m_Connection.ServerLog.LogBufferChanged -= RefreshServerLogTextBox;
						m_Connection.ClientLog.LogBufferChanged -= RefreshClientLogTextBox;
					}

					m_Connection = value;

					// If we have a new connection, update the connection label and
					// register for the new logs events.
					if (m_Connection != null)
					{
						m_Connection.ClientLog.LogBufferChanged += RefreshClientLogTextBox;
						m_Connection.ServerLog.LogBufferChanged += RefreshServerLogTextBox;

						m_ConnectionName.Text = m_Connection.ToString();
					}
					// Otherwise, just set the connection label to "No Selection".
					else
					{
						m_ConnectionName.Text = "No Selection";
					}

					// Refresh the per-connection log text boxes.
					RefreshClientLogTextBox();
					RefreshServerLogTextBox();
				}
			}
		}

		/// <returns>
		/// The MoriartyManager associated with this MoriartyConnectionControl, or
		/// update it to a new value.
		/// </returns>
		public MoriartyManager Manager
		{
			get
			{
				return m_Manager;
			}

			set
			{
				// If updating the manager, perform maintenance to keep our text
				// boxes in sync.
				if (value != m_Manager)
				{
					// If the old manager was valid, removed our registered event handler.
					if (m_Manager != null)
					{
						m_Manager.Log.LogBufferChanged -= RefreshManagerLogTextBox;
					}

					m_Manager = value;

					// Register for events if the new manager is valid.
					if (m_Manager != null)
					{
						m_Manager.Log.LogBufferChanged += RefreshManagerLogTextBox;
					}

					// Bring the manager text box in sync with the Manager Logger.
					RefreshManagerLogTextBox();
				}
			}
		}

		/// <summary>
		/// Set the "Manager" tab to the selected tab.
		/// </summary>
		public void SelectManagerTab()
		{
			m_MoriartyConnectionTabControl.SelectedTab = m_ManagerTabPage;
		}
	}
} // namespace Moriarty.UI
