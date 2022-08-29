// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

namespace System.Runtime.InteropServices
{
	public sealed class ComVisibleAttribute : Attribute
	{
		public ComVisibleAttribute(bool visibility)
		{
			Value = visibility;
		}

		public bool Value { get; }
	}
}
