/*
	TimeInterval.cs
	AUTO GENERATED - DO NOT MODIFY
	API FOR NATIVE CLASS INSTANCE

	Run GenerateScriptBindings.bat in the Utilities folder to re-generate bindings.

	Copyright (c) 2018-2022 Demiurge Studios Inc.  All rights reserved.
*/

using System.Diagnostics.Contracts;

namespace Native
{
	public abstract class TimeInterval
	{
		interface IStatic
		{
			TimeInterval FromMicroseconds(int a0);
			TimeInterval FromSecondsInt64(int a0);
			TimeInterval FromSecondsDouble(double a0);
			TimeInterval FromHoursInt64(int a0);
			TimeInterval FromDaysInt64(int a0);
		}

		static IStatic s_udStaticApi;

		static TimeInterval()
		{
			s_udStaticApi = SlimCS.dyncast<IStatic>(CoreUtilities.DescribeNativeUserData("TimeInterval"));
		}

		public static extern TimeInterval operator+(TimeInterval a0, TimeInterval a1);
		public static extern TimeInterval operator-(TimeInterval a0, TimeInterval a1);
		public static extern bool operator<(TimeInterval a0, TimeInterval a1);
		public static extern bool operator>(TimeInterval a0, TimeInterval a1);
		public static extern bool operator<=(TimeInterval a0, TimeInterval a1);
		public static extern bool operator>=(TimeInterval a0, TimeInterval a1);
		public static extern TimeInterval operator-(TimeInterval a0);

		public static TimeInterval FromMicroseconds(int a0)
		{
			return s_udStaticApi.FromMicroseconds(a0);
		}

		public static TimeInterval FromSecondsInt64(int a0)
		{
			return s_udStaticApi.FromSecondsInt64(a0);
		}

		public static TimeInterval FromSecondsDouble(double a0)
		{
			return s_udStaticApi.FromSecondsDouble(a0);
		}

		public static TimeInterval FromHoursInt64(int a0)
		{
			return s_udStaticApi.FromHoursInt64(a0);
		}

		public static TimeInterval FromDaysInt64(int a0)
		{
			return s_udStaticApi.FromDaysInt64(a0);
		}

		[Pure] public abstract int GetSeconds();
		[Pure] public abstract double GetSecondsAsDouble();
		[Pure] public abstract bool IsZero();
	}
}
