/*
	EngineCommandLineArgs.cs
	AUTO GENERATED - DO NOT MODIFY
	API FOR NATIVE CLASS INSTANCE

	Run GenerateScriptBindings.bat in the Utilities folder to re-generate bindings.

	Copyright (c) 2018-2022 Demiurge Studios Inc.  All rights reserved.
*/

namespace Native
{
	public abstract class EngineCommandLineArgs
	{
		interface IStatic
		{
			string AutomationScript();
			string MoriartyServer();
			bool NoCooking();
			bool PreferUsePackageFiles();
		}

		static IStatic s_udStaticApi;

		static EngineCommandLineArgs()
		{
			s_udStaticApi = SlimCS.dyncast<IStatic>(CoreUtilities.DescribeNativeUserData("EngineCommandLineArgs"));
		}

		public static string AutomationScript()
		{
			return s_udStaticApi.AutomationScript();
		}

		public static string MoriartyServer()
		{
			return s_udStaticApi.MoriartyServer();
		}

		public static bool NoCooking()
		{
			return s_udStaticApi.NoCooking();
		}

		public static bool PreferUsePackageFiles()
		{
			return s_udStaticApi.PreferUsePackageFiles();
		}
	}
}
