using System;

class X
{
	public static void Main ()
	{
		int a;
		goto X;
	A:
		SlimCS.print (a);
		goto Y;
	X:
		a = 1;
		goto A;
	Y:
		return;
	}
}