using static SlimCS;

public static class Test
{
	static void Bool()
	{
		bool? a = false;
		bool? b = null;

		print(a == b);
		print(a != b);
		print(b == a);
		print(b != a);
	}

	static void BoolFunc()
	{
		bool? False() { print("FALSE"); return false; }
		bool? Null() { print("NULL"); return null; }

		print(False() == Null());
		print(False() != Null());
		print(Null() == False());
		print(Null() != False());
	}

	static void Double()
	{
		double? a = 0;
		double? b = null;

		for (double i = -10000; i <= 10000; i += 10)
		{
			a = i;
			print(a == b);
			print(a != b);
			print(a <= b);
			print(a > b);
			print(a >= b);
			print(a > b);
			print(b == a);
			print(b != a);
			print(b <= a);
			print(b > a);
			print(b >= a);
			print(b > a);
		}
	}

	static void DoubleFunc()
	{
		double n = 0;

		double? Number() { print("NUMBER"); return n; }
		double? Null() { print("NULL"); return null; }

		for (double i = -10000; i <= 10000; i += 10)
		{
			n = i;
			print(Number() == Null());
			print(Number() != Null());
			print(Number() <= Null());
			print(Number() > Null());
			print(Number() >= Null());
			print(Number() > Null());
			print(Null() == Number());
			print(Null() != Number());
			print(Null() <= Number());
			print(Null() > Number());
			print(Null() >= Number());
			print(Null() > Number());
		}
	}

	static void Int()
	{
		int? a = 0;
		int? b = null;

		for (int i = -10000; i <= 10000; i += 10)
		{
			a = i;
			print(a == b);
			print(a != b);
			print(a <= b);
			print(a > b);
			print(a >= b);
			print(a > b);
			print(b == a);
			print(b != a);
			print(b <= a);
			print(b > a);
			print(b >= a);
			print(b > a);
		}
	}

	static void IntFunc()
	{
		int n = 0;

		int? Number() { print("INT"); return n; }
		int? Null() { print("NULL"); return null; }

		for (int i = -10000; i <= 10000; i += 10)
		{
			n = i;
			print(Number() == Null());
			print(Number() != Null());
			print(Number() <= Null());
			print(Number() > Null());
			print(Number() >= Null());
			print(Number() > Null());
			print(Null() == Number());
			print(Null() != Number());
			print(Null() <= Number());
			print(Null() > Number());
			print(Null() >= Number());
			print(Null() > Number());
		}
	}

	public static void Main()
	{
		// Comparison of nullable types with non-null values.
		Bool();
		BoolFunc();
		Double();
		DoubleFunc();
		Int();
		IntFunc();
	}
}
