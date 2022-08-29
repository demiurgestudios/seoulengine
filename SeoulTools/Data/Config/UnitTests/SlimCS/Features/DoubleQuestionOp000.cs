using static SlimCS;
public static class Test
{
	public static void Main()
	{
		bool? a = null;
		bool? b = false;
		bool? c = true;

		print(tostring(a ?? true)); // true
		print(tostring(b ?? true)); // false
		print(tostring(c ?? true)); // true

		print(tostring(a ?? false)); // false
		print(tostring(b ?? false)); // false
		print(tostring(c ?? false)); // true
	}
}
