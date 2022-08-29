// Root windows form of the Moriarty application.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Text;
using System.Threading;
using System.Windows.Forms;

using Moriarty.Commands;
using Moriarty.Settings;
using Moriarty.Utilities;

namespace Moriarty.UI
{
	public partial class MoriartyMainForm : Form
	{
		#region Private members
		KeyboardCaptureForm m_KeyboardCaptureForm = null;
		MoriartyManager m_Manager = null;
		int m_nConnectionCount = 0;
		Thread m_LaunchTask = null;

		void UpdateConnectionLabel(int nConnectionCount)
		{
			// Update the connection label - either none, 1, or many.
			m_ConnectionCountLabel.Text = (0 == nConnectionCount
				? "No Connections"
				: (1 == nConnectionCount ? "1 Connection" : nConnectionCount.ToString() + " Connections"));
		}

		/// <summary>
		/// Called by the MoriartyConnectionManager when a connection disconnects -
		/// MoriartyMainForm uses this to update UI state in response to the dropped connection.
		/// </summary>
		void ConnectionManager_OnDisconnect(MoriartyConnection connection)
		{
			// Must run on the UI thread - we don't care about when it finishes, so
			// just call BeginInvoke(), which runs it asynchronously.
			BeginInvoke((Action)delegate()
			{
				// Increment and cache the connection count, then update the label.
				int nConnectionCount = Interlocked.Decrement(ref m_nConnectionCount);
				UpdateConnectionLabel(nConnectionCount);

				// If the dropped connection is the currently selected connection, update
				// the connection control and keyboard capture form state to reflect this.
				if (connection == m_MoriartyConnectionControl.Connection)
				{
					m_MoriartyConnectionControl.Connection = null;

					if (null != m_KeyboardCaptureForm)
					{
						m_KeyboardCaptureForm.Connection = null;
					}
				}

				// Find this connection in the Connection ListView.
				int iIndex = -1;
				for (int i = 0; i < m_ConnectionList.Items.Count; ++i)
				{
					if (m_ConnectionList.Items[i].Tag == connection)
					{
						iIndex = i;
						break;
					}
				}

				// If found, remove it from the ListView.
				if (iIndex >= 0)
				{
					m_ConnectionList.Items.RemoveAt(iIndex);
				}
			});
		}

		/// <summary>
		/// Called by the MoriartyConnectionManager when a connection is established -
		/// MoriartyMainForm uses this to update UI state in response to the established connection.
		/// </summary>
		void ConnectionManager_OnConnect(MoriartyConnection connection)
		{
			// Must run on the UI thread - we don't care about when it finishes, so
			// just call BeginInvoke(), which runs it asynchronously.
			BeginInvoke((Action)delegate()
			{
				// Create a new entry for the Connection ListView and add it to the list.
				ListViewItem item = new ListViewItem();
				item.ImageIndex = (int)connection.Platform;
				item.Tag = connection;
				item.Text = connection.ToString();
				m_ConnectionList.Items.Add(item);

				// Select the newest connection.
				item.Selected = true;

				// Add the connection to the count and then update the connection label.
				int nConnectionCount = Interlocked.Increment(ref m_nConnectionCount);
				UpdateConnectionLabel(nConnectionCount);
			});
		}

		/// <summary>
		/// Call at startup, or when the project's list of run targets has changed,
		/// to update the state of the ComboBox used to display the list of run targets.
		/// </summary>
		void RefreshTargetsComboBox()
		{
			// Add launch targets from the user settings to the drop down box.
			m_TargetSelectionComboBox.Items.Clear();
			for (int i = 0; i < m_Manager.UserSettings.Settings.LaunchTargets.Count; ++i)
			{
				m_TargetSelectionComboBox.Items.Add(m_Manager.UserSettings.Settings.LaunchTargets[i]);
			}

			// If we have an active target, select it, otherwise use the first in the list.
			if (m_TargetSelectionComboBox.Items.Count > 0)
			{
				string sActiveTarget = m_Manager.UserSettings.Settings.ActiveTarget;
				int iToSelect = 0;
				for (int i = 0; i < m_TargetSelectionComboBox.Items.Count; ++i)
				{
					LaunchTargetSettings settings = (m_TargetSelectionComboBox.Items[i] as LaunchTargetSettings);
					if (null != settings && sActiveTarget == settings.Name)
					{
						iToSelect = i;
						break;
					}
				}
				m_TargetSelectionComboBox.SelectedIndex = iToSelect;
			}
		}

