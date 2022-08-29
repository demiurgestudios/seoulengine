using static SlimCS;
public static class Test
{
	public static void Main()
	{
		double? w = null;
		double? x = null;
		double? y = 0.0;
		double? z = 1.0;

		print(w);
		print(x);
		print(y);
		print(z);

		print(x == 0);
		print(x != 0);
		print(x == y);
		print(x != y);
		print(x == w);
		print(x != w);
		print(x == null);
		print(x != null);

		print(0 == y);
		print(0 != y);
		print(z == y);
		print(z != y);
		print(null == y);
		print(null != y);
	}
}
