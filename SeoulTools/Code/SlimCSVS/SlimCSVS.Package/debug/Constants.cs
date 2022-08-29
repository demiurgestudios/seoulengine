// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;

namespace SlimCSVS.Package.debug
{
	public static class Constants
	{
		public static readonly TimeSpan kQueryTimeout = new TimeSpan(0, 0, 1);

		public const string ksEngineGuid = "8D9184DB-1FDE-4F59-8EF4-3AA1C26FA333";
		public static readonly Guid kEngineGuid = new Guid(ksEngineGuid);
		public const string ksEngineName = "SlimCS";
		public const string ksProgramName = "SlimCS Process";

		public const string ksFactoryGuid = "A21CA60D-51B7-4121-A8C9-1BD003E4129C";
		public const string ksPackageGuid = "AE09C581-EF30-4D55-8A5D-D98313CA5399";

		public static class LanguageGuids
		{
			public static readonly Guid None = new Guid("00000000-0000-0000-0000-000000000000");

			public static readonly Guid Basic = new Guid("3a12d0b8-c26c-11d0-b442-00a0244a1dd2");
			public static readonly Guid C = new Guid("63a08714-fc37-11d2-904c-00c04fa302a1");
			public static readonly Guid CIL = new Guid("af046cd3-d0e1-11d2-977c-00a0c9b4d50c");
			public static readonly Guid Cobol = new Guid("af046cd1-d0e1-11d2-977c-00a0c9b4d50c");
			public static readonly Guid Cpp = new Guid("3a12d0b7-c26c-11d0-b442-00a0244a1dd2");
			public static readonly Guid CSharp = new Guid("3f5162f8-07c6-11d3-9053-00c04fa302a1");
			public static readonly Guid Java = new Guid("3a12d0b4-c26c-11d0-b442-00a0244a1dd2");
			public static readonly Guid JScript = new Guid("3a12d0b6-c26c-11d0-b442-00a0244a1dd2");
			public static readonly Guid MCpp = new Guid("4b35fde8-07c6-11d3-9053-00c04fa302a1");
			public static readonly Guid Pascal = new Guid("af046cd2-d0e1-11d2-977c-00a0c9b4d50c");
			public static readonly Guid SMC = new Guid("0d9b9f7b-6611-11d3-bd2a-0000f80849bd");
		}

		public static class LanguageNames
		{
			public const string CSharp = "C#";
		}
	}
}
