-- Utility class, wraps a 2D rectangle specified by
-- (left, top) and coordinates and a (height, width)
-- pair.
--
-- The Bounds constructor accepts the top-left and
-- bottom right coordinates to define the rectangle.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.



local Bounds = class 'Bounds'

function Bounds:Constructor(fLeft, fTop, fRight, fBottom)

	self.x = fLeft
	self.y = fTop
	self.width = fRight - fLeft
	self.height = fBottom - fTop
end

function Bounds:__tostring()


	return '{x=' .. self.x .. ', y=' .. self.y .. ', width=' .. self.width .. ', height=' .. self.height .. '}'
end





return Bounds
