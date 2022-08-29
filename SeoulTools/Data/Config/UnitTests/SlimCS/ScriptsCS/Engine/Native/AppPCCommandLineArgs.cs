/*
	AppPCCommandLineArgs.cs
	AUTO GENERATED - DO NOT MODIFY
	API FOR NATIVE CLASS INSTANCE

	Run GenerateScriptBindings.bat in the Utilities folder to re-generate bindings.

	Copyright (c) Demiurge Studios, Inc.
	
	This source code is licensed under the MIT license.
	Full license details can be found in the LICENSE file
	in the root directory of this source tree.
*/

#if SEOUL_PLATFORM_WINDOWS
namespace Native
{
	public abstract class AppPCCommandLineArgs
	{
		interface IStatic
		{
			bool AcceptGDPR();
			bool D3D11Headless();
			bool EnableWarnings();
			bool FMODHeadless();
			bool FreeDiskAccess();
			string RunScript();
			bool DownloadablePackageFileSystemsEnabled();
			bool PersistentTest();
			CommandLineArgWrapper_String_ RunUnitTests();
			string RunAutomatedTest();
			string GenerateScriptBindings();
			string TrainerFile();
			string UIManager();
			bool VerboseMemoryTooling();
			string VideoDir();
		}

		static IStatic s_udStaticApi;

		static AppPCCommandLineArgs()
		{
			s_udStaticApi = SlimCS.dyncast<IStatic>(CoreUtilities.DescribeNativeUserData("AppPCCommandLineArgs"));
		}

		public static bool AcceptGDPR()
		{
			return s_udStaticApi.AcceptGDPR();
		}

		public static bool D3D11Headless()
		{
			return s_udStaticApi.D3D11Headless();
		}

		public static bool EnableWarnings()
		{
			return s_udStaticApi.EnableWarnings();
		}

		public static bool FMODHeadless()
		{
			return s_udStaticApi.FMODHeadless();
		}

		public static bool FreeDiskAccess()
		{
			return s_udStaticApi.FreeDiskAccess();
		}

		public static string RunScript()
		{
			return s_udStaticApi.RunScript();
		}

		public static bool DownloadablePackageFileSystemsEnabled()
		{
			return s_udStaticApi.DownloadablePackageFileSystemsEnabled();
		}

		public static bool PersistentTest()
		{
			return s_udStaticApi.PersistentTest();
		}

		public static CommandLineArgWrapper_String_ RunUnitTests()
		{
			return s_udStaticApi.RunUnitTests();
		}

		public static string RunAutomatedTest()
		{
			return s_udStaticApi.RunAutomatedTest();
		}

		public static string GenerateScriptBindings()
		{
			return s_udStaticApi.GenerateScriptBindings();
		}

		public static string TrainerFile()
		{
			return s_udStaticApi.TrainerFile();
		}

		public static string UIManager()
		{
			return s_udStaticApi.UIManager();
		}

		public static bool VerboseMemoryTooling()
		{
			return s_udStaticApi.VerboseMemoryTooling();
		}

		public static string VideoDir()
		{
			return s_udStaticApi.VideoDir();
		}
	}
}
#endif
