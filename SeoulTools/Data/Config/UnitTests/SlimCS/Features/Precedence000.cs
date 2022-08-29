using static SlimCS;
public static class Test
{
	public static void Main()
	{
		int a = 5; print(a);
		int b = 6; print(b);
		int c = 7; print(c);
		int d = 8; print(d);
		int e = 1; print(e);
		int f = 2147483647; print(e);

		int i = f & 0 | a % 7 + -b * +c - d ^ f / ~e << 7 >> 2; print(i);
	}
}
