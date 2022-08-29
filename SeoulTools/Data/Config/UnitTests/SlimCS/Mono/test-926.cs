class Program
{
	public static void Main ()
	{
		SlimCS.print("Hi");
	}
}

public class MyClass
{
	protected internal virtual int this[int i]
	{
		protected get
		{
			return 2;
		}
		set
		{
		}
	}
}

public class Derived : MyClass
{
	protected internal override int this[int i]
	{
		protected get
		{
			return 4;
		}
	}
}