using static SlimCS;

public static class Utility
{
	public const double k9x16 = 9.0 / 16.0;
	public const double k2x3 = 2.0 / 3.0;
	public const double k3x4 = 3.0 / 4.0;
	public const double k16x9 = 16.0 / 9.0;

	public static double GetNearestAspectRatio(double fAspectRatio)
	{
		if (fAspectRatio <= (k9x16 + k2x3) / 2.0) { return k9x16; }
		else if (fAspectRatio <= (k2x3 + k3x4) / 2.0) { return k2x3; }
		else if (fAspectRatio <= k3x4 + 0.1) { return k3x4; }
		else if (fAspectRatio <= k16x9 + 0.1) { return k16x9; }
		else
		{
			return fAspectRatio;
		}
	}
}

public static class Test
{
	static void Print(double fAspectRatio)
	{
		switch (Utility.GetNearestAspectRatio(fAspectRatio))
		{
			case Utility.k2x3:
				print("2x3");
				break;
			case Utility.k9x16:
				print("9x16");
				break;
			case Utility.k3x4:
				print("3x4");
				break;
			case Utility.k16x9:
				print("16x9");
				break;
			default:
				print(fAspectRatio);
				break;
		}
	}

	public static void Main()
	{
		Print(0.5);
		Print(0.6);
		Print(0.7);
		Print(0.8);
		Print(0.9);
		Print(1.0);
		Print(1.1);
		Print(1.2);
		Print(1.3);
		Print(1.4);
		Print(1.5);
		Print(1.6);
		Print(1.7);
	}
}
