// Regression for inline initialization handling and
// invocation of parents in light of virtual methods.
using static SlimCS;

public class A
{
	public double x { get { return 0.0; } }
	public double y { get { return 1.0; } }
}

public class B : A
{
	public double z { get; set; }
}

public class C : B
{
	public double w { get { return 3.0; } }
	public double a { get; set; }
}

public static class Test
{
	public static void Main()
	{
		var a = new A();
		print(a.x);
		print(a.y);

		var b = new B();
		print(b.x);
		print(b.y);
		print(b.z);
		
		var c = new C();
		print(c.x);
		print(c.y);
		print(c.z);
		print(c.w);
		print(c.a);
	}
}
