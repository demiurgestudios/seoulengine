// Command sent from the server to a client when full keyboard
// capture is enabled and a Unicode character event is captured.
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
	public sealed class KeyboardCharEventCommand : BaseMoriartyCommand
	{
		#region Private members
		UInt32 m_Character = 0;
		#endregion

		public KeyboardCharEventCommand(UInt32 character)
			: base(MoriartyConnection.ERPC.KeyboardCharEvent)
		{
			m_Character = character;
		}

		/// <summary>
		/// Send the char data as unicode character code to the client.
		/// </summary>
		protected override void WriteCommand(BinaryWriter writer)
		{
			writer.NetWriteUInt32(m_Character);
		}
	}
} // namespace Moriarty.Commands
