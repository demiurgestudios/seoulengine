using System;

// Static array initializers test

enum S
{
	Foo = 5
}

class C
{
	public static int Main ()
	{
		S[] s = new S [] { S.Foo, S.Foo, S.Foo, S.Foo, S.Foo, S.Foo, S.Foo, S.Foo, S.Foo, S.Foo, S.Foo, S.Foo };
		SlimCS.print (s [5]);
			
		return 0;
	}
}
