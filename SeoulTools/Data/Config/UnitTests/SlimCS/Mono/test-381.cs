public class Application
{
	public static void Main(string[] args)
	{
		if (true)
		{
			string thisWorks = "nice";
			SlimCS.print(thisWorks);
		}
		else
		{
			string thisDoesnt;
			SlimCS.print();
			thisDoesnt = "not so";
			SlimCS.print(thisDoesnt);
		}
	}
}

