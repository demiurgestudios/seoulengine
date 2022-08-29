using System;

public delegate void FooDelegate ();

public class X {
	public static readonly FooDelegate Print = delegate {
		SlimCS.print ("delegate!");
        };

	public static void Main ()
	{
		Print ();
	}
}
