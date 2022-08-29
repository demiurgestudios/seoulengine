using System;

class A
{
	static int Test (int i)
	{
		switch (i) {
		case 1:
			SlimCS.print ("1");
			if (i > 0)
				goto LBL4;
			SlimCS.print ("2");
			break;

		case 3:
			SlimCS.print ("3");
		LBL4:
			SlimCS.print ("4");
			return 0;
		}

		return 1;
	}

	public static int Main ()
	{
		return Test (1);
	}
}
