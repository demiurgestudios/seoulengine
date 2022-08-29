using static SlimCS;
public static class Test
{
	public static void Main()
	{
		int[] value = new int[1] { 1 };
		int? res = value?[0];
		print(res);
	}
}
