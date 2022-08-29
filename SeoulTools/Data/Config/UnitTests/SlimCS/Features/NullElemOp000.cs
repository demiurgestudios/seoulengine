using static SlimCS;
public static class Test
{
	public static void Main()
	{
		int[] value = null;
		int? res = value?[0];
		print(res);
	}
}
