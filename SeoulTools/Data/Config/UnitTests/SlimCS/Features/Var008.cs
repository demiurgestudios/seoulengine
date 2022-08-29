using static SlimCS;
class Test
{
	class A {}

	public static int Main ()
	{
		var list = new A ();
		print(list);
		var a = list as object;
		print(a);
		object o = a;
		print(o);
		return 0;
	}
}
