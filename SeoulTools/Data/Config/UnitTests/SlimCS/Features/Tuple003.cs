using static SlimCS;

public static class Test
{
	public static (double, double, double) t(double a, double b, double c)
	{
		return (a, b, c);
	}

	public static void Main()
	{
		double a = 5.0;
		double b = 6.0;
		double c = 7.0;

		(a, b, c) = t(1.0, 2.0, 3.0);
		print(a, b, c);
		print(_G._);

		(a, b, _) = t(10.0, 11.0, 12.0);
		print(a, b, c);
		print(_G._);

		(a, _, c) = t(13.0, 14.0, 15.0);
		print(a, b, c);
		print(_G._);

		(_, b, c) = t(16.0, 17.0, 18.0);
		print(a, b, c);
		print(_G._);

		(_, _, c) = t(19.0, 20.0, 21.0);
		print(a, b, c);
		print(_G._);
		(a, _, _) = t(22.0, 23.0, 24.0);
		print(a, b, c);
		print(_G._);
		(_, b, _) = t(25.0, 26.0, 27.0);
		print(a, b, c);
		print(_G._);

		(_, _, _) = t(28.0, 29.0, 30.0);
		print(a, b, c);
		print(_G._);
	}
}
