using static SlimCS;
class Test
{
	int var;
	public Test(int var, int i)
	{
		print(var, i);
		var = i;
		print(var, i);
	}

	public static void Main ()
	{
		new Test(0, 1);
	}
}
