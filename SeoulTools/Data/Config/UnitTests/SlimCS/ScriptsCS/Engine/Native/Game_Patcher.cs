/*
	Game_Patcher.cs
	AUTO GENERATED - DO NOT MODIFY
	API FOR NATIVE CLASS INSTANCE

	Run GenerateScriptBindings.bat in the Utilities folder to re-generate bindings.

	Copyright (c) Demiurge Studios, Inc.
	
	This source code is licensed under the MIT license.
	Full license details can be found in the LICENSE file
	in the root directory of this source tree.
*/

namespace Native
{
	public abstract class Game_Patcher : UI_Movie
	{
		interface IStatic
		{
			bool StayOnLoadingScreen();
		}

		static IStatic s_udStaticApi;

		static Game_Patcher()
		{
			s_udStaticApi = SlimCS.dyncast<IStatic>(CoreUtilities.DescribeNativeUserData("Game_Patcher"));
		}

		public abstract void OnPatcherStatusFirstRender();

		public static bool StayOnLoadingScreen()
		{
			return s_udStaticApi.StayOnLoadingScreen();
		}
	}
}
