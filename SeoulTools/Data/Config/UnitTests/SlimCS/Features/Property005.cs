using static SlimCS;
public static class Test
{
	static int count; public static int Count { get { return count; } }
	public static void Main()
	{
		count = 100;
		for (var i = 0; i < Count; ++i)
		{
			print(i);
		}
	}
}
