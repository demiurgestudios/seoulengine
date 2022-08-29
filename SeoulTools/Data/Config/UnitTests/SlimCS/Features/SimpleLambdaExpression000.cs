using static SlimCS;

// Regression for a case where returning an anonymous lambda
// produces invalid Lua code.
public static class Test
{
	public class Argument
	{
	}

	public class Filter
	{
		public delegate bool ArgDelegate(Argument arg);
		public static ArgDelegate MakeFilter()
		{
			return arg => true;
		}
	}

	public static void Main()
	{
		var del = Filter.MakeFilter();
		var b = del(null);
		print(b);
	}
}
