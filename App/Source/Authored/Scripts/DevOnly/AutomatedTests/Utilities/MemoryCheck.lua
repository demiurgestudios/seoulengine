--[[
 MemoryCheck.lua
 Applies a memory check and reporting. Can be used in
 multiple automated tests.

 Copyright (c) Demiurge Studios, Inc.
 
 This source code is licensed under the MIT license.
 Full license details can be found in the LICENSE file
 in the root directory of this source tree.
--]]

-- Growth percentange - max is increased by this value after it is reached.
local kfPeakIncrease = 0.25

-- NOTE: For both thresholds below, we are expecting "miscellaneous" overhead
-- of about 30 MB - that includes Java memory on Android, memory for the
-- program executable, allocator page overhead, etc.
--
-- Peak (single frame) threshold. This can introduce OOM conditions
-- but is not expected to hit low memory thresholds (since a single peak
-- is unlikely to last long enough to overlap with the app entering the background).
--
-- An exact universal value for this cannot be defined, but a target of 256 MB
-- allows us to comfortably stay under 60% of the memory of a 512 MB Android device.
local kiMaxPeak = 256 * 1024 * 1024
-- Average (rolling avg.) threshold. This is a better reflection of low
-- memory conditions and triggers at lower values. Low memory is a problem once
-- an app loses focus. If above the extreme threshold (TRIM_MEMORY_COMPLETE on Android,
-- or the low memory trigger in general on iOS), the app will be terminated as soon
-- as it enters the background.
--
-- An exact universal value for this cannot be defined, but a guess of the general
-- low memory threshold on Android is 25-40% of device memory (200+30 MB is ~45% of
-- a 512 MB device memory). iOS is generally 50-70% of device memory, so if we
-- fit on Android, we will fit comfortably on iOS (since our tech has a comparable
-- memory footprint on all our mobile platforms).
local kiMaxAvg = 200 * 1024 * 1024

-- Track whether we've issued a memory warning or not.
local s_fNextPeak = kiMaxPeak
local s_fNextAvg = kiMaxAvg

-- Accumulation
local s_iAvgSamples = nil
local s_aRollingAvg = {}

-- Format memory in bytes into a friendly string.
local function GetMemoryUsageString(iMemoryUsageInBytes)
	if (iMemoryUsageInBytes > 1024 * 1024) then
		return tostring(math.ceil(iMemoryUsageInBytes / (1024 * 1024))) .. " MBs"
	elseif (iMemoryUsageInBytes > 1024) then
		return tostring(math.ceil(iMemoryUsageInBytes / 1024)) .. " KBs"
	else
		return tostring(iMemoryUsageInBytes) .. " Bs"
	end
end

-- Utility, creates a friendly string of all memory buckets in tBuckets.
local function MemoryBucketsToString(tBuckets)
	local tSorted = {}

	-- Iterate over the budgets table and convert into a form that is sortable.
	for k,v in pairs(tBuckets) do
		-- Only include budgets that have a memory usage > 0.
		if v > 0 then
			table.insert(tSorted, { k=k, v=v })
		end
	end

	-- Sort the new table.
	table.sort(tSorted, function(a, b) return (a.v == b.v and (a.k < b.k) or (a.v > b.v)) end)

	local tPrintable = {}

	-- Finally generate a table that can be converted to a string.
	for _,v in ipairs(tSorted) do
		table.insert(tPrintable, v.k .. ': ' .. GetMemoryUsageString(v.v))
	end

	-- Now return the concat of the printable table.
	return table.concat(tPrintable, '\n'), tSorted[1]
end

local function GetMemoryUsage(tRequested)
	local iMemoryUsage = 0
	for _,v in pairs(tRequested) do
		iMemoryUsage = iMemoryUsage + v
	end
	return iMemoryUsage
end

