using foo = Foo;

namespace Foo {
	class A { }
}

class X {
	static foo::A a = new Foo.A ();
	public static void Main ()
	{
		SlimCS.print (a.GetType ());
	}
}
