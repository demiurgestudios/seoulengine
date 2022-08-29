using static SlimCS;
using System;

class Test
{
	public static void Main ()
	{
		// Make sure that out-of-int range values produce
		// expected integers.
		double i = 2147483648.0;
		print((int)i);
		i = 4294967296.0;
		print((int)i);

		i = 2147483647.0;
		print((int)i);
		++i;
		print((int)i);

		i = 4294967295.0;
		print((int)i);
		++i;
		print((int)i);
	}
}
