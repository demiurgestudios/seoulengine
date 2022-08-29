-- Module of engine HTTP components and project agnostic utilities.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.



local HTTP = class_static 'HTTP'

local udHTTP = nil

function HTTP.cctor()

	udHTTP = CoreUtilities.NewNativeUserData(ScriptEngineHTTP)
	if nil == udHTTP then

		error 'Failed instantiating ScriptEngineHTTP.'
	end

	Globals.RegisterGlobalDispose(
		function()

			udHTTP:CancelAllRequests()
		end)
end

function HTTP.CancelAllRequests()

	udHTTP:CancelAllRequests()
end

function HTTP.CreateCachedRequest(
	sURL,
	oCallback,
	sMethod,
	bResendOnFailure)

	return udHTTP:CreateCachedRequest(sURL, oCallback, sMethod, bResendOnFailure)
end

function HTTP.CreateRequest(
	sURL,
	oCallback,
	sMethod,
	bResendOnFailure)

	return udHTTP:CreateRequest(sURL, oCallback, sMethod, bResendOnFailure)
end

function HTTP.GetCachedData(sURL)

	local sData, tHeaders = udHTTP:GetCachedData(sURL)
	return sData, tHeaders
end

function HTTP.OverrideCachedDataBody(sURL, tData)

	udHTTP:OverrideCachedDataBody(sURL, tData)
end
return HTTP
