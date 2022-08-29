using static SlimCS;
class Test
{
	class A {}

	public static int Main ()
	{
		var (a, b) = (1, 2);
		print(a);
		print(b);
		
		var (c, e, d) = (3, 4, 5);
		print(c);
		print(e);
		print(d);
		
		var (f, g, h) = (a, d, 6);
		print(f);
		print(g);
		print(h);
		
		var (_, var, i) = (7, 8, 9);
		print(var);
		print(i);
		
		return 0;
	}
}
