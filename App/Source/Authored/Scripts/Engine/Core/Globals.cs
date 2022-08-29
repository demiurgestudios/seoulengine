// Core Globals, requiring this file causes global side effects
// and adds functions and values to global tables.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using static SlimCS;

static class Globals
{
	/// <summary>
	/// Global dispose functions will be called
	/// once just before the script environment
	/// is destroyed.
	/// </summary>
	public static void RegisterGlobalDispose(System.Action func)
	{
		table.insert(s_aGlobalDispose, func);
	}

	public static void RegisterGlobalMainThreadInit(System.Action func)
	{
		table.insert(s_aMainThreadInit, func);
	}

	public static void RegisterOnHotLoad(System.Action func)
	{
		table.insert(s_aOnHotLoad, func);
	}

	public static void RegisterPostHotLoad(System.Action func)
	{
		table.insert(s_aPostHotLoad, func);
	}

	public static void RegisterOnScriptInitializeComplete(System.Action func)
	{
		table.insert(s_aScriptInit, func);
	}

	static Table<double, System.Action> s_aGlobalDispose = new Table<double, System.Action>();
	static Table<double, System.Action> s_aMainThreadInit = new Table<double, System.Action>();
	static Table<double, System.Action> s_aOnHotLoad = new Table<double, System.Action>();
	static Table<double, System.Action> s_aPostHotLoad = new Table<double, System.Action>();
	static Table<double, System.Action> s_aScriptInit = new Table<double, System.Action>();

	static void Run(Table<double, System.Action> a)
	{
		foreach (var (_, func) in ipairs(a))
		{
			pcall(func);
		}
	}

	static Globals()
	{
		// Global hook invoked by native at a point where
		// this script VM is pending release.
		_G["SeoulDispose"] = (Vfunc0)(() =>
		{
			// Iterate over registered functions.
			var a = s_aGlobalDispose;
			s_aGlobalDispose = new Table<double, System.Action>();

			Run(a);
		});

		_G["SeoulMainThreadInit"] = (Vfunc0)(() =>
		{
			// Iterate over registered functions.
			var a = s_aMainThreadInit;
			s_aMainThreadInit = new Table<double, System.Action>();

			Run(a);
		});

		// Global hook invoked by native when a hotload is about to occur.
		_G["SeoulOnHotload"] = (Vfunc0)(() =>
		{
			// Iterate over registered functions - unlike
			// global dipose and init, we leave the table unmodified
			// (this callback can be fired multiple times in the same
			// VM instance).
			var a = s_aOnHotLoad;

			Run(a);
		});

		// Global hook invoked by native on the new VM when a hotload complete.
		_G["SeoulPostHotload"] = (Vfunc0)(() =>
		{
			// Iterate over registered functions.
			var a = s_aPostHotLoad;
			s_aPostHotLoad = new Table<double, System.Action>();

			Run(a);
		});

		_G["ScriptInitializeComplete"] = (Vfunc0)(() =>
		{
			// Iterate over registered functions.
			var a = s_aScriptInit;
			s_aScriptInit = new Table<double, System.Action>();

			Run(a);
		});
	}
}
