// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

namespace System.Diagnostics
{
	public sealed class ConditionalAttribute : Attribute
	{ 
		public ConditionalAttribute(string conditionString)
		{
			ConditionString = conditionString;
		}

		public string ConditionString { get; private set; }
	}

	namespace Contracts
	{
		/// <summary>
		/// Indicates that a type or method is pure, that is, it does not make any visible
		/// state changes.
		/// </summary>
		public sealed class PureAttribute : Attribute
		{
			public PureAttribute()
			{
			}
		}
	}
}
