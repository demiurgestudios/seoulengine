using static SlimCS;
using static T2;

static class T2
{
	public static int nameof (string s)
	{
		 print(s);
		return 2;
	}
}

class Test
{
	public static int Main ()
	{
		string s = ""; print(s);
		var v = nameof (s); print(v);
		if (v != 2)
			return 1;

		return 0;
	}
}
