/*
	ScriptUIMovie.cs
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
	public abstract class ScriptUIMovie : UI_Movie
	{
		public abstract void AppendFx(params object[] asArgs);
		public abstract void AppendSoundEvents(params string[] asArgs);
		public abstract void BindAndConstructRoot(string sClassName);
		public abstract object GetStateConfigValue(string sKey);
		public abstract RootMovieClip GetSiblingRootMovieClip(string sSiblingName);
		[Pure] public abstract (double, double, double, double) GetWorldCullBounds();
		public abstract Bitmap NewBitmap();
		public abstract MovieClip NewMovieClip(string sClassName, params object[] varargs);
		public abstract (double, double) GetMousePositionFromWorld(double fX, double fY);
		public abstract void OnAddToParent(object ludInstance);
		[Pure] public abstract (double, double) ReturnMousePositionInWorld();
		[Pure] public abstract RootMovieClip GetRootMovieClip();
		[Pure] public abstract int GetLastViewportWidth();
		[Pure] public abstract int GetLastViewportHeight();
	}
}