		#region Control handlers
		/// <summary>
		/// Called when this MoriartyMainForm is closed.
		/// </summary>
		void MoriartyMainForm_Closed(object sender, FormClosedEventArgs e)
		{
			// If a previous task is finished, clear it out.
			if (null != m_LaunchTask)
			{
				if (!m_LaunchTask.IsAlive)
				{
					m_LaunchTask = null;
				}
			}

			if (null != m_LaunchTask)
			{
				m_LaunchTask.Abort();
				m_LaunchTask.Join();
				m_LaunchTask = null;
			}

			m_Manager.ConnectionManager.OnDisconnect -= ConnectionManager_OnDisconnect;
			m_Manager.ConnectionManager.OnConnect -= ConnectionManager_OnConnect;

			m_Manager.Dispose();
			m_Manager = null;
		}

		void MoriartyMainForm_Closing(object sender, FormClosingEventArgs e)
		{
			// If a previous task is finished, clear it out.
			if (null != m_LaunchTask)
			{
				if (!m_LaunchTask.IsAlive)
				{
					m_LaunchTask = null;
				}
			}

			if (null != m_LaunchTask)
			{
				m_Manager.Log.LogMessage("Launch is in progress, please wait for the launch operation to complete before closing Moriarty.");
				e.Cancel = true;
			}
		}

		/// <summary>
		/// Called when the "About" menu strip item is clicked.
		/// </summary>
		void m_AboutToolStripMenuItem_Click(object sender, EventArgs e)
		{
			MessageBox.Show(
				"Moriarty Copyright (c) 2008-2022 Demiurge Studios Inc. All rights reserved.",
				"About Moriarty",
				MessageBoxButtons.OK,
				MessageBoxIcon.Information);
		}
		
		/// <summary>
		/// Called when the selected connection in the Connection ListView has changed.
		/// </summary>
		void m_ConnectionList_ItemSelectionChanged(object sender, ListViewItemSelectionChangedEventArgs e)
		{
			// If this is a selection event, change the connection in the MoriartyConnectionControl
			// to the newly selected connection.
			if (e.IsSelected)
			{
				m_MoriartyConnectionControl.Connection = (MoriartyConnection)e.Item.Tag;
			}
			// Otherwise, clear the selected connection.
			else
			{
				m_MoriartyConnectionControl.Connection = null;
			}

			// Keep the keyboard capture form in sync.
			if (null != m_KeyboardCaptureForm)
			{
				m_KeyboardCaptureForm.Connection = m_MoriartyConnectionControl.Connection;
			}
		}

		/// <summary>
		/// Called when the "Exit" menu strip item is clicked.
		/// </summary>
		void m_ExitToolStripMenuItem_Click(object sender, EventArgs e)
		{
			Close();
		}

		/// <summary>
		/// Called when the "Show Keyboard Capture Form" menu strip item is clicked.
		/// </summary>
		void m_KeyboardCaptureWindowToolStripMenuItem_Click(object sender, EventArgs e)
		{
			// If we don't already have a form or if it's been disposed, recreate it.
			if (null == m_KeyboardCaptureForm || m_KeyboardCaptureForm.IsDisposed)
			{
				// If we're recreating the capture form because it was disposed,
				// remove it as an owned form first.
				if (null != m_KeyboardCaptureForm)
				{
					RemoveOwnedForm(m_KeyboardCaptureForm);
					m_KeyboardCaptureForm = null;
				}

				// Create a new KeyboardCaptureForm, with this MoriartyMainForm as
				// its owner.
				m_KeyboardCaptureForm = new KeyboardCaptureForm();
				m_KeyboardCaptureForm.Connection = m_MoriartyConnectionControl.Connection;
				AddOwnedForm(m_KeyboardCaptureForm);

				// Center the capture form to the MoriartyMainForm.
				Point keyboardCaptureFormDesktopLocation = new Point();
				keyboardCaptureFormDesktopLocation.X = DesktopLocation.X +
					(int)(DesktopBounds.Width * 0.5f - m_KeyboardCaptureForm.DesktopBounds.Width * 0.5f);
				keyboardCaptureFormDesktopLocation.Y = DesktopLocation.Y +
					(int)(DesktopBounds.Height * 0.5f - m_KeyboardCaptureForm.DesktopBounds.Height * 0.5f);

				m_KeyboardCaptureForm.SetDesktopLocation(
					keyboardCaptureFormDesktopLocation.X,
					keyboardCaptureFormDesktopLocation.Y);
			}

			// In all cases, make sure the form is visible and make it the focus item.
			m_KeyboardCaptureForm.Show();
			m_KeyboardCaptureForm.Focus();
		}

		/// <summary>
		/// Registered callback - invoked when a MoriartyCollectionEditor form is closed.
		/// We use this to respond to changes to the launchable targets list.
		/// </summary>
		void MoriartyCollectionEditor_CollectionModified(object collection)
		{
			if (m_Manager.UserSettings.Settings.LaunchTargets == collection)
			{
				RefreshTargetsComboBox();
			}
		}

