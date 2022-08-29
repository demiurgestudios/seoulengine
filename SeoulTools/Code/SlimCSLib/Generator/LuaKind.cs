// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

namespace SlimCSLib.Generator
{
	/// <summary>
	/// Classification of types into a few special categories
	/// for particular handling.
	/// </summary>
	public enum LuaKind
	{
		/// <summary>
		/// Nothing specific, handled typically.
		/// </summary>
		NotSpecial,

		/// <summary>
		/// Our flexible "dynamic" cast function that allows
		/// coercion from anything to anything.
		/// </summary>
		Dyncast,

		/// <summary>
		/// Our replacement for a dynamic. Allows limited dynamic like
		/// behavior.
		/// </summary>
		Dval,

		/// <summary>
		/// Tuple made up of generic types, used for marshaling
		/// multiple return values and a few other contexts.
		/// </summary>
		Tuple,

		/// <summary>
		/// Instantiation of the ipairs utility type, converted
		/// into a call to the ipairs function in Lua.
		/// </summary>
		Ipairs,

		/// <summary>
		/// Called to the initialization progress hook - used to
		/// track progress of VM startup.
		/// </summary>
		OnInitProgress,

		/// <summary>
		/// Instantiation of the pairs utility type, converted
		/// into a call to the pairs function in Lua.
		/// </summary>
		Pairs,

		/// <summary>
		/// Call of the require() utility.
		/// </summary>
		Require,

		/// <summary>
		/// Instantiation of the Table utility type, exactly
		/// equivalent to a Lua table.
		/// </summary>
		Table,
	}
}