local function ComputeRollingAverage(iVal)
	table.insert(s_aRollingAvg, iVal)
	local count = #s_aRollingAvg
	if s_iAvgSamples then
		while count > s_iAvgSamples do
			table.remove(s_aRollingAvg, 1)
			count = count - 1
		end
	end

	local fAvg = 0
	for i=1,count do
		local v = s_aRollingAvg[i]
		fAvg = fAvg + v
	end
	fAvg = fAvg / count

	return fAvg
end

local function ReportMemory(udNativeAutomationFunctions, sMsg, tBuckets, fVal, fMax)
	udNativeAutomationFunctions:Warn(
		'Memory warning, ' .. sMsg .. ' is ' .. GetMemoryUsageString(fVal) ..
		" which is over the threshold of " .. GetMemoryUsageString(fMax) .. ".")
	udNativeAutomationFunctions:Warn("Memory buckets:")

	local str, _ = MemoryBucketsToString(tBuckets)
	udNativeAutomationFunctions:Warn(str)

	-- Include script memory usage
	local tScriptBuckets = udNativeAutomationFunctions:GetScriptMemoryUsageBuckets()
	if next(tScriptBuckets) then
		udNativeAutomationFunctions:Warn("Script memory buckets:")
		local str, _ = MemoryBucketsToString(tScriptBuckets)
		udNativeAutomationFunctions:Warn(str)
	end
end

local function CheckMemory(udNativeAutomationFunctions, sMsg, tBuckets, fVal, fMax)
	local bReported = false
	if fVal > fMax then
		ReportMemory(
			udNativeAutomationFunctions,
			sMsg,
			tBuckets,
			fVal,
			fMax)
		fMax = fMax + (fMax * kfPeakIncrease)
		bReported = true
	end

	return fMax, bReported
end

local s_MarkerSeconds = nil
local s_Ticks = 0
local s_LastPeakReportMarker = nil
local s_CurrentMaxPeak = 0

return function(udNativeAutomationFunctions)
	-- Check and potentially issue a memory warning.
	return function()
		local now = Engine.GetGameTimeInSeconds()
		local tBuckets = udNativeAutomationFunctions:GetRequestedMemoryUsageBuckets()
		local iPeak = GetMemoryUsage(tBuckets)
		local fAvg = ComputeRollingAverage(iPeak)

		-- Periodic peak reporting (every 15 seconds).
		s_CurrentMaxPeak = math.max(s_CurrentMaxPeak, iPeak)
		if not s_LastPeakReportMarker then
			s_LastPeakReportMarker = now
		elseif (now - s_LastPeakReportMarker) >= 30 then
			s_LastPeakReportMarker = nil
			udNativeAutomationFunctions:Log('MEMORY (peak of last 15 seconds): ' .. GetMemoryUsageString(s_CurrentMaxPeak))
			s_CurrentMaxPeak = 0
		end

		-- Peak.
		s_fNextPeak = CheckMemory(udNativeAutomationFunctions, 'single frame peak (OOM threshold)', tBuckets, iPeak, s_fNextPeak)
		-- Avg.
		if not s_iAvgSamples then
			s_Ticks = s_Ticks + 1
			-- Compute the sampling window of average.
			if not s_MarkerSeconds then
				s_MarkerSeconds = now
			else
				local fDelta = (now - s_MarkerSeconds)
				if fDelta >= 5.0 then
					-- We want the average to reflect 5 seconds of samples.
					s_iAvgSamples = math.floor((s_Ticks * 5.0) / fDelta)
					-- Reporting.
					udNativeAutomationFunctions:Log('Memory checking is using ' .. tostring(s_iAvgSamples) .. ' for the size of the averaging window.')
				end
			end
		else
			local fPrev = s_fNextAvg
			local bReported = false
			s_fNextAvg, bReported = CheckMemory(udNativeAutomationFunctions, 'average (low memory threshold)', tBuckets, fAvg, s_fNextAvg)

			-- Also log details - 'Unknown' logs all buckets. We only
			-- do this the first time (kiMaxAvg == fPrev) to avoid too
			-- much spam in the log.
			if bReported and kiMaxAvg == fPrev then
				udNativeAutomationFunctions:LogMemoryDetails('Unknown')
			end
		end
	end
end
