using System;

public class Foo<T>
	where T : A
{
	public void Test (T t)
	{
		SlimCS.print (t);
		SlimCS.print (t.GetType ());
		t.Hello ();
	}
}

public class A
{
	public void Hello ()
	{
		SlimCS.print ("Hello World");
	}
}

public class B : A
{
}

class X
{
	public static void Main ()
	{
		Foo<B> foo = new Foo<B> ();
		foo.Test (new B ());
	}
}
