using static SlimCS;
namespace A
{
	public partial class B
	{
		public static void Two(int i)
		{
			print(i);
			(new B()).Three(false);
		}
	}
}

