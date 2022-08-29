// Table of edit text event types, roughly corresponds
// to ActionScript equivalents.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

// Events related to mouse input.
public static class EditTextEvent
{
	// Fired once editing begins.
	public const string START_EDITING = "startEditing";

	// Fired when editing finished
	public const string STOP_EDITING = "stopEditing";

	// Fired when editing finished
	public const string APPLY_EDITING = "applyEditing";
}
