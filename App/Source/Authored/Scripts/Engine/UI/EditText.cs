// Represents a Falcon::EditTextInstance in script. Most often,
// used to call SetText() or SetTextToken() to update the body
// of an EditText instance.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using static SlimCS;

public static class FocusEvent
{
	public const string FOCUS_IN = "focusIn";
}

public sealed class EditText : DisplayObject
{
	/// <summary>
	/// Convenience - resturns the text of an EditText
	/// restricted to its EditingMaxChars value, if set.
	/// </summary>
	/// <param name="txt">The edit text to apply.</param>
	/// <returns>The limited text value.</returns>
	public string LimitedText
	{
		get
		{
			var sText = GetText();
			var iChars = EditingMaxChars ?? 0;
			if (iChars >= 0)
			{
				sText = CoreNative.StringSub(sText, 0, iChars);
			}

			return sText;
		}
	}

	public void CommitFormatting()
	{
		dyncast<Native.ScriptUIEditTextInstance>(m_udNativeInstance).CommitFormatting();
	}

	public bool GetAutoSizeBottom()
	{
		return dyncast<Native.ScriptUIEditTextInstance>(m_udNativeInstance).GetAutoSizeBottom();
	}

	public bool GetAutoSizeContents()
	{
		return dyncast<Native.ScriptUIEditTextInstance>(m_udNativeInstance).GetAutoSizeContents();
	}

	public bool GetAutoSizeHorizontal()
	{
		return dyncast<Native.ScriptUIEditTextInstance>(m_udNativeInstance).GetAutoSizeHorizontal();
	}

	public (int, int, int, int) GetCursorColor()
	{
		return dyncast<Native.ScriptUIEditTextInstance>(m_udNativeInstance).GetCursorColor();
	}

	public bool GetHasTextEditFocus()
	{
		return dyncast<Native.ScriptUIEditTextInstance>(m_udNativeInstance).GetHasTextEditFocus();
	}

	public int GetNumLines()
	{
		return dyncast<Native.ScriptUIEditTextInstance>(m_udNativeInstance).GetNumLines();
	}

	public string GetPlainText()
	{
		return dyncast<Native.ScriptUIEditTextInstance>(m_udNativeInstance).GetPlainText();
	}

	public string GetText()
	{
		return dyncast<Native.ScriptUIEditTextInstance>(m_udNativeInstance).GetText();
	}

	public string GetXhtmlText()
	{
		return dyncast<Native.ScriptUIEditTextInstance>(m_udNativeInstance).GetXhtmlText();
	}

	public bool GetVerticalCenter()
	{
		return dyncast<Native.ScriptUIEditTextInstance>(m_udNativeInstance).GetVerticalCenter();
	}

	public bool GetXhtmlParsing()
	{
		return dyncast<Native.ScriptUIEditTextInstance>(m_udNativeInstance).GetXhtmlParsing();
	}

	public (double, double, double, double) GetTextBounds()
	{
		return ((double, double, double, double))dyncast<Native.ScriptUIEditTextInstance>(m_udNativeInstance).GetTextBounds();
	}

	public double GetTextHeight()
	{
		var (_, t, _, b) = GetTextBounds();
		return b - t;
	}

	public double GetTextWidth()
	{
		var (l, _, r, _) = GetTextBounds();
		return r - l;
	}

	public (double, double, double, double) GetLocalTextBounds()
	{
		return ((double, double, double, double))dyncast<Native.ScriptUIEditTextInstance>(m_udNativeInstance).GetLocalTextBounds();
	}

	public double GetVisibleCharacters()
	{
		return dyncast<Native.ScriptUIEditTextInstance>(m_udNativeInstance).GetVisibleCharacters();
	}

	public (double, double, double, double) GetWorldTextBounds()
	{
		return ((double, double, double, double))dyncast<Native.ScriptUIEditTextInstance>(m_udNativeInstance).GetWorldTextBounds();
	}

