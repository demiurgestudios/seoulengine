// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.
namespace System.Runtime.CompilerServices
{
	/// <summary>
	/// Specifies the details of how a method is implemented. This class cannot be inherited.
	/// </summary>
	public sealed class MethodImplAttribute : Attribute
	{
		/// <summary>
		/// Initializes a new instance of the System.Runtime.CompilerServices.MethodImplAttribute
		/// class.
		/// </summary>
		public MethodImplAttribute()
		{
		}

		/// <summary>
		/// Initializes a new instance of the System.Runtime.CompilerServices.MethodImplAttribute
		/// class with the specified System.Runtime.CompilerServices.MethodImplOptions value.
		/// </summary>
		/// <param name="methodImplOptions">A System.Runtime.CompilerServices.MethodImplOptions value specifying properties of the attributed method.</param>
		public MethodImplAttribute(MethodImplOptions methodImplOptions)
		{
		}
	}
}
