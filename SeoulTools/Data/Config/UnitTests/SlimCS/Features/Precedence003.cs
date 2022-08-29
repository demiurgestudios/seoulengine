using static SlimCS;
public static class Test
{
	public static void Main()
	{
		double? a = null;
		double? b = 0;
		bool c = false;
		bool d = true;
		
		print(@string.lower($"{a ?? 4}{b ?? 5}{c && true}{d && true}{c || true}{d || true}")); // TODO: lower is to handle "True" in C# vs. "true" in Lua.
		print(@string.lower($"{b == 0}{b != 0}{b >= 0}{b <= 0}{b < 0}{b > 0}")); // TODO: lower is to handle "True" in C# vs. "true" in Lua.
		print($"{b / 1}{b + 1}{b - 1}{b * 1}");
	}
}
