using globalA = A;

class A { }

class X {
	class A { }
	public static void Main ()
	{
		global::A a = new globalA ();
		SlimCS.print (a.GetType ());
	}
}
