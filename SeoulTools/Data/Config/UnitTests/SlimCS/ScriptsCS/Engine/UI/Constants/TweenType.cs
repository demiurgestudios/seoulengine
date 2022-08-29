// Controls tween behavior.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

// When calling AddTweenCruve(), this is the curve type.
public enum TweenType
{
	// Ease in-out with a cubic shape.
	m_iInOutCubic,

	// Ease in-out with a quadratic shape.
	m_iInOutQuadratic,

	// Ease in-out with a quartic shape.
	m_iInOutQuartic,

	// Simple linear interpolation.
	m_iLine,

	// Sine curve, high frequency.
	m_iSinStartFast,

	// Since curve, low frequency.
	m_iSinStartSlow,
}
