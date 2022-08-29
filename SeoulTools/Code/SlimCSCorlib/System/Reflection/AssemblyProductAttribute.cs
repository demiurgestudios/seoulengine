// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

namespace System.Reflection
{
	public sealed class AssemblyProductAttribute : Attribute
	{
		public AssemblyProductAttribute(string product)
		{
			Product = product;
		}

		public string Product { get; }
	}
}
