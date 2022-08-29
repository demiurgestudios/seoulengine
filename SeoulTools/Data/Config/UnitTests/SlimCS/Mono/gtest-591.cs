// Compiler options: -r:gtest-591-lib.dll

using System;

namespace A0.B0.C0.D0
{
	public class D<T>
	{
	}
}

public class E
{
	public A0.B0.C0.D0.D<int> F;
	public static void Main ()
	{
		var e = new E ();
		SlimCS.print (e.F);
	}
}
