using static SlimCS;
using System;

class Test
{
	public static void Main ()
	{
		// Make sure an appropriate exception is raised.
		try
		{
			object o = null;
			print((int)o);
		}
		catch (Exception e)
		{
			print(e.Message);
		}
	}
}
