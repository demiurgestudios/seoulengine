using static SlimCS;
public static class Test
{
	// Test for regression case, static property with setter incorrectly invoked.
	static int s_Value;
	public static int Value
	{
		get
		{
			return s_Value;
		}

		set
		{
			print(value);
			s_Value = value;
			print(s_Value);
		}
	}

	public static void Main()
	{
		for (var i = 0; i < 100; ++i)
		{
			print(Value);
			Value = i;
			print(Value);
		}
	}
}
