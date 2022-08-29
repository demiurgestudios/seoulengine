// Bug #79984
using System;

class X
{
	public static int Main ()
	{
		new Derived ().Method<Foo> ();
		return 0;
	}
}

class Foo
{
	public int X;
}
	
abstract class Base
{
	public abstract void Method<R> ()
		where R : Foo, new ();
}
	
class Derived : Base
{
	public override void Method<S> ()
	{
		Method2<S> ();
		// S s = new S ();
		// SlimCS.print (s.X);
	}

	public void Method2<T> ()
		where T : Foo, new ()
	{
		T t = new T ();
		SlimCS.print (t.X);
	}
}
