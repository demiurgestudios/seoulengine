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

	static int j;
	static D e;
	
	void M ()
	{
		int l = 100;

		D d = delegate {
			int b;
			b = 1;
			e = delegate {
					j = l + b;
				};
			};
		d ();
	}
	
}
