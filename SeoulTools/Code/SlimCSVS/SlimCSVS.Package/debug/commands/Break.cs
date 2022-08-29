// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System.IO;

namespace SlimCSVS.Package.debug.commands
{
	sealed class Break : BaseCommand
	{
		public Break()
			: base(Connection.ERPC.BreakCommand)
		{
		}

		protected override void WriteCommand(BinaryWriter writer)
		{
		}
	}
}
