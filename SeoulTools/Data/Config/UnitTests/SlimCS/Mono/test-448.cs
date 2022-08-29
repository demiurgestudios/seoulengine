using System;

public class MonoDivideProblem
{
	static int dividend = unchecked((int)0x80000000);
	static int divisor = 1;
	public static void Main(string[] args)
	{
		SlimCS.print("Dividend/Divisor = {0}", dividend/divisor);
	}

}
