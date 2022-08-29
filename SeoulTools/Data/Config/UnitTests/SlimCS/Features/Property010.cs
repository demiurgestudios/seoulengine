using static SlimCS;

// Test for a regression - auto properties
// that override an abstract must emit
// a synthesized get/set pair.
public abstract class A
{
	public abstract int Foo { get; set; }
}

public class B : A
{
	public override int Foo { get; set; }
}

public static class Test
{
	public static void Main()
	{
		var b = new B();
		print(b.Foo);
		b.Foo = 5;
		print(b.Foo);

		A a = b;
		print(a.Foo);
		a.Foo = 3;
		print(a.Foo);
	}
}
