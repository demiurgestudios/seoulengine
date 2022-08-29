using static SlimCS;
public static class Test
{
	static int TestImpl(int i)
	{
		switch (i) {
		case 1:
			print("1");
			if (i > 0) goto LBL4;
			print("2");
			break;

		case 3:
			print("3");
		LBL4:
			print("4");
			return 0;
		}

		return 1;
	}

	public static int Main()
	{
		return TestImpl(1);
	}
}
