using static SlimCS;
using System;
using static TestClass;

internal class Test
{
	public static void Main (string[] args)
	{
		var res = Directions.Up;
		print(res);
	}
}

public enum Directions
{
	Up,
	NotUp,
}

public static class TestClass
{
	public static int Directions;
}
