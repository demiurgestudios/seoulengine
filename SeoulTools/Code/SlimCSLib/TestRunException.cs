// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;

namespace SlimCSLib
{
	public sealed class TestRunException : Exception
	{
		public TestRunException(string sMessage, string sBodyLua)
			: base(sMessage)
		{
			BodyLua = sBodyLua;
		}

		public string BodyLua { get; private set; }
	}
}
