-- Utility class, wraps a 2D vector.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.





local Vector2D = class 'Vector2D'

function Vector2D:Constructor(fX, fY)

	self.x = fX
	self.y = fY
end

function Vector2D:GetLength()


	return math.sqrt(self.x * self.x + self.y * self.y)
end

function Vector2D.__add(
	v1, v2)

	return Vector2D:New(v1.x + v2.x, v1.y + v2.y)
end

function Vector2D.__sub(
	v1, v2)

	return Vector2D:New(v1.x - v2.x, v1.y - v2.y)
end

function Vector2D.__mul(
	v, fScalar)

	return Vector2D:New(v.x * fScalar, v.y * fScalar)
end

function Vector2D.__div(
	v, fScalar)

	return Vector2D:New(v.x / fScalar, v.y / fScalar)
end

function Vector2D:__tostring()


	return '{x=' .. self.x .. ', y=' .. self.y .. '}'
end



return Vector2D
