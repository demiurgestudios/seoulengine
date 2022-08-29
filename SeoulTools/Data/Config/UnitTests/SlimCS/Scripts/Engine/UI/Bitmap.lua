-- Represents a Falcon::BitmapInstance in script.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.



local Bitmap = class('Bitmap', 'DisplayObject')

function Bitmap:ResetTexture()

	self.m_udNativeInstance:ResetTexture()
end

function Bitmap:SetIndirectTexture(symbol, iWidth, iHeight)

	self.m_udNativeInstance:SetIndirectTexture(symbol, iWidth, iHeight)
end

function Bitmap:SetTexture(filePath, iWidth, iHeight, bPrefetch)

	self.m_udNativeInstance:SetTexture(filePath, iWidth, iHeight, bPrefetch)
end
return Bitmap
