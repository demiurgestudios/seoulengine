using static SlimCS;
public static class Test
{
	// This is a regression - the result of
	// evaluating a null inside a string interpolation
	// should be the empty string, which means
	// we need to wrap all arguments (even strings)
	// in a tostring() call, since lua will incorrectly
	// trigger a null access error otherwise.
	public static void Main()
	{
		string s = null;

		// Working around the fact that we allow C# to
		// differ from Lua in this regard (in Lua/SlimCS,
		// this will become 'nil' while in stock C# it is '').
		string sEval = $"{s}";
		if (sEval == "nil") { sEval = ""; }

		print($"This value is null: " + sEval);
	}
}
