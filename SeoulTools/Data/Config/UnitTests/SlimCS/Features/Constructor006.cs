// Regression for inline initialization handling and
// invocation of parents in light of virtual methods.
using static SlimCS;

public abstract class A
{
	public A()
	{
		DoIt();
	}

	public abstract void DoIt();
}

public class B : A
{
	public int Check = 5;

	public B()
	{
		Check = 7;
		print("B: " + Check.ToString());
	}

	public override void DoIt()
	{
		print("A: " + Check.ToString());
		Check = 2;
	}
}

public static class Test
{
	public static void Main()
	{
		new B();
	}
}
