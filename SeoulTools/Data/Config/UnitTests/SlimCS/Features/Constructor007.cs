// Regression for inline initialization handling and
// invocation of parents in light of virtual methods.
using static SlimCS;

public sealed class A
{
	public int Check = 5;

	public A(bool b = false)
	{
		if (b)
		{
			Check = 7;
			print(Check);
		}
		else
		{
			print(Check);
		}
	}
}

public static class Test
{
	public static void Main()
	{
		var a = new A();
		print(a.Check);
		a = new A(true);
		print(a.Check);
	}
}
