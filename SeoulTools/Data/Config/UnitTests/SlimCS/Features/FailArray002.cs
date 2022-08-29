using static SlimCS;
public static class Test
{
	public static void Main()
	{
		var a = new int[3, 3];
		print(a[0][0], a[1][1], a[2][2]);
	}
}
