using static SlimCS;

public static class Test
{
	public static (double, double, double) t(double a, double b, double c)
	{
		return (a, b, c);
	}

	public static void Main()
	{
		{
			(var a, var b, var c) = t(1.0, 2.0, 3.0);
			print(a, b, c);
			print(_G._);
		}

		{
			(var a, var b, _) = t(1.0, 2.0, 3.0);
			print(a, b);
			print(_G._);
		}

		{
			(var a, _, var c) = t(1.0, 2.0, 3.0);
			print(a, c);
			print(_G._);
		}

		{
			(var _, var b, var c) = t(1.0, 2.0, 3.0);
			print(b, c);
			print(_G._);
		}

		{
			(var _, var _, var c) = t(1.0, 2.0, 3.0);
			print(c);
			print(_G._);
		}

		{
			(var a, var _, var _) = t(1.0, 2.0, 3.0);
			print(a);
			print(_G._);
		}

		{
			(var _, var b, var _) = t(1.0, 2.0, 3.0);
			print(b);
			print(_G._);
		}

		{
			(var _, var _, var _) = t(1.0, 2.0, 3.0);
			print(_G._);
		}
	}
}
