using static SlimCS;
public class A
{
	public A()
	{
		print(1);
	}
	public A(int i)
	{
		print(i);
	}
	public A(int i, int j)
	{
		print(i, j);
	}
}

public class B : A
{
	public B()
	{
		print(2);
	}
	public B(int i)
		: base(i, i+5)
	{
		print(i);
	}
}

public class C : B
{
	public C(int a)
		: base(a)
	{
		print(a);
	}
}

public class D : B
{
	public D()
		: base(7)
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