	public void SetAutoSizeBottom(bool bAutoSizeBottom)
	{
		dyncast<Native.ScriptUIEditTextInstance>(m_udNativeInstance).SetAutoSizeBottom(bAutoSizeBottom);
	}

	public void SetAutoSizeContents(bool bAutoSizeContents)
	{
		dyncast<Native.ScriptUIEditTextInstance>(m_udNativeInstance).SetAutoSizeContents(bAutoSizeContents);
	}

	public void SetAutoSizeHorizontal(bool bAutoSizeHorizontal)
	{
		dyncast<Native.ScriptUIEditTextInstance>(m_udNativeInstance).SetAutoSizeHorizontal(bAutoSizeHorizontal);
	}

	public void SetCursorColor(int iR, int iG, int iB, int iA)
	{
		dyncast<Native.ScriptUIEditTextInstance>(m_udNativeInstance).SetCursorColor(iR, iG, iB, iA);
	}

	public void SetPlainText(string sText)
	{
		dyncast<Native.ScriptUIEditTextInstance>(m_udNativeInstance).SetPlainText(sText);
	}

	public void SetText(string sText)
	{
		dyncast<Native.ScriptUIEditTextInstance>(m_udNativeInstance).SetText(sText);
	}

	public void SetTextToken(string sTextToken)
	{
		dyncast<Native.ScriptUIEditTextInstance>(m_udNativeInstance).SetTextToken(sTextToken);
	}

	public void SetVerticalCenter(bool bVerticalCenter)
	{
		dyncast<Native.ScriptUIEditTextInstance>(m_udNativeInstance).SetVerticalCenter(bVerticalCenter);
	}

	public void SetVisibleCharacters(int chars)
	{
		dyncast<Native.ScriptUIEditTextInstance>(m_udNativeInstance).SetVisibleCharacters(chars);
	}

	public void SetXhtmlParsing(bool bXhtmlParsing)
	{
		dyncast<Native.ScriptUIEditTextInstance>(m_udNativeInstance).SetXhtmlParsing(bXhtmlParsing);
	}

	public void SetXhtmlText(string sText)
	{
		dyncast<Native.ScriptUIEditTextInstance>(m_udNativeInstance).SetXhtmlText(sText);
	}

	/// <summary>
	/// Optional - used if StartEditing() without complete arguments is called.
	/// </summary>
	public string EditingDescription { get; set; }

	/// <summary>
	/// Optional - used if StartEditing() without complete arguments is called.
	/// </summary>
	public int? EditingMaxChars { get; set; }

	/// <summary>
	/// Optional - used if StartEditing() without complete arguments is called.
	/// </summary>
	public string EditingRestrictedCharacters { get; set; }

	public bool StartEditing(
		MovieClip eventReceiver,
		bool bAllowNonLatinKeyboard = false)
	{
		var sDescription = EditingDescription ?? string.Empty;
		var iMaxCharacters = EditingMaxChars ?? 0;
		var sRestrict = EditingRestrictedCharacters ?? string.Empty;

		return StartEditing(eventReceiver, sDescription, iMaxCharacters, sRestrict, bAllowNonLatinKeyboard);
	}

	public bool StartEditing(
		MovieClip eventReceiver,
		string sDescription,
		int iMaxCharacters,
		string sRestrict,
		bool bAllowNonLatinKeyboard = false)
	{
		var bReturn = dyncast<Native.ScriptUIEditTextInstance>(m_udNativeInstance).StartEditing(
			dyncast<Native.ScriptUIMovieClipInstance>(eventReceiver.NativeInstance),
			sDescription,
			iMaxCharacters,
			sRestrict,
			bAllowNonLatinKeyboard);

		// Focus in on first start/successful start.
		if (bReturn)
		{
			DispatchEvent(this, FocusEvent.FOCUS_IN);
		}

		return bReturn;
	}

	public void StopEditing()
	{
		dyncast<Native.ScriptUIEditTextInstance>(m_udNativeInstance).StopEditing();
	}
}
