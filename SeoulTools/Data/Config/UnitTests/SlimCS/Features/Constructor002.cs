using static SlimCS;
public class A
{
	public A()
	{
		print(1);
	}
}

public class B : A
{
	public B()
	{
		print(2);
	}
}

public class C : B
{
	public C(int a)
	{
		print(a);
	}
}

public class D : B
{
	public D()
	{
		print(3);
	}
}

public static class Test
{
	public static void Main()
	{
		new C(3);
		new D();
	}
}
