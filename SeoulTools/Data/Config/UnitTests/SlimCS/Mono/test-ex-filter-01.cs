using System;

class X
{
	public static int Main ()
	{
		int x = 4;
		try {
			throw null;
		} catch (NullReferenceException) when (x > 0) {
			SlimCS.print ("catch");
			return 0;
		}
	}
}