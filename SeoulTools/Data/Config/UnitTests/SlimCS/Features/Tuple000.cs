using static SlimCS;

public static class Test
{
	public static void Main()
	{
		{
			(var a, var b, var c) = (1.0, 2.0, 3.0);
			print(a, b, c);
			print(_G._);
		}

		{
			(var a, var b, _) = (1.0, 2.0, 3.0);
			print(a, b);
			print(_G._);
		}

		{
			(var a, _, var c) = (1.0, 2.0, 3.0);
			print(a, c);
			print(_G._);
		}

		{
			(var _, var b, var c) = (1.0, 2.0, 3.0);
			print(b, c);
			print(_G._);
		}

		{
			(var _, var _, var c) = (1.0, 2.0, 3.0);
			print(c);
			print(_G._);
		}

		{
			(var a, var _, var _) = (1.0, 2.0, 3.0);
			print(a);
			print(_G._);
		}

		{
			(var _, var b, var _) = (1.0, 2.0, 3.0);
			print(b);
			print(_G._);
		}

		{
			(var _, var _, var _) = (1.0, 2.0, 3.0);
			print(_G._);
		}
	}
}
