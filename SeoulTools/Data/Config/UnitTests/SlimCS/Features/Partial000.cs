using static SlimCS;
namespace A
{
	public partial class B
	{
		void Three(bool b)
		{
			print(b);
			Four("Hello World");
		}

		public static void One()
		{
			print("One");
			Two(2);
		}
	}
}

public static class Test
{
	public static void Main()
	{
		A.B.One();
	}
}
