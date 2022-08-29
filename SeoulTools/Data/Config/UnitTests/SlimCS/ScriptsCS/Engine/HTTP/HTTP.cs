// Module of engine HTTP components and project agnostic utilities.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using static SlimCS;

public static class HTTP
{
	static readonly Native.ScriptEngineHTTP udHTTP = null;

	static HTTP()
	{
		udHTTP = CoreUtilities.NewNativeUserData<Native.ScriptEngineHTTP>();
		if (null == udHTTP)
		{
			error("Failed instantiating ScriptEngineHTTP.");
		}

		Globals.RegisterGlobalDispose(
			() =>
			{
				udHTTP.CancelAllRequests();
			});
	}

	public static void CancelAllRequests()
	{
		udHTTP.CancelAllRequests();
	}

	public static Native.ScriptEngineHTTPRequest CreateCachedRequest(
		string sURL,
		object oCallback = null,
		string sMethod = HTTPMethods.m_sMethodGet,
		bool bResendOnFailure = true)
	{
		return udHTTP.CreateCachedRequest(sURL, oCallback, sMethod, bResendOnFailure);
	}

	public static Native.ScriptEngineHTTPRequest CreateRequest(
		string sURL,
		object oCallback = null,
		string sMethod = HTTPMethods.m_sMethodGet,
		bool bResendOnFailure = true)
	{
		return udHTTP.CreateRequest(sURL, oCallback, sMethod, bResendOnFailure);
	}

	public static (string, Table) GetCachedData(string sURL)
	{
		(var sData, var tHeaders) = udHTTP.GetCachedData(sURL);
		return (sData, tHeaders);
	}

	public static void OverrideCachedDataBody(string sURL, Table tData)
	{
		udHTTP.OverrideCachedDataBody(sURL, tData);
	}
}
