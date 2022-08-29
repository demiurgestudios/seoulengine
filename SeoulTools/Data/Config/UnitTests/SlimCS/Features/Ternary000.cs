using static SlimCS;
public static class Test
{
	class A
	{
		public A()
		{
			print("At A");
		}
	}

	class B : A
	{
		public B()
		{
			print("At B");
		}
	}

	public static void Main()
	{
		bool bTrue = true;
		print(bTrue ? new A() : new B());
		print(!bTrue ? new A() : new B());
		print(bTrue ? null : new B());
		print(!bTrue ? null : new B());
		print(bTrue ? new A() : null);
		print(!bTrue ? new A() : null);
	}
}
