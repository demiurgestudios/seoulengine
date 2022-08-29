using static SlimCS;
using Extra;

namespace Extra
{
	static class S
	{
		public static int Prefix (this string s, string prefix)
		{
			print(0);
			return 1;
		}
	}
}

static class SimpleTest
{
	public static int Prefix (this string s, string prefix, bool bold)
	{
		print(2);
		return 0;
	}
}

public static class Test
{
	public static int Main()
	{
		var res = "foo".Prefix ("1");
		print(res);

		if (res != 1)
			return 1;

		return 0;
	}
}
