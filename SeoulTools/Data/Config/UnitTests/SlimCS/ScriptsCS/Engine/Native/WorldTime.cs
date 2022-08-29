/*
	WorldTime.cs
	AUTO GENERATED - DO NOT MODIFY
	API FOR NATIVE CLASS INSTANCE

	Run GenerateScriptBindings.bat in the Utilities folder to re-generate bindings.

	Copyright (c) Demiurge Studios, Inc.
	
	This source code is licensed under the MIT license.
	Full license details can be found in the LICENSE file
	in the root directory of this source tree.
*/

using System.Diagnostics.Contracts;

namespace Native
{
	public abstract class WorldTime
	{
		interface IStatic
		{
			WorldTime FromMicroseconds(double iMicroseconds);
			WorldTime FromSecondsInt64(double iSeconds);
			WorldTime FromSecondsDouble(double fSeconds);
			WorldTime FromYearMonthDayUTC(double iYear, double uMonth, double iDay);
			int IsCurrentlyDST();
			WorldTime ParseISO8601DateTime(string sDateTime);
			WorldTime GetUTCTime();
		}

		static IStatic s_udStaticApi;

		static WorldTime()
		{
			s_udStaticApi = SlimCS.dyncast<IStatic>(CoreUtilities.DescribeNativeUserData("WorldTime"));
		}

		public static extern WorldTime operator+(WorldTime a0, TimeInterval a1);
		public static extern TimeInterval operator-(WorldTime a0, WorldTime a1);
		public static extern bool operator<(WorldTime a0, WorldTime a1);
		public static extern bool operator>(WorldTime a0, WorldTime a1);
		public static extern bool operator<=(WorldTime a0, WorldTime a1);
		public static extern bool operator>=(WorldTime a0, WorldTime a1);

		public static WorldTime FromMicroseconds(double iMicroseconds)
		{
			return s_udStaticApi.FromMicroseconds(iMicroseconds);
		}

		public static WorldTime FromSecondsInt64(double iSeconds)
		{
			return s_udStaticApi.FromSecondsInt64(iSeconds);
		}

		public static WorldTime FromSecondsDouble(double fSeconds)
		{
			return s_udStaticApi.FromSecondsDouble(fSeconds);
		}

		public static WorldTime FromYearMonthDayUTC(double iYear, double uMonth, double iDay)
		{
			return s_udStaticApi.FromYearMonthDayUTC(iYear, uMonth, iDay);
		}

		public static int IsCurrentlyDST()
		{
			return s_udStaticApi.IsCurrentlyDST();
		}

		[Pure] public abstract int GetSeconds();
		[Pure] public abstract double GetSecondsAsDouble();
		[Pure] public abstract bool IsZero();

		public static WorldTime ParseISO8601DateTime(string sDateTime)
		{
			return s_udStaticApi.ParseISO8601DateTime(sDateTime);
		}

		[Pure] public abstract string ToISO8601DateTimeUTCString();
		public abstract void AddSeconds(int a0);
		public abstract void AddMinutes(int a0);
		public abstract void AddHours(int a0);
		public abstract void AddDays(int a0);
		[Pure] public abstract WorldTime GetDayStartTime(double iOffsetHoursUTC);
		[Pure] public abstract WorldTime GetNextDayStartTime(double iOffsetHoursUTC);

		public static WorldTime GetUTCTime()
		{
			return s_udStaticApi.GetUTCTime();
		}
	}
}
