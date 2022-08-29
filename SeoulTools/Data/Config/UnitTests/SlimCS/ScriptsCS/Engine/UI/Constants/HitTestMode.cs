// Flags that can be applied to hit testing to control what is hit.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

// Hit test options to SetHitTestSelfMode() and
// SetHitTestChildrenMode().
public static class HitTestMode
{
	// These correlate with constants in FalconConstants.h
	public const int m_iOff = 0; // (no bits)
	public const int m_iClickInputOnly = 1 << 0;
	public const int m_iHorizontalDragInputOnly = 1 << 1;
	public const int m_iVerticalDragInputOnly = 1 << 2;
	public const int m_iDragInputOnly = 1 << 1 | 1 << 2;
	public const int m_iClickAndDragInput = 1 << 0 | 1 << 1 | 1 << 2;
	public const int m_iMaskAll = 255; // (0xFF)
}
