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
using System.Linq;
using System.Text;
using System.Windows.Forms;

using Moriarty.Commands;

namespace Moriarty.UI
{
	public partial class KeyboardCaptureForm : Form
	{
		#region Constants
		public const UInt32 VK_SHIFT = 0x10;
		public const UInt32 VK_CONTROL = 0x11;
		public const UInt32 VK_MENU = 0x12;
		public const UInt32 VK_LSHIFT = 0xA0;
		public const UInt32 VK_LCONTROL = 0xA2;
		public const UInt32 VK_LMENU = 0xA4;
		public const int WM_CHAR = 0x0102;
		public const int WM_KEYDOWN = 0x0100;
		public const int WM_KEYUP = 0x0101;
		public const int WM_SYSCHAR = 0x0106;
		public const int WM_SYSKEYDOWN = 0x0104;
		public const int WM_SYSKEYUP = 0x0105;
		#endregion

		#region Private members
		MoriartyConnection m_Connection = null;

		/// <summary>
		/// Sent in any situation where the keyboard should be considered lost
		/// by the MoriartyClient.
		/// </summary>
		void SendKeyboardFocusLost()
		{
			// Send a "all keys up" event to the active connection.
			MoriartyConnection connection = m_Connection;
			if (null != connection)
			{
				connection.SendCommand(new KeyboardKeyEventCommand(0u, KeyEventType.AllReleased));
			}
		}

		void KeyboardCaptureForm_FormClosed(object sender, FormClosedEventArgs e)
		{
			SendKeyboardFocusLost();
		}

		void KeyboardCaptureForm_LostFocus(object sender, EventArgs e)
		{
			SendKeyboardFocusLost();
		}

		protected override void WndProc(ref Message m)
		{
			// If we don't have a connection, or we don't have input focus, don't capture input.
			MoriartyConnection connection = m_Connection;
			if (null == connection || !Focused)
			{
				base.WndProc(ref m);
				return;
			}

			// Otherwise, check for keyboard events and potentially capture input.
			long lParam = m.LParam.ToInt64();
			UInt32 wParam = unchecked((UInt32)m.WParam.ToInt64());

			// Handler for "capture keyboard mode", char events.
			switch (m.Msg)
			{
			case WM_CHAR: // Fall-through
			case WM_SYSCHAR:
				connection.SendCommand(new KeyboardCharEventCommand(wParam));
				return;
			};

			KeyEventType eKeyEvent = KeyEventType.AllReleased;

			// Handler for "capture keyboard mode", key events.
			switch (m.Msg)
			{
				case WM_SYSKEYDOWN:
					if (!(0 == (lParam & 0x20000000)))
					{
						eKeyEvent = KeyEventType.Pressed;
					}
					break;
				case WM_KEYDOWN:
					eKeyEvent = KeyEventType.Pressed;
					break;
				case WM_SYSKEYUP:
					eKeyEvent = KeyEventType.Released;
					break;
				case WM_KEYUP:
					eKeyEvent = KeyEventType.Released;
					break;
				default:
					break;
			};

			// If we have a pressed or released event, send the command.
			if (KeyEventType.AllReleased != eKeyEvent)
			{
				connection.SendCommand(new KeyboardKeyEventCommand(wParam, eKeyEvent));

				// If key is a special key, also send an event specifying the left or right version of the
				// key, in addition to the key code that doesn't include a left or right specifier.
				switch (wParam)
				{
					case VK_SHIFT:
						connection.SendCommand(new KeyboardKeyEventCommand(
							(UInt32)(VK_LSHIFT + ((lParam & 0x1000000) >> 24)),
							eKeyEvent));
						break;
					case VK_CONTROL:
						connection.SendCommand(new KeyboardKeyEventCommand(
							(UInt32)(VK_LCONTROL + ((lParam & 0x1000000) >> 24)),
							eKeyEvent));
						break;
					case VK_MENU:
						connection.SendCommand(new KeyboardKeyEventCommand(
							(UInt32)(VK_LMENU + ((lParam & 0x1000000) >> 24)),
							eKeyEvent));
						break;
				}
			}
			else
			{
				base.WndProc(ref m);
			}
		}
		#endregion

		public KeyboardCaptureForm()
		{
			InitializeComponent();

			FormClosed += new FormClosedEventHandler(KeyboardCaptureForm_FormClosed);
			LostFocus += new EventHandler(KeyboardCaptureForm_LostFocus);
		}

		/// <summary>
		/// Retrieve or update the MoriartyConnection that is associated with the KeyboardCaptureForm.
		/// </summary>
		public MoriartyConnection Connection
		{
			get
			{
				return m_Connection;
			}

			set
			{
				// Release the existing connection, if we have one.
				SendKeyboardFocusLost();

				m_Connection = value;
			}
		}
	}
}
