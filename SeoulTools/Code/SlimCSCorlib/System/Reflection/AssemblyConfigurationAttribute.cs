// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

namespace System.Reflection
{
	public sealed class AssemblyConfigurationAttribute : Attribute
	{
		public AssemblyConfigurationAttribute(string configuration)
		{
			Configuration = configuration;
		}

		public string Configuration { get; }
	}
}
