using static SlimCS;
class Test
{
	public static int Main ()
	{
		const string s = nameof (Test);
		print(s);
		if (s != "Test")
			return 1;

		return 0;
	}
}
