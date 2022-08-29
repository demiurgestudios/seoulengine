using static SlimCS;
public static class Test
{
	static void add()
	{
		for (int i = -10; i <= 10; ++i)
		{
			for (int j = 10; j >= -10; --j)
			{
				int a = i, b = j;
				print(a + b);

				print(a + -2); print(-2 + a); print(b + -2); print(-2 + b);
				print(a + -1); print(-1 + a); print(b + -1); print(-1 + b);
				print(a + -0); print(-0 + a); print(b + -0); print(-0 + b);
				print(a +  1); print( 1 + a); print(b +  1); print( 1 + b);
				print(a +  2); print( 2 + a); print(b +  2); print( 2 + b);
			}
		}
	}

	static void div()
	{
		for (int i = -10; i <= 10; ++i)
		{
			for (int j = 10; j >= -10; --j)
			{
				int a = i, b = j;
				if (0 != b) print(a / b);

				print(a / -2); if (0 != a) print(-2 / a); print(b / -2); if (0 != b) print(-2 / b);
				print(a / -1); if (0 != a) print(-1 / a); print(b / -1); if (0 != b) print(-1 / b);
				               if (0 != a) print(-0 / a);                if (0 != b) print(-0 / b);
				print(a /  1); if (0 != a) print( 1 / a); print(b /  1); if (0 != b) print( 1 / b);
				print(a /  2); if (0 != a) print( 2 / a); print(b /  2); if (0 != b) print( 2 / b);
			}
		}
	}

	static void mod()
	{
		for (int i = -10; i <= 10; ++i)
		{
			for (int j = 10; j >= -10; --j)
			{
				int a = i, b = j;
				if (0 != b) print(a % b);

				print(a % -2); if (0 != a) print(-2 % a); print(b % -2); if (0 != b) print(-2 / b);
				print(a % -1); if (0 != a) print(-1 % a); print(b % -1); if (0 != b) print(-1 % b);
				               if (0 != a) print(-0 % a);                if (0 != b) print(-0 % b);
				print(a %  1); if (0 != a) print( 1 % a); print(b %  1); if (0 != b) print( 1 % b);
				print(a %  2); if (0 != a) print( 2 % a); print(b %  2); if (0 != b) print( 2 % b);
			}
		}
	}

	static void mul()
	{
		for (int i = -10; i <= 10; ++i)
		{
			for (int j = 10; j >= -10; --j)
			{
				int a = i, b = j;
				print(a * b);

				print(a * -2); print(-2 * a); print(b * -2); print(-2 * b);
				print(a * -1); print(-1 * a); print(b * -1); print(-1 * b);
				print(a * -0); print(-0 * a); print(b * -0); print(-0 * b);
				print(a *  1); print( 1 * a); print(b *  1); print( 1 * b);
				print(a *  2); print( 2 * a); print(b *  2); print( 2 * b);
			}
		}
	}

	static void sub()
	{
		for (int i = -10; i <= 10; ++i)
		{
			for (int j = 10; j >= -10; --j)
			{
				int a = i, b = j;
				print(a - b);

				print(a - -2); print(-2 - a); print(b - -2); print(-2 - b);
				print(a - -1); print(-1 - a); print(b - -1); print(-1 - b);
				print(a - -0); print(-0 - a); print(b - -0); print(-0 - b);
				print(a -  1); print( 1 - a); print(b -  1); print( 1 - b);
				print(a -  2); print( 2 - a); print(b -  2); print( 2 - b);
			}
		}
	}

	public static void Main()
	{
		add();
		div();
		mod();
		mul();
		sub();
	}
}
