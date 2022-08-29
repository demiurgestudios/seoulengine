// Comand sent from the server to a client when full keyboard
// capture is enabled and a key is pressed, released, or the entire
// keyboard focus is lossed, indicating all keys should be released.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

using Moriarty.Extensions;

namespace Moriarty.Commands
{
	public enum KeyEventType
	{
		/// <summary>
		/// Key was pressed down
		/// </summary>
		Pressed,

		/// <summary>
		/// Key was released from being pressed down
		/// </summary>
		Released,

		/// <summary>
		/// All keys were released - triggered on keyboard focus loss
		/// </summary>
		AllReleased,
	}

	public sealed class KeyboardKeyEventCommand : BaseMoriartyCommand
	{
		#region Private members
		UInt32 m_uVirtualKeyCode = 0u;
		KeyEventType m_eKeyEventType = KeyEventType.AllReleased;
		#endregion

		public KeyboardKeyEventCommand(UInt32 uVirtualKeyCode, KeyEventType eKeyEventType)
			: base(MoriartyConnection.ERPC.KeyboardKeyEvent)
		{
			m_uVirtualKeyCode = uVirtualKeyCode;
			m_eKeyEventType = eKeyEventType;
		}

		/// <summary>
		/// Send the KeyEvent data as a vritual key code and a single byte
		/// that represents the event type.
		/// </summary>
		protected override void WriteCommand(BinaryWriter writer)
		{
			writer.NetWriteUInt32(m_uVirtualKeyCode);
			writer.NetWriteByte((byte)m_eKeyEventType);
		}
	}
} // namespace Moriarty.Commands
