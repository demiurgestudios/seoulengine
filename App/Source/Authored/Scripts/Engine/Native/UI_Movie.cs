/*
	UI_Movie.cs
	AUTO GENERATED - DO NOT MODIFY
	API FOR NATIVE CLASS INSTANCE

	Run GenerateScriptBindings.bat in the Utilities folder to re-generate bindings.

	Copyright (c) 2018-2022 Demiurge Studios Inc.  All rights reserved.
*/

using System.Diagnostics.Contracts;

namespace Native
{
	public abstract class UI_Movie
	{
		[Pure] public abstract bool AcceptingInput();
		[Pure] public abstract string GetMovieTypeName();
		public abstract bool HasActiveFx(bool a0);
		[Pure] public abstract bool IsTop();
		public abstract void SetAcceptInput(bool a0);
		public abstract void SetAllowInputToScreensBelow(bool a0);
		public abstract void SetPaused(bool a0);
		public abstract void SetTrackedSoundEventParameter(int a0, string a1, double a2);
		public abstract void StartSoundEvent(string a0);
		public abstract void StartSoundEventWithOptions(string a0, bool a1);
		public abstract int StartTrackedSoundEvent(string a0);
		public abstract int StartTrackedSoundEventWithOptions(string a0, bool a1);
		public abstract void StopTrackedSoundEvent(int a0);
		public abstract void StopTrackedSoundEventImmediately(int a0);
		public abstract void TriggerTrackedSoundEventCue(int a0);
		[Pure] public abstract double GetMovieHeight();
		[Pure] public abstract double GetMovieWidth();
		[Pure] public abstract bool StateMachineRespectsInputWhitelist();
	}
}
