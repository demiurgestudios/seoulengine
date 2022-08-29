using static SlimCS;

public static class Test
{
	public static void Main()
	{
		var t = new Table()
		{
			[1] = new Table()
				{
					[1] = new Table() { ["Name"] = "Test1" },
				},
			[2] = new Table()
				{
					[1] = new Table() { ["Name"] = "Test2" },
				},
			[3] = new Table()
				{
					[1] = new Table() { ["Name"] = "Test3" },
				},
		};
		
		print(((Table)((Table)t[1])[1])["Name"]);
		print(((Table)((Table)t[2])[1])["Name"]);
		print(((Table)((Table)t[3])[1])["Name"]);
		print(t[4]);
	}
}
