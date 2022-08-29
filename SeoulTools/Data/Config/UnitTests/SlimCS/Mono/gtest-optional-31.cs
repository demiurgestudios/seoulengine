using System;

class Test
{
	public static int M (bool b = false)
	{
		SlimCS.print ("PASS");
		return 0;
	}

	public static int M (params string[] args)
	{
		SlimCS.print ("FAIL");
		return 1;
	}
	
	public static int Main ()
	{
		return M ();
	}
}
