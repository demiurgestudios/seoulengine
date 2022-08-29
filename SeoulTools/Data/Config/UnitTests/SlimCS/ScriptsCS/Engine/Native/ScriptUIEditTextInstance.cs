/*
	ScriptUIEditTextInstance.cs
	AUTO GENERATED - DO NOT MODIFY
	API FOR NATIVE CLASS INSTANCE

	Run GenerateScriptBindings.bat in the Utilities folder to re-generate bindings.

	Copyright (c) Demiurge Studios, Inc.
	
	This source code is licensed under the MIT license.
	Full license details can be found in the LICENSE file
	in the root directory of this source tree.
*/

using System.Diagnostics.Contracts;

namespace Native
{
	public abstract class ScriptUIEditTextInstance : ScriptUIInstance
	{
		public abstract void CommitFormatting();
		[Pure] public abstract bool GetAutoSizeBottom();
		[Pure] public abstract bool GetAutoSizeContents();
		[Pure] public abstract bool GetAutoSizeHorizontal();
		[Pure] public abstract (int, int, int, int) GetCursorColor();
		[Pure] public abstract bool GetHasTextEditFocus();
		[Pure] public abstract int GetNumLines();
		[Pure] public abstract string GetPlainText();
		[Pure] public abstract string GetText();
		[Pure] public abstract string GetXhtmlText();
		[Pure] public abstract bool GetVerticalCenter();
		[Pure] public abstract int GetVisibleCharacters();
		[Pure] public abstract bool GetXhtmlParsing();
		public abstract void SetAutoSizeBottom(bool a0);
		public abstract void SetAutoSizeContents(bool a0);
		public abstract void SetAutoSizeHorizontal(bool a0);
		public abstract void SetCursorColor(int a0, int a1, int a2, int a3);
		public abstract void SetPlainText(string a0);
		public abstract void SetText(string a0);
		public abstract void SetXhtmlText(string a0);
		public abstract void SetTextToken(string a0);
		public abstract void SetVerticalCenter(bool a0);
		public abstract void SetVisibleCharacters(int a0);
		public abstract void SetXhtmlParsing(bool a0);

		public abstract bool StartEditing(
			Native.ScriptUIMovieClipInstance eventReceiver,
			string sDescription,
			int iMaxCharacters,
			string sRestrict,
			bool bAllowNonLatinKeyboard);

		public abstract void StopEditing();
		[Pure] public abstract (double?, double?, double?, double?) GetTextBounds();
		[Pure] public abstract (double?, double?, double?, double?) GetLocalTextBounds();
		[Pure] public abstract (double?, double?, double?, double?) GetWorldTextBounds();
	}
}
