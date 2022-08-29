using static SlimCS;

// Test for a regression - complex case
// with interface and class.
public abstract class A
{
	public abstract int Foo { get; set; }
}

public class B : A
{
	public override int Foo { get; set; } = 27;
}

public class C : B
{
	public new int Foo { get; set; } = 45;
}

public static class Test
{
	public static void Main()
	{
		var c = new C();
		print(c.Foo);
		c.Foo = 5;
		print(c.Foo);

		B b = c;
		print(b.Foo);
		b.Foo = 3;
		print(b.Foo);

		A a = b;
		print(a.Foo);
		a.Foo = 7;
		print(a.Foo);
	}
}
