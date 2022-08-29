using static SlimCS;
public class A
{
	string m_s = null;
}

public class B : A
{
	public B()
	{
		print("B0");
	}

	public B(bool b)
	{
		print("B1");
	}

	public B(int i)
	{
		print("B2");
	}
}
public class C : B
{
	public C()
		: this(true)
	{
		print("C0");
	}

	public C(bool b)
		: base(b)
	{
		print("C1");
	}

	public C(int i)
		: base(i)
	{
		print("C2");
	}
}
public class D : C
{
}
public class E : D
{
	public E()
	{
		print("E0");
	}

	public E(int i)
	{
		print("E1");
	}
}
public class F : E
{
	public F()
		: this(27)
	{
		print("F0");
	}

	public F(int i)
		: base(i)
	{
		print("F1");
	}
}

public static class Test
{
	public static void Main()
	{
		new C(false);
		new C(true);
		new C(0);
		new C(1);
		new D();
		new E();
		new E(39);
		new F();
		new F(3999);
	}
}
