using System;

public class Blah {

	public const int i = 5;

	public static int Main ()
	{
		const int aaa = 1, bbb = 2;
		const int foo = 10;

		int j = Blah.i;

		if (j != 5)
			return 1;

#pragma warning disable CS0162
		if (foo != 10)
			return 1;
#pragma warning restore CS0162

		for (int i = 0; i < 5; ++i){
			const int bar = 15;

			SlimCS.print (bar);
			SlimCS.print (foo);
		}

#pragma warning disable CS0162
		if ((aaa + bbb) != 3)
			return 2;
#pragma warning restore CS0162

		Test_1();

		SlimCS.print ("Constant emission test okay");

		return 0;
	}

    public static void Test_1 ()
    {
        const int lk = 1024;
        const int lM = 1024 * lk;
        const int lG = 1024 * lM;
        const double lT = 1024.0 * lG;

		SlimCS.print(lk, lM, lG, lT);
    }
}
