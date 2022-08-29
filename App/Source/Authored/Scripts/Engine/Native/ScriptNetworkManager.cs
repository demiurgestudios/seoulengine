/*
	ScriptNetworkManager.cs
	AUTO GENERATED - DO NOT MODIFY
	API FOR NATIVE CLASS INSTANCE

	Run GenerateScriptBindings.bat in the Utilities folder to re-generate bindings.

	Copyright (c) 2018-2022 Demiurge Studios Inc.  All rights reserved.
*/

namespace Native
{
	public abstract class ScriptNetworkManager
	{
		public abstract Native.ScriptNetworkMessenger NewMessenger(string hostname, int port, string encryptionKeyBase32);
	}
}
