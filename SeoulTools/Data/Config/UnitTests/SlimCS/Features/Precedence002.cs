using static SlimCS;
public static class Test
{
	public static void Main()
	{
		double? a = null;
		double? b = 0;
		bool c = false;
		bool d = true;
		
		print(concat(a ?? 4, b ?? 5, c && true, d && true, c || true, d || true));
		print(concat(b == 0, b != 0, b >= 0, b <= 0, b < 0, b > 0));
		print(concat(b / 1, b + 1, b - 1, b * 1));
	}
}
