using System;

delegate void D ();

class X {

	public static int Main ()
	{
		X x = new X();
		x.M (10);
		e ();
		SlimCS.print ("J should be 11= {0}", j);
		e ();
                SlimCS.print ("J should be 11= {0}", j);
		x.M (100);
		e ();
                SlimCS.print ("J should be 101= {0}", j);
		if (j != 101)
			return 3;
		SlimCS.print ("OK");
		return 0;
	}

	static int j;
	static D e;
	
	void M (int a)
	{
		SlimCS.print ("A is=" + a);	
		D d = delegate {
			int b;
			b = 1;
			e = delegate {
					SlimCS.print ("IN NESTED DELEGATE: {0}", a);
					j = a + b;
				};
			};
		d ();
	}
	
}
