//Compiler options: -warnaserror -warn:4

using System;
interface IFoo
{
}

class Bar
{
}

class Program
{
	public static void Main()
	{
		IFoo foo = null;
		if (foo is IFoo)
			SlimCS.print("got an IFoo"); // never prints
			
		Bar bar = null;
		if (bar is Bar)
			SlimCS.print("got a bar"); // never prints
	}
}
