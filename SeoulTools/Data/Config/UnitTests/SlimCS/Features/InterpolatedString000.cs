using static SlimCS;
public static class Test
{
	public static int A()
	{
		print("Called A");
		return 5;
	}

	public static int B()
	{
		print("Called B");
		return 1;
	}

	public class C {}
	public static C D()
	{
		print("Called D");
		return new C();
	}

	public static void Main()
	{
		string s;
		int res = 5;

		s = $"Result {res}"; print(s);
		s = $"Result {   res   }  "; print(s);
		s = $"Result { res, 7 }"; print(s);
		s = $"Result { res, -57 }"; print(s);
		s = $""; print(s);
		s = $"Result { res } { res }++"; print(s);
		s = $"Result {{ res }} { res }"; print(s);
		s = $"Result { res /* foo */ }"; print(s);
		s = $"{{0}}"; print(s);
		s = $"{ $"{ res }" }"; print(s);
		s = $" \u004d "; print(s);
		s = $@"   {A()}"; print(s);
		s = $@" {B()}  {A()}"; print(s);
		s = $@" {A()} {B()} {A()}"; print(s);
		s = $@"  {D()} {B()} {A()}"; print(s);
		s = $@"  {A()} {D()} {B()}"; print(s);
		s = $@"  {A()} {B()} {D()}"; print(s);
	}
}
