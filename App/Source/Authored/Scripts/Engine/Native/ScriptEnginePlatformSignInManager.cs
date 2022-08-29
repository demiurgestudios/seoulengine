/*
	ScriptEnginePlatformSignInManager.cs
	AUTO GENERATED - DO NOT MODIFY
	API FOR NATIVE CLASS INSTANCE

	Run GenerateScriptBindings.bat in the Utilities folder to re-generate bindings.

	Copyright (c) 2018-2022 Demiurge Studios Inc.  All rights reserved.
*/

using System.Diagnostics.Contracts;

namespace Native
{
	public abstract class ScriptEnginePlatformSignInManager
	{
		[Pure] public abstract int GetStateChangeCount();
		[Pure] public abstract bool IsSignedIn();
		[Pure] public abstract bool IsSigningIn();
		[Pure] public abstract bool IsSignInSupported();
		public abstract void SignIn();
		public abstract void SignOut();
	}
}
