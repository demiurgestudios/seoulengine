using System;

public class Tests
{
	public virtual ServiceType GetService<ServiceType> (params object[] args) where ServiceType : class
	{
		SlimCS.print ("asdafsdafs");
		return null;
	}

	public static int Main ()
	{
		new Tests ().GetService<Tests> ();
		return 0;
	}
}
