// Table of general event types, roughly corresponds
// to ActionScript equivalents.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

// General events, no specific category.
public static class Event
{
	// The enter frame event matches ActionScript/Flash semantics. When registered,
	// it is dispatched once per advance of the current movie. As a result, this event
	// is received at the Flash movie's frame rate (e.g. registers against a 30 FPS
	// movie will receive this event 30 times per second).
	public const string ENTER_FRAME = "enterFrame";

	// The tick event is a custom SeoulEngine event. It is dispatched once per app
	// update frame. As a result, this event is received at the app's frame rate (e.g.
	// registers against this even will receive updates at 60 FPS, if the app is updating
	// at 60 FPS, even if the current movie was authored at 30 FPS).
	public const string TICK = "tick";

	// The tick scaled event is a custom SeoulEngine event. It is dispatched once per app
	// update frame and scaled based on game time scale. As a result, this event is 
	// received at the app's frame rate (e.g. registers against this even will receive updates 
	// at 60 FPS, if the app is updating at 60 FPS, even if the current movie was authored at 30 FPS).
	public const string TICK_SCALED = "tickScaled";

	// Event fire when a hyperlink is clicked within an EditText instance.
	public const string LINK_CLICKED = "linkClicked";
}
