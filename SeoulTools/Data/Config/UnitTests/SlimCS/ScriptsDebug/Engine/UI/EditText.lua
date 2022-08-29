-- Represents a Falcon::EditTextInstance in script. Most often,
-- used to call SetText() or SetTextToken() to update the body
-- of an EditText instance.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.



local FocusEvent = class_static 'FocusEvent'

FocusEvent.FOCUS_IN = 'focusIn'


local EditText = class('EditText', 'DisplayObject')

--- <summary>
--- Convenience - resturns the text of an EditText
--- restricted to its EditingMaxChars value, if set.
--- </summary>
--- <param name="txt">The edit text to apply.</param>
--- <returns>The limited text value.</returns>


function EditText:get_LimitedText()

	local sText = self:GetText()
	local iChars = self.EditingMaxChars or 0
	if iChars >= 0 then

		sText = CoreNative.StringSub(sText, 0, iChars)
	end

	return sText
end


function EditText:CommitFormatting()

	self.m_udNativeInstance:CommitFormatting()
end

function EditText:GetAutoSizeBottom()

	return self.m_udNativeInstance:GetAutoSizeBottom()
end

function EditText:GetAutoSizeContents()

	return self.m_udNativeInstance:GetAutoSizeContents()
end

function EditText:GetAutoSizeHorizontal()

	return self.m_udNativeInstance:GetAutoSizeHorizontal()
end

function EditText:GetCursorColor()

	return self.m_udNativeInstance:GetCursorColor()
end

function EditText:GetHasTextEditFocus()

	return self.m_udNativeInstance:GetHasTextEditFocus()
end

function EditText:GetNumLines()

	return self.m_udNativeInstance:GetNumLines()
end

function EditText:GetPlainText()

	return self.m_udNativeInstance:GetPlainText()
end

function EditText:GetText()

	return self.m_udNativeInstance:GetText()
end

function EditText:GetXhtmlText()

	return self.m_udNativeInstance:GetXhtmlText()
end

function EditText:GetVerticalCenter()

	return self.m_udNativeInstance:GetVerticalCenter()
end

function EditText:GetXhtmlParsing()

	return self.m_udNativeInstance:GetXhtmlParsing()
end

function EditText:GetTextBounds()

	return self.m_udNativeInstance:GetTextBounds()
end

function EditText:GetTextHeight()

	local _, t, _, b = self:GetTextBounds()
	return b - t
end

function EditText:GetTextWidth()

	local l, _, r, _ = self:GetTextBounds()
	return r - l
end

function EditText:GetLocalTextBounds()

	return self.m_udNativeInstance:GetLocalTextBounds()
end

function EditText:GetVisibleCharacters()

	return self.m_udNativeInstance:GetVisibleCharacters()
end

function EditText:GetWorldTextBounds()

	return self.m_udNativeInstance:GetWorldTextBounds()
end

function EditText:SetAutoSizeBottom(bAutoSizeBottom)

	self.m_udNativeInstance:SetAutoSizeBottom(bAutoSizeBottom)
end

function EditText:SetAutoSizeContents(bAutoSizeContents)

	self.m_udNativeInstance:SetAutoSizeContents(bAutoSizeContents)
end

function EditText:SetAutoSizeHorizontal(bAutoSizeHorizontal)

	self.m_udNativeInstance:SetAutoSizeHorizontal(bAutoSizeHorizontal)
end

function EditText:SetCursorColor(iR, iG, iB, iA)

	self.m_udNativeInstance:SetCursorColor(iR, iG, iB, iA)
end

function EditText:SetPlainText(sText)

	self.m_udNativeInstance:SetPlainText(sText)
end

function EditText:SetText(sText)

	self.m_udNativeInstance:SetText(sText)
end

function EditText:SetTextToken(sTextToken)

	self.m_udNativeInstance:SetTextToken(sTextToken)
end

function EditText:SetVerticalCenter(bVerticalCenter)

	self.m_udNativeInstance:SetVerticalCenter(bVerticalCenter)
end

function EditText:SetVisibleCharacters(chars)

	self.m_udNativeInstance:SetVisibleCharacters(chars)
end

function EditText:SetXhtmlParsing(bXhtmlParsing)

	self.m_udNativeInstance:SetXhtmlParsing(bXhtmlParsing)
end

function EditText:SetXhtmlText(sText)

	self.m_udNativeInstance:SetXhtmlText(sText)
end

--- <summary>
--- Optional - used if StartEditing() without complete arguments is called.
--- </summary>


--- <summary>
--- Optional - used if StartEditing() without complete arguments is called.
--- </summary>


--- <summary>
--- Optional - used if StartEditing() without complete arguments is called.
--- </summary>


function EditText:StartEditing(
	eventReceiver,
	bAllowNonLatinKeyboard)

	local sDescription = self.EditingDescription or ''--[[String.Empty]]
	local iMaxCharacters = self.EditingMaxChars or 0
	local sRestrict = self.EditingRestrictedCharacters or ''--[[String.Empty]]

	return self:StartEditing5_80E305EB(eventReceiver, sDescription, iMaxCharacters, sRestrict, bAllowNonLatinKeyboard)
end

function EditText:StartEditing5_80E305EB(
	eventReceiver,
	sDescription,
	iMaxCharacters,
	sRestrict,
	bAllowNonLatinKeyboard)

	local bReturn = self.m_udNativeInstance:StartEditing(
		eventReceiver:get_NativeInstance(),
		sDescription,
		iMaxCharacters,
		sRestrict,
		bAllowNonLatinKeyboard)

	-- Focus in on first start/successful start.
	if bReturn then

		self:DispatchEvent(self, 'focusIn'--[[FocusEvent.FOCUS_IN]])
	end

	return bReturn
end

function EditText:StopEditing()

	self.m_udNativeInstance:StopEditing()
end
return EditText
