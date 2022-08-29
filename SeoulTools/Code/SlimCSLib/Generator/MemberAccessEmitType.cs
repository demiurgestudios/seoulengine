// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

namespace SlimCSLib.Generator
{
	public enum MemberAccessEmitType
	{
		/// <summary>
		/// A property or field lookup in base. Not necessarily an invocation.
		/// </summary>
		BaseAccess,

		/// <summary>
		/// The full member access expression is actually a property event add method from class's base.
		/// </summary>
		BaseEventAdd,

		/// <summary>
		/// The full member access expression is actually an invocation of an event from class's base.
		/// </summary>
		BaseEventRaise,

		/// <summary>
		/// The full member access expression is actually a property event remove method from class's base.
		/// </summary>
		BaseEventRemove,

		/// <summary>
		/// The full member access expression is actually a property getter from class's base.
		/// </summary>
		BaseGetter,

		/// <summary>
		/// Method invoke is a base invocation (e.g. base.A()).
		/// </summary>
		BaseMethod,

		/// <summary>
		/// The full member access expression is actually a property setter from class's base.
		/// </summary>
		BaseSetter,

		/// <summary>
		/// Member access is normal (use the '.' in Lua).
		/// </summary>
		Normal,

		/// <summary>
		/// The full member access expression is actually a property event add method.
		/// </summary>
		EventAdd,

		/// <summary>
		/// The full member access expression is actually an invocation of an event.
		/// </summary>
		EventRaise,

		/// <summary>
		/// The full member access expression is actually a property event remove method.
		/// </summary>
		EventRemove,

		/// <summary>
		/// Member access is actually an extension method (elide the target of the member access).
		/// </summary>
		Extension,

		/// <summary>
		/// The full member access expression is actually a property getter.
		/// </summary>
		Getter,

		/// <summary>
		/// Member access is a send invoke (use the ':' in Lua).
		/// </summary>
		Send,

		/// <summary>
		/// Identical to Send, except that the target function is
		/// not an instance member method but has an invocation signature
		/// that allows it to be converted into a send (e.g. a lambda
		/// that is called a.b(a, ...).
		/// </summary>
		SendConverted,

		/// <summary>
		/// The full member access expression is actually a property setter.
		/// </summary>
		Setter,
	}
}
