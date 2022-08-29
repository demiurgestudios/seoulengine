using static SlimCS;

// Test for a regression where properties
// in templated types would not be correctly/consistently
// recognzied as auto properties.
public class A<T>
{
	public int Foo { get; set; }
}

public static class Test
{
	public static void Main()
	{
		var a = new A<int>();
		print(a.Foo);
		a.Foo = 5;
		print(a.Foo);
	}
}
