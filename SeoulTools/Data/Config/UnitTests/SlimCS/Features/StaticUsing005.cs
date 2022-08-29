using static SlimCS;
using static A;
using static B;

class A
{
	public class TestMe
	{
		public TestMe()
		{
			TestMe1();
		}
	}

	public static int TestMe1 ()
	{
		print(0);
		return 0;
	}
}

class B
{
	public static int TestMe2 ()
	{
		print(1);
		return 1;
	}

	public class TestMe1
	{
		public TestMe1()
		{
			TestMe2();
		}
	}
}

class Test
{
	public static void Main ()
	{
		new TestMe1();
	}
}
