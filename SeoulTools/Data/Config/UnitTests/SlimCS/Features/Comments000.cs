using static SlimCS;
public static class Test
{
	/** Test that various lua characters [[
	 ** do not ]] cause problems on conversion
	 ** --
	 ** --[[
	 ** --]]
	 */
	public static void Main()
	{
		print("Hello World");
	}

	// -- [[ ]] --[[ --]]
	static void More()
	{
	}

	/// <summary>
	/// [[ ]] -- --[[ --]]
	/// </summary>
	static void EvenMore()
	{
	}
}
