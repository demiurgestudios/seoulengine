using static SlimCS;
public static class Test
{
	public static void Main()
	{
		S s = new S();
		D d = s.Foo;
		d();
	}
}

public delegate void D();

public sealed class S
{
	public void Foo(int i)
	{
		print("1");
	}
}

public static class Extension
{
	public static void Foo(this S s)
	{
		print("2");
	}
}
