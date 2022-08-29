using static SlimCS;

// Test for a regression - complex case
// with interface and class.
public abstract class A
{
	public abstract int Foo { get; set; }
	public void MethodFoo()
	{
		print("Hello from A");
	}
	public abstract int Bar { get; protected set; }
}

public abstract class B : A
{
	public override int Foo { get; set; } = 27;
}

public class C : B
{
	public new int Foo { get; set; } = 33;
	public override int Bar { get; protected set; } = 45;
}

public class D : C
{
	public new int Foo { get; set; } = -13;
	public new int Bar { get; set; } = 25;
}

public static class Test
{
	public static void Main()
	{
		var d = new D();
		print(d.Foo); print(d.Bar);
		d.Foo = 1;
		d.Bar = 2;
		print(d.Foo); print(d.Bar);

		C c = d;
		print(c.Foo); print(c.Bar);
		c.Foo = 3;
		// Inaccessible, intentional: c.Bar = 4;
		print(c.Foo); print(c.Bar);

		B b = c;
		print(b.Foo); print(b.Bar);
		b.Foo = 5;
		// Inaccessible, intentional: b.Bar = 6;
		print(b.Foo); print(b.Bar);

		A a = b;
		print(a.Foo); print(a.Bar);
		a.Foo = 7;
		// Inaccessible, intentional: a.Bar = 8;
		print(a.Foo); print(a.Bar);
	}
}
