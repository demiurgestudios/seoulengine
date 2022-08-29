using System;
using System.Collections.Generic;

interface IFoo
{
	void Bar();
	IList<T> Bar<T>();
}

class Foo : IFoo
{
	public void Bar()
	{
		SlimCS.print("Bar");
	}
	
	public IList<T> Bar<T>()
	{
		SlimCS.print("Bar<T>");
		return null;
	}
}

class BugReport
{
	public static void Main(string[] args)
	{
		Foo f = new Foo();
		f.Bar();
		f.Bar<int>();
	}
}


