using static SlimCS;
static class Test
{
	static void Foo(this object o)
	{
		print(o);
	}

	public static void Main()
	{
		const object o = null;
		print(o);
		o.Foo();
		print("HERE");
	}
}
