/*
	ScriptEngineHTTP.cs
	AUTO GENERATED - DO NOT MODIFY
	API FOR NATIVE CLASS INSTANCE

	Run GenerateScriptBindings.bat in the Utilities folder to re-generate bindings.

	Copyright (c) Demiurge Studios, Inc.
	
	This source code is licensed under the MIT license.
	Full license details can be found in the LICENSE file
	in the root directory of this source tree.
*/

using System.Diagnostics.Contracts;

namespace Native
{
	public abstract class ScriptEngineHTTP
	{
		public abstract void CancelAllRequests();

		public abstract ScriptEngineHTTPRequest CreateRequest(
			string sURL,
			object oCallback = null,
			string sMethod = HTTPMethods.m_sMethodGet,
			bool bResendOnFailure = true);

		public abstract ScriptEngineHTTPRequest CreateCachedRequest(
			string sURL,
			object oCallback = null,
			string sMethod = HTTPMethods.m_sMethodGet,
			bool bResendOnFailure = true);

		[Pure] public abstract (string, SlimCS.Table) GetCachedData(string sURL);
		public abstract void OverrideCachedDataBody(string sURL, SlimCS.Table tData);
	}
}
