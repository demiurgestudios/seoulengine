using static SlimCS;
namespace A
{
	public class T
	{
		public class N
		{

		}
	}
}

namespace B
{
	using static A.T;

	static class Test
	{
		static void Main ()
		{
			var t = typeof (N);
			print(t);
			var u = new N ();
			print(u);
		}
	}
}
