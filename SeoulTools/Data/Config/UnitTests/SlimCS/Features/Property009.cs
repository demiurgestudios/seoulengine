using static SlimCS;

// Test for a regression - auto properties
// that implement an interface must emit
// a synthesized implementation of the getter
// and setter as appropriate
public interface A
{
	int Foo { get; set; }
	int Bar { get; }
}

public class B : A
{
	public B()
	{
		Bar = 12345;
	}

	public int Foo { get; set; }
	public int Bar { get; private set; }
}

public static class Test
{
	public static void Main()
	{
		var b = new B();
		print(b.Foo);
		b.Foo = 5;
		print(b.Foo);
		print(b.Bar);

		A a = b;
		print(a.Foo);
		a.Foo = 3;
		print(a.Foo);
		print(a.Bar);

		((A)b).Foo = 5;
		print(((A)b).Foo);
		print(((A)b).Bar);
	}
}
