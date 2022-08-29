// MovieScroller.cs
// Static global class that enables horizontal scrolling
// of entire UI movies.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using static SlimCS;

public delegate void ClickedDelegate(MovieClip oMovieClip, double iX, double iY);

public delegate void MouseDelegate(MovieClip oMovieClip, double iX, double iY, bool bInShape);

public delegate void MouseUpDelegate(MovieClip oMovieClip, double iX, double iY, bool bInShape, double iInputCaptureHitTestMask);

public delegate void MouseWheelDelegate(MovieClip oMovieClip, double iX, double iY, double fDelta);
