// Main entry point of the App UI's Lua code. Loaded into a VM
// specific to UI.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using static SlimCS;

[TopLevelChunk]
static class MainTopLevelChunk
{
	delegate void NativeConstruct(object oInstance);

	delegate bool NativeHas(string sName);

	delegate object NativeNew(string sName);

	/// <summary>
	/// Main entry point of the application.
	/// </summary>
	/// <remarks>
	/// This is the script invoked by the game to populate the game's
	/// script VM. The TopLevelChunk attribute indicates that
	/// this class is handled specially by SlimCS. The body
	/// of Chunk will be emitted verbatim as top-level Lua code.
	/// </remarks>
	static void Chunk()
	{
		// Absolutely first - if it exists, instantiate the native
		// instance that manages any native "script lifespan tied"
		// code. This is code that is implemented globally in native
		// but is a pre-requisite of any and all script implementation.
		if (dyncast<NativeHas>(_G["SeoulHasNativeUserData"])("AppSpecificNativeScriptDependencies"))
		{
			var ud = dyncast<NativeNew>(_G["SeoulNativeNewNativeUserData"])("AppSpecificNativeScriptDependencies");
			dyncast<NativeConstruct>(dyncast<Table>(ud)["Construct"])(ud);
			_G["AppSpecificNativeScriptDependenciesGlobalObject__5dd3aa5836981818 "] = ud;

			// Assign a closure to SeoulIsFullyInitialized so that the patcher can query
			// whether the script VM has fully initialized, based on game logic.
			var isFullyInitialized = (System.Func<object, bool>)dyncast<Table>(ud)["IsFullyInitialized"];
			if (null != isFullyInitialized)
			{
				_G["SeoulIsFullyInitialized"] = (System.Func<bool>)(() =>
				{
					return isFullyInitialized(ud);
				});
			}
		}

		// Kick off SlimCS initialization (this registers and static constructs
		// all types defined in SlimCS).
		require("SlimCSMain");
		require("SlimCSFiles");
	}
}
