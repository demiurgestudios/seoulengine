using static SlimCS;

public static class Test
{
	public class A
	{
		public A()
		{
			print(GetType().Name);
			print(this.GetType().Name);
		}
	}

	public static void Main ()
	{
		var a = new A();
		print(a.GetType().Name);
		var b = new B();
		print(b.GetType().Name);
		var c = new C();
		print(c.GetType().Name);
	}
}

public class B
{
	public B()
	{
		print(GetType().Name);
		print(this.GetType().Name);
	}
}

public class C : Test.A
{
	public C()
	{
		print(GetType().Name);
		print(this.GetType().Name);
	}
}
