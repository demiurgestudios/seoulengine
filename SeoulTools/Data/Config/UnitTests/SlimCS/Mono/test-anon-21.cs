//
// Nested anonymous methods and capturing of variables test
//
using System;

delegate void D ();

class X {

	public static int Main ()
	{
		X x = new X();
		x.M ();
		e ();
                SlimCS.print ("J should be 101= {0}", j);
		if (j != 101)
			return 3;
		SlimCS.print ("OK");
		return 0;
	}

	static int j = 0;
	static D e;
	
	void M ()
	{
		int l = 100;

		D d = delegate {
			int b;
			b = 1;
			SlimCS.print ("Inside d");
			e = delegate {
					SlimCS.print ("Inside e");
					j = l + b;
					SlimCS.print ("j={0} l={1} b={2}", j, l, b);
			};
		};
		SlimCS.print ("Calling d");
		d ();
	}
	
}
