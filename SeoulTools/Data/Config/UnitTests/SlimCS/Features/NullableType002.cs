using static SlimCS;
public static class Test
{
	public static void Main()
	{
		bool? w = null;
		bool? x = null;
		bool? y = false;
		bool? z = true;

		print(w);
		print(x);
		print(y);
		print(z);

		print(x == false);
		print(x != false);
		print(x == y);
		print(x != y);
		print(x == w);
		print(x != w);
		print(x == null);
		print(x != null);

		print(false == y);
		print(false != y);
		print(z == y);
		print(z != y);
		print(null == y);
		print(null != y);
	}
}
