// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

namespace System
{
	public struct ValueTuple
	{
		public static ValueTuple Create() =>
			new ValueTuple();
		public static ValueTuple<T1> Create<T1>(T1 item1) =>
			new ValueTuple<T1>(item1);
		public static ValueTuple<T1, T2> Create<T1, T2>(T1 item1, T2 item2) =>
			new ValueTuple<T1, T2>(item1, item2);
		public static ValueTuple<T1, T2, T3> Create<T1, T2, T3>(T1 item1, T2 item2, T3 item3) =>
			new ValueTuple<T1, T2, T3>(item1, item2, item3);
		public static ValueTuple<T1, T2, T3, T4> Create<T1, T2, T3, T4>(T1 item1, T2 item2, T3 item3, T4 item4) =>
			new ValueTuple<T1, T2, T3, T4>(item1, item2, item3, item4);
		public static ValueTuple<T1, T2, T3, T4, T5> Create<T1, T2, T3, T4, T5>(T1 item1, T2 item2, T3 item3, T4 item4, T5 item5) =>
			new ValueTuple<T1, T2, T3, T4, T5>(item1, item2, item3, item4, item5);
		public static ValueTuple<T1, T2, T3, T4, T5, T6> Create<T1, T2, T3, T4, T5, T6>(T1 item1, T2 item2, T3 item3, T4 item4, T5 item5, T6 item6) =>
			new ValueTuple<T1, T2, T3, T4, T5, T6>(item1, item2, item3, item4, item5, item6);
		public static ValueTuple<T1, T2, T3, T4, T5, T6, T7> Create<T1, T2, T3, T4, T5, T6, T7>(T1 item1, T2 item2, T3 item3, T4 item4, T5 item5, T6 item6, T7 item7) =>
			new ValueTuple<T1, T2, T3, T4, T5, T6, T7>(item1, item2, item3, item4, item5, item6, item7);
		public static ValueTuple<T1, T2, T3, T4, T5, T6, T7, T8> Create<T1, T2, T3, T4, T5, T6, T7, T8>(T1 item1, T2 item2, T3 item3, T4 item4, T5 item5, T6 item6, T7 item7, T8 item8) =>
			new ValueTuple<T1, T2, T3, T4, T5, T6, T7, T8>(item1, item2, item3, item4, item5, item6, item7, item8);
	}

	public struct ValueTuple<T1>
	{
		public T1 Item1;
		public ValueTuple(T1 item1)
		{
			Item1 = item1;
		}
	}

	public struct ValueTuple<T1, T2>
	{
		public T1 Item1;
		public T2 Item2;
		public ValueTuple(T1 item1, T2 item2)
		{
			Item1 = item1;
			Item2 = item2;
		}
	}

	public struct ValueTuple<T1, T2, T3>
	{
		public T1 Item1;
		public T2 Item2;
		public T3 Item3;
		public ValueTuple(T1 item1, T2 item2, T3 item3)
		{
			Item1 = item1;
			Item2 = item2;
			Item3 = item3;
		}
	}

	public struct ValueTuple<T1, T2, T3, T4>
	{
		public T1 Item1;
		public T2 Item2;
		public T3 Item3;
		public T4 Item4;
		public ValueTuple(T1 item1, T2 item2, T3 item3, T4 item4)
		{
			Item1 = item1;
			Item2 = item2;
			Item3 = item3;
			Item4 = item4;
		}
	}

	public struct ValueTuple<T1, T2, T3, T4, T5>
	{
		public T1 Item1;
		public T2 Item2;
		public T3 Item3;
		public T4 Item4;
		public T5 Item5;
		public ValueTuple(T1 item1, T2 item2, T3 item3, T4 item4, T5 item5)
		{
			Item1 = item1;
			Item2 = item2;
			Item3 = item3;
			Item4 = item4;
			Item5 = item5;
		}
	}

	public struct ValueTuple<T1, T2, T3, T4, T5, T6>
	{
		public T1 Item1;
		public T2 Item2;
		public T3 Item3;
		public T4 Item4;
		public T5 Item5;
		public T6 Item6;
		public ValueTuple(T1 item1, T2 item2, T3 item3, T4 item4, T5 item5, T6 item6)
		{
			Item1 = item1;
			Item2 = item2;
			Item3 = item3;
			Item4 = item4;
			Item5 = item5;
			Item6 = item6;
		}
	}

	public struct ValueTuple<T1, T2, T3, T4, T5, T6, T7>
	{
		public T1 Item1;
		public T2 Item2;
		public T3 Item3;
		public T4 Item4;
		public T5 Item5;
		public T6 Item6;
		public T7 Item7;
		public ValueTuple(T1 item1, T2 item2, T3 item3, T4 item4, T5 item5, T6 item6, T7 item7)
		{
			Item1 = item1;
			Item2 = item2;
			Item3 = item3;
			Item4 = item4;
			Item5 = item5;
			Item6 = item6;
			Item7 = item7;
		}
	}

	public struct ValueTuple<T1, T2, T3, T4, T5, T6, T7, T8>
	{
		public T1 Item1;
		public T2 Item2;
		public T3 Item3;
		public T4 Item4;
		public T5 Item5;
		public T6 Item6;
		public T7 Item7;
		public T8 Item8;
		public ValueTuple(T1 item1, T2 item2, T3 item3, T4 item4, T5 item5, T6 item6, T7 item7, T8 item8)
		{
			Item1 = item1;
			Item2 = item2;
			Item3 = item3;
			Item4 = item4;
			Item5 = item5;
			Item6 = item6;
			Item7 = item7;
			Item8 = item8;
		}
	}
}
