using System;

class X
{
	public static void Main ()
	{
		int i;
		if (true) {
			i = 3;
		}

		SlimCS.print (i);

		int i2;
		if (false) {
			throw new ApplicationException ();
		} else {
			i2 = 4;
		}

		SlimCS.print (i2);
	}
}