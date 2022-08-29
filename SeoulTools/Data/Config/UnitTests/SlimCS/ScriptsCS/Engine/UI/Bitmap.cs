// Represents a Falcon::BitmapInstance in script.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using static SlimCS;

public sealed class Bitmap : DisplayObject
{
	public void ResetTexture()
	{
		dyncast<Native.ScriptUIBitmapInstance>(m_udNativeInstance).ResetTexture();
	}

	public void SetIndirectTexture(string symbol, double iWidth, double iHeight)
	{
		dyncast<Native.ScriptUIBitmapInstance>(m_udNativeInstance).SetIndirectTexture(symbol, iWidth, iHeight);
	}

	public void SetTexture(Native.FilePath filePath, double iWidth, double iHeight, bool bPrefetch = false)
	{
		dyncast<Native.ScriptUIBitmapInstance>(m_udNativeInstance).SetTexture(filePath, iWidth, iHeight, bPrefetch);
	}
}
