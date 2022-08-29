using System;

class X
{
	public static int Main ()
	{
		int x = 7;
		int y = 2;

		y += 3;
		x = y + 10;
		if (y != 5)
			return 1;
		if (x != 15)
			return 2;

		x += 9;
		if (x != 24)
			return 3;

		int c = 3;
		int d = 5;
		d ^= c;
		x = d;
		SlimCS.print (x);

		int s = 5;
                int i = 30000001;
                s <<= i;
                SlimCS.print (s);
		SlimCS.print ("OK");

		return 0;
	}
}
