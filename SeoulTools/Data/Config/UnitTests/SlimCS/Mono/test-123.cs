class X {

	static object get_non_null ()
	{
		return new X ();
	}

	static object get_null ()
	{
		return null;
	}

	public static int Main ()
	{
		int a = 5;
		object o;

		//
		// compile time
		//
		if (!(get_non_null () is object))
			return 1;

		if (get_null () is object)
			return 2;

		if (!(a is object))
			return 3;

		//
		// explicit reference
		//
		if (null is object)
			return 4;

		o = a;
		if (!(o is int))
			return 5;

		object oi = 1;
		if (!(oi is int))
			return 7;

		SlimCS.print ("Is tests pass");
		return 0;
	}
}
