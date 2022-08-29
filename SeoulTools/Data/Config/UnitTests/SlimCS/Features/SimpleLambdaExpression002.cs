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
		public delegate void ArgDelegate(Argument arg);

		public static void DoIt()
		{
			print("Hello World");
		}

		public static ArgDelegate MakeFilter()
		{
			return arg => DoIt();
		}
	}

	public static void Main()
	{
		var del = Filter.MakeFilter();
		del(null);
	}
}
