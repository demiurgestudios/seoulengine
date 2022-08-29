using static SlimCS;
interface C
{
	int H();
}

class A : C
{
	public int F()
	{
		return 1;
	}

	public virtual int G()
	{
		return 2;
	}

	public int H()
	{
		return 10;
	}
}

class B : A
{
	public new int F()
	{
		return 3;
	}

	public override int G()
	{
		return 4;
	}

	public new int H()
	{
		return 11;
	}
}

public static class Test
{
	public static void Main()
	{
		int result = 0; print(result);
		B b = new B (); print(b);
		A a = b; print(a);
		if (a.F () != 1)
		{
			result |= 1 << 0;
		}
		print(result);

		if (b.F () != 3)
		{
			result |= 1 << 1;
		}
		print(result);

		if (b.G () != 4)
		{
			result |= 1 << 2;
		}
		print(result);

		if (a.G () != 4)
		{
			print("more: " + a.G ());
			result |= 1 << 3;
		}
		print(result);

		if (a.H () != 10)
		{
			result |= 1 << 4;
		}
		print(result);

		if (b.H () != 11)
		{
			result |= 1 << 5;
		}
		print(result);

		if (((A)b).H () != 10)
		{
			result |= 1 << 6;
		}
		print(result);

		if (((B)a).H () != 11)
		{
			result |= 1 << 7;
		}
		print(result);
	}
}
