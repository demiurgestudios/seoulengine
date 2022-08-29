using static SlimCS;

public abstract class A
{
	int m;
	public virtual int Foo { get { return m; } set { m = value; } }
}

public class B : A
{
	int m;
	public override int Foo { get { return m; } set { m = value; } }
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
		a.Foo = 7;
		print(a.Foo);
	}
}
