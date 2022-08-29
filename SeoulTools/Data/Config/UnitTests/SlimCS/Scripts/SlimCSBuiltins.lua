-- Glue required by SlimCS. Implements the body of various
-- builtin functionality (e.g. string.IsNullOrEmpty).
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.



local SlimCSStringLib = class_static 'SlimCSStringLib'

function SlimCSStringLib.IsNullOrEmpty(
	s)

	return nil == s or ''--[[String.Empty]] == s
end

function SlimCSStringLib.ReplaceAll(
	s, pattern, replacement)

	return CoreNative.StringReplace(s, pattern, replacement)
end
return SlimCSStringLib
