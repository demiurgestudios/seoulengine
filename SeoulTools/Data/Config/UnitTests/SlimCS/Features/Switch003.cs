using static SlimCS;
public static class Test
{
	static bool ok;

	public static void TestIt ()
	{
		int n=0;
		while (true) {
			print(n);
			++n;
			print(n);
			if (n==5)
				break;
			print(n);
			switch (0) {
			case 0: print(n); break;
			}
		}
		ok = true;
	}

	public static int Main ()
	{
		TestIt ();
		return ok ? 0 : 1;
	}
}
