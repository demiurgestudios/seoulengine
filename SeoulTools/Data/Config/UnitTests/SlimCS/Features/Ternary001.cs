using static SlimCS;
public static class Test
{
	const int Zero = 0;
	const string ksOne = "One";
	const string ksTwo = "Two";
	const string ksThree = "Three";

	static string GetString(int i, bool b)
	{
		string variation = Zero == i ?
			b ? ksOne : ksTwo :
			b ? "" : ksThree;

		return variation;
	}

	public static void Main()
	{
		print(GetString(0, false));
		print(GetString(0, true));
		print(GetString(1, false));
		print(GetString(1, true));
	}
}