		/// <summary>
		/// Called when this MoriartyMainForm is first shown - this is a workaround
		/// because this Form's handle is not valid in its constructor, and MoriartyConnectionControl.Manager
		/// will call BeginInvoke().
		/// </summary>
		void MoriartyMainForm_Shown(object sender, EventArgs e)
		{
			m_MoriartyConnectionControl.Manager = m_Manager;
		}

		/// <summary>
		/// Called when the "Run Target" button is clicked.
		/// </summary>
		void m_RunTargetButton_Click(object sender, EventArgs e)
		{
			// Get the target launch settings.
			LaunchTargetSettings settings = (m_TargetSelectionComboBox.SelectedItem as LaunchTargetSettings);

			// Set the manager log box as the active tab, since any results of clicking
			// "Run" will be displayed there.
			m_MoriartyConnectionControl.SelectManagerTab();

			// If either value is null for some reason, log the error and don't attempt to launch.
			if (null == settings)
			{
				m_Manager.Log.LogMessage("Failed running target, invalid or unspecified target settings.");
				return;
			}

			// If a previous task is finished, clear it out.
			if (null != m_LaunchTask)
			{
				if (!m_LaunchTask.IsAlive)
				{
					m_LaunchTask = null;
				}
			}

			// If a task is alread running, fail.
			if (null != m_LaunchTask)
			{
				m_Manager.Log.LogMessage("Failed running target, a launch task is already active.");
				return;
			}

			// Launch the target.
			m_LaunchTask = new Thread(new ThreadStart(delegate()
				{
					try
					{
						m_Manager.LaunchGame(settings);
					}
					catch (Exception exception)
					{
						m_Manager.Log.LogMessage("Failed launching target \"" + settings.Name + "\", error: " + exception.Message);
					}
				}));
			m_LaunchTask.Start();
		}

		/// <summary>
		/// Called when the "Save User Properties" menu strip item is clicked.
		/// </summary>
		void m_SaveUserSettingsToolStripMenuItem_Click(object sender, EventArgs e)
		{
			m_Manager.UserSettings.Save();
		}

		/// <summary>
		/// Called when the "Launch Target" combo box value has changed.
		/// </summary>
		void m_TargetSelectionComboBox_SelectedIndexChanged(object sender, EventArgs e)
		{
			// Get the launch settings from the selected item - if we have a non-null value,
			// update the user setting's active target name.
			LaunchTargetSettings settings = (m_TargetSelectionComboBox.SelectedItem as LaunchTargetSettings);
			bool bCanLaunch = false;
			if (null != settings)
			{
				m_Manager.UserSettings.Settings.ActiveTarget = settings.Name;
				bCanLaunch = m_Manager.CanLaunch(settings.Platform);
			}

			// If the target is a valid selection, and the platform of the target settings
			// supports launching, enable the run button.
			m_RunTargetButton.Enabled = (
				bCanLaunch &&
				m_TargetSelectionComboBox.SelectedIndex >= 0);
		}
		#endregion
		#endregion

		public MoriartyMainForm()
		{
			InitializeComponent();

			// Setup control handlers.
			Shown += new EventHandler(MoriartyMainForm_Shown);
			FormClosed += MoriartyMainForm_Closed;
			FormClosing += MoriartyMainForm_Closing;
			m_ConnectionList.ItemSelectionChanged += new ListViewItemSelectionChangedEventHandler(m_ConnectionList_ItemSelectionChanged);
			m_TargetSelectionComboBox.SelectedIndexChanged += new EventHandler(m_TargetSelectionComboBox_SelectedIndexChanged);
			MoriartyCollectionEditor.CollectionModified += new MoriartyCollectionEditor.CollectionModifiedDelegate(MoriartyCollectionEditor_CollectionModified);

			// TODO: Determine the path in a less hardcoded way.
			m_Manager = new MoriartyManager(FileUtility.ResolveAndSimplifyPath(Path.Combine(
				Path.GetDirectoryName(Application.ExecutablePath),
				@"..\..\..\..\..\Moriarty.xml")));

			// Register handlers for when connections are disconnected and established.
			m_Manager.ConnectionManager.OnConnect += ConnectionManager_OnConnect;
			m_Manager.ConnectionManager.OnDisconnect += ConnectionManager_OnDisconnect;

			// If we have a project name, append it to the title bar.
			if (!string.IsNullOrEmpty(m_Manager.ProjectSettings.Settings.ProjectName))
			{
				Text = Text + " - " + m_Manager.ProjectSettings.Settings.ProjectName;
			}

			// Setup the initial target combo box states.
			RefreshTargetsComboBox();

			// Setup the user settings property grid to the loaded user settings object.
			m_UserSettings.SelectedObject = m_Manager.UserSettings.Settings;
		}

		private void m_ConnectionList_SelectedIndexChanged(object sender, EventArgs e)
		{

		}
	}
} // namespace Moriarty.UI
