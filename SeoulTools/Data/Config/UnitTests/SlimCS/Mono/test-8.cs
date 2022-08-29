class X {
	public static int Main (string [] args)
	{
		int a, b, c, d;

		a = 10;
		b = 10;
		c = 14;
		d = 14;

		if ((a + b) != 20)
			return 1;
		if ((a + d) != 24)
			return 2;
		if ((c + d) != 28)
			return 3;
		if ((b + c) != 24)
			return 4;

		var tmp = a;
		a++;
		if (tmp != 10)
			return 5;
		++a;
		if (a != 12)
			return 6;
		tmp = b;
		b--;
		if (tmp != 10)
			return 7;
		--b;
		if (b != 8)
			return 8;

		return 0;
	}
}
