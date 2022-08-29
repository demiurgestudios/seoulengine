-- Utility class, wraps a 2D position.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.





local Point = class 'Point'

function Point:Constructor(fX, fY)

	self.x = fX
	self.y = fY
end

function Point:GetLength()


	return math.sqrt(self.x * self.x + self.y * self.y)
end

function Point.__add(
	point1, point2)

	return Point:New(point1.x + point2.x, point1.y + point2.y)
end

function Point.__sub(
	point1, point2)

	return Point:New(point1.x - point2.x, point1.y - point2.y)
end

function Point.__mul(
	v, fScalar)

	return Point:New(v.x * fScalar, v.y * fScalar)
end

function Point.__div(
	v, fScalar)

	return Point:New(v.x / fScalar, v.y / fScalar)
end

function Point:__tostring()


	return '{x=' .. self.x .. ', y=' .. self.y .. '}'
end



return Point
