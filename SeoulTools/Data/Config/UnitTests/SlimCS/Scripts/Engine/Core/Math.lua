-- Our math extensions. Adds functionality not available in the builtin
-- math module.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.





local Math = class 'Math'

function Math.Clamp(
	fValue, fMinimum, fMaximum)

	return math.min(math.max(fValue, fMinimum), fMaximum)
end

function Math.Lerp(
	fA, fB, fAlpha)

	return asnum(fA * (1 - fAlpha) + fB * fAlpha)
end

function Math.Round(
	fValue)

	if fValue > 0 then

		return math.floor(fValue + 0.5)

	else

		return math.ceil(fValue - 0.5)
	end
end
return Math
