//
// Tests the resulting value of operator + (U x, E y)
// as well as implicit conversions in the above operator.
//
using System;
class X {
	enum A : int {
		a = 1, b, c
	}

	enum Test : int {
		A = 1,
		B
	}

	public static int Main ()
	{
		// Now try the implicit conversions for underlying types in enum operators
		int b = 1;
		int s = (int) (Test.A + b);

		const int e = A.b + 1 - A.a;

		//
		// Make sure that other operators still work
		if (Test.A != Test.A)
			return 3;
		if (Test.A == Test.B)
			return 4;

		const A e2 = 3 - A.b;
		if (e2 != A.a)
			return 5;

		return 0;
	}
}
