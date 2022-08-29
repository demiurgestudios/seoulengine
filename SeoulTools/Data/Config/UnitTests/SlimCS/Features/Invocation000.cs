using static SlimCS;
public static class Test
{
	class A
	{
		string b = "1";

		public A()
		{
			DoIt("2");
		}

		public void DoIt(string s)
		{
			print(b);
			print(s);
			print(DoIt2(s));
			print(DoIt2("3"));
			A.DoIt3(this, "4");
			print(b);
		}

		public string DoIt2(string s)
		{
			return s;
		}

		public static void DoIt3(A a, string s)
		{
			a.b = s;
		}
	}

	public static void Main()
	{
		new A();
	}
}
