using static SlimCS;
namespace A.B
{
	public static class G<T>
	{
		public class DD
		{
		}

		public static object Dock () => null;
	}
}

namespace N2
{
	using static A.B.G<int>;

	class Test : DD
	{
		public static void Main ()
		{
			print(Dock ());
		}
	}
}
