using static SlimCS;
public static class Test
{
	public class A
	{
	}

	public class B
	{
	}

	public class C : B
	{
	}

	public class D : B
	{
	}

	public static void Main()
	{
		object o = null;

		o = System.Activator.CreateInstance(typeof(A));
		print(o);
		o = System.Activator.CreateInstance(o.GetType());
		print(o);

		o = System.Activator.CreateInstance(typeof(B));
		print(o);
		o = System.Activator.CreateInstance(o.GetType());
		print(o);

		o = System.Activator.CreateInstance(typeof(C));
		print(o);
		o = System.Activator.CreateInstance(o.GetType());
		print(o);

		o = System.Activator.CreateInstance(typeof(D));
		print(o);
		o = System.Activator.CreateInstance(o.GetType());
		print(o);
	}
}
