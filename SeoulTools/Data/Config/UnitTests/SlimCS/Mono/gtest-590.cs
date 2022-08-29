using System;

enum E
{
	V
}

class C
{
	public static void Main ()
	{
		byte? foo = 0;
		E e = 0;
		var res = foo - e;
		SlimCS.print (res);
		var res2 = e - foo;
		SlimCS.print (res2);
		res = null;
		res2 = null;
	}
}